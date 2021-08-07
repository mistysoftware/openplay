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
 */

#ifndef __OP_GLOBALS__
#define __OP_GLOBALS__

	#ifndef __NETMODULE__
	#include 			"NetModule.h"
	#endif

//  -----------------------------  Public Prototypes
void initialize_openplay(FileDesc *file);

//	------------------------------	Public Definitions

	#define MAXIMUM_CACHED_ENDPOINTS (8)
	#define MAXIMUM_DOOMED_ENDPOINTS (8)

//	------------------------------	Public Types

	struct module_data {
		NMProtocol module;
		FileDesc module_spec;
	};
	typedef struct module_data	module_data;
	
	
	struct loaded_modules_data {
		NMType 					type;
		ConnectionRef 			connection; /* linked library reference. */
		NMSInt32    			connection_binded_count;
		NMSInt32    			endpoints_instantiated;
		Endpoint *				first_loaded_endpoint;
		loaded_modules_data *	next;
	};
	typedef struct loaded_modules_data	loaded_modules_data;
	

	struct op_globals {
		/* cache the protocols */
		NMUInt32 				ticks_at_last_protocol_search;
		NMSInt16				module_count;
		module_data *			modules;
		FileDesc				file_spec;
		loaded_modules_data *	loaded_modules;

		NMSInt16				res_refnum;
	#if OP_PLATFORM_MAC_MACHO
		CFBundleRef				selfBundleRef;
	#endif

	#ifdef OP_API_NETWORK_OT

		Endpoint *				endpoint_cache[MAXIMUM_CACHED_ENDPOINTS];
		NMSInt32 				cache_size;

		Endpoint *				doomed_endpoints[MAXIMUM_DOOMED_ENDPOINTS];
		NMSInt16 				doomed_endpoint_count;

	#elif defined(OP_API_NETWORK_WINSOCK)

		HINSTANCE dll_instance;

	#elif defined(OP_API_NETWORK_SOCKETS)

	#else
		#error "Porting problem - check op_globals structure"
	#endif
	};
	typedef struct op_globals	op_globals;

	extern op_globals 	gOp_globals;

#endif // __OP_GLOBALS__

