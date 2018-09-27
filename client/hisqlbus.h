
/**
 * hisqlbus.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-04-10 09:34:14
 * Last Modified : 2018-04-10 09:34:14
 */

#ifndef __HISQLBUS_H__
#define __HISQLBUS_H__

#include <stddef.h>

#include "../third/hiredis/hiredis.h"

#define FAILURE           (-1)
#define SUCCESS           ( 0)
#define SUCCESS_WITH_INFO ( 1)
#define SUCCESSED(rv)     (((rv)&(-1))==0)

// Synchronization flag of query operation
#define EXEC_SYNC  (0)
#define EXEC_ASYNC (1)

// Max counter of the number of sql statement
#define MAX_SQL_COUNT 64

typedef struct sqlbus sqlbus;
typedef struct sqlbusMCClient sqlbusMCClient;
typedef struct sqlbusMCOperFunctions sqlbusMCOperFunctions;
typedef struct sqlbusReaderFunctions sqlbusReaderFunctions;

/**
 * sqlbusRequest - Formate of sqlbus client request
 */
typedef struct sqlbusRequest {
	char *messageType;
	char *receiveChannel;
	char *clientName;

	struct {
		struct {
			char *update;
			char *insert;
		}ui;
		char *statement;
	} requestStatement;

	char *universalUniqueIdentification;
	uint32_t timestamp;
	uint32_t synchronizationType;
	uint32_t processIdentification;
} sqlbusRequest;

/**
 * sqlbusResponse - Formate of sqlbus server response
 */
typedef struct sqlbusResponse {
	int status;
#ifdef HI_SQL_BUS_DEBUG_MAIN
	// Used to show response string for debug
	char *originData;
#endif
	char *messageType;
	char *serverName;
	char *databaseType;
	char *messageInfo;
	char *universalUniqueIdentification;
	uint32_t fieldCounter;
	uint32_t rowCounter;
	uint32_t timestamp;
	uint32_t messageIdentification;
	uint32_t processIdentification;
	// The Json root of all result responsed
	void *reserveData;
	// The Json root of field name list, it's a node of @reserveData
	void *rowData;
	// The Json root of row result list, it's a node of @reserveData
	void *fieldData;
} sqlbusResponse;

/**
 * SerializationProtocol - Serialization protocol of request and response
 */
typedef struct SerializationProtocol {
	sqlbusRequest request;
	sqlbusResponse response;
} SerializationProtocol;

/**
 * errorInfo - Error message storage
 */
typedef struct errorInfo {
	uint32_t etag;
	uint32_t capacity;
	char *errmsg;
} errorInfo;

/**
 * sqlbusMCClient - Information for client to connect memcache server
 */
struct sqlbusMCClient {
	uint16_t port;
	char host[64];
	char perm[64];
	char instance[64];
	char *recvQueue;
	char *sendQueue;
	redisContext *redis;
};

/**
 * sqlbusMCOperFunctions - All operation function for operateing memcache
 */
struct sqlbusMCOperFunctions {
	// Connect to memcache server
	int (*conn)  (sqlbus *bus);
	// Authentication memcache server connection
	int (*auth)  (sqlbus *bus);
	// Recevie a response from memcache server
	int (*recv)  (sqlbus *bus);
	// Close the connection
	int (*close) (sqlbus *bus);
	// Select database instance of memcache server
	int (*select)(sqlbus *bus, char *instance);
	// Send update and insert sql statement request to memcache server
	int (*uisend) (sqlbus *bus, char *update, char *insert);
	// Send a request to memcache server
	// @sync is a flag of synchronization
	// when @sync = QUERY_SYNC, memcache server will response the database operation result
	// when @sync = QUERY_ASYNC, memcache will not response nothing
	// otherwise, error
	int (*send)  (sqlbus *bus, void *data, int sync);
};

/**
 * sqlbusReaderFunctions - All operation function for read result set
 */
struct sqlbusReaderFunctions {
	// Judge request is normal or not
	// @perfect has three type return value
	// when -1, @perfect run failure or not fetch response
	// when 0, request is perfect
	// when 1, @perfect run success, but request was wrong, error msg save in @messageInfo
	int (*perfect) (struct sqlbus *bus);
	// Get the count number of columns
	int (*fcount)  (struct sqlbus *bus, int *cnt);
	// Get a column name by the index by order
	int (*fname)   (struct sqlbus *bus, int idx, char **fname);
	// Get the count number of rows
	int (*rcount)  (struct sqlbus *bus, int *cnt);
	// Get the column value by row index and column name
	int (*value)   (struct sqlbus *bus, int ridx, char *fname, char **value);
	// Get the column value by row index and column index
	int (*valueIdx)(struct sqlbus *bus, int ridx, int cidx, char **value);
	// Release the memory of response reulst
	int (*free)    (struct sqlbus *bus);
};

/**
 * sqlbus - Primary interface of sqlbus client libray
 */
struct sqlbus {
	errorInfo error;
	struct sqlbusMCClient *mc;
	struct sqlbusMCOperFunctions *ofn;
	struct sqlbusReaderFunctions *rfn;
	struct SerializationProtocol *proto;
};

__BEGIN_DECLS

/**
 * sqlbusCreate - Create a sqlbus operating handle
 *
 * @port: Port of memcache server
 * @host: Host name or ip of memcache server
 * @auth: Permission to memcache server authenticated
 *
 * if parameter is "" or NULL, it'll use default options.
 * During creating handler, it does not any connection.
 *
 * Return value:
 *  NULL: Create handler failure
 *  !(NULL): Create handler success
 */
void *sqlbusCreate(uint16_t port, char *host, char *auth);

/**
 * sqlbusFinalize - Close connection to memcache server
 *
 * Return value:
 *  No return values
 */
void  sqlbusFinalize(sqlbus *bus);

__END_DECLS

#endif /* __HISQLBUS_H__ */
