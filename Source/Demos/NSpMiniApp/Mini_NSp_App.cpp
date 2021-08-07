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

// --------------------------------------------------------------
//
//  FILE:  Mini_NSp_App.cpp
// 
//  DESCRIPTION:  A stripped-down version of the NSp Test App,
//  designed to help walk NSp newcomers through the most basic  
//  steps needed to use NetSprocket.
//
// --------------------------------------------------------------

#include <time.h>
#include <stdio.h>
#include <iostream>

#include "OpenPlay.h"
#include "OPUtils.h"

using namespace std;  //introduces namespace std

const long  kMyMessageType = 42;
const int   kDataStrLen = 256;      /* Max packets to receive & store. */
const char  kCannedMessage[ kDataStrLen ] = "Who's turn is it to feed Clarus?\0";

#if TARGET_OS_MAC
  #define d_GAME_ID    'MOOF'
#else
  #define d_GAME_ID    0x4d4f4f46  /* hex for "MOOF" */
#endif


/* The sample structure that will be sent. */
typedef struct {

	NSpMessageHeader  header;
	char  dataStr[ kDataStrLen ];
	
} TestMessage;

typedef enum { kNone, kTCP, kAppleTalk } ProtocolType;


NMBoolean  gInServerMode = false;
ProtocolType  gProtocolSelected = kNone;
TestMessage  *gDataToSend = new TestMessage;
TestMessage  *gDataReceived = NULL;

NMStr31  NBPName = "\pDogCow";
NMStr31  NBPType = "\pNSpT";		// Must be a 4 character string.  CRT, July 2, 2000
NMStr31  kConstPlayerName = "\pClarus";
NMStr31  kGameName = "\pDogCow";
NMStr31  kPlayerName = "\pClarus";
NMStr31  kMyPassword = "\pMoof";

NSpPlayerType    kPlayerType = 0;
NSpPlayerID      kPlayerID = 0;

const NMInetPort  kTCP_Port = 25710;
const NMUInt32    kMaxPlayers = 16;


/* --------------  Function Prototypes  -------------- */

NMBoolean  Check_Error( const char* errMessage, NMErr error );


    /* --------------  Game Object Management -------------- */

NMBoolean  Host_Game( NSpGameReference  &gGameObject, 
                 NSpProtocolListReference  gProtocolListRef );
NMBoolean  Join_Game( NSpGameReference  &gGameObject, 
                 NSpAddressReference  gAddressRef );
NMBoolean  Join_Game_AT( NSpGameReference  &gGameObject);
NMBoolean  Join_Game_IP( NSpGameReference  &gGameObject);
NMBoolean  Wait_For_Approval(NSpGameReference  &gGameObject, char *theDenyReason);
NMBoolean  Dispose_Game( NSpGameReference  &gGameObject );

    /* --------------  Message Routines -------------- */

NMBoolean  Send_Message( NSpGameReference  gGameObject );
void     Get_Messages( NSpGameReference  gGameObject );
void     Handle_Message(NSpMessageHeader *messageHeaderPtr);
void     Release_Message( NSpGameReference  gGameObject, TestMessage  *message );


    /* --------------  Player Management -------------- */

NMBoolean  Get_Player_Info( NSpGameReference  gGameObject );
NMBoolean  Print_Players( NSpGameReference  gGameObject );


    /* --------------  Protocol Management -------------- */

NMBoolean  Create_Empty_Protocol_List( NSpProtocolListReference  &gProtocolListRef );
NMBoolean  Protocol_Create_AppleTalk( NSpProtocolReference &gProtocolRef );
NMBoolean  Protocol_Create_IP( NSpProtocolReference &gProtocolRef );
NMBoolean  Append_To_Protocol_List( NSpProtocolListReference  gProtocolListRef,
                                  NSpProtocolReference gProtocolRef );

    /* --------------  Utilities -------------- */

NMBoolean  Get_Game_Info( NSpGameReference  gGameObject );


    /* --------------  Higher Level Helpers -------------- */

void  Choose_Protocol( void );
NMBoolean  Create_GameObject( NSpGameReference &gGameObject, 
                            NSpProtocolReference &gProtocolRef,
                            NSpProtocolListReference gProtocolListRef,
                            NSpAddressReference gAddressRef );




/* --------------  Begin Code  -------------- */

int 
main( void ) 
{	

	NSpGameReference          gGameObject = NULL ;
	NSpProtocolReference      gProtocolRef = NULL;
	NSpProtocolListReference  gProtocolListRef = NULL;
	NSpAddressReference       gAddressRef = NULL;

	NSpGameInfo  *gGameInfo = NULL;

	char  command;
	NMBoolean done = false;
	NMBoolean success;
	
	NMErr error = NSpInitialize( 0, 0, 0, d_GAME_ID, 0 );
	success = Check_Error( "Error: Init_NetSprocket(), initing NSp.", error );
	
	if ( !success ) {
		cerr << "Exiting..." << endl;
		return( 1 );
	}
	
	cout << "NetSprocket initialized..." << endl;
	
	// Print version info
	NMNumVersion  v = NSpGetVersion();
	cout << endl << "NSp Version " 
	     << hex << (short) v.majorRev << "."
	     << hex << (short) v.minorAndBugRev << endl;

	Choose_Protocol(); 

	if ( !success ) {
		cerr << "Exiting..." << endl;
		return( 1 );
	}
	
	success = Create_GameObject( gGameObject, gProtocolRef, gProtocolListRef, gAddressRef ); 
	
	if ( !success ) {
		cerr << "Exiting..." << endl;
		return( 1 );
	}


	while ( !done ) {
	
		cout << endl << endl;
		cout << "Options:" << endl << endl;
		cout << "\t 1) Get Game Info" << endl;
		cout << "\t 2) Print Player List" << endl;
		cout << "\t 3) Send Message to Other Players" << endl;
		cout << "\t 4) Print Incoming Messages" << endl;
		cout << "\t 5) Quit" << endl << endl;
		cout << "\t Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '1':
				Get_Game_Info( gGameObject ); break;
				
			case '2':
				Print_Players( gGameObject ); break;
										
			case '3':
				Send_Message( gGameObject ); break;
										
			case '4':
				Get_Messages( gGameObject ); break;
								
			case '5':
				done = true; break;
				
			default:
				cerr << "Invalid entry." << endl;
		}
	}

	if ( gGameObject != NULL ) {
	
		Dispose_Game( gGameObject );
	}

	cout << endl << "Done." << endl;
	
	return( 0 );
}


NMBoolean
Check_Error( const char* errMessage, NMErr error )
{
	NMBoolean  noError = true;

	if ( error != kNMNoError ) {
		cerr << errMessage << endl;
		cerr << "\tError code = " << error << endl;
		noError = false;
	}
	
	return( noError );
}




void 
Choose_Protocol( void ) 
{
	NMBoolean  done = false;
	char  command;

	while ( !done ) {
	
		cout << endl << endl;
		cout << "Select a protocol:" << endl << endl;
		
		cout << "\t 1) Use TCP/IP" << endl;
#if TARGET_OS_MAC
		cout << "\t 2) Use Appletalk" << endl;
#endif
		cout << "\n\t Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '1':
				gProtocolSelected = kTCP;
				done = true; 
				break;

#if TARGET_OS_MAC
			case '2':
				gProtocolSelected = kAppleTalk;
				done = true; 
				break;
#endif

			default:
				cerr << "Invalid entry." << endl;
		}
		
	}
}



NMBoolean 
Create_GameObject(  NSpGameReference &gGameObject, 
                    NSpProtocolReference &gProtocolRef,
                    NSpProtocolListReference gProtocolListRef,
                    NSpAddressReference       gAddressRef )
{
	NMBoolean done = false;
	NMBoolean status = false;
	char  command;

	while ( !done ) {

		cout << endl << endl;
		cout << "Select Game Mode:" << endl << endl;

		cout << "\t 1) Host Game" << endl;
		cout << "\t 2) Join Game" << endl;
		cout << "\n\t Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '1':
			
				cout << "\nStarting up server..." << endl;
				
				Create_Empty_Protocol_List( gProtocolListRef );

				if ( gProtocolSelected == kAppleTalk ) {
				
					status = Protocol_Create_AppleTalk( gProtocolRef );
				}
				else {

					status = Protocol_Create_IP( gProtocolRef );
				}
				
				if ( status ) {

					Append_To_Protocol_List( gProtocolListRef, gProtocolRef );

					status = Host_Game( gGameObject, gProtocolListRef );
				}
				
				if ( status ) {
					cout << "Server active." << endl;
				}

				done = true; 
				break;


			case '2':
			
				if ( gProtocolSelected == kAppleTalk ) {
				
					status = Join_Game_AT( gGameObject );
				}		
				else if ( gProtocolSelected == kTCP ) {
				
					status = Join_Game_IP( gGameObject ); 
				}
				else {
				
					cerr << "Error: No protocol selected." << endl;
				}
				done = true; 
				break;		
									
			default:
				cerr << "Invalid entry." << endl;
		}
	}

	return( status );
}



NMBoolean  
Protocol_Create_AppleTalk( NSpProtocolReference &gProtocolRef )
{
	NMBoolean result = true;

#if !TARGET_OS_MAC
	cout << "AppleTalk not available on this platform." << endl;
	return( false );
#endif

	if ( gProtocolRef != NULL )
	{
		cerr << "Warning: Protocol_Create_AppleTalk(), overwriting existing protocol ref." << endl;
	}

	gProtocolRef = NSpProtocol_CreateAppleTalk( NBPName, NBPType, 0, 0 );		

	if ( gProtocolRef == NULL ) {
		cerr << "Error: Protocol_Create_AppleTalk(), creating AppleTalk protocol ref." << endl;
		result = false;
	}
	
	return( result );
}



NMBoolean  
Protocol_Create_IP( NSpProtocolReference &gProtocolRef )
{
	NMBoolean result = true;

	if ( gProtocolRef != NULL )
	{
		cerr << "Warning: Protocol_Create_IP(), overwriting existing protocol ref." << endl;
	}

	gProtocolRef = NSpProtocol_CreateIP( kTCP_Port, 0, 0 );		

	if ( gProtocolRef == NULL ) {
		cerr << "Error: Protocol_Create_IP(), creating IP protocol ref." << endl;
		result = false;
	}

	return( result );
}



NMBoolean  
Host_Game( NSpGameReference  &gGameObject, 
           NSpProtocolListReference  gProtocolListRef )
{
	NMBoolean noError = true;

	if ( gProtocolListRef == NULL ) {
	    
	    cerr << "Error: Host_Game(), ProtocolList not initialized." << endl;
		return( false );
	}

	NMErr error = NSpGame_Host( &gGameObject, 
	                               gProtocolListRef, 
	                               kMaxPlayers, 
	                               kGameName,
	                               kMyPassword,
	                               kConstPlayerName,
	                               kPlayerType,
	                               kNSpClientServer,
	                               0
	                             );

	noError = Check_Error( "Error: Host_Game(), hosting game.", error );

	return( noError );
}



NMBoolean
Join_Game( NSpGameReference  &gGameObject, NSpAddressReference  addressRef )
{
	NMBoolean		approved = false;
	char		denyReason[64]="Error from NSpGame_Join().\0";
	NMBoolean		noError = true;
	
	if ( addressRef == NULL ) {
	    
	    cerr << "Error: Join_Game(), AddressRef not initialized." << endl;
		return( false );
	}

	cout << "\nAttempting to join game..." << endl;

	NMErr error = NSpGame_Join( &gGameObject, 
	                               addressRef, 
	                               kPlayerName, 
	                               kMyPassword,
	                               kPlayerType,
	                               0,
	                               NULL,
	                               0
	                             );

	noError = Check_Error( "Error: Join_Game(), joining game.", error );
	
	
	if ( noError )
	{
		cout << "Connected.  Waiting for approval..." << endl;		
		approved = Wait_For_Approval(gGameObject, denyReason);
	}
		
	if (approved)
	{
			cout << "Join approved, connected to server." << endl;		
	}
	else
	{
		if (gGameObject != NULL)
		{
			cout << "Join failed:" << denyReason << endl;
			
			error = NSpGame_Dispose(gGameObject, kNSpGameFlag_ForceTerminateGame);
	
			if (error == kNMNoError)
				gGameObject = NULL;
		
			noError = Check_Error( "Error: Join_Game(), disposing game.", error );
		}
	}
	
	return( noError );
}



NMBoolean
Join_Game_AT( NSpGameReference  &gGameObject)
{
	NSpAddressReference  addressRef;
	char                 gameNameC[ kNSpStr32Len ];
	char                 gameTypeC[ kNSpStr32Len ];
	NMBoolean              result = true;

#if !TARGET_OS_MAC
	cout << "You can't use AppleTalk on this platform!" << endl;
	return( false );
#endif

	//  Freakin' Pascal Strings.  ARRRRRGGGGGGHHHHH!   CRT, July 2, 2000
	
	sprintf(gameNameC, "%#s", NBPName);
	sprintf(gameTypeC, "%#s", NBPType);
	
	addressRef = NSpCreateATlkAddressReference(gameNameC, gameTypeC, NULL);

	if ( addressRef == NULL ) {
		return( false );
	}

	result = Join_Game( gGameObject, addressRef );
	
	NSpReleaseAddressReference( addressRef );
	
	return( result );
}



NMBoolean
Join_Game_IP( NSpGameReference  &gGameObject)
{
	NSpAddressReference  	addressRef;
	char					theIPAddress[ kDataStrLen ];
	char            		kTCP_PortStr[  kNSpStr32Len  ];
	NMBoolean                 result = true;

	sprintf( kTCP_PortStr, "%i", kTCP_Port );

	cout << "\nEnter host IP address (e.g. 152.34.7.254):  ";
	cin >> theIPAddress;
	
	addressRef = NSpCreateIPAddressReference( theIPAddress, kTCP_PortStr );

	if ( addressRef == NULL ) {
		return( false );
	}
	
	result = Join_Game( gGameObject, addressRef );
	
	NSpReleaseAddressReference(addressRef);
	

	return( result );
}



NMBoolean
Wait_For_Approval(NSpGameReference &gGameObject, char *theDenyReason)
{
	NMBoolean 				approved = false;
	NMBoolean					response = false;
	NMUInt32			startTime;
	NMUInt32			currentTime;
	NSpMessageHeader		*message;
	clock_t					clocks;					

	if (gGameObject != NULL)
	{
		// So, since you've attempted to join, check for messages saying whether it's been
		// approved or not...

		clocks = machine_tick_count();
		startTime = clocks / MACHINE_TICKS_PER_SECOND;

			
		while (response == false)
		{
#ifdef OP_PLATFORM_WINDOWS
		MSG	msg;

				// check for pending messages.		
				//** This should probably only look for messages in my message queue, and not everyones, but I don't have access
				//** to a list of windows right here (OpenPlay does though)
				// Changed 'while' to 'if' for PeekMessage(), so we stop as soon as we're open-complete.
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
			}
#endif	
			if ((message = NSpMessage_Get(gGameObject)) != NULL)
			{
				switch(message->what)
				{		
					case kNSpJoinApproved:
						response = true;
						approved = true;
					break;			
	
					case kNSpJoinDenied:
						sprintf(theDenyReason, "%#s\0", 
							 ((NSpJoinDeniedMessage *) message) -> reason );
						response = true;
						approved = false;
					break;			
							
					case kNSpError:
						sprintf(theDenyReason, "Error # %d\0", ((NSpErrorMessage *) message) -> error );
						response = true;
						approved = false;
					break;
							
					case kNSpGameTerminated:
						sprintf(theDenyReason, "Lost connection to server.\0");
						response = true;
						approved = false;
					break;
				}
				
				NSpMessage_Release(gGameObject, message);
			}

			clocks = machine_tick_count();
			currentTime = clocks / MACHINE_TICKS_PER_SECOND;

			if ( ( currentTime - startTime > 60 ) && (response == false) )
			{		
					strcpy(theDenyReason, "Connection timed out.  Try again later.\0");
					response = true;
					approved = false;
			}
		}
	}
	
	return approved;
}



NMBoolean  
Dispose_Game( NSpGameReference  &gGameObject )
{
	NMBoolean result = true;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Dispose_Game(), GameObject not active." << endl;
		return( false );
	}

	NMErr error = NSpGame_Dispose( gGameObject, 
	                                  kNSpGameFlag_ForceTerminateGame
	                                );

	result = Check_Error( "Error: Dispose_Game(), disposing game.", error );
	
	if ( result == true ) {
	
		gGameObject = NULL;
	}

	return( result );
}



NMBoolean
Get_Game_Info( NSpGameReference  gGameObject )
{
	NSpGameInfo  gameInfo;
	NMBoolean  noError;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Get_Game_Info(), GameObject not initialized." << endl;
		return( false );
	}

	NMErr error = NSpGame_GetInfo( gGameObject, &gameInfo);

	noError = Check_Error( "Error: Get_Game_Info(), getting game info.", error );
	
	if ( noError ) {
	
		//  Freakin' Pascal Strings.  ARRRRRGGGGGGHHHHH!   CRT, July 2, 2000

		char	gameNameC[ kNSpStr32Len ];
		char	gamePassC[ kNSpStr32Len ];
		
		sprintf(gameNameC, "%#s", gameInfo.name);
		sprintf(gamePassC, "%#s", gameInfo.password);

		cout << endl << "GAME INFO:" << endl;
		cout << "\t Max Players:  " << gameInfo.maxPlayers << endl;
		cout << "\t Current Players:  " << gameInfo.currentPlayers << endl;
		cout << "\t Current Groups:  " << gameInfo.currentGroups << endl;
		cout << "\t Topology (1 == Client/Server):  " << gameInfo.topology << endl;
		cout << "\t Game Name:  " << gameNameC << endl;
		cout << "\t Game Password:  " << gamePassC << endl << endl;
	}

	return( noError );
}



NMBoolean
Send_Message( NSpGameReference  gGameObject )
{
	NMBoolean result;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Send_Message(), GameObject not initialized." << endl;
		return( false );
	}
	
	NSpClearMessageHeader( ( NSpMessageHeader* ) gDataToSend );
	
	// Populate the header the way you want it...
	
	gDataToSend->header.what = kMyMessageType;
	gDataToSend->header.to = kNSpAllPlayers;
	gDataToSend->header.messageLen = sizeof(NSpMessageHeader) + 
										strlen(kCannedMessage) + 1;

	strcpy( gDataToSend->dataStr, kCannedMessage );
	
	
	NMErr error = NSpMessage_Send(gGameObject, &gDataToSend->header,
	                                    kNSpSendFlag_Registered);
	                                
	result = Check_Error( "Error: Send_Message(), sending message.", error );

	if ( result ) {
		cout << "\nMessage sent." << endl;
	}

	return( result );
}



void
Get_Messages(NSpGameReference  gGameObject)
{
	NSpMessageHeader	*messageHeaderPtr;
	int					messageCount = 0;
		
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Get_Messages(), GameObject not initialized." << endl;
		return;
	}
	
	cout << endl;

	while( ( messageHeaderPtr = NSpMessage_Get( gGameObject ) ) != NULL )
	{
		messageCount++;
		
		// Handle the message...
			
		Handle_Message( messageHeaderPtr );
		
		// And release it...
		
		NSpMessage_Release(gGameObject, messageHeaderPtr);
	}
	
	cout << "Get_Messages() Summary:  Received " 
	     << messageCount << " new message(s)." << endl;
}



void
Handle_Message(NSpMessageHeader *messageHeaderPtr)
{
	// Print out the type of message received

	switch(messageHeaderPtr -> what)
	{
		case kNSpJoinRequest:
			cout << "Got JoinRequest message." << endl;
			break;
		
		case kNSpJoinApproved:
			cout << "Got JoinApproved message." << endl;
			break;
		
		case kNSpJoinDenied:
			cout << "Got JoinDenied message." << endl;
			break;

		case kNSpPlayerJoined:
			cout << "Got PlayerJoined message." << endl;
			break;

		case kNSpPlayerLeft:
			cout << "Got PlayerLeft message." << endl;
			break;

		case kNSpError:
			cout << "Got Error message." << endl;
			break;
		
		case kNSpHostChanged:
			cout << "Got HostChanged message." << endl;
			break;

		case kNSpGameTerminated:
			cout << "Got GameTerminated message." << endl;
			break;

		case kNSpGroupCreated:
			cout << "Got GroupCreated message." << endl;
			break;

		case kNSpGroupDeleted:
			cout << "Got GroupDeleted message." << endl;
			break;
		
		case kNSpPlayerAddedToGroup:
			cout << "Got PlayerAddedToGroup message." << endl;
			break;

		case kNSpPlayerRemovedFromGroup:
			cout << "Got PlayerRemovedFromGroup message." << endl;
			break;

		case kNSpPlayerTypeChanged:
			cout << "Got PlayerTypeChanged message." << endl;
			break;

		case kMyMessageType:
			cout << "Got custom message: " << 
					((TestMessage *) messageHeaderPtr)->dataStr << endl;
			break;
		
		default:
			cout << "Got message of type " << messageHeaderPtr -> what << "." << endl;
	}
}



void
Release_Message( NSpGameReference  gGameObject, TestMessage  *message )
{
	NMBoolean result = true;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Release_Message(), GameObject not initialized." << endl;
		return;
	}

	if ( message != NULL ) {

		NSpMessage_Release( gGameObject, (NSpMessageHeader*) message );
		message = NULL;
	}
}



NMBoolean
Print_Players( NSpGameReference  gGameObject )
{
	NMErr						error;
	NSpPlayerInfoPtr			playerInfoPtr;
	NSpPlayerEnumerationPtr		playerEnumPtr;
	int							playerIndex;
	int							groupIndex;
	int							numPlayers;
	char						playerNameC[ kNSpStr32Len ];
	NMBoolean  noError = true;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Print_Players(), GameObject not initialized." << endl;
		return( false );
	}

	error = NSpPlayer_GetEnumeration(gGameObject, &playerEnumPtr);

	noError = Check_Error( "Error: Print_Players(), getting enumeration.", error );

	if ( noError )
	{
		numPlayers = playerEnumPtr -> count;

		cout << "\nPlayer List \t There are currently " << numPlayers << " players:" << endl;

		for ( playerIndex = 0; playerIndex < numPlayers; playerIndex++)
		{
			playerInfoPtr = playerEnumPtr -> playerInfo[playerIndex];
						
			sprintf(playerNameC, "%#s", playerInfoPtr->name);

			cout << endl << "\t\t NetSprocket ID:  " << playerInfoPtr->id << endl;
			cout << "\t\t Player Type:  " << playerInfoPtr->type << endl;
			cout << "\t\t Player Name:  " << playerNameC << endl;
			cout << "\t\t Group Count:  " << playerInfoPtr->groupCount << endl;
			cout << "\t\t Group IDs:  " << endl;
		
			for (groupIndex = 0; groupIndex < playerInfoPtr->groupCount; groupIndex++)
				cout << "\t\t\t  " << playerInfoPtr->groups[groupIndex] << endl;
		
		}
	
		NSpPlayer_ReleaseEnumeration(gGameObject, playerEnumPtr);
	}

	return( noError );
}

	

NMBoolean  
Create_Empty_Protocol_List( NSpProtocolListReference  &gProtocolListRef )
{
	NMBoolean result;
	
	if ( gProtocolListRef != NULL  ) {
	
		cerr << "Warning: Create_Empty_Protocol_List(), overwriting existing protocol list." << endl;
	}
	
	NMErr  error = NSpProtocolList_New( NULL, &gProtocolListRef );
	
	result = Check_Error( "Error: Create_Empty_Protocol_List(), creating list.", error );

	return( result );
}



NMBoolean  
Append_To_Protocol_List( NSpProtocolListReference  gProtocolListRef,
                         NSpProtocolReference gProtocolRef )
{
	NMBoolean result;
	
	if ( gProtocolListRef == NULL || gProtocolRef == NULL ) {
	    
	    cerr << "Error: Append_To_Protocol_List(), NULL protocol or list." << endl;
		return( false );
	}

	NMErr  error = NSpProtocolList_Append( gProtocolListRef, gProtocolRef );
	
	result = Check_Error( "Error: Append_To_Protocol_List(), appending to list.", error );

	return( result );
}

// EOF
