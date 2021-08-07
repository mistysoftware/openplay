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


#ifndef __NSPPROTOCOLREF__
#define __NSPPROTOCOLREF__

//	------------------------------	Includes

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#include "TPointerArray.h"


/*******************************************************************************
** Module Names
********************************************************************************/
//	------------------------------	Public Definitions

/* LR -- unused, duplicate usage, bad!

	#define kInetModuleID		"Inet"
	#define kAtlkModuleID		"Atlk"
	#define kModuleIDLength		4

	enum
	{
		kAppleTalk = 1,
		kTCPIP
	};
*/

//	------------------------------	Public Types

	typedef struct ATInfo
	{
		unsigned char	nbpName[32];
		unsigned char	nbpType[32];
	} ATInfo;

	typedef struct IPInfo
	{
		NMInetPort	port;
	} IPInfo;


	typedef NMUInt32		NSpProtocolPriv;

	/*
	class NSpProtocolPriv
	{
	public:
		enum {class_ID = 'Prot'};
		NSpProtocolPriv();
		~NSpProtocolPriv();
		
		NMErr	CreateConfiguration(const char *inConfiguration);
		NMErr	GetConfigString(char *outConfigString);
		
		inline	NMUInt32 GetID(void) { return mID;}
		inline	NMUInt32 GetMaxRTT(void) { return mMaxRTT;}
		inline	NMUInt32 GetMinThruput(void) { return mMinThruput;}
		inline	void	*GetCustomData(void) {return mCustomData;}
		
	protected:
		NMErr	ParseConfigString(const char *inConfigString);

		NMUInt32		mID;
		NMUInt32		mMaxRTT;
		NMUInt32		mMinThruput;
		char 		mConfigString[kNSpMaxDefinitionStringLen];
		void		*mCustomData;
		
	};
	*/



	class NSpProtocolListPriv
	{
	public:
		enum {class_ID = 'PLst'};
		NSpProtocolListPriv();
		~NSpProtocolListPriv();

		NMUInt32			GetCount(void);
		NSpProtocolPriv		*GetIndexedItem(NMUInt32	inIndex);
		NMErr			Append(NSpProtocolPriv *inProtocol);
		NMErr			Remove(NSpProtocolPriv *inProtocol);
		NMErr			RemoveIndexedItem(NMUInt32 inIndex);
		
	protected:	
		TPointerArray	*mList;
	};

#endif // __NSPPROTOCOLREF__

