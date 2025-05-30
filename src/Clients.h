/*******************************************************************************
 * Copyright (c) 2009, 2024 IBM Corp. and Ian Craggs
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *    https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs - add SSL support
 *    Ian Craggs - fix for bug 413429 - connectionLost not called
 *    Ian Craggs - change will payload to binary
 *    Ian Craggs - password to binary
 *    Ian Craggs - MQTT 5 support
 *******************************************************************************/

#if !defined(CLIENTS_H)
#define CLIENTS_H

#include <stdint.h>
#include "MQTTTime.h"
#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#endif
#if defined(OPENSSL)
#include <openssl/ssl.h>
#endif
#include "MQTTClient.h"
#include "LinkedList.h"
#include "MQTTClientPersistence.h"
#include "Socket.h"

/**
 * Stored publication data to minimize copying
 */
typedef struct
{
	char *topic;
	int topiclen;
	char* payload;
	int payloadlen;
	int refcount;
	uint8_t mask[4];
} Publications;

/**
 * Client publication message data
 */
typedef struct
{
	int qos;
	int retain;
	int msgid;
	int MQTTVersion;
	MQTTProperties properties;
	Publications *publish;
	START_TIME_TYPE lastTouch;		    /**> used for retry and expiry */
	char nextMessageType;	/**> PUBREC, PUBREL, PUBCOMP */
	int len;				/**> length of the whole structure+data */
} Messages;

/**
 * Client will message data
 */
typedef struct
{
	char *topic;
	int payloadlen;
	void *payload;
	int retained;
	int qos;
} willMessages;

typedef struct
{
	SOCKET socket;
	START_TIME_TYPE lastSent;
	START_TIME_TYPE lastReceived;
	START_TIME_TYPE lastPing;
#if defined(OPENSSL)
	SSL* ssl;
	SSL_CTX* ctx;
	char *https_proxy;
	char *https_proxy_auth;
#endif
	char *http_proxy;
	char *http_proxy_auth;
	int websocket; /**< socket has been upgraded to use web sockets */
	char *websocket_key;
	const MQTTClient_nameValue* httpHeaders;
} networkHandles;


/* connection states */
/** no connection in progress, see connected value */
#define NOT_IN_PROGRESS  0x0
/** TCP connection in progress */
#define TCP_IN_PROGRESS  0x1
/** SSL connection in progress */
#define SSL_IN_PROGRESS  0x2
/** Websocket connection in progress */
#define WEBSOCKET_IN_PROGRESS   0x3
/** TCP completed, waiting for MQTT ACK */
#define WAIT_FOR_CONNACK 0x4
/** Proxy connection in progress */
#define PROXY_CONNECT_IN_PROGRESS 0x5
/** Disconnecting */
#define DISCONNECTING    -2

/**
 * Data related to one client
 * The entire structure is initialized to 0 on creation, so all fields default to 0.
 */
typedef struct Clients
{
	char* clientID;					      /**< the string id of the client */
	const char* username;					/**< MQTT v3.1 user name */
	int passwordlen;              /**< MQTT password length */
	const void* password;					/**< MQTT v3.1 binary password */
	unsigned int cleansession : 1;	/**< MQTT V3 clean session flag */
	unsigned int cleanstart : 1;		/**< MQTT V5 clean start flag */
	unsigned int connected : 1;		/**< whether it is currently connected */
	unsigned int good : 1; 			  /**< if we have an error on the socket we turn this off */
	unsigned int ping_outstanding : 1;
	unsigned int ping_due : 1;      /**< we couldn't send a ping so we should send one when we can */
	signed int connect_state : 4;
	START_TIME_TYPE ping_due_time;  /**< the time at which the ping should have been sent (ping_due) */
	networkHandles net;             /**< network info for this client */
	int msgID;                      /**< the MQTT message id */
	int socket_fwmark;              /**< the socket fwmark */
	int keepAliveInterval;          /**< the MQTT keep alive interval */
	int savedKeepAliveInterval;     /**< saved keep alive interval, in case reset by server keep alive */
	int retryInterval;              /**< the MQTT retry interval for QoS > 0 */
	int maxInflightMessages;        /**< the max number of inflight outbound messages we allow */
	willMessages* will;             /**< the MQTT will message, if any */
	List* inboundMsgs;              /**< inbound in flight messages */
	List* outboundMsgs;				/**< outbound in flight messages */
	int connect_count;              /**< the number of outbound messages on reconnect - to ensure we send them all */
	int connect_sent;               /**< the current number of outbound messages on reconnect that we've sent */
	List* messageQueue;             /**< inbound complete but undelivered messages */
	List* outboundQueue;            /**< outbound queued messages */
	unsigned int qentry_seqno;
	void* phandle;                  /**< the persistence handle */
	MQTTClient_persistence* persistence; /**< a persistence implementation */
    MQTTPersistence_beforeWrite* beforeWrite; /**< persistence write callback */
    MQTTPersistence_afterRead* afterRead; /**< persistence read callback */
    void* beforeWrite_context;      /**< context to be used with the persistence beforeWrite callbacks */
    void* afterRead_context;        /**< context to be used with the persistence afterRead callback */
	void* context;                  /**< calling context - used when calling disconnect_internal */
	int MQTTVersion;                /**< the version of MQTT being used, 3, 4 or 5 */
	unsigned int sessionExpiry;     /**< MQTT 5 session expiry */
	char* httpProxy;                /**< HTTP proxy */
	char* httpsProxy;               /**< HTTPS proxy */
#if defined(OPENSSL)
	MQTTClient_SSLOptions *sslopts; /**< the SSL/TLS connect options */
	SSL_SESSION* session;           /**< SSL session pointer for fast handhake */
#endif
} Clients;

int clientIDCompare(void* a, void* b);
int clientSocketCompare(void* a, void* b);

/**
 * Configuration data related to all clients
 */
typedef struct
{
	const char* version;
	List* clients;
} ClientStates;

#endif
