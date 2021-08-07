/*************************************************************************************
#
#		main.c
#
#		This is the main entry point.
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
	#include <NumberFormatting.h>
	#include <Sound.h>
	#include <TextUtils.h>
	#include <ToolUtils.h>
#endif

#include <stdio.h>

#if (!OP_PLATFORM_MAC_MACHO)
	#include <sioux.h>
#endif

#include "NetStuff.h"

#include "App.h"
#include "Global.h"
#include "Proto.h"

//----------------------------------------------------------------------
//
//	main - main entry point of our program
//
//
//----------------------------------------------------------------------

int main(void)
{
	OSErr			err;
	short			m = 5;
		
	#if (!OP_PLATFORM_MAC_MACHO)
		SIOUXSettings.autocloseonquit = false;
		SIOUXSettings.asktosaveonclose = true;
		SIOUXSettings.initializeTB = false;
		SIOUXSettings.setupmenus = false;
		SIOUXSettings.standalone = true;
	#endif

	#if (!OP_PLATFORM_MAC_CARBON_FLAG)
		MaxApplZone();
	#endif //!OP_PLATFORM_MAC_CARBON_FLAG
	
	for (;m == 0;m--) 				// alloc the master pointers
	{
		MoreMasters();
	}
		
	err = Initialize();
	if( !err )
	{
		EventLoop();

		ShutdownNetworking();
	}	
	AERemove();	
//	ExitToShell();
	return 0;
}


//----------------------------------------------------------------------
//
//	HandleError - basic error notification procedure
//
//
//----------------------------------------------------------------------

void HandleError(short errNo,Boolean fatal)
{
	DialogPtr			errDialog;
	short				itemHit;
	short				c;
	Handle				button;
	short				itemType;
	Rect				itemRect;
	StringHandle		errString;
	Str255				numString;
	GrafPtr				oldPort;
	
	GetPort(&oldPort);
	SysBeep(30);
	
	#if (OP_PLATFORM_MAC_CARBON_FLAG)
	{
		Cursor theCursor;
		GetQDGlobalsArrow(&theCursor);
		SetCursor(&theCursor);
	}
	#else
		SetCursor(&qd.arrow);
	#endif //OP_PLATFORM_MAC_CARBON_FLAG
	
	errDialog = GetNewDialog(rErrorDlg, nil, (WindowPtr) -1);
	if (errDialog != nil) 
	{
		#if (OP_PLATFORM_MAC_CARBON_FLAG)
		{
			Pattern grayPat;
			GetQDGlobalsGray(&grayPat);
			ShowWindow(GetDialogWindow(errDialog));
			SetPortWindowPort(GetDialogWindow(errDialog));
			PenPat(&grayPat);
		}
		#else
			ShowWindow(errDialog);
			SetPort(errDialog);
			PenPat(&qd.gray);								// frame user areas
		#endif //OP_PLATFORM_MAC_CARBON_FLAG
		
		for (c = 7;c < 9;c++) 
		{
			GetDialogItem(errDialog,c,&itemType,&button,&itemRect);
			FrameRect(&itemRect);
		}
		PenNormal();

		GetDialogItem( errDialog, fatal ? 2 : 1, &itemType, &button, &itemRect );
		HiliteControl( (ControlHandle)button, 255);		 //	unhilite appropriate button
		
		SetDialogDefaultItem(errDialog,fatal  ? 1 : 2);
		
		NumToString( errNo, numString );
		if ( errNo < 0 )
			errNo = -errNo + 200;
		errString = GetString( 128 + errNo );
		if ( errString == nil )
				errString = GetString( 128 );
		HLock((Handle)errString);
		
		ParamText( numString, *errString, nil, nil );
	
		HUnlock((Handle)errString);
		
		ModalDialog( nil, &itemHit );
		
		SetPort( oldPort );

		#if (OP_PLATFORM_MAC_CARBON_FLAG)
			DisposeDialog(errDialog);
		#else
			DisposeWindow( errDialog );
		#endif //OP_PLATFORM_MAC_CARBON_FLAG
	}
	else
		ExitToShell();		// since didn't have mem to open dialog assume the worst
			
	if (fatal) 
		ExitToShell();		//	it's a bad one, get out of here
	
}


//----------------------------------------------------------------------
//
//	HandleAlert - display alert and then exit to shell
//
//
//----------------------------------------------------------------------

void HandleAlert(short alertID)
{
	short			item;
	
	
	item = Alert(alertID,nil);
	ExitToShell();

}