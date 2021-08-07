/*
	File:		NetStuff.cp

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1999 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(cjd)	Chris De Salvo

	Change History (most recent first):

	   <SP8>	  3/9/99	cjd		Added new enumerations depending on whether you're the client or
									the host.
*/

//¥	------------------------------------------------------------------------------------------	¥
//¥
//¥	Copyright © 1996, 1997 Apple Computer, Inc., All Rights Reserved
//¥
//¥
//¥		You may incorporate this sample code into your applications without
//¥		restriction, though the sample code has been provided "AS IS" and the
//¥		responsibility for its operation is 100% yours.  However, what you are
//¥		not permitted to do is to redistribute the source as "DSC Sample Code"
//¥		after having made changes. If you're going to re-distribute the source,
//¥		we require that you make it clear in the source that the code was
//¥		descended from Apple Sample Code, but that you've made changes.
//¥
//¥		Authors:
//¥			Jamie Osborne
//¥			Chris De Salvo
//¥
//¥	------------------------------------------------------------------------------------------	¥

//¥	------------------------------	Includes
#include "OpenPlay.h"

#if (!OP_PLATFORM_MAC_MACHO)
	#include <Fonts.h>
	#include <MacWindows.h>
	#include <Menus.h>
	#include <OpenTptInternet.h>
	#include <Resources.h>
	#include <TextUtils.h>
	#include <time.h>
#endif

#include "NetStuff.h"

//dont include openplay stuff for old nsp build
#if USING_OLD_NSP 
	#define SWAP4(val) NULL
#else
	#include "OPUtils.h"
	#include "String_Utils.h"
#endif

#if (!OP_PLATFORM_MAC_MACHO)
	#include <PLStringFuncs.h>
#endif

#include <stdio.h>
#include <string.h>

//¥	------------------------------	Private Definitions

//¥	If this is set to 1 then the player enumeration menu item iterates by calling NSpPlayer_GetInfo
//¥	rather than using the data in the PlayerEnumeration data structure.  This is merely for coverage
//¥	testing of the GetInfo call on the client side.
#define USE_PLAYER_GET_INFO			1

//¥	------------------------------	Private Types

enum
{
	kStandardMessageSize	= 1500,
	kBufferSize				= 0, 
	kQElements				= 0,
	kTimeout				= 5000,
	kMaxPlayers				= 8
};

//¥	------------------------------	Private Variables

static NMStr31				gameName;
static NMStr31				password;
static NMStr31				playerName;
static NMStr31				kJoinDialogLabel = "\pChoose a Game:";
static Boolean				gInCallback = false;
static UInt32				gLastUpdate = 0;
UInt32						gUpdateFrequency = 2;
static PlayerInputMessage	gPlayerMessage;
static UInt32				gPlayerMessageSize = 400;
static GameStateMessage		gGameStateMessage;
static UInt32				gGameStateMessageSize = 1024;
static UInt32				gSendOptions = kNSpSendFlag_Normal;
static UInt32				gHostSendOptions = kNSpSendFlag_Registered | kNSpSendFlag_SelfSend;
static MenuHandle			gHostMenuH;
UInt32						gHostUpdateFrequency = 2;
static UInt32				gLastHostUpdate = 0;
static WindowStuff			gWindowStuff[8];
static NSpPlayerID			gMyPlayerID = 0;
static Str255				gContextString = "\pNetSprocket Test Context!!";

//¥	------------------------------	Private Functions

static WindowStuff *GetFreeWindowStuff(void);
static void ReleaseWindowStuff(WindowStuff *inStuff);
static WindowStuff *GetPlayersWindowStuff(NSpPlayerID inPlayer);
static WindowPtr FindPlayersWindow(NSpPlayerID inPlayer);
static void InvalPlayerWindow(NSpPlayerID inPlayer);
static pascal Boolean MessageHandler(NSpGameReference inGameRef, NSpMessageHeader *inMessage, void *inContext);
static void InitNetMenu(MenuHandle menu);
static void GetChooserName(Str255 name, Boolean host);
static void CloseAllWindows(void);
static void AddHostMenu(void);
static void DoAdjustMenu(MenuRef m, UInt32 options, UInt32 updateFrequency, UInt32 messageSize);
static void DoSendLeaveMessage(NSpPlayerID inID);
static void DoHandleMessage(NSpMessageHeader *inMessage);
static void GetMessages(void);
static void UpdateNetWindows(void);

static void GetInfoEnumeratePlayers(void);
static void EnumeratePlayers(void);
static void encode_transmission(WindowStuff *stuff, UInt8 *data, Boolean isGameState);
static void decode_transmission(long fromID, WindowStuff *stuff, UInt8 *data, long length, Boolean isGameState);

//¥	------------------------------	Public Variables

Boolean 			gHost = false;
NSpGameReference	gNetGame = nil;

//¥	--------------------	GetFreeWindowStuff

// this is not reentrant
static WindowStuff *
GetFreeWindowStuff(void)
{
	for (int i = 0; i < 8; i++)
	{
		if (gWindowStuff[i].id == 0)
			return &gWindowStuff[i];
	}
	
	return nil;
}

//¥	--------------------	ReleaseWindowStuff

static void
ReleaseWindowStuff(WindowStuff *inStuff)
{
	if (inStuff == nil){
		printf("passed in nill stuff pointer!\n");
		fflush(stdout);
	}
	else
		memset(inStuff, 0, sizeof(WindowStuff));
}

//¥	--------------------	GetPlayersWindowStuff

static WindowStuff *
GetPlayersWindowStuff(NSpPlayerID inPlayer)
{
	for (int i = 0; i < 8; i++)
	{
		if (gWindowStuff[i].id == inPlayer)
			return &gWindowStuff[i];
	}
	
	return nil;

}

//¥	--------------------	FindPlayersWindow
//¥
//¥		Finds the window associated with the given player
static WindowPtr
FindPlayersWindow(NSpPlayerID inPlayer)
{
	WindowPtr	w;
	WindowStuff *stuff;
	w = FrontWindow();

	while (w != nil)
	{
		stuff = (WindowStuff *) GetWRefCon(w);	
		if (stuff->id == inPlayer)
			break;
		else
			w = GetNextWindow(w);
	}
	
	return w;
}

//¥	--------------------	InvalPlayerWindow

static void
InvalPlayerWindow(NSpPlayerID inPlayer)
{
WindowPtr	w = FindPlayersWindow(inPlayer);

	if (w)
	{
	GrafPtr		oldPort;
	
		GetPort(&oldPort);
		
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		Rect portRect;
		
		GetPortBounds(GetWindowPort(w),&portRect);
		SetPortWindowPort(w);
		InvalWindowRect(w,&portRect);
#else
		SetPort(w);
		InvalRect(&w->portRect);
#endif
		
		SetPort(oldPort);
	}
}


//¥	--------------------	MessageHandler
//¥
//¥	this function gets called every time NetSprocket receives a new message.
//¥	we use it to prevent our message q from getting too full if we don't idle enough
//¥	a good example is when the user has clicked in the menu bar and is reading menus

static pascal Boolean
MessageHandler(NSpGameReference inGameRef, NSpMessageHeader *inMessage, void *inContext)
{
	gInCallback = true;
//	if it's a system message, just leave it in the q, so that we can handle it later (when we're
//	not at interrupt time)
	if (inMessage->what & kNSpSystemMessagePrefix)
		return true;	// tells NetSprocket to put the message in the message Q
	else	// it's one of ours
	{
		WindowStuff *stuff = GetPlayersWindowStuff(gMyPlayerID);
		
		if (stuff)
		{
			//if we got a gamestate message, update our local gamestate count
			if (inMessage->what == kGameStateMessage)
				stuff->gameStateCount++;
				
			//if we got a player input message, update the player input count on the player its from
			if (inMessage->what == kPlayerInputMessage)
			{
				WindowStuff *stuff2 = GetPlayersWindowStuff(inMessage->from);
				if (stuff2)
				{
					stuff2->playerInputCount++;
					stuff2->changed = true;
				}
			}
				
		}
		
		/*if (inMessage->what == kPlayerInputMessage)
		{
			if (stuff)
			{
				stuff->lastMessage = inMessage->id;
				stuff->gameStateCount++;
				stuff->changed = true;
			}				
		}
		//its a player input message arriving at the host
		if (inMessage->what == kPlayerInputMessage && gHost)
		{			
			WindowStuff *stuff = GetPlayersWindowStuff(inMessage->from);
			if (stuff)
			{
				stuff->lastMessage = inMessage->id;
				stuff->gameStateCount++;
				stuff->changed = true;
			}				
			return true;
		}
		//its a game state message arriving at a client
		else if (inMessage->what == kGameStateMessage && !gHost)
		{
			WindowStuff *stuff = GetPlayersWindowStuff(gMyPlayerID);
			if (stuff)
			{
				stuff->lastMessage = inMessage->id;
				stuff->gameStateCount++;
				stuff->changed = true;
			}				
			return true;
		}*/
	}
	gInCallback = false;

	//¥	If it's some state we haven't considered then better leave it in the queue
	return (true);
}

//¥	--------------------	GetChooserName

static void
GetChooserName(Str255 name, Boolean host)
{
StringHandle	userName;
	
	userName = GetString(-16096);
	if (userName == nil)
	{
		//name[0] = 0;
		
		//if we're hosting, we're host-player-guy of course.
		if (host)
			PLstrcpy(name,"\pHost-Player-Guy");
		//if we're a client - give ourself a random-ish name
		else
		{
			char timeString[64];
			long theTime;
			time(&theTime);
			sprintf(timeString,"%d",theTime);
			PLstrcpy(name,"\pClient-Player00");
			name[name[0] - 1] = timeString[strlen(timeString) - 2];
			name[name[0] - 0] = timeString[strlen(timeString) - 1];
		}
	}
	else
	{	
		PLstrcpy(name, *userName);
		ReleaseResource ((Handle) userName);
	}
}

//¥	--------------------	RefreshWindow

void
RefreshWindow(WindowPtr inWindow)
{
	WindowStuff *stuff = (WindowStuff *) GetWRefCon(inWindow);
	//in carbon, it would appear we're getting the sioux window or something
	//so just return on a NULL stuff item...
	if (stuff == NULL)
		return;
				
	//see if throughput needs updating
	UInt32 time = TickCount();
	if (stuff->lastThroughputUpdate + 60 < time){
		stuff->lastThroughputUpdate = time;
		
		//message rate
		stuff->gameStateRate = stuff->gameStateCount - stuff->prevGameStateCount;
		stuff->playerInputRate = stuff->playerInputCount - stuff->prevPlayerInputCount;
		
		//pure data throughput
		stuff->gameStateThroughput = (float)(stuff->gameStateBytes - stuff->prevGameStateBytes)/1024;
		stuff->playerInputThroughput = (float)(stuff->playerInputBytes - stuff->prevPlayerInputBytes)/1024;
		
		//total throughput with headers
		stuff->totalGameStateThroughput = (float)(stuff->totalGameStateBytes - stuff->prevTotalGameStateBytes)/1024;
		stuff->totalPlayerInputThroughput = (float)(stuff->totalPlayerInputBytes - stuff->prevTotalPlayerInputBytes)/1024;

		stuff->prevGameStateCount = stuff->gameStateCount;
		stuff->prevPlayerInputCount = stuff->playerInputCount;
		
		stuff->prevPlayerInputBytes = stuff->playerInputBytes;
		stuff->prevGameStateBytes = stuff->gameStateBytes;
		
		stuff->prevTotalPlayerInputBytes = stuff->totalPlayerInputBytes;
		stuff->prevTotalGameStateBytes = stuff->totalGameStateBytes;
	
	}
	
	if (stuff->id == gMyPlayerID) //its the local player
	{
		sprintf(stuff->TextLine1,"%d gamestates received;(%d/sec); %d bytes; (%.1fk/sec); w/header: %d (%.1fk/sec)",
			stuff->gameStateCount,stuff->gameStateRate,stuff->gameStateBytes,stuff->gameStateThroughput,stuff->totalGameStateBytes,stuff->totalGameStateThroughput);
		sprintf(stuff->TextLine2,"%d player-inputs sent;(%d/sec); %d bytes; (%.1fk/sec); w/header: %d (%.1fk/sec)",
			stuff->playerInputCount,stuff->playerInputRate,stuff->playerInputBytes,stuff->playerInputThroughput,stuff->totalPlayerInputBytes,stuff->totalPlayerInputThroughput);
	}
	else //its a remote player - swap sent/received
	{
		sprintf(stuff->TextLine1,"%d gamestates sent;(%d/sec); %d bytes; (%.1fk/sec); w/header: %d (%.1fk/sec)",
			stuff->gameStateCount,stuff->gameStateRate,stuff->gameStateBytes,stuff->gameStateThroughput,stuff->totalGameStateBytes,stuff->totalGameStateThroughput);
		sprintf(stuff->TextLine2,"%d player-inputs received;(%d/sec); %d bytes; (%.1fk/sec); w/header: %d (%.1fk/sec)",
			stuff->playerInputCount,stuff->playerInputRate,stuff->playerInputBytes,stuff->playerInputThroughput,stuff->totalPlayerInputBytes,stuff->totalPlayerInputThroughput);
	}
	
	//if we're hosting, we can include error info on their player-inputs
	if (gHost)
	{
		//if this is our host endpoint, we can include errors on both player inputs and gamestates
		//otherwise its a remote - we can only check player inputs
		if (stuff->id == gMyPlayerID)
		{
			sprintf(stuff->TextLine3,"%d corrupt gamestates received by player;  %d corrupt player-inputs received from player",stuff->erroneousGameStates,stuff->erroneousPlayerInputs);
			sprintf(stuff->TextLine4,"%d out-of-order gamestates received by player;  %d out-of-order player-inputs received from player",stuff->outOfOrderGameStates,stuff->outOfOrderPlayerInputs);
		}
		else
		{
			sprintf(stuff->TextLine3, " %d corrupt player-inputs received from player",stuff->erroneousPlayerInputs);
			sprintf(stuff->TextLine4, " %d out-of-order player-inputs received from player",stuff->outOfOrderPlayerInputs);
		}
	}
	else
	{
		sprintf(stuff->TextLine3,"%d corrupt gamestates from host",stuff->erroneousGameStates);
		sprintf(stuff->TextLine4,"%d gamestate discontinuities from host",stuff->outOfOrderGameStates);
	}
	
	c2pstr(stuff->TextLine1);
	c2pstr(stuff->TextLine2);
	c2pstr(stuff->TextLine3);
	c2pstr(stuff->TextLine4);
	
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
        Rect portRect;
	
		GetPortBounds(GetWindowPort(inWindow),&portRect);
		EraseRect(&portRect);
	#else
		EraseRect(&inWindow->portRect);
#endif //OP_PLATFORM_MAC_CARBON_FLAG
	
	TextSize(10);
	MoveTo(5,20);
	DrawString((UInt8*)stuff->TextLine1);	
	MoveTo(5,35);
	DrawString((UInt8*)stuff->TextLine2);	
	MoveTo(5,50);
	DrawString((UInt8*)stuff->TextLine3);	
	MoveTo(5,65);
	DrawString((UInt8*)stuff->TextLine4);	
}


//¥	--------------------	InitNetMenu

static void
InitNetMenu(MenuHandle menu)
{
	SetItemMark(menu, iJunk, noMark);
	SetItemMark(menu, iNormal, noMark);
	SetItemMark(menu, iRegistered, noMark);
	SetItemMark(menu, iBlocking, noMark);
	SetItemMark(menu, i1X, noMark);
	SetItemMark(menu, i10X, noMark);
	SetItemMark(menu, i30X, noMark);
	SetItemMark(menu, iNoLimit, noMark);
	SetItemMark(menu, i400, noMark);
	SetItemMark(menu, i1K, noMark);
	SetItemMark(menu, i10K, noMark);
	SetItemMark(menu, i100K, noMark);
}

//¥	--------------------	InitNetworking

NMErr
InitNetworking(NSpGameID inGameID)
{
	NMErr	status;
	
	printf("Initializing NetSprocket... ");
	fflush(stdout);
	memset(gWindowStuff, 0, sizeof(WindowStuff) * 8);
	status = NSpInitialize(kStandardMessageSize, kBufferSize, kQElements, inGameID, kTimeout);
	if (status != noErr){
		printf("NSpInitialize returned error %d\n", status);
		fflush(stdout);
	}
	else
	{
		printf("Done\n");
		fflush(stdout);
	}	
//	Install an async message handler.  We do this so that we won't get hosed if we
//	don't call HandleNetworking often enough (such as a tight loop for mouse-down
	status = NSpInstallAsyncMessageHandler(MessageHandler, gContextString);
	if (status != noErr)
	{
		printf("NSpInstallAsyncMessageHandler returned error %d\n", status);
		fflush(stdout);
	}
	MenuHandle h = GetMenuHandle(131);
	if (h)
		InitNetMenu(h);
	
	return status;
}


//¥	--------------------	CloseAllWindows

static void
CloseAllWindows(void)
{
WindowPtr	w, nextWind;

	w = FrontWindow();

	while (w != nil)
	{
		nextWind = GetNextWindow(w);
		WindowStuff *stuff = (WindowStuff *) GetWRefCon(w);
		if (stuff)
		{
			//silly rabbit, these are preallocated. - we still check for a non-null stuff
			//since the sioux window's will be NULL and we dont wanna kill that.
			//DisposePtr((Ptr) stuff);
			DisposeWindow(w);
		}		
		w = nextWind;
	}
}


//¥	--------------------	ShutdownNetworking

void
ShutdownNetworking(void)
{
NMErr	status;
	
	if (gNetGame)
	{
		status = NSpGame_Dispose( gNetGame, 0 );
		if (status != noErr)
		{
			if (status == kNSpNoHostVolunteersErr)
			{
				printf("unable to find replacement host. Terminating game.\n");
				fflush(stdout);
			}
			else
			{
				printf("NSpGame_Dispose returned error %d\n", status);
				fflush(stdout);
			}
//			I REALLY want it to shutdown
			status = NSpGame_Dispose( gNetGame, kNSpGameFlag_ForceTerminateGame );	
			if (status != noErr)
			{
				printf("NSpGame_Dispose (force) returned error %d\n", status);
				fflush(stdout);
			}
		}
		
		if (gHost)
		{
			DeleteMenu(132);
			ReleaseResource((Handle) gHostMenuH);
			DrawMenuBar();
		}

		gNetGame = NULL;
	}

//	if there are any windows left, kill them
	CloseAllWindows();
}


//¥	--------------------	AddHostMenu

static void
AddHostMenu(void)
{
	gHostMenuH = GetMenu(132);
	if (gHostMenuH == nil)
		DEBUG_PRINT("AddHostMenu: Couldn't find menu resource!");
	else
		InsertMenu(gHostMenuH, 0);
		
	InitNetMenu(gHostMenuH);
	
	DrawMenuBar();
}

//¥	--------------------	HandleNetMenuChoice

void
HandleNetMenuChoice(short menu, short item)
{
	MenuHandle h = GetMenuHandle(menu);
	UInt32 *options;
	UInt32 *frequency;
	UInt32 *size;

	if (menu == 132)	// host menu
	{
		options = &gHostSendOptions;
		frequency = &gHostUpdateFrequency;
		size = &gGameStateMessageSize;
	}
	else
	{
		options = &gSendOptions;
		frequency = &gUpdateFrequency;
		size = &gPlayerMessageSize;
	}
				
	switch(item)
	{
		case iJunk:
			*options &= 0xFF0FFFFF;
			*options |= kNSpSendFlag_Junk;
			printf("message type set to kNSpSendFlag_Junk\n");
			fflush(stdout);
		break;
		case iNormal:
			*options &= 0xFF0FFFFF;
			*options |= kNSpSendFlag_Normal;
			printf("message type set to kNSpSendFlag_Normal\n");
			fflush(stdout);
		break;
		case iRegistered:
			*options &= 0xFF0FFFFF;
			*options |= kNSpSendFlag_Registered;
			printf("message type set to kNSpSendFlag_Registered\n");
			fflush(stdout);
		break;
		case iBlocking:
			if (*options & kNSpSendFlag_Blocking)
				*options &= ~kNSpSendFlag_Blocking;
			else
				*options |= kNSpSendFlag_Blocking;
			printf("kNSpSendFlag_Blocking set to %d\n",((*options & kNSpSendFlag_Blocking) != 0));
			fflush(stdout);
		break;
		case i1X:
			*frequency = 60;
			printf("frequency set to %d\n",60 / *frequency);
			fflush(stdout);
			break;
			
		case i10X:
			*frequency = 6;
			printf("frequency set to %d\n",60 / *frequency);
			fflush(stdout);
			break;
			
		case i30X:
			*frequency = 2;
			printf("frequency set to %d\n",60 / *frequency);
			fflush(stdout);
			break;
			
		case iNoLimit:
			*frequency = 0;
			printf("frequency set to %d\n",60 / *frequency);
			fflush(stdout);
			break;
			
		case i4:
			*size = 4;
			printf("size set to %d\n",*size);
			fflush(stdout);
			break;

		case i16:
			*size = 16;
			printf("size set to %d\n",*size);
			fflush(stdout);
			break;
			
		case i400:
			*size = 400;
			printf("size set to %d\n",*size);
			fflush(stdout);
			break;
			
		case i1K:
			*size = 1024;
			printf("size set to %d\n",*size);
			fflush(stdout);
			break;
			
		case i10K:
			*size = 10240;
			printf("size set to %d\n",*size);
			fflush(stdout);
			break;
			
		case i100K:
			*size = 102400;
			printf("size set to %d\n",*size);
			fflush(stdout);
			break;
			
		case iEnumerate:
#if USE_PLAYER_GET_INFO
			if (131 != menu)
				GetInfoEnumeratePlayers();
			else
#endif
				EnumeratePlayers();
			break;
			
		default:
		break;
	}
	
}

//¥	--------------------	DoAdjustMenu

static void
DoAdjustMenu(MenuRef m, UInt32 options, UInt32 updateFrequency, UInt32 messageSize)
{
	
	if (options & kNSpSendFlag_Junk)
		SetItemMark(m, iJunk, checkMark);
	else
		SetItemMark(m, iJunk, noMark);

	if (options & kNSpSendFlag_Normal)
		SetItemMark(m, iNormal, checkMark);
	else
		SetItemMark(m, iNormal, noMark);

	if (options & kNSpSendFlag_Registered)
		SetItemMark(m, iRegistered, checkMark);
	else
		SetItemMark(m, iRegistered, noMark);
		
	if (options & kNSpSendFlag_Blocking)
		SetItemMark(m, iBlocking, checkMark);
	else
		SetItemMark(m, iBlocking, noMark);
		
	SetItemMark(m, i1X, noMark);
	SetItemMark(m, i10X, noMark);
	SetItemMark(m, i30X, noMark);
	SetItemMark(m, iNoLimit, noMark);
	switch(updateFrequency)
	{
		case 60:
			SetItemMark(m, i1X, checkMark);
		break;
		case 6:
			SetItemMark(m, i10X, checkMark);
		break;
		case 2:
			SetItemMark(m, i30X, checkMark);
		break;
		default:
			SetItemMark(m, iNoLimit, checkMark);
		break;
	}

	SetItemMark(m, i4, noMark);
	SetItemMark(m, i16, noMark);
	SetItemMark(m, i400, noMark);
	SetItemMark(m, i1K, noMark);
	SetItemMark(m, i10K, noMark);
	SetItemMark(m, i100K, noMark);
	switch(messageSize)
	{
		case 4:
			SetItemMark(m, i4, checkMark);
		break;
		case 16:
			SetItemMark(m, i16, checkMark);
		break;
		case 400:
			SetItemMark(m, i400, checkMark);
		break;
		case 1024:
			SetItemMark(m, i1K, checkMark);
		break;
		case 10240:
			SetItemMark(m, i10K, checkMark);
		break;
		case 102400:
			SetItemMark(m, i100K, checkMark);
		break;
	}
	
}	

//¥	--------------------	AdjustNetMenus

void
AdjustNetMenus(void)
{
	MenuRef m = GetMenuHandle(131);
	
	DoAdjustMenu(m, gSendOptions, gUpdateFrequency, gPlayerMessageSize);

	if (gHost)
	{
		m = GetMenuHandle(132);
		if (m)
			DoAdjustMenu(m, gHostSendOptions, gHostUpdateFrequency, gGameStateMessageSize);
	}
}

//¥	--------------------	DoHost

NMErr
DoHost(void)
{
	NMErr 	status;
	Str255		chooserName;
	NSpProtocolListReference	theList = NULL;
	Boolean okHit;
	
//	Create an empty protocol list
	status = NSpProtocolList_New(NULL, &theList);
	if (status != noErr)
	{
		printf("NSpProtocolList_New returned error: %d\n", status);
		fflush(stdout);
		goto failure;
	}
	
//	Now present a UI for the hosting
//	Note!  Do NOT pass in string constants, as the user can change these values
	GetChooserName(chooserName,true);
	PLstrncpy(playerName, chooserName, 31);	
	PLstrcpy(gameName, "\pNetSprocket Test");
	password[0] = 0;
	
	okHit = NSpDoModalHostDialog(theList, gameName, playerName, password, nil);
	if (!okHit)
	{
		printf("User pressed cancel (or dialog not implemented)\n");
		fflush(stdout);
		status = kUserCancelled;
		goto failure;
	}

	//	Now host the game
	status = NSpGame_Host(&gNetGame, theList, kMaxPlayers, gameName,
				password, playerName, 0, kNSpClientServer, 0);
	if (status != noErr)
	{
		printf("NSpGame_Host returned error %d\n", status);
		fflush(stdout);
		goto failure;
	}
	
	gHost = true;
	
	
	if (status == noErr)
		AddHostMenu();
		
	return status;
	
failure:
	if (theList != nil)
		NSpProtocolList_Dispose(theList);
		
	return status;
}

//¥	--------------------	DoJoin

NMErr
DoJoin(void)
{
	NSpAddressReference	theAddress;
	NMErr				status;
	Str255				chooserName;
	
	GetChooserName(chooserName,false);
	PLstrncpy(playerName, chooserName, 31);	
	password[0] = 0;
	
//	Present the UI for joining a game
//	passing an empty string (not nil) for the type causes NetSprocket to use
//	the game id passed in to initialize
	theAddress = NSpDoModalJoinDialog("\p", kJoinDialogLabel, playerName, password, NULL);
	if (theAddress == NULL)		// The user cancelled
	{
		printf("User pressed cancel (or dialog not implemented)\n");
		fflush(stdout);	
		return kUserCancelled;
	}
	
	status = NSpGame_Join(&gNetGame, theAddress, playerName, password, 0, NULL, 0, 0);
	if (status != noErr)
	{
		printf("NSpGame_Join returned error %d\n", status);
		fflush(stdout);
	}
	else		
		gHost = false;	

	return status;
}

//¥	--------------------	DoSendLeaveMessage

static void
DoSendLeaveMessage(NSpPlayerID inID)
{
NSpMessageHeader m;
NMErr status;
	
	NSpClearMessageHeader(&m);
	
	m.what = kLeaveMessage;
	m.to = inID;
	m.messageLen = sizeof(NSpMessageHeader);

	status = NSpMessage_Send(gNetGame, &m, kNSpSendFlag_Registered);

}

//¥	--------------------	DoCloseNetWindow

void
DoCloseNetWindow(WindowPtr inWindow)
{
	if (gHost)
	{
		WindowStuff *stuff = (WindowStuff *) GetWRefCon(inWindow);	
		DoSendLeaveMessage(stuff->id);
	}
	else
		ShutdownNetworking();
	
}

//¥	--------------------	DoHandleMessage

static void
DoHandleMessage(NSpMessageHeader *inMessage)
{
	switch (inMessage->what)
	{
		case kPlayerInputMessage:
		{
			WindowStuff *stuff = GetPlayersWindowStuff(inMessage->from);
			
			//there's a chance this might get here before the player-joined for the player it belongs to
			if (stuff == NULL)
			{
				printf("Got a PlayerInput message from a player we don't know exists. (%d)\nperhaps we just havn't gotten the PlayerJoined yet\n",inMessage->from);
				fflush(stdout);
				break;
			}
			decode_transmission(inMessage->from,stuff,((UInt8*)inMessage) + sizeof(*inMessage),inMessage->messageLen - sizeof(*inMessage),false);
			stuff->playerInputBytes += (inMessage->messageLen - sizeof(*inMessage));	
			stuff->totalPlayerInputBytes += (inMessage->messageLen);	
			stuff->changed = true;
		}
		break;
		
		//a gamestate got to us
		case kGameStateMessage:
		{
			WindowStuff *stuff;
			stuff = GetPlayersWindowStuff(gMyPlayerID);	

			//there's a chance this might get here before the player-joined for the player it belongs to
			if (stuff == NULL)
			{
				printf("Got a GameState message from a player we don't know exists. (%d)\nperhaps we just havn't gotten the PlayerJoined yet\n",inMessage->from);
				fflush(stdout);			
				break;
			}
			
			decode_transmission(inMessage->from,stuff,((UInt8*)inMessage) + sizeof(*inMessage),inMessage->messageLen - sizeof(*inMessage),true);
			stuff->gameStateBytes += (inMessage->messageLen - sizeof(*inMessage));
			stuff->totalGameStateBytes += (inMessage->messageLen);
			stuff->changed = true;
		}
		break;
		
		case kLeaveMessage:
			printf("Got a message telling us to leave the game.");
			fflush(stdout);
			ShutdownNetworking();
		break;
		case kNSpPlayerJoined:
		{
			NSpPlayerJoinedMessage *theMessage = (NSpPlayerJoinedMessage *) inMessage;
			printf("Got player joined message: %d\n", theMessage->playerInfo.id);
			fflush(stdout);

			//the host makes windows for all joiners, and clients only make a window for themselves
			if (gHost || theMessage->playerInfo.id == NSpPlayer_GetMyID(gNetGame))
			{
				static Rect boundsRect = {50,50,125,630};
				gMyPlayerID = NSpPlayer_GetMyID(gNetGame);
				
				WindowPtr wind = NewCWindow(nil,&boundsRect,"\pWindow", true, noGrowDocProc,(WindowPtr)-1,false,0);
				ShowWindow(wind);	
				OffsetRect(&boundsRect,50,50);	
				
				if (!wind)
				{
					printf("Failed to create a new window!\n");
					fflush(stdout);
				}
				else
				{
					WindowStuff *stuff = GetFreeWindowStuff();
					if (stuff == nil)
					{
						printf("Couldn't alloc memory to show window.\n");
						fflush(stdout);
						DisposeWindow(wind);
					}
					else
					{
						stuff->id = theMessage->playerInfo.id;
						stuff->first_decode = true;
						
						stuff->gameStateCount = 0;
						stuff->playerInputCount = 0;
						stuff->prevGameStateCount = 0;
						stuff->prevPlayerInputCount = 0;
						stuff->gameStateRate = 0;
						stuff->playerInputRate = 0;
						
						stuff->gameStateBytes = 0;
						stuff->playerInputBytes = 0;
						stuff->prevGameStateBytes = 0;
						stuff->prevPlayerInputBytes = 0;
						stuff->gameStateThroughput = 0;
						stuff->playerInputThroughput = 0;
						
						stuff->totalGameStateBytes = 0;
						stuff->totalPlayerInputBytes = 0;
						stuff->prevTotalGameStateBytes = 0;
						stuff->prevTotalPlayerInputBytes = 0;
						stuff->totalGameStateThroughput = 0;
						stuff->totalPlayerInputThroughput = 0;
						
						stuff->lastThroughputUpdate = TickCount();

						stuff->erroneousGameStates = 0;
						stuff->outOfOrderGameStates = 0;
						
						stuff->erroneousPlayerInputs = 0;
						stuff->outOfOrderPlayerInputs = 0;
						
						stuff->transmitCodeCountGameState = 0;
						stuff->receiveCodeCountGameState = 0;
						
						stuff->transmitCodeCountPlayerInput = 0;
						stuff->receiveCodeCountPlayerInput = 0;
						
						PLstrcpy((UInt8*)stuff->TextLine1, "\pNew Player");
						PLstrcpy((UInt8*)stuff->TextLine2, "\p");
						PLstrcpy((UInt8*)stuff->TextLine3, "\p");
						PLstrcpy((UInt8*)stuff->TextLine4, "\p");
						
						//tell whether its a local or remote player
						{
							char *whatIsIt;
							if ((gHost) && (theMessage->playerInfo.id == gMyPlayerID))
								whatIsIt = "host";
							else
								whatIsIt = "client";
								
							if (theMessage->playerInfo.id == gMyPlayerID)
							{
								char nameString[256];
								sprintf(nameString,"%#s (local player; %s; id:%d)",theMessage->playerInfo.name,whatIsIt,theMessage->playerInfo.id);
								c2pstr(nameString);
								SetWTitle(wind,(UInt8*)nameString);
							}
							else
							{
								char nameString[256];
								sprintf(nameString,"%#s (remote player; %s; id:%d)",theMessage->playerInfo.name,whatIsIt,theMessage->playerInfo.id);
								c2pstr(nameString);
								SetWTitle(wind,(UInt8*)nameString);
							}
						}
						SetWRefCon(wind, (long) stuff);
						InvalPlayerWindow(stuff->id);
					}
				}
			}
		}	
		break;
			
		case kNSpPlayerLeft:
		{
		char	name[32];
		
			NSpPlayerLeftMessage *theMessage = (NSpPlayerLeftMessage *) inMessage;

			BlockMoveData(theMessage->playerName + 1, name, theMessage->playerName[0]);
			name[theMessage->playerName[0]] = '\0';
			
			printf("Got player left message: %s, player #%d\n", name, theMessage->playerID);
			fflush(stdout);
			

			if (gHost)
			{
				WindowPtr w = FindPlayersWindow(theMessage->playerID);
				if (w)
				{
					WindowStuff *stuff = (WindowStuff *) GetWRefCon(w);
					ReleaseWindowStuff(stuff);
					DisposeWindow(w);
				}
				else
				{
					printf("Didn't find the window for the player that left!!\n");
					fflush(stdout);
				}
			}	
		}
		break;
		case kNSpGameTerminated:
			printf("Got Game terminated message\n");
			fflush(stdout);
			ShutdownNetworking();
		break;
						
		default:
			break;
	}

}

//¥	--------------------	GetMessages

static void
GetMessages(void)
{	
	NSpMessageHeader	*theMessage;

	if (gNetGame == NULL)
		return;
	theMessage = NSpMessage_Get(gNetGame);	
	while (theMessage != NULL)
	{
		DoHandleMessage(theMessage);
		if (gNetGame == NULL)
			break; //get out if our message handling killed our game..
		NSpMessage_Release(gNetGame, theMessage);
		theMessage = NSpMessage_Get(gNetGame);
	}
	
}

//¥	--------------------	UpdateNetWindows

static void
UpdateNetWindows(void)
{
WindowPtr	w;

	w = FrontWindow();

	while (w != nil)
	{
		WindowStuff *stuff = (WindowStuff *) GetWRefCon(w);
		if (stuff && stuff->changed)
		{
			GrafPtr		oldPort;
		
			GetPort(&oldPort);
			
		#ifdef OP_PLATFORM_MAC_CARBON_FLAG
			SetPortWindowPort(w);
		#else
			SetPort(w);
		#endif //OP_PLATFORM_MAC_CARBON_FLAG
			
			RefreshWindow(w);
			SetPort(oldPort);
			
			stuff->changed = false;
		}
		w = GetNextWindow(w);		
	}
}

//¥	--------------------	HandleNetwork

void
HandleNetwork(void)
{
NMErr	status;

	if (!gNetGame)
		return;

	GetMessages();

//	update any windows as necessary
	UpdateNetWindows();
	
	UInt32 time = TickCount();
	WindowStuff *stuff = GetPlayersWindowStuff(gMyPlayerID);	
	
	//see if its time to send data to the host
	if (gNetGame && time > gLastUpdate + gUpdateFrequency)
	{
		//if we're not the host, we record outgoing playerinputs
		if (!gHost)
		{
			stuff->playerInputCount++;
			stuff->playerInputBytes += gPlayerMessageSize;
			stuff->totalPlayerInputBytes += gPlayerMessageSize + sizeof(NSpMessageHeader);
			stuff->changed = true;
		}
		
		NSpClearMessageHeader(&gPlayerMessage.h);
		
		gPlayerMessage.h.what = kPlayerInputMessage;
		gPlayerMessage.h.to = kNSpHostID;	/* %% was kNSpHostOnly */
		gPlayerMessage.h.messageLen = gPlayerMessageSize + sizeof(NSpMessageHeader);

		//Send my info to the host
		encode_transmission(stuff,gPlayerMessage.data,false);				
		status = NSpMessage_Send(gNetGame, &gPlayerMessage.h, gSendOptions);
		if (status != noErr)
		{
			printf("NSpMessage_Send returned error: %d\n", status);
			fflush(stdout);
		}

		gLastUpdate = time;
	}		
	
	//if we're the host, see if its time to send out data
	if (gHost && time > gLastHostUpdate + gHostUpdateFrequency)
	{

		//increment the number sent to all remoteclients... 
		{
			for (int i = 0;i < 8;i++)
			{
				if (gWindowStuff[i].id != gMyPlayerID) //its a remote stuff window
				{
					gWindowStuff[i].gameStateCount++;
					gWindowStuff[i].gameStateBytes += gGameStateMessageSize;
					gWindowStuff[i].totalGameStateBytes += gGameStateMessageSize + sizeof(NSpMessageHeader);
					gWindowStuff[i].changed = true;
				}
			}
		}
		NSpClearMessageHeader(&gGameStateMessage.h);

		gGameStateMessage.h.what = kGameStateMessage;
		gGameStateMessage.h.to = kNSpAllPlayers;
		gGameStateMessage.h.messageLen = gGameStateMessageSize + sizeof(NSpMessageHeader);

		//Send to everyone
		encode_transmission(stuff,gGameStateMessage.data,true);		
		status = NSpMessage_Send(gNetGame, &gGameStateMessage.h, gHostSendOptions);
		if (status != noErr)
		{
			printf("NSpMessage_Send returned error: %d\n", status);
			fflush(stdout);
		}
		gLastHostUpdate = time;
	}	
}

//¥	--------------------	GetInfoEnumeratePlayers

static void
GetInfoEnumeratePlayers(void)
{
NMErr				theErr;
NSpPlayerEnumerationPtr	thePlayers;
UInt32					i;

	if (nil == gNetGame)
	{
		printf("gNetGame is nil.\n");
		fflush(stdout);
		return;
	}

	theErr = NSpPlayer_GetEnumeration(gNetGame,	&thePlayers);

	if (noErr != theErr)
	{
		printf("NSpPlayer_GetEnumeration returned %ld\n", theErr);
		fflush(stdout);
		return;
	}
	
	printf("PlayerID    IP Address         Name\n");
	printf("--------    ---------------    ----------------\n");
	fflush(stdout);
	for (i = 0; i < thePlayers->count; i++)
	{
	char				name[sizeof (NSpPlayerName)];
	NSpPlayerInfoPtr	thePlayer;
	OTAddress			*address = NULL;
	NMErr			theErr;
	
		NSpPlayer_GetInfo(gNetGame, thePlayers->playerInfo[i]->id, &thePlayer);
	
		BlockMoveData(thePlayer->name + 1, name, thePlayer->name[0]);
		name[thePlayer->name[0]] = '\0';
		printf("%0.8ld    ", thePlayer->id);
		fflush(stdout);

		#if (OP_PLATFORM_MAC_MACHO)
			printf("Unavailable in posix build\n");
			fflush(stdout);
		#else
			theErr = NSpPlayer_GetOTAddress(gNetGame, thePlayer->id, &address);
		
			if ((noErr != theErr) || (NULL == address))
			{
				printf("Error %9ld", theErr);
				fflush(stdout);
			}
			else
			{
				if (AF_INET == address->fAddressType)
				{
				InetAddress	*ip;
	
					ip = (InetAddress *) address;
					
					printf("%3d.%3d.%3d.%3d",
						(ip->fHost & 0xFF000000) >> 24,
						(ip->fHost & 0x00FF0000) >> 16,
						(ip->fHost & 0x0000FF00) >>  8,
						(ip->fHost & 0x000000FF) >>  0);
					fflush(stdout);
				}
				else
				{
					printf("N/A - AppleTalk");
					fflush(stdout);
				}
			}
		#endif
		
		if (NULL != address)
			DisposePtr((Ptr) address);
		
		printf("    %s\n", name);
		fflush(stdout);
	}
}

//¥	--------------------	EnumeratePlayers

static void
EnumeratePlayers(void)
{
NMErr	theErr;
NSpPlayerEnumerationPtr	thePlayers;
UInt32		i;

	if (nil == gNetGame)
	{
		printf("gNetGame is nil.\n");
		fflush(stdout);
	}	

	theErr = NSpPlayer_GetEnumeration(gNetGame,	&thePlayers);
	if (noErr != theErr)
	{
		printf("NSpPlayer_GetEnumeration returned %ld\n", theErr);
		fflush(stdout);
		return;
	}
	
		printf("PlayerID    Name\n");
		printf("--------    -------------------------------\n");
		fflush(stdout);
	for (i = 0; i < thePlayers->count; i++)
	{
	char	name[sizeof (NSpPlayerName)];
	
		BlockMoveData(thePlayers->playerInfo[i]->name + 1, name, thePlayers->playerInfo[i]->name[0]);
		name[thePlayers->playerInfo[i]->name[0]] = '\0';
		printf("%0.8ld    %s\n", thePlayers->playerInfo[i]->id, name);
		fflush(stdout);
	}
}

 void encode_transmission(WindowStuff *stuff, UInt8 *data, Boolean isGameState)
{
	long counter;
	UInt32 *ptr = (UInt32*)data;
	UInt32 *transmitCodeCount;
	
	if (isGameState)
	{
		counter = gGameStateMessageSize/4;
		transmitCodeCount = &stuff->transmitCodeCountGameState;
	}
	else
	{
		counter = gPlayerMessageSize/4;
		transmitCodeCount = &stuff->transmitCodeCountPlayerInput;
	}
	
	while (counter > 0){
		*ptr = (*transmitCodeCount)++;
		#if (!big_endian)
			(*ptr) = SWAP4((*ptr));
		#endif
		ptr++;
		counter--;
	}
}

void decode_transmission(long fromID, WindowStuff *stuff, UInt8 *data, long length, Boolean isGameState)
{
	long errors = 0;
	UInt32 *ptr = (UInt32*)data;
	long counter = length/4;
	UInt32 firstValue = ptr[0];
	UInt32 *receiveCodeCount;
	UInt32 *erroneousCount;
	UInt32 *outOfOrderCount;
	if (length == 0)
		return;
		
	if (isGameState)
	{
		receiveCodeCount = &stuff->receiveCodeCountGameState;
		erroneousCount = &stuff->erroneousGameStates;
		outOfOrderCount = &stuff->outOfOrderGameStates;
	}
	else
	{
		receiveCodeCount = &stuff->receiveCodeCountPlayerInput;
		erroneousCount = &stuff->erroneousPlayerInputs;
		outOfOrderCount = &stuff->outOfOrderPlayerInputs;
	}
		
	//first off, we see if this packet starts where the last left off - if not, we increment out-of-order count
	#if (!big_endian)
		firstValue = SWAP4(firstValue);
	#endif

	if (firstValue != (*receiveCodeCount))
	{
		//the only time its not out-of-order is when its the first one we get and we're not the host
		if ((!stuff->first_decode) || (gHost))
		{
			(*outOfOrderCount)++;
			printf("out-of-order packet from player %d: expected start value %d; got %d;\n",fromID,*receiveCodeCount,firstValue);
			fflush(stdout);
		}
		stuff->changed = true;
	}

	stuff->first_decode = false;
	counter--;

	*receiveCodeCount = *ptr + 1; //start on value one above first

	//if we're processing a 1 word message, just increment what we expect to be next
	if (counter == 0)
		return;
	
	ptr++;

	//now go through the packet, incrementing as we go
	while (counter > 0)
	{
		#if (!big_endian)
			(*ptr) = SWAP4((*ptr));
		#endif
		if (*ptr != *receiveCodeCount)
		{
			printf("erroneous packet from player %d with start value %d\n",fromID,firstValue);
			fflush(stdout);
			(*erroneousCount)++;
			stuff->changed = true;
			return; //give up on this one
		}
		ptr++;
		(*receiveCodeCount)++;
		counter--;
	}
}
