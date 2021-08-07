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

#include "NSpPrefix.h"
#include "NSpGame.h"
#include "NSpGamePrivate.h"
#include "String_Utils.h"
#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif

#ifdef OP_API_NETWORK_OT
	#include "OTUtils.h"
	#include <OpenTptAppleTalk.h>
	#include <OpenTptInternet.h>
#endif

#define	kNSp_STRn_GameClassStrings 	-21000


NMUInt32	kQGrowthSize = 20;

//----------------------------------------------------------------------------------------
// NSpGame::NSpGame
//----------------------------------------------------------------------------------------

NSpGame::NSpGame(
		NMUInt32		inMaxPlayers,
		NMConstStr31Param	inGameName,
		NMConstStr31Param	inPassword,
		NSpTopology		inTopology,
		NSpFlags		inFlags)
{
	NMUInt32	i;

	mObjectID = class_ID;
	mAsyncMessageHandler = NULL;
	mAsyncMessageContext = NULL;
	mFreeQLen = mMessageQLen = mCookieQLen = 0;
	mPendingMessages = NULL;

	//Ä	Initialize the player count
	mGameInfo.maxPlayers = (inMaxPlayers == 0) ? 0xFFFFFFFF : inMaxPlayers;
	mNextAvailableGroupID = -2;
	mNextMessageID = 1;

	if (inGameName == NULL)
	{
		//Ä	Use "untitled game" if we get a NULL
//		GetIndString(mGameInfo.name, kNSp_STRn_GameClassStrings, kUntitledGame);
		doCopyC2PStrMax("Untitled Game", mGameInfo.name, 31);
	}
	else
	{
		doCopyPStrMax(inGameName, mGameInfo.name, 31);
	}

	//Ä	Set up the password for entry into the game
	if (inPassword)
		doCopyPStrMax(inPassword, mGameInfo.password, 31);
	else
		mGameInfo.password[0] = 0;

	mGameInfo.topology = inTopology;
	mFlags = inFlags;

	mPlayerID = 0;
	mTimeStampDifferential = 0;

	//Ä	Allocate our queues
	mEventQ = new NMLIFO();
	op_assert(mEventQ);
	mEventQ->Init();

	mSystemEventQ = new NMLIFO();
	op_assert(mSystemEventQ);
	mSystemEventQ->Init();

	mCookieQ = new NMLIFO();
	op_assert(mCookieQ);
	mCookieQ->Init();

	//Ä	Lets make a bunch of event records, and add them to the free q
	mFreeQ = new NMLIFO();
	op_assert(mFreeQ);
	mFreeQ->Init();
	
	ERObject	*item;
	NSpMessageHeader *message;

	for (i = 0; i < gQElements; i++)
	{
		message = (NSpMessageHeader *)InterruptSafe_alloc(gStandardMessageSize);
		
		op_assert(message);

		item = new ERObject(message, gStandardMessageSize);
		op_assert(item);
		
		mFreeQ->Enqueue(item);
		mFreeQLen++;
	}

	for (i = 0; i < gQElements; i++)
	{
		item = new ERObject();
		mCookieQ->Enqueue(item);
		mCookieQLen++;
	}


	//Ä	Initialize our player list
	mPlayerList = new NSp_InterruptSafeList();
	op_assert(mPlayerList);

	mPlayerListIter = new NSp_InterruptSafeListIterator(*mPlayerList);
	op_assert(mPlayerListIter);

	//Ä	Create our group list
	mGroupList = new NSp_InterruptSafeList();
	op_assert(mGroupList);
	
	mGroupListIter = new NSp_InterruptSafeListIterator(*mGroupList);
	op_assert(mGroupListIter);

	mGameState = kRunning;

	mGameInfo.maxPlayers = 0;
	mGameInfo.currentPlayers = 0;
	mGameInfo.currentGroups = 0;
	mGameInfo.topology = 0;
	mGameInfo.reserved = 0;
	mGameInfo.name[0] = 0;
	mGameInfo.password[0] = 0;
}

//----------------------------------------------------------------------------------------
// NSpGame::~NSpGame
//----------------------------------------------------------------------------------------

NSpGame::~NSpGame()
{
	if (mEventQ)
		delete mEventQ;

	if (mSystemEventQ)
		delete mSystemEventQ;

	if (mFreeQ)
		delete mFreeQ;

	if (mPlayerListIter)
		delete mPlayerListIter;

	if (mGroupListIter)
		delete mGroupListIter;

	if (mPlayerList)
		delete mPlayerList;

	if (mGroupList)
		delete mGroupList;
}

#if defined(__MWERKS__)	
#pragma mark ====== ERObject and Buffer =========
#endif

//----------------------------------------------------------------------------------------
// NSpGame::GetFreeERObject
//----------------------------------------------------------------------------------------

ERObject *
NSpGame::GetFreeERObject(NMUInt32 inSize)
{
	ERObject 			*item = NULL;
	NSpMessageHeader	*message = NULL;
	NMSInt32			i = 0;
	NMErr 				status = kNMNoError;

	if (inSize <= gStandardMessageSize)
	{
		if (mFreeQ->IsEmpty())
		{
			for (i = 0; i < kQGrowthSize; i++)
			{
					//Ä	Do an interrupt-safe alloc
					message = (NSpMessageHeader *) InterruptSafe_alloc(gStandardMessageSize);
					if (!message){
						status = kNSpMemAllocationErr;
						goto error;
					}

					item = new ERObject(message, inSize);
					if (item == NULL){
						status = kNSpMemAllocationErr;
						goto error;
					}

					mFreeQ->Enqueue(item);
					mFreeQLen++;
			}
		}

		item = (ERObject *)mFreeQ->Dequeue();
	}
	else
	{
		item = GetCookieERObject();
		if (item == NULL){
			status = kNSpMemAllocationErr;
			goto error;
		}

		message = (NSpMessageHeader *) InterruptSafe_alloc(inSize);
		if (message == NULL){
			status = kNSpMemAllocationErr;
			goto error;
		}

		item->SetNetMessage(message, inSize);
	}

	error:
	if (status)
	{
		return (NULL);
	}

	if (item != NULL)
		mFreeQLen--;

	return (item);
}

//----------------------------------------------------------------------------------------
// NSpGame::GetCookieERObject
//----------------------------------------------------------------------------------------

ERObject 
*NSpGame::GetCookieERObject(void)
{
	ERObject	*item;
	NMSInt32	i = 0;
	NMErr		status = kNMNoError;

	if (mCookieQ->IsEmpty())
	{
		for (i = 0; i < kQGrowthSize; i++)
		{
			item = new ERObject();
			if (item == NULL){
				status = kNSpMemAllocationErr;
				goto error;
			}

			mCookieQ->Enqueue(item);
			mCookieQLen++;
		}
	}

	item = (ERObject *)mCookieQ->Dequeue();

	error:
	if (status)
	{
		NMErr code = status;
		(void) code;

		mCookieQLen--;
		return ((ERObject *) mCookieQ->Dequeue());
	}

	if (item)
		mCookieQLen--;

	return (item);
}

//----------------------------------------------------------------------------------------
// NSpGame::ReleaseERObject
//----------------------------------------------------------------------------------------

void
NSpGame::ReleaseERObject(ERObject *inObject)
{
NSpMessageHeader	*message;

	if (inObject->GetMaxMessageLen() == gStandardMessageSize)
	{
		inObject->Clear();
		mFreeQ->Enqueue(inObject);
		mFreeQLen++;
	}
	else
	{
		message = inObject->RemoveNetMessage();
		InterruptSafe_free(message);
		mCookieQ->Enqueue(inObject);
		mCookieQLen++;
	}
}

//----------------------------------------------------------------------------------------
// NSpGame::GetNewDataBuffer
//----------------------------------------------------------------------------------------

NMUInt8 *
NSpGame::GetNewDataBuffer(NMUInt32	inLength)
{

	return ((NMUInt8 *) InterruptSafe_alloc(inLength));
}

//----------------------------------------------------------------------------------------
// NSpGame::FreeDataBuffer
//----------------------------------------------------------------------------------------

void
NSpGame::FreeDataBuffer(NMUInt8 *inBuf)
{
	if (NULL != inBuf)
		InterruptSafe_free(inBuf);
}

//----------------------------------------------------------------------------------------
// NSpGame::FreeNetMessage
//----------------------------------------------------------------------------------------

void
NSpGame::FreeNetMessage(NSpMessageHeader *inMessage)
{
ERObject	*theObject;

	//Ä	If it's a big message, we alloced it, so just free it
	if (inMessage->messageLen > gStandardMessageSize)
	{
		InterruptSafe_free(inMessage);
		return;
	}

	//Ä	It's a standard sized message.  Get a cookie to reuse it
	theObject = GetCookieERObject();

	if (!theObject)
	{
		InterruptSafe_free(inMessage);
		return;
	}

	//Ä	Release the message back to the free q
	theObject->SetNetMessage(inMessage, gStandardMessageSize);
	ReleaseERObject(theObject);
}
#if defined(__MWERKS__)	
#pragma mark ======= Players =========
#endif

//----------------------------------------------------------------------------------------
// NSpGame::NSpPlayer_GetInfo
//----------------------------------------------------------------------------------------

NMErr
NSpGame::NSpPlayer_GetInfo(NSpPlayerID inID, NSpPlayerInfoPtr *outInfo)
{
	NMErr					status = kNMNoError;
	NSp_InterruptSafeListIterator	iter(*mPlayerList);
	NSp_InterruptSafeListMember	*theItem;
	PlayerListItem				*thePlayer = NULL;
	NMBoolean					found = false;
	NMUInt32					infoSize;
	NSpPlayerInfoPtr			theInfo = NULL;
	NSpGroupID					*groups;
	NMUInt32					groupCount;

	op_vassert(inID > 0, "NSpPlayer_GetInfo: inID < 1");

	if (inID < 1)
		return (kNSpInvalidPlayerIDErr);

	//Ä	First find the player in the list
	while (!found && iter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *)theItem;

		if (thePlayer->id == inID)
			found = true;
	}

	//Ä	If we didn't find it, bail
	if (!found)
		return 	(kNSpInvalidPlayerIDErr);

	//Ä	Our player list doesn't really contain the groups.  We need to look through
	//Ä	the group list and determine which groups we belong to.
	//Ä	This could be improved.
	FillInGroups(inID, &groups, &groupCount);

	//Ä	Otherwise, copy the info
	infoSize = sizeof (NSpPlayerInfo) + (((groupCount == 0) ? 0 : groupCount -1) * sizeof (NSpGroupID));
	theInfo = (NSpPlayerInfoPtr) InterruptSafe_alloc(infoSize);
	if (theInfo == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	machine_move_data(thePlayer->info, theInfo, sizeof (NSpPlayerInfo) - 8);
	if (groupCount)
	{
		theInfo->groupCount = groupCount;
		machine_move_data(groups, &theInfo->groups, (groupCount * sizeof (NSpGroupID)));
		ReleaseGroups(groups);
	}
	else
	{
		theInfo->groupCount = 0;
		theInfo->groups[0] = 0;
	}

	*outInfo = theInfo;

error:
	if (status)
	{
		if (theInfo)
			InterruptSafe_free(theInfo);

		*outInfo = NULL;
	}

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpPlayer_ReleaseInfo
//----------------------------------------------------------------------------------------

void
NSpGame::NSpPlayer_ReleaseInfo(NSpPlayerInfoPtr inInfo)
{
	op_vassert(NULL != inInfo, "NSpPlayer_ReleaseInfo:  inInfo == NULL");

	if (NULL == inInfo)
		return;

	InterruptSafe_free(inInfo);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpPlayer_GetEnumeration
//----------------------------------------------------------------------------------------

NMErr
NSpGame::NSpPlayer_GetEnumeration(NSpPlayerEnumerationPtr *outPlayers)
{
	NMErr					status = kNMNoError;
	NSp_InterruptSafeListIterator	iter(*mPlayerList);
	NSp_InterruptSafeListMember	*theItem;
	PlayerListItem				*thePlayer;
	NSpPlayerInfoPtr			theInfo = NULL;
	NSpPlayerEnumerationPtr		thePlayers = NULL;

	if (mGameInfo.currentPlayers < 1)
	{
		*outPlayers = NULL;
		return (kNSpNoPlayersErr);
	}

	//Ä	Allocate enough for the enumeration
	thePlayers = (NSpPlayerEnumerationPtr) InterruptSafe_alloc(sizeof (NSpPlayerEnumeration) +
					(sizeof (NSpPlayerInfoPtr) * (mGameInfo.currentPlayers - kVariableLengthArray)));
	if (thePlayers == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	//Ä	We're going to increment this as we add, in case we have an allocation problem
	thePlayers->count = 0;

	while (iter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *)theItem;

		//Ä	This will allocate and fill in the info
		status = NSpPlayer_GetInfo(thePlayer->id, &theInfo);
		//ThrowIfOSErr_(status);
		if (status)
			goto error;

		thePlayers->playerInfo[thePlayers->count] = theInfo;
		thePlayers->count++;
	}

	*outPlayers = thePlayers;

error:
	if (status)
	{
		if (thePlayers)
		{
			NSpPlayer_ReleaseEnumeration(thePlayers);
		}

		*outPlayers = NULL;
	}

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpPlayer_ReleaseEnumeration
//----------------------------------------------------------------------------------------

void
NSpGame::NSpPlayer_ReleaseEnumeration(NSpPlayerEnumerationPtr inPlayers)
{
NMUInt32	count = inPlayers->count;

	op_vassert(NULL != inPlayers, "NSpPlayer_ReleaseEnumeration: inPlayers == NULL");

	if (NULL == inPlayers)
		return;

	//Ä	NSpMessage_Release all the player info structures
	for (NMSInt32 i = 0; i < count; i++)
	{
		NSpPlayer_ReleaseInfo(inPlayers->playerInfo[i]);
	}

	//Ä	Then release the players structure itself
	InterruptSafe_free(inPlayers);
}

//----------------------------------------------------------------------------------------
// NSpGame::AddPlayer
//----------------------------------------------------------------------------------------

NMBoolean
NSpGame::AddPlayer(NSpPlayerInfo *inInfo, CEndpoint *inEndpoint)
{
	PlayerListItem	*theListItem = NULL;
	NMUInt32 infoSize;
	NMErr status = kNMNoError;

	op_vassert_return((inInfo != NULL),"NULL info record passed to AddPlayer.",false);

	theListItem = new PlayerListItem;
	if (theListItem == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	theListItem->endpoint = inEndpoint;

	infoSize = sizeof (NSpPlayerInfo) + (((inInfo->groupCount == 0) ? 0 : inInfo->groupCount -1) * sizeof (NSpGroupID));
	theListItem->info = (NSpPlayerInfoPtr) InterruptSafe_alloc(infoSize);
	if (theListItem->info == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	machine_move_data(inInfo, theListItem->info, infoSize);
	theListItem->id = inInfo->id;

	mPlayerList->Append(theListItem);

	mGameInfo.currentPlayers++;

error:
	if (status)
	{
		NMErr code = status;
		(void) code;

		if (theListItem)
		{
			delete (theListItem);
		}

		return (false);
	}

	return (true);
}

//----------------------------------------------------------------------------------------
// NSpGame::GetPlayerListItem
//----------------------------------------------------------------------------------------

PlayerListItem *
NSpGame::GetPlayerListItem(NSpPlayerID inPlayerID)
{
	NSp_InterruptSafeListIterator	iter(*mPlayerList);
	NSp_InterruptSafeListMember	*theItem;
	PlayerListItem				*thePlayer = NULL;
	NMBoolean					found = false;
	NMErr						status = kNMNoError;

	if (inPlayerID < 1)
		return (NULL);

	//Try_
	{
		//Ä	First find the player in the list
		while (!found && iter.Next(&theItem) )
		{
			thePlayer = (PlayerListItem *)theItem;

			if (thePlayer->id == inPlayerID)
			{
				found = true;
			}
		}

		if (!found)
			return 	(NULL);
		else
			return (thePlayer);
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		(void) code;

		return (NULL);
	}
}

//----------------------------------------------------------------------------------------
// NSpGame::HandlePlayerLeft
//----------------------------------------------------------------------------------------

NMBoolean
NSpGame::HandlePlayerLeft(NSpPlayerLeftMessage *inMessage)
{
NMBoolean	handled = true;

	handled = RemovePlayer(inMessage->playerID, true);

	return (handled);
}

//----------------------------------------------------------------------------------------
// NSpGame::HandlePlayerTypeChangedMessage
//----------------------------------------------------------------------------------------

NMBoolean
NSpGame::HandlePlayerTypeChangedMessage(NSpPlayerTypeChangedMessage *inMessage)
{
NSp_InterruptSafeListIterator	iter(*mPlayerList);
NSp_InterruptSafeListMember 	*theItem;
NMBoolean						found = false;
NMBoolean						handled = true;

	while (!found && iter.Next(&theItem))
	{
	PlayerListItem	*thePlayer;
	
		thePlayer = (PlayerListItem *) theItem;

		if (thePlayer->id == inMessage->player)
		{
		NSpPlayerInfoPtr	info;
		
			info = thePlayer->info;
			info->type = inMessage->newType;
			found = true;
		}
	}
	
	if (found)
		return (handled);
	else
		return (false);// kNSpInvalidPlayerIDErr);
}

#if defined(__MWERKS__)	
#pragma mark ======== Groups ========
#endif

//----------------------------------------------------------------------------------------
// NSpGame::NSpGroup_GetInfo
//----------------------------------------------------------------------------------------

NMErr
NSpGame::NSpGroup_GetInfo(NSpGroupID inGroupID, NSpGroupInfoPtr *outInfo)
{
	NMErr					status = kNMNoError;
	NSp_InterruptSafeListIterator	iter(*mGroupList);
	NSp_InterruptSafeListIterator	*playerIterator = NULL;
	NSp_InterruptSafeListMember	*theItem;
	GroupListItem				*theGroup = NULL;
	PlayerListItem				*thePlayer;
	NMBoolean					found = false;
	NMUInt32					infoSize;
	NSpGroupInfoPtr				theInfo = NULL;
	NMUInt32					count = 0;

	if (inGroupID > -1)
		return (kNSpInvalidGroupIDErr);

	//Ä	First find the group in the list
	while (!found && iter.Next(&theItem))
	{
		theGroup = (GroupListItem *)theItem;
		if (theGroup->id == inGroupID)
		{
			found = true;
		}
	}

	//Ä	If we didn't find it, bail
	if (!found)
		return (kNSpInvalidGroupIDErr);

	//Ä	Otherwise, allocate enough for all the player IDs
	infoSize = sizeof (NSpGroupInfo) +
		(((theGroup->playerCount == 0) ? 0 : theGroup->playerCount - 1) * sizeof (NSpPlayerID));

	theInfo = (NSpGroupInfoPtr) InterruptSafe_alloc(infoSize);
	if (theInfo == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	theInfo->id = inGroupID;
	theInfo->playerCount = theGroup->playerCount;

	//Ä	Iterate through our player list
	playerIterator = new NSp_InterruptSafeListIterator(*theGroup->players);
	if (playerIterator == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	//Ä	Fill in the info with the play IDs
	while (playerIterator->Next(&theItem))
	{
		thePlayer = (PlayerListItem *)((UInt32ListMember *)theItem)->GetValue();
		theInfo->players[count] = thePlayer->id;
		count++;
	}

	//Ä	Release the iterator
	delete playerIterator;

	playerIterator = NULL;

	*outInfo = theInfo;

error:
	if (status)
	{
		if (theInfo)
			NSpGroup_ReleaseInfo(theInfo);

		if (playerIterator)
		{
			delete playerIterator;
		}
	}

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpGroup_ReleaseInfo
//----------------------------------------------------------------------------------------

void
NSpGame::NSpGroup_ReleaseInfo(NSpGroupInfoPtr inInfo)
{
	if (NULL != inInfo)
		InterruptSafe_free((char *)inInfo);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpGroup_GetEnumeration
//----------------------------------------------------------------------------------------

NMErr
NSpGame::NSpGroup_GetEnumeration(NSpGroupEnumerationPtr *outGroups)
{
	NMErr					status = kNMNoError;
	NSp_InterruptSafeListIterator	iter(*mGroupList);
	NSp_InterruptSafeListMember	*theItem;
	GroupListItem				*theGroup;
	NSpGroupInfoPtr				theInfo = NULL;
	NSpGroupEnumerationPtr		theGroups;

	if (mGameInfo.currentGroups < 1)
	{
		*outGroups = NULL;
		return (kNSpNoGroupsErr);
	}

	//Ä	Allocate enough for the enumeration
	theGroups = (NSpGroupEnumerationPtr) InterruptSafe_alloc(sizeof (NSpGroupEnumeration) + 
	(sizeof (NSpGroupInfoPtr) * (mGameInfo.currentGroups - kVariableLengthArray)));

	if (NULL == theGroups)
	{
		*outGroups = NULL;
		return (kNSpMemAllocationErr);
	}

	//Ä	We're going to increment this as we add, in case we have an allocation problem
	theGroups->count = 0;

	while (iter.Next(&theItem))
	{
		theGroup = (GroupListItem *)theItem;

		//Ä	This will allocate and fill in the info
		status = NSpGroup_GetInfo(theGroup->id, &theInfo);

		if (kNMNoError != status)
		{
			*outGroups = NULL;
			NSpGroup_ReleaseEnumeration(theGroups);

			return (status);
		}

		theGroups->groups[theGroups->count] = theInfo;
		theGroups->count++;
	}

	*outGroups = theGroups;

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpGroup_ReleaseEnumeration
//----------------------------------------------------------------------------------------

void NSpGame::NSpGroup_ReleaseEnumeration(NSpGroupEnumerationPtr inGroups)
{
	op_vassert_justreturn((inGroups != NULL),"Passed NULL pointer to NSpGroup_ReleaseEnumeration.  Don't do that!");

	for (NMSInt32 i = 0; i < inGroups->count; i++)
	{
		NSpGroup_ReleaseInfo(inGroups->groups[i]);
	}

	InterruptSafe_free(inGroups);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpGroup_Create
//----------------------------------------------------------------------------------------

//Ä	This assigns a group id locally, and tells everyone else
//Ä	Maybe it should ask the server for a group ID
NMErr
NSpGame::NSpGroup_Create(NSpGroupID *outGroupID)
{
NSpGroupID			id;
TCreateGroupMessage	message;
NMErr			status = kNMNoError;

	id = mNextAvailableGroupID--;	//Ä	Group IDs go down as you 'increment'

	//Ä	Send a message to the server, requesting a new group
	NSpClearMessageHeader(&message.header);

	message.header.to = kNSpAllPlayers;
	message.header.what = kNSCreateGroup;
	message.header.messageLen = sizeof (TCreateGroupMessage);
	message.id = id;
	message.requestingPlayer = mPlayerID;

	//Ä	Handle this immediately, since the user may try to use this
	//Ä	group ID as soon as she creates it
	NMBoolean handled = HandleCreateGroupMessage(&message);

	if (false == handled)
	{
		*outGroupID = 0;
		return (kNSpCreateGroupFailedErr);
	}

	status = SendUserMessage(&message.header, kNSpSendFlag_Registered);

	if (kNMNoError == status)
		*outGroupID = id;

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpGroup_Delete
//----------------------------------------------------------------------------------------

//Ä	Send a message to everyone, telling them to delete the group
NMErr
NSpGame::NSpGroup_Delete(NSpGroupID inGroupID)
{
TDeleteGroupMessage		message;
NMErr				status = kNMNoError;

	//Ä	Send a message to everyone, including ourselves, telling them to make a new group
	NSpClearMessageHeader(&message.header);

	message.header.to = kNSpAllPlayers;
	message.header.what = kNSDeleteGroup;
	message.header.messageLen = sizeof (TDeleteGroupMessage);
	message.id = inGroupID;
	message.requestingPlayer = mPlayerID;

	status = SendUserMessage(&message.header, kNSpSendFlag_Registered);

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpGroup_AddPlayer
//----------------------------------------------------------------------------------------

NMErr
NSpGame::NSpGroup_AddPlayer(NSpGroupID inGroupID, NSpPlayerID inPlayerID)
{
TAddPlayerToGroupMessage	message;
NMErr					status = kNMNoError;

	//Ä	Send a message to everyone, including ourselves, telling them to make a new group
	NSpClearMessageHeader(&message.header);

	message.header.to = kNSpAllPlayers;
	message.header.what = kNSAddPlayerToGroup;
	message.header.messageLen = sizeof (TAddPlayerToGroupMessage);
	message.group = inGroupID;
	message.player = inPlayerID;

	//Ä	Send a message to all, telling them to create this new group
	status = SendUserMessage(&message.header, kNSpSendFlag_Registered);

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpGroup_RemovePlayer
//----------------------------------------------------------------------------------------

NMErr
NSpGame::NSpGroup_RemovePlayer(NSpGroupID inGroupID, NSpPlayerID inPlayerID)
{
TRemovePlayerFromGroupMessage	message;
NMErr						status = kNMNoError;

#if DEBUG
	NSp_InterruptSafeListIterator 		groupIter(*mGroupList);
	NSp_InterruptSafeListIterator 		playerIter(*mPlayerList);
	NSp_InterruptSafeListMember 		*theItem;
	GroupListItem						*theGroup;
	PlayerListItem						*thePlayer;
	NMBoolean							found = false;

	if (inGroupID > -1 || mGameInfo.currentGroups < 1)
		return (kNSpInvalidGroupIDErr);

	if (inPlayerID < 1 || mGameInfo.currentPlayers < 1)
		return (kNSpInvalidPlayerIDErr);

	//Ä	First find the group in the list
	while (!found && groupIter.Next(&theItem))
	{
		theGroup = (GroupListItem *)theItem;

		if (theGroup->id == inGroupID)
			found = true;
	}

	//Ä	If we didn't find it, bail
	if (!found)
		return (kNSpInvalidGroupIDErr);

	found = false;

	//Ä	Locate this player
	while (!found && playerIter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *)theItem;
		if (thePlayer->id == inPlayerID)
		{
			found = true;
		}
	}

	//Ä	If we didn't find it, bail
	if (!found)
		return (kNSpInvalidPlayerIDErr);

#endif

	//Ä	Send a message to everyone, including ourselves, telling them to make a new group
	NSpClearMessageHeader(&message.header);

	message.header.to = kNSpAllPlayers;
	message.header.what = kNSRemovePlayerFromGroup;
	message.header.messageLen = sizeof (TRemovePlayerFromGroupMessage);
	message.group = inGroupID;
	message.player = inPlayerID;

	//Ä	Send a message to all, telling them to create this new group
	status = SendUserMessage(&message.header, kNSpSendFlag_Registered);

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame::FillInGroups
//----------------------------------------------------------------------------------------

//Ä	This is an O(n^2) routine.  Blegh!
void
NSpGame::FillInGroups(NSpPlayerID inPlayer, NSpGroupID **outGroups, NMUInt32 *outGroupCount)
{
	NSpGroupID					*groups = NULL;
	NMUInt32						groupCount = 0;
	NSp_InterruptSafeListIterator 	iter(*mGroupList);
	NSp_InterruptSafeListIterator 	*playerIterator = NULL;
	NSp_InterruptSafeListMember 	*theItem;
	GroupListItem				*theGroup;
	PlayerListItem				*thePlayer;
	NMBoolean				found = false;
	NMErr					status = kNMNoError;

	if (mGameInfo.currentGroups == 0)
	{
		*outGroups = NULL;
		*outGroupCount = 0;
		return;
	}

	//Ä	We don't know how many groups this player is a member of until we iterate
	//Ä	Through the list.  Therefore, alloc enough for all the groups
	groups = (NSpGroupID *)InterruptSafe_alloc(mGameInfo.currentGroups * sizeof (NSpGroupID));
	if (groups == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	while (iter.Next(&theItem))
	{
		theGroup = (GroupListItem *)theItem;

		if (theGroup->playerCount > 0)
		{
			playerIterator = new NSp_InterruptSafeListIterator(*theGroup->players);
			if (playerIterator == NULL){
				status = kNSpMemAllocationErr;
				goto error;
			}

			found = false;

			while (playerIterator->Next(&theItem) && !found)
			{
				thePlayer = (PlayerListItem *)((UInt32ListMember *)theItem)->GetValue();

				if (thePlayer->id == inPlayer)
					found = true;
			}

			groups[groupCount++] = theGroup->id;

			delete playerIterator;
		}

		if (groupCount > mGameInfo.currentGroups)
		{
			DEBUG_PRINT("We've iterated over more groups than there are in the game!  Email NetSprocket team.");
			//Throw_(kNSpInvalidGroupIDErr);
			status = kNSpInvalidGroupIDErr;
			goto error;
		}
	}

error:
	if (status)
	{
		if (groups)
			InterruptSafe_free(groups);

		*outGroups = NULL;
		*outGroupCount = 0;
	}

	if (groupCount == 0)
	{
		InterruptSafe_free(groups);
		*outGroups = NULL;
		*outGroupCount = 0;
	}
	else
	{
		*outGroups = groups;
		*outGroupCount = groupCount;
	}
}

//----------------------------------------------------------------------------------------
// NSpGame::ReleaseGroups
//----------------------------------------------------------------------------------------

void
NSpGame::ReleaseGroups(NSpGroupID *inGroups)
{
	op_vassert_justreturn((inGroups != NULL),"Null pointer passed to Release Groups.  Don't Do that!");

	InterruptSafe_free(inGroups);
}

//----------------------------------------------------------------------------------------
// NSpGame::HandleCreateGroupMessage
//----------------------------------------------------------------------------------------

NMBoolean
NSpGame::HandleCreateGroupMessage(TCreateGroupMessage *inMessage)
{
NSp_InterruptSafeListIterator	iter(*mGroupList);
NSp_InterruptSafeListMember	*theItem;
GroupListItem				*theGroup;
NMBoolean					found = false;
GroupListItem				*groupEntry;

	groupEntry = new GroupListItem;

	if (NULL == groupEntry)
		return (false);

	//Ä	find the group in the list
	while ((false == found) && iter.Next(&theItem))
	{
		theGroup = (GroupListItem *)theItem;

		//Ä	The group already exists, just ignore the message and move on
		if (theGroup->id == inMessage->id)
			return (true);
	}

	groupEntry->id = inMessage->id;
	groupEntry->playerCount = 0;

	mGroupList->Append(groupEntry);

	mGameInfo.currentGroups++;

	//Ä	Pass the message up to the client
	if (NULL != mAsyncMessageHandler)
	{
	NSpCreateGroupMessage	clientMessage;
	
		NSpClearMessageHeader(&clientMessage.header);
		clientMessage.header.to = kNSpAllPlayers;
		clientMessage.header.what = kNSpGroupCreated;
		clientMessage.header.messageLen = sizeof (NSpCreateGroupMessage);
		clientMessage.header.when = inMessage->header.when;
		clientMessage.groupID = inMessage->id;
		clientMessage.requestingPlayer = inMessage->requestingPlayer;

		(void) (mAsyncMessageHandler)((NSpGameReference) this->GetGameOwner(), (NSpMessageHeader *) &clientMessage, mAsyncMessageContext);
	}

	return (true);
}

//----------------------------------------------------------------------------------------
// NSpGame::HandleDeleteGroupMessage
//----------------------------------------------------------------------------------------

NMBoolean
NSpGame::HandleDeleteGroupMessage(TDeleteGroupMessage *inMessage)
{
NMBoolean	handled;

	handled = (kNMNoError == DoDeleteGroup(inMessage->id));

	//Ä	Pass the message up to the client
	if ((true == handled) && (NULL != mAsyncMessageHandler))
	{
	NSpDeleteGroupMessage	clientMessage;
	
		NSpClearMessageHeader(&clientMessage.header);
		clientMessage.header.to = kNSpAllPlayers;
		clientMessage.header.what = kNSpGroupDeleted;
		clientMessage.header.messageLen = sizeof (NSpDeleteGroupMessage);
		clientMessage.header.when = inMessage->header.when;
		clientMessage.groupID = inMessage->id;
		clientMessage.requestingPlayer = inMessage->requestingPlayer;

		(void) (mAsyncMessageHandler)((NSpGameReference) this->GetGameOwner(), (NSpMessageHeader *) &clientMessage, mAsyncMessageContext);
	}

	return (handled);
}

//----------------------------------------------------------------------------------------
// NSpGame::DoDeleteGroup
//----------------------------------------------------------------------------------------

NMErr
NSpGame::DoDeleteGroup(NSpGroupID inID)
{
NSp_InterruptSafeListIterator	iter(*mGroupList);
NSp_InterruptSafeListMember	*theItem;
GroupListItem				*theGroup;
NMBoolean					bDeleteAll = false;

	if (kNSpAllGroups == inID)
		bDeleteAll = true;

	if (mGameInfo.currentGroups < 1)
		return (kNSpNoGroupsErr);

	//Ä	find the group in the list
	while (iter.Next(&theItem))
	{
		theGroup = (GroupListItem *)theItem;

		if (bDeleteAll || theGroup->id == inID)
		{
			mGroupList->Remove(theItem);
			delete theGroup;
			mGameInfo.currentGroups--;

			if (false == bDeleteAll)
				break;
		}
	}

	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// NSpGame::HandleAddPlayerToGroupMessage
//----------------------------------------------------------------------------------------

NMBoolean
NSpGame::HandleAddPlayerToGroupMessage(TAddPlayerToGroupMessage *inMessage)
{
	NSp_InterruptSafeListIterator	iter(*mGroupList);
	NSp_InterruptSafeListIterator 	playerIter(*mPlayerList);
	NSp_InterruptSafeListMember 	*theItem;
	PlayerListItem				*thePlayer = NULL;
	GroupListItem				*theGroup = NULL;
	NMBoolean					found = false;
	NMErr						status = kNMNoError;
	
	if (mGameInfo.currentPlayers < 1)
		return (false);

	//Ä	First find the player in the list
	while (!found && playerIter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *)theItem;
		if (thePlayer->id == inMessage->player)
		{
			found = true;
		}
	}

	//Ä	If we didn't find it, bail
	if (!found)
		return (false);

	if (mGameInfo.currentGroups < 1)
		return (false);

	//Try_
	{
		NMBoolean	handled = false;
	
		found = false;

		//Ä	First find the group in the list
		while (!found && iter.Next(&theItem))
		{
			theGroup = (GroupListItem *)theItem;
			if (theGroup->id == inMessage->group)
			{
				found = true;
			}
		}

		//Ä	If we didn't find it, bail
		if (!found)
			return (false);

		//Ä	We found it.  Let's add the player
		handled = theGroup->AddPlayer(thePlayer);

		//Ä	Pass the message up to the client
		if ((true == handled) && (NULL != mAsyncMessageHandler))
		{
		NSpAddPlayerToGroupMessage	clientMessage;
		
			NSpClearMessageHeader(&clientMessage.header);
			clientMessage.header.to = kNSpAllPlayers;
			clientMessage.header.what = kNSpPlayerAddedToGroup;
			clientMessage.header.messageLen = sizeof (NSpAddPlayerToGroupMessage);
			clientMessage.header.when = inMessage->header.when;
			clientMessage.group = inMessage->group;
			clientMessage.player = inMessage->player;
			
			(void) (mAsyncMessageHandler)((NSpGameReference) this->GetGameOwner(), (NSpMessageHeader *) &clientMessage, mAsyncMessageContext);
		}
		
		return (handled);
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		(void) code;

		return (false);
	}
}

//----------------------------------------------------------------------------------------
// NSpGame::HandleRemovePlayerFromGroupMessage
//----------------------------------------------------------------------------------------

NMBoolean
NSpGame::HandleRemovePlayerFromGroupMessage(TRemovePlayerFromGroupMessage *inMessage)
{
	NSp_InterruptSafeListIterator	iter(*mGroupList);
	NSp_InterruptSafeListMember	*theItem;
	GroupListItem				*theGroup = NULL;
	NMBoolean					found = false;
	NMErr						status = kNMNoError;

	if (mGameInfo.currentGroups < 1)
		return (false);

	//Try_
	{
		NMBoolean	handled = false;
	
		//Ä	First find the group in the list
		while (!found && iter.Next(&theItem))
		{
			theGroup = (GroupListItem *)theItem;

			if (theGroup->id == inMessage->group)
				found = true;
		}

		//Ä	If we didn't find it, bail
		if (!found)
			return (false);

		//Ä	We found it.  Let's add the player
		handled = theGroup->RemovePlayer(inMessage->player);

		//Ä	Pass the message up to the client
		if ((true == handled) && (NULL != mAsyncMessageHandler))
		{
		NSpRemovePlayerFromGroupMessage	clientMessage;
		
			NSpClearMessageHeader(&clientMessage.header);
			clientMessage.header.to = kNSpAllPlayers;
			clientMessage.header.what = kNSpPlayerRemovedFromGroup;
			clientMessage.header.messageLen = sizeof (NSpRemovePlayerFromGroupMessage);
			clientMessage.header.when = inMessage->header.when;
			clientMessage.group = inMessage->group;
			clientMessage.player = inMessage->player;
			
			(void) (mAsyncMessageHandler)((NSpGameReference) this->GetGameOwner(), (NSpMessageHeader *) &clientMessage, mAsyncMessageContext);
		}

		return (handled);
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		(void) code;

		return (false);
	}

	return (true);
}

#if defined(__MWERKS__)	
#pragma mark ======= Messaging =========
#endif

//----------------------------------------------------------------------------------------
// NSpGame::DoSelfSend
//----------------------------------------------------------------------------------------

NMErr
NSpGame::DoSelfSend(NSpMessageHeader *inHeader, void *inBody, NSpFlags inFlags, NMBoolean inCopy)
{
UNUSED_PARAMETER(inBody);
UNUSED_PARAMETER(inFlags);
UNUSED_PARAMETER(inCopy);


	ERObject		*theERObject;

	theERObject = GetFreeERObject(inHeader->messageLen);

	if (NULL == theERObject)
		return (kNSpFreeQExhaustedErr);

	//Ä	Message headers and their data are now contiguous in memory, so
	//	we can copy them with one move...
	machine_move_data(inHeader, theERObject->PeekNetMessage(), inHeader->messageLen);

	//Ä	Set the when
	theERObject->PeekNetMessage()->when = ::GetTimestampMilliseconds() + mTimeStampDifferential;

	HandleEventForSelf(theERObject, NULL);

	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// NSpGame::HandleEventForSelf
//----------------------------------------------------------------------------------------

void
NSpGame::HandleEventForSelf(ERObject *inERObject, CEndpoint *inEndpoint)
{
NMBoolean	enQ = true;

	//Ä	Set the endpoint and address
	if (inEndpoint)
		inERObject->SetEndpoint(inEndpoint);

	if (mAsyncMessageHandler && !((inERObject->PeekNetMessage()->what & kPrivateMessage) == kPrivateMessage) )
	{
		enQ = ((NSpMessageHandlerProcPtr)mAsyncMessageHandler)((NSpGameReference) this->GetGameOwner(),
		                    inERObject->PeekNetMessage(), mAsyncMessageContext);	}

	if (enQ)
	{
		mEventQ->Enqueue(inERObject);
		mMessageQLen++;
	}
	else
	{
		ReleaseERObject(inERObject);
	}
}

//----------------------------------------------------------------------------------------
// NSpGame::ServiceSystemQueue
// Note:  Do NOT call this function at interrupt time!  The whole reason for
// this function's existence was to move non-interrupt-safe handling out of
// interrupt time!
//----------------------------------------------------------------------------------------

void
NSpGame::ServiceSystemQueue(void)
{
	ERObject *theERObject = NULL;

	if (mSystemEventQ->IsEmpty())
		return;
	
	// Grab the private message list from the private event queue...
	
	theERObject = (ERObject *) mSystemEventQ->StealList();
	
	DEBUG_PRINT("Servicing the private message queue...");
	
	// Note:  The previous call clears the system event queue automically,
	// so it is immediately ready to accept new system events at interrupt time.
	// Now that we have a handle to a linked list of messages that have
	// built up since the last time this function was called, and it is
	// currently NOT interrupt time, we can walk through the messages and
	// handle them...
	
	if (theERObject == NULL) // <-- Shouldn't be possible, but just in case...
		return;
	else
		::NewReverse(&theERObject);
		
	while(theERObject != NULL)
	{
		// Before messing with the current ERObject, capture the pointer
		// to the next ERObject...
		
		ERObject *nextERObject = (ERObject *) theERObject->fNext;
		
		// Handle the current ERObject the way that they used to be handled at
		// interrupt time...
		
		this->HandleEvent(theERObject, theERObject->GetEndpoint() , theERObject->GetCookie());
	
		theERObject = nextERObject;
	}
	
	
}

//----------------------------------------------------------------------------------------
// NSpGame::NSpMessage_Get
//----------------------------------------------------------------------------------------

NMBoolean
NSpGame::NSpMessage_Get(NSpMessageHeader **outMessage)
{
	ERObject			*theERObject = NULL;
	NSpMessageHeader	*theMessage = NULL;
	NMErr				status = kNMNoError;

	//Ä	If there aren't already messages in our list, dump the q
	if (!mPendingMessages)
	{
		if (mEventQ->IsEmpty())
			return false;

		mPendingMessages = (ERObject *) mEventQ->StealList();
		::NewReverse(&mPendingMessages);
		
	}

	theERObject = mPendingMessages;
	mPendingMessages = (ERObject *) mPendingMessages->fNext;

	mMessageQLen--;

	theMessage = theERObject->PeekNetMessage();
	if (theMessage == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	//Ä	This eats out the nice creme filling and leaves only the cookie
	theMessage = theERObject->RemoveNetMessage();
	*outMessage = theMessage;

	//Ä	Put the ERObject in the cookie q, waiting for a new NSpMessageHeader before going back to the free q
	mCookieQ->Enqueue(theERObject);
	mCookieQLen++;

	return (true);

error:
	if (status)
	{
		return (false);
	}
	return true;
}

//----------------------------------------------------------------------------------------
// NSpGame::GetRTT
//----------------------------------------------------------------------------------------

NMUInt32
NSpGame::GetRTT(NSpPlayerID inPlayer)
{
UNUSED_PARAMETER(inPlayer);

	DEBUG_PRINT("GetRTT not implemented");

	return 0;
}

//----------------------------------------------------------------------------------------
// NSpGame::GetThruput
//----------------------------------------------------------------------------------------

NMUInt32
NSpGame::GetThruput(NSpPlayerID inPlayer)
{
UNUSED_PARAMETER(inPlayer);

	DEBUG_PRINT("GetThruput not implemented");

	return 0;
}

//----------------------------------------------------------------------------------------
// NSpGame::GetCurrentTimeStamp
//----------------------------------------------------------------------------------------

NMUInt32
NSpGame::GetCurrentTimeStamp(void)
{
//UnsignedWide	baseTime;
NMUInt32			result;

//	::machine_timestamp(&baseTime);
	
//	result = ::machine_timestamp_milliseconds(&baseTime);
	result = ::GetTimestampMilliseconds();

	result += mTimeStampDifferential;

	return (result);
}

//----------------------------------------------------------------------------------------
// NSpGame::InstallAsyncMessageHandler
//----------------------------------------------------------------------------------------

void
NSpGame::InstallAsyncMessageHandler(
		NSpMessageHandlerProcPtr	inAsyncMessageHandler,
		void						*inAsyncMessageContext)
{
	mAsyncMessageHandler = inAsyncMessageHandler;
	mAsyncMessageContext = inAsyncMessageContext;
}

//----------------------------------------------------------------------------------------
// NSpGame::InstallCallbackHandler
//----------------------------------------------------------------------------------------

void
NSpGame::InstallCallbackHandler(
		NSpCallbackProcPtr	inCallbackHandler,
		void				*inCallbackContext)
{
	mCallbackHandler = inCallbackHandler;
	mCallbackContext = inCallbackContext;
}

//----------------------------------------------------------------------------------------
// NSpGame::GetQState
//----------------------------------------------------------------------------------------

void
NSpGame::GetQState(NMUInt32 *outFreeQ, NMUInt32 *outCookieQ, NMUInt32 *outMessageQ)
{
	*outFreeQ = mFreeQLen;
	*outCookieQ = mCookieQLen;
	*outMessageQ = mMessageQLen;
}


//----------------------------------------------------------------------------------------
// NSpGame::IsSystemEvent
//----------------------------------------------------------------------------------------

NMBoolean
NSpGame::IsSystemEvent(ERObject *inERObject)
{
	NSpMessageHeader	*theMessage = inERObject->PeekNetMessage();

	if (theMessage->what & kNSpSystemMessagePrefix)
		return true;
	else
		return false;
}
