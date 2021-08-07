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

#ifndef __NETSPROCKETLIB__
#define __NETSPROCKETLIB__

//	------------------------------	Includes

	#ifdef OP_API_PLUGIN_MAC_CFM
		#include <CodeFragments.h>
	#endif

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#ifndef __NETMODULE__
	#include 			"NetModule.h"
	#endif

//	------------------------------	Public Functions


#ifdef __cplusplus
extern "C" {
#endif

	#ifdef OP_API_PLUGIN_MAC_CFM
		OSErr 	cfInitialize(CFragInitBlockPtr ibp);
		void 	cfTerminate(void);
	#endif

	void GetQState(NSpGameReference inGame, NMUInt32 *outFreeQ, NMUInt32 *outCookieQ, NMUInt32 *outMessageQ);
	NMBoolean GetDebugMode( void );
	NMBoolean GetSynchronous( void );
	void SetDebugMode(NMBoolean inDebugMode);	

#ifdef __cplusplus
}
#endif

#endif // __NETSPROCKETLIB__