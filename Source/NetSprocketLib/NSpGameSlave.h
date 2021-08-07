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

#ifndef __NSPGAMESLAVE__
#define __NSPGAMESLAVE__

//	------------------------------	Includes

	#include "NSpGame.h"
	#include "CATEndpoint_OP.h"
	#include "CIPEndpoint_OP.h"

//	------------------------------	Public Types

	class NSpGameSlave : public NSpGame
	{
	public:
		enum { class_ID = 'Slav' };

		NSpGameSlave(NSpFlags inFlags);
		~NSpGameSlave();

	//	Method for joining
				NMErr	Join(NMConstStr31Param inPlayerName, NMConstStr31Param inPassword, NSpPlayerType inType,
									void *inCustomData, NMUInt32 inCustomDataLen,  NSpAddressReference inAddress);

	//	Methods for handling events

		virtual void		HandleEvent(ERObject *inERObject, CEndpoint *inEndpoint, void *inCookie);
		virtual void		HandleNewEvent(ERObject *inERObject, CEndpoint *inEndpoint, void *inCookie);

		NMUInt32 GetBacklog( void );

		NMErr 	SendJoinRequest(NMConstStr31Param inPlayerName, NMConstStr31Param inPassword, 
											NSpPlayerType inType, void *inCustomData, NMUInt32 inCustomDataLen);
		virtual NMErr	SendUserMessage(NSpMessageHeader *inMessage, NSpFlags inFlags);
		virtual NMErr	SendTo(NSpPlayerID inTo, NMSInt32 inWhat, void *inData, NMUInt32 inLen, NSpFlags inFlags);
		virtual NMErr	PrepareForDeletion(NSpFlags inFlags);
		
		virtual	NMErr	HandleEndpointDisconnected(CEndpoint *inEndpoint);
		virtual void	IdleEndpoints(void);

	protected:
		virtual	NMBoolean	RemovePlayer(NSpPlayerID inPlayer, NMBoolean inDisconnect);

		NMErr		MakeGroupListFromJoinApprovedMessage(NSpGroupInfoPtr *inGroups, NMUInt32 inCount);	
		NMBoolean	HandleJoinApproved(TJoinApprovedMessagePrivate *inMessage, NMUInt32 inTimeReceived);		
		NMBoolean	HandleJoinDenied(NSpJoinDeniedMessage *inMessage);	
		NMBoolean	HandlePlayerJoined(NSpPlayerJoinedMessage *inMessage);
		NMBoolean	HandleGameTerminated(NSpMessageHeader *inMessage);
		NMBoolean	ProcessSystemMessage(NSpMessageHeader *inMessage);

		CEndpoint		*mEndpoint;
	};


#endif // __NSPGAMESLAVE__
