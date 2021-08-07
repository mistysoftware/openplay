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


/*
	Notes:
	
	These functions will only be used by non-big-endian platforms, and they will
	be used on both send and receive, so that proper byte order will be achieved
	regardless of what combination of send & receive platforms we have.
		
*/


//	------------------------------	Includes

#include "ByteSwapping.h"


//----------------------------------------------------------------------------------------
// SwapBytesForSend
//----------------------------------------------------------------------------------------

void SwapBytesForSend(NSpMessageHeader *inMessage)
{
	//Ä	Some system messages have content that requires byte-swapping...
	SwapSystemMessageContent(inMessage);
	//	Note:	Since we know nothing about the content of the user-defined messages,
	//	the user will be responsible for byte-swapping all variables that require it.
	
	//Ä	All headers require byte-swapping...	
	SwapHeaderByteOrder(inMessage);
}

//----------------------------------------------------------------------------------------
// SwapSystemMessageContent
//----------------------------------------------------------------------------------------

void SwapSystemMessageContent(NSpMessageHeader *inMessage)
{
	switch (inMessage->what)
	{
		case kNSpJoinDenied:
			// Join denied message requires no byte-swapping.
			break;

		case kNSpJoinRequest:
			SwapJoinRequest(inMessage);
			break;

		case kNSpJoinApproved:
			SwapJoinApproved(inMessage, true);	// "True" means message is "outgoing".
			break;

		case kNSpPlayerJoined:
			SwapPlayerJoined(inMessage, true);	// "True" means message is "outgoing".
			break;
			
		case kNSpPlayerLeft:
			SwapPlayerLeft(inMessage);
			break;

		case kNSCreateGroup:
		case kNSDeleteGroup:	
			SwapCreateGroup(inMessage);	// Delete group and create group messages
			break;						// have identical structures.
		

		case kNSpGameTerminated:
			// Game terminated message requires no byte-swapping.
			break;

		case kNSAddPlayerToGroup:
		case kNSRemovePlayerFromGroup:			// Add and remove player to/from group
			SwapAddPlayerToGroup(inMessage);	// have identical structures.
			break;

		case kNSPauseGame:
			// Pause game message requires no byte-swapping.
			break;

		case kNSResumeGame:
			// Resume game message requires no byte-swapping.
			break;

		case kNSpPlayerTypeChanged:
			SwapPlayerTypeChanged(inMessage);
			break;
	}
}

//----------------------------------------------------------------------------------------
// SwapHeaderByteOrder
//----------------------------------------------------------------------------------------

void SwapHeaderByteOrder(NSpMessageHeader *inMessage)	// DONE
{	
	inMessage->version = SWAP4(inMessage->version);
	inMessage->what = SWAP4(inMessage->what);
	inMessage->from = SWAP4(inMessage->from);
	inMessage->to = SWAP4(inMessage->to);
	inMessage->id = SWAP4(inMessage->id);
	inMessage->when = SWAP4(inMessage->when);
	inMessage->messageLen = SWAP4(inMessage->messageLen);
}

//----------------------------------------------------------------------------------------
// SwapJoinRequest
//----------------------------------------------------------------------------------------

void SwapJoinRequest(NSpMessageHeader *inMessage)	// DONE
{
#if !big_endian

	NSpJoinRequestMessage	*joinRequestPtr;
	
	joinRequestPtr = (NSpJoinRequestMessage *) inMessage;
	
	joinRequestPtr->theType = SWAP4(joinRequestPtr->theType);
	joinRequestPtr->customDataLen = SWAP4(joinRequestPtr->customDataLen);
	
	//  The custom data itself is an unknown structure supplied and used only by
	//	the user.  He must take care of byteswapping requirements himself.
	
#else
	UNUSED_PARAMETER (inMessage)
#endif	// big_endian == false
}

//----------------------------------------------------------------------------------------
// SwapJoinApproved
//----------------------------------------------------------------------------------------

void	SwapJoinApproved(NSpMessageHeader *inMessage, NMBoolean outgoing)
{
#if !big_endian

	TJoinApprovedMessagePrivate		*joinApprovedPtr;
	NMUInt8							*dataPtr;
	NSpPlayerInfoPtr				playerInfoPtr;
	NSpGroupInfoPtr					groupInfoPtr;
	NMUInt32						playerIndex;
	NMUInt32						groupIndex;
	
	joinApprovedPtr = (TJoinApprovedMessagePrivate *) inMessage;
	
	joinApprovedPtr->receivedTimeStamp = SWAP4(joinApprovedPtr->receivedTimeStamp);
	joinApprovedPtr->groupIDStartRange = SWAP4(joinApprovedPtr->groupIDStartRange);
	

	// In order to loop over the players and groups, we need to get the appropriate
	// values from the playerCount and groupCount fields.  If this is an
	// incoming message, then those field must be byte-swapped first.  Otherwise,
	// we must wait to byte-swap them after the players and group lists have been
	// byte-swapped...

	if (outgoing == false)
	{
		joinApprovedPtr->playerCount = SWAP4(joinApprovedPtr->playerCount);
		joinApprovedPtr->groupCount = SWAP4(joinApprovedPtr->groupCount);
	}
	
	// Now cycle through the data and byte-swap whatever you need to...
	
	dataPtr = (NMUInt8 *) &joinApprovedPtr->data;
	
	// Work on the playerInfo structures...
	// Note:  Only the first three fields of each player info structure are
	// present in this message, because the other fields are regarding group
	// membership, which is redundant with the group info that is contained
	// below.
	
	for (playerIndex = 0; playerIndex < joinApprovedPtr->playerCount; playerIndex++)
	{
		playerInfoPtr = (NSpPlayerInfoPtr) dataPtr;
	
		playerInfoPtr->id = SWAP4(playerInfoPtr->id);
		playerInfoPtr->type = SWAP4(playerInfoPtr->type);

		dataPtr += kJoinApprovedPlayerInfoSize;
	}
	
	// Work on the groupInfo structures...
	
	for (groupIndex = 0; groupIndex < joinApprovedPtr->groupCount; groupIndex++)
	{
		groupInfoPtr = (NSpGroupInfoPtr) dataPtr;
	
		groupInfoPtr->id = SWAP4(groupInfoPtr->id);
		
		if (outgoing == false)
			groupInfoPtr->playerCount = SWAP4(groupInfoPtr->playerCount);

		for (playerIndex = 0; playerIndex < groupInfoPtr->playerCount; playerIndex++)
			groupInfoPtr->players[playerIndex] = SWAP4(groupInfoPtr->players[playerIndex]);
		
		dataPtr += sizeof(NSpGroupInfo) + ((groupInfoPtr->playerCount == 0) ? 0 : groupInfoPtr->playerCount - 1) * sizeof(NSpPlayerID);
		
		if (outgoing)
			groupInfoPtr->playerCount = SWAP4(groupInfoPtr->playerCount);
		
	}

//	MessageBox(NULL, "Done", "Hack", MB_OK);  //crt

	if (outgoing)
	{
		joinApprovedPtr->playerCount = SWAP4(joinApprovedPtr->playerCount);
		joinApprovedPtr->groupCount = SWAP4(joinApprovedPtr->groupCount);
	}

#else
	UNUSED_PARAMETER(inMessage);
	UNUSED_PARAMETER(outgoing);
#endif	// big_endian == false
}

//----------------------------------------------------------------------------------------
// SwapPlayerJoined
//----------------------------------------------------------------------------------------

void	SwapPlayerJoined(NSpMessageHeader *inMessage, NMBoolean outgoing)	// DONE
{
#if !big_endian

	NSpPlayerJoinedMessage		*playerJoinedPtr;
	NMUInt32					groupIndex;
	
	
	playerJoinedPtr = (NSpPlayerJoinedMessage *) inMessage;
	
	playerJoinedPtr->playerCount = SWAP4(playerJoinedPtr->playerCount);
	
	// Now we have to fix the player info structure...
	
	playerJoinedPtr->playerInfo.id = SWAP4(playerJoinedPtr->playerInfo.id);
	playerJoinedPtr->playerInfo.type = SWAP4(playerJoinedPtr->playerInfo.type);
	
	// In order to loop over the groups that the player is in, we have to
	// get the appropriate value out of the groupCount field.  If this is an
	// incoming message, then that field must be byte-swapped first.  Otherwise,
	// we must wait to byte-swap it after the groups list has been swapped...
	
	if (outgoing == false)
		playerJoinedPtr->playerInfo.groupCount = SWAP4(playerJoinedPtr->playerInfo.groupCount);

	for (groupIndex = 0; groupIndex < playerJoinedPtr->playerInfo.groupCount; groupIndex++)
		playerJoinedPtr->playerInfo.groups[groupIndex] = 
				SWAP4(playerJoinedPtr->playerInfo.groups[groupIndex]);
	
	if (outgoing)
		playerJoinedPtr->playerInfo.groupCount = SWAP4(playerJoinedPtr->playerInfo.groupCount);
	
#else
	UNUSED_PARAMETER(inMessage);
	UNUSED_PARAMETER(outgoing);
#endif	// big_endian == false
}

//----------------------------------------------------------------------------------------
// SwapPlayerLeft
//----------------------------------------------------------------------------------------

void	SwapPlayerLeft(NSpMessageHeader *inMessage)	// DONE
{
#if !big_endian
	NSpPlayerLeftMessage	*playerLeftPtr;

	playerLeftPtr = (NSpPlayerLeftMessage *) inMessage;
	
	playerLeftPtr->playerID = SWAP4(playerLeftPtr->playerID);
	playerLeftPtr->playerCount = SWAP4(playerLeftPtr->playerCount);
	
#else
	UNUSED_PARAMETER (inMessage);
#endif	// big_endian == false
}

//----------------------------------------------------------------------------------------
// SwapCreateGroup
//----------------------------------------------------------------------------------------

void	SwapCreateGroup(NSpMessageHeader *inMessage)	// DONE
{
#if !big_endian

	TCreateGroupMessage *groupCreatePtr;
	
	groupCreatePtr = (TCreateGroupMessage *) inMessage;
	
	groupCreatePtr->id = SWAP4(groupCreatePtr->id);
	groupCreatePtr->requestingPlayer = SWAP4(groupCreatePtr->requestingPlayer);
	
#else
	UNUSED_PARAMETER(inMessage);
#endif	// big_endian == false
}

//----------------------------------------------------------------------------------------
// SwapAddPlayerToGroup
//----------------------------------------------------------------------------------------

void	SwapAddPlayerToGroup(NSpMessageHeader *inMessage)	// DONE
{
#if !big_endian
	TAddPlayerToGroupMessage *addPlayerGroupPtr;
	
	addPlayerGroupPtr = (TAddPlayerToGroupMessage *) inMessage;
	
	addPlayerGroupPtr->group = SWAP4(addPlayerGroupPtr->group);
	addPlayerGroupPtr->player = SWAP4(addPlayerGroupPtr->player);
	
#else
	UNUSED_PARAMETER(inMessage);
#endif	// big_endian == false
}

//----------------------------------------------------------------------------------------
// SwapPlayerTypeChanged
//----------------------------------------------------------------------------------------

void	SwapPlayerTypeChanged(NSpMessageHeader *inMessage)	// DONE
{
#if !big_endian

	NSpPlayerTypeChangedMessage *playerTypeChangedPtr;
	
	playerTypeChangedPtr = (NSpPlayerTypeChangedMessage *) inMessage;
	
	playerTypeChangedPtr->player = SWAP4(playerTypeChangedPtr->player);
	playerTypeChangedPtr->newType = SWAP4(playerTypeChangedPtr->newType);

#else
	#pragma unused (inMessage)
#endif	// big_endian == false
}

