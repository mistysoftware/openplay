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
#include "NSpLists_OP.h"


//----------------------------------------------------------------------------------------
// PlayerListItem::PlayerListItem 
//----------------------------------------------------------------------------------------

PlayerListItem::PlayerListItem()
{
	id = 0;
	endpoint = NULL;
	info = NULL;
}

//----------------------------------------------------------------------------------------
// PlayerListItem::~PlayerListItem 
//----------------------------------------------------------------------------------------

PlayerListItem::~PlayerListItem()
{
	//Ä	We delete the info but not the endpoint.  Someone else might own it.
	if (NULL != info)
		InterruptSafe_free(info);
}

//----------------------------------------------------------------------------------------
// GroupListItem::GroupListItem 
//----------------------------------------------------------------------------------------

GroupListItem::GroupListItem()
{
	id = 0;
	playerCount = 0;
	players = new NSp_InterruptSafeList();
}

//----------------------------------------------------------------------------------------
// GroupListItem::~GroupListItem 
//----------------------------------------------------------------------------------------

GroupListItem::~GroupListItem()
{
	delete players;
}

//----------------------------------------------------------------------------------------
// GroupListItem::AddPlayer 
//----------------------------------------------------------------------------------------

NMBoolean
GroupListItem::AddPlayer(PlayerListItem *inPlayer)
{
UInt32ListMember	*item;

	if (NULL == players)
		return (false);
		
	//Ä	We can't just add this item, since it belongs to another list
	//Ä	Instead add an item that contains a pointer to it
	item = new UInt32ListMember((NMUInt32) inPlayer);

	if (NULL == item)
		return (false);

	players->Append(item);
	playerCount++;
	
	return (true);
}

//----------------------------------------------------------------------------------------
// GroupListItem::RemovePlayer 
//----------------------------------------------------------------------------------------

NMBoolean
GroupListItem::RemovePlayer(NSpPlayerID inPlayer)
{
NSp_InterruptSafeListIterator	iter(*players);
NSp_InterruptSafeListMember	*	theItem;
PlayerListItem				*	player;
NMBoolean						found = false;
	
	if (NULL == players)
		return (false);
		
	//Ä	First find the player in the list
	while ((false == found) && iter.Next(&theItem))
	{
		player = (PlayerListItem *) (((UInt32ListMember *) theItem)->GetValue());

		if (player->id == inPlayer)
			found = true;
	}

	//Ä	If we didn't find it, bail
	if (false == found)
		return 	(false);
		
	//Ä	We did find it.  Let's remove it
	if (players->Remove(theItem))
		playerCount--;
	else
	{
#if INTERNALDEBUG
		debug_message("We found the player, but we couldn't remove it.  Bogus.");
#endif
		return (false);
	}
	
	return (true);
}

//----------------------------------------------------------------------------------------
// SendQItem::SendQItem 
//----------------------------------------------------------------------------------------

SendQItem::SendQItem(void *inData, NMUInt32 inBytesSent)
{
	void	*savedData = NULL;
	NMUInt32	tempMessageLength;
	NMUInt32	messageLength;
	NMUInt32	leftToSend;
	
	
	
	//Ä	Initialize everything to something boring, just in case.
	
	mData = NULL;
	
	mTotalSent = 0;
	
	mMessageLength = 0;


	//Ä	Now fill them in right.
	
	//Ä	If this is a non-Macintosh platform, then every path to this point
	//	has byte-swapped the header.  So we need to byte-swap the variable
	//	"messageLength" again if we want to use the right value, which we
	//	do.  
	//
	//	To be sure that this works properly, I believe that tempMessageLength
	//	must always maintain the same data type as does NSpMessageHeader.messageLen.
	//	Perhaps we should define a type NSpMessageLenType and use that to define
	//	both variables to ensure future consistency?
	//
	//	(CRT, July 7, 2000)
	
	tempMessageLength = ((NSpMessageHeader *)inData)->messageLen;
	
#if !big_endian
	tempMessageLength = SWAP4(tempMessageLength);
#endif
	
	messageLength = tempMessageLength;
		
	leftToSend = messageLength - inBytesSent;
			
	savedData = (void *) InterruptSafe_alloc(leftToSend);
	//ThrowIfNULL_(savedData);
	op_assert(savedData);
						
	machine_move_data((char *)inData + inBytesSent, savedData, leftToSend);
	
	mTotalSent = 0;
	
	mMessageLength = leftToSend;
	
	mData = savedData;			

}

//----------------------------------------------------------------------------------------
// SendQItem::AdvanceBuffPtr 
//----------------------------------------------------------------------------------------

void
SendQItem::AdvanceBuffPtr(NMUInt32 inBytes)
{
	mTotalSent += inBytes;
}

NMUInt32
SendQItem::BytesLeft()
{
	NMUInt32	bytes;
	
	bytes = mMessageLength - mTotalSent;

	return (bytes);
}

//----------------------------------------------------------------------------------------
// SendQItem::~SendQItem 
//----------------------------------------------------------------------------------------

SendQItem::~SendQItem()
{
		
	if (mData)
		InterruptSafe_free(mData);
	
}

