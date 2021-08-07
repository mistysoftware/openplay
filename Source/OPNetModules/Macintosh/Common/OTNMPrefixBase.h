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

#ifndef __OTNMPREFIXBASE__
#define __OTNMPREFIXBASE__


//	------------------------------	Includes

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#include "OPUtils.h"

	#include "DebugPrint.h"
	#include "Utility.h"

//	------------------------------	Public Definitions
	

#ifdef __cplusplus


// Don't even think about changing this unless you really, really know what you're doing.
// Excessive death may result.

	#define USEOTNEW 		1


	#if USEOTNEW
		#include "OTGlobalNewDebug.h"
	#endif

#endif // __cplusplus

//	------------------------------	Public Types
//	------------------------------	Public Variables
//	------------------------------	Public Functions

#endif // __OTNMPREFIXBASE__
