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

//	------------------------------	Includes

#include "IPEnumerator.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions
//	------------------------------	Public Variables


//----------------------------------------------------------------------------------------
// IPEnumerationItemPriv::IPEnumerationItemPriv
//----------------------------------------------------------------------------------------

IPEnumerationItemPriv::IPEnumerationItemPriv(IPEnumerationResponsePacket *inPacket)
{
	age = 0;

	//	Copy the response to the enumeration item
	packet = *inPacket;
	
	if (packet.customEnumDataLen > 0)
	{
		userEnum.customEnumData = new char[packet.customEnumDataLen];

		machine_move_data(extract_enumeration_data_from_ip_response((char *)inPacket), 
			userEnum.customEnumData, packet.customEnumDataLen);
	}
	else
	{
		userEnum.customEnumData = NULL;
	}
	
	//	Fill in the user's part of the structure
	userEnum.customEnumDataLen = packet.customEnumDataLen;		
	userEnum.id = (NMType) this;
	userEnum.name = packet.name;
}

//----------------------------------------------------------------------------------------
// IPEnumerationItemPriv::~IPEnumerationItemPriv
//----------------------------------------------------------------------------------------

IPEnumerationItemPriv::~IPEnumerationItemPriv()
{
	if (userEnum.customEnumData)
		delete[] userEnum.customEnumData;
}

//----------------------------------------------------------------------------------------
// IPEnumerator::IPEnumerator
//----------------------------------------------------------------------------------------

IPEnumerator::IPEnumerator(
	NMIPConfigPriv				*inConfig,
	NMEnumerationCallbackPtr	inCallback,
	void						*inContext,
	NMBoolean					inActive)
{
	mConfig = *inConfig;
	mCallback = inCallback;
	mContext = inContext;
	bActive = inActive;
	
	build_ip_enumeration_request_packet((char *) mPacketData);
}

//----------------------------------------------------------------------------------------
// IPEnumerator::~IPEnumerator
//----------------------------------------------------------------------------------------

IPEnumerator::~IPEnumerator()
{
	DeleteAllItems(&mList, IPEnumerationItemPriv*);
}

//----------------------------------------------------------------------------------------
// IPEnumerator::HandleReply
//----------------------------------------------------------------------------------------

void
IPEnumerator::HandleReply(
	NMUInt8		*inData,
	NMUInt32		inLen,
	InetAddress	*inAddress)
{
	DEBUG_ENTRY_EXIT("IPEnumerator::HandleReply");

IPEnumerationResponsePacket	*thePacket = (IPEnumerationResponsePacket *) inData;
IPEnumerationItemPriv		*theItem;
	
	UNUSED_PARAMETER(inLen);

	//	Step 1.  Find out if this is really a lookup reply
	if (thePacket->responseCode != kReplyFlag)
		return;
		
	//	if the packet doesn't have a real host, we assume they don't know who they are
	//	(the schmucks), and set the address to the address of the sender.  This is NOT
	//	ALWAYS the right thing to do.  For instance, you could have a server that doles
	//	out addresses in response to an enum request.
	if (thePacket->host == 0)
		thePacket->host = inAddress->fHost;
		
	//	Step 2.  Create the enumeration item we'll store in our list
	theItem = new IPEnumerationItemPriv((IPEnumerationResponsePacket *) inData);

	if (!theItem)
	{
		op_vpause("Unable to alloc memory for the enumeration response!");
		return;
	}
		
	//	Make sure it's not already in the list
	NMUInt32 count = mList.Count();
	IPEnumerationItemPriv *searchItem;
	
	for (NMSInt32 i = 0; i < count; i++)
	{
		searchItem = (IPEnumerationItemPriv *) mList.GetIndexedItem(i);

		if (searchItem->packet.host == theItem->packet.host
		&&	searchItem->packet.port == theItem->packet.port)
		{
			delete theItem;
			searchItem->age = 0;
			return;
		}
	}
	
	//	Add it to the list
	mList.AddItem(theItem);
	
	//	Step 4. Make the callback to the user
	(mCallback)(mContext, kNMEnumAdd, &theItem->userEnum);
}

//----------------------------------------------------------------------------------------
// IPEnumerator::SetConfigFromEnumerationItem
//----------------------------------------------------------------------------------------

NMErr
IPEnumerator::SetConfigFromEnumerationItem(
	NMIPConfigPriv			*ioConfig,
	IPEnumerationItemPriv	*inItem)
{
	ioConfig->type = kModuleID;
	ioConfig->version = kVersion;
	OTInitInetAddress(&ioConfig->address, inItem->packet.port, inItem->packet.host);

	strcpy(ioConfig->name, inItem->packet.name);

	ioConfig->customEnumDataLen = inItem->userEnum.customEnumDataLen;

	if (ioConfig->customEnumDataLen > 0)
		machine_move_data(inItem->userEnum.customEnumData, ioConfig->customEnumData, ioConfig->customEnumDataLen);
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// IPEnumerator::IdleList
//----------------------------------------------------------------------------------------

void
IPEnumerator::IdleList(NMUInt32 timeSinceLastEnum)
{

	//	Go through the list and age the items.  Delete any items that are too old
	NMUInt32 count = mList.Count();
	IPEnumerationItemPriv *item;
	
	for (NMSInt32 i = count - 1; i >= 0; i--)
	{
		item = (IPEnumerationItemPriv *) mList.GetIndexedItem(i);
		item->age += timeSinceLastEnum;

		if (item->age > kMaxEnumResponseItemAge)
		{
			(mCallback)(mContext, kNMEnumDelete, &item->userEnum);
			mList.RemoveItemByIndex(i);
			delete item;
		}
	}
}
