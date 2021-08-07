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
 
//	------------------------------	Includes

#ifndef __OPENPLAY__
#include 			"OpenPlay.h"
#endif
#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif

#include "resource.h"

#include "OPUtils.h"
#include "portable_files.h"
#include "op_definitions.h"
#include "op_globals.h"
#include "String_Utils.h"

//	------------------------------	Public Variables


//	------------------------------	Private Declarations

#define kNSpIPAddrStrSize 	 128

//	------------------------------	Private Variables

static NMBoolean _result;
static char _IPAddressStr[ kNSpIPAddrStrSize ] = "127.0.0.1";
static char _GameStr[ kNSpIPAddrStrSize ] = "myGame";
static char _IPPortStr[ kNSpStr32Len ] = "2407";
static char _PlayerStr[ kNSpStr32Len ] = "Player";
static char _PasswordStr[ kNSpStr32Len ] = "";

//----------------------------------------------------------------------------------------
// Dialog callback procedure
//----------------------------------------------------------------------------------------

static BOOL CALLBACK _DlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
	static NSpEventProcPtr  eventProc;

	switch(msg)
	{
		case WM_INITDIALOG:
			eventProc = (NSpEventProcPtr)lParam;

			SetDlgItemText( hDlg, IDC_EDIT_GAME, IDD_DLG_HOST == _result ? _GameStr : _IPAddressStr );
			SetDlgItemText( hDlg, IDC_EDIT_PORT, _IPPortStr );
			SetDlgItemText( hDlg, IDC_EDIT_PLAYER, _PlayerStr );
			SetDlgItemText( hDlg, IDC_EDIT_PSWD, _PasswordStr );

			return( TRUE );

		case WM_COMMAND:
			if (wParam == IDCANCEL)
			{
				_result = 0;
				goto dlgdone;
			}
			else if( wParam == IDOK )
			{
				_result = 1;

				GetDlgItemText( hDlg, IDC_EDIT_GAME, IDD_DLG_HOST == _result ? _GameStr : _IPAddressStr, kNSpIPAddrStrSize );
				GetDlgItemText( hDlg, IDC_EDIT_PORT, _IPPortStr, kNSpStr32Len );
				GetDlgItemText( hDlg, IDC_EDIT_PLAYER, _PlayerStr, kNSpStr32Len );
				GetDlgItemText( hDlg, IDC_EDIT_PSWD, _PasswordStr, kNSpStr32Len );
dlgdone:
				EndDialog( hDlg, FALSE );
				return( TRUE );
			}
			else if( eventProc )	// call our event procedure
			{
				NMEvent er;

				er.hWnd = hDlg; er.msg = msg; er.wParam = wParam; er.lParam = lParam;
				eventProc( &er );
			}
	}
	return( FALSE );
}

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

	if ( ioProtocolList == NULL )
	{
		return( false );
	}

	// Setup params for dialog
	if( ioGameName )
		doCopyP2CStr( ioGameName, _GameStr );
	if( ioPlayerName )
		doCopyP2CStr( ioPlayerName, _PlayerStr );
	if( ioPassword )
		doCopyP2CStr( ioPassword, _PasswordStr );

	// Run Dialog
	_result = IDD_DLG_HOST;
	DialogBoxParam( gOp_globals.dll_instance, MAKEINTRESOURCE(IDD_DLG_HOST), GetTopWindow( NULL ), (DLGPROC)_DlgProc, (LPARAM)inEventProcPtr );
	if ( _result )
	{
		NMUInt16 mIPPort = atoi(_IPPortStr);
	
		protocolRef = NSpProtocol_CreateIP( mIPPort, 0UL, 0UL );		

		if ( protocolRef == NULL )
		{
			_result = false;
		}
		else
		{
			NSpProtocolList_Append( ioProtocolList, protocolRef );

			// Setup results from dialog
			if( ioGameName )
				doCopyC2PStrMax( _GameStr, ioGameName, kNSpStr32Len );
			if( ioPlayerName )
				doCopyC2PStrMax( _PlayerStr, ioPlayerName, kNSpStr32Len );
			if( ioPassword )
				doCopyC2PStrMax( _PasswordStr, ioPassword, kNSpStr32Len );
		}		
	}
	return( _result );
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
	NSpAddressReference  addrRef;
		
	UNUSED_PARAMETER(inEntityListLabel);
	UNUSED_PARAMETER(inGameType);

	// Setup params for dialog
	if( ioName )
		doCopyP2CStr( ioName, _PlayerStr );
	if( ioPassword )
		doCopyP2CStr( ioPassword, _PasswordStr );

	// Run Dialog
	_result = IDD_DLG_JOIN;
	DialogBoxParam( gOp_globals.dll_instance, MAKEINTRESOURCE(IDD_DLG_JOIN), GetTopWindow( NULL ), (DLGPROC)_DlgProc, (LPARAM)inEventProcPtr );
	if ( !_result )
	{
		return( NULL );
	}

	// Setup results from dialog
	if( ioName )
		doCopyC2PStrMax( _PlayerStr, ioName, kNSpStr32Len );
	if( ioPassword )
		doCopyC2PStrMax( _PasswordStr, ioPassword, kNSpStr32Len );

	addrRef = NSpCreateIPAddressReference( _IPAddressStr, _IPPortStr );

	return( addrRef );
}
