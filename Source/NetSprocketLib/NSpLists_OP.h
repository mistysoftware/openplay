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

#ifndef __NSPLISTS__
#define __NSPLISTS__

//	------------------------------	Includes

	#include "OpenPlay.h"
	#include "CEndpoint_OP.h"
	#include "NSp_InterruptSafeList.h"

//	------------------------------	Public Types

	class PlayerListItem : public NSp_InterruptSafeListMember
	{
	public:
		 PlayerListItem();
		~PlayerListItem();
		
	public: 
		NSpPlayerID		id;
		CEndpoint		*endpoint;
		NSpPlayerInfoPtr info;
	};


	class GroupListItem : public NSp_InterruptSafeListMember
	{
	public:
		GroupListItem();
		~GroupListItem();

		NMBoolean		AddPlayer(PlayerListItem *inPlayer);
		NMBoolean		RemovePlayer(NSpPlayerID inPlayer);
	public:	
		NSpGroupID			id;
		NMUInt32			playerCount;
		NSp_InterruptSafeList	*players;
	};

	class UInt32ListMember : public NSp_InterruptSafeListMember
	{
	public:
		UInt32ListMember(NMUInt32 inItem) {mValue = inItem;}
		~UInt32ListMember() {}

		inline NMUInt32 GetValue(void) {return mValue;}
	protected:
		NMUInt32	mValue;
	};

	class SendQItem : public NMLink
	{
	public:
		SendQItem(void *inData, NMUInt32 inBytesSent);
		~SendQItem();
		
		void	AdvanceBuffPtr(NMUInt32 inBytes);
		NMUInt32	BytesLeft();
		
		void			*mData;
		NMUInt32 		mMessageLength;
		NMUInt32		mTotalSent;
	};

#endif // __NSPLISTS__
