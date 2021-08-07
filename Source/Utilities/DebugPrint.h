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

#ifndef __DEBUGPRINTF__
#define __DEBUGPRINTF__

#include "OPUtils.h"

//	------------------------------	Includes
//	------------------------------	Public Definitions
//	------------------------------	Public Types
//	------------------------------	Public Variables
//	------------------------------	Public Functions

#ifdef __cplusplus

	#if DEBUGCALLCHAIN

		class DebugPrintEntryExit
		{
		public:

			DebugPrintEntryExit(char *inFunctionName);
			~DebugPrintEntryExit();

			static	NMUInt32 	gDepth;

		protected:

			char	mFunctionName[256];
			
		};

		#define DEBUG_ENTRY_EXIT(x)		DebugPrintEntryExit		debugChain(x)

	#else

		#define DEBUG_ENTRY_EXIT(x)

	#endif // DEBUGCALLCHAIN

#endif // __cplusplus

#endif // __DEBUGPRINTF__
