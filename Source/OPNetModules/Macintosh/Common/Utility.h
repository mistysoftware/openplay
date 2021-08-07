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

#ifndef __UTILITY__
#define __UTILITY__

//	------------------------------	Includes
//	------------------------------	Public Definitions

	#define ROUNDLONG(x)	RoundLong(x)
	#define ALIGNLONG(y)	AlignLong(y)

//	------------------------------	Public Types
//	------------------------------	Public Variables
//	------------------------------	Public Functions

#ifdef __cplusplus

	class SetTempPort {
		public:

		inline	SetTempPort(WindowRef window) { 	GetPort(&fCurPort);
													if (fCurPort != (GrafPtr)GetWindowPort(window))
														SetPortWindowPort(window); }
	#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		inline SetTempPort(DialogRef dialog)
												{
													GetPort(&fCurPort);

													if (fCurPort != GetDialogPort(dialog))
														SetPortDialogPort(dialog);
												}
	#endif
				~SetTempPort()	{ ::SetPort(fCurPort); }

		private:	
			GrafPtr fCurPort;	
	};

	inline NMUInt32
	RoundLong(NMUInt32 value) { return ((value + 3) & ~3); }

	inline void
	AlignLong(unsigned char * &ptr) { ptr = (unsigned char *) RoundLong((NMUInt32) ptr);  }


extern "C" {
#endif // __cplusplus

	extern void			RadixSort(unsigned char ** labelsList, NMUInt32 listSize, NMUInt32 maxLabelSize);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __UTILITY__
