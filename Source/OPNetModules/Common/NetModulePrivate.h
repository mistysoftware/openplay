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

#ifndef __NETMODULEPRIVATE__
#define __NETMODULEPRIVATE__
	
	
/*	------------------------------	Includes */
	#include "NetModule.h"


/*	------------------------------	Public Definitions */


	

/*	------------------------------	Public Types */

	enum
	{
		kNMGetModuleInfo			= 0,

		/* Endpoint functions */
		kNMOpen,
		kNMClose,
		
		kNMAcceptConnection,
		kNMRejectConnection,

		kNMIsAlive,
		kNMFreeAddress,
		kNMGetAddress,
		kNMSetTimeout,

		kNMCreateConfig,
		kNMGetConfigLen,
		kNMGetConfig,
		kNMDeleteConfig,

		kNMIdle,
		kNMFunctionPassThrough,

		/* Data functions */
		kNMSendDatagram,
		kNMReceiveDatagram,
		kNMSend,
		kNMReceive,

		kNMGetRequiredDialogFrame,
		kNMSetupDialog,
		kNMHandleEvent,
		kNMHandleItemHit,
		kNMTeardownDialog,

		kNMBindEnumerationItemToConfig,
		kNMStartEnumeration,
		kNMIdleEnumeration,
		kNMEndEnumeration,
		kNMStopAdvertising,
		kNMStartAdvertising,

		kNMEnterNotifier,
		kNMLeaveNotifier,

		kNMGetIdentifier,
        
		NUMBER_OF_MODULE_FUNCTIONS
	};

	#ifdef INCLUDE_MODULE_NAMES

		const char	*module_names[] =
		{
			"NMGetModuleInfo",

			/* Endpoint functions */
			"NMOpen",
			"NMClose",
			
			"NMAcceptConnection",
			"NMRejectConnection",

			"NMIsAlive",
			"NMFreeAddress",
			"NMGetAddress",
			"NMSetTimeout",

			"NMCreateConfig",
			"NMGetConfigLen",
			"NMGetConfig",
			"NMDeleteConfig",

			"NMIdle",
			"NMFunctionPassThrough",

			/* Data functions */
			"NMSendDatagram",
			"NMReceiveDatagram",
			"NMSend",
			"NMReceive",
			
			"NMGetRequiredDialogFrame",
			"NMSetupDialog",
			"NMHandleEvent",
			"NMHandleItemHit",
			"NMTeardownDialog",

			"NMBindEnumerationItemToConfig",
			"NMStartEnumeration",
			"NMIdleEnumeration",
			"NMEndEnumeration",
			"NMStopAdvertising",
			"NMStartAdvertising",
			"NMEnterNotifier",
			"NMLeaveNotifier",
			
			"NMGetIdentifier"
		};

		#define NUMBER_OF_MODULES_TO_LOAD (sizeof (module_names) / sizeof (module_names[0]))

	#endif // INCLUDE_MODULE_NAMES

#ifdef __cplusplus
extern "C" {
#endif

	/* Module information */
	typedef NMErr 		(*NMGetModuleInfoPtr)(NMModuleInfo *outInfo);

	/* Endpoint functions */
	typedef NMErr 		(*NMOpenPtr)(NMConfigRef inConfig, NMEndpointCallbackFunction *inCallback, void *inContext, NMEndpointRef *outEndpoint, NMBoolean inActive);
	typedef NMErr		(*NMClosePtr)(NMEndpointRef inEndpoint, NMBoolean inOrderly);

	typedef NMErr		(*NMAcceptConnectionPtr)(NMEndpointRef inEndpoint, void *inCookie, NMEndpointCallbackFunction *inCallback, void *inContext);
	typedef NMErr		(*NMRejectConnectionPtr)(NMEndpointRef inEndpoint, void *inCookie);

	typedef NMBoolean	(*NMIsAlivePtr)(NMEndpointRef inEndpoint);

	typedef NMErr		(*NMFreeAddressPtr)(NMEndpointRef inEndpoint, void **outAddress);
	typedef NMErr		(*NMGetAddressPtr)(NMEndpointRef inEndpoint, NMAddressType addressType, void **outAddress);

	typedef NMErr		(*NMSetTimeoutPtr)(NMEndpointRef inEndpoint, NMUInt32 inTimeout);	/* in milliseconds */

	typedef NMErr		(*NMCreateConfigPtr)(char *inConfigStr, NMType inGameID, const char *inGameName, const void *inEnumData, NMUInt32 inEnumDataLen, NMConfigRef *outConfig);
	typedef NMSInt16	(*NMGetConfigLenPtr)(NMConfigRef inConfig);
	typedef NMErr		(*NMGetConfigPtr)(NMConfigRef inConfig, char *outConfigStr, NMSInt16 *ioConfigStrLen);
	typedef NMErr		(*NMDeleteConfigPtr)(NMConfigRef inConfig);

	typedef NMErr		(*NMBindEnumerationItemToConfigPtr)(NMConfigRef inConfig, NMType inID);
	typedef NMErr		(*NMStartEnumerationPtr)(NMConfigRef inConfig, NMEnumerationCallbackPtr inCallback, void *inContext, NMBoolean inActive);
	typedef NMErr		(*NMIdleEnumerationPtr)(NMConfigRef inConfig);
	typedef NMErr		(*NMEndEnumerationPtr)(NMConfigRef inConfig);
	typedef void		(*NMStopAdvertisingPtr)(NMEndpointRef inEndpoint);	// [Edmark/PBE] 11/8/99 changed parameter from void
	typedef void		(*NMStartAdvertisingPtr)(NMEndpointRef inEndpoint);	// [Edmark/PBE] 11/8/99 changed parameter from void

	typedef NMErr		(*NMIdlePtr)(NMEndpointRef inEndpoint);
	typedef NMErr		(*NMFunctionPassThroughPtr)(NMEndpointRef inEndpoint, NMUInt32 inSelector, void *inParamBlock);

	typedef NMErr       (*NMGetIdentifierPtr)(NMEndpointRef inEndpoint, char* outIdStr, NMSInt16 inMaxLen);

	/* Data functions */
	typedef NMErr		(*NMSendDatagramPtr)(NMEndpointRef inEndpoint, unsigned char *inData, NMUInt32 inSize, NMFlags inFlags);
	typedef NMErr		(*NMReceiveDatagramPtr)(NMEndpointRef inEndpoint, unsigned char * outData, NMUInt32 *outSize, NMFlags *outFlags);
	typedef NMErr		(*NMSendPtr)(NMEndpointRef inEndpoint, void *inData, NMUInt32 inSize, NMFlags inFlags);
	typedef NMErr		(*NMReceivePtr)(NMEndpointRef inEndpoint, void *outData, NMUInt32 *ioSize, NMFlags *outFlags);

	/* Enter/Leave Notifier Functions */
	typedef NMErr		(*NMEnterNotifierPtr)(NMEndpointRef inEndpoint, NMEndpointMode endpointMode);
	typedef NMErr		(*NMLeaveNotifierPtr)(NMEndpointRef inEndpoint, NMEndpointMode endpointMode);

	/* Dialog functions */
	typedef void		(*NMGetRequiredDialogFramePtr)(NMRect *r, NMConfigRef config);
	typedef NMErr 		(*NMSetupDialogPtr)(NMDialogPtr dialog, short frame, short inBaseItem, NMConfigRef config);
	typedef NMBoolean	(*NMHandleEventPtr)(NMDialogPtr dialog, NMEvent *event, NMConfigRef config);
	typedef NMErr		(*NMHandleItemHitPtr)(NMDialogPtr dialog, short inItemHit, NMConfigRef config);
	typedef NMBoolean	(*NMTeardownDialogPtr)(NMDialogPtr dialog, NMBoolean inUpdateConfig, NMConfigRef config);

#ifdef __cplusplus
}
#endif

/*	------------------------------	Public Variables */
/*	------------------------------	Public Functions */

#endif


