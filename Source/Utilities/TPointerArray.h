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

#ifndef __TPOINTERARRAY__
#define __TPOINTERARRAY__

//	------------------------------	Includes
	
	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif

//	------------------------------	Public Definitions

	#define	kArrayOutOfMemoryErr		45432

	// A handy macro to delete all the items in a list and call the right destructor
	#define DeleteAllItems(listPtr, type)	{void* item;												\
											 	while ( (item = (listPtr)->RemoveLastItem()) != NULL )	\
											 	{														\
											 		delete (type)item;									\
											 	}														\
											}									

//	------------------------------	Public Types

	class TPointerArray
	{
	public:
							TPointerArray(NMUInt32 growBy = 0);
		virtual				~TPointerArray();

		NMErr			AddItem(void* theItem);
		NMErr			InsertItem(void* theItem, NMUInt32 ndx);

		NMBoolean			FindItemIndex(void* item, NMUInt32* index);
		
		void*				RemoveItem(void* theItem);
		void*				RemoveItemByIndex(NMUInt32 ndx);
		void*				RemoveLastItem();
		void				RemoveAllItems();
		
		void*				operator[](NMUInt32	 ndx) const		{ return GetIndexedItem(ndx) ;};
		void*				GetIndexedItem(NMUInt32	 ndx) const;
		NMErr			SetIndexedItem(NMUInt32	 ndx, void* theItem);
		
		NMUInt32			Count() const						{ return fNumberOfItems; };
		
	private:

		NMErr			GrowBy(NMUInt32	 numNewItems);
		void				Shrink();
		

		NMUInt32			fNumberOfItems;
		NMUInt32			fMaxItems;
		NMUInt32			fGrowBy;
		void**				fItemArray;
	};

//	------------------------------	Public Variables
//	------------------------------	Public Functions

#endif // __TPOINTERARRAY__
