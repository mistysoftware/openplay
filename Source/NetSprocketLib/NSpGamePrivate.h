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

#ifndef __NSPGAMEPRIVATE__
#define __NSPGAMEPRIVATE__

//	------------------------------	Includes

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#include "NSpGameMaster.h"
	#include "NSpGameSlave.h"

//	------------------------------	Public Types


	class NSpGamePrivate
	{
	public:
		enum {class_ID = 'NSpP'};
		NSpGamePrivate();
		~NSpGamePrivate();
		
		inline	void	SetMaster(NSpGameMaster *inMaster) { mMaster = inMaster; mMaster->SetGameOwner(this);}
		inline	void	SetSlave(NSpGameSlave *inSlave) { mSlave = inSlave; mSlave->SetGameOwner(this);}
		inline	NSpGameMaster	*GetMaster(void) { return mMaster;}
		inline	NSpGameSlave	*GetSlave(void) { return mSlave;}
		
				NSpGame		*GetGameObject(void);
				
				NMErr	PrepareForDeletion(NSpFlags inFlags);
				NMBoolean	NSpMessage_Get(NSpMessageHeader **outMessage);

	protected:
		NSpGameMaster	*mMaster;
		NSpGameSlave	*mSlave;

	};
	
#endif // __NSPGAMEPRIVATE__
