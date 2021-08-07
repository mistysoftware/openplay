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

#include "TPointerArray.h"
#include "OPUtils.h"

#include <string.h>



//	------------------------------	Private Definitions

#define kGrowPointerArrayBy			16

//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions
//	------------------------------	Public Variables


//----------------------------------------------------------------------------------------
// TPointerArray::TPointerArray
//----------------------------------------------------------------------------------------

TPointerArray::TPointerArray(NMUInt32 growBy)
{
	fGrowBy 		= growBy == 0 ? kGrowPointerArrayBy : growBy;
	fNumberOfItems 	= 0;
	fMaxItems 		= 0;
	fItemArray 		= NULL;
}

//----------------------------------------------------------------------------------------
// TPointerArray::~TPointerArray
//----------------------------------------------------------------------------------------

TPointerArray::~TPointerArray()
{
	if (fItemArray != NULL)
		delete[] fItemArray;
}

//----------------------------------------------------------------------------------------
// TPointerArray::AddItem
//----------------------------------------------------------------------------------------

NMErr
TPointerArray::AddItem(void* theItem)
{
NMErr	theErr = GrowBy(1);
	
	if (theErr == kNMNoError)
	{
		fItemArray[fNumberOfItems] = theItem;
		fNumberOfItems++;
	}

	return theErr;
}

//----------------------------------------------------------------------------------------
// TPointerArray::InsertItem
//----------------------------------------------------------------------------------------

NMErr
TPointerArray::InsertItem(void* theItem, NMUInt32 ndx)
{
	if (ndx > fNumberOfItems)
	{
		ndx = fNumberOfItems;
	}
	
	NMErr theErr = GrowBy(1);
	
	if (theErr == kNMNoError)	// room available
	{
		NMUInt32 numToMove = fNumberOfItems - ndx;

		if (numToMove > 0)
		{
			machine_move_data((void*) &fItemArray[ndx], (void*) &fItemArray[ndx+1], numToMove * sizeof(void*));
		}

		fItemArray[ndx] = theItem;
		fNumberOfItems++;
	}

	return theErr;
}

//----------------------------------------------------------------------------------------
// TPointerArray::SetIndexedItem
//----------------------------------------------------------------------------------------

NMErr
TPointerArray::SetIndexedItem(NMUInt32 ndx, void* theItem)
{
NMErr	theErr = kNMNoError;
	
	if (ndx > fNumberOfItems)
	{
		theErr = -1;
	}
	
	if (theErr == kNMNoError)	// goo index
	{
		fItemArray[ndx] = theItem;
	}

	return theErr;
}

//----------------------------------------------------------------------------------------
// TPointerArray::FindItemIndex
//----------------------------------------------------------------------------------------

NMBoolean
TPointerArray::FindItemIndex(void* item, NMUInt32* index)
{
NMBoolean	found = false;
	
	for (NMUInt32 ndx = 0; ndx < fNumberOfItems; ndx++)
	{
		if (fItemArray[ndx] == item)
		{
			*index	= ndx;
			found	= true;
			break;
		}
	}
	
	return found;
}

//----------------------------------------------------------------------------------------
// TPointerArray::RemoveItem
//----------------------------------------------------------------------------------------

void *
TPointerArray::RemoveItem(void* removeItem)
{
void	*retItem = NULL;

	for (NMUInt32 ndx = 0; ndx < fNumberOfItems; ndx++)
	{
		if (fItemArray[ndx] == removeItem)
		{
			retItem = RemoveItemByIndex(ndx);
			break;
		}
	}
	
	return retItem;
}

//----------------------------------------------------------------------------------------
// TPointerArray::RemoveItemByIndex
//----------------------------------------------------------------------------------------

void *
TPointerArray::RemoveItemByIndex(NMUInt32 ndx)
{
void	*retItem = NULL;

	if (ndx < fNumberOfItems)
	{
		retItem = fItemArray[ndx];

		if (ndx+1 < fNumberOfItems)
		{
			machine_move_data(&fItemArray[ndx+1], &fItemArray[ndx], (fNumberOfItems - ndx - 1) * sizeof(void*));
		}

		fNumberOfItems--;
		Shrink();
	}

	return retItem;
}

//----------------------------------------------------------------------------------------
// TPointerArray::RemoveLastItem
//----------------------------------------------------------------------------------------

void *
TPointerArray::RemoveLastItem()
{
void	*retItem = NULL;

	if (fNumberOfItems != 0)
	{
		retItem = fItemArray[fNumberOfItems - 1];
		
		fNumberOfItems--;
		
		Shrink();
	}
	
	return retItem;
}

//----------------------------------------------------------------------------------------
// TPointerArray::GetIndexedItem
//----------------------------------------------------------------------------------------

void *
TPointerArray::GetIndexedItem(NMUInt32 ndx) const
{
	if (ndx < fNumberOfItems)
	{
		return fItemArray[ndx];
	}
	else
	{
		return NULL;
	}
}

//----------------------------------------------------------------------------------------
// TPointerArray::RemoveAllItems
//----------------------------------------------------------------------------------------

void
TPointerArray::RemoveAllItems()
{
	fNumberOfItems 	= 0;
	fMaxItems 		= 0;
	
	if (fItemArray != NULL)
	{
		delete[] fItemArray;
		fItemArray = NULL;
	}
}

//----------------------------------------------------------------------------------------
// TPointerArray::GrowBy
//----------------------------------------------------------------------------------------

NMErr
TPointerArray::GrowBy(NMUInt32 numNewItems)
{
NMErr	theErr = kNMNoError;

	if ((fNumberOfItems + numNewItems) > fMaxItems)	// do we really need to grow?
	{
		void** newArray = new void* [fMaxItems + fGrowBy];

		if (newArray != NULL)
		{
			if (fNumberOfItems != 0)
			{
				machine_copy_data((void*) fItemArray, (void*) newArray, fNumberOfItems * sizeof(void*));
			}

			delete[] fItemArray;

			fItemArray = newArray;
			fMaxItems += fGrowBy;
		}
		else
		{
			theErr = kArrayOutOfMemoryErr;
		}
	}
	
	return theErr;
}

//----------------------------------------------------------------------------------------
// TPointerArray::Shrink
//----------------------------------------------------------------------------------------

void
TPointerArray::Shrink()
{
	if (fNumberOfItems == 0)
	{
		//	the base case is that there are no more items in the array.
		//	in this case, fItemArray is deleted and set to NULL.
	
		if (fItemArray != NULL)
		{
			delete[] fItemArray;
			fItemArray = NULL;
		}
		
		fMaxItems = 0;
	}
	else
	{
		//	if the fNumberOfItems is more than fGrowBy less than fMaxItems,
		//	then fItemArray is resized.
		
		if (fMaxItems > (fNumberOfItems + fGrowBy))
		{
			NMUInt32 numBlocks = (fNumberOfItems / fGrowBy) + 1;
			NMUInt32 newSize = fGrowBy * numBlocks;
			
			void** newArray = new void* [newSize];
			
			if (newArray != NULL)
			{
				for (NMUInt32 i = 0; i < fNumberOfItems; i++)
				{
					newArray[i] = fItemArray[i];
				}
				
				delete[] fItemArray;
				fItemArray = newArray;

				fMaxItems = newSize;
			}
		}
	} 
}

