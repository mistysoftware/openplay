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

#include "NetModule.h"
#include "configuration.h"
#include "configfields.h"
#include "EndpointDisposer.h"
#include "EndpointHander.h"
#include "OTATEndpoint.h"
#include "OTUtils.h"
#include "SetupLibraryState.h"
#include "OTEnumerators.h"
#include "Exceptions.h"
#include "String_Utils.h"
#include <sioux.h>

//	------------------------------	Private Definitions

//defined in NetModule.h now
//#define kDefaultNetSprocketMode false;

//	------------------------------	Private Types

enum
{
	kDefaultATTimeout	= 5000
};

enum
{
	kInfoStrings		= 2000,
	kDITLID				= 2000
};

enum
{
	kATModuleNameIndex	= 1,
	kCopyrightIndex		= 2
};
		
enum
{
	kZoneListName		= 1,
	kZoneList
};

//	------------------------------	Private Variables

static const char *		kATNBPName = "ATName";
static const char *		kATNBPType = "ATType";
static const char *		kATNBPZone = "ATZone";
static const char *		kATModuleName = "AppleTalk";
static const StringPtr	kDefaultZone = "\p*";
static const char *		kATModuleCopyright = "1997-2004 Apple Computer, Inc.";
static const Rect		kFrameSize = {0, 0, 210, 200};

static OTNotifyUPP		gATNotifierUPP = NULL;
static OTNotifyUPP		gATZLNotifierUPP = NULL;
static NMBoolean 		gzlDone = false;

//	------------------------------	Private Functions

extern "C" {
OSErr 	__myinitialize(CFragInitBlockPtr ibp);
void 	__myterminate(void);
OSErr  __initialize(CFragInitBlockPtr ibp);
void  __terminate(void);
NMErr	NMStartup(void);
NMErr	NMShutdown(void);
}

static void NMHandleConfigUserItems(DialogPtr theDialog, DialogItemIndex itemNo);
static NMErr ParseConfigString(const char *inConfigStr, NMType inGameID, const char *inGameName, 
								const void *inEnumData, NMUInt32 inDataLen, NMATConfigPriv *outConfig);
static void CreateListItemFromZoneList(DialogPtr whatDialog, DialogItemIndex whichItem, short LDEFID,
                                       ListHandle *setList, unsigned char *zoneList, unsigned char *defaultZone);
static OSErr SkankUpAnLDEF(short whichID, ListDefProcPtr whatProc);
static void DetachLDEF(short whichID);
static pascal void OffsetLDEFProc(short lMessage, NMBoolean lSelect, Rect *lRect, Cell lCell,
                                  short lDataOffset, short lDataLen, ListHandle lHandle);
static NMErr SetSearchZonesFromList(ListHandle whatList);
static NMErr NMRestartEnumeration(NMConfigRef inConfig);

pascal void ZLNotifier(void* contextPtr, ATEventCode code, NMErr result, void* cookie);
pascal void NotifyProc(void* contextPtr, ATEventCode code, NMErr result, void* cookie);

//	------------------------------	Public Variables

FSSpec						gFSSpec;
static NMModuleInfo			gModuleInfo;
static NMSInt16 			gBaseItem;
static CEntitiesEnumerator	*gEnumerator = NULL;
TPointerArray				gEnumList;
NMUInt32					gLastCount;
NMEnumerationCallbackPtr 	gEnumCallback;
void						*gEnumContext;
NMUInt32					gLastCallbackCount;
StringPtr					gZoneSearchList = NULL;	// a collection of strings, terminated with a null
StringPtr					gFullZoneList = NULL;	// a collection of strings, terminated with a null
Str32						gMyZoneName;
NMUInt32					gLookupSpecifer = kUseConfig;
NMBoolean 					gTerminating = false;
ListHandle					gZoneListRecord;

//ECF011117 we export this - its presence in our lib keeps OpenPlay from loading us while running native in OS-X
//(since we don't work there anyway, why confuse people?)
//it doesnt matter what this value is; only that it exists...
Boolean ClassicOnly;

//	--------------------	__myinitialize

#pragma mark === Initialization and Shutdown ===


//----------------------------------------------------------------------------------------
// __myinitialize
//----------------------------------------------------------------------------------------

OSErr
__myinitialize(CFragInitBlockPtr ibp)
{
OSErr	err;

	err = __initialize(ibp);
	
	if (err != kNMNoError)
		return (err);
	
	// we should always be kDataForkCFragLocator when we are a shared library.
	// assuming this is is the case, we want to open our resource file so we
	// can access our resources if needed. our termination routine will close
	// the file.
	if (ibp->fragLocator.where == kDataForkCFragLocator)
	{
		gFSSpec.vRefNum = ibp->fragLocator.u.onDisk.fileSpec->vRefNum;
		gFSSpec.parID = ibp->fragLocator.u.onDisk.fileSpec->parID;
		doCopyPStrMax(ibp->fragLocator.u.onDisk.fileSpec->name, gFSSpec.name, 64);
	}
	else
	{
		return (-1);
	}

	gModuleInfo.size= sizeof (NMModuleInfo);
	gModuleInfo.type = kModuleID;
	strcpy(gModuleInfo.name, kATModuleName);
	strcpy(gModuleInfo.copyright, kATModuleCopyright);
	gModuleInfo.maxPacketSize = 576;
	gModuleInfo.maxEndpoints = kNMNoEndpointLimit;
	gModuleInfo.flags= kNMModuleHasStream | kNMModuleHasDatagram | kNMModuleHasExpedited | kNMModuleRequiresIdleTime;
	
	err = NMStartup();

	DEBUG_PRINTonERR("NMStartup returned error: %ld\n", err);

	//set up sioux if we're using it - they won't be able to move the window, so stick it somewhere
	//unique..
	SIOUXSettings.userwindowtitle = "\pNetModuleAT debug output";
	SIOUXSettings.toppixel = 350;
	SIOUXSettings.leftpixel = 70;
	SIOUXSettings.rows = 40;
	SIOUXSettings.setupmenus = false;
	SIOUXSettings.standalone = false;

	return (err);
}

//----------------------------------------------------------------------------------------
// __myterminate
//----------------------------------------------------------------------------------------

void
__myterminate(void)
{
	gTerminating = true;

	//	See if there are any endpoints still trying to close.  If so, give them a moment
	//	this is totally OT dependent, but who cares?  We're never going to do a classic version...	
	if (! OTEndpoint::sZombieList.IsEmpty())
	{
	UnsignedWide	startTime;
	NMUInt32			elapsedMilliseconds;
		
		//	signal that the party's over, and anyone left should die NOW!
		EndpointDisposer::sLastChance = true;
		
		OTGetTimeStamp(&startTime); 

		while (! OTEndpoint::sZombieList.IsEmpty()) 
		{
			elapsedMilliseconds = OTElapsedMilliseconds(&startTime);

			if (elapsedMilliseconds > 5000)
			{
				op_vpause("Timed out waiting for endpoints to finish. OT is probably hosed.");
				break;
			}
		};

	}
	__terminate();		//  "You're Terminated."
	NMShutdown();		// because we use OT's global new (and delete), we must call
						//	CloseOpenTransport only as the very last thing.  This includes
						//	the global static destructor chain from _terminate
}

//----------------------------------------------------------------------------------------
// NMStartup
//----------------------------------------------------------------------------------------

// Called by the App (or whatever) to set thing up
NMErr
NMStartup(void)
{
NMErr	status = kNMNoError;

	//	Make sure we have the necessary libraries available for this module

	status = OTUtils::StartOpenTransport(OTUtils::kOTCheckForAppleTalk,5000,2500);

	if (status == kNMNoError) {
		gATNotifierUPP = NewOTNotifyUPP(NotifyProc);
		if (gATNotifierUPP == NULL)
			status = kNMProtocolInitFailedErr;
	}

	if (status == kNMNoError) {
		gATZLNotifierUPP = NewOTNotifyUPP(ZLNotifier);
		if (gATZLNotifierUPP == NULL)
			status = kNMProtocolInitFailedErr;
	}

	// prepare the dead endpoint list
	gDeadEndpoints->fHead = NULL;

	return (status);
}

//----------------------------------------------------------------------------------------
// NMShutdown
//----------------------------------------------------------------------------------------

// Called only when the module is being unloaded.  
NMErr
NMShutdown(void)
{
	// cleanup any remaining endpoints [gg]
	EndpointHander::CleanupEndpoints();

	//	Query- what if other modules have OT open, too?
	if (OTUtils::OTInitialized())
	{
		if (gATNotifierUPP != NULL) {
			DisposeOTNotifyUPP(gATNotifierUPP);
			gATNotifierUPP = NULL;
		}
		if (gATZLNotifierUPP != NULL) {
			DisposeOTNotifyUPP(gATZLNotifierUPP);
			gATZLNotifierUPP = NULL;
		}
		OTUtils::CloseOpenTransport();
	}

	return (kNMNoError);
}


#pragma mark === Endpoints ===

//----------------------------------------------------------------------------------------
// NMOpen
//----------------------------------------------------------------------------------------

// Open creates the endpoint and either listens or makes a connection
NMErr
NMOpen(NMConfigRef inConfig, NMEndpointCallbackFunction *inCallback, void *inContext, NMEndpointRef *outEndpoint, NMBoolean inActive)
{
	DEBUG_ENTRY_EXIT("NMOpen");

	//NMErr				err = kNMNoError;
	NMATEndpointPriv	*epRef = NULL;
	NMErr				status = kNMNoError;

	op_vassert_return((inCallback != NULL),"Callback function is NIL!",kNMParameterErr);
	op_vassert_return((outEndpoint != NULL),"outEndpoint endpoint is NIL!",kNMParameterErr);

	//	Alloc, but don't initialize the NMEndpoint
	status = MakeNewATEndpointPriv(inConfig, inCallback, inContext, kNMNormalMode, false, &epRef);	// If this returns an error, there's nothing to clean up

	if (status != kNMNoError)
		return (status);
		
	//Try_
	{
		//	Either connect or listen (passive = listen, active = connect)
		if (inActive)
		{
			status = epRef->ep->Connect();
			//ThrowIfOSErr_(status);
			if (status)
				goto error;
		}
		else
		{
			status = epRef->ep->Listen();
			//ThrowIfOSErr_(status);
			if (status)
				goto error;
		}

		*outEndpoint = (NMEndpointRef) epRef;
		
		if (inActive)
			epRef->ep->mState = Endpoint::kRunning;
		else
			epRef->ep->mState = Endpoint::kListening;
			
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		*outEndpoint = NULL;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMClose
//----------------------------------------------------------------------------------------

//	Closes a connection (or listener) and deletes the endpoint
//	This is an ASYNC call!
NMErr
NMClose(NMEndpointRef inEndpoint, NMBoolean inOrderly)
{
	DEBUG_ENTRY_EXIT("NMClose");

NMErr				err = kNMNoError;
NMATEndpointPriv	*epRef = (NMATEndpointPriv *)inEndpoint;
Endpoint 			*ep = (Endpoint *) epRef->ep;

UNUSED_PARAMETER(inOrderly);

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);

	ep->Close();
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMAcceptConnection
//----------------------------------------------------------------------------------------

NMErr
NMAcceptConnection(
	NMEndpointRef				inEndpoint,
	void						*inCookie, 
	NMEndpointCallbackFunction	*inCallback,
	void						*inContext)
{
	DEBUG_ENTRY_EXIT("NMAcceptConnection");

	NMErr				status = kNMNoError;
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;
	Endpoint			*ep = epRef->ep;
	NMATEndpointPriv	*newEPRef;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
	op_vassert_return((inCookie != NULL),"Cookie is NULL!",kNMParameterErr);
	op_vassert_return((inCallback != NULL),"Callback is NULL!",kNMParameterErr);
		
	//Try_
	{
		// First, we need to create a new endpoint that will be returned to the user
		status = MakeNewATEndpointPriv(NULL, inCallback, inContext, ep->mMode, ep->mNetSprocketMode, &newEPRef);

		if (status != kNMNoError)	// we must reject if we couldn't alloc an endpoint container
		{
			DEBUG_PRINT("Rejecting connection because MakeNewATEndpointPriv returned err:%d", status);
			ep->RejectConnection(inCookie);

			return status;
		}

		//ThrowIfOSErr_(status);
		if (status)
			goto error;

		status = ep->AcceptConnection(newEPRef->ep, inCookie);	// Hands off the new connection async

		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;	
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMRejectConnection
//----------------------------------------------------------------------------------------

NMErr
NMRejectConnection(NMEndpointRef inEndpoint, void *inCookie)
{
	DEBUG_ENTRY_EXIT("NMRejectConnection");

	NMErr				status = kNMNoError;
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;
	Endpoint 			*ep = (Endpoint *) epRef->ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((inCookie != NULL),"Cookie is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	//Try_
	{
		status = ep->RejectConnection(inCookie);			
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

#pragma mark === Information ===

//----------------------------------------------------------------------------------------
// NMGetModuleInfo
//----------------------------------------------------------------------------------------

NMErr
NMGetModuleInfo(NMModuleInfo *outInfo)
{
NMErr	err = kNMNoError;
		
	op_vassert_return((outInfo != NULL),"NMModuleInfo is NIL!",kNMParameterErr);

	if (outInfo->size >= sizeof (NMModuleInfo))
	{
		NMSInt32 size_to_copy = (outInfo->size<gModuleInfo.size) ? outInfo->size : gModuleInfo.size;
	
		machine_copy_data(&gModuleInfo, outInfo, size_to_copy);
	}
	else
	{
		err = kNMModuleInfoTooSmall;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// NMFreeAddress
//----------------------------------------------------------------------------------------

NMErr
NMFreeAddress(NMEndpointRef inEndpoint, void **outAddress)
{
	DEBUG_ENTRY_EXIT("NMFreeAddress")

	NMErr				err = noErr;
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *)inEndpoint;
	OTEndpoint 			*ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);

	ep = (OTEndpoint *) epRef->ep;
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);

	err = ep->FreeAddress( outAddress );

	return( err );
}

//----------------------------------------------------------------------------------------
// NMGetAddress
//----------------------------------------------------------------------------------------

NMErr
NMGetAddress(NMEndpointRef inEndpoint, NMAddressType addressType, void **outAddress)
{
	DEBUG_ENTRY_EXIT("NMGetAddress")

	NMErr				err = noErr;
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *)inEndpoint;
	OTEndpoint 			*ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);

	ep = (OTEndpoint *) epRef->ep;
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);

	err = ep->GetAddress( addressType, outAddress );

	return( err );
}

//----------------------------------------------------------------------------------------
// NMIsAlive
//----------------------------------------------------------------------------------------

NMBoolean
NMIsAlive(NMEndpointRef inEndpoint)
{
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;
	Endpoint 			*ep = (Endpoint *) epRef->ep;
	NMBoolean			result = false;
	NMErr				status = kNMNoError;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	//Try_
	{
		result = ep->IsAlive();
		return result;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return false;
	}
}

#pragma mark === Utility ===

//----------------------------------------------------------------------------------------
// NMSetTimeout
//----------------------------------------------------------------------------------------

NMErr
NMSetTimeout(NMEndpointRef inEndpoint, NMUInt32 inTimeout)
{
NMErr				err = kNMNoError;
NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;
Endpoint 			*ep = (Endpoint *) epRef->ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
	op_vassert_return((inTimeout >= 1),"Timeout of less than 1 millisecond requested!",kNMParameterErr);
		
	ep->SetTimeout(inTimeout);	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMGetIdentifier
//----------------------------------------------------------------------------------------

OP_DEFINE_API_C(NMErr)
NMGetIdentifier(NMEndpointRef inEndpoint,  char * outIdStr, NMSInt16 inMaxLen)
{
    NMErr				err = kNMNoError;
    NMATEndpointPriv	*epRef;
    Endpoint 			*ep;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((outIdStr != NULL),"Identifier string ptr is NIL!",kNMParameterErr);
	op_vassert_return((inMaxLen > 0),"Max string length is less than one!",kNMParameterErr);

	epRef = (NMATEndpointPriv *) inEndpoint;
	ep = (Endpoint *) epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
	
    return ep->GetIdentifier(outIdStr, inMaxLen);
}


//----------------------------------------------------------------------------------------
// NMIdle
//----------------------------------------------------------------------------------------

NMErr
NMIdle(NMEndpointRef inEndpoint)
{
	UNUSED_PARAMETER(inEndpoint);

	EndpointHander::CleanupEndpoints();

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMFunctionPassThrough
//----------------------------------------------------------------------------------------

NMErr
NMFunctionPassThrough(NMEndpointRef inEndpoint, NMUInt32 inSelector, void *inParamBlock)
{
	DEBUG_ENTRY_EXIT("NMFunctionPassThrough");

	NMErr				status = kNMNoError;
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;
	Endpoint 			*ep = (Endpoint *) epRef->ep;

	if (inEndpoint != NULL)
	{	
		op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
		op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
		op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
	}
		
	//Try_
	{
		status = DoPassThrough(inEndpoint, inSelector, inParamBlock);
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

#pragma mark === Data ===

//----------------------------------------------------------------------------------------
// NMSendDatagram
//----------------------------------------------------------------------------------------

NMErr
NMSendDatagram(
	NMEndpointRef	inEndpoint,
	NMUInt8			*inData,
	NMUInt32		inSize,
	NMFlags			inFlags)
{
	DEBUG_ENTRY_EXIT("NMSendDatagram");

	NMErr				status = kNMNoError;
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;
	Endpoint 			*ep = (Endpoint *) epRef->ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
	op_vassert_return((inData != NULL),"Data pointer is NULL!",kNMParameterErr);
	op_vassert_return((inSize > 0),"Data size < 1!",kNMParameterErr);
	op_vassert_return((inSize <= gModuleInfo.maxPacketSize),"Data size %d greater than max data size!",kNMTooMuchDataErr);
		
	//Try_
	{
		status = ep->SendDatagram(inData, inSize, inFlags);
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMReceiveDatagram
//----------------------------------------------------------------------------------------

NMErr
NMReceiveDatagram(
	NMEndpointRef	inEndpoint,
	NMUInt8			*ioData,
	NMUInt32		*ioSize,
	NMFlags			*outFlags)
{
	DEBUG_ENTRY_EXIT("NMReceiveDatagram");

	NMErr				status = kNMNoError;
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;
	Endpoint 			*ep = (Endpoint *) epRef->ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
	op_vassert_return((ioData != NULL),"Data pointer is NULL!",kNMParameterErr);
	op_vassert_return((ioSize != NULL),"Data size pointer is NULL!",kNMParameterErr);
	op_vassert_return((outFlags != NULL),"Flags pointer is NULL!",kNMParameterErr);
	op_vassert_return((*ioSize >= 1),"Data size < 1!",kNMParameterErr);
	
	//Try_
	{
		status = ep->ReceiveDatagram(ioData, ioSize, outFlags);
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMSend
//----------------------------------------------------------------------------------------

NMSInt32
NMSend(
	NMEndpointRef	inEndpoint,
	void			*inData,
	NMUInt32		inSize,
	NMFlags			inFlags)
{
	DEBUG_ENTRY_EXIT("NMSend");

	NMSInt32			rv;
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;
	Endpoint 			*ep = (Endpoint *) epRef->ep;
	NMErr				status = kNMNoError;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
	op_vassert_return((inData != NULL),"Data pointer is NULL!",kNMParameterErr);
	op_vassert_return((inSize > 0),"Data size < 1!",kNMParameterErr);
		
	//Try_
	{
		rv = ep->Send(inData, inSize, inFlags);
		return rv;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMReceive
//----------------------------------------------------------------------------------------

NMErr
NMReceive(
	NMEndpointRef	inEndpoint,
	void			*ioData,
	NMUInt32		*ioSize,
	NMFlags			*outFlags)
{
	DEBUG_ENTRY_EXIT("NMReceive");

	NMErr				status = kNMNoError;
	NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;
	Endpoint 			*ep = (Endpoint *) epRef->ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
	op_vassert_return((ioData != NULL),"Data pointer is NULL!",kNMParameterErr);
	op_vassert_return((ioSize != NULL),"Data size pointer is NULL!",kNMParameterErr);
	op_vassert_return((outFlags != NULL),"Flags pointer is NULL!",kNMParameterErr);
	op_vassert_return((*ioSize >= 1),"Data size < 1!",kNMParameterErr);
		
	//Try_
	{
		status = ep->Receive(ioData, ioSize, outFlags);
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

#pragma mark === Enter/Leave Notifier ===

//----------------------------------------------------------------------------------------
//	Calls the "Enter Notifier" function on the requested endpoint (stream or datagram).
//----------------------------------------------------------------------------------------

NMErr
NMEnterNotifier(NMEndpointRef inEndpoint, NMEndpointMode endpointMode)
{
	DEBUG_ENTRY_EXIT("NMClose");

NMErr				err = noErr;
NMATEndpointPriv	*epRef = (NMATEndpointPriv *)inEndpoint;
Endpoint 			*ep = (Endpoint *) epRef->ep;

	err = ep->EnterNotifier(endpointMode);
	
	return err;
}


//----------------------------------------------------------------------------------------
//	Calls the "Leave Notifier" function on the requested endpoint (stream or datagram).
//----------------------------------------------------------------------------------------

NMErr
NMLeaveNotifier(NMEndpointRef inEndpoint, NMEndpointMode endpointMode)
{
	DEBUG_ENTRY_EXIT("NMClose");

NMErr				err = noErr;
NMATEndpointPriv	*epRef = (NMATEndpointPriv *)inEndpoint;
Endpoint 			*ep = (Endpoint *) epRef->ep;

	err = ep->LeaveNotifier(endpointMode);
	
	return err;
}

#pragma mark === Config ===

//----------------------------------------------------------------------------------------
// NMCreateConfig
//----------------------------------------------------------------------------------------

NMErr
NMCreateConfig(
	char		*inConfigStr,
	NMType		inGameID,
	const char	*inGameName,
	const void	*inEnumData,
	NMUInt32	inDataLen,
	NMConfigRef	*outConfig)
{
	DEBUG_ENTRY_EXIT("NMCreateConfig");

	NMErr			status = kNMNoError;
	NMATConfigPriv	*config = NULL;

	if (inConfigStr == NULL)
	{
		op_vassert_return((inGameName != NULL),"Game name is NIL!",kNMParameterErr);
		op_vassert_return((inGameID != 0),"Game ID is 0.  Use your creator type!",kNMParameterErr);
	}

	op_vassert_return((outConfig != NULL),"outConfig pointer is NIL!",kNMParameterErr);

	config = new NMATConfigPriv;
	if (config == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	status = ParseConfigString(inConfigStr, inGameID, inGameName, inEnumData, inDataLen, config);
	if (status)
		goto error;

	if (status == kNMNoError)
		*outConfig = (NMConfigRef) config;

error:
	if (status)
	{
		delete config;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMGetConfigLen
//----------------------------------------------------------------------------------------

NMSInt16
NMGetConfigLen(NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMGetConfigLen");

NMSInt16		configLen = 1024;
char			configString[1024];	// yes, a hack.
NMErr			err;
	
	op_vassert_return((inConfig != NULL),"Config ref is NULL!",0);

	err = NMGetConfig(inConfig, configString, &configLen);
	
	return configLen;
	
}

//----------------------------------------------------------------------------------------
// NMGetConfig
//----------------------------------------------------------------------------------------

NMErr
NMGetConfig(
	NMConfigRef		inConfig,
	char			*outConfigStr,
	NMSInt16		*ioConfigStrLen)
{
	DEBUG_ENTRY_EXIT("NMGetConfig");

	NMATConfigPriv *theConfig = (NMATConfigPriv *) inConfig;
	NMBoolean		putToken;
	NMUInt32		tokenLen;
	char			tempStr[33];
	NMErr			status = kNMNoError;
	
	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((outConfigStr != NULL),"outConfigStr is NULL!",kNMParameterErr);
	op_vassert_return((theConfig->type == kModuleID),"Config ref doesn't belong to this module!",kNMInvalidConfigErr);
	op_vassert_return((theConfig->version <= kVersion),"Config ref belongs to a later version of this module!",kNMNewerVersionErr);
	op_vassert_return((ioConfigStrLen != NULL),"ioConfigStrLen is NULL!",kNMParameterErr);
		
	//Try_
	{
		
		*outConfigStr = '\0';

		//	Write the type
		tokenLen = sizeof (NMType);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigModuleType, LONG_DATA, &theConfig->type, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the version
		tokenLen = sizeof (NMUInt32);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigModuleVersion, LONG_DATA, &theConfig->version, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the game id
		tokenLen = sizeof (NMUInt32);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigGameID, LONG_DATA, &theConfig->gameID, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the name
		tokenLen = strlen (theConfig->name);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigGameName, STRING_DATA, &theConfig->name, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the connection mode
		tokenLen = sizeof (NMUInt32);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigEndpointMode, LONG_DATA, &theConfig->connectionMode, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the NetSprocket mode
		tokenLen = sizeof (NMBoolean);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigNetSprocketMode, BOOLEAN_DATA, &theConfig->netSprocketMode, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write protocol-specific stuff
		//	Write the NBP name
		doCopyPStrMax(theConfig->nbpName, (unsigned char *) tempStr, 33);
		p2cstr((unsigned char *) tempStr);
		tokenLen = strlen (tempStr);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kATNBPName, STRING_DATA, tempStr, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);
		
		//	Write the NBP type
		doCopyPStrMax(theConfig->nbpType, (unsigned char *) tempStr, 33);
		p2cstr((unsigned char *) tempStr);
		tokenLen = strlen (tempStr);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kATNBPType, STRING_DATA, tempStr, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);
		
		//	Write the NBP name
		doCopyPStrMax(theConfig->nbpZone, (unsigned char *) tempStr, 33);
		p2cstr((unsigned char *) tempStr);
		
		tokenLen = strlen (tempStr);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kATNBPZone, STRING_DATA, tempStr, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);
		
		*ioConfigStrLen = strlen (outConfigStr);
			
		return kNMNoError;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMDeleteConfig
//----------------------------------------------------------------------------------------

NMErr
NMDeleteConfig(NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMDeleteConfig");

NMATConfigPriv *theConfig = (NMATConfigPriv *) inConfig;

	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);

	delete theConfig;
	return kNMNoError;
}

#ifndef OP_PLATFORM_MAC_CARBON_FLAG
// TODO - find a carbonized replacement
static RoutineDescriptor NMHandleItemRD
	= BUILD_ROUTINE_DESCRIPTOR(uppUserItemProcInfo, NMHandleConfigUserItems);
#endif

#pragma mark === Human Interface ===

//----------------------------------------------------------------------------------------
// NMSetupDialog
//----------------------------------------------------------------------------------------

NMErr
NMSetupDialog(
	NMDialogPtr	dialog,
	NMSInt16	frame,
	NMSInt16	inBaseItem,
	NMConfigRef	inConfig)
{
	DEBUG_ENTRY_EXIT("NMSetupDialog");
	
	Handle				ourDITL;
	NMATConfigPriv		*theConfig = (NMATConfigPriv *) inConfig;
	SetupLibraryState	duh;	// Make sure we're accessing the right resource fork
	NMErr				status = kNMNoError;	
			
	SetTempPort			port(dialog);

	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",kNMParameterErr);

	gBaseItem = inBaseItem;
	
	//	Try to load in our DITL.  If we fail, we should bail
	
	ourDITL = Get1Resource('DITL', kDITLID);
	if (ourDITL == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}
	
	if (ourDITL == NULL)
		return kNMResourceErr;

	// Append our DITL relative to the frame by passing the negative of the frame's id
	AppendDITL(dialog, ourDITL, -frame);
	ReleaseResource(ourDITL);

	// Set up our user items for our list
	{
	//DialogItemType		iType;
	//Handle				iHandle;
	//Rect				iRect;


		// install same user item function pointer for both items
		//GetDialogItem(dialog,(DialogItemIndex) (kZoneList+gBaseItem),
		//	&iType,&iHandle,&iRect);

#ifdef OP_PLATFORM_MAC_CARBON_FLAG
	op_vpause("NMSetupDialog - put back the thing");
#else

		//SetDialogItem(dialog,(DialogItemIndex) (kZoneList+gBaseItem),
		//	iType,(Handle) &NMHandleItemRD,&iRect);
#endif // OP_PLATFORM_MAC_CARBON_FLAG

	}

	// set our lookup method to be as configured
	(void) InternalSetLookupMethod(kSearchSpecifiedZones);

	// sjb 19990405 now set up the zone list if we have one
	// Get zone list
	(void) InternalGetZoneList(&gFullZoneList, kDefaultATTimeout);

	// sjb 19990408 get our zone to default to
	(void) InternalGetMyZone(gMyZoneName);

	// set the default search list to only our zone
	(void) InternalSetSearchZone(gMyZoneName);

	// Insert it into the list manager list
	CreateListItemFromZoneList(dialog,(DialogItemIndex) (kZoneList+gBaseItem),kLDEFOffsetFakeID,
	                           &gZoneListRecord,gFullZoneList,gMyZoneName);

	return kNMNoError;

error:
	return status;
}

//----------------------------------------------------------------------------------------
// SkankUpAnLDEF
//----------------------------------------------------------------------------------------

OSErr
SkankUpAnLDEF(short whichID, ListDefProcPtr whatProc)
{
#ifndef OP_PLATFORM_MAC_CARBON_FLAG
Handle						LDEFResource;
OSErr						retErr;
RoutineDescriptor			inPlace =
	BUILD_ROUTINE_DESCRIPTOR(uppListDefProcInfo,whatProc);

	LDEFResource = Get1Resource('LDEF',whichID);

	if (NULL == LDEFResource)
	{
		retErr = ResError();

		if (kNMNoError == retErr)
			retErr = resNotFound;

		return retErr;
	}

	// can't let this purge since we are going to modify it
	HNoPurge(LDEFResource);

	SetHandleSize(LDEFResource, sizeof (inPlace));

	if (kNMNoError != (retErr = ResError()))
	{
		ReleaseResource(LDEFResource);
		return retErr;
	}

	// store our RD in here, yuck!
	// Note that a RD is really data, sort of, so use BlockMove, not BMD
	BlockMove(&inPlace,*LDEFResource,sizeof (inPlace));	

	// Don't you dare call changed resource on this resource, okay?
#else
	UNUSED_PARAMETER(whichID);
	UNUSED_PARAMETER(whatProc);

#endif // OP_PLATFORM_MAC_CARBON_FLAG

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// DetachLDEF
//----------------------------------------------------------------------------------------

void
DetachLDEF(short whichID)
{
Handle						LDEFResource;

	LDEFResource = Get1Resource('LDEF',whichID);

	if (NULL != LDEFResource)
	{
		DetachResource(LDEFResource);
	}
}

//----------------------------------------------------------------------------------------
// OffsetLDEFProc
//----------------------------------------------------------------------------------------

static pascal void
OffsetLDEFProc(short lMessage, NMBoolean lSelect, Rect *lRect, Cell lCell,
               short lDataOffset, short lDataLen, ListHandle lHandle)
{
NMUInt8  *dataStart = (NMUInt8 *) (*lHandle)->refCon;

	UNUSED_PARAMETER(lDataOffset);
	UNUSED_PARAMETER(lDataLen);

	switch (lMessage)
	{
		case lInitMsg:
			// nada
			break;

		case lDrawMsg:
		case lHiliteMsg:
		{
		NMUInt32	dataOffset;
		NMSInt16	cellDataLen;
		NMUInt8		*realData;
		NMUInt32    realDataLen;
		FontInfo	curFontInfo;

			// get 4-byte offset from cell data
			cellDataLen = sizeof (dataOffset);
			LGetCell(&dataOffset,&cellDataLen,lCell,lHandle);

			realData = dataStart + dataOffset;
			realDataLen = *realData++;

			GetFontInfo(&curFontInfo);

			EraseRect(lRect);
			MoveTo(lRect->left,lRect->top+curFontInfo.ascent);
			DrawText(realData,0,realDataLen);

			if (lSelect)
			{
				LMSetHiliteMode(LMGetHiliteMode() & ~(1<<hiliteBit));
				InvertRect(lRect);
			}
		}
		break;

		case lCloseMsg:
			// nada
			break;
	}
}

//----------------------------------------------------------------------------------------
// CreateListItemFromZoneList
//----------------------------------------------------------------------------------------

void
CreateListItemFromZoneList(
	DialogPtr		whatDialog,
	DialogItemIndex	whichItem,
	NMSInt16		LDEFID,
	ListHandle		*setList,
	NMUInt8			*zoneList,
	NMUInt8			*defaultZone)
{
GWorldPtr				oldGWorld;
GDHandle				oldGDevice;

	UNUSED_PARAMETER(LDEFID);

	GetGWorld(&oldGWorld,& oldGDevice);
	
	#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		SetPortDialogPort(whatDialog);
	#else
		SetPort(whatDialog);
	#endif	// OP_PLATFORM_MAC_CARBON_FLAG

	if (zoneList[0] && setList)
	{
	DialogItemType			itemType;
	Handle					itemHandle;
	Rect					itemRect;
	ListHandle				newList;
	Rect					dataBounds = { 0, 0, 0, 1 };	// 1 column wide, no rows
	Point					cellSize;
	FontInfo				curFontInfo;

		GetDialogItem(whatDialog,whichItem,&itemType,&itemHandle,&itemRect);

		itemRect.right -= 16;			// to make room for scroll bar
		itemRect.top += 1;
		itemRect.bottom -= 1;
		itemRect.left += 1;

		GetFontInfo(&curFontInfo);

		cellSize.h = itemRect.right - itemRect.left;
		cellSize.v = curFontInfo.ascent + curFontInfo.descent + curFontInfo.leading;

#ifndef OP_PLATFORM_MAC_CARBON_FLAG
		// make our fake LDEF first
		if (kNMNoError !=  SkankUpAnLDEF(LDEFID,OffsetLDEFProc))
			goto errOut;			// hmm, return an error?

		// sjb shrink item box to be multiple of cellSize.v
		newList = LNew(&itemRect, &dataBounds, cellSize, LDEFID, GetDialogWindow(whatDialog), 0, 0, 0, 1);

		// Danger, evil alert!
		// It is evil to assume that the ListManager doesn't mind that I
		// am detaching the LDEF after it gets it.  Too bad.
		DetachLDEF(LDEFID);
#else
		newList = LNew(&itemRect, &dataBounds, cellSize, 0, GetDialogWindow(whatDialog), 0, 0, 0, 1);
#endif // OP_PLATFORM_MAC_CARBON_FLAG

		if (newList)
		{
		NMUInt8		*itemStr = zoneList;
		NMUInt32	countZones = 0;

			// return list
			*setList = newList;

			// set real data base in refcon
			(*newList)->refCon = (NMSInt32) zoneList;

			// add items
			while (itemStr[0])
			{
			NMSInt16	newRow;
			Cell		newCell;
			NMUInt32	thisOffset;

				newRow = LAddRow(1,32767,newList);

				newCell.h = 0;
				newCell.v = newRow;

				thisOffset = itemStr-zoneList;
				LSetCell(&thisOffset,4,newCell,newList);

				// this doesn't work since defaultZone is wrong
				if (0 == RelString(itemStr,defaultZone, 1, 1))
					LSetSelect(true, newCell, newList);		// select our cell

				itemStr += itemStr[0] + 1;
			}

			// now scroll to default zone
			LAutoScroll(newList);

			LSetDrawingMode(1,newList);		// turn on drawing now
		}	
	}

errOut:
	SetGWorld(oldGWorld,oldGDevice);
}

//----------------------------------------------------------------------------------------
// SetSearchZonesFromList
//----------------------------------------------------------------------------------------

NMErr
SetSearchZonesFromList(ListHandle whatList)
{
	if (0 == whatList)
		return paramErr;

Cell		curCell = { 0,0 };
NMUInt32	listLength = 1;

	// once through to get the total length
	while (LGetSelect(true,&curCell,whatList))
	{
	NMUInt32			getBuff;
	NMSInt16			getLen = sizeof (getBuff);
	StringPtr			cellData;

		LGetCell(&getBuff,&getLen,curCell,whatList);

		cellData = ((unsigned char *) ((*whatList)->refCon)) + getBuff;

		listLength += 1 + cellData[0];

		// now go to the next cell
		LNextCell(true,true,&curCell,whatList);
	}

	NMUInt8	*zoneData = new NMUInt8 [listLength];

	if (0 == zoneData)
		return memFullErr;

	NMUInt8	*zoneStor = zoneData;

	curCell.h = curCell.v = 0;

	// now get all the cell data into the zone list
	while (LGetSelect(true, &curCell, whatList))
	{
	NMUInt32			getBuff;
	NMSInt16			getLen = sizeof (getBuff);
	StringPtr			cellData;

		LGetCell(&getBuff,&getLen,curCell,whatList);

		cellData = ((unsigned char *) ((*whatList)->refCon)) + getBuff;

		machine_move_data(cellData,zoneStor,1+cellData[0]);
		zoneStor += 1+cellData[0];

		// now go to the next cell
		LNextCell(true,true,&curCell,whatList);
	}

	zoneStor[0] = 0;			// terminate

	// set to search
	NMErr retErr = InternalSetSearchZones(zoneData);

	delete [] zoneData;

	return retErr;
}

//----------------------------------------------------------------------------------------
// NMHandleConfigUserItems
//----------------------------------------------------------------------------------------

void
NMHandleConfigUserItems(DialogPtr theDialog, DialogItemIndex itemNo)
{
DialogItemType			iType;
Handle					iHandle;
Rect					iRect;
RgnHandle				dialogVisRgn;

	GetDialogItem(theDialog,itemNo,&iType,&iHandle,&iRect);

	switch (itemNo - gBaseItem)
	{
		case kZoneList:
			FrameRect(&iRect);
			dialogVisRgn = NewRgn();

			#ifdef OP_PLATFORM_MAC_CARBON_FLAG
				GetPortVisibleRegion(GetDialogPort(theDialog), dialogVisRgn);
			#else
				dialogVisRgn = theDialog->visRgn;
			#endif	// OP_PLATFORM_MAC_CARBON_FLAG

			LUpdate(dialogVisRgn, gZoneListRecord);
			DisposeRgn(dialogVisRgn);
			break;
	}
}

//----------------------------------------------------------------------------------------
// NMHandleEvent
//----------------------------------------------------------------------------------------

NMBoolean NMHandleEvent(NMDialogPtr dialog, NMEvent *event, NMConfigRef inConfig)
{
NMATConfigPriv	*theConfig = (NMATConfigPriv *) inConfig;
SetTempPort		port(dialog);

	UNUSED_PARAMETER(event);

	op_vassert_return((theConfig != NULL),"Config ref is NULL!",false);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",false);
	op_vassert_return((event != NULL),"event ptr is NULL!",false);

	return false;
}

//----------------------------------------------------------------------------------------
// NMHandleItemHit
//----------------------------------------------------------------------------------------

NMErr
NMHandleItemHit(NMDialogPtr dialog, short inItemHit, NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMHandleItemHit");

NMATConfigPriv	*theConfig = (NMATConfigPriv *) inConfig;
NMSInt16 		theRealItem = inItemHit - gBaseItem;
SetTempPort		port(dialog);
EventRecord		evtToUse;

	op_vassert_return((theConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",kNMParameterErr);

	// get an event (null) so we get the mouse and keyboard values back that ModalDialog ate
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
	op_vpause("NMHandleItemHit - put back OSEventAvail in some way");
#else
	OSEventAvail(everyEvent, &evtToUse);
#endif

	GlobalToLocal(&(evtToUse.where));

	switch (inItemHit - gBaseItem)
	{
		case kZoneList:
			(void) LClick(evtToUse.where,evtToUse.modifiers,gZoneListRecord);

			// Now, update our zone search list from the new list
			// and begin searching again
			SetSearchZonesFromList(gZoneListRecord);

			// begin searching again
			(void) NMRestartEnumeration(inConfig);
			break;
	}

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMTeardownDialog
//----------------------------------------------------------------------------------------

NMBoolean
NMTeardownDialog(
	NMDialogPtr	dialog,
	NMBoolean	inUpdateConfig,
	NMConfigRef	ioConfig)
{
	DEBUG_ENTRY_EXIT("NMTeardownDialog");

NMATConfigPriv	*theConfig = (NMATConfigPriv *) ioConfig;
SetTempPort		port(dialog);
	
	op_vassert_return((theConfig != NULL),"Config ref is NULL!",true);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",true);

	// First un setup the dialog pointers so we don't get called later
	{
	DialogItemType		iType;
	Handle				iHandle;
	Rect				iRect;

		// install same user item function pointer for both items
		GetDialogItem(dialog,(DialogItemIndex) (kZoneList+gBaseItem),
			&iType,&iHandle,&iRect);

		SetDialogItem(dialog,(DialogItemIndex) (kZoneList+gBaseItem),
			iType,(Handle) 0,&iRect);

		// install same user item function pointer for both items
		GetDialogItem(dialog,(DialogItemIndex) (kZoneList+gBaseItem),
			&iType,&iHandle,&iRect);

		SetDialogItem(dialog,(DialogItemIndex) (kZoneList+gBaseItem),
			iType,(Handle) 0,&iRect);
	}

	if (inUpdateConfig)
	{
	}

	(void) InternalDisposeZoneList(gFullZoneList);
	gFullZoneList = 0;

	return true;
}

//----------------------------------------------------------------------------------------
// NMGetRequiredDialogFrame
//----------------------------------------------------------------------------------------

void
NMGetRequiredDialogFrame(NMRect *r, NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMGetRequiredDialogFrame");

	UNUSED_PARAMETER(inConfig);

	//	I don't know of any reason why the config is going to affect the size of the frame
	*r = kFrameSize;
}

#pragma mark === Enumeration ===

// This is for the callbacks from the OT enumerator

//----------------------------------------------------------------------------------------
// NotifyProc
//----------------------------------------------------------------------------------------

pascal void
NotifyProc(void* contextPtr, ATEventCode code, NMErr result, void* cookie)
{
	NMUInt32				count;
	NMBoolean				allFound;
	//NMErr				status;
	ATEnumerationItemPriv	*item = NULL;
	Str32					name, zone, type;
	ATAddress				addr;
	NMSInt32				i;
	NMErr					status = kNMNoError;
	
	DEBUG_ENTRY_EXIT("NotifyProc");
	
	UNUSED_PARAMETER(cookie);
	UNUSED_PARAMETER(result);
	UNUSED_PARAMETER(contextPtr);
	
	switch (code)
	{
		case AT_NEW_ENTITIES:
			status = gEnumerator->GetCount(&allFound, &count);

			if (status == kNMNoError && count > gLastCount)
			{
				do
				{
					i = gLastCount + 1;
					item = new ATEnumerationItemPriv;
					if (item == NULL){
						status = kNSpMemAllocationErr;
						goto error;
					}
					
					status = gEnumerator->GetIndexedEntity(i, zone, type, name, &addr);
					if (status)
						goto error;
					
					// fill in the enum structure
					item->enumIndex = i;
					doCopyPStr(name, item->name);

					if (zone[1] != '*')
					{
						doConcatPStr(item->name, "\p@");
						doConcatPStr(item->name, zone);
					}

					item->userEnum.id = (NMType) item;
					p2cstr(item->name);
					item->userEnum.name = (char *) item->name;
					item->userEnum.customEnumDataLen = 0;
					item->userEnum.customEnumData = NULL;
					 
					//	Add it to the list
					gEnumList.AddItem(item);
	
				} while (++gLastCount < count);
			}
			else
			{
				DEBUG_PRINT("GetCount returned an error in enum notifier: %d", status);
			}

error:
			if (status)
			{
				DEBUG_PRINT("Caught exception: %d.  gLastCount: %d, count: %d, index:%d", status, gLastCount, count, i);
				if( item )
					delete item;

				op_vassert((code != kNMOutOfMemoryErr),"Ran out of memory for enumeration responses!");
			}
		break;

		case AT_ENTITIESLIST_COMPLETE:
			status = gEnumerator->GetCount(&allFound, &count);
			break;

		default:
			break;
	}
}

//----------------------------------------------------------------------------------------
// NMRestartEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMRestartEnumeration(NMConfigRef inConfig)
{
NMATConfigPriv	*theConfig = (NMATConfigPriv *) inConfig;
NMErr		status = kNMNoError;
StringPtr		zone;
StringPtr		name;
StringPtr		type;

	DEBUG_ENTRY_EXIT("NMRestartEnumeration");

	if (0 == gEnumerator)
		return kNMNotEnumeratingErr;

	status = gEnumerator->CancelLookup();
	op_warn(status == kNMNoError);

	//	The way we do a lookup depends on the config.  We might want to look in all
	//	zones, or in a particular zone.  By default, the configurator specifies
	//	the current zone.

	if (gLookupSpecifer == kUseConfig)
	{
		zone = theConfig->nbpZone;
	}
	else
	{
		zone = gZoneSearchList;
	}

	//	Clear out any cruft from a previous enum
	gLastCount = 0;
	gLastCallbackCount = 0;
	DeleteAllItems(&gEnumList, ATEnumerationItemPriv*);

	name = kATSearchAllNames;
	type = (theConfig->nbpType[0] == 0) ? kATSearchAllEntityTypes : theConfig->nbpType;

	(gEnumCallback)(gEnumContext, kNMEnumClear, NULL);

	//	start the lookup going
	status = gEnumerator->StartLookup(zone, type, name, true);

	return status;
}

//----------------------------------------------------------------------------------------
// NMStartEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMStartEnumeration(NMConfigRef inConfig, NMEnumerationCallbackPtr inCallback, void *inContext, NMBoolean inActive)
{
	DEBUG_ENTRY_EXIT("NMStartEnumeration");

	NMATConfigPriv	*theConfig = (NMATConfigPriv *) inConfig;
	NMErr			status = kNMNoError;
	StringPtr		zone;
	StringPtr		name;
	StringPtr		type;

	UNUSED_PARAMETER(inActive);

	op_vassert_return((theConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((inCallback != NULL),"Callback function ptr is NULL!",kNMParameterErr);

	if (gEnumerator != NULL)
		return kNMAlreadyEnumeratingErr;
	
	gEnumerator = new COTEntitiesEnumerator();

	if (gEnumerator == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	//	Clear out any cruft from a previous enum
	gLastCount = 0;
	gLastCallbackCount = 0;
	
	DeleteAllItems(&gEnumList, ATEnumerationItemPriv*);

	//	Set the callback info
	gEnumCallback = inCallback;
	gEnumContext = inContext;
	
	status = gEnumerator->Initialize(kATAppleTalkDefaultPortRef, gATNotifierUPP, NULL);
	if (status)
		goto error;

	//	The way we do a lookup depends on the config.  We might want to look in all
	//	zones, or in a particular zone.  By default, the configurator specifies
	//	the current zone.

	if (gLookupSpecifer == kUseConfig)
	{
		zone = theConfig->nbpZone;
	}
	else
	{
		zone = gZoneSearchList;
	}

	name = kATSearchAllNames;
	type = (theConfig->nbpType[0] == 0) ? kATSearchAllEntityTypes : theConfig->nbpType;

	//	first clear out any current enums
	(gEnumCallback)(gEnumContext, kNMEnumClear, NULL);

	//	start the lookup going
	
	status = gEnumerator->StartLookup(zone, type, name, true);

error:
	if (status)
	{
		delete gEnumerator;
		gEnumerator = NULL;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMIdleEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMIdleEnumeration(NMConfigRef inConfig)
{
NMATConfigPriv	*theConfig = (NMATConfigPriv *) inConfig;
NMErr			status = kNMNoError;
NMUInt32			count;
ATEnumerationItemPriv *item;

	if (!gEnumerator)
		return kNMNotEnumeratingErr;

#ifndef OP_PLATFORM_MAC_CARBON_FLAG
	//	Give the enumerator time
	SystemTask();
#endif
	
	//	How many enumerations responses do we have now?
	count = gEnumList.Count();

	//	DEBUG_PRINT("enum count is %d", count);
	//	If we have more than we did last time, make the callbacks
	if (count > gLastCallbackCount)
	{
		for (; gLastCallbackCount < count; gLastCallbackCount++)
		{
			item = (ATEnumerationItemPriv *) gEnumList.GetIndexedItem(gLastCallbackCount);
			(gEnumCallback)(gEnumContext, kNMEnumAdd, &item->userEnum);
		}
	}
	
	return kNMNoError;

}

//----------------------------------------------------------------------------------------
// NMEndEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMEndEnumeration(NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMEndEnumeration");

NMATConfigPriv	*theConfig = (NMATConfigPriv *) inConfig;
NMErr			status = kNMNoError;
	
	if (! gEnumerator)
		return kNMNotEnumeratingErr;

	status = gEnumerator->CancelLookup();
	op_warn(status == kNMNoError);
	
	delete gEnumerator;
	gEnumerator = NULL;

	// sjb 19990413 reset counts since we are done
	gLastCount = 0;
	gLastCallbackCount = 0;
	DeleteAllItems(&gEnumList, ATEnumerationItemPriv*);

	return status;
}

//----------------------------------------------------------------------------------------
// NMBindEnumerationItemToConfig
//----------------------------------------------------------------------------------------

NMErr
NMBindEnumerationItemToConfig(NMConfigRef inConfig, NMHostID inID)
{	
	DEBUG_ENTRY_EXIT("NMBindEnumerationItemToConfig");


	//	the Host ID is really a pointer to the enumeration item
	ATEnumerationItemPriv 	*item = (ATEnumerationItemPriv *) inID;
	NMATConfigPriv 			*config = (NMATConfigPriv *) inConfig;
	ATAddress				addr;
	NMErr					status = kNMNoError;
	
	op_warn(item != NULL);

	if (gEnumerator == NULL)
		return kNMNotEnumeratingErr;

		
	//Try_
	{

		status = gEnumerator->GetIndexedEntity(item->enumIndex, config->nbpZone, config->nbpType, config->nbpName, &addr);
		//ThrowIfOSErr_(status);
		if (status)
			goto error;

		//	Set up our config structure with the info from this enumeration item
		// 19990129 sjb config->name is a CStr, make it so
		doCopyPStr(config->nbpName,(StringPtr) config->name);
		p2cstr((StringPtr) config->name);
			
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMStartAdvertising
//----------------------------------------------------------------------------------------

NMBoolean
NMStartAdvertising(NMEndpointRef inEndpoint)
{
NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;

	return epRef->ep->SetQueryForwarding(true);
}

//----------------------------------------------------------------------------------------
// NMStopAdvertising
//----------------------------------------------------------------------------------------

NMBoolean
NMStopAdvertising(NMEndpointRef inEndpoint)
{
NMATEndpointPriv	*epRef = (NMATEndpointPriv *) inEndpoint;

	return epRef->ep->SetQueryForwarding(false);
}

#pragma mark === Internal Only ===

//----------------------------------------------------------------------------------------
// ParseConfigString
//----------------------------------------------------------------------------------------

static NMErr
ParseConfigString(
	const char		*inConfigStr,
	NMType			inGameID,
	const char		*inGameName, 
	const void		*inEnumData,
	NMUInt32		inDataLen,
	NMATConfigPriv	*outConfig)
{
	NMErr	status = kNMNoError;
	NMBoolean	gotToken;
	NMSInt32	tokenLen;
	
	outConfig->type = kModuleID;
	outConfig->version = kVersion;

	outConfig->gameID = inGameID;
	outConfig->connectionMode = kNMNormalMode;

	if (inGameName)
	{
		strncpy(outConfig->name, inGameName, kMaxGameNameLen);
		outConfig->name[kMaxGameNameLen-1]= 0;
	}
	else
	{
		strcpy(outConfig->name, "unknown");
	}

	if (inDataLen)
	{
		if (inDataLen > kNMEnumDataLen)
			return kNMInvalidConfigErr;
		machine_move_data(inEnumData, outConfig->customEnumData, inDataLen);
	}		

	outConfig->customEnumDataLen = inDataLen;
	
	//	default name for the game is the one they passed in
	strncpy((char *) outConfig->nbpName, inGameName, kMaxGameNameLen);
	c2pstr((char *)outConfig->nbpName);

	//	default zone is "current zone"
	doCopyPStr(kDefaultZone, outConfig->nbpZone);
	
	//	default type is the game id (four character code turned into PStr
	outConfig->nbpType[0] = 4;
	machine_move_data((char *) &inGameID, (char *) &outConfig->nbpType[1], 4);

	//  default netSprocketMode
	outConfig->netSprocketMode = kDefaultNetSprocketMode;
	
	//	If we got a null string, just create a default config
	if (inConfigStr == NULL)
		return kNMNoError;
	 
	//Try_
	{	
	
		//	Generic module information

		//	Read the type
		tokenLen = sizeof (NMType);
		gotToken = get_token(inConfigStr, kConfigModuleType, LONG_DATA, &outConfig->type, &tokenLen);
		//op_warn(gotToken);

		if (!gotToken)
			outConfig->type = kModuleID;

		//	Read the version
		tokenLen = sizeof (NMUInt32);
		gotToken = get_token(inConfigStr, kConfigModuleVersion, LONG_DATA, &outConfig->version, &tokenLen);
		//op_warn(gotToken);

		if (!gotToken)
			outConfig->version = kVersion;

		//	Read the game id
		tokenLen = sizeof (NMUInt32);
		gotToken = get_token(inConfigStr, kConfigGameID, LONG_DATA, &outConfig->gameID, &tokenLen);
		//op_warn(gotToken);

		if (!gotToken)
			outConfig->gameID = 0;

		//	Read the game name
		//	jwo 9/30/97: the input param overrides this, so only use the config if there's no input param
		if (inGameName == NULL || inGameName[0] == 0)
		{
			tokenLen = kMaxGameNameLen;
			gotToken = get_token(inConfigStr, kConfigGameName, STRING_DATA, &outConfig->name, &tokenLen);
			//op_warn(gotToken);

			if (!gotToken)
				strcpy(outConfig->name, "unknown");
		}

		//	Read the connection mode
		tokenLen = sizeof (NMUInt32);
		gotToken = get_token(inConfigStr, kConfigEndpointMode, LONG_DATA, &outConfig->connectionMode, &tokenLen);
		//op_warn(gotToken);

		if (!gotToken)
			outConfig->connectionMode = kNMNormalMode;

		//	Read the netSprocketMode mode
		tokenLen = sizeof (NMUInt32);
		gotToken = get_token(inConfigStr, kConfigNetSprocketMode, LONG_DATA, &outConfig->netSprocketMode, &tokenLen);
		//op_warn(gotToken);

		if (!gotToken)
			outConfig->netSprocketMode = kDefaultNetSprocketMode;	
			

		//	read the custom data, if any
		//	jwo 9/30/97: the input param overrides this, so only use the config if there's no input param
		if (inDataLen == 0)
		{
			tokenLen = kNMEnumDataLen;
			gotToken = get_token(inConfigStr, kConfigCustomData, BINARY_DATA, &outConfig->customEnumData, &tokenLen);

			if (gotToken)		
				outConfig->customEnumDataLen = tokenLen;
		}

		//	Now read the protocol specific stuff
		
		//	NB: We assume that get_token doesn't change the string buffer if the token isn't found!
		//	Read the NBP name
		tokenLen = sizeof (Str32);
		gotToken = get_token(inConfigStr, kATNBPName, STRING_DATA, (char *)&outConfig->nbpName, &tokenLen);

		if (gotToken)
			c2pstr((char *) outConfig->nbpName);
		
		//	Read the NBP type
		tokenLen = sizeof (Str32);
		gotToken = get_token(inConfigStr, kATNBPType, STRING_DATA, (char *)&outConfig->nbpType, &tokenLen);
		//op_warn(gotToken);

		if (gotToken)
			c2pstr((char *) outConfig->nbpType);
		
		//	Read the NBP zone
		tokenLen = sizeof (Str32);
		gotToken = get_token(inConfigStr, kATNBPZone, STRING_DATA, (char *)&outConfig->nbpZone, &tokenLen);
		//op_warn(gotToken);

		if (gotToken)
			c2pstr((char *) outConfig->nbpZone);
		
		return kNMNoError;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// MakeNewATEndpointPriv
//----------------------------------------------------------------------------------------

NMErr
MakeNewATEndpointPriv(
	NMConfigRef					inConfig,
	NMEndpointCallbackFunction	*inCallback, 
	void						*inContext,
	NMUInt32					inMode,
	NMBoolean					inNetSprocketMode,
	NMATEndpointPriv			**theEP)
{
	Endpoint			*ep = NULL;
	NMATConfigPriv		*config = (NMATConfigPriv *) inConfig;
	NMUInt32			mode = (config == NULL) ? inMode : config->connectionMode;
	NMErr 				status = kNMNoError;

	*theEP = new NMATEndpointPriv;
	if (*theEP == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	ep = new OTATEndpoint((NMEndpointRef) *theEP, mode);
	if (ep == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}
	
	
	//  Set NetSprocket mode
	ep->mNetSprocketMode = (config == NULL) ? inNetSprocketMode : config->netSprocketMode;

	//	Install the user's callback function
	status = ep->InstallCallback(inCallback, inContext);
	if (status)
		goto error;
	
	//	Allow the endpoint to do one-time init stuff
	status = ep->Initialize(inConfig);
	if (status)
		goto error;

	(*theEP)->ep = ep;
	(*theEP)->id = kModuleID;

error:
	if (status)
	{
		delete *theEP;
		delete ep;
		
		*theEP = NULL;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// DisposeATEndpointPriv
//----------------------------------------------------------------------------------------

void
DisposeATEndpointPriv(NMATEndpointPriv *inEP)
{
	if (! inEP)
		return;
		
	delete inEP->ep;	// Delete the AT endpoint
	delete inEP; 		// delete user's version
}

//----------------------------------------------------------------------------------------
// CloseEPComplete
//----------------------------------------------------------------------------------------

// Just disposes of the endpoint.  The user has already been notified
void
CloseEPComplete(NMEndpointRef inEPRef)
{
	DisposeATEndpointPriv((NMATEndpointPriv *)inEPRef);
}

#pragma mark === Passthrough handlers ===

//----------------------------------------------------------------------------------------
// DoPassThrough
//----------------------------------------------------------------------------------------

NMErr
DoPassThrough(
	NMEndpointRef	inEndpoint,
	NMType			inSelector,
	void			*inParamBlock)
{
NMErr	err;
	
	UNUSED_PARAMETER(inEndpoint);
	
	switch (inSelector)
	{
		case kSetLookupMethodSelector:
			err = InternalSetLookupMethod(((SetLookupMethodPB *)inParamBlock)->method);
			break;
			
		case kSetSearchZonesSelector:
			err = InternalSetSearchZones(((SetSearchZonesPB *)inParamBlock)->zones);
			break;
			
		case kGetZoneListSelector:
			err = InternalGetZoneList(&((GetZoneListPB *)inParamBlock)->zones,
			                           ((GetZoneListPB *)inParamBlock)->timeout);
			break;
			
		case kDisposeZoneListSelector:
			err = InternalDisposeZoneList(((GetZoneListPB *) inParamBlock)->zones);

			if (kNMNoError == err)
				((GetZoneListPB *) inParamBlock)->zones = 0;
			break;

		case kGetMyZoneSelector:
			err = InternalGetMyZone(((GetMyZonePB *) inParamBlock)->zoneName);
			break;

		default:
			err = kNMUnknownPassThrough;
			DEBUG_PRINT("Unknown passthrough: %d.", inSelector);
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// InternalSetLookupMethod
//----------------------------------------------------------------------------------------

NMErr
InternalSetLookupMethod(NMSInt32 method)
{
	gLookupSpecifer = method;
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// InternalSetSearchZone
//----------------------------------------------------------------------------------------

NMErr
InternalSetSearchZone(unsigned char *zone)
{
NMUInt32		zoneLength = zone[0]+1;
NMUInt32		totalLength = zoneLength+1;
unsigned char	*storeP = new unsigned char[totalLength];

	if (0 == storeP)
		return memFullErr;

	machine_move_data(zone,storeP,zoneLength);
	storeP[zoneLength] = 0;		// second terminator

	if (gZoneSearchList)
		delete [] gZoneSearchList;

	gZoneSearchList = storeP;

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// InternalSetSearchZones
//----------------------------------------------------------------------------------------

NMErr
InternalSetSearchZones(unsigned char *zones)
{
NMUInt8		*zoneSkip = zones;

	while (zoneSkip[0])
	{
		// skip a string
		zoneSkip += zoneSkip[0] + 1;
	}

	// zoneSkip now points at last char in info to keep
	NMUInt32	totalLength = zoneSkip - zones + 1;

	unsigned char	*storeP = new unsigned char[totalLength];

	if (0 == storeP)
		return memFullErr;

	machine_move_data(zones,storeP,totalLength);

	if (gZoneSearchList)
		delete [] gZoneSearchList;

	gZoneSearchList = storeP;

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// ZLNotifier
//----------------------------------------------------------------------------------------

pascal void
ZLNotifier(void* contextPtr, ATEventCode code, NMErr result, void* cookie)
{
	UNUSED_PARAMETER(contextPtr);
	UNUSED_PARAMETER(result);
	UNUSED_PARAMETER(cookie);

	if (code == AT_ZONESLIST_COMPLETE || code == AT_UNEXPECTED_EVENT)
		gzlDone = true;
}

//----------------------------------------------------------------------------------------
// InternalGetZoneList
//----------------------------------------------------------------------------------------

NMErr
InternalGetZoneList(unsigned char **toZones, NMUInt32 timeout)
{
NMErr		err = kNMNoError;
NMUInt32		count;
StringPtr	outString = NULL;
StringPtr	p;
NMBoolean	allFound;

	timeout = kDefaultATTimeout;

	COTZonesEnumerator *enumerator = new COTZonesEnumerator();

	if (! enumerator)
	{
		err = kNMOutOfMemoryErr;
	}
	else
	{
		gzlDone = false;
		err = enumerator->Initialize(kATAppleTalkDefaultPortRef, gATZLNotifierUPP, NULL);

		if (err == kNMNoError)
		{
		UnsignedWide	startTime;
		NMUInt32			elapsedMilliseconds;
			
			OTGetTimeStamp(&startTime); 

			while (!gzlDone) 
			{
				elapsedMilliseconds = OTElapsedMilliseconds(&startTime);

				if (elapsedMilliseconds > timeout)
					break;
			};

			enumerator->GetCount(&allFound, &count);

			if (! allFound)
			{
				err = kNMTimeoutErr;
				op_vpause("Timed out before all zones were found!");
			}

			enumerator->Sort();

			// Zones are not actually String32's, since they can contain 32 characters
			// plus the length bytes!
			outString = (StringPtr) new char[sizeof (Str32) * count + 1];

			if (! outString)
			{
				err = kNMOutOfMemoryErr;
			}
			else
			{
				p = outString;

				for (OneBasedIndex i = 1; i <= count; i++)
				{
					enumerator->GetIndexedZone(i, p);
					p += *p + 1;
				}

				*p = 0;
			}
		}

		delete enumerator;
	}

	*toZones = outString;
	return err;
}

//----------------------------------------------------------------------------------------
// InternalDisposeZoneList
//----------------------------------------------------------------------------------------

NMErr
InternalDisposeZoneList(unsigned char *inPB)
{
	if (inPB)
		delete [] inPB;

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// InternalGetMyZone
//----------------------------------------------------------------------------------------

NMErr
InternalGetMyZone(unsigned char *zoneStore)
{
NMErr status;
	
	COTZonesEnumerator *enumerator = new COTZonesEnumerator();

	if (! enumerator)
		return kNMOutOfMemoryErr;

	// sjb need to initialize zone enumerator here or we get nothing.
	enumerator->Initialize(kATAppleTalkDefaultPortRef, 0, 0);

	status = ATGetMachineZone((ATZonesEnumeratorRef) enumerator, zoneStore);

	delete enumerator;
	
	return status;
}
