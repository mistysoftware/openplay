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

#ifndef __NSPGAMEMASTER__
#define __NSPGAMEMASTER__


//	------------------------------	Includes


	#include "NSpGame.h"
	#ifndef __NETMODULE__
	#include 			"NetModule.h"
	#endif

	#ifdef OP_API_NETWORK_OT
		#include <OpenTptInternet.h>
	#endif

	#include "CATEndpoint_OP.h"
	#include "CIPEndpoint_OP.h"

	#include "NSpProtocolRef.h"

//	------------------------------	Public Types



	class NSpGameMaster : public NSpGame
	{
	public:
		enum { class_ID = 'Mstr' };
		enum { kLowPriorityPollFrequency = 10};
		
		NSpGameMaster(NMUInt32 inMaxPlayers, NMConstStr31Param inGameName, NMConstStr31Param inPassword, NSpTopology inTopology, NSpFlags inFlags);

		virtual		~NSpGameMaster();

	//	Methods for advertising and unadvertising
		NMErr		HostAT(NSpProtocolPriv *inProt);
		NMErr		HostIP(NSpProtocolPriv *inProt);
		NMErr		UnHostAT(void);
		NMErr		UnHostIP(void);

		NMErr		AddLocalPlayer(NMConstStr31Param inPlayerName, NSpPlayerType inPlayerType, NSpProtocolPriv *theProt);
		NMErr		FreePlayerAddress(void **outAddress);
		NMErr		GetPlayerIPAddress(const NSpPlayerID inPlayerID, char **outAddress);

#if OP_API_NETWORK_OT
		NMErr		GetPlayerAddress(const NSpPlayerID inPlayerID, OTAddress **outAddress);
#endif

		NMErr		ChangePlayerType(const NSpPlayerID inPlayerID, const NSpPlayerType inNewType);
		NMErr		ForceRemovePlayer(const NSpPlayerID inPlayerID);

	//	Methods for handling events
					NMErr		InstallJoinRequestHandler(NSpJoinRequestHandlerProcPtr inHandler, void *inContext);
		virtual	NMErr		HandleEndpointDisconnected(CEndpoint *inEndpoint);
		virtual	void		IdleEndpoints(void);
		
	//	Methods for sending data
		virtual NMErr		SendUserMessage(NSpMessageHeader *inMessage, NSpFlags inFlags);
		virtual NMErr		SendSystemMessage(NSpMessageHeader *inMessage, NSpFlags inFlags);
		virtual NMErr		SendTo(NSpPlayerID inTo, NMSInt32 inWhat, void *inData, NMUInt32 inLen, NSpFlags inFlags);
		
	//	Methods for handing incoming data
		virtual	void		HandleEvent(ERObject *inERObject, CEndpoint *inEndpoint, void *inCookie);
		virtual	void		HandleNewEvent(ERObject *inERObject, CEndpoint *inEndpoint, void *inCookie);
		virtual	NMErr		PrepareForDeletion(NSpFlags inFlags);
					NMUInt32 GetBacklog( void );

	//	Accessors
		inline	void		SetMaxRTT(NMUInt32 inMaxRTT) { mMaxRTT = inMaxRTT;}
		inline	NMUInt32	GetMaxRTT(void)	{return mMaxRTT;}
		
		inline	NMUInt32	GetProtocols(void) { return mProtocols;}
		inline	void		SetProtocols(NMUInt32 inProtocols) { mProtocols = inProtocols;};
		
	protected:
	//	Methods for handling system requests
		virtual	NMBoolean	RemovePlayer(NSpPlayerID inPlayer, NMBoolean inDisconnect);

		NMBoolean 	ProcessSystemMessage(NSpMessageHeader *inMessage, NMBoolean *doForward);
		NMBoolean	HandleJoinRequest(NSpJoinRequestMessage *inMessage, CEndpoint *inEndpoint, void *inCookie, NMUInt32 inReceivedTime);	
		NMErr			MakeJoinApprovedMessage(TJoinApprovedMessagePrivate **theMessage, NSpPlayerEnumerationPtr thePlayers,
													NSpGroupEnumerationPtr theGroups, NSpPlayerID inPlayer, NMUInt32 inReceivedTime);
		NMBoolean	IsCorrectPassword(const NMUInt8 *inPassword);
		NMErr			SendJoinApproved(CEndpoint *inEndpoint, NSpPlayerID inID, NMUInt32 inReceivedTime);
		NMErr			SendJoinDenied(CEndpoint *inEndpoint, void *inCookie, const NMUInt8 *inMessage);		
		NMErr			SendPauseGame(void);
		NMErr			SendBecomeHostRequest(void);
		NMErr			NotifyPlayerJoined(NSpPlayerInfo *info);
		NMErr			ForwardMessage(NSpMessageHeader *inMessage);
		NMErr			RouteMessage(NSpMessageHeader *inHeader, NMUInt8 *inBody, NSpFlags inFlags);
		NMBoolean	NegotiateNewHost(void);	
		NMErr			SendJoinRequest(NMConstStr31Param inPlayerName, NSpPlayerType inType);

		NSpPlayerID	mNextAvailablePlayerNumber;
		NSpGroupID	mNextPlayersGroupStartRange;
			
		NMUInt32		mMaxRTT;			// Max round-trip-time for a prospective player

		NMBoolean	bPasswordRequired;	// Flag specifying whether or not a password is required
		NMBoolean	bHeadlessServer;
		
		COTATEndpoint	*mAdvertisingATEndpoint;	 // AppleTalk endpoint we use for serving
		NMBoolean		bAdvertisingAT;
		
		COTIPEndpoint	*mAdvertisingIPEndpoint;	 // TCP/IP endpoint we use for serving
		NMBoolean		bAdvertisingIP;
		
		CEndpoint		*mPlayersEndpoint;			// The endpoint used when the player wants to send something
		
		NMUInt32		mLowPriorityPollCount;
		
		NSp_InterruptSafeListIterator	*mSystemPlayerIterator;
		NSp_InterruptSafeListIterator	*mSystemGroupIterator;

	//	Callback cruft
		NSpJoinRequestHandlerProcPtr	mJoinRequestHandler;
		void			*mJoinRequestContext;
		NMUInt32		mProtocols;	// a bit array of the protocols this host is using
	};


#endif // __NSPGAMEMASTER__

