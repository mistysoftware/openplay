/*************************************************************************************
#
#		events.c
#
#		This segment handles the basic event calls.
#
#		Author(s): 	Michael Marinkovich
#					Apple Developer Technical Support
#					marink@apple.com
#
#		Modification History: 
#
#			2/10/96		MWM 	Initial coding
#			3/2/02		ECF		Carbon/OpenPlay port				 			 
#
#		Copyright © 1992-96 Apple Computer, Inc., All Rights Reserved
#
#
#		You may incorporate this sample code into your applications without
#		restriction, though the sample code has been provided "AS IS" and the
#		responsibility for its operation is 100% yours.  However, what you are
#		not permitted to do is to redistribute the source as "DSC Sample Code"
#		after having made changes. If you're going to re-distribute the source,
#		we require that you make it clear in the source that the code was
#		descended from Apple Sample Code, but that you've made changes.
#
*************************************************************************************/

#include "OpenPlay.h"

#if (!OP_PLATFORM_MAC_MACHO)
	#include <Devices.h>
	#include <DiskInit.h>
	#include <Events.h>
	#include <Dialogs.h>
	#include <Gestalt.h>
	#include <MacWindows.h>
	#include <OSUtils.h>
	#include <Palettes.h>
	#include <ToolUtils.h>
	#include <stdio.h>
#endif

#include "NetStuff.h"

#include "App.h"
#include "Proto.h"

#if (!OP_PLATFORM_MAC_MACHO)
	#include <sioux.h>
#endif

//----------------------------------------------------------------------
//
//	Global
//
//----------------------------------------------------------------------

extern Boolean		gInBackground;
extern Boolean		gDone;
extern Boolean		gHasAbout;		// have an about box?
extern UInt32		gUpdateFrequency;
extern UInt32		gHostUpdateFrequency;

//----------------------------------------------------------------------
//
//	EventLoop - main entry and loop for all event processing
//
//
//----------------------------------------------------------------------

void EventLoop(void)
{
	EventRecord		event;
		
	gDone = false;
		
	while (! gDone)
	{
		long pause = 1;
		//pause for a millisecond unless one of the update frequencies is set to continuous
		if ((gUpdateFrequency == 0) || (gHostUpdateFrequency == 0))
			pause = 0;
		WaitNextEvent(everyEvent, &event,pause,nil);
									
		// go ahead and handle the event just 
		// in case we have and idle
		DoEvent(&event);

	}
}


//----------------------------------------------------------------------
//
//	MyGetSleep - return sleep value based upon whether or not the app
//				 is in the background.
//
//----------------------------------------------------------------------

short MyGetSleep(void)
{
	short		sleep = 30;
	
	if (gInBackground)
		sleep = 1L;

	return sleep;

}


//----------------------------------------------------------------------
//
//	CustomWindowEvent - Handles custom procs assigned to a window. 
//						Different window kinds can easily have unique event
//						handlers, ie. floaters, dialogs, documentprocs
//----------------------------------------------------------------------

void CustomWindowEvent(short eventType,WindowRef window,void *refCon)
{
#pragma unused (eventType, window, refCon)
}


//----------------------------------------------------------------------
//
//	DoEvent - event dispatcher, called by eventloop
//				
//
//----------------------------------------------------------------------

void DoEvent(EventRecord *event)
{
//	short			kind;
	long			menuChoice;
	Point			thePoint;
	Boolean			active;
	WindowRef		window;
	
	window = FrontWindow();

	switch(event->what) 
	{
		case nullEvent:
			HandleNetwork();
			break;
			
		case mouseDown:
			#if (!OP_PLATFORM_MAC_MACHO)
				if (SIOUXHandleOneEvent(event))
					return;
			#endif
			HandleMouseDown(event);
			break;
							
		case mouseUp:
			#if (!OP_PLATFORM_MAC_MACHO)
				if (SIOUXHandleOneEvent(event))
					return;		
			#endif
			break;
							
		case keyDown:
		case autoKey:
			if (event->modifiers & cmdKey) //	is cmd key down
			{
				AdjustMainMenus();
				menuChoice = MenuKey(event->message & charCodeMask);
//				kind = GetWindKind(window);
				//if (kind < kDocKind || kind > kFloatKind) 			// not our window
					HandleMenuChoice(window, (void *)&menuChoice);	// default menu
				//else	
				//	CustomWindowEvent(kMenuProc, window, (void *)&menuChoice);
			}
			break;
							
		case activateEvt:
			#if (!OP_PLATFORM_MAC_MACHO)
				if (SIOUXHandleOneEvent(event))
					return;		
			#endif
			gInBackground = event->modifiers & activeFlag;
			active = gInBackground;
			CustomWindowEvent(kActivateProc, (WindowRef)event->message, &active);
			break;
							
		case updateEvt:
			#if (!OP_PLATFORM_MAC_MACHO)
				if (SIOUXHandleOneEvent(event))
					return;		
			#endif
			UpdateWindow((WindowRef)event->message);
			break;
							
		case diskEvt:
			if (HiWord(event->message) != noErr) 
			{
				SetPt(&thePoint, 50, 50);
				#if (!OP_PLATFORM_MAC_CARBON_FLAG)
				{
					OSErr err = DIBadMount(thePoint, event->message);
				}
				#endif //OP_PLATFORM_MAC_CARBON_FLAG
			}
			break;
							
		case osEvt:
			switch ((event->message >> 24) & 0x0FF) 
			{		
				case suspendResumeMessage:	
					gInBackground = event->message & resumeFlag;
					active = gInBackground;
					CustomWindowEvent(kActivateProc, FrontWindow(), &active);
					break;
			}
			break;
	
		case kHighLevelEvent:
			AEProcessAppleEvent(event);
			break;
	}

}			



//----------------------------------------------------------------------
//
//	DoIdle - handle Idle events
//				
//
//----------------------------------------------------------------------

void DoIdle(WindowRef window, void *refCon)
{

}


//----------------------------------------------------------------------
//
//	HandleMouseDown - 
//				
//
//----------------------------------------------------------------------

void HandleMouseDown(EventRecord *event)
{
	long			menuChoice;
	short			thePart;
//	short			kind;
	WindowRef		window;
		

	thePart = FindWindow(event->where,&window);
		
	switch(thePart) 
	{
		case inMenuBar:
			AdjustMainMenus();

			menuChoice = MenuSelect(event->where);
			window = FrontWindow();
//			kind = GetWindKind(window);
//			if (kind < kDocKind || kind > kAboutKind) 			// not our window
				HandleMenuChoice(window, (void *)&menuChoice);	// default menu
//			else	
//				CustomWindowEvent(kMenuProc, window, (void *)&menuChoice);
			break;

		case inContent:
			if (window != FrontWindow())
				SelectWindow(window);
//			else
//				CustomWindowEvent(kInContentProc, window, &event->where);
	
			break;

		case inSysWindow:
			#if (!OP_PLATFORM_MAC_CARBON_FLAG)
				SystemClick(event,window);
			#endif //OP_PLATFORM_MAC_CARBON_FLAG
			
			break;
												
		case inDrag:
			if (window != FrontWindow())
				SelectWindow(window);
				
			#if (OP_PLATFORM_MAC_CARBON_FLAG)
			{
				BitMap screenBitMap;
				GetQDGlobalsScreenBits(&screenBitMap);
				DragWindow(window, event->where,&screenBitMap.bounds);
			}
			#else
				DragWindow(window, event->where,&qd.screenBits.bounds);			
			#endif //OP_PLATFORM_MAC_CARBON_FLAG
			
			break;
						
		case inGoAway:
//			if (TrackGoAway(window, event->where))
//				RemoveWindow(window);
			break;
						
		case inZoomIn:
		case inZoomOut:
			if (TrackBox(window,event->where,thePart)) 
				CustomWindowEvent(kInZoomProc, window,&thePart);
			break;
						
		case inGrow:
//			CustomWindowEvent(kInGrowProc, window, &event->where);
			break;
	}
	
}


//----------------------------------------------------------------------
//
//	HandleMenuChoice - 
//				
//
//----------------------------------------------------------------------

void HandleMenuChoice(WindowRef window, void *refCon)
{
	long 		menuChoice;
	short		item, menu;
	Str255		daName;
	
	menuChoice = *(long *)refCon;
	
	item = LoWord(menuChoice);
	menu = HiWord(menuChoice);
		
	switch(menu) 
	{
		case mApple:
			switch(item) 
			{					
				case iAbout:
//				CreateWindow( rAboutWin, NULL, NULL, "\pAbout", true, 0, kAboutKind, (WindowRef)-1, true, NULL );
				NoteAlert( rAboutAlert, NULL );
				break;

				default:
					GetMenuItemText(GetMenuHandle(mApple),item,daName);
					#if (!OP_PLATFORM_MAC_CARBON_FLAG)
					{
						short		daRefNum;
						daRefNum = OpenDeskAcc(daName);
					}
					#endif //!OP_PLATFORM_MAC_CARBON_FLAG
					break;
			}	
			break;
					
		case mFile:
			switch(item) 
			{
				case iHost:
					DoHost();
					break;
				case iJoin:
					DoJoin();
					break;
				case iLeave:
					ShutdownNetworking();
					break;
				case iQuit:
					gDone = true;
					break;
					
				default:
					break;	
			}
			break;
					
		default:
			HandleNetMenuChoice(menu, item);
			break;	
		
	}
	
	HiliteMenu(0);
	
}


//----------------------------------------------------------------------
//
//	AdjustMainMenus - 
//				
//
//----------------------------------------------------------------------

void AdjustMainMenus(void)
{
	MenuRef		menu;
	
	menu = GetMenuHandle(mFile);
	
	#if (OP_PLATFORM_MAC_CARBON_FLAG)
		EnableMenuItem(menu, iHost);
		EnableMenuItem(menu, iJoin);
		EnableMenuItem(menu, iLeave);
	#else
		EnableItem(menu, iHost);
		EnableItem(menu, iJoin);
		EnableItem(menu, iLeave);
	#endif

	if (gNetGame)
	{
		#if (OP_PLATFORM_MAC_CARBON_FLAG)
			DisableMenuItem(menu, iHost);
			DisableMenuItem(menu, iJoin);
			EnableMenuItem(menu, iLeave);
		#else
			DisableItem(menu, iHost);
			DisableItem(menu, iJoin);
			EnableItem(menu, iLeave);
		#endif //OP_PLATFORM_MAC_CARBON_FLAG
	}
	else
	{
		#if (OP_PLATFORM_MAC_CARBON_FLAG)
			EnableMenuItem(menu, iHost);
			EnableMenuItem(menu, iJoin);
			DisableMenuItem(menu, iLeave);
		#else
			EnableItem(menu, iHost);
			EnableItem(menu, iJoin);
			DisableItem(menu, iLeave);
		#endif //OP_PLATFORM_MAC_CARBON_FLAG
	}	


	AdjustNetMenus();
}


//----------------------------------------------------------------------
//
//	HandleContentClick - 
//				
//
//----------------------------------------------------------------------

void HandleContentClick(WindowRef window, void *refCon)
{

}


//----------------------------------------------------------------------
//
//	HandleZoomClick - 
//				
//
//----------------------------------------------------------------------

void HandleZoomClick(WindowRef window, void *refCon)
{

}


//----------------------------------------------------------------------
//
//	HandleGrow - 
//				
//
//----------------------------------------------------------------------

void HandleGrow(WindowRef window, void *refCon)
{

}


//----------------------------------------------------------------------
//
//	UpdateWindow - update dispatcher for document windows.
//				 
//
//----------------------------------------------------------------------

void UpdateWindow(WindowRef window) 
{
	GrafPtr		oldPort;
	
	
	GetPort(&oldPort);
	
	#if (OP_PLATFORM_MAC_CARBON_FLAG)
		SetPortWindowPort(window);
	#else
		SetPort(window);
	#endif //OP_PLATFORM_MAC_CARBON_FLAG

	BeginUpdate(window);
//	CustomWindowEvent(kUpdateProc, window, nil);
	RefreshWindow(window);
	EndUpdate(window);
	
	SetPort(oldPort);

}


//----------------------------------------------------------------------
//
//	DoActivate - 
//				 
//
//----------------------------------------------------------------------

void DoActivate(WindowRef window, void *refCon)
{
//	Boolean		becomingActive;
//	DocHnd		doc;
	
	#if (OP_PLATFORM_MAC_CARBON_FLAG)
		SetPortWindowPort(window);
	#else
		SetPort(window);
	#endif
/*
	doc = (DocHnd)GetWRefCon(window);

	if(doc != nil && GetIsAppWindow(window)) 
	{
		becomingActive = *(Boolean *)refCon;
		if (becomingActive) 
		{
		}
		
		else 
		{
		}
	}
*/
}

