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

#ifndef __IPENUMERATOR__
#define __IPENUMERATOR__

//	------------------------------	Includes

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#include "NetModuleIP.h"
	#include "TPointerArray.h"

//	------------------------------	Public Definitions
//	------------------------------	Public Types

	class IPEnumerationItemPriv
	{
	public:
		IPEnumerationItemPriv(IPEnumerationResponsePacket * inPacket);
		~IPEnumerationItemPriv();
		NMUInt32					age;	// in milliseconds
		NMEnumerationItem			userEnum;
		IPEnumerationResponsePacket	packet;
	} ;


	class IPEnumerator
	{
	public:
		 		 IPEnumerator(NMIPConfigPriv *inConfig, NMEnumerationCallbackPtr inCallback, void *inContext, NMBoolean inActive);
				~IPEnumerator();
				
		enum	{kMaxEnumResponseItemAge = 4000, kMaxTimeBetweenPings = 1000};	// milliseconds
				
		virtual	NMErr	StartEnumeration(void) = 0;
		virtual NMErr	IdleEnumeration(void) = 0;
		virtual NMErr	EndEnumeration(void) = 0;
		
		virtual NMErr	SendQuery(void) = 0;
				NMErr	SetConfigFromEnumerationItem(NMIPConfigPriv *ioConfig, IPEnumerationItemPriv *inItem);
					
	protected:
		void	HandleReply(NMUInt8 *inData, NMUInt32 inLen, InetAddress *inAddress);
		void	IdleList(NMUInt32 timeSinceLastEnum);
		
	protected:
		NMEnumerationCallbackPtr	mCallback;
		void 						*mContext;
		NMIPConfigPriv				mConfig;
		NMBoolean					bActive;
		TPointerArray				mList;
		
		NMUInt8						mPacketData[kQuerySize];
	};

#endif	// __IPENUMERATOR__

