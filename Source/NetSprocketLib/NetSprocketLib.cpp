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

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif

#include "NSpPrefix.h"

#ifndef __NETMODULE__
#include	"NetModule.h"
#endif

#ifdef OP_API_NETWORK_OT
	#include <Gestalt.h>
	#include <OpenTransport.h>
	#include "OTUtils.h"
#endif

#include "NetSprocketLib.h"
#include "NSpGamePrivate.h"
#include "NSpGameMaster.h"
#include "NSpGameSlave.h"
#include "NSpProtocolRef.h"
#include <stdio.h>
#include "String_Utils.h"
#include "NSpVersion.h"

#ifdef OP_API_NETWORK_OT
	extern "C" int __initialize(CFragInitBlockPtr ibp);
	extern "C" int __terminate(void);
	
//	extern OSErr	__init_lib(CFragInitBlockPtr initBlockPtr);
//	extern void		__term_lib(void);

#endif

//	NMSInt32  gFileRefNum = 0;

enum
{
	kDefaultMemoryPoolSize = 100000,
	kDefaultQElements = 100
};

enum
{
	kUsingAppleTalk = 0x00000001,
	kUsingIP 		= 0x00000002
};


#define kUberMode		3


static NMNumVersion		gVersion;
NMUInt32				gEndpointConnectTimeout = 0;

// These globals are initialized on a per-library-connection basis

//QDGlobals				qd;

NMBoolean				gPPInitialized;
NSp_InterruptSafeList	*gGameList;
NMUInt32				gStandardMessageSize;
//NMUInt32				gBufferSize;
NMUInt32				gQElements;
NMUInt32				gTimeout;
NMUInt32				gCreatorType;
NMBoolean				gDebugMode;
NMBoolean				gPolling;


#ifdef OP_API_NETWORK_OT
	FSSpec					gFSSpec;
#endif

// Async stuff
NSpJoinRequestHandlerProcPtr gJoinRequestHandler;
void 						*gJoinRequestContext;

NSpMessageHandlerProcPtr gAsyncMessageHandler;
void					 *gAsyncMessageContext;

NSpCallbackProcPtr		gCallbackHandler;
void					*gCallbackContext;



//----------------------------------------------------------------------------------------
// NSpInitialize
//----------------------------------------------------------------------------------------

NMErr
NSpInitialize(NMUInt32 inStandardMessageSize, NMUInt32 inBufferSize, NMUInt32 inQElements, NSpGameID inGameID, NMUInt32 inTimeout)
{
	//SetupLibraryState	state;
	
	if (inBufferSize < 200000)	// if they ask for less than 200k, prealloc 200k
		inBufferSize = 200000;
		
	//Ä	We shan't be initialized more than once per application
	if (gPPInitialized)
		return (kNSpAlreadyInitializedErr);
	
#ifdef OP_API_NETWORK_OT
	NMErr			status = kNMNoError;

	NMErr		theErr;
	NMSInt32	result;
	
	//Ä	Make sure that Mac OS 8.1 or later is installed
	theErr = Gestalt(gestaltSystemVersion, &result);
	
	if ((kNMNoError != theErr) || (result < 0x00000810))
		return (kNSpInitializationFailedErr);
		
	status = OTUtils::StartOpenTransport(OTUtils::kOTDontCheckProtocol,inBufferSize,10240);

	if (status != kNMNoError)
		return (kNSpInitializationFailedErr);
	
#endif

	
	//Ä	Set the standard message size
	if (inStandardMessageSize == 0)
		gStandardMessageSize = 576;					// The size of an AppleTalk packet
	else if (inStandardMessageSize < sizeof (NSpMessageHeader))
		gStandardMessageSize = sizeof (NSpMessageHeader);		// Our size needs to be at least as big as the NSpMessageHeader
	else
		gStandardMessageSize = inStandardMessageSize;

	gTimeout = (inTimeout > 5000) ? inTimeout : 5000;
	
	gDebugMode = GetDebugMode();
	
	//Ä	Make a new list for all the games we create in this app
	
	gGameList = new NSp_InterruptSafeList;

	if (NULL == gGameList)
		return (kNSpInitializationFailedErr);

	//Ä	Set our globals
//	if (0 != inBufferSize)
//		gBufferSize = inBufferSize;
//	else
//		gBufferSize = kDefaultMemoryPoolSize;
		
	if (0 != inQElements)
		gQElements = inQElements;
	else
		gQElements = kDefaultQElements;
		
	gCreatorType = inGameID;
	gJoinRequestHandler = NULL;
	gJoinRequestContext = NULL;
	gAsyncMessageHandler = NULL;
	gAsyncMessageContext = NULL;
	gCallbackHandler = NULL;
	gCallbackContext = NULL;
	
	gPPInitialized = true;

	return (kNMNoError);
}

#if defined(__MWERKS__)
#pragma mark === Protcol References === 
#endif

//----------------------------------------------------------------------------------------
// NSpProtocol_New
//----------------------------------------------------------------------------------------

NMErr
NSpProtocol_New(const char *inDefinitionString, NSpProtocolReference *outReference)
{
	PConfigRef		theRef;	
	NMErr			status;
	char			customConfig[256];
	char			tempString[256];
	NMType			netModuleType;
	NMUInt32		gameID = 0;
	char			gameName[32]="\0";
	char			*parsePtr;
	char			*tokenPtr;
	char			netModuleTypeString[32];

	
	op_vassert_return(NULL != inDefinitionString, "NSpProtocol_New: inDefinitionString == NULL", kNSpInvalidDefinitionErr);
	
	//Ä	Create the configuration...
	
	strcpy(customConfig, inDefinitionString);

	// Parse the config string for type of protocol...
		
	strcpy(tempString, customConfig);
	
	parsePtr = strstr(tempString,"type=") + 5;
				
	tokenPtr = strtok(parsePtr, "\t");
				
	strcpy(netModuleTypeString, tokenPtr);

	netModuleType = (NMType) atol(netModuleTypeString);

	status = ProtocolCreateConfig(	netModuleType,
		                            gameID,
		                           	gameName,
		                            NULL, 
		                            0, 
		                            customConfig, 
		                            &theRef
		                          );

	if (status == kNMNoError)	
		*outReference = (NSpProtocolReference) theRef;
	else
		*outReference = NULL;

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpProtocol_Dispose
//----------------------------------------------------------------------------------------

void
NSpProtocol_Dispose(NSpProtocolReference inProtocolRef)
{
	NMErr status;
	
	//Ä	Bad!  The established NetSprocket interface doesn't allow for us
	//  to return status!  Perhaps we should provide a new function that
	//	performs the same function but *does* return status.  And we should
	//	encourage developers to start using the new function.  (CRT, Aug 2000)
		
	if (inProtocolRef != NULL)
		status = ProtocolDisposeConfig((PConfigRef) inProtocolRef);
}

//----------------------------------------------------------------------------------------
// NSpProtocol_ExtractDefinitionString
//----------------------------------------------------------------------------------------

NMErr
NSpProtocol_ExtractDefinitionString(NSpProtocolReference inProtocolRef, char *outDefinitionString)
{

	NMErr		status;
	NMSInt16	configStrLen;
	
	if (inProtocolRef != NULL)
	{
		status = ProtocolGetConfigStringLen((PConfigRef) inProtocolRef, &configStrLen);
		
		if (status == kNMNoError)
			status = ProtocolGetConfigString((PConfigRef) inProtocolRef, outDefinitionString, configStrLen);
	}
	else
		status = kNSpInvalidProtocolRefErr;
		
	return (status);
}

#if defined(__MWERKS__)
#pragma mark === Protcol Lists ===
#endif

//----------------------------------------------------------------------------------------
// NSpProtocolList_New
//----------------------------------------------------------------------------------------

NMErr
NSpProtocolList_New(NSpProtocolReference inProtocolRef, NSpProtocolListReference *outList)
{
NSpProtocolListPriv	*theList = NULL;
NMErr			status = kNMNoError;

	theList = new NSpProtocolListPriv();
	if (theList == NULL){
	status = kNSpMemAllocationErr;
		goto error;
	}
	
	if (inProtocolRef)
	{
		NSpProtocolPriv	*theProtocol = (NSpProtocolPriv *) inProtocolRef;
		
		status = theList->Append(theProtocol);
	}
	
	error:
	*outList = (NSpProtocolListReference) theList;

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpProtocolList_Dispose
//----------------------------------------------------------------------------------------

void
NSpProtocolList_Dispose(NSpProtocolListReference inProtocolList)
{
NSpProtocolListPriv	*theList = (NSpProtocolListPriv *) inProtocolList;

	op_vassert(NULL != theList, "NSpProtocolList_Dispose: inProtocolList == NULL");

	if (NULL != theList)
		delete (theList);
}

//----------------------------------------------------------------------------------------
// NSpProtocolList_Append
//----------------------------------------------------------------------------------------

NMErr
NSpProtocolList_Append(NSpProtocolListReference inProtocolList, NSpProtocolReference inProtocolRef)
{
NSpProtocolListPriv	*theList = (NSpProtocolListPriv *) inProtocolList;
NSpProtocolPriv		*theProtocol = (NSpProtocolPriv *)  inProtocolRef;
NMErr			status;
	
	op_vassert_return(NULL != theList, "NSpProtocolList_Append: inProtocolList == NULL", kNSpInvalidProtocolRefErr);
	op_vassert_return(NULL != theProtocol, "NSpProtocolList_Append: inProtocolRef == NULL", kNSpInvalidProtocolRefErr);

	status = theList->Append(theProtocol);
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpProtocolList_Remove
//----------------------------------------------------------------------------------------

NMErr
NSpProtocolList_Remove(NSpProtocolListReference inProtocolList, NSpProtocolReference inProtocolRef)
{
NSpProtocolListPriv	*theList = (NSpProtocolListPriv *) inProtocolList;
NSpProtocolPriv		*theProtocol = (NSpProtocolPriv *)  inProtocolRef;
NMErr			status;
	
	op_vassert_return(NULL != theList, "NSpProtocolList_Remove: inProtocolList == NULL", kNSpInvalidProtocolListErr);
	op_vassert_return(NULL != theProtocol, "NSpProtocolList_Remove: inProtocolRef == NULL", kNSpInvalidProtocolRefErr);

	status = theList->Remove(theProtocol);
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpProtocolList_RemoveIndexed
//----------------------------------------------------------------------------------------

NMErr
NSpProtocolList_RemoveIndexed(NSpProtocolListReference inProtocolList, NMUInt32 inIndex)
{
NSpProtocolListPriv	*theList = (NSpProtocolListPriv *) inProtocolList;
NMErr			status;
	
	op_vassert_return(NULL != theList, "NSpProtocolList_RemoveIndexed: inProtocolList == NULL", kNSpInvalidProtocolListErr);

	status = theList->RemoveIndexedItem(inIndex);
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpProtocolList_GetCount
//----------------------------------------------------------------------------------------

NMUInt32
NSpProtocolList_GetCount(NSpProtocolListReference inProtocolList)
{
NSpProtocolListPriv	*theList = (NSpProtocolListPriv *) inProtocolList;

	op_vassert_return(NULL != theList, "NSpProtocolList_GetCount: inProtocolList == NULL", 0);

	return (theList->GetCount());
}

//----------------------------------------------------------------------------------------
// NSpProtocolList_GetIndexedRef
//----------------------------------------------------------------------------------------

NSpProtocolReference
NSpProtocolList_GetIndexedRef(NSpProtocolListReference inProtocolList, NMUInt32 inIndex)
{
NSpProtocolListPriv	*theList = (NSpProtocolListPriv *) inProtocolList;
NSpProtocolPriv		*theProtocol;
	
	op_vassert_return(NULL != theList, "NSpProtocolList_GetIndexedRef: inProtocolList == NULL", NULL);

	theProtocol = theList->GetIndexedItem(inIndex);

	return ((NSpProtocolReference) theProtocol);
}

//----------------------------------------------------------------------------------------
// NSpProtocol_CreateAppleTalk
//----------------------------------------------------------------------------------------

NSpProtocolReference
NSpProtocol_CreateAppleTalk(NMConstStr31Param inNBPName, NMConstStr31Param inNBPType, NMUInt32 inMaxRTT, NMUInt32 inMinThruput)
{
#ifdef OP_API_NETWORK_OT

	UNUSED_PARAMETER(inMaxRTT);
	UNUSED_PARAMETER(inMinThruput);

	NMErr				status;
	NMType					netModuleType;
	NMUInt32				gameID;
	char					gameName[32];
	PConfigRef				theRef;
	char					customConfig[256];

	op_vassert_return(NULL != inNBPName, "NSpProtocol_CreateAppleTalk: inNBPName == NULL", NULL);

	//Ä	Fix bug 1379460, that allows you to register without specifying an nbp name
	
	if (inNBPName[0] < 1)
		return (NULL);
	
	//Ä	Create the configuration...
	
	netModuleType = (NMType) kATModuleType;
	
	if (NULL == inNBPType)
	{
		gameID = (NMUInt32) gCreatorType;
	}
	else
	{
		gameID = *((NMUInt32 *) &(inNBPType[1])) ;
	}
	
	sprintf(gameName, "%#s", inNBPName);
		
	
	//Ä	Create the configuration...
	
	strcpy(customConfig, "netSprocket=true\0");
	
	status = ProtocolCreateConfig(	netModuleType,
		                            gameID,
		                           	gameName,
		                            NULL, 
		                            0, 
		                            customConfig, 
		                            &theRef
		                          );

	if (status != kNMNoError)
		return (NULL);
				
	return ( (NSpProtocolReference) theRef );
	
#else
	UNUSED_PARAMETER(inNBPName);
	UNUSED_PARAMETER(inNBPType);
	UNUSED_PARAMETER(inMaxRTT);
	UNUSED_PARAMETER(inMinThruput);
	return (NULL);
	
#endif
}

//----------------------------------------------------------------------------------------
// NSpProtocol_CreateIP
//----------------------------------------------------------------------------------------

NSpProtocolReference
NSpProtocol_CreateIP(NMInetPort inPort, NMUInt32 inMaxRTT, NMUInt32 inMinThruput)
{
	UNUSED_PARAMETER(inMaxRTT);
	UNUSED_PARAMETER(inMinThruput);

	NMErr				status;
	NMType					netModuleType;
	NMUInt32				gameID;
	char					customConfig[256];
	PConfigRef				theRef;	
	
	
	//Ä	Create the configuration...
	
	netModuleType = (NMType) kIPModuleType;
	
	gameID = (NMUInt32) gCreatorType;
	
	if (inPort != 0)
	{
		// Stupid OpenPlay Windows bug requires that you include the whole config
		// string even when you only want to override one specific value...
		
#ifdef OP_API_NETWORK_OT
			sprintf(customConfig, "IPport=%u\tnetSprocket=true\0", inPort);
#else
			sprintf(customConfig, "type=%u\tversion=256\tgameID=%u\tgameName=unknown\t"
							"mode=%u\tIPaddr=127.0.0.1\tIPport=%u\tnetSprocket=true", 
							netModuleType, gameID, kUberMode, inPort);
#endif
	}
	else
		strcpy(customConfig, "netSprocket=true");

	
	status = ProtocolCreateConfig(	netModuleType,
		                            gameID,
		                           	NULL,
		                            NULL, 
		                            0, 
		                            customConfig, 
		                            &theRef
		                          );

	
	if (status != kNMNoError)
	{
		return (NULL);
	}
	
	return ( (NSpProtocolReference) theRef );	
		
}

#if defined(__MWERKS__)
#pragma mark  === Human Interface ===
#endif

#if( !defined(OP_PLATFORM_MAC_MACHO) && !defined(OP_PLATFORM_MAC_CFM) && !defined(OP_PLATFORM_WINDOWS) )

//----------------------------------------------------------------------------------------
// NSpDoModalJoinDialog
//----------------------------------------------------------------------------------------
NSpAddressReference
NSpDoModalJoinDialog			(const unsigned char 	inGameType[32],
								 const unsigned char 	inEntityListLabel[256],
								 unsigned char 			ioName[32],
								 unsigned char 			ioPassword[32],
								 NSpEventProcPtr 		inEventProcPtr)
{
	UNUSED_PARAMETER(inGameType)
	UNUSED_PARAMETER(inEntityListLabel)
	UNUSED_PARAMETER(ioName)
	UNUSED_PARAMETER(ioPassword)
	UNUSED_PARAMETER(inEventProcPtr)

	return NULL;
}

//----------------------------------------------------------------------------------------
// NSpDoModalHostDialog
//----------------------------------------------------------------------------------------
NMBoolean
NSpDoModalHostDialog			(NSpProtocolListReference	ioProtocolList,
								 unsigned char 				ioGameName[32],
								 unsigned char 				ioPlayerName[32],
								 unsigned char 				ioPassword[32],
								 NSpEventProcPtr 			inEventProcPtr)
{
	UNUSED_PARAMETER(ioProtocolList)
	UNUSED_PARAMETER(ioGameName)
	UNUSED_PARAMETER(ioPlayerName)
	UNUSED_PARAMETER(ioPassword)
	UNUSED_PARAMETER(inEventProcPtr)

	return false;
}

#endif


#if defined(__MWERKS__)
#pragma mark === Hosting and Joining ===
#endif

//Ä	--------------------	NSpGame_Host

/*
	Creates a new Game Master, meaning it can
	host and route.
*/


//----------------------------------------------------------------------------------------
// NSpGame_Host
//----------------------------------------------------------------------------------------

NMErr
NSpGame_Host(
		NSpGameReference			*outGame,
		NSpProtocolListReference	inProtocolList,
		NMUInt32					inMaxPlayers,
		NMConstStr31Param				inGameName,
		NMConstStr31Param				inPassword,
		NMConstStr31Param				inPlayerName,
		NSpPlayerType				inPlayerType,
		NSpTopology					inTopology,
		NSpFlags					inFlags)
{
	NSpGameMaster		*master = NULL;
	NSpGamePrivate		*theGame = NULL;
	NSpGameInfo			*info;
	NSpProtocolListPriv	*theList = (NSpProtocolListPriv *) inProtocolList;
	UInt32ListMember	*theMember = NULL;
	NSpProtocolPriv		*theProt;
	NMUInt32			count;
	NMType				netModuleType;
	NSpProtocolPriv		*atRef = NULL;
	NSpProtocolPriv		*ipRef = NULL;
	NMErr				status = kNMNoError;

	op_vassert_return(kNSpClientServer == inTopology, "NSpGame_Host: inTopology is not valid", kNSpTopologyNotSupportedErr);
	op_vassert_return(NULL != theList, "NSpGame_Host: inProtocolList == NULL", kNSpInvalidProtocolListErr);

	//Ä	Create the game object
	theGame = new NSpGamePrivate();

	op_vassert(NULL != theGame, "NSpGame_Host: NSpGamePrivate is NULL");
	if (theGame == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}
			

	//Ä	Create the host
	master = new NSpGameMaster(inMaxPlayers, inGameName, inPassword, inTopology, inFlags);
	if (master == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}
			
	//Ä	Attach the master
	theGame->SetMaster(master);
	
	*outGame = (NSpGameReference) theGame;
	
	
	//Ä	We need to keep track of these game objects.  Create a new q element.  Do this now since it might fail
	theMember = new UInt32ListMember( (NMUInt32) theGame);
	if (!theMember){
		status = kNSpMemAllocationErr;
		goto error;
	}
		
	//Ä	If the user has installed a join request handler, add it now
	if (gJoinRequestHandler)
		status = master->InstallJoinRequestHandler(gJoinRequestHandler, gJoinRequestContext);
	if (status)
		goto error;

	//Ä	If the user has installed an async message handler, add it now
	if (gAsyncMessageHandler)
		master->InstallAsyncMessageHandler(gAsyncMessageHandler, gAsyncMessageContext);
	
	//Ä	If the user has installed a callback handler, add it now
	if (gCallbackHandler)
		master->InstallCallbackHandler(gCallbackHandler, gCallbackContext);
	
	//Ä	Find out how many protocols we're going to be advertising on
	count = theList->GetCount();

	//Ä	Confirm that we want to advertise on SOMETHING
	if (count == 0)
	{
		status = kNSpInvalidProtocolListErr;
		goto error;
		//Throw_(kNSpInvalidProtocolListErr);
	}
	
	//Ä	For each protocol, advertise		
	for (NMSInt32 i = 0; i < count; i++)
	{
		theProt = theList->GetIndexedItem(i);
		
		if (theProt == NULL){
			status = kNSpMemAllocationErr;
			goto error;
		}

/* LR -- added function to return type so we don't have to parse config string!

		err = ProtocolGetConfigStringLen((PConfigRef) theProt, &configStrLen);
		ThrowIfOSErr_(err);

		err = ProtocolGetConfigString((PConfigRef) theProt, configString, configStrLen);
		ThrowIfOSErr_(err);
		
		typeStartPtr = strstr(configString,"type=") + 5;
		
		tokenPtr = strtok(typeStartPtr, "\t");
		
		strcpy(netModuleTypeString, tokenPtr);
		
		netModuleType = (NMType) atol(netModuleTypeString);
*/
		netModuleType = ProtocolGetConfigType( (PConfigRef)theProt );
		switch (netModuleType)
		{
			case kATModuleType:
			{
#ifdef OP_API_NETWORK_OT
				status = master->HostAT(theProt);
				//ThrowIfOSErr_(err);
				if (status)
					goto error;
				NMUInt32 prots = master->GetProtocols();
				prots |= kUsingAppleTalk;
				master->SetProtocols(prots);
				atRef = theProt;
#else
			status = kNSpFeatureNotImplementedErr;
			goto error;
			//Throw_(kNSpFeatureNotImplementedErr);

#endif
			}
			break;
			 
			case kIPModuleType:
			{
				status = master->HostIP(theProt);						
				//ThrowIfOSErr_(err);
				if (status)
					goto error;
				NMUInt32 prots = master->GetProtocols();
				prots |= kUsingIP;
				master->SetProtocols(prots);
				ipRef = theProt;	
			}
			break;
			
			default:
				//Throw_(kNSpHostFailedErr);
				status = kNSpHostFailedErr;
				goto error;
				break;
		}
	}
	
	//Ä	We've successfully created the game object.  Store it in our list
	gGameList->Append(theMember);


	//Ä	Now tell them to add the local player (if any)

	if (atRef)
		status = master->AddLocalPlayer(inPlayerName, inPlayerType, atRef);		
	else
		status = master->AddLocalPlayer(inPlayerName, inPlayerType, ipRef);
		
	//ThrowIfOSErr_(err);
	if (status)
		goto error;

	//Ä	Set up our info structure
	info = master->GetGameInfo();		
	info->maxPlayers = inMaxPlayers;
	info->topology = inTopology;

	if (inPlayerName)
		doCopyPStrMax(inGameName, info->name, 31);

	if (inPassword)
		doCopyPStrMax(inPassword, info->password, 31);
		
	error:
	if (status)
	{
		if (theGame)
			delete (theGame);

		if (theMember)
		{
			gGameList->Remove(theMember);
			delete (theMember);
		}

		*outGame = NULL;
		
		if (status > -30360 || status < -30420) //%% why are we doing this???
			status = kNSpHostFailedErr;
	}
	
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame_EnableAdvertising
//----------------------------------------------------------------------------------------

NMErr
NSpGame_EnableAdvertising(NSpGameReference inGame, NSpProtocolReference inProtocol, NMBoolean inEnable)
{
	UNUSED_PARAMETER(inGame);
	UNUSED_PARAMETER(inProtocol);
	UNUSED_PARAMETER(inEnable);

	return (kNSpFeatureNotImplementedErr);

/*
NMErr		err = kNMNoError;
NSpGameMaster	*theGame;	//LR good way to crash w/NULL = ((NSpGamePrivate *)inGame)->GetMaster();
NSpProtocolPriv	*theProt = (NSpProtocolPriv *) inProtocol;
NMBoolean		didOne = false;
NMType ptype;

//Ä	This feature was never truly implemented in NetSprocket to begin with, so until we hook up
//	advertising via the OpenPlay layer, perhaps we should just return a "Feature Not Implemented"
//	error...

	op_vassert_return(NULL != inGame, "NSpGameEnableAdvertising: inGame == NULL", kNSpInvalidGameRefErr);

	ptype = ProtocolGetConfigType( (PConfigRef)theProt );
	if (inEnable)
	{
		if ((NULL == theProt && (theGame->GetProtocols() & kUsingAppleTalk)) || ptype == kATModuleType)
		{
			err = theGame->HostAT(NULL);
			didOne = (err == kNMNoError);				
		}

		if ((NULL == theProt && (theGame->GetProtocols() & kUsingIP)) || ptype == kIPModuleType)
		{
			err = theGame->HostIP(NULL);
			didOne = (err == kNMNoError);				
		}
		else if (NULL != theProt)
		{
			err = kNSpInvalidProtocolRefErr;
		}
	}
	else
	{
		if ((theProt == NULL && (theGame->GetProtocols() & kUsingAppleTalk)) || ptype == kATModuleType)
		{
			err = theGame->UnHostAT();
			didOne = (err == kNMNoError);				
		}
		if ((theProt == NULL && (theGame->GetProtocols() & kUsingIP)) || ptype == kIPModuleType)
		{
			err = theGame->UnHostIP();
			didOne = (err == kNMNoError);				
		}
		else if (theProt != NULL)
		{
			err = kNSpInvalidProtocolRefErr;
		}
			
	}
	
	if (err == kNSpNotAdvertisingErr || didOne)
		err = kNMNoError;
	
	return (err);
*/
}

//----------------------------------------------------------------------------------------
// NSpGame_Join
//----------------------------------------------------------------------------------------

NMErr
NSpGame_Join(
		NSpGameReference	*outGame,
		NSpAddressReference	inAddress,
		NMConstStr31Param		inName, 
		NMConstStr31Param		inPassword,
		NSpPlayerType		inType, 
		void				*inCustomData,
		NMUInt32			inCustomDataLen,
		NSpFlags			inFlags)
{
	UInt32ListMember	*theMember;
	NSpGamePrivate		*theGame = NULL;
	NSpGameSlave		*slave = NULL;
	NSpGameInfo			*info;
	NMErr				status = kNMNoError;

	//Ä	Create the private game object
	theGame = new NSpGamePrivate();
	if (theGame == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}
	
	//Ä	Create a new game object with only null initialization
	slave = new NSpGameSlave(inFlags);
	if (slave == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}
	
	theGame->SetSlave(slave);
	
	*outGame = (NSpGameReference) theGame;
	
	if (gAsyncMessageHandler)
		slave->InstallAsyncMessageHandler(gAsyncMessageHandler, gAsyncMessageContext);

	if (gCallbackHandler)
		slave->InstallCallbackHandler(gCallbackHandler, gCallbackContext);

	//Ä	Insert the pointer to the game into our list
	theMember = new UInt32ListMember( (NMUInt32) theGame);
	if (theMember == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	
	//Ä	Tell the game object to join the specified game
	status = slave->Join(inName, inPassword, inType, inCustomData, inCustomDataLen, inAddress);
	//ThrowIfOSErr_(err);
	if (status)
		goto error;
		
	gGameList->Append(theMember);

	info = slave->GetGameInfo();
	
	//*	Set up our info structure
	info->maxPlayers = 0;
	info->topology = kNSpClientServer;		//Ä HACK!!!!

	//*	Get password, game name sent in JoinAccepted message (2.2 or later only!)
	if (inPassword)
		doCopyPStrMax(inPassword, info->password, 31);
	
	error:
	if (status)
	{
		if (theGame)
			delete (theGame);
			
		*outGame = NULL;
	}
	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGame_Dispose
//----------------------------------------------------------------------------------------

NMErr
NSpGame_Dispose(NSpGameReference inGame, NSpFlags inFlags)
{
	NSp_InterruptSafeListIterator	iter(*gGameList);
	NSp_InterruptSafeListMember	*item;
	UInt32ListMember			*member;
	NMErr						err = kNMNoError;
	NSpGamePrivate				*theGame = (NSpGamePrivate *)inGame;
		
	op_vassert_return(NULL != inGame, "NSpGame_Dispose: inGame == NULL", kNSpInvalidGameRefErr);

	while (iter.Next(&item))
	{
		member = (UInt32ListMember *) item;

		if (member->GetValue() == (NMUInt32) theGame)
		{
			err = theGame->PrepareForDeletion(inFlags);
			
			if (err == kNMNoError)
			{
				gGameList->Remove(item);
				delete theGame;
				delete member;
			}
			//ECF- is there a reason we'd need to clump all returned errors together?
			//else
			//	err = kNSpInvalidGameRefErr;
				
			break;
		}
	}
	
	return (err);	
}

//----------------------------------------------------------------------------------------
// NSpGame_GetInfo
//----------------------------------------------------------------------------------------

NMErr
NSpGame_GetInfo(NSpGameReference inGame, NSpGameInfo  *ioInfo)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	NSpGame			*game;
	
	op_vassert_return(NULL != inGame, "NSpGame_GetInfo: inGame == NULL", kNSpInvalidGameRefErr);

	game = theGame->GetGameObject();

	if (NULL == game)
		return (kNSpInvalidGameRefErr);
		
	*ioInfo = *game->GetGameInfo();
	
	return (kNMNoError);
}

#if defined(__MWERKS__)
#pragma mark === Messaging ===
#endif

//----------------------------------------------------------------------------------------
// NSpMessage_Send
//----------------------------------------------------------------------------------------

NMErr
NSpMessage_Send(NSpGameReference inGame, NSpMessageHeader *inMessage, NSpFlags inFlags)
{
NMErr		err = kNMNoError;
NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
NSpGame			*game;
	
	op_vassert_return(NULL != inGame, "NSpMessage_Send: inGame == NULL", kNSpInvalidGameRefErr);

	game = theGame->GetGameObject();

	if (game == NULL)
		return (kNSpInvalidGameRefErr);
		
// dair, allow arbitrary sized messages as per NSp
#if 0
	if (inMessage->messageLen > 102400)
		return (kNSpMessageTooBigErr);
#endif

	err = game->SendUserMessage(inMessage, inFlags);


	//keep our memory reserve full
#ifdef OP_API_NETWORK_OT
		UpkeepOTMemoryReserve();
#endif
	
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpMessage_SendTo
//----------------------------------------------------------------------------------------

NMErr
NSpMessage_SendTo(
		NSpGameReference	inGame,
		NSpPlayerID			inTo,
		NMSInt32			inWhat, 
		void*				inData,
		NMUInt32			inDataLen, 
		NSpFlags 			inFlags)
{
NMErr		err = kNMNoError;
NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
NSpGame			*game;
	
	op_vassert_return(NULL != inGame, "NSpMessage_SendTo: inGame == NULL", kNSpInvalidGameRefErr);

	game = theGame->GetGameObject();

	if (NULL == game)
		return (kNSpInvalidGameRefErr);

// dair, allow arbitrary sized messages as per NSp
#if 0
	if (inDataLen > 102400)
		return (kNSpMessageTooBigErr);
#endif

	err = game->SendTo(inTo, inWhat, inData, inDataLen, inFlags);

	return (err);
}

//----------------------------------------------------------------------------------------
// NSpMessage_Get
//----------------------------------------------------------------------------------------

NSpMessageHeader *
NSpMessage_Get(NSpGameReference inGame)
{
	NMBoolean			gotEvent;
	NSpGamePrivate		*theGame = (NSpGamePrivate *)inGame;
	NSpMessageHeader	*theMessage = NULL;
	NMErr				status = kNMNoError;
	
	op_vassert_return(NULL != inGame, "NSpMessage_Get: inGame == NULL", NULL);

	//Try_
	{
		gotEvent = theGame->NSpMessage_Get(&theMessage);

		if (false == gotEvent)
			theMessage = NULL;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		(void) code;
		return (NULL);
	}	
	
	//keep our memory reserve full - i hope this function isnt called at interrupt time...
#ifdef OP_API_NETWORK_OT
		UpkeepOTMemoryReserve();
#endif
	
	return (theMessage);
}

//----------------------------------------------------------------------------------------
// NSpMessage_GetBacklog
//----------------------------------------------------------------------------------------

NMUInt32
NSpMessage_GetBacklog( NSpGameReference inGame )
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	NSpGameMaster *game = theGame->GetMaster();

	return game->GetBacklog();
}

//----------------------------------------------------------------------------------------
// NSpMessage_Release
//----------------------------------------------------------------------------------------

void
NSpMessage_Release(NSpGameReference inGame, NSpMessageHeader *inMessage)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	NMErr status = kNMNoError;
	op_vassert_justreturn(NULL != inGame, "NSpMessage_Release: inGame == NULL");
	op_vassert_justreturn(NULL != inMessage, "NSpMessage_Release: inMessage == NULL");
	
	//Try_
	{	
		(theGame->GetGameObject())->FreeNetMessage(inMessage);
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		(void) code;
	}
}

#if defined(__MWERKS__)
#pragma mark === Player Information ===
#endif

//----------------------------------------------------------------------------------------
// NSpPlayer_GetMyID
//----------------------------------------------------------------------------------------

NSpPlayerID
NSpPlayer_GetMyID(NSpGameReference inGame)
{
NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;

	op_vassert_return(NULL != inGame, "NSpPlayer_GetMyID: inGame == NULL", 0);

	return ((theGame->GetGameObject())->NSpPlayer_GetMyID());
}

//----------------------------------------------------------------------------------------
// NSpPlayer_GetInfo
//----------------------------------------------------------------------------------------

NMErr
NSpPlayer_GetInfo(NSpGameReference inGame, NSpPlayerID inPlayerID, NSpPlayerInfoPtr *outInfo)
{
	NMErr 			err = kNMNoError;
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	op_vassert_return(NULL != inGame, "NSpPlayer_GetInfo: inGame == NULL", kNSpInvalidGameRefErr);

	err = (theGame->GetGameObject())->NSpPlayer_GetInfo(inPlayerID, outInfo);
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpPlayer_ReleaseInfo
//----------------------------------------------------------------------------------------

void
NSpPlayer_ReleaseInfo(NSpGameReference inGame, NSpPlayerInfoPtr inInfo)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	NMErr status = kNMNoError;
	op_vassert_justreturn(NULL != inGame, "NSpPlayer_ReleaseInfo: inGame == NULL");
	op_vassert_justreturn(NULL != inInfo, "NSpPlayer_ReleaseInfo:  inInfo == NULL");

	(theGame->GetGameObject())->NSpPlayer_ReleaseInfo(inInfo);
}

//----------------------------------------------------------------------------------------
// NSpPlayer_GetEnumeration
//----------------------------------------------------------------------------------------

NMErr
NSpPlayer_GetEnumeration(NSpGameReference inGame, NSpPlayerEnumerationPtr *outPlayers)
{
	NMErr		 	err = kNMNoError;
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	
	op_vassert_return(NULL != inGame, "NSpPlayer_GetEnumeration: inGame == NULL", kNSpInvalidGameRefErr);

	err = (theGame->GetGameObject())->NSpPlayer_GetEnumeration(outPlayers);
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpPlayer_ReleaseEnumeration
//----------------------------------------------------------------------------------------

void
NSpPlayer_ReleaseEnumeration(NSpGameReference inGame, NSpPlayerEnumerationPtr inPlayers)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	NMErr status = kNMNoError;
	op_vassert_justreturn(NULL != inGame, "NSpPlayer_ReleaseEnumeration: inGame == NULL");
	op_vassert_justreturn(NULL != inPlayers, "NSpPlayer_ReleaseEnumeration: inPlayers == NULL");
		
	//Try_
	{
		(theGame->GetGameObject())->NSpPlayer_ReleaseEnumeration(inPlayers);
	}
	//Catch_(err)
	error:
	if (status)
	{
		NMErr code = status;
		//(void) err;
	}
}

//----------------------------------------------------------------------------------------
// NSpPlayer_GetRoundTripTime
//----------------------------------------------------------------------------------------

NMSInt32
NSpPlayer_GetRoundTripTime(NSpGameReference inGame, NSpPlayerID inPlayer)
{
	UNUSED_PARAMETER(inPlayer);

//NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	
	op_vassert_return(NULL != inGame, "NSpPlayer_GetRoundTripTime: inGame == NULL", kNSpInvalidGameRefErr);

	return (kNSpFeatureNotImplementedErr);
	
//	return theGame->GetRTT(inPlayer);
}

//----------------------------------------------------------------------------------------
// NSpPlayer_GetThruput
//----------------------------------------------------------------------------------------

NMSInt32
NSpPlayer_GetThruput(NSpGameReference inGame, NSpPlayerID inPlayer)
{
	UNUSED_PARAMETER(inPlayer);

//NMErr	 	err = kNMNoError;
//NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	
	op_vassert_return(NULL != inGame, "NSpPlayer_GetThruput: inGame == NULL", kNSpInvalidGameRefErr);

	return (kNSpFeatureNotImplementedErr);

//	return theGame->GetThruput(inPlayer);
}

//----------------------------------------------------------------------------------------
// NSpPlayer_ChangeType
//----------------------------------------------------------------------------------------

NMErr
NSpPlayer_ChangeType(
		NSpGameReference	inGame,
		NSpPlayerID 		inPlayerID,
		NSpPlayerType 		inNewType)
{
NSpGamePrivate	*theGame = (NSpGamePrivate *) inGame;
NSpGameMaster	*server;

	op_vassert_return(NULL != inGame, "NSpPlayer_ChangeType: inGame == NULL", kNSpInvalidGameRefErr);

	//Ä	Make sure that this is the server game reference
	server = theGame->GetMaster();
	
	if (NULL == server)
		return (kNSpInvalidGameRefErr);

	//Ä	Ok, we've got a server game object, ask it to apply the change
	return (server->ChangePlayerType(inPlayerID, inNewType));
}

//----------------------------------------------------------------------------------------
// NSpPlayer_Remove
//----------------------------------------------------------------------------------------

NMErr
NSpPlayer_Remove(NSpGameReference inGame, NSpPlayerID inPlayerID)
{
NSpGamePrivate	*theGame = (NSpGamePrivate *) inGame;
NSpGameMaster	*server;

	op_vassert_return(NULL != inGame, "NSpPlayer_Remove: inGame == NULL", kNSpInvalidGameRefErr);

	//Ä	Make sure that this is the server game reference
	server = theGame->GetMaster();
	
	if (NULL == server)
		return (kNSpInvalidGameRefErr);

	//Ä	Ok, we've got a server game object, ask it to remove the player
	return (server->ForceRemovePlayer(inPlayerID));
}

#if defined(__MWERKS__)
#pragma mark === Group Management ===
#endif

//----------------------------------------------------------------------------------------
// NSpGroup_New
//----------------------------------------------------------------------------------------

NMErr
NSpGroup_New(NSpGameReference inGame, NSpGroupID *outGroupID)
{
	NMErr		 	err = kNMNoError;
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	
	op_vassert_return(NULL != inGame, "NSpGroup_New: inGame == NULL", kNSpInvalidGameRefErr);

	err = (theGame->GetGameObject())->NSpGroup_Create(outGroupID);
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpGroup_Dispose
//----------------------------------------------------------------------------------------

NMErr
NSpGroup_Dispose(NSpGameReference inGame, NSpGroupID inGroupID)
{
	NMErr		 	err = kNMNoError;
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	
	op_vassert_return(NULL != inGame, "NSpGroup_Dispose: inGame == NULL", kNSpInvalidGameRefErr);

	err = (theGame->GetGameObject())->NSpGroup_Delete(inGroupID);
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpGroup_AddPlayer
//----------------------------------------------------------------------------------------

NMErr
NSpGroup_AddPlayer(NSpGameReference inGame, NSpGroupID inGroupID, NSpPlayerID inPlayerID)
{
	NMErr		 	err = kNMNoError;
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;

	op_vassert_return(NULL != inGame, "NSpGroup_AddPlayer: inGame == NULL", kNSpInvalidGameRefErr);

	err = (theGame->GetGameObject())->NSpGroup_AddPlayer(inGroupID, inPlayerID);
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpGroup_RemovePlayer
//----------------------------------------------------------------------------------------

NMErr
NSpGroup_RemovePlayer(NSpGameReference inGame, NSpGroupID inGroupID, NSpPlayerID inPlayerID)
{
	NMErr		 	err = kNMNoError;
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	
	op_vassert_return(NULL != inGame, "NSpGroup_RemovePlayer: inGame == NULL", kNSpInvalidGameRefErr);

	err = (theGame->GetGameObject())->NSpGroup_RemovePlayer(inGroupID, inPlayerID);
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpGroup_GetInfo
//----------------------------------------------------------------------------------------

NMErr
NSpGroup_GetInfo(NSpGameReference inGame, NSpGroupID inGroupID, NSpGroupInfoPtr *outInfo)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	NMErr err = kNMNoError;
	
	op_vassert_return(NULL != inGame, "NSpGroup_GetInfo: inGame == NULL", kNSpInvalidGameRefErr);

	err = (theGame->GetGameObject())->NSpGroup_GetInfo(inGroupID, outInfo);
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpGroup_ReleaseInfo
//----------------------------------------------------------------------------------------

void
NSpGroup_ReleaseInfo(NSpGameReference inGame, NSpGroupInfoPtr inInfo)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	NMErr status = kNMNoError;
	op_vassert_justreturn(NULL != inGame, "NSpGroup_ReleaseInfo: inGame == NULL");
	op_vassert_justreturn(NULL != inInfo, "NSpGroup_ReleaseInfo: inInfo == NULL");

	(theGame->GetGameObject())->NSpGroup_ReleaseInfo(inInfo);
}

//----------------------------------------------------------------------------------------
// NSpGroup_GetEnumeration
//----------------------------------------------------------------------------------------

NMErr
NSpGroup_GetEnumeration(NSpGameReference inGame, NSpGroupEnumerationPtr *outGroups)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	NMErr err = kNMNoError;
	
	op_vassert_return(NULL != inGame, "NSpGroup_GetEnumeration: inGame == NULL", kNSpInvalidGameRefErr);

	err = (theGame->GetGameObject())->NSpGroup_GetEnumeration(outGroups);
	return (err);
}

//----------------------------------------------------------------------------------------
// NSpGroup_ReleaseEnumeration
//----------------------------------------------------------------------------------------

void
NSpGroup_ReleaseEnumeration(NSpGameReference inGame, NSpGroupEnumerationPtr inGroups)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	NMErr status = kNMNoError;
	op_vassert_justreturn(NULL != inGame, "NSpGroup_ReleaseEnumeration: inGame == NULL");
	op_vassert_justreturn(NULL != inGroups, "NSpGroup_ReleaseEnumeration: inGroups == NULL");

	(theGame->GetGameObject())->NSpGroup_ReleaseEnumeration(inGroups);
}

#if defined(__MWERKS__)
#pragma mark === Utilities ===
#endif

//----------------------------------------------------------------------------------------
// NSpGetVersion
//----------------------------------------------------------------------------------------

NMNumVersion
NSpGetVersion(void)
{

#ifdef OP_API_NETWORK_OT
	NMSInt16	refNum;
	Handle		versResource = NULL;

	refNum = FSpOpenResFile(&gFSSpec, fsRdPerm);
	
	if (refNum > 0)
	{
		versResource = Get1Resource ('vers', 1);

		if (versResource)
			gVersion = (**((NMNumVersion**) versResource));
	
		CloseResFile (refNum);
	}
	else
#endif
	{

	//Ä	Note that we do nothing fancy to obtain the version if this is a non-Mac platform.
	//	Perhaps there *is* something we can do, but that is left for the OpenSource community...

		gVersion.majorRev = __NSpVersionMajor__;
		gVersion.minorAndBugRev = (__NSpVersionMinor__ << 4) | __NSpVersionBugFix__;
		gVersion.stage = __NSpReleaseStage__;
		gVersion.nonRelRev = __NSpNonRelRev__;
	}
	
	return (gVersion);
}

//----------------------------------------------------------------------------------------
// NSpSetConnectTimeout
//----------------------------------------------------------------------------------------

void
NSpSetConnectTimeout(NMUInt32 inSeconds)
{
	gEndpointConnectTimeout = inSeconds;
}

//----------------------------------------------------------------------------------------
// NSpClearMessageHeader
//----------------------------------------------------------------------------------------

void
NSpClearMessageHeader(NSpMessageHeader *inMessage)
{
	inMessage->version = 0;
	inMessage->what = 0;
	inMessage->from = 0;
	inMessage->to = 0;
	inMessage->id = 0;
	inMessage->when = 0;
	inMessage->messageLen = 0;
}

//----------------------------------------------------------------------------------------
// NSpGetCurrentTimeStamp
//----------------------------------------------------------------------------------------

NMUInt32
NSpGetCurrentTimeStamp(NSpGameReference inGame)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *)inGame;
	
	op_vassert_return(NULL != inGame, "NSpGetCurrentTimeStamp: inGame == NULL", (NMUInt32)kNSpInvalidGameRefErr);

	return ((theGame->GetGameObject())->GetCurrentTimeStamp());
}

//----------------------------------------------------------------------------------------
// NSpPlayer_FreeAddress
//----------------------------------------------------------------------------------------

NMErr
NSpPlayer_FreeAddress(
	NSpGameReference	inGame,
	void				**outAddress)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *) inGame;
	NSpGameMaster	*server;

	//Ä	Make sure that this is the server game reference
	server = theGame->GetMaster();
	
	if (NULL == server)
		return (kNSpInvalidGameRefErr);

	//Ä	Ok, our setup is kosher.  Jump into the server
	//Ä	game object and free the address
	return (server->FreePlayerAddress((void **)outAddress));
}

//----------------------------------------------------------------------------------------
// NSpPlayer_GetIPAddress
//----------------------------------------------------------------------------------------

NMErr
NSpPlayer_GetIPAddress(
	NSpGameReference	inGame,
	NSpPlayerID			inPlayerID,
	char				**outAddress)
{
	NSpGamePrivate	*theGame = (NSpGamePrivate *) inGame;
	NSpGameMaster	*server;

	//Ä	Make sure that this is the server game reference
	server = theGame->GetMaster();
	
	if (NULL == server)
		return (kNSpInvalidGameRefErr);

	//Ä	Ok, our setup is kosher.  Jump into the server
	//Ä	game object and get the address
	return (server->GetPlayerIPAddress(inPlayerID, outAddress));
}


#ifdef OP_API_NETWORK_OT

//----------------------------------------------------------------------------------------
// NSpPlayer_GetOTAddress
//----------------------------------------------------------------------------------------

NMErr
NSpPlayer_GetOTAddress(
	NSpGameReference	inGame,
	NSpPlayerID			inPlayerID,
	OTAddress			**outAddress)
{
NSpGamePrivate	*theGame = (NSpGamePrivate *) inGame;
NSpGameMaster	*server;

	op_vassert_return(NULL != inGame, "NSpPlayer_GetOTAddress: inGame == NULL", kNSpInvalidGameRefErr);
	op_vassert_return(NULL != outAddress, "NSpPlayer_GetOTAddress: outAddress == NULL", paramErr);

	//Ä	Make sure that this is the server game reference
	server = theGame->GetMaster();
	
	if (NULL == server)
		return (kNSpInvalidGameRefErr);

	//Ä	Ok, our setup is kosher.  Jump into the server
	//Ä	game object and get the address
	return (server->GetPlayerAddress(inPlayerID, outAddress));
}

//----------------------------------------------------------------------------------------
// NSpConvertOTAddrToAddressReference
//----------------------------------------------------------------------------------------

NSpAddressReference
NSpConvertOTAddrToAddressReference(OTAddress *inAddress)
{	
	
		PConfigRef theConfigRef;
				
		//Ä	Determine whether the OTAddress is an AppleTalk address or an IP address and
		//	call the appropriate function...

		if (AF_ATALK_DDPNBP == inAddress->fAddressType)
		{
			NBPEntity		theNBPEntity;
			char			theName[64];
			char			theType[64];
			char			theZone[64];
			
			
			OTSetNBPEntityFromAddress(&theNBPEntity, (NMUInt8 *)  ( (DDPNBPAddress *)inAddress) -> fNBPNameBuffer, 105);
	
			OTExtractNBPZone(&theNBPEntity, theZone);
			OTExtractNBPType(&theNBPEntity, theType);
			OTExtractNBPName(&theNBPEntity, theName);
			
			theConfigRef = (PConfigRef) NSpCreateATlkAddressReference(theName, theType, theZone);
		}
		else if (AF_INET == inAddress->fAddressType)
		{
			InetAddress				*theInetAddressPtr;
			NMInetPort				theInetPort;
			InetHost				theInetHost;
			char					theIPAddress[16];
			char					theIPPort[16];
			unsigned char			*byte;

			//	Cast the OTAddress as an InetAddress and then pull out the parts you need...
	
			theInetAddressPtr = (InetAddress *) inAddress;
	
			theInetPort = theInetAddressPtr->fPort;

			theInetHost = theInetAddressPtr->fHost;

			byte = (unsigned char *) &theInetHost;
			
			sprintf(theIPAddress,"%u.%u.%u.%u\0", byte[0], byte[1], byte[2], byte[3]);

			sprintf(theIPPort,"%d\0", theInetPort);
			
			theConfigRef = (PConfigRef) NSpCreateIPAddressReference(theIPAddress, theIPPort);
		}
		else
		{
			theConfigRef = NULL;
		}
		
		return (NSpAddressReference) theConfigRef;
		
}

//----------------------------------------------------------------------------------------
// NSpConvertAddressReferenceToOTAddr
//----------------------------------------------------------------------------------------

OTAddress *
NSpConvertAddressReferenceToOTAddr(NSpAddressReference inAddress)
{
	OTAddress		*outOTAddress;
	InetAddress	*	anInetAddress;
	char			configString[1024];
	char			tempString[256];
	char			*parsePtr;
	char			*tokenPtr;
/*
	NMErr			status;
	NMSInt16		configStrLen;
	char			netModuleTypeString[32];
*/
	NMType			netModuleType;
	char			addrString[256] = "\0";
	char			ipAddr[32] = "\0";
	char			portString[32] = "\0";
	char			name[32] = "\0";
	char			type[32] = "\0";
	char			zone[32] = "\0";
	NMErr		 	err = kNMNoError;
	NMErr			status = kNMNoError;

	// Get the config string for the protocol...

/*	LR -- added function to return type w/o having to do all this string parsing!

	status = ProtocolGetConfigStringLen((PConfigRef) inAddress, &configStrLen);
	ThrowIfOSErr_(status);

	status = ProtocolGetConfigString((PConfigRef) inAddress, configString, configStrLen);
	ThrowIfOSErr_(status);
	
	// Parse the config string for type of protocol...
	
	strcpy(tempString, configString);
	
	parsePtr = strstr(tempString,"type=") + 5;
			
	tokenPtr = strtok(parsePtr, "\t");
			
	strcpy(netModuleTypeString, tokenPtr);
			
	netModuleType = (NMType) atol(netModuleTypeString);
*/
	netModuleType = ProtocolGetConfigType( (PConfigRef)inAddress );
	switch (netModuleType)
	{
		case kATModuleType:
		{
			// Parse the config string for name, type, and zone of AppleTalk configuration...
			
			strcpy(tempString, configString);
			
			parsePtr = strstr(tempString,"ATName=") + 7;
			
			tokenPtr = strtok(parsePtr, "\t");
			
			strcpy(name, tokenPtr);

			strcpy(tempString, configString);

			parsePtr = strstr(tempString,"ATType=") + 7;
			
			tokenPtr = strtok(parsePtr, "\t");
			
			strcpy(type, tokenPtr);

			strcpy(tempString, configString);

			parsePtr = strstr(tempString,"ATZone=") + 7;
			
			tokenPtr = strtok(parsePtr, "\t");
			
			strcpy(zone, tokenPtr);

			sprintf(addrString, "%s:%s@%s", name, type, zone);
			outOTAddress = (OTAddress *)OTUtils::MakeDDPNBPAddressFromString(addrString);
		}
		break;
				 
		case kIPModuleType:
		{
			// Parse the config string for IP address and port of TCP/IP configuration...

			strcpy(tempString, configString);

			parsePtr = strstr(tempString,"IPaddr=") + 7;
			
			tokenPtr = strtok(parsePtr, "\t");
			
			strcpy(ipAddr, tokenPtr);

			strcpy(tempString, configString);

			parsePtr = strstr(tempString,"IPport=") + 7;
			
			tokenPtr = strtok(parsePtr, "\t");
			
			strcpy(portString, tokenPtr);

			sprintf(addrString, "%s:%s", ipAddr, portString);
			anInetAddress = new InetAddress;	//weird that one new's it for us and the other doesn't, but that's the case...
			if (anInetAddress == NULL){
				status = kNSpMemAllocationErr;
				goto error;
			}
			err = OTUtils::MakeInetAddressFromString(addrString,anInetAddress);
			outOTAddress = (OTAddress *)anInetAddress;
		}
		break;
				
		default:
			//Throw_(NULL);
			status = -1;
			goto error;
		break;
	}

	error:
	if (status)
	{
		outOTAddress = NULL;
	}
	
	return outOTAddress;
}

//----------------------------------------------------------------------------------------
// NSpReleaseOTAddress
//----------------------------------------------------------------------------------------

void
NSpReleaseOTAddress(OTAddress *inAddress)
{	
	if (inAddress != NULL)
	{
		if (AF_ATALK_DDPNBP == inAddress->fAddressType)
		{
			DDPNBPAddress *nbpAddressPtr = (DDPNBPAddress *) inAddress;
			delete nbpAddressPtr;
		}
		else if (AF_INET == inAddress->fAddressType)
		{
			InetAddress *inetAddressPtr = (InetAddress *) inAddress;
			delete inetAddressPtr;
		}
		
		inAddress = NULL;
	}

}
#endif	//	OP_API_NETWORK_OT

//----------------------------------------------------------------------------------------
// NSpCreateATlkAddressReference
//----------------------------------------------------------------------------------------

NSpAddressReference NSpCreateATlkAddressReference(char *inName, char *inType, char *inZone)
{

#ifdef OP_API_NETWORK_OT
	
	NMErr			status;
	PConfigRef		outConfigRef = NULL;	
	NMType			netModuleType;
	char			customConfig[64];
	
	
	//Ä	Create the configuration...
	
	netModuleType = (NMType) kATModuleType;
	
	
	if (inZone != NULL)
		sprintf(customConfig, "ATZone=%s\tnetSprocket=true\0", inZone);
	else
		strcpy(customConfig, "netSprocket=true\0");
	
	status = ProtocolCreateConfig(	netModuleType,
		                            *( (NMUInt32 *) inType),
		                           	inName,
		                            NULL, 
		                            0, 
		                            customConfig, 
		                            &outConfigRef
		                          );	
	if (status != kNMNoError)
		return (NULL);
	
		
	return (NSpAddressReference) outConfigRef;
	
	#else
		UNUSED_PARAMETER(inName);
		UNUSED_PARAMETER(inType);
		UNUSED_PARAMETER(inZone);
	
		return (NULL); 
		
	#endif
}

//----------------------------------------------------------------------------------------
// NSpCreateIPAddressReference
//----------------------------------------------------------------------------------------

// dair, made input parameters const
NSpAddressReference NSpCreateIPAddressReference(const char *inIPAddress, const char *inIPPort)
{	
	NMErr		status;
	NMType		netModuleType;
	NMUInt32	gameID;
	char		customConfig[256];
	//char		configString[256]="\0";
	PConfigRef	outConfigRef = NULL;
	

	//Ä	Create the configuration...
	
	netModuleType = (NMType) kIPModuleType;
	
	gameID = (NMUInt32) gCreatorType;
			
	sprintf(customConfig, "type=%u\tversion=256\tgameID=%u\tgameName=unknown\t"
							"mode=%u\tIPaddr=%s\tIPport=%s\tnetSprocket=true", 
							netModuleType, gameID, kUberMode,
							inIPAddress, inIPPort);

	status = ProtocolCreateConfig(	netModuleType,
		                            gameID,
		                           	NULL,
		                            NULL, 
		                            0, 
		                            customConfig, 
		                            &outConfigRef
		                          );
		
	if (status != kNMNoError)
		return (NULL);
		

	return (NSpAddressReference) outConfigRef;
		
}

//----------------------------------------------------------------------------------------
// NSpReleaseAddressReference
//----------------------------------------------------------------------------------------

void
NSpReleaseAddressReference(NSpAddressReference inAddress)
{
	NMErr status;
	
	if (inAddress != NULL)
		status = ProtocolDisposeConfig((PConfigRef)inAddress);

}

#if defined(__MWERKS__)
#pragma mark === Async stuff ===
#endif

//----------------------------------------------------------------------------------------
// NSpInstallCallbackHandler
//----------------------------------------------------------------------------------------

NMErr
NSpInstallCallbackHandler(NSpCallbackProcPtr inHandler, void *inContext)
{
//	op_vassert_return(NULL != inHandler, "NSpInstallCallbackHandler: inHandler == NULL", paramErr);

	gCallbackHandler = inHandler;
	gCallbackContext = inContext;

	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// NSpInstallJoinRequestHandler
//----------------------------------------------------------------------------------------

NMErr
NSpInstallJoinRequestHandler(NSpJoinRequestHandlerProcPtr inHandler, void *inContext)
{
NMErr	err = kNMNoError;
	
//	op_vassert_return(NULL != inHandler, "NSpInstallJoinRequestHandler: inHandler == NULL", paramErr);

	gJoinRequestHandler = inHandler;
	gJoinRequestContext = inContext;

	return (err);
}

//----------------------------------------------------------------------------------------
// NSpInstallAsyncMessageHandler
//----------------------------------------------------------------------------------------

NMErr
NSpInstallAsyncMessageHandler(NSpMessageHandlerProcPtr inHandler, void *inContext)
{
NMErr	err = kNMNoError;
	
//	op_vassert_return(NULL != inHandler, "NSpInstallAsyncMessageHandler: inHandler == NULL", paramErr);

	gAsyncMessageHandler = inHandler;
	gAsyncMessageContext = inContext;

	return (err);
}

//----------------------------------------------------------------------------------------
// GetQState
//----------------------------------------------------------------------------------------

void
GetQState(NSpGameReference inGame, NMUInt32 *outFreeQ, NMUInt32 *outCookieQ, NMUInt32 *outMessageQ)
{	
	NSpGamePrivate *theGame = (NSpGamePrivate *)inGame;
	(theGame->GetGameObject())->GetQState(outFreeQ, outCookieQ, outMessageQ);
}

//----------------------------------------------------------------------------------------
// SetDebugMode
//----------------------------------------------------------------------------------------

void
SetDebugMode(NMBoolean inDebugMode)
{
	gDebugMode = inDebugMode;
}

//----------------------------------------------------------------------------------------
// GetDebugMode
//----------------------------------------------------------------------------------------

NMBoolean
GetDebugMode( void )
{
#if DEBUG
	return true;
/*FSSpec	theSpec;
NMErr	theError;
	
	theError = FSMakeFSSpec( 0, 0, (ConstStr255Param)"\pNSpSetDebugMode", &theSpec );

	if (kNMNoError == theError)
		return true;

	return (false);
		
#else
	return (false);*/
#endif
	return false;
}
