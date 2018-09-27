
/**
 * hisqlbus.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-04-10 09:34:14
 * Last Modified : 2018-04-10 09:34:14
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/param.h>

#include "util.h"
#include "hisqlbus.h"

#ifdef mFree
# undef mFree
#endif
# define mFree(ptr) do { if(ptr) { free(ptr); ptr = NULL; } }while(0)

const int defaultMemcachePort = 6379;
const int defaultErrMsgcapacity = 4096;
const int defaultMemcacheConnectTimeOut = 5;
const int defaultMemcacheWaitResponseTimeOut = 50;

const char defaultMemcacheHost[] = "127.0.0.1";
const char defaultMemcacheSendQueue[] = "SqlBusDefaultQueue";

// Connect to memcache server(redis-server)
static int mcc_conn(sqlbus *bus);

// Authentication memcache connection
static int mcc_auth(sqlbus *bus);

// Recveive response from memcache server
static int mcc_recv(sqlbus *bus);

// Discon connection from memcache server
static int mcc_close(sqlbus *bus);

// Select a database instance of memcache server
static int mcc_select(sqlbus *bus, char *instance);

// Send a request to memcache server
static int mcc_send(sqlbus *bus, void *data, int sync);

// Send a request with update and insert sql statement to memcache server
static int mcc_ui_send(sqlbus *bus, char *update, char *insert);

	// Judge request is normal or not
static int reader_perfect(struct sqlbus *bus);

// Get columns counter
static int reader_fcount(struct sqlbus *bus, int *cnt);

// Get columns(field) name by index
static int reader_fname(struct sqlbus *bus, int idx, char **fname);

// Get row counter
static int reader_rcount(struct sqlbus *bus, int *cnt);

// Get a value by row index(same as the number of record) and field name
static int reader_rvalue(struct sqlbus *bus, int ridx, char *fname, char **value);

// Get a value by row index and field index
static int reader_rvalue_idx(struct sqlbus *bus, int ridx, int cidx, char **value);

//  Finishing result set read, release the memory resource
static int reader_finalize(struct sqlbus *bus);

// Used to set a error message
static int serror(errorInfo *err, int etag, const char *fmt, ...);

// Default memcache operated function
static sqlbusMCOperFunctions defaultMCOFunctions = {
	.conn   = mcc_conn,
	.auth   = mcc_auth,
	.recv   = mcc_recv,
	.close  = mcc_close,
	.select = mcc_select,
	.send   = mcc_send,
	.uisend  = mcc_ui_send,
};

// Default reader function for biz tranp
static sqlbusReaderFunctions defaultRFunctions = {
	.perfect  = reader_perfect,
	.fcount   = reader_fcount,
	.fname    = reader_fname,
	.rcount   = reader_rcount,
	.value    = reader_rvalue,
	.valueIdx = reader_rvalue_idx,
	.free     = reader_finalize,
};

void *sqlbusCreate(uint16_t port, char *host, char *auth)
{
	char *err = NULL;
	char *sendQueue = NULL;
	struct sqlbus *bus = NULL;
	struct sqlbusMCClient *mc = NULL;
	struct SerializationProtocol *proto = NULL;

	err = malloc(defaultErrMsgcapacity);
	bus = malloc(sizeof(struct sqlbus));
	mc = malloc(sizeof(struct sqlbusMCClient));
	proto = malloc(sizeof(struct SerializationProtocol));
	sendQueue = strdup(defaultMemcacheSendQueue);

	if(!bus || !mc || !err || !proto || !sendQueue) {
		mFree(bus);
		mFree(mc);
		mFree(err);
		mFree(proto);
		mFree(sendQueue);
		return(NULL);
	}

	mc->port = port?port:defaultMemcachePort;
	sprintf(mc->host, "%s", host?host:defaultMemcacheHost);
	sprintf(mc->perm, "%s", auth?auth:"");
	*mc->instance = 0x0;
	mc->sendQueue = sendQueue;
	mc->recvQueue = NULL;

	bus->mc = mc;
	bus->mc->redis = NULL;
	bus->proto = proto;
	bus->ofn = &defaultMCOFunctions;
	bus->rfn = &defaultRFunctions;

	bus->error.etag = 0;
	bus->error.errmsg = err;
	bus->error.capacity = defaultErrMsgcapacity;

	bus->proto->response.status = -1;
	bus->proto->request.requestStatement.ui.update = NULL;
	bus->proto->request.requestStatement.ui.insert = NULL;
	bus->proto->request.requestStatement.statement = NULL;

	return(bus);
}

void sqlbusFinalize(sqlbus *bus)
{
	if(!bus || !bus->ofn) {
		return;
	}

	if(bus->mc->redis)
		bus->ofn->close(bus);

	if(bus->mc) {
		mFree(bus->mc->sendQueue);
		mFree(bus->mc->recvQueue);
		mFree(bus->mc);
	}

	mFree(bus->error.errmsg);
	mFree(bus->proto);
	mFree(bus);

	return;
}

static int resize_error_buffer(errorInfo *err, int new_size)
{
	if(!err) {
		return(FAILURE);
	}

	char *ptr = malloc(new_size);
	if(!ptr) {
		return(FAILURE);
	}
	mFree(err->errmsg);
	err->errmsg = ptr;
	err->capacity = new_size;

	return(SUCCESS);
}

static int serror(errorInfo *err, int etag, const char *fmt, ...)
{
	printf("-------------------------------\n");
	int msglen = 0;
	char errmsg[16*1024] = "";

	if(!err || !err->errmsg || !fmt) {
		return(FAILURE);
	}

	va_list ap;
	va_start(ap, fmt);
	msglen = vsnprintf(errmsg, 16*1024, fmt, ap);
	va_end(ap);

	if(msglen >= err->capacity) {
		resize_error_buffer(err, roundup(msglen, 1024));
	}
	snprintf(err->errmsg, err->capacity, "%s", errmsg);
	printf("%s\n", err->errmsg);
	err->etag = etag;

	return(SUCCESS);
}

static int mcc_conn(sqlbus *bus)
{
	redisContext *rc = NULL;

	struct timeval tv = {
		.tv_sec = defaultMemcacheConnectTimeOut,
		.tv_usec = 0,
	};

	if(!bus || !bus->mc) {
		return(FAILURE);
	}

	rc = redisConnectWithTimeout(bus->mc->host, bus->mc->port, tv);
	if(rc == NULL) {
		serror(&bus->error, 1, "Connect redis failed. Host[%s] Port(%d)", bus->mc->host, bus->mc->port);
		return(FAILURE);
	}
	else if(rc->err) {
		serror(&bus->error, 1, "Connect redis failed. Host[%s] Port(%d):%s", bus->mc->host, bus->mc->port, rc->errstr);
		redisFree(rc);
		return(FAILURE);
	}

	bus->mc->redis = rc;
	return(SUCCESS);
}

static int mcc_auth(sqlbus *bus)
{
	static const char ok[] = "OK";
	static const char invalid[] = "invalid password";
	static const char notset[] = "no password is set";

	int retval = 0;
	redisReply *reply = NULL;

	if(!bus) {
		return(FAILURE);
	}

	if(!bus->mc || !bus->mc->redis)
	{
		serror(&bus->error, 1, "Client handle does not init.");
		return(FAILURE);
	}

	reply = redisCommand(bus->mc->redis, "AUTH %s", bus->mc->perm);
	if(!reply)
	{
		serror(&bus->error, 1, "Authentication failure for unknow reasone.");
		return(FAILURE);
	}
	else
	{
		if(reply->type == REDIS_REPLY_STATUS && !strcasecmp(reply->str, ok)) {
			retval = SUCCESS;
		}
		else if(reply->type == REDIS_REPLY_ERROR)
		{
			if(strstr(reply->str, invalid)) {
				retval = FAILURE;
			}
			else if(strstr(reply->str, notset)) {
				retval = SUCCESS;
			}
		}
		freeReplyObject(reply);
	}

	return(retval);
}

static int mcc_recv(sqlbus *bus)
{
	redisReply *reply = NULL;

	if(!bus) {
		return(FAILURE);
	}

	if(!bus->mc || !bus->mc->redis) {
		serror(&bus->error, 1, "Client handle does not init.");
		return(FAILURE);
	}

	if(!bus->mc->recvQueue) {
		serror(&bus->error, 1, "Not set receive queue name.");
		return(FAILURE);
	}

	reply = redisCommand(bus->mc->redis, "BRPOP %s %d",
			bus->mc->recvQueue, defaultMemcacheWaitResponseTimeOut);
	if(!reply)
	{
		serror(&bus->error, 1, "Read message queue[%s] failure for unknow reasone.", bus->mc->recvQueue);
		return(FAILURE);
	}
	else
	{
		if(reply->type != REDIS_REPLY_ARRAY || reply->elements != 2) {
			if(reply->type == REDIS_REPLY_NIL) {
				serror(&bus->error, 1, "Read message queue[%s] failure for no message", bus->mc->recvQueue);
			}
			else {
				serror(&bus->error, 1, "Read message queue[%s] failure for %s", bus->mc->recvQueue, reply->str);
			}
			freeReplyObject(reply);
			bus->proto->response.status = 0;
			return(FAILURE);
		}

#ifdef HI_SQL_BUS_DEBUG_MAIN
		bus->proto->response.originData = strdup(reply->element[1]->str);
#endif

		if(unMarshallJson(bus, reply->element[1]->str)) {
			bus->proto->response.status = 0;
			freeReplyObject(reply);
			return(FAILURE);
		}
	}
	freeReplyObject(reply);

	bus->proto->response.status = 1;

	return(SUCCESS);
}

static int mcc_close(sqlbus *bus)
{
	if(!bus || !bus->mc || !bus->mc->redis)
		return(FAILURE);

	redisFree(bus->mc->redis);
	bus->mc->redis = NULL;
	return(SUCCESS);
}

static int mcc_select(sqlbus *bus, char *instance)
{
	if(!bus || !bus->mc || !bus->mc->redis || !instance) {
		return(FAILURE);
	}

	snprintf(bus->mc->instance, sizeof(bus->mc->instance)-2, "%s", instance);

	redisReply *reply = redisCommand(bus->mc->redis, "SELECT %s", bus->mc->instance);
	if(reply == NULL || reply->type != REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR)
	{
		if(reply) {
			serror(&bus->error, 1, "Select memcache server instance failure.%s", reply->str);
		}
		else {
			serror(&bus->error, 1, "Select memcache server instance failure.");
		}
		freeReplyObject(reply);
		return(FAILURE);
	}

	return(SUCCESS);
}

static int mcc_send(sqlbus *bus, void *data, int sync)
{
	int retval = SUCCESS;
	char *requestString = NULL;
	redisReply *reply = NULL;
	sqlbusRequest *request = NULL;

	if(!bus || !data) {
		return(FAILURE);
	}

	if(!bus->mc || !bus->mc->redis ) {
		serror(&bus->error, 1, "Client handle does not init.");
		return(FAILURE);
	}

	if(sync != EXEC_SYNC && sync != EXEC_ASYNC) {
		serror(&bus->error, 1, "@sync must be EXEC_SYNC or EXEC_ASYNC.");
		return(FAILURE);
	}

	request = &bus->proto->request;

	if(getUuid(&request->universalUniqueIdentification)) {
		serror(&bus->error, 1, "Get uuid from system failure.");
		return(FAILURE);
	}

	getPid(&request->processIdentification);
	getAppName(&request->clientName);
	getTimestamp(&request->timestamp);

	request->synchronizationType = sync;
	request->messageType = strdup("request");
	request->requestStatement.statement = strdup(data);
	request->receiveChannel = strdup(request->universalUniqueIdentification);

	if(marshallJson(bus, &requestString)) {
		retval = FAILURE;
		serror(&bus->error, 1, "marshallJson failure.");
		goto err;
	}

	reply = redisCommand(bus->mc->redis, "LPUSH %s %s", bus->mc->sendQueue, requestString);
	if(reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if(reply && reply->str)
			serror(&bus->error, 1, "LPUSH failure:%s", reply->str);
		else
			serror(&bus->error, 1, "LPUSH failure.");
		freeReplyObject(reply);
		retval = FAILURE;
		goto err;
	}
	freeReplyObject(reply);

	bus->mc->recvQueue = strdup(request->receiveChannel);

err:
	mFree(request->clientName);
	mFree(request->messageType);
	mFree(request->receiveChannel);
	mFree(request->requestStatement.statement);
	mFree(request->universalUniqueIdentification);

	return(retval);
}

static int mcc_ui_send(sqlbus *bus, char *update, char *insert)
{
	int retval = SUCCESS;
	char *requestString = NULL;
	redisReply *reply = NULL;
	sqlbusRequest *request = NULL;

	if(!bus || !update || !insert) {
		return(FAILURE);
	}

	if(!bus->mc || !bus->mc->redis ) {
		serror(&bus->error, 1, "Client handle does not init.");
		return(FAILURE);
	}

	request = &bus->proto->request;

	if(getUuid(&request->universalUniqueIdentification)) {
		serror(&bus->error, 1, "Get uuid from system failure.");
		return(FAILURE);
	}

	getPid(&request->processIdentification);
	getAppName(&request->clientName);
	getTimestamp(&request->timestamp);

	request->synchronizationType = EXEC_ASYNC;
	request->messageType = strdup("request");
	request->requestStatement.ui.update = strdup(update);
	request->requestStatement.ui.insert = strdup(insert);
	request->receiveChannel = strdup(request->universalUniqueIdentification);

	if(marshallJson(bus, &requestString)) {
		retval = FAILURE;
		serror(&bus->error, 1, "marshallJson failure.");
		goto err;
	}

	reply = redisCommand(bus->mc->redis, "LPUSH %s %s", bus->mc->sendQueue, requestString);
	if(reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if(reply && reply->str)
			serror(&bus->error, 1, "LPUSH failure:%s", reply->str);
		else
			serror(&bus->error, 1, "LPUSH failure.");
		freeReplyObject(reply);
		retval = FAILURE;
		goto err;
	}
	freeReplyObject(reply);

	bus->mc->recvQueue = strdup(request->receiveChannel);

err:
	mFree(request->clientName);
	mFree(request->messageType);
	mFree(request->receiveChannel);
	mFree(request->universalUniqueIdentification);
	mFree(request->requestStatement.ui.update);
	mFree(request->requestStatement.ui.insert);

	return(retval);
}

static int is_response_ready(struct sqlbus *bus)
{
	if(!bus) {
		return(FAILURE);
	}

	if(bus->proto->response.status != 1) {
		serror(&bus->error, 1, "%s", "Response data is not ready.");
		return(FAILURE);
	}

	return(SUCCESS);
}

static int reader_perfect(struct sqlbus *bus)
{
	if(!bus || !bus->proto) {
		return(FAILURE);
	}

	if(is_response_ready(bus) != SUCCESS){
		return(FAILURE);
	}

	if(bus->proto->response.messageIdentification != 0) {
		serror(&bus->error, 1, "%s", bus->proto->response.messageInfo);
		return(SUCCESS_WITH_INFO);
	}

	return(SUCCESS);
}

static int reader_fcount(struct sqlbus *bus, int *cnt)
{
	if(!bus || !cnt) {
		return(FAILURE);
	}

	if(is_response_ready(bus) != SUCCESS){
		return(FAILURE);
	}

	*cnt = bus->proto->response.fieldCounter;

	return(SUCCESS);
}

static int reader_fname(struct sqlbus *bus, int idx, char **fname)
{
	if(!bus || !fname || !bus->proto) {
		return(FAILURE);
	}

	if(is_response_ready(bus) != SUCCESS) {
		return(FAILURE);
	}

	if(idx < 0 || idx >= bus->proto->response.fieldCounter) {
		serror(&bus->error, 1, "Fields index is wrong, should be in [0,%d]", bus->proto->response.fieldCounter-1);
		return(FAILURE);
	}

	if(getFieldName(bus->proto->response.fieldData, idx, fname) != SUCCESS) {
		serror(&bus->error, 1, "Read field name failure when parse return value.");
		return(FAILURE);
	}

	return(SUCCESS);
}

static int reader_rcount(struct sqlbus *bus, int *cnt)
{
	if(!bus || !cnt) {
		return(FAILURE);
	}

	if(is_response_ready(bus) != SUCCESS){
		return(FAILURE);
	}

	*cnt = bus->proto->response.rowCounter;

	return(SUCCESS);
}

static int reader_rvalue(struct sqlbus *bus, int ridx, char *fname, char **value)
{
	int idx = 0;

	if(!bus || !fname || !bus->proto) {
		return(FAILURE);
	}

	if(is_response_ready(bus) != SUCCESS) {
		return(FAILURE);
	}

	if(ridx < 0 || ridx >= bus->proto->response.rowCounter) {
		serror(&bus->error, 1, "Rows index is wrong, should be in [0,%d]", bus->proto->response.rowCounter-1);
		return(FAILURE);
	}

	if(getFieldIdx(bus->proto->response.fieldData, fname, &idx) != SUCCESS) {
		serror(&bus->error, 1, "Can't find field named %s.", fname);
		return(FAILURE);
	}

	if(getRowValue(bus->proto->response.rowData, ridx, idx, value) != SUCCESS) {
		serror(&bus->error, 1, "An error was happened when read row value.");
		return(FAILURE);
	}

	return(SUCCESS);
}

static int reader_rvalue_idx(struct sqlbus *bus, int ridx, int cidx, char **value)
{
	if(!bus || !bus->proto || !value) {
		return(FAILURE);
	}

	if(is_response_ready(bus) != SUCCESS) {
		return(FAILURE);
	}

	if(ridx < 0 || ridx >= bus->proto->response.rowCounter) {
		serror(&bus->error, 1, "Rows index is wrong, should be in [0,%d]", bus->proto->response.rowCounter-1);
		return(FAILURE);
	}

	if(cidx < 0 || cidx >= bus->proto->response.fieldCounter) {
		serror(&bus->error, 1, "Fields index is wrong, should be in [0,%d]", bus->proto->response.fieldCounter-1);
		return(FAILURE);
	}

	if(getRowValue(bus->proto->response.rowData, ridx, cidx, value) != SUCCESS) {
		serror(&bus->error, 1, "An error was happened when read row value.");
		return(FAILURE);
	}

	return(SUCCESS);
}

static int reader_finalize(struct sqlbus *bus)
{
	if(!bus || !bus->proto) {
		return(FAILURE);
	}

	sqlbusResponse *response = &bus->proto->response;

	if(response->status != 1) {
		return(FAILURE);
	}

#ifdef HI_SQL_BUS_DEBUG_MAIN
	mFree(response->originData);
#endif

	mFree(response->messageType);
	mFree(response->serverName);
	mFree(response->databaseType);
	mFree(response->messageInfo);
	mFree(response->universalUniqueIdentification);

	response->fieldCounter = 0;
	response->rowCounter = 0;
	response->timestamp = 0;
	response->messageIdentification = 0;
	response->processIdentification = 0;

	releaseJsonSource(response->reserveData);

	response->rowData = NULL;
	response->fieldData = NULL;
	response->reserveData = NULL;

	return(SUCCESS);
}
