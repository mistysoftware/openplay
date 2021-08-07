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


/*
	Some theory of operation:
	
	It may seem strange that we create another endpoint when we create the host's player, but there
	is a good reason for this.  Because the endpoints aren't re-entrant, we need to make sure that we
	never try to send anything while we are in the endpoint's notifier.  Doing so will cause a state error.
	If this is a headless server, we need to be careful not to use the SendUserMessage function, since
	it relies on this player endpoint, which is not created for the headless server.

	If you're looking at rewriting this code, I recommend we split the server and the player into
	different processes, so that the server code can run completely separate from the client code.

	Because the server does routing between protocols, NetSprocket games could be a security hole
	in a firewall.  Since they can take traffice from one (outside) net and put it on an inside net
*/

#include "NSpPrefix.h"
#include "NSpGameMaster.h"
#include "NetSprocketLib.h"
#include "ByteSwapping.h"

#ifndef __NETMODULE__
	#include "NetModule.h"
#endif

#ifdef OP_API_NETWORK_OT
	#include <OpenTptAppleTalk.h>
	#include <OpenTptInternet.h>
	#include <OpenTptSerial.h>
#endif

#include "String_Utils.h"



#define	kCustomLocalJoinRequest		0xFFFFFFFF


//----------------------------------------------------------------------------------------
// NSpGameMaster::NSpGameMaster
//----------------------------------------------------------------------------------------

NSpGameMaster::NSpGameMaster(
		NMUInt32		inMaxPlayers,
		NMConstStr31Param	inGameName,
		NMConstStr31Param	inPassword, 
		NSpTopology		inTopology,
		NSpFlags		inFlags) : NSpGame(inMaxPlayers, inGameName, inPassword, inTopology, inFlags)
{

	mNextAvailablePlayerNumber = 1;
	mNextPlayersGroupStartRange = -1024;
	
	//Ä	Initialize our OT stuff
	bAdvertisingAT = false;
	bAdvertisingIP = false;
	mAdvertisingATEndpoint = NULL;
	mAdvertisingIPEndpoint = NULL;
	mPlayersEndpoint = NULL;
	mJoinRequestHandler = NULL;
	mJoinRequestContext = NULL;
	mObjectID = NSpGameMaster::class_ID;	
	mSystemPlayerIterator = new NSp_InterruptSafeListIterator(*mPlayerList);
	mSystemGroupIterator = new NSp_InterruptSafeListIterator(*mGroupList);
	
	bPasswordRequired =  ((inPassword == NULL) || (inPassword[0] == 0)) ? false : true;

	mLowPriorityPollCount = kLowPriorityPollFrequency;
	mProtocols = 0;
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::~NSpGameMaster
//----------------------------------------------------------------------------------------

NSpGameMaster::~NSpGameMaster()
{
	if (mAdvertisingATEndpoint)
	{
		DEBUG_PRINT("Calling Close in NSpGameMaster::~NSpGameMaster (1)");
		mAdvertisingATEndpoint->Close();
		mAdvertisingATEndpoint = NULL;
	}

	if (mAdvertisingIPEndpoint)
	{
		DEBUG_PRINT("Calling Close in NSpGameMaster::~NSpGameMaster (2)");
		mAdvertisingIPEndpoint->Close();
		mAdvertisingIPEndpoint = NULL;
	}

	if (mPlayersEndpoint)
	{
		DEBUG_PRINT("Calling Close in NSpGameMaster::~NSpGameMaster (3)");
		mPlayersEndpoint->Close();
		mPlayersEndpoint = NULL;
	}

	if (mSystemPlayerIterator)
		delete mSystemPlayerIterator;

	if (mSystemGroupIterator)
		delete mSystemGroupIterator;
}

#ifdef OP_API_NETWORK_OT

//----------------------------------------------------------------------------------------
// NSpGameMaster::HostAT
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::HostAT(NSpProtocolPriv *inProt)
{
	NMErr	status = kNMNoError;

	if (bAdvertisingAT)
	{
		status = kNSpAlreadyAdvertisingErr;
	}
	else
	{
		if (mAdvertisingATEndpoint == NULL)
		{
			mAdvertisingATEndpoint = new COTATEndpoint(this);
			if (mAdvertisingATEndpoint == NULL){
				status = kNSpMemAllocationErr;
				goto error;
			}

			status = mAdvertisingATEndpoint->InitAdvertiser(inProt);
			if (status)
				goto error;

			bAdvertisingAT = true;
		}
		else
		{
			bAdvertisingAT = mAdvertisingATEndpoint->Host(true);
		}
	}

error:
	if (status)
	{
		NMErr code = status;
		if (mAdvertisingATEndpoint != NULL)
		{
			DEBUG_PRINT("Calling Close in NSpGameMaster::HostAT");
			mAdvertisingATEndpoint->Close();
		}

		return (code);
	}

	return (status);
}

#endif //	OP_API_NETWORK_OT

//----------------------------------------------------------------------------------------
// NSpGameMaster::HostIP
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::HostIP(NSpProtocolPriv *inProt)
{
	NMErr	status = kNMNoError;

	if (bAdvertisingIP)
	{
		status = kNSpAlreadyAdvertisingErr;
	}
	else
	{
		if (mAdvertisingIPEndpoint == NULL)
		{
			mAdvertisingIPEndpoint = new COTIPEndpoint(this);
			if (mAdvertisingIPEndpoint == NULL){
				status = kNSpMemAllocationErr;
				goto error;
			}
			

			status = mAdvertisingIPEndpoint->InitAdvertiser(inProt);
			if (status)
				goto error;
			
			bAdvertisingIP = true;
		}
		else
		{
			bAdvertisingIP = mAdvertisingIPEndpoint->Host(true);
		}
	}

error:
	if (status)
	{
		if (mAdvertisingIPEndpoint != NULL)
		{
			DEBUG_PRINT("Calling Close in NSpGameMaster::HostIP");
			mAdvertisingIPEndpoint->Close();
		}
	}
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::UnHostAT
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::UnHostAT(void)
{
NMErr status = kNMNoError;
	
	if (!bAdvertisingAT || mAdvertisingATEndpoint == NULL)
		status = kNSpNotAdvertisingErr;
	else
		bAdvertisingAT = mAdvertisingATEndpoint->Host(false);
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::UnHostIP
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::UnHostIP(void)
{
NMErr status = kNMNoError;
	
	if (!bAdvertisingIP || mAdvertisingIPEndpoint == NULL)
		status = kNSpNotAdvertisingErr;
	else
		bAdvertisingIP = mAdvertisingIPEndpoint->Host(false);
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::AddLocalPlayer
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::AddLocalPlayer(NMConstStr31Param inPlayerName, NSpPlayerType inPlayerType, 
								NSpProtocolPriv *theProt)
{
	NMErr	status = kNMNoError;
	CEndpoint	*ep = NULL;
	
	
	//Ä	Check the flags.  Create a player on this workstation if necessary
	if (inPlayerName == NULL || inPlayerName[0] == 0)
	{
		bHeadlessServer = true;
		mPlayerID = kNSpMasterEndpointID;
	}
	else
	{
		bHeadlessServer = false;

		//Ä	Create our endpoint
		if (bAdvertisingAT)
		{
#ifdef OP_API_NETWORK_OT
			ep = mAdvertisingATEndpoint;
			mPlayersEndpoint = new COTATEndpoint(this);
			if (mPlayersEndpoint == NULL){
				status = kNSpMemAllocationErr;
				goto error;
			}
#else
			return (kNSpFeatureNotImplementedErr);

#endif
		}
		else if (bAdvertisingIP)
		{
			ep = mAdvertisingIPEndpoint;
			mPlayersEndpoint = new COTIPEndpoint(this);
			if (mPlayersEndpoint == NULL){
				status = kNSpMemAllocationErr;
				goto error;
			}
		}
		else
		{
			return (kNSpNotAdvertisingErr);
		}
			
		//Ä	Initialize it
		status = mPlayersEndpoint->InitNonAdvertiser(theProt);
		if (status)
			goto error;
	
		status = SendJoinRequest(inPlayerName, inPlayerType);
		if (status)
			goto error;

error:
		if (status)
		{
			NMErr code = status;
			return (code);
		}
	}

	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::InstallJoinRequestHandler
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::InstallJoinRequestHandler(NSpJoinRequestHandlerProcPtr inHandler, void *inContext)
{
NMErr status = kNMNoError;
	
	mJoinRequestHandler = inHandler;
	mJoinRequestContext = inContext;
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::IsCorrectPassword
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameMaster::IsCorrectPassword(const NMUInt8 *inPassword)
{
//	return ((::CompareString(inPassword, mGameInfo.password, NULL) == 0) ? true : false);
	return ::doComparePStr(inPassword, mGameInfo.password);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::HandleJoinRequest
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameMaster::HandleJoinRequest(
		NSpJoinRequestMessage	*inMessage,
		CEndpoint				*inEndpoint,
		void					*inCookie,
		NMUInt32				inReceivedTime)
{
	NMBoolean		handled = true;
	NMBoolean		localRequest = false;
	NMBoolean		approved = true;
	NSpPlayerID 	player;
	NMUInt8		 	message[256];
	NMErr			status = kNMNoError;
	CEndpoint 		*commEndpoint = NULL;

	// dair, added NSpJoinResponseMessage support
	NSpJoinResponseMessage	msgJoinResponse;

	memset(&msgJoinResponse, 0x00, sizeof(msgJoinResponse));

	msgJoinResponse.header.what       = kNSpJoinResponse;
	msgJoinResponse.header.from       = kNSpMasterEndpointID;
	msgJoinResponse.header.version    = kVersion10Message;
	msgJoinResponse.header.id         = mNextMessageID++;
	msgJoinResponse.header.messageLen = sizeof(NSpJoinResponseMessage);
	
	//	Initialize our message string to be empty
	message[0] = 0;
		
	if (inMessage->customDataLen == kCustomLocalJoinRequest)		//Ä	It's a local request
	{
		approved = true;					//Ä	Always approve the local request
		localRequest = true;
	}
	else if (approved && mGameInfo.currentPlayers >= mGameInfo.maxPlayers)
	{
		doCopyC2PStr("Maximum allowed players already in game", message);
		approved = false;	
	}	
	else if (approved && mJoinRequestHandler != NULL)		//Ä	Call the custom join handler, if one is installed
	{
		// dair, added NSpJoinResponseMessage support
		approved = ((NSpJoinRequestHandlerProcPtr) mJoinRequestHandler)((NSpGameReference) this->GetGameOwner(), inMessage, mJoinRequestContext, message, &msgJoinResponse);
	}

	//Ä	Reject them if they gave an incorrect password
	else if (approved && bPasswordRequired && !IsCorrectPassword(inMessage->password))
	{
		doCopyC2PStr("Incorrect password", message);
		approved = false;
	}
	
	if (approved)
	{
	
		commEndpoint = inEndpoint->Clone(inCookie);
		if (commEndpoint == NULL){
			status = kNSpMemAllocationErr;
			goto error;
		}
	
		player = mNextAvailablePlayerNumber++;

		//Ä	Add an entry to the playermap list
		NSpPlayerInfo	info;

		info.id = player;
		info.type = inMessage->theType;
		info.groupCount = 0;
		info.groups[0] = 0;
		doCopyPStrMax(inMessage->name, info.name, 31);
		
		AddPlayer(&info, commEndpoint);

		if (localRequest)
		{
			mPlayerID = player;
			status = kNMNoError;		//Ä	Don't send the join approved to a local joiner
		}
		else
		{
				// dair, added NSpJoinResponseMessage support
			if (msgJoinResponse.responseDataLen != 0)
			{
				msgJoinResponse.header.to = player;
				status                    = commEndpoint->SendMessage(&msgJoinResponse.header, ((NMUInt8 *) &msgJoinResponse) + sizeof(NSpMessageHeader), kNSpSendFlag_Registered);
			}

			//Ä	Include the old player map for the new player	
			status = SendJoinApproved(commEndpoint, player, inReceivedTime);
		}
		
		if (status == kNMNoError)
		{
			//Ä	Now notify everyone else that there is a new player
			//Ä	If this fails, we could get out of sync
			status = NotifyPlayerJoined(&info);
		}
		else 
		{
			RemovePlayer(player, true);		//Ä	We couldn't send the approval for some reason.  Undo the add.
			DEBUG_PRINT("Failed to send Join Approved.");
		}
	}
	else
	{
			// dair, added NSpJoinResponseMessage support
		if (msgJoinResponse.responseDataLen != 0)
			status = inEndpoint->SendMessage(&msgJoinResponse.header, ((NMUInt8 *) &msgJoinResponse) + sizeof(NSpMessageHeader), kNSpSendFlag_Registered);

		status = SendJoinDenied(inEndpoint, inCookie, message);
	}

error:
	if (status)
	{
		NMErr code = status;
		(void) code;
	}
	
	return (handled);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::NotifyPlayerJoined
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::NotifyPlayerJoined(NSpPlayerInfo *info)
{
NMErr				status;
NSpPlayerJoinedMessage	theMessage;
		
	NSpClearMessageHeader((NSpMessageHeader *)&theMessage);
	theMessage.header.what = kNSpPlayerJoined;
	theMessage.header.to = kNSpAllPlayers;			
	theMessage.header.messageLen = sizeof(NSpPlayerJoinedMessage);
	theMessage.playerCount = mGameInfo.currentPlayers;
	theMessage.playerInfo = *info;
	
	status = SendSystemMessage(&theMessage.header, kNSpSendFlag_Registered);
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::SendSystemMessage
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::SendSystemMessage(NSpMessageHeader *inMessage, NSpFlags inFlags)
{
	
	inMessage->from = kNSpMasterEndpointID;
	inMessage->version = kVersion10Message;
	inMessage->id = mNextMessageID++;

	return (RouteMessage(inMessage, (NMUInt8 *) inMessage + sizeof (NSpMessageHeader), inFlags));
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::ForwardMessage
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::ForwardMessage(NSpMessageHeader *inMessage)
{
	NSpFlags	flags;
	
	if (mGameState == kStopped)
		return (kNSpGameTerminatedErr);
		
	flags = (inMessage->version & kSendFlagsMask) & ~kLocalSendFlagMask;
		
	return (RouteMessage(inMessage, (NMUInt8 *)inMessage + sizeof (NSpMessageHeader), flags));
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::RouteMessage
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::RouteMessage(NSpMessageHeader *inHeader, NMUInt8 *inBody, NSpFlags inFlags)
{
	NMErr	 						status = kNMNoError;
	NSp_InterruptSafeListIterator	*iter = mSystemPlayerIterator;
	NSp_InterruptSafeListIterator	*groupIter = mSystemGroupIterator;
	NSp_InterruptSafeListMember 	*theItem;
	PlayerListItem					*thePlayer;
	NMBoolean						found = false;
	NMBoolean						performSelfSend = false;
	NSpPlayerID						fromPlayer;
	NSpPlayerID						toPlayer;

	fromPlayer = inHeader->from;
	toPlayer = inHeader->to;
	
	if (toPlayer == kNSpAllPlayers)			//Ä	To all
	{
		//Ä	Send to the local player first, provided he wasn't the sender.
		//	We do this before the loop, because we must not byte-swap a
		//	message to the local player...
		if (fromPlayer != mPlayerID)
			status = DoSelfSend(inHeader, inBody, inFlags);

#if !big_endian
		//Ä	Byte-swap the message for sending now, and don't do it on each call
		//	to SendMessage(), or else we will have the wrong Byte-Order every
		//	other send...
		//  We also must perform the OR function with the inFlags prior to the byte-swap,
		//  rather than after.  These two lines should only be used preceding a call to
		//	SendMessage() with a final argument of "false".
		inHeader->version |= inFlags;
		SwapBytesForSend(inHeader);	// This will byte-swap the header and known system message content.
#endif
		
		//Ä	Now loop through all players...
		iter->Reset();
		while (iter->Next(&theItem))
		{
			thePlayer = (PlayerListItem *)theItem;
			
			if (fromPlayer != thePlayer->id)	//Ä	Don't send it to the sender
			{
				if (thePlayer->id != mPlayerID)		//Ä	Don't byte-swap (already done).
					status = thePlayer->endpoint->SendMessage(inHeader, inBody, inFlags, false);
			}			
		}
	}
	else if (toPlayer == kNSpMasterEndpointID)		//Ä	To the host only
	{
		status = DoSelfSend(inHeader, inBody, inFlags);
	}
	else if (toPlayer == mPlayerID)			//Ä	To us
	{
		status = DoSelfSend(inHeader, inBody, inFlags);
	}
	else if (toPlayer > kNSpAllPlayers)		//Ä	To a specific player that is not us
	{
		//Ä	Find that person
		//Ä	How can we make this faster?
		iter->Reset();

		while (iter->Next(&theItem))
		{
			thePlayer = (PlayerListItem *)theItem;
			if (thePlayer->id == toPlayer)
			{
				if (thePlayer->id == mPlayerID)
					status = DoSelfSend(inHeader, inBody, inFlags);
				else
					status = thePlayer->endpoint->SendMessage(inHeader, inBody, inFlags);
				break;
			}
		}
	}
	else if (toPlayer < kNSpMasterEndpointID)		//Ä	To a group
	{	
		NSp_InterruptSafeListIterator 	*playerIterator = NULL;
		GroupListItem				*theGroup = NULL;
			
		groupIter->Reset();
		
		found = false;	

		//Ä	First find the group in the list
		while (!found && groupIter->Next(&theItem))
		{
			theGroup = (GroupListItem *)theItem;
			if (theGroup->id == inHeader->to)
			{
				found = true;
			}
		}
		
		//Ä	If we didn't find it, bail
		if (!found)
			return 	(kNSpInvalidGroupIDErr);

		//Ä	Now we have the group, lets iterate over the players, sending to them
		//Ä	Iterate through our player list
		playerIterator = new NSp_InterruptSafeListIterator(*theGroup->players);
		if (playerIterator == NULL){
			op_vpause("NSpGameMaster::RouteMessage - playerIterator == NULL");
			status = kNSpMemAllocationErr;
			goto error;
		}
		
#if !big_endian
		//Ä	Byte-swap the message for sending now, and don't do it on each call
		//	to SendMessage(), or else we will have the wrong Byte-Order every
		//	other send...
		//  We also must perform the OR function with the inFlags prior to the byte-swap,
		//  rather than after.  These two lines should only be used preceding a call to
		//	SendMessage() with a final argument of "false".  (Perhaps we should change it so that
		//	this Oring *always* occurs outside of SendMessage() !?!?)
		inHeader->version |= inFlags;
		SwapBytesForSend(inHeader);	// This will byte-swap the header and known system message content.
#endif
		
		//Ä	Now loop through all players in the group...
		while (playerIterator->Next(&theItem))
		{
			thePlayer = (PlayerListItem *)((UInt32ListMember *)theItem)->GetValue();

			if (thePlayer->id != fromPlayer)
			{
				if (thePlayer->id == mPlayerID)		//Ä	Will need to byte-swap below.
					performSelfSend = true;
				else								//Ä	Don't byte-swap (already done above).
					status = thePlayer->endpoint->SendMessage(inHeader, inBody, inFlags, false);
				
				if (status != kNMNoError)
				{
#if INTERNALDEBUG
					DEBUG_PRINT("SendMessage to %d returned %ld", thePlayer->id, status);
#endif
				}
			}
		}
		
		//Ä	If the local player was in the group, then you have to send the message
		//	to him as well.  But since the message is currently byte-swapped, we
		//	first need to swap it back...
		if (performSelfSend)
		{
#if !big_endian
			SwapBytesForSend(inHeader);	// This will byte-swap the header and known system message content.
#endif
			status = DoSelfSend(inHeader, inBody, inFlags);
		}
		delete playerIterator;
	}
	
error:
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::SendUserMessage
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::SendUserMessage(NSpMessageHeader *inMessage, NSpFlags inFlags)
{
	NMErr						status = kNMNoError;
	NSp_InterruptSafeListIterator	*groupIter = mGroupListIter;
	NSp_InterruptSafeListIterator	*playerIter;
	NSp_InterruptSafeListMember	*theItem;
	PlayerListItem				*thePlayer;
	GroupListItem				*theGroup;
	NSpPlayerID					inTo;
	NMSInt32					inWhat;
	void						*inData;
#if !big_endian
	NMBoolean						swapBack = true;	//?? why here and not in next routine?
#endif

	if (mGameState == kStopped)
		return (kNSpGameTerminatedErr);

	if (bHeadlessServer)
		return (kNSpSendFailedErr);

	inMessage->from = mPlayerID;
	inMessage->version = kVersion10Message;
	inMessage->id = mNextMessageID++;

	inTo = inMessage->to;
	inWhat = inMessage->what;
	inData = (NMUInt8 *)inMessage + sizeof(NSpMessageHeader);
	
	if (inTo == kNSpAllPlayers)			//Ä	To all
	{
		//Ä	First give it to ourselves
		if (inFlags & kNSpSendFlag_SelfSend)
			status = DoSelfSend(inMessage, (NMUInt8 *) inData, inFlags);

		//Ä	Now hand it off to the host to send to everyone
		status = mPlayersEndpoint->SendMessage(inMessage, (NMUInt8 *) inData, inFlags);
	}
	else if (inTo == kNSpMasterEndpointID)		//Ä	To the host only
	{
		status = mPlayersEndpoint->SendMessage(inMessage, (NMUInt8 *) inData, inFlags);
	}
	else if (inTo == mPlayerID)			//Ä	To us
	{
		status = DoSelfSend(inMessage, inData, inFlags);
#if !big_endian
		swapBack = false;				//Ä	Message never got sent & swapped, so don't swap back.
#endif
	}
	else if (inTo > kNSpAllPlayers)		//Ä	To a specific player
	{
		status = mPlayersEndpoint->SendMessage(inMessage, (NMUInt8 *) inData, inFlags);
	}
	else if (inTo < kNSpMasterEndpointID)		//Ä	To a group
	{
		//Ä	If we're a member, send it to ourselves before sending to the host
		groupIter->Reset();

		while (groupIter->Next(&theItem))
		{
			theGroup = (GroupListItem *) theItem;

			if (theGroup->id == inMessage->to)
			{
				playerIter = new NSp_InterruptSafeListIterator(*theGroup->players);

				if (playerIter == NULL)
					break;
				
				while (playerIter->Next(&theItem))
				{
					thePlayer = (PlayerListItem *)(((UInt32ListMember *)theItem)->GetValue());

					if (thePlayer->id == mPlayerID)
					{
						status = DoSelfSend(inMessage, inData, inFlags);
						break;
					}
				}
				delete playerIter;
				break;
			}
		}

		//Ä	Now send it to the host
		status = mPlayersEndpoint->SendMessage(inMessage, (NMUInt8 *) inData, inFlags);
	}

	// The user may end up resending their message or (unwisely) using the fields in the header
	// for some purpose, but they will be shocked to find that the header has been swapped on
	// them!  So if it has been swapped, then swap it back before returning...
	
#if !big_endian
	if (swapBack)
		SwapHeaderByteOrder(inMessage);	
#endif

error:
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::IdleEndpoints
//----------------------------------------------------------------------------------------
void
NSpGameMaster::IdleEndpoints(void)
{
	//fixme - theoretically, we should be idling all our endpoints?
	if (mPlayersEndpoint)
		mPlayersEndpoint->Idle();
}


//----------------------------------------------------------------------------------------
// NSpGameMaster::SendTo
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::SendTo(NSpPlayerID inTo, NMSInt32 inWhat, void *inData, NMUInt32 inLen, NSpFlags inFlags)
{
	NMErr							status = kNMNoError;
	NSpMessageHeader				*headerPtr;
	NMUInt8							*dataPtr = NULL;
	NSp_InterruptSafeListIterator	*groupIter = mGroupListIter;
	NSp_InterruptSafeListIterator	*playerIter;
	NSp_InterruptSafeListMember		*theItem;
	PlayerListItem					*thePlayer;
	GroupListItem					*theGroup;

	if (mGameState == kStopped)
		return (kNSpGameTerminatedErr);

	//€	Don't allow anyone to send a user message on a headless server
	//€	Fixes bug 1366502
	if (bHeadlessServer)
		return (kNSpSendFailedErr);
	
	dataPtr = new NMUInt8[inLen + sizeof(NSpMessageHeader)];	//### $Brian new [] vs. new()
	
	if (dataPtr == NULL)
		return (kNSpSendFailedErr);

	headerPtr = (NSpMessageHeader *) dataPtr;
	
	if (headerPtr == NULL)
		return (kNSpSendFailedErr);
		
	NSpClearMessageHeader(headerPtr);
			
	headerPtr->from = mPlayerID;
	headerPtr->to = inTo;
	headerPtr->version = kVersion10Message;
	headerPtr->id = mNextMessageID++;
	headerPtr->what = inWhat;
	headerPtr->messageLen = inLen + sizeof(NSpMessageHeader);
	
	machine_move_data(inData, ((NMUInt8 *) headerPtr) + sizeof(NSpMessageHeader), inLen);

	if (inTo == kNSpAllPlayers)			//Ä	To all
	{
		//Ä	First give it to ourselves
		if (inFlags & kNSpSendFlag_SelfSend)
			status = DoSelfSend(headerPtr, (NMUInt8 *) inData, inFlags);

		//Ä	Now hand it off to the host to send to everyone
		status = mPlayersEndpoint->SendMessage(headerPtr, (NMUInt8 *) inData, inFlags);
	}
	else if (inTo == kNSpMasterEndpointID)		//Ä	To the host only
	{
		status = mPlayersEndpoint->SendMessage(headerPtr, (NMUInt8 *) inData, inFlags);
	}
	else if (inTo > kNSpAllPlayers)		//Ä	To a specific player
	{
		status = mPlayersEndpoint->SendMessage(headerPtr, (NMUInt8 *) inData, inFlags);
	}
	else if (inTo == mPlayerID)			//Ä	To us
	{
		status = DoSelfSend(headerPtr, inData, inFlags);
	}
	else if (inTo < kNSpMasterEndpointID)		//Ä	To a group
	{
		//Ä	If we're a member, send it to ourselves before sending to the host
		groupIter->Reset();

		while (groupIter->Next(&theItem))
		{
			theGroup = (GroupListItem *) theItem;

			if (theGroup->id == inTo)
			{
				playerIter = new NSp_InterruptSafeListIterator(*theGroup->players);

				if (playerIter == NULL)
					break;
				
				while (playerIter->Next(&theItem))
				{
					thePlayer = (PlayerListItem *)(((UInt32ListMember *)theItem)->GetValue());

					if (thePlayer->id == mPlayerID)
					{
						status = DoSelfSend(headerPtr, inData, inFlags);
						break;
					}
				}
				delete playerIter;
				break;
			}
		}

		//Ä	Now send it to the host
		status = mPlayersEndpoint->SendMessage(headerPtr, (NMUInt8 *) inData, inFlags);
	}

error:
	if (dataPtr != NULL)
		delete[] dataPtr;	//### $Brian delete [] vs. delete

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::SendJoinApproved
//----------------------------------------------------------------------------------------
/*
	We need to send not only the approval message, but also all of the player and group information
	for the current game state.  This could be fairly large.  We'll create a player enumeration
	and a group enumeration and send it.
*/

NMErr
NSpGameMaster::SendJoinApproved(CEndpoint *inEndpoint, NSpPlayerID inID, NMUInt32 inReceivedTime)
{
NMErr					status = kNMNoError;
TJoinApprovedMessagePrivate	*theMessage = NULL;
NSpPlayerEnumerationPtr		thePlayers = NULL;
NSpGroupEnumerationPtr		theGroups = NULL;
	
		
	//	Get our player enumeration
	status = NSpPlayer_GetEnumeration(&thePlayers);
		
	//	Get our group enumeration
	if (status == kNMNoError || status == kNSpNoPlayersErr)
		status = NSpGroup_GetEnumeration(&theGroups);
	
		
	//	Make the message.  This will alloc mem we need to free!
	if (status == kNMNoError || status == kNSpNoGroupsErr)
	{
		status = MakeJoinApprovedMessage(&theMessage, thePlayers, theGroups, inID, inReceivedTime);
	}

	if (thePlayers)
		NSpPlayer_ReleaseEnumeration(thePlayers);
		
	if (theGroups)
		NSpGroup_ReleaseEnumeration(theGroups);

	if (status == kNMNoError)
	{
		status = inEndpoint->SendMessage(&(theMessage->header), (NMUInt8 *)&theMessage->header + sizeof(NSpMessageHeader), kNSpSendFlag_Registered);
		
		if (status != kNMNoError)
		{
			DEBUG_PRINT("Failed in SendMessage.");
		}
		
		//Ä	Free the memory from the buffer.  Free the mem alloced by MakeJoinApprovedMessage
		InterruptSafe_free(theMessage);
	}
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::MakeJoinApprovedMessage
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::MakeJoinApprovedMessage(
		TJoinApprovedMessagePrivate	**theMessage,
		NSpPlayerEnumerationPtr		thePlayers,
		NSpGroupEnumerationPtr		theGroups,
		NSpPlayerID					inPlayer,
		NMUInt32					inReceivedTime)
{
NMUInt32			playerCount, groupCount;
NMUInt32			playerDataSize = 0;
NMUInt32			groupDataSize = 0;
NMUInt32			messageSize;
NMUInt32			i;
NSpPlayerInfoPtr	playerInfo;
NSpGroupInfoPtr		groupInfo;
NMErr			status = kNMNoError;
NMUInt32			size;

	if (thePlayers == NULL)
	{
		playerCount = 0;
		playerDataSize = 0;
	}
	else
	{
		playerCount = thePlayers->count;
		playerDataSize = playerCount * kJoinApprovedPlayerInfoSize;
	}

	if (theGroups == NULL)
	{
		groupCount = 0;
		groupDataSize = 0;
	}
	else
	{
		groupCount = theGroups->count;

		for (i = 0; i < groupCount; i++)
		{
			groupInfo = theGroups->groups[i];

			groupDataSize += sizeof (NSpGroupInfo);
			
			if (groupInfo->playerCount > 0)
				groupDataSize += (groupInfo->playerCount - kVariableLengthArray) * sizeof (NSpPlayerID);
		}
	}

	//Ä	Now we can construct the size of the entire message
	messageSize = ((sizeof (TJoinApprovedMessagePrivate) - 1) + playerDataSize + groupDataSize);

	messageSize += mGameInfo.name[0] + 1;	//LR 2.2 -- we tack the game name to the end of the message

	//Ä	Alloc the message
	*theMessage = (TJoinApprovedMessagePrivate *) InterruptSafe_alloc(messageSize);

	if (*theMessage == NULL)
		return (kNSpMemAllocationErr);
		
	//Ä	Fill in the fields we know
	NSpClearMessageHeader(&((*theMessage)->header));
	(*theMessage)->header.what = kNSpJoinApproved;
	(*theMessage)->header.from = kNSpMasterEndpointID;
	(*theMessage)->header.to = inPlayer;
	(*theMessage)->header.messageLen = messageSize;
	(*theMessage)->receivedTimeStamp = inReceivedTime;
	(*theMessage)->groupIDStartRange = mNextPlayersGroupStartRange;
	(*theMessage)->playerCount = playerCount;
	(*theMessage)->groupCount = groupCount;

	//Ä	Set the version
	(*theMessage)->header.version = kVersion10Message;
	(*theMessage)->header.id = mNextMessageID++;

	//Ä	Now fill in the player stuff and the group stuff
	NMUInt8 *p = (*theMessage)->data;

	if (playerCount > 0)
	{
		NSpPlayerInfoPtr pip = (NSpPlayerInfoPtr) p;

		for (i = 0; i < playerCount; i++)
		{
			playerInfo = thePlayers->playerInfo[i];

			//Ä	We only copy the first three fields, since the last two are redundant with
			//Ä	the group info sent next
			machine_move_data((NMUInt8 *) playerInfo, (NMUInt8 *) pip, kJoinApprovedPlayerInfoSize);

			//Ä	Increment the pointer into our message buffer
			p += kJoinApprovedPlayerInfoSize;
			pip = (NSpPlayerInfo *)p;
		}
	}
	
	if (groupCount > 0)
	{
		for (i = 0; i < groupCount; i++)
		{
			groupInfo = theGroups->groups[i];
			
			size = sizeof (NSpGroupInfo);
			
			if (groupInfo->playerCount > 0)
				size += (groupInfo->playerCount - kVariableLengthArray) * sizeof (NSpPlayerID);

			machine_move_data(groupInfo, p, size);
			p += size;
		}
	}

	//	Add the game name to the end of the buffer. This will be ignored in 2.1 and earlier,
	//	while 2.2 and later will recognize the extra data and copy it to the local game info record
	doCopyPStr( mGameInfo.name, (unsigned char *)p );

	//Ä	Decrement the group range for the next player
	mNextPlayersGroupStartRange -= 1024;
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::SendJoinDenied
//----------------------------------------------------------------------------------------

//Ä	This is quirky, but since we never created a "Real" endpoint for this person,
//Ä	We need to pass the message to the endpoint here
NMErr
NSpGameMaster::SendJoinDenied(CEndpoint *inEndpoint,  void *inCookie, const NMUInt8 *inMessage)
{
NSpJoinDeniedMessage	theReply;
NMErr				err = kNMNoError;
	
	theReply.header.what = kNSpJoinDenied;
	theReply.header.from = kNSpMasterEndpointID;
	theReply.header.to = 0;				//Ä	This person has no honor (or playerID)
	theReply.header.version = kVersion10Message;
	theReply.header.id = mNextMessageID++;
	theReply.header.messageLen = sizeof(NSpJoinDeniedMessage);

	if (inMessage)
		doCopyPStrMax(inMessage, theReply.reason, 255);
	else
		theReply.reason[0] = 0;

	inEndpoint->Veto(inCookie, &theReply.header);
	
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::HandleEvent
//----------------------------------------------------------------------------------------

void
NSpGameMaster::HandleEvent(ERObject *inERObject, CEndpoint *inEndpoint, void *inCookie)
{
	//Ä	We need to forward this on to whoever is supposed to get it, possibly including us
	NSpMessageHeader	*theMessage = inERObject->PeekNetMessage();
		
	//Ä	Any system event should be handled immediately
	if (theMessage->what & kNSpSystemMessagePrefix)
	{
	NMBoolean	passToUser = false;
	NMBoolean	doForward = false;	
				
		if (theMessage->what == kNSpJoinRequest)
		{
			SwapJoinRequest(theMessage);
			
			HandleJoinRequest((NSpJoinRequestMessage *)theMessage, inEndpoint, inCookie, inERObject->GetTimeReceived());
			passToUser = false;
		}	
		else
		{
			passToUser = ProcessSystemMessage(theMessage, &doForward);
		}
			
		//	Note:  there are currently no conditions under which either passToUser or doForward
		//	reach this point as "true".  Be very careful if you make changes which make them so.
		//	(All necessary forwarding and passing to the user takes place via SendSystemMessage()
		//	calls within ProcessSystemMessage().)  If you are wondering why you never
		//	get the group related system messages in message queues, it is because they are declared
		//	as kPrivateMessage types, not because of this forwarding scheme.
		//	This is a very convoluted design, IMO.  See my notes inside of ProcessSystemMessage() to
		//	see why I think this.  CRT Sep 2000.
		
		if (doForward)
			ForwardMessage(theMessage);		//Ä	Forward the message to all recipients, except the sender

		if (!passToUser)
		{
			ReleaseERObject(inERObject);
			return;
		}
		//Ä	Allow the user to have a crack at it, if he installed a message handler
		//Ä	User function returns a boolean telling whether or not to enqueue the message
		else if (mAsyncMessageHandler && !((mAsyncMessageHandler)((NSpGameReference) this->GetGameOwner(), theMessage, mAsyncMessageContext)))
		{
			ReleaseERObject(inERObject);
			return;
		}
	
		ReleaseERObject(inERObject);
	}
	else
	{
		ForwardMessage(theMessage);		//Ä	Forward the message to all recipients, except the sender
		
		ReleaseERObject(inERObject);
	}
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::HandleNewEvent
//----------------------------------------------------------------------------------------

void
NSpGameMaster::HandleNewEvent(ERObject *inERObject, CEndpoint *inEndpoint, void *inCookie)
{
//	DEBUG_PRINT("Handling a private event in NSpGameMaster::HandleNewEvent...");

	// Capture the endpoint and cookie information in the ERObject...
	
	if (inEndpoint)
		inERObject->SetEndpoint(inEndpoint);
	
	if (inCookie)
		inERObject->SetCookie(inCookie);
	
	// Determine whether the event is a system event, and if so, then put
	// it on the private message queue...
	
	if (this->IsSystemEvent(inERObject))
		mSystemEventQ->Enqueue(inERObject);
	else	// otherwise, handle it immediately...
		this->HandleEvent(inERObject, inEndpoint, inCookie);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::GetBacklog
//----------------------------------------------------------------------------------------

NMUInt32 NSpGameMaster::GetBacklog( void )
{
	return mPlayersEndpoint->GetBacklog();
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::PrepareForDeletion
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::PrepareForDeletion(NSpFlags inFlags)
{
NMErr	status  = kNMNoError;
NMBoolean	newHost = false;

#if 0
	if (!(inFlags & kNSpGameFlag_ForceTerminateGame))
	{
		//Ä	Negotiate for a new host
		newHost = NegotiateNewHost();
	}
#endif
		
	if (newHost)
	{
		//Ä Fill in
	}
	else if (inFlags & kNSpGameFlag_ForceTerminateGame)	//Ä	They want us to quit even if we can't find another host
	{
	NSpGameTerminatedMessage	message;

		//Ä	Send our termination
		NSpClearMessageHeader(&message.header);
		
		message.header.what = kNSpGameTerminated;
		message.header.to = kNSpAllPlayers;
		message.header.version = kVersion10Message;
		message.header.id = mNextMessageID++;

		message.header.messageLen = sizeof(NSpGameTerminatedMessage);
		
		//Ä	Tell everyone the game is over
		status = SendSystemMessage(&message.header, kNSpSendFlag_Registered);
		
		mGameState = kStopped;

		RemovePlayer(kNSpAllPlayers, true);
		
		//Ä	Delete our advertising endpoint
		if (mAdvertisingATEndpoint)
		{
			DEBUG_PRINT("Calling Close in NSpGameMaster::PrepareForDeletion (1)");
			mAdvertisingATEndpoint->Close();
			mAdvertisingATEndpoint = NULL;
		}

		if (mAdvertisingIPEndpoint)
		{
			DEBUG_PRINT("Calling Close in NSpGameMaster::PrepareForDeletion (2)");
			mAdvertisingIPEndpoint->Close();
			mAdvertisingIPEndpoint = NULL;
		}	
	}
	else
	{
		status = kNSpNoHostVolunteersErr;
	}
		
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::ProcessSystemMessage
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameMaster::ProcessSystemMessage(NSpMessageHeader *inMessage, NMBoolean *doForward)
{
	NMBoolean	passToUser = false;
	NMBoolean	handled;
	
	// Just in case any of the throws below prevent this variable from getting set...
	*doForward = false;
	
	switch (inMessage->what)
	{

		//  CRT 9/00	Note:  By the current logic, the Master doesn't ever receive a kNSpPlayerLeft or
		//	kNSpPlayerJoined this way, so the following cases don't ever get executed.  If we ever 
		//	make a change such that it does, though, the values for "passToUser" and "doForward" 
		//	may not be correct, in general!  Commenting them out now for safety...		

/*		
		case kNSpPlayerLeft:
			SwapPlayerLeft(inMessage);
			handled = HandlePlayerLeft((NSpPlayerLeftMessage *) inMessage);
			ThrowIfNot_(handled);
			passToUser = true;
			*doForward = true;	// 	<--- Note:  I don't think this is correct, but by the current logic
			break;				//	the Master doesn't ever receive a kNSpPlayerLeft this way, so this case doesn't
								//	ever get executed.  If we ever make a change such that it does, though,
								//	we may need to rethink this "doForward" value.

		case kNSpPlayerJoined:
			SwapPlayerJoined(inMessage);
			passToUser = true;
			*doForward = true;	// 	<--- same note as directly above.
			break;

*/

		case kNSCreateGroup:
			SwapCreateGroup(inMessage);
			handled = HandleCreateGroupMessage((TCreateGroupMessage *)inMessage);
			// User's async message handler is called from within previous function, just to let the user
			// know that a group has been created.  But the message won't appear in any queue, regardless
			// of returned value.  This stinks, IMO.  CRT Sep 2000.
			
			if (true == handled)
			{
				inMessage->to = kNSpAllPlayers;
				SendSystemMessage(inMessage, kNSpSendFlag_Registered);
			}
				
			op_warn(handled);
			
			passToUser = false;	
			*doForward = false;	// Already sent message everywhere (above).  No need to forward again.
			break;

		case kNSDeleteGroup:			// Delete group and create group messages
			SwapCreateGroup(inMessage);	// have identical structures.
			handled = HandleDeleteGroupMessage((TDeleteGroupMessage *)inMessage);
			// User's async message handler is called from within previous function, just to let the user
			// know that a group has been deleted.  But the message won't appear in any queue, regardless
			// of returned value.  This stinks, IMO.  CRT Sep 2000.
			
			
			if (true == handled)
			{
				inMessage->to = kNSpAllPlayers;
				SendSystemMessage(inMessage, kNSpSendFlag_Registered);
			}
				
			op_warn(handled);
			
			passToUser = false;
			*doForward = false;	// Already sent message everywhere (above).  No need to forward again.
			break;

		case kNSAddPlayerToGroup:
			SwapAddPlayerToGroup(inMessage);
			handled = HandleAddPlayerToGroupMessage((TAddPlayerToGroupMessage *)inMessage);
			// User's async message handler is called from within previous function, just to let the user
			// know that a player has been added to a group.  But the message won't appear in any queue, regardless
			// of returned value.  This stinks, IMO.  CRT Sep 2000.
			
			
			if (true == handled)
			{
				inMessage->to = kNSpAllPlayers;
				SendSystemMessage(inMessage, kNSpSendFlag_Registered);
			}

			op_warn(handled);
			
			passToUser = false;
			*doForward = false;	// Already sent message everywhere (above).  No need to forward again.
			break;

		case kNSRemovePlayerFromGroup:			// Add and remove player to/from group
			SwapAddPlayerToGroup(inMessage);	// have identical structures.
			handled = HandleRemovePlayerFromGroupMessage((TRemovePlayerFromGroupMessage *)inMessage);
			// User's async message handler is called from within previous function, just to let the user
			// know that a player has been removed from a group.  But the message won't appear in any queue, regardless
			// of returned value.  This stinks, IMO.  CRT Sep 2000.
			
			if (true == handled)
			{
				inMessage->to = kNSpAllPlayers;
				SendSystemMessage(inMessage, kNSpSendFlag_Registered);
			}

			op_warn(handled);
			
			passToUser = false;
			*doForward = false;	// Already sent message everywhere (above).  No need to forward again.
			break;

		case kNSpPlayerTypeChanged:
			SwapPlayerTypeChanged(inMessage);
			handled = HandlePlayerTypeChangedMessage((NSpPlayerTypeChangedMessage *) inMessage);
			// User's async message handler will be called after this message is routed via the SendSystemMessage()
			// call below.  This message will be put into the NetSprocket queue, if the user's async handler
			// returns "true".  The difference between this message and those above is that this one is not
			// declared as a "kPrivateMessage".
			
			if (true == handled)
			{
				inMessage->to = kNSpAllPlayers;
				SendSystemMessage(inMessage, kNSpSendFlag_Registered);
			}

			op_warn(handled);
			
			passToUser = false;
			*doForward = false;	// Already sent message everywhere (above).  No need to forward again.
			break;
	}
	
	return (passToUser);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::NegotiateNewHost
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameMaster::NegotiateNewHost()
{	
	NMErr	status;
	
	//Ä	Pause the game!
	mGameState = kPaused;
	status = SendPauseGame();
	
	//Ä	Ask for someone else to volunteer
	status = SendBecomeHostRequest();

	return (false);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::SendPauseGame
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::SendPauseGame()
{
	return (kNMNoError);
	
TPauseGameMessage	message;
NMErr			status;
	
	NSpClearMessageHeader(&message);
	message.what = kNSPauseGame;
	message.to = kNSpAllPlayers;
	message.messageLen = sizeof(TPauseGameMessage);
	
	//Ä	status = SendUserMessage(&message, kNSpSendFlag_Registered);

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::SendBecomeHostRequest
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::SendBecomeHostRequest()
{
NMErr	status = kNMNoError;
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::RemovePlayer
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameMaster::RemovePlayer(NSpPlayerID inPlayer, NMBoolean inDisconnect)
{
NMErr					status = kNMNoError;
NSp_InterruptSafeListIterator	iter(*mPlayerList);
NSp_InterruptSafeListIterator	groupIter(*mGroupList);
NSp_InterruptSafeListMember 	*theItem;
PlayerListItem				*thePlayer;
GroupListItem				*theGroup;
NMBoolean					removeAll = false;
NMBoolean					found = false;
	
	if (inPlayer == kNSpAllPlayers)
		removeAll = true;
		
	//Ä	If we're removing all players, remove all groups, too
	if (removeAll)
	{
		status = DoDeleteGroup(kNSpAllGroups);
	}
	else if (mGameInfo.currentGroups > 0)	//Ä	Remove this player from any groups
	{
		while (groupIter.Next(&theItem))
		{
			theGroup = (GroupListItem *)theItem;

			if (!removeAll)
				theGroup->RemovePlayer(inPlayer);
			else
			{
				theGroup->playerCount = 0;
				delete theGroup->players;
				theGroup->players = new NSp_InterruptSafeList();
			}
		}
	}
	
	while (!found && iter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *)theItem;

		if (removeAll || thePlayer->id == inPlayer)
		{
			mPlayerList->Remove(theItem);
			
			if (thePlayer->endpoint)
			{
				if (inDisconnect)
				{
					thePlayer->endpoint->Disconnect(true);
				}

				DEBUG_PRINT("Calling Close in NSpGameMaster::RemovePlayer");
				thePlayer->endpoint->Close();
			
				if (thePlayer->endpoint == mPlayersEndpoint)
					mPlayersEndpoint = NULL;
			}

			delete thePlayer;
			
			mGameInfo.currentPlayers--;

			if (!removeAll)
				found = true;
		}
	}
			
	return (true);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::HandleEndpointDisconnected
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::HandleEndpointDisconnected(CEndpoint *inEndpoint)
{
NMErr					status;
NSp_InterruptSafeListIterator	iter(*mPlayerList);
NSp_InterruptSafeListMember 	*theItem;
PlayerListItem				*thePlayer;
NMBoolean					found = false;
NSpPlayerLeftMessage 		message;
NSpPlayerID					thePlayerID = 0;
NSpPlayerName				thePlayerName;

	if (inEndpoint == mPlayersEndpoint)
		return (kNMNoError);
		
	//Ä	Find out which player it was
	while (!found && iter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *)theItem;

		if (thePlayer->endpoint == inEndpoint)
		{
			thePlayerID = thePlayer->id;
			machine_move_data(thePlayer->info->name, thePlayerName, sizeof (NSpPlayerName));
			found = true;
		}
	}
	
	if (thePlayerID == 0)
		return (kNSpInvalidPlayerIDErr);
	
	//Ä	First remove this person from our list
	RemovePlayer(thePlayerID, false);

	//Ä	Notify everyone that this player has left the game
	NSpClearMessageHeader(&message.header);
	
	message.header.what = kNSpPlayerLeft;
	message.header.to = kNSpAllPlayers;
	message.header.version = kVersion10Message;
	message.header.id = mNextMessageID++;
	message.playerCount = mGameInfo.currentPlayers;
	message.playerID = thePlayerID;
	machine_move_data(thePlayerName, message.playerName, sizeof (NSpPlayerName));
	message.header.messageLen = sizeof(NSpPlayerLeftMessage);

	status = SendSystemMessage(&message.header, kNSpSendFlag_Registered);

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::SendJoinRequest
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::SendJoinRequest(NMConstStr31Param inPlayerName, NSpPlayerType inType)
{
NMErr				status;
NSpJoinRequestMessage	theMessage;

	NSpClearMessageHeader(&theMessage.header);

	theMessage.header.what = kNSpJoinRequest;
	theMessage.header.messageLen = sizeof(NSpJoinRequestMessage);
	theMessage.header.to = kNSpMasterEndpointID;
	theMessage.theType = inType;
	theMessage.customDataLen = kCustomLocalJoinRequest;			//Ä	Signifying a local join request
	doCopyPStrMax(inPlayerName, theMessage.name, 31);

	status = SendUserMessage( &theMessage.header, kNSpSendFlag_Registered);
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::FreePlayerAddress
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::FreePlayerAddress(void **outAddress)
{
	NMErr err;
	NSp_InterruptSafeListIterator	iter(*mPlayerList);
	NSp_InterruptSafeListMember 	*theItem;
	PlayerListItem					*thePlayer;

	if( iter.Next(&theItem) )	/* get first player */
	{
		thePlayer = (PlayerListItem *) theItem;
		if (thePlayer->endpoint)
		{
			PEndpointRef		openPlayEndpoint;

			openPlayEndpoint = thePlayer->endpoint->GetReliableEndpoint();

			err = ProtocolFreeEndpointAddress(openPlayEndpoint, outAddress);	/* module must sanity check param! */
		}
		else
			err = kNMBadStateErr;

		return(err);
	}
	else
		return( kNSpNoPlayersErr );
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::GetPlayerIPAddress
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::GetPlayerIPAddress(const NSpPlayerID inPlayerID, char **outAddress)
{
NMErr							theError = kNMNoError;
NSp_InterruptSafeListIterator	iter(*mPlayerList);
NSp_InterruptSafeListMember 	*theItem;
PlayerListItem					*thePlayer;

	while (iter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *) theItem;

		if (thePlayer->id == inPlayerID)
		{
			if (thePlayer->endpoint)
			{
			PEndpointRef		openPlayEndpoint;

				openPlayEndpoint = thePlayer->endpoint->GetReliableEndpoint();

				theError = ProtocolGetEndpointAddress(openPlayEndpoint, kNMIPAddressType, (void**)outAddress);
				
				return (theError);
			}
		}
	}

	return (kNSpInvalidPlayerIDErr);
}

#if OP_API_NETWORK_OT

//----------------------------------------------------------------------------------------
// NSpGameMaster::GetPlayerAddress
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::GetPlayerAddress(const NSpPlayerID inPlayerID, OTAddress **outAddress)
{
NMErr							theError = kNMNoError;
NSp_InterruptSafeListIterator	iter(*mPlayerList);
NSp_InterruptSafeListMember 	*theItem;
PlayerListItem					*thePlayer;

	while (iter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *) theItem;

		if (thePlayer->id == inPlayerID)
		{
			if (thePlayer->endpoint)
			{
			PEndpointRef		openPlayEndpoint;

				openPlayEndpoint = thePlayer->endpoint->GetReliableEndpoint();

				theError = ProtocolGetEndpointAddress(openPlayEndpoint, kNMOTAddressType, (void**)outAddress);

				return (theError);
			}
		}
	}

	return (kNSpInvalidPlayerIDErr);
}

#endif

//----------------------------------------------------------------------------------------
// NSpGameMaster::ChangePlayerType
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::ChangePlayerType(const NSpPlayerID inPlayerID, const NSpPlayerType inNewType)
{
NSp_InterruptSafeListIterator	iter(*mPlayerList);
NSp_InterruptSafeListMember 	*theItem;
PlayerListItem				*thePlayer;
NMBoolean					found = false;
NMErr					status = kNMNoError;
	
	while (!found && iter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *) theItem;

		if (thePlayer->id == inPlayerID)
		{
		NSpPlayerInfoPtr			info;
		NSpPlayerTypeChangedMessage	message;
		
			found = true;

			info = thePlayer->info;
		
			if (inNewType == info->type)	// Nothing to do.
			{
				status = kNMNoError;
			}
			else
			{
				NSpClearMessageHeader(&message.header);
			
				message.header.to = kNSpAllPlayers;
				message.header.what = kNSpPlayerTypeChanged;
				message.header.messageLen = sizeof (NSpPlayerTypeChangedMessage);
				message.player = inPlayerID;
				message.newType = inNewType;
	
				status = SendUserMessage(&message.header, kNSpSendFlag_Registered);
//				status = SendSystemMessage(&message.header, kNSpSendFlag_Registered);
				
				if (kNMNoError == status)
					info->type = inNewType;
			}		
		}
	}
	
	if (found)
		return (status);	
	else		
		return (kNSpInvalidPlayerIDErr);
}

//----------------------------------------------------------------------------------------
// NSpGameMaster::ForceRemovePlayer
//----------------------------------------------------------------------------------------

NMErr
NSpGameMaster::ForceRemovePlayer(const NSpPlayerID inPlayerID)
{
NSp_InterruptSafeListIterator	iter(*mPlayerList);
NSp_InterruptSafeListMember 	*theItem;
PlayerListItem				*thePlayer;
NMBoolean					found = false;
NMErr					status = kNMNoError;

	if (0 >= inPlayerID)		// Can't remove a group or all players.  So PlayerID must be greater than 0.
		return (kNSpInvalidPlayerIDErr);

	while (!found && iter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *) theItem;

		if (thePlayer->id == inPlayerID)
		{
		NSpPlayerLeftMessage	message;

			found = true;

			machine_move_data(thePlayer->info->name, message.playerName, sizeof (NSpPlayerName));
		
			//Ä	First remove this person from our list
			RemovePlayer(inPlayerID, false);
		
			//Ä	Then notify everyone that this player has left the game
			NSpClearMessageHeader(&message.header);
			
			message.header.what = kNSpPlayerLeft;
			message.header.to = kNSpAllPlayers;
			message.header.version = kVersion10Message;
			message.header.id = mNextMessageID++;
			message.playerCount = mGameInfo.currentPlayers;
			message.playerID = inPlayerID;
			message.header.messageLen = sizeof(NSpPlayerLeftMessage);
		
			status = SendSystemMessage(&message.header, kNSpSendFlag_Registered);
		}
	}
	
	if (found)
		return (status);	
	else		
		return (kNSpInvalidPlayerIDErr);
}
