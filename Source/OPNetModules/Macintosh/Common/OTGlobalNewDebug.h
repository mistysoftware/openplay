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

#ifndef __OTGLOBALNEWDEBUG__
#define __OTGLOBALNEWDEBUG__

//	------------------------------	Includes

	#ifndef __STDDEF__
	#include <StdDef.h>
	#endif
	#ifndef __OTUTILS__
	#include "OTUtils.h"
	#endif

	#if PRAGMA_IMPORT_SUPPORTED
	#pragma import on
	#endif

//	------------------------------	Public Definitions
//	------------------------------	Public Types
//	------------------------------	Public Variables

//	------------------------------	Public Functions



	#if PRAGMA_IMPORT_SUPPORTED
	#pragma import off
	#endif

	inline void *
	operator new(size_t size)
	{
		if (!OTUtils::OTInitialized())
		{
			DEBUG_PRINT("Calling OT NewPtr when OT is unloaded! ");
			return NULL;
		}
	
		size_t	realSize = size + 4;
		NMUInt32	*tag = (NMUInt32 *) InterruptSafe_alloc(realSize);

		*tag = 'good';

		return tag + 1;
	}

	inline void *
	operator new[](size_t size)
	{
	
		if (!OTUtils::OTInitialized())
		{
			DEBUG_PRINT("Calling OT NewPtr when OT is unloaded! ");
			return NULL;
		}
	
		size_t	realSize = size + 4;
		NMUInt32	*tag = (NMUInt32 *) InterruptSafe_alloc(realSize);

		*tag = 'gdar';

		return tag + 1;
	}

	inline void *
	operator new(size_t size, size_t extra)
	{
	size_t	realSize = size + extra + 4;
	NMUInt32	*tag = (NMUInt32 *) InterruptSafe_alloc(realSize);

		*tag = 'good';

		return tag + 1;
	}

	inline void
	operator delete(void* theMem) throw() //gmp 4/1/2004 added throw() for cwp9
	{
		if (theMem == NULL)
			return;
			
		if (! OTUtils::OTInitialized())
		{
			DEBUG_PRINT("Calling OT Free when OT is unloaded! %x", theMem);
			return;
		}
			
		NMUInt8 *realPtr = (NMUInt8 *)theMem - 4;
		NMUInt32 *tag = (NMUInt32 *) realPtr;

		if (*tag == 'gdar')
		{
			DEBUG_PRINT("Used non-array operator to delete an array");
		}
		
		//	make sure this is a valid pointer
		if (*tag != 'gdar' && *tag != 'good')
		{
			DEBUG_PRINT("Invalid pointer, %x", realPtr);
			return;
		}

		*tag = 'bad ';
		InterruptSafe_free(realPtr);
	}
		
	inline void
	operator delete[](void* theMem) throw() //gmp 4/1/2004 added throw() for cwp9
	{
		if (theMem == NULL)
			return;
			
		if (!OTUtils::OTInitialized())
		{
			DEBUG_PRINT("Calling OT Free when OT is unloaded! %x", theMem);
			return;
		}
			
		NMUInt8 *realPtr = (NMUInt8 *)theMem - 4;
		NMUInt32 *tag = (NMUInt32 *) realPtr;

		if (*tag == 'good')
		{
			DEBUG_PRINT("Used array operator to delete a non-array");
		}
		
		//	make sure this is a valid pointer
		if (*tag != 'gdar' && *tag != 'good')
		{
			DEBUG_PRINT("Invalid pointer, %x", realPtr);
			return;
		}

		*tag = 'bad ';
		InterruptSafe_free(realPtr);
	}

#endif // __OTGLOBALNEWDEBUG__

