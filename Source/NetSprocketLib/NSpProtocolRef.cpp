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
#include "NSpProtocolRef.h"
#include <string.h>
#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif

#ifdef OP_API_NETWORK_OT
	#include <OpenTptAppleTalk.h>
	#include <OpenTptInternet.h>
#endif


#include <stdio.h>

/*

extern NMUInt32 gCreatorType;
//----------------------------------------------------------------------------------------
// NSpProtocolPriv::NSpProtocolPriv 
//----------------------------------------------------------------------------------------
NSpProtocolPriv::NSpProtocolPriv()
{
	mID = 0;
	mMaxRTT = 0;
	mMinThruput = 0;
	mConfigString[0] = '\0';
	mCustomData = NULL;
}

//----------------------------------------------------------------------------------------
// NSpProtocolPriv::~NSpProtocolPriv 
//----------------------------------------------------------------------------------------

NSpProtocolPriv::~NSpProtocolPriv()
{
	if (mCustomData)
		InterruptSafe_free(mCustomData);
}

//----------------------------------------------------------------------------------------
// NSpProtocolPriv::CreateConfiguration 
//----------------------------------------------------------------------------------------

NMErr NSpProtocolPriv::CreateConfiguration(const char *inConfiguration)
{
	NMErr	status;
	
	strcpy(mConfigString, inConfiguration);
	
	status = ParseConfigString(inConfiguration);
	
	return status;
}

//----------------------------------------------------------------------------------------
// NSpProtocolPriv::ParseConfigString 
//----------------------------------------------------------------------------------------

NMErr NSpProtocolPriv::ParseConfigString(const char *inConfiguration)
{
	NMErr	status = kNMNoError;
	const char	*p = inConfiguration;
	const char 	*endOfString = p + kNSpMaxDefinitionStringLen;

	Try_
	{	

		if (strncmp(inConfiguration, kATModuleType, 4) == 0)
		{
		
		#ifndef OP_API_NETWORK_OT
		
			return (kNSpInvalidProtocolRefErr);	// No such thing as AppleTalk on a non-MacOS platform.
			
		#else
		
			ATInfo	*atInfo = (ATInfo *)InterruptSafe_alloc(sizeof(ATInfo));
			ThrowIfNULL_(atInfo);
			
			mID = kATModuleType;
			p += 4;		
			
			const char *q = p;
			NMSInt32 i;

			sscanf(p, "%d\n%d\n", &mMaxRTT, &mMinThruput);

//			Ugly, could use some cleaning.
			for (i = 0; i < 2; i++)
			{
				while (*q++ != '\n' && q < endOfString) {};
			}
//			At this point, we should be pointing to the name
			p = q;
			while (*q != '\n' && q <= endOfString)
				q++;
			if (*q == '\n')
			{
				NMSInt32 len = q - p;
				machine_move_data(p, &atInfo->nbpName[1], len);
				atInfo->nbpName[0] = len;
				p = ++q;
				while (*q != '\n' && q < endOfString)
					q++;
				len = q - p;
				machine_move_data(p, &atInfo->nbpType[1], len);
				atInfo->nbpType[0] = len;
			}
			else
			{
				atInfo->nbpType[0] = 4;
				machine_move_data(&gCreatorType, &atInfo->nbpType[1], 4);
			}
			
			mCustomData = atInfo;
			
		#endif
		}
		else if (strncmp(inConfiguration, kIPModuleType, 4) == 0)
		{
			IPInfo	*info = new IPInfo;
			ThrowIfNULL_(info);
			NMSInt32		cruft;
			
			mID = kIPModuleType;
			p += 4;
			
			sscanf(p, "%d\n%d\n%d\n", &mMaxRTT, &mMinThruput, &cruft);
			info->port = (NMInetPort) cruft;
			
			mCustomData = info;
		}
		else
			status = kNSpInvalidDefinitionErr;
	}
	Catch_(code)
	{
		if (code == kNSpNilPointerErr)
			return kNSpMemAllocationErr;
	}	
	return status;
}

//----------------------------------------------------------------------------------------
// NSpProtocolPriv::GetConfigString 
//----------------------------------------------------------------------------------------

NMErr NSpProtocolPriv::GetConfigString(char *outConfigString)
{
	NMErr status = kNMNoError;
	strcpy(outConfigString, mConfigString);
	
	return status;
}
*/

//----------------------------------------------------------------------------------------
// NSpProtocolListPriv::NSpProtocolListPriv 
//----------------------------------------------------------------------------------------

NSpProtocolListPriv::NSpProtocolListPriv()
{
	mList = new TPointerArray();
}

//----------------------------------------------------------------------------------------
// NSpProtocolListPriv::~NSpProtocolListPriv 
//----------------------------------------------------------------------------------------

NSpProtocolListPriv::~NSpProtocolListPriv()
{
	if (mList)
		delete mList;
}

//----------------------------------------------------------------------------------------
// NSpProtocolListPriv::GetCount 
//----------------------------------------------------------------------------------------

NMUInt32 NSpProtocolListPriv::GetCount(void)
{
	return mList->Count();
}

//----------------------------------------------------------------------------------------
// NSpProtocolListPriv::GetIndexedItem 
//----------------------------------------------------------------------------------------

NSpProtocolPriv *NSpProtocolListPriv::GetIndexedItem(NMUInt32	inIndex)
{
	return (NSpProtocolPriv *) mList->GetIndexedItem(inIndex);
}

//----------------------------------------------------------------------------------------
// NSpProtocolListPriv::Append 
//----------------------------------------------------------------------------------------

NMErr NSpProtocolListPriv::Append(NSpProtocolPriv *inProtocol)
{
	return	mList->AddItem(inProtocol);
}

//----------------------------------------------------------------------------------------
// NSpProtocolListPriv::Remove 
//----------------------------------------------------------------------------------------

NMErr NSpProtocolListPriv::Remove(NSpProtocolPriv *inProtocol)
{
	NMErr	status;
	
	if (mList->RemoveItem(inProtocol))
		status = kNMNoError;
	else
		status = kNSpInvalidProtocolRefErr;
		
	return status;
}

//----------------------------------------------------------------------------------------
// NSpProtocolListPriv::RemoveIndexedItem 
//----------------------------------------------------------------------------------------

NMErr NSpProtocolListPriv::RemoveIndexedItem(NMUInt32	inIndex)
{
	NMErr status;
	
	if (mList->RemoveItemByIndex(inIndex))
		status = kNMNoError;
	else
		status = kNSpInvalidProtocolRefErr;

	return status;		
}

