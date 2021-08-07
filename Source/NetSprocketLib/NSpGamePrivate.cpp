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
#include "NSpGamePrivate.h"


//----------------------------------------------------------------------------------------
// NSpGamePrivate::NSpGamePrivate
//----------------------------------------------------------------------------------------

NSpGamePrivate::NSpGamePrivate()
{
	mMaster = NULL;
	mSlave = NULL;
}

//----------------------------------------------------------------------------------------
// NSpGamePrivate::~NSpGamePrivate
//----------------------------------------------------------------------------------------

NSpGamePrivate::~NSpGamePrivate()
{
	if (mMaster)
		delete mMaster;
	if (mSlave)
		delete mSlave;
}

//----------------------------------------------------------------------------------------
// NSpGamePrivate::GetGameObject
//----------------------------------------------------------------------------------------

NSpGame *NSpGamePrivate::GetGameObject(void)
{
	if (mMaster)
		return mMaster;
	else
		return mSlave;
}

//----------------------------------------------------------------------------------------
// NSpGamePrivate::PrepareForDeletion
//----------------------------------------------------------------------------------------

NMErr NSpGamePrivate::PrepareForDeletion(NSpFlags inFlags)
{
	NMErr	status = kNMNoError;
	
	if (mMaster)
		status = mMaster->PrepareForDeletion(inFlags);
	else if (mSlave)
		status = mSlave->PrepareForDeletion(inFlags);
					
	return status;
}

//----------------------------------------------------------------------------------------
// NSpGamePrivate::NSpMessage_Get
//----------------------------------------------------------------------------------------

NMBoolean NSpGamePrivate::NSpMessage_Get(NSpMessageHeader **outMessage)
{
	NMBoolean	gotEvent = false;
	
	if (mMaster)
	{
		//provide idle processing time if needed
		mMaster->IdleEndpoints();
		// Get messages from the internal (aka "Private") message queue,
		// process them, and put them in the user's queue when appropriate...
		mMaster->ServiceSystemQueue();
		// Now get the user's message...
		gotEvent = mMaster->NSpMessage_Get(outMessage);
	}
	else if (mSlave)
	{
		//provide idle processing time if needed
		mSlave->IdleEndpoints();
		// Get messages from the internal (aka "Private") message queue,
		// process them, and put them in the user's queue when appropriate...
		mSlave->ServiceSystemQueue();
		// Now get the user's message...
		gotEvent = mSlave->NSpMessage_Get(outMessage);
	}
	
	return gotEvent;
}
