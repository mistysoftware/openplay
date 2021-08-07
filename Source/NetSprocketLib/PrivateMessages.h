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

#ifndef __PRIVATEMESSAGES__
#define __PRIVATEMESSAGES__

//	------------------------------	Includes

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#include "NSpPrefix.h"

//	------------------------------	Public Definitions

	// Private message 'what' types
	enum 
	{
		kPrivateMessage = 		0xC0000000,
		kNSCreateGroup = 		kPrivateMessage | 0x00000002,
		kNSHostMakeGroupReq = 	kPrivateMessage | 0x00000003,
		kNSDeleteGroup = 		kPrivateMessage | 0x00000004,
		kNSAddPlayerToGroup =	kPrivateMessage | 0x00000005,
		kNSRemovePlayerFromGroup =	kPrivateMessage | 0x00000006,
		kRTTPing =				kPrivateMessage | 0x00000007,
		kRTTPingReply = 		kPrivateMessage | 0x00000008,
		kThruputPing = 			kPrivateMessage | 0x00000009,
		kThruputPingReply =		kPrivateMessage | 0x0000000A,
		kNSPauseGame = 			kPrivateMessage | 0x0000000B,
		kNSResumeGame = 		kPrivateMessage | 0x0000000C,
		kNSBecomeHostRequest =	kPrivateMessage | 0x0000000D,
		kNSBecomeHostReply =	kPrivateMessage | 0x0000000E,
		kNSHostTransferInfo =	kPrivateMessage | 0x0000000F
	};

	enum {kNSpAllGroups = 0};

//	------------------------------	Public Types

	typedef struct TJoinApprovedMessagePrivate
	{
		NSpMessageHeader	header;
		NMUInt32			receivedTimeStamp;
		NSpGroupID			groupIDStartRange;
		NMUInt32			playerCount;
		NMUInt32			groupCount;
		NMUInt8				data[kVariableLengthArray];
	} TJoinApprovedMessagePrivate;


	typedef struct TCreateGroupMessage
	{
		NSpMessageHeader	header;
		NSpGroupID			id;
		NSpPlayerID			requestingPlayer;
	} TCreateGroupMessage;

	typedef struct TAddPlayerToGroupMessage
	{
		NSpMessageHeader	header;
		NSpGroupID			group;
		NSpPlayerID			player;
	} TAddPlayerToGroupMessage;


	typedef TCreateGroupMessage TDeleteGroupMessage;
	typedef NSpMessageHeader	THostMakeGroupReq;
	typedef TAddPlayerToGroupMessage TRemovePlayerFromGroupMessage;
	typedef NSpMessageHeader	TPauseGameMessage;
	typedef NSpMessageHeader	TResumeGameMessage;

	typedef struct TThruputMessage
	{
		NSpMessageHeader	header;
		NMUInt32			count;
	} TThruputMessage;

	const NMUInt32 kJoinApprovedPlayerInfoSize = sizeof(NSpPlayerInfo) - sizeof(NSpGroupID) - sizeof(NMUInt32);

	enum { kSendFlagsMask = 0x00FFFFFF};
	enum { kLocalSendFlagMask = 0x0000000F};
	enum { kVersionMask = 0xFF000000};

	enum
	{
		kNSpPrivBadMessageVersionErr = -100,
		kNSpPrivReceiveFailedErr
	};

#endif // __PRIVATEMESSAGES__
