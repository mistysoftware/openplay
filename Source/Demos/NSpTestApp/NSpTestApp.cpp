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

// --------------------------------------------------------------
//
//  FILE:  NSpTestApp.cpp
// 
//  DESCRIPTION:  Simple app for testing NSp API. Uses a text menu
//                interface - not very fancy but extremely portable.
//
//  AUTHOR (i.e. the one to blame ;-) ):  Joe Gervais, 6/00
//
//  CO-AUTHOR (i.e. the one to co-blame 8-O ):  Randy Thompson, 7/00
//
// --------------------------------------------------------------

#ifndef __OPENPLAY__
#include 			"OpenPlay.h"
#endif
#include "OPUtils.h"
#include <time.h>
#include <stdio.h>
#include <iostream>

#if (OP_PLATFORM_MAC_CFM)
	#if (! defined(OP_PLATFORM_MAC_CARBON_FLAG))
		#include <OpenTransport.h>
		#include <Dialogs.h>
	#endif
#elif (OP_PLATFORM_WINDOWS)
  #include <windows.h>
  #include <winbase.h>
#endif

using namespace std;  //introduces namespace std

const long  kMyMessageType = 42;
const int   kDataStrLen = 256;      /* Max packets to receive & store. */
const char  kCannedRegisteredMessage[ kDataStrLen ] = "Just a random registered message";
const char  kCannedJunkMessage[ kDataStrLen ] = "Just a random junk message";

#if OP_PLATFORM_MAC_CFM
  #define d_GAME_ID    'MOOF'
#else
  #define d_GAME_ID    0x4d4f4f46  /* hex for "MOOF" */
#endif


/* The sample structure that will be sent. */
typedef struct {

	NSpMessageHeader  header;
	char  dataStr[ kDataStrLen ];
	
} TestMessage;


NMBoolean  gInServerMode = false;

TestMessage  *gDataToSend = new TestMessage;
TestMessage  *gDataReceived = NULL;

static void doC2PStr(char *str)
{
	long size = strlen(str);
	long x = size;
	while (x > 0){
		str[x] = str[x - 1];
		x--;
	}
	str[0] = size;
}
static void doP2CStr(char *str)
{
	long size = str[0];
	long x = 0;
	while (x < size){
		str[x] = str[x + 1];
		x++;
	}
	str[x] = 0;
}

//we don't use \p formatted strings cuz they're not always supported
//we just set them to normal c strings and convert at runtime...
/*NMStr31  NBPName = "\pDogCow";
NMStr31  NBPType = "\pNSpT";		// Must be a 4 character string.  CRT, July 2, 2000
NMStr31  kConstPlayerName = "\pClarus";
NMStr31  kGameName = "\pDogCow";
NMStr31  kPlayerName = "\pClarus";
NMStr31  kMyPassword = "\pMoof";*/

NMStr31  NBPName = "DogCow";
NMStr31  NBPType = "NSpT";
NMStr31  kConstPlayerName = "Clarus";
NMStr31  kGameName = "DogCow";
NMStr31  kPlayerName = "Clarus";
NMStr31  kMyPassword = "Moof";

NSpPlayerType    kPlayerType = 1;
NSpPlayerID      kPlayerID = 0;

const NMInetPort  kTCP_Port = 25710;
const NMUInt32  kMaxPlayers = 16;

NSpPlayerID			ourNSpID = 12345;

/* --------------  Function Prototypes  -------------- */

void  Check_Error( const char* errMessage, NMErr error );
void  Init_NetSprocket( void );

    /* --------------  Dialog Functions  -------------- */

void  Do_Modal_Host( NSpProtocolListReference  &gProtocolListRef );
void  Do_Modal_Join( NSpAddressReference &gAddressRef );


    /* --------------  Game Object Management -------------- */

void  Host_Game( NSpGameReference  &gGameObject, 
                 NSpProtocolListReference  gProtocolListRef );
void  Join_Game( NSpGameReference  &gGameObject, 
                 NSpAddressReference  gAddressRef );
void  Join_Game_AT( NSpGameReference  &gGameObject);
void  Join_Game_IP( NSpGameReference  &gGameObject);
NMBoolean Wait_For_Approval(NSpGameReference  &gGameObject, char *theDenyReason);
void  Dispose_Game( NSpGameReference  &gGameObject );

    /* --------------  Message Routines -------------- */

void  Send_Message( NSpGameReference  gGameObject, int messageType );
void  Send_Message_To( NSpGameReference  gGameObject );
void  Get_One_Message( NSpGameReference  gGameObject );
void  Get_All_Messages( NSpGameReference  gGameObject );
void  Handle_Message(NSpMessageHeader *messageHeaderPtr);
void  Release_Message( NSpGameReference  gGameObject, TestMessage  *message );


    /* --------------  Player Management -------------- */

void Get_Player_ID( NSpGameReference  gGameObject );
void Get_Player_Info( NSpGameReference  gGameObject );
void Change_Player_Type( NSpGameReference  gGameObject );
void Remove_Player( NSpGameReference  gGameObject );
void Enumerate_Players( NSpGameReference  gGameObject );


    /* --------------  Group Management -------------- */

void Get_Group_Info( NSpGameReference  gGameObject );
void Add_Player_Group( NSpGameReference  gGameObject );
void Remove_Player_Group( NSpGameReference  gGameObject );
void Enumerate_Groups( NSpGameReference  gGameObject );
void Create_New_Group( NSpGameReference  gGameObject );
void Delete_Group( NSpGameReference  gGameObject );


    /* --------------  Protocol Management -------------- */

void  Create_Empty_Protocol_List( NSpProtocolListReference  &gProtocolListRef );
void  Protocol_Create_AppleTalk( NSpProtocolReference &gProtocolRef );
void  Protocol_Create_IP( NSpProtocolReference &gProtocolRef );
void  Append_To_Protocol_List( NSpProtocolListReference  gProtocolListRef,
                               NSpProtocolReference gProtocolRef );
void  Get_Indexed_Protocol_From_List( NSpProtocolListReference  gProtocolListRef, 
                                      NSpProtocolReference gProtocolRef,
                                      NMUInt32 index );                                     
void  Get_Protocol_String( NSpProtocolReference  gProtocolRef,
						   char	*definitionString);
void  Protocol_Create_From_String(NSpProtocolReference &gProtocolRef, 
                                  char *definitionString );
void  Remove_From_Protocol_List( NSpProtocolListReference  gProtocolListRef,
                                 NSpProtocolReference gProtocolRef );
void  Remove_Indexed_From_Protocol_List( NSpProtocolListReference  gProtocolListRef, 
                                         NMUInt32 index );
void  Dispose_Protocol( NSpProtocolReference &gProtocolRef );
void  Dispose_Protocol_List( NSpProtocolListReference  &gProtocolListRef );
void  Get_Protocol_List_Count( NSpProtocolListReference  gProtocolListRef );

    /* --------------  Utilities -------------- */

void  Get_Game_Info( NSpGameReference  gGameObject );

void  Enable_Advertising( NMBoolean state, 
                          NSpGameReference gGameObject, 
                          NSpProtocolReference gProtocolRef );
                        
NMNumVersion  Get_Version( void );    // Kind of silly - Should just do it in place....
void        Set_Connect_Timeout( NMUInt32 seconds );    // Same here....
NMUInt32    Get_Current_TimeStamp( NSpGameReference  gGameObject );


    /* --------------  Asynchronous Routines -------------- */

#if OP_PLATFORM_MAC_CFM
static pascal NMBoolean
#else
NMBoolean
#endif
MyAsyncMessageHandler(NSpGameReference gameRef, NSpMessageHeader *messagePtr, void *inContext);

	
#if OP_PLATFORM_MAC_CFM
static pascal NMBoolean
#else
NMBoolean
#endif
MyJoinRequestHandler(NSpGameReference gameRef, NSpJoinRequestMessage *messagePtr, 
					 void *inContext, unsigned char *outReason, NSpJoinResponseMessage *responseMsg);


    /* --------------  Higher Level Helpers -------------- */

void  Do_Dialogs( NSpAddressReference &gAddressRef, 
                  NSpProtocolListReference gProtocolListRef );
void  Do_Protocols( NSpProtocolReference &, NSpProtocolListReference &, char *gProtocolDefString);
void  Do_GameObject(  NSpGameReference &, 
                      NSpProtocolListReference, 
                      NSpAddressReference );
void  Do_Messages(  NSpGameReference );
void  Do_Players(  NSpGameReference gGameObject );
void  Do_Groups(  NSpGameReference gGameObject );
void  Do_Utilities( NSpGameReference );
void  Do_Quit(NSpGameReference &gGameObject);




/* --------------  Begin Code  -------------- */

int 
main( void ) 
{	

	NSpGameReference          gGameObject = NULL ;
	NSpProtocolReference      gProtocolRef = NULL;
	NSpProtocolListReference  gProtocolListRef = NULL;
	NSpAddressReference       gAddressRef = NULL;
	char                      gProtocolDefString[1024] = "\0";
	
	//NSpGameInfo  *gGameInfo = NULL;

//	NMEvent event;

#if OP_PLATFORM_MAC_CFM

#ifndef OP_PLATFORM_MAC_CARBON_FLAG
	InitGraf((Ptr) &qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);		
	InitCursor();
	MaxApplZone();
	FlushEvents(everyEvent, 0);
#else
	InitCursor();
	FlushEvents(everyEvent, 0);
#endif // ! OP_PLATFORM_MAC_CARBON_FLAG
#endif

	//convert our c strings to pascal
	doC2PStr((char*)NBPName);
	doC2PStr((char*)NBPType);
	doC2PStr((char*)kConstPlayerName);
	doC2PStr((char*)kGameName);
	doC2PStr((char*)kPlayerName);
	doC2PStr((char*)kMyPassword);
	
	
	char  command;
	NMBoolean done = false;
	
	while ( !done ) {
	
		cout << endl << endl;
		cout << "MAIN MENU" << endl << endl;
		cout << "\t 0) Init NetSprocket" << endl;
		cout << "\t 1) Manage Protocols..." << endl;
		cout << "\t 2) Dialogs..." << endl;
		cout << "\t 3) Manage Game Object..." << endl;
		cout << "\t 4) Messages..." << endl;
		cout << "\t 5) Manage Players..." << endl;
		cout << "\t 6) Manage Groups..." << endl;
		cout << "\t 7) Utilities..." << endl;
		cout << "\t q) Quit" << endl << endl;
		cout << "Command: " ;
		cin >> command;

		switch( command ) {
		
			case '0':
				Init_NetSprocket(); 
			break;
				
			case '1':
				Do_Protocols( gProtocolRef, gProtocolListRef, gProtocolDefString ); 
			break;		
			
			case '2':
				Do_Dialogs( gAddressRef, gProtocolListRef ); 
			break;

			case '3':
				Do_GameObject( gGameObject, 
				               gProtocolListRef, 
				               gAddressRef); 
			break;			
			
			case '4':
				Do_Messages( gGameObject ); 
			break;
						
			case '5':
				Do_Players( gGameObject ); 
			break;

			case '6':
				Do_Groups( gGameObject ); 
			break;

			case '7':
				Do_Utilities( gGameObject ); 
			break;
						
			case 'q':
				Do_Quit(gGameObject);
				done = true; 
			break;
				
			default:
				cerr << "Invalid entry." << endl;
			break;
		}
	}


	cout << endl << "Done." << endl;
	
	return( 0 );
}


void
Check_Error( const char* errMessage, NMErr error )
{
	if ( error != kNMNoError ) {
		cerr << errMessage << endl;
		cerr << "\tError code = " << error << endl;
	}
}


void 
Init_NetSprocket( void )
{
	NMErr error = NSpInitialize( 0, 0, 0, d_GAME_ID, 0 );

	Check_Error( "Error: Init_NetSprocket(), initing NSp.", error );

	
	if (error == kNMNoError)
		error = NSpInstallAsyncMessageHandler(MyAsyncMessageHandler, NULL);
	
	Check_Error( "Error: Init_NetSprocket(), Installing async message handler.", error );

	if (error == kNMNoError)
		error = NSpInstallJoinRequestHandler(MyJoinRequestHandler, NULL);

	Check_Error( "Error: Init_NetSprocket(), Installing join request handler.", error );
}


void 
Do_Dialogs( NSpAddressReference &gAddressRef, 
            NSpProtocolListReference gProtocolListRef ) 
{
	NMBoolean done = false;
	char  command;

	while ( !done ) {
	
		cout << endl << endl;
		cout << "DIALOG MENU" << endl << endl;
		cout << "\t 1) Host Dialog..." << endl;
		cout << "\t 2) Join Dialog..." << endl;
		cout << "\t b) Back to Main Menu" << endl << endl;
		cout << "Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '1':
				Do_Modal_Host( gProtocolListRef ); break;
				
			case '2':
				Do_Modal_Join( gAddressRef ); break;		
									
			case 'b':
				done = true; break;
				
			default:
				cerr << "Invalid entry." << endl;
		}
	}
}



void 
Do_Protocols( NSpProtocolReference &gProtocolRef, 
              NSpProtocolListReference &gProtocolListRef,
              char *gProtocolDefString) 
{
	NMBoolean done = false;
	char  command;
	NMUInt32 index;

	while ( !done ) {
	
		cout << endl << endl;
		cout << "PROTOCOL MENU" << endl << endl;
		cout << "\t 0) Create Empty Protocol List" << endl;
		cout << "\t 1) Create AT Protocol" << endl;
		cout << "\t 2) Create IP Protocol" << endl;
		cout << "\t 3) Get Definition String from Protocol" << endl;
		cout << "\t 4) Create Protocol from Definition String" << endl;
		cout << "\t 5) Append Current Protocol to List" << endl;
		cout << "\t 6) Get ProtocolList Count" << endl;
		cout << "\t 7) Get Indexed Protocol from List..." << endl;
		cout << "\t 8) Remove Indexed Protocol from List..." << endl;
		cout << "\t 9) Dispose Current Protocol" << endl;
		cout << "\t a) Dispose Protocol List" << endl;
		cout << "\t b) Back to Main Menu" << endl << endl;
		cout << "Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '0':
				Create_Empty_Protocol_List( gProtocolListRef ); break;

			case '1':
				Protocol_Create_AppleTalk( gProtocolRef ); break;

			case '2':
				Protocol_Create_IP( gProtocolRef ); break;

			case '3':
				Get_Protocol_String(gProtocolRef, gProtocolDefString); 
				break;

			case '4':
				Protocol_Create_From_String(gProtocolRef, gProtocolDefString ); break;
				
			case '5':
				Append_To_Protocol_List( gProtocolListRef, gProtocolRef ); break;

			case '6':
				Get_Protocol_List_Count(gProtocolListRef); break;

			case '7':
				cout << "Enter index: ";
				cin >> index;
				Get_Indexed_Protocol_From_List( gProtocolListRef,
				                                gProtocolRef,
				                                index 
				                              ); 
				break;

			case '8':
				cout << "Enter index: ";
				cin >> index;
				Remove_Indexed_From_Protocol_List( gProtocolListRef,
				                                   index 
				                                 ); 
				break;

			case '9':
				Dispose_Protocol( gProtocolRef ); break;

			case 'a':
				Dispose_Protocol_List( gProtocolListRef ); break;

			case 'b':
				done = true; break;
				
			default:
				cerr << "Invalid entry." << endl;
		}
	}
}



void 
Do_GameObject(  NSpGameReference &gGameObject, 
                NSpProtocolListReference gProtocolListRef, 
                NSpAddressReference gAddressRef )
{
	NMBoolean done = false;
	char  command;

	while ( !done ) {
		cout << endl << endl;
		cout << "GAME OBJECT MENU" << endl << endl;
		cout << "\t 1) Host Game" << endl;
		cout << "\t 2) Join AppleTalk Game" << endl;
		cout << "\t 3) Join TCP/IP Game..." << endl;
		cout << "\t 4) Join Using Dialog Address" << endl;
		cout << "\t 5) Dispose Game" << endl;
		cout << "\t b) Back to Main Menu" << endl << endl;
		cout << "Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '1':
				Host_Game( gGameObject, gProtocolListRef ); break;
				
			case '2':
				Join_Game_AT( gGameObject ); break;		

			case '3':
				Join_Game_IP( gGameObject ); break;		
									
			case '4':
				Join_Game( gGameObject, gAddressRef ); break;
	
			case '5':
				Dispose_Game( gGameObject ); break;		
									
			case 'b':
				done = true; break;
				
			default:
				cerr << "Invalid entry." << endl;
		}
	}
}



void 
Do_Messages(  NSpGameReference gGameObject ) 
{
	NMBoolean done = false;
	char  command;

	while ( !done ) {
	
		cout << endl << endl;
		cout << "MESSAGES MENU" << endl << endl;
		cout << "\t 0) Send Junk Message to All" << endl;
		cout << "\t 1) Send Message to All" << endl;
		cout << "\t 2) Send Message to Player/Group..." << endl;
		cout << "\t 3) Get One Message" << endl;
		cout << "\t 4) Get All Messages" << endl;
		cout << "\t b) Back to Main Menu" << endl << endl;
		cout << "Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '0':
				Send_Message( gGameObject, kNSpSendFlag_Normal ); break;

			case '1':
				Send_Message( gGameObject, kNSpSendFlag_Registered ); break;
				
			case '2':
				Send_Message_To( gGameObject ); break;
				
			case '3':
				Get_One_Message( gGameObject ); break;		
									
			case '4':
				Get_All_Messages(gGameObject); break;
										
			case 'b':
				done = true; break;
				
			default:
				cerr << "Invalid entry." << endl;
		}
	}
}


void 
Do_Players(  NSpGameReference gGameObject )
{
	NMBoolean done = false;
	char  command;

	while ( !done ) {
	
		cout << endl << endl;
		cout << "PLAYERS MENU" << endl << endl;
		cout << "\t 0) Get My NetSprocket ID" << endl;
		cout << "\t 1) Get Player Info..." << endl;
		cout << "\t 2) Change Player Type..." << endl;
		cout << "\t 3) Remove Player..." << endl;
		cout << "\t 4) Enumerate Players..." << endl;
		cout << "\t b) Back to Main Menu" << endl << endl;
		cout << "Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '0':
				Get_Player_ID( gGameObject ); break;

			case '1':
				Get_Player_Info( gGameObject ); break;
				
			case '2':
				Change_Player_Type( gGameObject ); break;		
									
			case '3':
				Remove_Player(gGameObject); break;
										
			case '4':
				Enumerate_Players(gGameObject); break;
										
			case 'b':
				done = true; break;
				
			default:
				cerr << "Invalid entry." << endl;
		}
	}
}


void 
Do_Groups(  NSpGameReference gGameObject )
{
	NMBoolean done = false;
	char  command;

	while ( !done ) {
	
		cout << endl << endl;
		cout << "GROUPS MENU" << endl << endl;
		cout << "\t 1) Get Group Info..." << endl;
		cout << "\t 2) Add Player to Group..." << endl;
		cout << "\t 3) Remove Player from Group..." << endl;
		cout << "\t 4) Enumerate Groups..." << endl;
		cout << "\t 5) Create New Group" << endl;
		cout << "\t 6) Delete Group..." << endl;
		cout << "\t b) Back to Main Menu" << endl << endl;
		cout << "Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '1':
				Get_Group_Info( gGameObject ); break;
				
			case '2':
				Add_Player_Group( gGameObject ); break;		
									
			case '3':
				Remove_Player_Group(gGameObject); break;
										
			case '4':
				Enumerate_Groups(gGameObject); break;
										
			case '5':
				Create_New_Group(gGameObject); break;
										
			case '6':
				Delete_Group(gGameObject); break;
										
			case 'b':
				done = true; break;
				
			default:
				cerr << "Invalid entry." << endl;
		}
	}
}


void 
Do_Utilities( NSpGameReference gGameObject )
{
	NMBoolean done = false;
	char  command;
	
	while ( !done ) {
	
		cout << endl << endl;
		cout << "UTILITIES MENU" << endl << endl;
		cout << "\t 1) Get Game Info" << endl;
		cout << "\t 2) Get Version" << endl;
		cout << "\t 3) Get Current Timestamp" << endl;
		cout << "\t 4) Set Connect Timeout..." << endl;
		cout << "\t 5) Get Player IP Address..." << endl;
#if OP_PLATFORM_MAC_CFM
		cout << "\t 6) Get Player OT Address..." << endl;
#endif
		cout << "\t b) Back to Main Menu" << endl << endl;
		cout << "Command: " ;
		cin >> command;
		
		switch( command ) {
		
			case '1':
			{
				Get_Game_Info( gGameObject ); break;
			}	
			case '2':
			{
				NMNumVersion  v = Get_Version();
				cout << endl << "Version (see NSpTypes.h):  " 
				     << hex << (short) v.majorRev << " "
				     << hex << (short) v.minorAndBugRev << " "
				     << hex << (short) v.stage << " "
				     << hex << (short) v.nonRelRev << " "
				     << endl;

				break;		
			}						
			case '3':
			{
				cout << endl << "Timestamp:  " 
				     << Get_Current_TimeStamp( gGameObject )
				     << endl; 
				break;		
			}						
			case '4':
			{
				NMUInt32  timeout;
				cout << "Enter connect timeout (seconds): ";
				cin >> timeout;
				Set_Connect_Timeout( timeout ); break;		
			}
			case '5':
			{
				NSpPlayerID  	whichPlayer;
				char			*theAddress;
				NMErr			error;
				
				cout << "Enter player ID: ";
				cin >> whichPlayer;
				error = NSpPlayer_GetIPAddress( gGameObject, whichPlayer, &theAddress );	
				
				Check_Error( "Error: NSpPlayer_GetIPAddress(), getting address.", error );
				
				if (error == kNMNoError)
				{
					cout << "Player's IP Address is: " << theAddress << endl;
					NSpPlayer_FreeAddress( gGameObject, (void **)&theAddress );
				}				
				break;
			}
#if OP_PLATFORM_MAC_CFM
			case '6':
			{
				NSpPlayerID  	whichPlayer;
				OTAddress 		*theAddress;
				NMErr		error;
				
				cout << "Enter player ID: ";
				cin >> whichPlayer;
				error = NSpPlayer_GetOTAddress( gGameObject, whichPlayer, &theAddress );	
				
				Check_Error( "Error: NSpPlayer_GetIPAddress(), getting address.", error );
				
				if (error == kNMNoError)
				{
					switch( theAddress->fAddressType )
					{
						case AF_INET:
							cout << "AF_INET Address type, Host: " <<
									((((InetAddress *)theAddress)->fHost & 0xFF000000) >> 24) << "." <<
									((((InetAddress *)theAddress)->fHost & 0x00FF0000) >> 16) << "." <<
									((((InetAddress *)theAddress)->fHost & 0x0000FF00) >>  8) << "." <<
									((((InetAddress *)theAddress)->fHost & 0x000000FF) >>  0) <<
									" Port: " << ((InetAddress *)theAddress)->fPort << endl;
							break;

						case AF_DNS:
							cout << "AF_DNS Address type, Host Name: " << ((DNSAddress *)theAddress)->fName << endl;
							break;
					}
					NSpPlayer_FreeAddress( gGameObject, (void **)&theAddress );
				}				
				break;
			}
#endif
			case 'b':
			{
				done = true; break;
			}	
			default:
				cerr << "Invalid entry." << endl;
		}
	}
}


void 
Do_Quit(NSpGameReference &gGameObject)
{
	if (gGameObject != NULL)
		Dispose_Game(gGameObject);
		
}

void
Do_Modal_Host( NSpProtocolListReference  &gProtocolListRef )
{	

#if OP_PLATFORM_MAC_CFM
	NMBoolean success;
	if ( gProtocolListRef == NULL ) {
	
		cerr << "Error: Do_Modal_Host(), Null protocol list." << endl;
		return;
	}
	
	success = NSpDoModalHostDialog(
					gProtocolListRef,
					(unsigned char*) kGameName,
					(unsigned char*) kPlayerName,
					(unsigned char*) kMyPassword,
					NULL
			  );
			  
	if ( !success ) {
		cerr << "Host game dialog cancelled..." << endl;
	}
#else
	cout << "NSpDoModalHostDialog not yet implemented." << endl;

#endif //OP_PLATFORM_MAC_CFM
}


void
Do_Modal_Join( NSpAddressReference  &gAddressRef )
{

#if OP_PLATFORM_MAC_CFM
	gAddressRef = NSpDoModalJoinDialog(
					NULL,
					"\pWoogie Games",
					(unsigned char*) kPlayerName,
					(unsigned char*) kMyPassword,
					NULL
			  );

	if ( gAddressRef == NULL ) {
		cerr << "Join game dialog cancelled..." << endl;
	}

#else
	cout << "NSpDoModalJoinDialog not yet implemented." << endl;

		//  Remove after NSpDoModalJoinDialog is implemented.
#endif OP_PLATFORM_MAC_CFM
	
}


void  
Protocol_Create_AppleTalk( NSpProtocolReference &gProtocolRef )
{

#if !OP_PLATFORM_MAC_CFM
	cout << "You can't use AppleTalk on this platform!" << endl;
#endif

	if ( gProtocolRef != NULL )
	{
		cerr << "Warning: Protocol_Create_AppleTalk(), overwriting existing protocol ref." << endl;
	}

	gProtocolRef = NSpProtocol_CreateAppleTalk( NBPName, NBPType, 0, 0 );		

	if ( gProtocolRef == NULL )
		cerr << "Error: Protocol_Create_AppleTalk(), creating AppleTalk protocol ref." << endl;

}



void  
Protocol_Create_IP( NSpProtocolReference &gProtocolRef )
{
	if ( gProtocolRef != NULL )
	{
		cerr << "Warning: Protocol_Create_IP(), overwriting existing protocol ref." << endl;
	}

	gProtocolRef = NSpProtocol_CreateIP( kTCP_Port, 0, 0 );		

	if ( gProtocolRef == NULL )
		cerr << "Error: Protocol_Create_IP(), creating IP protocol ref." << endl;

}



void  
Dispose_Protocol( NSpProtocolReference &gProtocolRef )
{
	if ( gProtocolRef != NULL ) {
	
		NSpProtocol_Dispose( gProtocolRef );
		gProtocolRef = NULL;
	}

}



void  
Dispose_Protocol_List( NSpProtocolListReference  &gProtocolListRef )
{
	if ( gProtocolListRef != NULL ) {
	
		NSpProtocolList_Dispose( gProtocolListRef );
		gProtocolListRef = NULL;
	}
}



void  
Enable_Advertising( NMBoolean state, NSpGameReference  gGameObject, NSpProtocolReference gProtocolRef )
{
	if ( gGameObject == NULL || gProtocolRef == NULL ) {
	    
	    cerr << "Error: Enable_Advertising(), GameObject or ProtocolRef not initialized." << endl;
		return;
	}

	NMErr error = NSpGame_EnableAdvertising( gGameObject, 
	                                            gProtocolRef, 
	                                            state
	                                          );

	Check_Error( "Error: Enable_Advertising(), 'nuff said.", error );
}



void  
Host_Game( NSpGameReference  &gGameObject, NSpProtocolListReference  gProtocolListRef )
{
	if ( gProtocolListRef == NULL ) {
	    
	    cerr << "Error: Host_Game(), ProtocolList not initialized." << endl;
		return;
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

	//if no error, get our id
	if (!error)
		ourNSpID = NSpPlayer_GetMyID(gGameObject);

	Check_Error( "Error: Host_Game(), hosting game.", error );
}



void
Join_Game( NSpGameReference  &gGameObject, NSpAddressReference  addressRef)
{
	NMBoolean					approved = false;
	char					denyReason[64]="Error from NSpGame_Join().\0";
	NSpAddressReference		newAddressReference = NULL;
//	char					testConfigString[256];
//	NMErr				theStatus;
		
	if ( addressRef == NULL ) {
	    
	    cerr << "Error: Join_Game(), AddressRef not initialized." << endl;
		return;
	}


	/* The next few lines are completely unnecessary.  They are present only to validate
	that the functions NSpConvertOTAddrToAddressReference, NSpConvertAddressReferenceToOTAddr,
	and NSpReleaseOTAddress are behaving as they should.
	
	We take an NSpAddressReference, convert it into an OTAddress and then back into 
	an NSpAddressReference, making sure to release the intermediate OTAddress afterwards.
	Then we use the final NSpAddressReference for the connection.  It will only work if all
	of the conversions are valid.
	*/

/*	
	#if OP_PLATFORM_MAC_CFM
	
	OTAddress 				*theOTAddressPtr = NULL;
	
	theOTAddressPtr = NSpConvertAddressReferenceToOTAddr(addressRef);
	
	if (theOTAddressPtr != NULL)
	{	
		newAddressReference = NSpConvertOTAddrToAddressReference(theOTAddressPtr);
	
		if (newAddressReference != NULL)
		{
			addressRef = newAddressReference;
		}
		else
		{
	         cerr << "Error: Join_Game(), Error calling NSpConvertOTAddrToAddressReference." << endl;
		}
		
		NSpReleaseOTAddress(theOTAddressPtr);
	}
	else
	{
	    cerr << "Error: Join_Game(), Error calling NSpConvertAddressReferenceToOTAddr." << endl;
	}
	
	#endif		//	OP_PLATFORM_MAC_CFM

	theStatus = NSpProtocol_ExtractDefinitionString( 
				(NSpProtocolReference)  addressRef, testConfigString);

	if (theStatus == kNMNoError)
		cout << testConfigString << endl;
*/
	NMErr error = NSpGame_Join( &gGameObject, 
	                               addressRef, 
	                               kPlayerName, 
	                               kMyPassword,
	                               kPlayerType,
	                               NULL,
	                               0,
	                               0
	                             );

	if (newAddressReference != NULL)
		NSpReleaseAddressReference(newAddressReference);
		
	Check_Error( "Error: Join_Game(), joining game.", error );
	
	
	if (error == kNMNoError)
	{
		cout << "Connected.  Waiting for approval..." << endl;		
		approved = Wait_For_Approval(gGameObject, denyReason);
	}
		
	if (approved)
	{
			cout << "Join approved." << endl;		
			
		//get our id
		ourNSpID = NSpPlayer_GetMyID(gGameObject);			
	}
	else
	{
		if (gGameObject != NULL)
		{
			cout << "Join failed:" << denyReason << endl;
			
			error = NSpGame_Dispose(gGameObject, kNSpGameFlag_ForceTerminateGame);
	
			if (error == kNMNoError)
				gGameObject = NULL;
		
			Check_Error( "Error: Join_Game(), disposing game.", error );
		}
	}
}


void
Join_Game_AT( NSpGameReference  &gGameObject)
{
NSpAddressReference 	addressRef;
char					gameNameC[32];
char					gameTypeC[32];

#if !OP_PLATFORM_MAC_CFM
	cout << "You can't use AppleTalk on this platform!" << endl;
#endif

	//  Freakin' Pascal Strings.  ARRRRRGGGGGGHHHHH!   CRT, July 2, 2000
	
	//ECF i dont think %#s is universally supported
	memcpy(gameNameC,NBPName,NBPName[0] + 1);
	doP2CStr(gameNameC);
	memcpy(gameTypeC,NBPType,NBPType[0] + 1);
	doP2CStr(gameTypeC);		
	//sprintf(gameNameC, "%#s", NBPName);
	//sprintf(gameTypeC, "%#s", NBPType);
	
	addressRef = NSpCreateATlkAddressReference(gameNameC, gameTypeC, NULL);

	Join_Game(gGameObject, addressRef);
	
	NSpReleaseAddressReference(addressRef);
}


void
Join_Game_IP( NSpGameReference  &gGameObject)
{
NSpAddressReference  	addressRef;
char					theIPAddress[16];
//char					testConfigString[256];
//NMErr				theStatus;

	cout << "Enter host IP address:";
	cin >> theIPAddress;
	
	addressRef = NSpCreateIPAddressReference(theIPAddress, "25710");


/*
	theStatus = NSpProtocol_ExtractDefinitionString( 
				(NSpProtocolReference)  addressRef, testConfigString);


	if (theStatus == kNMNoError)
		cout << testConfigString << endl;
*/
	Join_Game(gGameObject, addressRef);
	
	NSpReleaseAddressReference(addressRef);
	
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
	
#if !OP_PLATFORM_MAC_CFM
//	return true;	// My current testing environment (Virtual PC) impedes asynchronous operation on the PC,
#endif				// so I'm not going to go into a tight loop waiting for the approval message from another
					// application.  It will never come.  You should remove this barrier if you have
					// a real PC or two different machines running test applications.
					
	if (gGameObject != NULL)
	{
		// So, since you've attempted to join, check for messages saying whether it's been
		// approved or not...

		clocks = machine_tick_count();
		startTime = clocks / MACHINE_TICKS_PER_SECOND;
			
		
		while (response == false)
		{
			if ((message = NSpMessage_Get(gGameObject)) != NULL)
			{
				switch(message->what)
				{		
					case kNSpJoinApproved:
						response = true;
						approved = true;
					break;			
	
					case kNSpJoinDenied:
						//ECF i dont think %#s is universally supported
						//sprintf(theDenyReason, "%#s",((NSpJoinDeniedMessage *) message) -> reason );
						memcpy(theDenyReason,((NSpJoinDeniedMessage*)message)->reason,
						((NSpJoinDeniedMessage*)message)->reason[0] + 1);
						doP2CStr(theDenyReason);
						response = true;
						approved = false;
					break;			
							
					case kNSpError:
						sprintf(theDenyReason, "Error # %d", (int)((NSpErrorMessage *) message) -> error );
						response = true;
						approved = false;
					break;
							
					case kNSpGameTerminated:
						sprintf(theDenyReason, "Lost connection to server.");
						response = true;
						approved = false;
					break;
				}
				
				NSpMessage_Release(gGameObject, message);
			}

			clocks = machine_tick_count();
			currentTime = clocks / MACHINE_TICKS_PER_SECOND;

			if ( ( currentTime - startTime > 10 ) && (response == false) )
			{					
					strcpy(theDenyReason, "Connection timed out.  Try again later.\0");
					response = true;
					approved = false;
			}
		}
	}
		
	return approved;
}



void  
Dispose_Game( NSpGameReference  &gGameObject )
{
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Dispose_Game(), GameObject not active." << endl;
		return;
	}

	NMErr error = NSpGame_Dispose( gGameObject, 
	                                  kNSpGameFlag_ForceTerminateGame
	                                );

	if (error == kNMNoError)
		gGameObject = NULL;
	
	Check_Error( "Error: Dispose_Game(), disposing game.", error );
}



void
Get_Game_Info( NSpGameReference  gGameObject )
{
	NSpGameInfo  gameInfo;

	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Get_Game_Info(), GameObject not initialized." << endl;
		return;
	}

	NMErr error = NSpGame_GetInfo( gGameObject, &gameInfo);

	Check_Error( "Error: Get_Game_Info(), getting game info.", error );
	
	if ( error == kNMNoError ) {
	
		//  Freakin' Pascal Strings.  ARRRRRGGGGGGHHHHH!   CRT, July 2, 2000

		char	gameNameC[32];
		char	gamePassC[32];
		
		memcpy(gameNameC,gameInfo.name,gameInfo.name[0] + 1);
		doP2CStr(gameNameC);
		memcpy(gamePassC,gameInfo.password,gameInfo.password[0] + 1);
		doP2CStr(gamePassC);		
		//sprintf(gameNameC, "%#s", gameInfo.name);
		//sprintf(gamePassC, "%#s", gameInfo.password);

		cout << endl << "GAME INFO:" << endl;
		cout << "\t Max Players:  " << gameInfo.maxPlayers << endl;
		cout << "\t Current Players:  " << gameInfo.currentPlayers << endl;
		cout << "\t Current Groups:  " << gameInfo.currentGroups << endl;
		cout << "\t Topology (1 == Client/Server):  " << gameInfo.topology << endl;
		cout << "\t Game Name:  " << gameNameC << endl;
		cout << "\t Game Password:  " << gamePassC << endl << endl;
	}
}



void
Send_Message( NSpGameReference  gGameObject, int messageType )
{
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Send_Message(), GameObject not initialized." << endl;
		return;
	}
	
	NSpClearMessageHeader( ( NSpMessageHeader* ) gDataToSend );
	
	// Populate the header the way you want it...
	
	gDataToSend->header.what = kMyMessageType;
	gDataToSend->header.to = kNSpAllPlayers;

	if (messageType == kNSpSendFlag_Registered)
		//strcpy( gDataToSend->dataStr, kCannedRegisteredMessage );
		sprintf(gDataToSend->dataStr,"just a friendly registered message from player %d",ourNSpID);
	else
		//strcpy( gDataToSend->dataStr, kCannedJunkMessage );
		sprintf(gDataToSend->dataStr,"just a friendly junk message from player %d",ourNSpID);
	
	gDataToSend->header.messageLen = sizeof(NSpMessageHeader) + 
										strlen(gDataToSend->dataStr) + 1;
	
	
	NMErr error = NSpMessage_Send(gGameObject, &gDataToSend->header, messageType);
	                                
	Check_Error( "Error: Send_Message(), sending message.", error );
}



void
Send_Message_To( NSpGameReference  gGameObject )
{
	NSpPlayerID		whichPlayer;
	int				dataSize;
	char			data[kDataStrLen];
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Send_Message_To(), GameObject not initialized." << endl;
		return;
	}
	
	cout << "Please enter ID of player (or group) to send to:" << endl;
	cin >> whichPlayer;
	
	strcpy(data, kCannedRegisteredMessage);
	
	dataSize = strlen(data) + 1;
	                                
	NMErr error = NSpMessage_SendTo( gGameObject, 
	                                    whichPlayer,
	                                    kMyMessageType,
	                                    data,
	                                    dataSize,
	                                    kNSpSendFlag_Registered 
	                                );
	                                
	Check_Error( "Error: Send_Message_To(), sending message.", error );
}


void
Get_One_Message( NSpGameReference  gGameObject )
{
	NSpMessageHeader	*messageHeaderPtr;

	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Get_One_Message(), GameObject not initialized." << endl;
		return;
	}

	messageHeaderPtr = NSpMessage_Get( gGameObject );

	if ( messageHeaderPtr == NULL ) {
		cout << "Get_One_Message(), no messages in queue." << endl;
	}
	else
	{
		// Handle the message...
			
		Handle_Message(messageHeaderPtr);
		
		// And release it...
		
		NSpMessage_Release(gGameObject, messageHeaderPtr);
	}
}


void
Get_All_Messages(NSpGameReference  gGameObject)
{
	NSpMessageHeader	*messageHeaderPtr;
	int					messageCount = 0;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Get_One_Message(), GameObject not initialized." << endl;
		return;
	}

	while( (messageHeaderPtr = NSpMessage_Get(gGameObject) ) != NULL)
	{
		messageCount++;
		
		// Handle the message...
			
		Handle_Message(messageHeaderPtr);
		
		// And release it...
		
		NSpMessage_Release(gGameObject, messageHeaderPtr);
	}
	
	cout << "Get_All_Messages(), Received " << messageCount << " messages." << endl;
}

void
Handle_Message(NSpMessageHeader *messageHeaderPtr)
{
		// At least print out what type of message was just received!

		switch(messageHeaderPtr -> what)
		{
			case kNSpJoinApproved:
				cout << "Got JoinApproved message." << endl;
			break;
			
			case kNSpJoinDenied:
				cout << "Got JoinDenied message." << endl;

			break;

			case kNSpPlayerJoined:
				cout << "Got PlayerJoined message (id " 
				<< ((NSpPlayerJoinedMessage*)messageHeaderPtr)->playerInfo.id << ")" << endl;
			break;

			case kNSpPlayerLeft:
				cout << "Got PlayerLeft message (id " 
				<< ((NSpPlayerLeftMessage*)messageHeaderPtr)->playerID << ")" << endl;
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
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Release_Message(), GameObject not initialized." << endl;
		return;
	}

	if ( message != NULL ) {

		NSpMessage_Release( gGameObject, (NSpMessageHeader*) message );
		message = NULL;
	}
}


void
Get_Player_Info( NSpGameReference  gGameObject )
{
	NSpPlayerInfo		*playerInfoPtr;
	NSpPlayerID			whichPlayer;
	int					groupIndex;
	NMErr 			error;
	
	
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Get_Player_Info(), GameObject not initialized." << endl;
		return;
	}

	cout << "Please enter ID of player to get info for:" << endl;
	cin >> whichPlayer;

	error = NSpPlayer_GetInfo(gGameObject, whichPlayer, &playerInfoPtr);

	Check_Error( "Error: Get_Player_Info(), getting player info.", error );
	
	if ( error == kNMNoError ) {
	
		//  Freakin' Pascal Strings.  ARRRRRGGGGGGHHHHH!   CRT, July 2, 2000

		char	playerNameC[32];
		
		
		memcpy(playerNameC,playerInfoPtr->name,playerInfoPtr->name[0] + 1);
		doP2CStr(playerNameC);		
		//sprintf(playerNameC, "%#s", playerInfoPtr->name);

		cout << endl << "PLAYER INFO:" << endl;
		cout << "\t NetSprocket ID:  " << playerInfoPtr->id << endl;
		cout << "\t Player Type:  " << playerInfoPtr->type << endl;
		cout << "\t Player Name:  " << playerNameC << endl;
		cout << "\t Group Count:  " << playerInfoPtr->groupCount << endl;
		cout << "\t Groups:  " << endl;
		
		for (groupIndex = 0; groupIndex < playerInfoPtr->groupCount; groupIndex++)
			cout << "\t\t  " << playerInfoPtr->groups[groupIndex] << endl;
		
		NSpPlayer_ReleaseInfo(gGameObject, playerInfoPtr);

	}
}


void
Get_Player_ID( NSpGameReference  gGameObject )
{
	NSpPlayerID			myNSpID;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Get_Player_ID(), GameObject not initialized." << endl;
		return;
	}

	myNSpID = NSpPlayer_GetMyID(gGameObject);

	cout << "\t My NetSprocket ID is:  " << myNSpID << endl;
}


void
Change_Player_Type( NSpGameReference  gGameObject )
{
	NSpPlayerID			whichPlayer;
	NSpPlayerType		whichType;
	NMErr			error;
	
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Change_Player_Type(), GameObject not initialized." << endl;
		return;
	}

	cout << "Please enter ID of player to change type for:" << endl;
	cin >> whichPlayer;

	cout << "Please enter a type for the player:" << endl;
	cin >> whichType;

	error = NSpPlayer_ChangeType(gGameObject, whichPlayer, whichType);

	Check_Error( "Error: Change_Player_Type(), changing player type.", error );

	if (error == kNMNoError)
		cout << "\t Player type successfully changed." << endl;
}

void
Remove_Player( NSpGameReference  gGameObject )
{
	NSpPlayerID			whichPlayer;
	NMErr			error;

	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Remove_Player(), GameObject not initialized." << endl;
		return;
	}

	cout << "Please enter ID of player to remove:" << endl;
	cin >> whichPlayer;


	error = NSpPlayer_Remove(gGameObject, whichPlayer);

	Check_Error( "Error: Remove_Player(), removing player.", error );

	if (error == kNMNoError)
		cout << "\t Player successfully removed." << endl;
}


void
Enumerate_Players( NSpGameReference  gGameObject )
{
	NMErr					error;
	NSpPlayerInfoPtr			playerInfoPtr;
	NSpPlayerEnumerationPtr		playerEnumPtr;
	int							playerIndex;
	int							groupIndex;
	int							numPlayers;
	char						playerNameC[32];


	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Enumerate_Players(), GameObject not initialized." << endl;
		return;
	}

	error = NSpPlayer_GetEnumeration(gGameObject, &playerEnumPtr);

	Check_Error( "Error: Enumerate_Players(), getting enumeration.", error );

	if (error == kNMNoError)
	{
		numPlayers = playerEnumPtr -> count;

		cout << "\t There are currently " << numPlayers << " players:" << endl;

		for ( playerIndex = 0; playerIndex < numPlayers; playerIndex++)
		{
			playerInfoPtr = playerEnumPtr -> playerInfo[playerIndex];
				
			memcpy(playerNameC,playerInfoPtr->name,playerInfoPtr->name[0] + 1);
			doP2CStr(playerNameC);

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
}


void
Get_Group_Info( NSpGameReference  gGameObject )
{
	NSpGroupInfo		*groupInfoPtr;
	NSpGroupID			whichGroup;
	int					playerIndex;
	NMErr 			error;
	

	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Get_Group_Info(), GameObject not initialized." << endl;
		return;
	}

	cout << "Please enter ID of group to get info for:" << endl;
	cin >> whichGroup;

	error = NSpGroup_GetInfo(gGameObject, whichGroup, &groupInfoPtr);

	Check_Error( "Error: Get_Group_Info(), getting group info.", error );
	
	if ( error == kNMNoError ) {
	
		cout << endl << "GROUP INFO:" << endl;
		cout << "\t Group ID:  " << groupInfoPtr->id << endl;
		cout << "\t Player Count:  " << groupInfoPtr->playerCount << endl;
		cout << "\t Player IDs:  " << endl;
		
		for (playerIndex = 0; playerIndex < groupInfoPtr->playerCount; playerIndex++)
			cout << "\t\t  " << groupInfoPtr->players[playerIndex] << endl;
		
		NSpGroup_ReleaseInfo(gGameObject, groupInfoPtr);
	}
}


void
Add_Player_Group( NSpGameReference  gGameObject )
{
	NSpPlayerID		whichPlayer;
	NSpGroupID		whichGroup;
	NMErr		error;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Add_Player_Group(), GameObject not initialized." << endl;
		return;
	}

	cout << "Please enter group ID to add a player to:" << endl;
	cin >> whichGroup;

	cout << "Please enter ID of player to add:" << endl;
	cin >> whichPlayer;

	error = NSpGroup_AddPlayer(gGameObject, whichGroup, whichPlayer);

	Check_Error( "Error: Add_Player_Group(), adding player.", error );

	if (error == kNMNoError)
		cout << "\t Player successfully added to group." << endl;
}

void
Remove_Player_Group( NSpGameReference  gGameObject )
{
	NSpPlayerID		whichPlayer;
	NSpGroupID		whichGroup;
	NMErr		error;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Remove_Player_Group(), GameObject not initialized." << endl;
		return;
	}

	cout << "Please enter group ID to remove a player from:" << endl;
	cin >> whichGroup;

	cout << "Please enter ID of player to remove:" << endl;
	cin >> whichPlayer;

	error = NSpGroup_RemovePlayer(gGameObject, whichGroup, whichPlayer);

	Check_Error( "Error: Remove_Player_Group(), removing player.", error );

	if (error == kNMNoError)
		cout << "\t Player successfully removed from group." << endl;
}

void
Enumerate_Groups( NSpGameReference  gGameObject )
{
	NMErr					error;
	NSpGroupInfoPtr				groupInfoPtr;
	NSpGroupEnumerationPtr		groupEnumPtr;
	int							playerIndex;
	int							groupIndex;
	int							numGroups;



	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Enumerate_Groups(), GameObject not initialized." << endl;
		return;
	}

	error = NSpGroup_GetEnumeration(gGameObject, &groupEnumPtr);

	Check_Error( "Error: Enumerate_Groups(), getting enumeration.", error );

	if (error == kNMNoError)
	{
		numGroups = groupEnumPtr -> count;

		cout << "\t There are currently " << numGroups << " groups:" << endl;

		for ( groupIndex = 0; groupIndex < numGroups; groupIndex++)
		{
			groupInfoPtr = groupEnumPtr -> groups[groupIndex];
						
			cout << endl << "\t\t Group ID:  " << groupInfoPtr->id << endl;
			cout << "\t\t Player Count:  " << groupInfoPtr->playerCount << endl;
			cout << "\t\t Player IDs:  " << endl;
		
			for (playerIndex = 0; playerIndex < groupInfoPtr->playerCount; playerIndex++)
				cout << "\t\t\t  " << groupInfoPtr->players[playerIndex] << endl;
		
		}
	
		NSpGroup_ReleaseEnumeration(gGameObject, groupEnumPtr);
	}
}

void
Create_New_Group( NSpGameReference  gGameObject )
{
	NSpGroupID		newGroup;
	NMErr		error;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Create_New_Group(), GameObject not initialized." << endl;
		return;
	}
	
	error = NSpGroup_New(gGameObject, &newGroup);

	Check_Error( "Error: Create_New_Group(), creating group.", error );

	if (error == kNMNoError)
		cout << "\t Successfully created group with ID " << newGroup << endl;

}

void
Delete_Group( NSpGameReference  gGameObject )
{
	NSpGroupID		whichGroup;
	NMErr		error;
	
	if ( gGameObject == NULL ) {
	    
	    cerr << "Error: Delete_Group(), GameObject not initialized." << endl;
		return;
	}

	cout << "Please enter ID of group to delete:" << endl;
	cin >> whichGroup;

	error = NSpGroup_Dispose(gGameObject, whichGroup);

	Check_Error( "Error: Delete_Group(), deleting group.", error );

	if (error == kNMNoError)
		cout << "\t Successfully deleted group with ID " << whichGroup << endl;

}

			

void  
Create_Empty_Protocol_List( NSpProtocolListReference  &gProtocolListRef )
{
	if ( gProtocolListRef != NULL  ) {
	
		cerr << "Warning: Create_Empty_Protocol_List(), overwriting existing protocol list." << endl;
	}
	
	NMErr  error = NSpProtocolList_New( NULL, &gProtocolListRef );
	
	Check_Error( "Error: Create_Empty_Protocol_List(), creating list.", error );
}



void  
Append_To_Protocol_List( NSpProtocolListReference  gProtocolListRef,
                         NSpProtocolReference gProtocolRef )
{
	if ( gProtocolListRef == NULL || gProtocolRef == NULL ) {
	    
	    cerr << "Error: Append_To_Protocol_List(), NULL protocol or list." << endl;
		return;
	}

	NMErr  error = NSpProtocolList_Append( gProtocolListRef, gProtocolRef );
	
	Check_Error( "Error: Append_To_Protocol_List(), appending to list.", error );
}



void  
Remove_From_Protocol_List( NSpProtocolListReference  gProtocolListRef,
                           NSpProtocolReference gProtocolRef )
{
	if ( gProtocolListRef == NULL || gProtocolRef == NULL ) {
	    
	    cerr << "Error: Remove_From_Protocol_List(), NULL protocol or list." << endl;
		return;
	}

	NMErr  error = NSpProtocolList_Remove( gProtocolListRef, gProtocolRef );
	
	Check_Error( "Error: Remove_From_Protocol_List(), removing from list.", error );
}



void  
Remove_Indexed_From_Protocol_List( NSpProtocolListReference  gProtocolListRef, 
                                   NMUInt32 index )
{
	if ( gProtocolListRef == NULL ) {
	    
	    cerr << "Error: Remove_Indexed_From_Protocol_List(), NULL list." << endl;
		return;
	}

	NMErr  error = NSpProtocolList_RemoveIndexed( gProtocolListRef, index );
	
	Check_Error( "Error: Remove_Indexed_From_Protocol_List(), removing from list.", error );
}


void
Get_Protocol_List_Count( NSpProtocolListReference  gProtocolListRef )
{
	if ( gProtocolListRef == NULL ) {
	    
	    cerr << "Error: Get_Protocol_List_Count(), NULL list." << endl;
		return;
	}

	cout << "\t Number of Protocols in List is " 
	<< NSpProtocolList_GetCount( gProtocolListRef ) << endl;
}



void
Get_Indexed_Protocol_From_List( NSpProtocolListReference  gProtocolListRef, 
                                NSpProtocolReference gProtocolRef,
                                NMUInt32 index )
{
	if ( gProtocolListRef == NULL ) {
	    
	    cerr << "Error: Get_Indexed_Protocol_From_List(), NULL list." << endl;
		return;
	}
	
	if ( gProtocolRef != NULL ) {
	
		cerr << "Warning: Get_Indexed_Protocol_From_List(), overwriting existing protocol ref." << endl;
	}

	gProtocolRef = NSpProtocolList_GetIndexedRef( gProtocolListRef, index );
		
	if ( gProtocolRef == NULL ) {
	
	    cerr << "Error: Get_Indexed_Protocol_From_List(), NULL result." << endl;
	}
	else
	{
		cout << "\t Index " << index << " of ProtocolList is " 
		<< (NMUInt32) gProtocolRef << endl;
	}
}



void
Get_Protocol_String( NSpProtocolReference  protocolRef, char *definitionString)
{
	NMErr					status;
	
	
	if ( protocolRef == NULL ) {
	    
	    cerr << "Error: Get_Protocol_String(), NULL reference." << endl;
		return;
	}
	
	status = NSpProtocol_ExtractDefinitionString(protocolRef, definitionString);
		
	if (status != kNMNoError)
	{
	    cerr << "Error: Get_Protocol_String(), can't get definition string." << endl;
	}
	else
	{
		cout << "\t Definition String is: " << definitionString << endl;
	}
}


void Protocol_Create_From_String(NSpProtocolReference &gProtocolRef, char *definitionString )
{
	NMErr	status;
	
	if ( gProtocolRef != NULL )
	{
		cerr << "Warning: Protocol_Create_From_String(), overwriting existing protocol ref." 
		<< endl;
	}
	
	if ( strlen(definitionString) == 0 )
	{
	    cerr << "Error: Protocol_Create_From_String(), definition string is empty." << endl;
	}
	else
	{
		status = NSpProtocol_New(definitionString, &gProtocolRef);
		
		if (status != kNMNoError)
	    	cerr << "Error: Protocol_Create_From_String(), can't create protocol." << endl;
		else
			cout << "\t Successfully created protocol. " << endl;
	}
}




NMNumVersion
Get_Version( void )
{

	return( NSpGetVersion() );
}



void
Set_Connect_Timeout( NMUInt32 seconds )
{
	NSpSetConnectTimeout( seconds );
}



NMUInt32
Get_Current_TimeStamp( NSpGameReference  gGameObject )
{
	if ( gGameObject == NULL ) {

	    cerr << "Error: Get_Current_TimeStamp(), NULL game object." << endl;
		return( 0 );
	}
	
	return( NSpGetCurrentTimeStamp( gGameObject ) );
}


#if OP_PLATFORM_MAC_CFM
pascal NMBoolean
#else
NMBoolean
#endif
MyAsyncMessageHandler(NSpGameReference gameRef, NSpMessageHeader *messagePtr, void *inContext)
{	
	#pragma unused (gameRef, messagePtr, inContext)
	
	// Give an indication that this function was executed...
/*	
	#if OP_PLATFORM_MAC_CFM
		SysBeep(10);
	#else
		Beep(10,0);
	#endif
*/
	return 1;
}
	
#if OP_PLATFORM_MAC_CFM
pascal NMBoolean
#else
NMBoolean
#endif
MyJoinRequestHandler(NSpGameReference gameRef, NSpJoinRequestMessage *messagePtr, 
					 void *inContext, unsigned char *outReason, NSpJoinResponseMessage *responseMsg)
{
	#pragma unused (gameRef, messagePtr, inContext, outReason)

	// Give an indication that this function was executed...
/*
	#if OP_PLATFORM_MAC_CFM
		SysBeep(10);
	#else
		Beep(10,0);
	#endif
*/
	return 1;
}

// EOF

// EOF  NSp_OP.cpp
