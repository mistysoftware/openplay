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

#include <ctype.h>

#include "Utility.h"
#include "String_Utils.h"

//	------------------------------	Private Definitions

#define GetChar(s, i)	((i) > (s)[0] ? 0 : toupper((s)[(i)]))

//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions
//	------------------------------	Public Variables

//	--------------------	Swap


//----------------------------------------------------------------------------------------
// Swap
//----------------------------------------------------------------------------------------

inline void
Swap(unsigned char**& a, unsigned char**& b)
{
register unsigned char** temp = a;

	a = b;
	b = temp;
}

//----------------------------------------------------------------------------------------
// RadixSort
//----------------------------------------------------------------------------------------

void
RadixSort(unsigned char** labelsList, NMUInt32 listSize, NMUInt32 maxLabelSize)
{
	if (listSize > 1)
	{
		enum
		{
			kNumCharacters = 256
		};
		
		NMUInt32 count[kNumCharacters];
		NMUInt32 accumulator[kNumCharacters];
		
		unsigned char** tempList	= (unsigned char**) new_pointer(listSize * sizeof (unsigned char*));
		unsigned char** toList		= tempList;
		unsigned char** fromList	= (unsigned char**)labelsList;
		
		register NMUInt32 passes = 0;
		register NMUInt32 i;
		register NMUInt32 length = maxLabelSize;
		
		// Find out max length
		register NMUInt32 maxLength = 0;
		for (i = 0; i < listSize; i++)
		{
			char len = fromList[i][0];
			if (len > maxLength)
			{
				maxLength = len;
			}
		}
		length = maxLength + (maxLength & 0x00000001);
	
		// Sort
		for (passes = length - 1; passes > 0; passes--)
		{
			//	zero out the count each time
			for (i = kNumCharacters - 1; i > 0 ; i--)
			{
				count[i] = 0;
			}
			count[0] = 0;
			
			//	count the number of occurrences of each character
			for (i = 0; i < listSize; i++)
			{
				unsigned char theChar = GetChar(fromList[i], passes);
				count[theChar]++;
			}
			
			//	accumulate the number of occurrences
			
			accumulator[0] = 0;
			
			for (i = 1; i < kNumCharacters; i++)
			{
				accumulator[i] = accumulator[i - 1] + count[i - 1];
			}
		
			//	copying the strings to the right place for this pass, anyways.
			
			for (i = 0; i < listSize; i++)
			{
				unsigned char theChar = GetChar(fromList[i], passes);
				
				NMUInt32 index = accumulator[theChar];
				
				toList[index] = fromList[i];
				
				accumulator[theChar]++;
			}
		
			Swap(toList, fromList);
			
		}
		
		if (((length - 2) % 2) != 0)
		{
			Swap(toList, fromList);
		}
		
		// Make sure the sorted data is in the list we were passed
		if (fromList != labelsList)
		{
		// Copy data into labelsList
		unsigned char** from = fromList;
		unsigned char** to	 = labelsList;

			for (i = listSize; i != 0; i--)
			{
				*to++ = *from++;
			}
		}
		
		dispose_pointer(tempList);
	}
}
