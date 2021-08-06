/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
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

#ifndef __NETMODULE__
#define __NETMODULE__

//	------------------------------	Includes

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif


	#ifdef	OP_PLATFORM_MAC_CFM
	
		#define OP_API_NETWORK_OT				1
		#define OP_API_PLUGIN_MAC_CFM			1

	#elif defined(OP_PLATFORM_MAC_MACHO)
	
		#define OP_API_NETWORK_SOCKETS			1
		#define OP_API_PLUGIN_MACHO				1


	#elif defined(OP_PLATFORM_WINDOWS)
	
		#define OP_API_NETWORK_WINSOCK			1
		#define OP_API_PLUGIN_WINDOWS			1


	#elif defined(OP_PLATFORM_UNIX)
	
		#define OP_API_NETWORK_SOCKETS			1
		
		#if defined(os_darwin)
			#define OP_API_PLUGIN_POSIX			1
		#else
			#define OP_API_PLUGIN_POSIX_DARWIN	1
		#endif

	#else
		#error "undefined platform"
	#endif

//	------------------------------	Public Definitions

	#if defined(OP_PLATFORM_UNIX)
		#define PENDPOINT_COOKIE      0x4f504c59  /* hex for "OPLY" */
		#define PENDPOINT_BAD_COOKIE  0x62616420  /* hex for "bad " */
	#else
		#define PENDPOINT_COOKIE      'OPLY'
		#define PENDPOINT_BAD_COOKIE  'bad '
	#endif

//is NetSprocketMode enabled for endpoints by default?
#define kDefaultNetSprocketMode false

//	------------------------------	Public Types


	typedef 	struct NMProtocolConfigPriv	*	NMConfigRef;
	typedef  	struct NMEndpointPriv		*	NMEndpointRef;

	typedef void 
	(NMEndpointCallbackFunction)(	NMEndpointRef	 			inEndpoint, 
									void* 						inContext,
									NMCallbackCode				inCode, 
									NMErr		 				inError, 
									void* 						inCookie);

//	------------------------------	Public Variables

#ifdef OP_PLATFORM_WINDOWS
		extern HINSTANCE 	application_instance;
#endif

//	------------------------------	Public Functions


#ifdef __cplusplus
extern "C" {
#endif

	/* Module initialization */

		OP_DEFINE_API_C(NMErr)
		NMGetModuleInfo		(	NMModuleInfo *		outInfo	);

	/* Endpoint functions */

		OP_DEFINE_API_C(NMErr)
		NMOpen				(	NMConfigRef 		inConfig, 
								NMEndpointCallbackFunction *	inCallback, 
								void *				inContext, 
								NMEndpointRef *		outEndpoint, 
								NMBoolean			inActive);

		OP_DEFINE_API_C(NMErr)
		NMClose				(	NMEndpointRef		inEndpoint,
								NMBoolean			inOrderly);

	/*	Do not call this function from within your callback handler! */

		OP_DEFINE_API_C(NMErr)
		NMAcceptConnection	(	NMEndpointRef		inEndpoint,
								void *				inCookie, 
								NMEndpointCallbackFunction *	inCallback, 
								void *				inContext);

		OP_DEFINE_API_C(NMErr)
		NMRejectConnection	(	NMEndpointRef 		inEndpoint, 
								void *				inCookie);

		OP_DEFINE_API_C(NMBoolean)
		NMIsAlive			(	NMEndpointRef		inEndpoint);

		OP_DEFINE_API_C(NMErr)
		NMFreeAddress		(	NMEndpointRef 		inEndpoint,
								void **				outAddress);

		OP_DEFINE_API_C(NMErr)
		NMGetAddress		(	NMEndpointRef 		inEndpoint,
								NMAddressType		addressType,
								void **				outAddress);

		OP_DEFINE_API_C(NMErr)
		NMSetTimeout		(	NMEndpointRef 		inEndpoint, 
								NMUInt32			inTimeout);	/* in milliseconds */

		OP_DEFINE_API_C(NMErr)
		NMGetIdentifier 	(	NMEndpointRef 		inEndpoint, 
								char *				outIdStr,
							    NMSInt16            inMaxLen);

	/* Enter/Leave Notifier functions */

		OP_DEFINE_API_C(NMErr)
		NMEnterNotifier		(	NMEndpointRef 		inEndpoint,
								NMEndpointMode		endpointMode);

		OP_DEFINE_API_C(NMErr)
		NMLeaveNotifier		(	NMEndpointRef		inEndpoint,
								NMEndpointMode		endpointMode);

	/* Configurator functions */

		OP_DEFINE_API_C(NMErr)
		NMCreateConfig		(			char *		inConfigStr, 
										NMType	inGameID,
								const	char *		inGameName,
								const	void *		inEnumData,
										NMUInt32	inDataLen,
										NMConfigRef *outConfig);
										
		OP_DEFINE_API_C(NMSInt16)
		NMGetConfigLen		(	NMConfigRef			inConfig);

		OP_DEFINE_API_C(NMErr)
		NMGetConfig			(	NMConfigRef			inConfig,
								char *				outConfigStr,
								NMSInt16 *			ioConfigStrLen);

		OP_DEFINE_API_C(NMErr)
		NMDeleteConfig		(	NMConfigRef			inConfig);

	/* Back door functions */

		OP_DEFINE_API_C(NMErr)
		NMIdle				(	NMEndpointRef		inEndpoint);

		OP_DEFINE_API_C(NMErr)
		NMFunctionPassThrough(	NMEndpointRef		inEndpoint,
								NMUInt32			inSelector,
								void *				inParamBlock);

	/* Data functions */

		OP_DEFINE_API_C(NMErr)
		NMSendDatagram(			NMEndpointRef 		inEndpoint, 
								NMUInt8 * 			inData, 
								NMUInt32			inSize, 
								NMFlags				inFlags);

		OP_DEFINE_API_C(NMErr)
		NMReceiveDatagram	(	NMEndpointRef		inEndpoint,
								NMUInt8 *			ioData,
								NMUInt32 *			ioSize,
								NMFlags *			outFlags);

		OP_DEFINE_API_C(NMErr)
		NMSend				(	NMEndpointRef 		inEndpoint, 
								void *				inData, 
								NMUInt32			inSize,
								NMFlags 			inFlags);

		OP_DEFINE_API_C(NMErr)
		NMReceive			(	NMEndpointRef		inEndpoint,
								void *				ioData, 
								NMUInt32 *			ioSize,
								NMFlags *			outFlags);

	/* Dialog functions */

		OP_DEFINE_API_C(void)
		NMGetRequiredDialogFrame(	NMRect *		r, 
									NMConfigRef 	inConfig);
		
		OP_DEFINE_API_C(NMErr)
		NMSetupDialog		(	NMDialogPtr 		dialog, 
								NMSInt16 			frame, 
								NMSInt16			inBaseItem, 
								NMConfigRef			inConfig);

		OP_DEFINE_API_C(NMBoolean)
		NMHandleEvent		(	NMDialogPtr			dialog, 
								NMEvent *			event, 
								NMConfigRef 		inConfig);
		
		OP_DEFINE_API_C(NMErr)
		NMHandleItemHit		(	NMDialogPtr			dialog, 
								NMSInt16			inItemHit, 
								NMConfigRef 		inConfig);
		
		OP_DEFINE_API_C(NMBoolean)
		NMTeardownDialog	(	NMDialogPtr 		dialog, 
								NMBoolean			inUpdateConfig, 
								NMConfigRef 		ioConfig);

	/* Enumeration functions */

		OP_DEFINE_API_C(NMErr)
		NMBindEnumerationItemToConfig(	NMConfigRef 	inConfig, 
										NMHostID 		inID);

		OP_DEFINE_API_C(NMErr)
		NMStartEnumeration	(	NMConfigRef 		inConfig, 
								NMEnumerationCallbackPtr inCallback, 
								void *				inContext, 
								NMBoolean 			inActive);

		OP_DEFINE_API_C(NMErr)
		NMIdleEnumeration	(	NMConfigRef 		inConfig);

		OP_DEFINE_API_C(NMErr)
		NMEndEnumeration	(	NMConfigRef 		inConfig);

		OP_DEFINE_API_C(NMBoolean)
		NMStartAdvertising	(	NMEndpointRef 		inEndpoint);

		OP_DEFINE_API_C(NMBoolean)
		NMStopAdvertising	(	NMEndpointRef 		inEndpoint);

#ifdef __cplusplus
}
#endif

#endif // __NETMODULE__

