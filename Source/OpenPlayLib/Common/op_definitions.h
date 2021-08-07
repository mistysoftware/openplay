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

#ifndef __OP_DEFINITIONS__
#define __OP_DEFINITIONS__

//	------------------------------	Includes

	#ifndef __NETMODULE__
	#include 			"NetModule.h"
	#endif
	#ifndef __NETMODULEPRIVATE__
	#include 			"NetModulePrivate.h"
	#endif

//	------------------------------	Public Types

#ifdef OP_API_PLUGIN_MAC_CFM
	typedef CFragConnectionID	ConnectionRef;
#elif defined(OP_API_PLUGIN_MACHO)
		typedef CFBundleRef		ConnectionRef;
#elif defined(OP_API_PLUGIN_WINDOWS)
	typedef	HINSTANCE		ConnectionRef; /* unknown for now */
#elif defined(OP_API_PLUGIN_POSIX) || defined(OP_API_PLUGIN_POSIX_DARWIN)
		typedef NMSInt32		ConnectionRef;
#else
	#error "Porting Error - need ConnectionRef type."
#endif

	struct ProtocolConfig
	{
		NMType type;
		void *configuration_data;
		NMConfigRef NMConfig;

		NMCreateConfigPtr NMCreateConfig;
		NMGetConfigLenPtr NMGetConfigLen;
		NMGetConfigPtr NMGetConfig;
		NMDeleteConfigPtr NMDeleteConfig;
		NMFunctionPassThroughPtr NMFunctionPassThrough;

		/* Dialog.. */
		NMGetRequiredDialogFramePtr NMGetRequiredDialogFrame;
		NMSetupDialogPtr NMSetupDialog;
		NMHandleEventPtr NMHandleEvent;
		NMHandleItemHitPtr NMHandleItemHit;
		NMTeardownDialogPtr NMTeardownDialog;

		/* binding the enumeration stuff. */
		NMBindEnumerationItemToConfigPtr NMBindEnumerationItemToConfig;
		NMStartEnumerationPtr NMStartEnumeration;
		NMIdleEnumerationPtr NMIdleEnumeration;
		NMEndEnumerationPtr NMEndEnumeration;
	//	NMStopAdvertisingPtr NMStopAdvertising;		// [Edmark/PBE] 11/8/99 moved to Endpoint 
	//	NMStartAdvertisingPtr NMStartAdvertising;	// [Edmark/PBE] 11/8/99 moved to Endpoint 

		FileDesc file;
		ConnectionRef connection;
	};
	typedef struct ProtocolConfig ProtocolConfig;

	struct callback_data
	{
		PEndpointCallbackFunction user_callback;
		void *user_context;
	};

	enum
	{ /* only used for freeing up the library */
		_state_unknown,
		_state_closing,
		_state_closed
	};

	struct Endpoint 
	{
		NMUInt32					cookie;
		NMType						type;
		NMSInt16					state;
		NMOpenFlags					openFlags;

		/* Data */
		ConnectionRef				connection;
		FileDesc					library_file; /* where the code is.. */
		
		/* Parent (NULL if none) */
		Endpoint			*		parent;
		
		/* Callback data */
		callback_data				callback;

		/* Private module specific data */
		NMEndpointRef				module;
		
		/* Function pointers for standard stuff */
		NMGetModuleInfoPtr			NMGetModuleInfo;

		NMGetConfigLenPtr			NMGetConfigLen;
		NMGetConfigPtr				NMGetConfig;

		NMOpenPtr					NMOpen;
		NMClosePtr					NMClose;

		NMAcceptConnectionPtr		NMAcceptConnection;
		NMRejectConnectionPtr		NMRejectConnection;

		NMSendDatagramPtr			NMSendDatagram;
		NMReceiveDatagramPtr		NMReceiveDatagram;

		NMSendPtr					NMSend;
		NMReceivePtr				NMReceive;

		NMEnterNotifierPtr			NMEnterNotifier;
		NMLeaveNotifierPtr			NMLeaveNotifier;
		
		NMSetTimeoutPtr				NMSetTimeout;
		NMIsAlivePtr				NMIsAlive;
		NMFreeAddressPtr			NMFreeAddress;
		NMGetAddressPtr				NMGetAddress;
		NMFunctionPassThroughPtr	NMFunctionPassThrough;
		NMIdlePtr					NMIdle;

		NMStopAdvertisingPtr 		NMStopAdvertising;
		NMStartAdvertisingPtr 		NMStartAdvertising;

		NMGetIdentifierPtr          NMGetIdentifier;

		Endpoint *					next;
	};
	typedef struct Endpoint Endpoint;

//	------------------------------	Public Functions

	extern void op_idle_synchronous(void);
	extern void refresh_protocols(void);
	extern void shutdown_report(void);

#endif // __OP_DEFINITIONS__
