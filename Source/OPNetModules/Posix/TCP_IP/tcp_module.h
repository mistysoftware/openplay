/*
 * Copyright (c) 1999-2004 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999-2004 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 *
*/

/*
	tcp_module.h
*/
#ifndef __TCP_MODULE__
#define __TCP_MODULE__

//	------------------------------	Includes
	#ifndef __NETMODULE__
	#include 			"NetModule.h"
	#endif
	
#ifdef OP_API_NETWORK_WINSOCK
	#include <winsock.h>
#elif defined(OP_API_NETWORK_SOCKETS)
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>
#endif

	#include "NetModulePrivate.h"
	#include "OPUtils.h"
	#include "machine_lock.h"
	#include "ip_enumeration.h"
	#include "DebugPrint.h"

//	------------------------------	Public Definitions

//	------------------------------	Public Types

	//define some stuff for windows to keep our codebase mostly-unmodified
	#ifdef OP_PLATFORM_WINDOWS
		typedef WORD word;
		#define op_errno WSAGetLastError()
		#define EINPROGRESS WSAEINPROGRESS
		#define EWOULDBLOCK WSAEWOULDBLOCK
		#define EMSGSIZE WSAEMSGSIZE
		#define usleep(a) Sleep(a/1000)
		#define pthread_exit ExitThread
		#define close closesocket
	#else
		#define op_errno errno
	#endif


	#if defined(OP_PLATFORM_MAC_MACHO) || defined(OP_PLATFORM_WINDOWS)
		typedef int 			posix_size_type;
	#else
		typedef unsigned int 	posix_size_type;
	#endif




	// without a worker thread, message retrieval is dependant on NMIdle() calls
	#define USE_WORKER_THREAD 1

	#define kIPConfigAddress  "IPaddr"
	#define kIPConfigPort     "IPport"

	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET (-1)
	#endif

	//ecf for some reason sticking these values  enum declaration gives me an error on red hat linux...
	#define TICKS_BETWEEN_ENUMERATION_REQUESTS (MACHINE_TICKS_PER_SECOND / 2)
	#define TICKS_BEFORE_GAME_DROPPED (2 * MACHINE_TICKS_PER_SECOND)
	enum {
	  kModuleID                          = 0x496e6574,  /* "Inet" needs to be same as kTCPIPProtocol */
	  kVersion                           = 0x00000100,
	  config_cookie                      = 0x49506366,  /* "IPcf" */
	  DEFAULT_TIMEOUT                    = 5*1000,      /* 5 seconds */
	  MAXIMUM_CONFIG_LENGTH              = 1024,
	  MAXIMUM_OUTSTANDING_REQUESTS       = 4
	  //TICKS_BETWEEN_ENUMERATION_REQUESTS = (MACHINE_TICKS_PER_SECOND / 2),
	  //TICKS_BEFORE_GAME_DROPPED          = (2 * MACHINE_TICKS_PER_SECOND)
	};

	enum {
		strNAMES = 128,
		idModuleName = 0,
		idCopyright,
		idDefaultHost
	};


	enum {
		_datagram_socket,
		_stream_socket,
		NUMBER_OF_SOCKETS,	
		_uber_connection = ((1 << _datagram_socket) | (1 << _stream_socket))
	};

	
	/* passthrough functions */
	enum {
	  _pass_through_set_debug_proc = 0x64656267  /* hex for "debg" */
	};

	typedef int (*status_proc_ptr)(const char *format, ...);

	struct 	NMEndpointPriv {
		NMEndpointRef next; //we're in a linked list
		NMEndpointRef parent; //if we were spawned from a host endpoint
		NMType cookie;
		NMBoolean alive; //we're alive until we give the close complete message
		NMBoolean dying; //we've given the endpoint died message
		NMBoolean needToDie; //socket bit the dust - need to start dying
		NMUInt32 version;
		NMSInt32 gameID;
		unsigned long timeout;
		long connectionMode;
		NMBoolean		advertising;
		NMBoolean 		netSprocketMode;	
		sockaddr_in remoteAddress; //in netsprocket mode we need to store the address for datagram sending	
		long valid_endpoints;
		NMEndpointCallbackFunction *callback;
		void *user_context;
		NMBoolean dynamically_assign_remote_udp_port;
		char name[kMaxGameNameLen+1];
		long host;
		word port;
#ifdef OP_API_NETWORK_SOCKETS
		int sockets[NUMBER_OF_SOCKETS];
#elif defined(OP_API_NETWORK_WINSOCK)
		unsigned int sockets[NUMBER_OF_SOCKETS];
#else
	#error "This posix types is not known"		
#endif		
		NMBoolean flowBlocked[NUMBER_OF_SOCKETS];
		NMBoolean newDataCallbackSent[NUMBER_OF_SOCKETS];
		status_proc_ptr status_proc;
		NMBoolean active;
		NMBoolean listener;
		NMErr opening_error;
	};

	enum {
		_new_game_flag= 0x01,
		_delete_game_flag= 0x02,
		_duplicate_game_flag= 0x04
	};

	struct udp_port_struct_from_server {
		word port;
	};

	struct available_game_data {
		long host;
		word port;
		word flags;
		long ticks_at_last_response;
		char name[kMaxGameNameLen+1];
	};

	#define MAXIMUM_GAMES_ALLOWED_BETWEEN_IDLE (10)

	struct NMProtocolConfigPriv {
		NMUInt32 cookie;
		NMType type;
		NMUInt32 version;
		NMSInt32 gameID;
		long connectionMode;
		NMBoolean netSprocketMode;
		char host_name[256];
	        struct sockaddr_in hostAddr;		/* remote host name */
		char name[kMaxGameNameLen + 1];
		char buffer[MAXIMUM_CONFIG_LENGTH];

	  /* Enumeration Data follows */
		struct available_game_data *games;
		short game_count;
		NMEnumerationCallbackPtr callback;
		void *user_context;
		int enumeration_socket;
		NMBoolean enumerating;
		NMBoolean activeEnumeration;
		struct available_game_data new_games[MAXIMUM_GAMES_ALLOWED_BETWEEN_IDLE];
		short new_game_count;
		NMUInt32 ticks_at_last_enumeration_request;
	};


//	------------------------------	Public Functions

	char *nm_getcstr(char *buffer, short resource_number, short string_number, short max_length);

#if (USE_WORKER_THREAD)
	void createWorkerThread(void);
	#ifdef OP_API_NETWORK_SOCKETS
		void* worker_thread_func(void *arg);
	#elif defined(OP_API_NETWORK_WINSOCK)
		DWORD WINAPI worker_thread_func(LPVOID arg);
	#endif
	void killWorkerThread(void);
#endif
	
int createWakeSocket(void);
void disposeWakeSocket(void);
void sendWakeMessage(void);
	
#ifdef OP_API_NETWORK_WINSOCK
	void initWinsock(void);
	void shutdownWinsock(void);
#endif

	void SetNonBlockingMode(int fd);
	
	
// --------------------------------  Globals
	extern NMUInt32 endpointListState;
	extern NMEndpointPriv *endpointList;
	extern machine_lock *endpointListLock;
	extern machine_lock *endpointWaitingListLock;
	extern machine_lock *notifierLock;
	extern NMSInt32	module_inited;
#endif  // __TCP_MODULE__
