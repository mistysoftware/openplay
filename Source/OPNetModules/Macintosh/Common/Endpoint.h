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

#ifndef __ENDPOINT__
#define __ENDPOINT__

//	------------------------------	Includes

	#include "NetModule.h"

//	------------------------------	Public Definitions
//	------------------------------	Public Types

	//enum
	//{
	//	kUnreliableMode = 0x00000001,
	//	kReliableMode = 0x00000002,
	//	kNormalMode = kUnreliableMode + kReliableMode
	//};

	class Endpoint
	{
	public:
				Endpoint(NMEndpointRef inRef, NMUInt32 inMode = kNMNormalMode);
		virtual ~Endpoint();

	typedef enum
	{
		kOpening = 1, 
		kListening,
		kConnecting,
		kHandingOff,
		kRunning,
		kClosing,
		kAborting,
		kDead
	} MyEPState;


		virtual NMErr		Initialize(NMConfigRef inConfig) = 0;
		virtual NMErr		Listen(void) = 0;
		virtual	NMErr		AcceptConnection(Endpoint *inNewEP, void *inCookie) = 0;
		virtual NMErr		RejectConnection(void *inCookie) = 0;
		virtual	void		Close(void) = 0;
		virtual NMErr		Connect(void) = 0;
		virtual NMBoolean	IsAlive(void) = 0;
		virtual NMErr		Idle(void) = 0;
		virtual	NMErr		FunctionPassThrough(NMType inSelector, void *inParamBlock) = 0;
		virtual NMErr		SendDatagram(const NMUInt8 * inData, NMUInt32 inSize, NMFlags inFlags) = 0;
		virtual NMErr		ReceiveDatagram(NMUInt8 * outData, NMUInt32 *outSize, NMFlags *outFlags) = 0;
		virtual NMSInt32	Send(const void *inData, NMUInt32 inSize, NMFlags inFlags) = 0;
		virtual NMErr		Receive(void *outData, NMUInt32 *ioSize, NMFlags *outFlags) = 0;
		
		virtual NMErr       GetIdentifier(char* outIdStr, NMSInt16 inMaxSize) = 0;

		virtual NMErr		EnterNotifier(NMUInt32	endpointMode) = 0;
		virtual NMErr		LeaveNotifier(NMUInt32	endpointMode) = 0;

				NMErr		InstallCallback(NMEndpointCallbackFunction *inCallback, void *inContext);

		inline	void		SetTimeout(NMUInt32 inTimeout) {mTimeout = inTimeout;}
		inline 	NMUInt32	GetTimeout(void) {return mTimeout;}

				NMBoolean	SetQueryForwarding(NMBoolean whatTo);
		inline 	NMBoolean	DoingQueryForwarding(void) {return mAreForwardingQueries;};

		NMEndpointRef				mRef;
		NMEndpointCallbackFunction *mCallback;
		void						*mContext;

		NMUInt32						mMode;
		NMBoolean					mNetSprocketMode;
		MyEPState					mState;	
		
	protected:
		NMUInt32						mTimeout;	// in milliseconds

	private:
		NMBoolean					mAreForwardingQueries;
	};

#endif	// __ENDPOINT__

