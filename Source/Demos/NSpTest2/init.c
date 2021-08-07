/***********************************************************************
#
#		init.c
#
#		basic initialization code.
#
#		Author: Michael Marinkovich
#				Apple Developer Technical Support
#
#
#		Modification History: 
#
#			6/4/95		MWM 	Initial coding					 
#			10/12/95	MWM		cleaned up
#			3/2/02		ECF		Carbon/OpenPlay port				 
#
#		Copyright © 1992-95 Apple Computer, Inc., All Rights Reserved
#
#
***********************************************************************/
#include "OpenPlay.h"

#if (!OP_PLATFORM_MAC_MACHO)
	#include <AppleEvents.h>
	#include <Displays.h>
	#include <Events.h>
	#include <Fonts.h>
	#include <Gestalt.h>
	#include <Menus.h>
	#include <OSUtils.h>
#endif

#include "NetStuff.h"
#include "OPUtils.h"

#include "App.h"
#include "Proto.h"

extern Boolean 			gHasDMTwo;
extern Boolean 			gHasDrag;
extern Boolean			gInBackground;
extern short			gWindCount;


//----------------------------------------------------------------------
//
//	Initialize - the main entry point for the initialization
//
//
//----------------------------------------------------------------------

OSErr Initialize(void)
{
	OSErr		err = noErr;
	NMErr	status;	
	
	ToolBoxInit();
	CheckEnvironment();
	err = InitApp();
	status = InitNetworking('BLAm');
	
	return err;
}


//----------------------------------------------------------------------
//
//	ToolBoxInit - initialization all the needed managers
//
//
//----------------------------------------------------------------------

void ToolBoxInit(void)
{

	#if (!OP_PLATFORM_MAC_CARBON_FLAG)
		InitGraf(&qd.thePort);
		InitFonts();
		InitWindows();
		InitMenus();
		TEInit();
		InitDialogs(nil);
	#endif //!OP_PLATFORM_MAC_CARBON_FLAG
	
	InitCursor();
		
	FlushEvents(everyEvent, 0);

}


//----------------------------------------------------------------------
//
//	CheckEnvironment - make sure we can run with current sys and managers.
//					   Also initialize globals - have drag & drop
//					  
//----------------------------------------------------------------------

void CheckEnvironment(void)
{
	
	// your stuff here
}


//----------------------------------------------------------------------
//
//	InitApp - initialization all the application specific stuff
//
//
//----------------------------------------------------------------------

OSErr InitApp(void)
{
	OSErr				err;

	// init AppleEvents
	err = AEInit();
	if( !err )
		err = MenuSetup();

	// init any globals
	gWindCount = 1;

	return err;
}


//----------------------------------------------------------------------
//
//	MenuSetup - 
//
//
//----------------------------------------------------------------------

OSErr MenuSetup(void)
{
	Handle			menu;
		
		
	menu = GetNewMBar(rMBarID);		//	get our menus from resource
	if( menu )
	{
		SetMenuBar(menu);
		DisposeHandle(menu);
	
#if (!OP_PLATFORM_MAC_CARBON_FLAG)
		AppendResMenu(GetMenuHandle(mApple ), 'DRVR');		//	add apple menu items
#endif //!OP_PLATFORM_MAC_CARBON_FLAG

		DrawMenuBar();
		return( noErr );
	}
	else
		return( fnfErr );
}
