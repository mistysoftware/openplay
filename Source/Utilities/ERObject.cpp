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

#include "NSpPrefix.h"

#include "ERObject.h"

//----------------------------------------------------------------------------------------
// ERObject::ERObject
//----------------------------------------------------------------------------------------

ERObject::ERObject()
{
	Init();
	mMessage = NULL;
	mMaxMessageLen = 0;
	mTimeReceived = 0;
	mEndpoint = NULL;
}

//----------------------------------------------------------------------------------------
// ERObject::ERObject
//----------------------------------------------------------------------------------------

ERObject::ERObject(NSpMessageHeader *inMessage, NMUInt32 inMaxLen) 
{
	mMessage = inMessage;
	mMaxMessageLen = inMaxLen;
	mEndpoint = NULL;
}

//----------------------------------------------------------------------------------------
// ERObject::~ERObject
//----------------------------------------------------------------------------------------

ERObject::~ERObject()
{
	if (mMessage)
	{
		delete mMessage;
	}
}

//----------------------------------------------------------------------------------------
// ERObject::Clear
//----------------------------------------------------------------------------------------

void ERObject::Clear()
{

	if (mMessage != NULL)
		NSpClearMessageHeader(mMessage);	
	mEndpoint = NULL;
	mTimeReceived = 0;
}

//----------------------------------------------------------------------------------------
// ERObject::CopyNetMessage
//----------------------------------------------------------------------------------------

NMBoolean ERObject::CopyNetMessage(NMUInt8 *inBuffer, NMUInt32 inLen)
{

//	Make sure we have a valid buffer to receive the net message
	if (!mMessage || mMaxMessageLen < inLen)
		return false;
		
//	Fill in the message structure
	machine_move_data(inBuffer, (char *) &mMessage, inLen);

	return true;
}

//----------------------------------------------------------------------------------------
// ERObject::CopyNetMessage
//----------------------------------------------------------------------------------------

NMBoolean ERObject::CopyNetMessage(NSpMessageHeader *inMessage)
{

//	Make sure we have a valid buffer to receive the net message
	if (!mMessage || mMaxMessageLen < inMessage->messageLen)
		return false;

	machine_move_data(inMessage, (char *) mMessage, inMessage->messageLen);
	
	return true;
}

//----------------------------------------------------------------------------------------
// ERObject::SetNetMessage
//----------------------------------------------------------------------------------------

NMBoolean ERObject::SetNetMessage(NSpMessageHeader *inMessage, NMUInt32 inMaxLen)
{
//	If we already have a message, don't erase it
	if (mMessage != NULL)
		return false;
		
	mMessage = inMessage;
	mMaxMessageLen = inMaxLen;
	
	return true;
}

//----------------------------------------------------------------------------------------
// ERObject::RemoveNetMessage
//----------------------------------------------------------------------------------------

NSpMessageHeader *ERObject::RemoveNetMessage(void)
{
	NSpMessageHeader *theMessage = mMessage;
	
	mMessage = NULL;
	mMaxMessageLen = 0;
	
	return theMessage;
}

//----------------------------------------------------------------------------------------
// ERObject::SetEndpoint
//----------------------------------------------------------------------------------------

void ERObject::SetEndpoint(CEndpoint *inEndpoint)
{
	mEndpoint = inEndpoint;
}

//----------------------------------------------------------------------------------------
// ERObject::SetCookie
//----------------------------------------------------------------------------------------

void ERObject::SetCookie(void *inCookie)
{
	mCookie = inCookie;
}

//----------------------------------------------------------------------------------------
// RecursiveReverse
//----------------------------------------------------------------------------------------

void RecursiveReverse(ERObject **headRef)
{
	ERObject	*first;
	ERObject	*rest;

	if (headRef == NULL) return;
	
	first = *headRef;
	rest = (ERObject *) first->fNext;
	
	if (rest == NULL) return;
	
	RecursiveReverse(&rest);
	
	first->fNext->fNext = (NMLink *) first;
	first->fNext = NULL;
	
	*headRef = rest;
}


//------------------------------------------------------------------------------------------
//  NewReverse
//------------------------------------------------------------------------------------------

void NewReverse(ERObject **object)
{
	ERObject *oldNext;
	ERObject *newNext = NULL;

	op_assert(object != NULL);
	op_vassert_justreturn(*object != NULL,"NewReverse: *object == NULL");	
	oldNext = (ERObject*)(*object)->fNext;
	
	while (oldNext)
	{
		(*object)->fNext = newNext;
		newNext = *object;
		*object = oldNext;
		oldNext = (ERObject*)(*object)->fNext;
	}
	(*object)->fNext = newNext;
}