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
	ip_module.h
*/

#ifndef __WIN_IP_MODULE__
#define __WIN_IP_MODULE__

	#include "NetModule.h"
	#include <winsock.h>
	#include "OPUtils.h"
	#include "ip_enumeration.h"

	enum {
		kModuleID= 'Inet',					// needs to be same as kTCPIPProtocol
		kVersion = 0x00000100,
		config_cookie= 'IPcf',
		DEFAULT_TIMEOUT= 5*1000, 		// 5 seconds
		MAXIMUM_CONFIG_LENGTH= 1024,
		MAXIMUM_OUTSTANDING_REQUESTS= 4,

		TICKS_BETWEEN_ENUMERATION_REQUESTS= (MACHINE_TICKS_PER_SECOND/2),
		TICKS_BEFORE_GAME_DROPPED= (2*MACHINE_TICKS_PER_SECOND)
	};

	enum {
		strNAMES= 128,
		idModuleName= 0,
		idCopyright,
		idDefaultHost
	};


	/* sjb 19990317 this needs to be shared per library/process, not per thread */
	typedef struct
	{
		HINSTANCE application_instance;
	} module_data;

	enum {
		_datagram_socket,
		_stream_socket,
		NUMBER_OF_SOCKETS,
		
		_uber_connection= ((1<<_datagram_socket) | (1<<_stream_socket))
	};

	enum { // function passthroughs
		_pass_through_set_debug_proc= 'debg'
	};

	typedef NMSInt32 (*status_proc_ptr)(const char *format, ...);

	struct NMEndpointPriv	 {
		NMType 							cookie;
		NMBoolean 						alive;
		NMUInt32 						version;
		NMUInt32 						gameID;
		NMUInt32						timeout;
		NMSInt32						connectionMode;
		NMBoolean 						netSprocketMode;
		NMSInt32						valid_endpoints;
		NMEndpointCallbackFunction *	callback;
		void *							user_context;
		NMBoolean 						dynamically_assign_remote_udp_port;
		char 							name[kMaxGameNameLen+1];
		NMUInt32						host;
		NMUInt16 						port;
		SOCKET 							socket[NUMBER_OF_SOCKETS];
		HWND 							window; // for WSAAsyncSelect()
		status_proc_ptr					status_proc;
		NMSInt32 						thread_id;
		NMBoolean						active;
		NMErr							opening_error;

	} ;
	typedef struct NMEndpointPriv		NMEndpointPriv;

	enum {
		_new_game_flag= 0x01,
		_delete_game_flag= 0x02,
		_duplicate_game_flag= 0x04
	};

	typedef struct  {
		NMUInt16 			port;
	} udp_port_struct_from_server;

	typedef struct  {
		NMUInt32		host;
		NMUInt16 		port;
		NMUInt16 		flags;
		NMSInt32		ticks_at_last_response;
		char 			name[kMaxGameNameLen+1];
	} available_game_data;

	#define MAXIMUM_GAMES_ALLOWED_BETWEEN_IDLE 10

	struct NMProtocolConfigPriv  {
		NMUInt32 					cookie;
		NMType 						type;
		NMUInt32					version;
		NMUInt32 					gameID;
		NMSInt32					connectionMode;
		NMBoolean					netSprocketMode;
		TCHAR 						host_name[256];		// host name in unicode
		SOCKADDR_IN 				hostAddr;		// remote host name
		char 						name[kMaxGameNameLen + 1];
		char 						buffer[MAXIMUM_CONFIG_LENGTH];

		// Enumeration Data follows
		available_game_data *		games;
		NMSInt32 					game_count;
		NMEnumerationCallbackPtr	callback;
		void *						user_context;
		SOCKET						enumeration_socket;
		NMBoolean					enumerating;
		NMBoolean					activeEnumeration;		
		available_game_data 		new_games[MAXIMUM_GAMES_ALLOWED_BETWEEN_IDLE];
		NMSInt32					new_game_count;
		NMUInt32					ticks_at_last_enumeration_request;
	};
	typedef struct NMProtocolConfigPriv		NMProtocolConfigPriv;

	/* ------- globals */
	extern module_data 		gModuleGlobals;
	extern LPCSTR 			szNetCallbackWndClass;

	/* ------ prototypes */
	char *nm_getcstr(char *buffer, NMSInt16 resource_number, NMSInt16 string_number, NMSInt16 max_length);


	LONG APIENTRY NetCallbackWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam);



#endif // __WIN_IP_MODULE__

