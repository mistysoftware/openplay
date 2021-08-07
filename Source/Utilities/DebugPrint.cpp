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

//	------------------------------	Includes

#include "DebugPrint.h"
#include <string.h>

#if DEBUGCALLCHAIN

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables

NMUInt32 DebugPrintEntryExit::gDepth = 0;

//	------------------------------	Private Functions
//	------------------------------	Public Variables

//	--------------------	DebugPrintEntryExit::DebutPrintEntryExit

//----------------------------------------------------------------------------------------
// DebugPrintEntryExit::DebugPrintEntryExit
//----------------------------------------------------------------------------------------

DebugPrintEntryExit::DebugPrintEntryExit(char *inFunctionName)
{
char	tabs[56];
	
	strcpy(mFunctionName, inFunctionName);

	for (NMSInt32 i = 0; i < gDepth; i++)
	{
		tabs[i*5] = '*';
		tabs[(i*5) + 1] = ' ';
		tabs[(i*5) + 2] = ' ';
		tabs[(i*5) + 3] = ' ';
		tabs[(i*5) + 4] = ' ';
	}
		
	tabs[gDepth*5] = '\0';
	DEBUG_PRINT("%sENTER: %s", tabs, mFunctionName);
	gDepth++;

	if (gDepth > 10)
		gDepth = 10;
}

//----------------------------------------------------------------------------------------
// DebugPrintEntryExit
//----------------------------------------------------------------------------------------

DebugPrintEntryExit::~DebugPrintEntryExit()
{	
char	tabs[56];
	
	if( gDepth )
	{
		--gDepth;

		for (NMSInt32 i = 0; i < gDepth; i++)
		{
			tabs[i*5] = '*';
			tabs[(i*5) + 1] = ' ';
			tabs[(i*5) + 2] = ' ';
			tabs[(i*5) + 3] = ' ';
			tabs[(i*5) + 4] = ' ';
		}
	}
	tabs[gDepth*5] = '\0';
	DEBUG_PRINT("%sLEAVE: %s", tabs, mFunctionName);
}

#endif // DEBUGCALLCHAIN
