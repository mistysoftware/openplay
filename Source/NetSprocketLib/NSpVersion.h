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
 
#ifndef __NSPVERSION__
#define __NSPVERSION__

// Stolen from MacTypes.r
typedef enum
{
        development = 0x20,
        alpha       = 0x40,
        beta        = 0x60,
        final       = 0x80,
        release     = 0x80
} op_release_defs;

	// The following corresponds to 2.2r1

	#define __NSpVersionMajor__		2
	#define __NSpVersionMinor__		2
	#define __NSpVersionBugFix__		0
	#define __NSpReleaseStage__		release
	#define __NSpNonRelRev__			1

#endif // __NSPVERSION__

