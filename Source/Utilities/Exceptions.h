/*
 * Copyright (c) 1999-2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999-2002 Apple Computer, Inc.  All Rights
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

#ifndef __OPENPLAYEXCEPTIONS__
#define __OPENPLAYEXCEPTIONS__

//	------------------------------	Includes

#ifdef OP_PLATFORM_MAC_CFM
	#include <errors.h>
#endif

//	------------------------------	Public Definitions

// sjb 19990419 these must be longs so they match the
// catch clauses and we don't throw them past the handlers
	const long err_NilPointer = 'nilP';
	const long err_AssertFailed = 'asrt';

	//ecf exceptions arent working on linux... argh...
	/*
	#define Try_ try

	#define Throw_(a) throw (a)

	#define Catch_(a) catch (long a)

	#define ThrowIfOSErr_(a)	\
		if (kNMNoError != (a))		\
		{						\
			throw a;			\
		}

	#define ThrowIfNil_(a)			\
		if (0 == (a))				\
		{							\
			throw err_NilPointer;	\
		}

	#define ThrowIfNULL_(a)			\
		if (0 == (a))				\
		{							\
			throw err_NilPointer;	\
		}

	#define ThrowIfNot_(a)			\
		if (0 == (a))				\
		{							\
			throw err_AssertFailed;	\
		}
	*/
#endif // __OPENPLAYEXCEPTIONS__

