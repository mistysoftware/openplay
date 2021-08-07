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
 
//	------------------------------	Includes

#ifndef __OPENPLAY__
#include 			"OpenPlay.h"
#endif
#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif


#ifdef OP_PLATFORM_MAC_MACHO
	#include <Carbon/Carbon.h>
#else
	#include <Dialogs.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <cstring>
	#include "OTUtils.h"
#endif


#include "OPUtils.h"
#include "portable_files.h"
#include "op_definitions.h"
#include "op_globals.h"
#include "String_Utils.h"

//	------------------------------	Public Variables

extern OSType  		gCreatorType;

//	------------------------------	Private Declarations

	#define	kNSpDialogResID 	 2500
	#define kNSpIPAddrStrSize 	 128


	typedef enum  {                 // OK and Cancel are already defined in Dialogs.h
		kIPJoinInfoLabel = 3, 
		kIPHostInfoLabel, 
		kIPHostLabel, 
		kIPHostText, 
		kIPPortLabel,
		kIPPortText,

		kATJoinInfoLabel, 
		kATHostInfoLabel, 
		kATGameNameLabel, 
		kATGameNameText, 
		
		kPlayerNameLabel,
		kPlayerNameText,
		kPasswordLabel,
		kPasswordText,

		kProtocolLabel,
		kIPProtocolButton, 
		kAppleTalkProtocolButton

	} eNSpDialogItem;


	typedef enum  {
	    kNSpJoinDialog, 
	    kNSpHostDialog
	    
	} eDialogMode;

	enum  {
		kSysEnvironsVersion = 1 
	};


	// Portable to international keyboards?
	enum  {
	    kEnterKey = 0x03, 
		kReturnKey = 0x0D, 
		kEscKey = 0x1B
	};

//	------------------------------	Private Variables

static unsigned char     gIPAddressStr[ kNSpIPAddrStrSize ];
static unsigned char     gIPPortStr[ kNSpStr32Len ];
static NMBoolean         gIPSelected = true;


//----------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------
static ControlHandle 
GetControlHandle( DialogPtr theBox, short theGetItem )
{
    short itemType;
    Rect itemRect;
    Handle theHandle;
    
    GetDialogItem( theBox, theGetItem, &itemType, &theHandle, &itemRect );

    return( (ControlHandle) theHandle );

}	// End  GetControlHandle()

//----------------------------------------------------------------------------------------
static void
HideATItems( DialogPtr dialog, eNSpDialogItem theATInfoLabel )
{
	Handle tempHandle;

	tempHandle = (Handle) GetControlHandle( dialog, kAppleTalkProtocolButton );
	SetControlValue( (ControlHandle) tempHandle, false );

	HideDialogItem( dialog, theATInfoLabel );	
	HideDialogItem( dialog, kATGameNameLabel );
	HideDialogItem( dialog, kATGameNameText );
}

//----------------------------------------------------------------------------------------
static void
ShowATItems( DialogPtr dialog, eNSpDialogItem theATInfoLabel )
{
	Handle tempHandle;

	tempHandle = (Handle) GetControlHandle( dialog, kAppleTalkProtocolButton );
	SetControlValue( (ControlHandle) tempHandle, true );

	ShowDialogItem( dialog, theATInfoLabel );	
	ShowDialogItem( dialog, kATGameNameLabel );
	ShowDialogItem( dialog, kATGameNameText );
}

//----------------------------------------------------------------------------------------
static void
HideIPItems( DialogPtr dialog, eNSpDialogItem theIPInfoLabel )
{
	Handle tempHandle;

	tempHandle = (Handle) GetControlHandle( dialog, kIPProtocolButton );
	SetControlValue( (ControlHandle) tempHandle, false );
	
	HideDialogItem( dialog, theIPInfoLabel );
	HideDialogItem( dialog, kIPHostLabel );
	HideDialogItem( dialog, kIPHostText );
	HideDialogItem( dialog, kIPPortLabel );
	HideDialogItem( dialog, kIPPortText );
}				

//----------------------------------------------------------------------------------------
static void
ShowIPItems( DialogPtr dialog, eNSpDialogItem theIPInfoLabel )
{
	Handle tempHandle;

	tempHandle = (Handle) GetControlHandle( dialog, kIPProtocolButton );
	SetControlValue( (ControlHandle) tempHandle, true );
	
	ShowDialogItem( dialog, theIPInfoLabel );
	ShowDialogItem( dialog, kIPHostLabel );
	ShowDialogItem( dialog, kIPHostText );
	ShowDialogItem( dialog, kIPPortLabel );
	ShowDialogItem( dialog, kIPPortText );
}				

//----------------------------------------------------------------------------------------
static NMBoolean
DoDialogLoop( DialogPtr dialog, 
              eNSpDialogItem  theATInfoLabel,
              eNSpDialogItem  theIPInfoLabel/*,
              unsigned char    ioGameName[ kNSpStr32Len ],
              unsigned char    ioPlayerName[ kNSpStr32Len ],
              unsigned char    ioPassword[ kNSpStr32Len ]*/ )
{  
	NMSInt16   hitItem = 0;
    NMBoolean  status = true;
	
	#ifdef OP_API_NETWORK_OT
		NMBoolean	tcpPresent = OTUtils::HasOpenTransportTCP();
	#else
		NMBoolean	tcpPresent = true;
	#endif
	
	do {
		ModalDialog( NULL, &hitItem );

        switch ( hitItem ) {

            case kIPProtocolButton:
 
 				if ( tcpPresent ) {
 				
	 				gIPSelected = true;
	 				ShowIPItems( dialog, theIPInfoLabel );
	 				HideATItems( dialog, theATInfoLabel );
	 			}
 				
				break;
             
			case kAppleTalkProtocolButton:
			
				gIPSelected = false;
 				ShowATItems( dialog, theATInfoLabel );
 				HideIPItems( dialog, theIPInfoLabel );
					
				break;
        }
 
    } while ( hitItem != ok && hitItem != cancel );
	
	if ( hitItem == cancel ) {
		status = false;
	}
	
	return( status );
	
}	// End  DoDialogLoop()


//----------------------------------------------------------------------------------------
static NMBoolean 
NSpDoModalDialog( eDialogMode mode,
                  unsigned char    ioGameName[ kNSpStr32Len ],
                  unsigned char    ioPlayerName[ kNSpStr32Len ],
                  unsigned char    ioPassword[ kNSpStr32Len ],
                  NSpEventProcPtr  inEventProcPtr )
{
	DialogPtr dialog = NULL;
	Handle tempHandle;
	eNSpDialogItem  theATInfoLabel;
	eNSpDialogItem  theIPInfoLabel;
	unsigned char   portPStr[ kNSpStr32Len ];
	NMBoolean       status = true;
	NMBoolean		aCloseResourceFile = false;
	
	UNUSED_PARAMETER(inEventProcPtr);
	
	machine_mem_zero( gIPAddressStr, kNSpIPAddrStrSize );
	machine_mem_zero( gIPPortStr, kNSpStr32Len );
	
	if (gOp_globals.res_refnum == -1)
	{
#if OP_PLATFORM_MAC_CFM
		gOp_globals.res_refnum = FSpOpenResFile((FSSpec *)&gOp_globals.file_spec,fsRdPerm);
#else
		gOp_globals.selfBundleRef = CFBundleGetBundleWithIdentifier ( CFSTR ( "com.apple.openplay" ) );
		if( gOp_globals.selfBundleRef )
			gOp_globals.res_refnum = CFBundleOpenBundleResourceMap ( gOp_globals.selfBundleRef );
		else
			DEBUG_PRINT( "ERROR in NSpDoModalDialog selfBundleRef is NULL" );
#endif
		aCloseResourceFile = true;
	}
	
	
	dialog = GetNewDialog( kNSpDialogResID, NULL, (WindowPtr)-1 );

    if ( dialog == NULL ) {
    	DEBUG_PRINT( "GetNewDialog failed" );
    	return( false );
    }
    
    if ( mode == kNSpJoinDialog ) {
	    theATInfoLabel = kATJoinInfoLabel;
		theIPInfoLabel = kIPJoinInfoLabel;
	}
	else {
	    theATInfoLabel = kATHostInfoLabel;
		theIPInfoLabel = kIPHostInfoLabel;	
	}


    /* set up our user items for various things */ 
    SetDialogDefaultItem( dialog, ok );
    SetDialogCancelItem( dialog, cancel );
    SetDialogTracksCursor( dialog, true );
        
	SelectDialogItemText( (DialogPtr) dialog, kIPHostText, 0, 0 );

	HideDialogItem( dialog, kIPJoinInfoLabel );	
	HideDialogItem( dialog, kIPHostInfoLabel );	

	HideDialogItem( dialog, kATJoinInfoLabel );	
	HideDialogItem( dialog, kATHostInfoLabel );	

	HideDialogItem( dialog, kIPProtocolButton );	
	HideDialogItem( dialog, kAppleTalkProtocolButton );
	
	HideATItems( dialog, theIPInfoLabel );
	ShowIPItems( dialog, theATInfoLabel );

	tempHandle = (Handle) GetControlHandle( dialog, kIPProtocolButton );
	SetControlValue( (ControlHandle) tempHandle, false );

	tempHandle = (Handle) GetControlHandle( dialog, kAppleTalkProtocolButton );
	SetControlValue( (ControlHandle) tempHandle, true );
	
	//	Set the value of the port field and others
	
	UInt16 port = ( gCreatorType % (32760 - 1024)) + 1024;
	psprintf(portPStr, "%i", port );
	SetDialogItemText( (Handle) GetControlHandle( dialog, kIPPortText ), portPStr );

	SetDialogItemText( (Handle) GetControlHandle( dialog, kATGameNameText ), ioGameName );

	SetDialogItemText( (Handle) GetControlHandle( dialog, kPlayerNameText ), ioPlayerName );

	SetDialogItemText( (Handle) GetControlHandle( dialog, kPasswordText ), ioPassword );

	ShowWindow( (WindowPtr) dialog );
    
	DrawDialog( dialog );
	
	status = DoDialogLoop( dialog, 
	                       theATInfoLabel, 
	                       theIPInfoLabel/*,
	                       ioGameName,
	                       ioPlayerName,
	                       ioPassword*/ );

	if ( status == true ) {
		
		GetDialogItemText( (Handle) GetControlHandle( dialog, kIPHostText ), gIPAddressStr );
		GetDialogItemText( (Handle) GetControlHandle( dialog, kIPPortText ), gIPPortStr );
		
		GetDialogItemText( (Handle) GetControlHandle( dialog, kATGameNameText ), ioGameName );
		GetDialogItemText( (Handle) GetControlHandle( dialog, kPlayerNameText ), ioPlayerName );
		GetDialogItemText( (Handle) GetControlHandle( dialog, kPasswordText ), ioPassword );
	}

	DisposeDialog( dialog );

	if (aCloseResourceFile)
	{
#if OP_PLATFORM_MAC_CFM
		CloseResFile(gOp_globals.res_refnum);
#else
		CFBundleCloseBundleResourceMap( gOp_globals.selfBundleRef, gOp_globals.res_refnum  );
#endif
		gOp_globals.res_refnum = -1;
	}

	return( status );
	
} // End  NSpDoModalJoinDialog()

//----------------------------------------------------------------------------------------
// Host Dialog
//----------------------------------------------------------------------------------------

NMBoolean              
NSpDoModalHostDialog( NSpProtocolListReference	ioProtocolList,
                      unsigned char    ioGameName[ kNSpStr32Len ],
                      unsigned char    ioPlayerName[ kNSpStr32Len ],
                      unsigned char    ioPassword[ kNSpStr32Len ],
                      NSpEventProcPtr  inEventProcPtr )
{
	NSpProtocolReference  protocolRef;
	NMBoolean status;

	if ( ioProtocolList == NULL ) {
		return( false );
	}

	status = NSpDoModalDialog( kNSpHostDialog, 
	                           ioGameName, 
	                           ioPlayerName, 
	                           ioPassword, 
	                           inEventProcPtr );
	
	if ( status == false ) {
		return( status );
	}
	else {
	
		if ( gIPSelected )
		{
			char portStr[kNSpStr32Len];

			doCopyP2CStr(gIPPortStr, portStr);
			NMUInt16 mIPPort = atoi(portStr);
		
			protocolRef = NSpProtocol_CreateIP( mIPPort, 0UL, 0UL );		

		}
		else {

			protocolRef = NSpProtocol_CreateAppleTalk( (unsigned char*) ioGameName, 
			                                           (unsigned char*) gCreatorType, 
			                                           0UL, 
			                                           0UL );			
		}
		
		if ( protocolRef == NULL ) {
			status = false;
		}
		else {
			NSpProtocolList_Append( ioProtocolList, protocolRef );
		}		
	}
	
	return( status );
}

//----------------------------------------------------------------------------------------
// Join Dialog
//----------------------------------------------------------------------------------------

NSpAddressReference  
NSpDoModalJoinDialog( const unsigned char  inGameType[ kNSpStr32Len ],
                      const unsigned char  inEntityListLabel[256],
                      unsigned char        ioName[ kNSpStr32Len ],
                      unsigned char        ioPassword[ kNSpStr32Len ],
                      NSpEventProcPtr      inEventProcPtr )
{
	NSpAddressReference  addrRef = NULL;
	char     theATZone[ kNSpStr32Len ] = "*";
	unsigned char     gameNameStr[ kNSpStr32Len ] = "";
	NMBoolean  success;
		
	UNUSED_PARAMETER(inEntityListLabel);

	success = NSpDoModalDialog( kNSpJoinDialog, 
	                           gameNameStr, 
	                           ioName, 
	                           ioPassword, 
	                           inEventProcPtr );

	if ( !success ) {
		return( NULL );
	}
	
	if ( gIPSelected ) {
	
		//ECF020111 we gotta convert these strings from pascal to c before shipping them in
		doP2CStr((char*)gIPAddressStr);
		doP2CStr((char*)gIPPortStr);
		addrRef = NSpCreateIPAddressReference( (char*) gIPAddressStr, (char*) gIPPortStr );
	}
	else {
	
		if ( inGameType == NULL ) {
		
			// Skanky. We should change this function to take a NMUInt32
			// as its Game Type parameter....
			addrRef = NSpCreateATlkAddressReference( (char*) gameNameStr, 
			                                         (char*) gCreatorType, 
			                                         theATZone );
		}
		else {
			addrRef = NSpCreateATlkAddressReference( (char*) gameNameStr, 
			                                         (char*) inGameType, 
			                                         theATZone );
		}			
	}

	return( addrRef );
}
