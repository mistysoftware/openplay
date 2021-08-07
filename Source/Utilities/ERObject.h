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

#ifndef __EROBJECT__
#define __EROBJECT__

//	------------------------------	Includes


	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#ifndef __NETMODULE__
	#include 			"NetModule.h"
	#endif

	#include "CEndpoint_OP.h"

	#ifdef OP_API_NETWORK_OT
		#include <OpenTptAppleTalk.h>
		#include <OpenTptInternet.h>
	#endif
	
//	------------------------------	Public Types


	class ERObject : public NMLink
	{
	public:
		ERObject();
		ERObject(NSpMessageHeader *inMessage, NMUInt32 inMaxLen);
		~ERObject();
		
		void	Clear();
		
		NMBoolean		CopyNetMessage(NMUInt8 *inBuffer, NMUInt32 inLen);
		NMBoolean		CopyNetMessage(NSpMessageHeader *inMessage);
		NMBoolean		SetNetMessage(NSpMessageHeader *inMessage, NMUInt32 inMaxLen);
		inline void		SetTimeReceived(NMUInt32 inTime) {mTimeReceived = inTime;}
		NSpMessageHeader 	*RemoveNetMessage(void);
		
		void	SetEndpoint(CEndpoint *inEndpoint);
		void	SetCookie(void *inCookie);
		
		inline	NSpMessageHeader 	*PeekNetMessage() {return mMessage;}
		inline 	CEndpoint 			*GetEndpoint() { return mEndpoint;}
		inline	NMUInt32			GetMaxMessageLen() {return mMaxMessageLen;}
		inline	NMUInt32			GetTimeReceived() {return mTimeReceived;}
		inline	void				*GetCookie() {return mCookie;}
		
	protected:
		NSpMessageHeader	*mMessage;
		NMUInt32			mMaxMessageLen;
		NMUInt32			mTimeReceived;
		CEndpoint			*mEndpoint;
		void				*mCookie;
	};

//	------------------------------	Public Functions

	extern void RecursiveReverse(ERObject **headRef);
	extern void NewReverse(ERObject **headRef);
	

#endif // __EROBJECT__
