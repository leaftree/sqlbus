
/**
 * log_watcher.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-09-12 14:52:25
 * Last Modified : 2018-09-12 14:52:25
 */

#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>

#include "log.h"
#include "setproctitle.h"

#define EVENT_TIMER_INTERVAL 1

#define LOG_FILE_SIZE_DIFFER_RANGE 128

#define EVENT_TYPE_INOTIFY 0
#define EVENT_TYPE_TIMER   1

struct event_record_set
{
	int fd;
	int type;
};

static struct epoll_event evgroup[1024];
static struct event_record_set Event_set[1024];

inline static void add_event_record_to_set(int fd, int type)
{
	Event_set[fd].fd = fd;
	Event_set[fd].type = type;
}

inline static int get_event_type(int fd)
{
	return Event_set[fd].type;
}

static char *path_join(char *s1, char *s2)
{
	static char buf[1024];
	memset(buf, 0x0, sizeof buf);
	sprintf(buf, "%s/%s", s1, s2);
	return buf;
}

static int event_ctl(int epfd, int opt, int notifyfd)
{
	struct epoll_event ev = {
		.events = EPOLLIN,
		.data.fd = notifyfd,
	};
	return epoll_ctl(epfd, opt, notifyfd, &ev);
}

static int event_add(int epfd, int notifyfd)
{
	return event_ctl(epfd, EPOLL_CTL_ADD, notifyfd);
}

/*
static int event_del(int epfd, int notifyfd)
{
	return event_ctl(epfd, EPOLL_CTL_ADD, notifyfd);
}
*/

static int add_notify_watch(int notifyfd, char *file)
{
	int event = IN_ATTRIB|IN_DELETE|IN_DELETE_SELF|IN_MOVE|IN_MOVE_SELF;
	return(inotify_add_watch(notifyfd, file, event));
}

static int del_notify_watch(int notifyfd, int wd)
{
	return(inotify_rm_watch(notifyfd, wd));
}

static int file_inode_changed(char *file, int ino)
{
	struct stat fStat;
	return (stat(file, &fStat) !=0 || fStat.st_ino != ino);
}

//
// do_notify_event -
//   If receive event MOVE, or delete, it will re-create the log file,
//   If receive event Attrib, it will check the real file exist or not.
//   Sqlbus does not close log file fd during running, so log watcher will not
//   receive the delete event from kernel, but can receive attrib event.
//
//   Surely the log file was deleted by somenoe or by timer for logfile backup
//   event, @do_notify_event will re-create log file immediately.
//
//   TODO: fixup bug
//     There is a bug, but I can't find it out. The bug appears in the following
//   operation:
//     $ mv app.log app.log.bak && touch a.log app.log && mv app.log.bak app.log
static int do_notify_event(int size, char *buf, int epfd, int notifyfd, sqlbus_log_t *meta)
{
	int reopen = 0;
	char *ptr = NULL;
	struct inotify_event *iev = NULL;

	for(ptr=buf; ptr<buf+size; ptr+=(sizeof(struct inotify_event)+iev->len))
	{
		iev = (struct inotify_event*)ptr;
		int mask = iev->mask;

		if(mask & IN_ATTRIB)
		{
			if(file_inode_changed(path_join(meta->catalog, meta->file), meta->inode)) {
				reopen = 1;
				break;
			}
		}
		else if((mask & IN_DELETE)  || (mask & IN_DELETE_SELF) ||
				(mask & IN_MOVE_SELF)   || (mask & IN_MOVED_FROM)  || (mask & IN_MOVED_TO)) {
			reopen = 1;
			break;
		}
	}

	if(reopen==1) {
		pthread_mutex_lock(&meta->mutex);
		del_notify_watch(notifyfd, iev->wd);
		fsync(meta->fd);
		close(meta->fd);
		reopen_log_file(meta);
		add_notify_watch(notifyfd, path_join(meta->catalog, meta->file));
		pthread_mutex_unlock(&meta->mutex);
	}
	return 0;
}

inline static int find_max_suffix(char *path, char *file)
{
	int suffix = 0;

	DIR *dir = opendir(path);
	if(dir==NULL) {
		return(-1);
	}

	struct dirent *d = NULL;
	while((d=readdir(dir))!=NULL) {
		if(strncmp(d->d_name, file, strlen(file))==0) {
			char *ptr = rindex(d->d_name, '.');
			if(ptr && (ptr+1) && isdigit(*(ptr+1))) {
				int tv = atoi(ptr+1);
				if(tv>suffix) {
					suffix=tv;
				}
			}
		}
	}
	closedir(dir);

	return(suffix);
}

inline static void backup(sqlbus_log_t *meta)
{
	if(!meta) {
		return;
	}

	char src[512] = "", obj[512] = "";
	char *file = meta->file;
	char *path = meta->catalog;
	int i, maxfix = find_max_suffix(path, file);
	maxfix = maxfix < 0 ? 0 : maxfix;

	for(i=maxfix; i>=0; i--) {
		sprintf(src, "%s/%s", path, file);
		sprintf(obj, "%s/%s.%d", path, file, i+1);
		if(i!=0) {
			sprintf(src+strlen(src), ".%d", i);
		}
		pthread_mutex_lock(&meta->mutex);
		fsync(meta->fd);
		rename(src, obj);
		pthread_mutex_unlock(&meta->mutex);
	}
}

/**
 * timestamp_from_midnight - get the timestamp begin from midnight
 *
 * return value:
 *   The Seconds far from midnight
 */
static int timestamp_from_midnight()
{
	time_t tnow = time(NULL);
	struct tm *timeptr = gmtime(&tnow);
	timeptr->tm_sec = 0;
	timeptr->tm_min = 0;
	timeptr->tm_hour = 0;

	time_t tmn = mktime(timeptr);
	return tnow - tmn;
}

//
// do_timer_event -
//
//  Only do one thing: rename the log file, doesn't close fd, doesn't re-create
//  the log file, because inotify-event will do this.
//
static int do_timer_event(int size, char *buf, int epfd, int evfd, sqlbus_log_t *meta)
{
	struct stat fStat;
	if(stat(path_join(meta->catalog, meta->file), &fStat)<0) {
		return(-1);
	}

	if(fStat.st_size >= meta->maxsize ||
			abs(fStat.st_size - meta->maxsize) < LOG_FILE_SIZE_DIFFER_RANGE ||
			abs(timestamp_from_midnight()-meta->cron) < EVENT_TIMER_INTERVAL) {
		backup(meta);
	}

	return(0);
}

static int do_watcher(int epfd, sqlbus_log_t *meta)
{
	int i = 0;
	int epret = epoll_wait(epfd, evgroup, 1024, 100);
	if(epret <= 0) {
		return(epret);
	}

	for(i=0; i <epret; i++)
	{
		char ebuf[1024*8] = "";
		int size = read(evgroup[i].data.fd, ebuf, sizeof(ebuf));
		if(size<=0) {
			continue;
		}

		if(get_event_type(evgroup[i].data.fd) == EVENT_TYPE_INOTIFY) {
			do_notify_event(size, ebuf, epfd, evgroup[i].data.fd, meta);
		}

		if(get_event_type(evgroup[i].data.fd) == EVENT_TYPE_TIMER) {
			do_timer_event(size, ebuf, epfd, evgroup[i].data.fd, meta);
		}
	}
	return epret;
}

//
// log_op_watcher_loop - log operations watcher loop
//
// @argv: global log handler
//
// @log_op_watcher_loop do two things:
//   1. watch the log file deleted
//   2. watch the log file need to be backed up
//
// return value:
//   NULL:
void *log_op_watcher_loop(void *argv)
{
	if(!argv) {
		return (void*)NULL;
	}
	set_process_name(SQLBUS_LOG_WATCHER_THREAD_NAME);

	sqlbus_log_t *logger = (sqlbus_log_t*)argv;
	int epfd = epoll_create(1024);

	int notifyfd = inotify_init();
	event_add(epfd, notifyfd);
	add_event_record_to_set(notifyfd, EVENT_TYPE_INOTIFY);
	add_notify_watch(notifyfd, path_join(logger->catalog, logger->file));

	int timerfd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC|TFD_NONBLOCK);
	if(timerfd<0) {
		LOG_ERROR(logger, "timerfd_create() failed for %s", strerror(errno));
		return(NULL);
	}
	struct timespec now;
	if(clock_gettime(CLOCK_REALTIME, &now)<0) {
		return(NULL);
	}
	struct itimerspec tspec;
	tspec.it_value.tv_sec = now.tv_sec + EVENT_TIMER_INTERVAL;
	tspec.it_value.tv_nsec = now.tv_nsec;
	tspec.it_interval.tv_sec = EVENT_TIMER_INTERVAL;
	tspec.it_interval.tv_nsec = 0;
	if(timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &tspec, NULL)<0) {
		return(NULL);
	}
	event_add(epfd, timerfd);
	add_event_record_to_set(timerfd, EVENT_TYPE_TIMER);

	while(1) {

		do_watcher(epfd, logger);

	}

	return(NULL);
}
