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

/*
	module_hi.c
*/

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
#include "OPUtils.h"
#include "NetModule.h"

#include "ip_enumeration.h"
#include "tcp_module.h"

	typedef struct ChildControlInfo
	{
		TCHAR		className[32];
		TCHAR		controlText[128];
		DWORD		winStyle;
		int			left;
		int			top;
		int			right;
		int			bottom;
	} ChildControlInfo;

	static const NMRect gNeededRect = { 0,0,70,250 };

	enum {	kJoinTitle = 0,
			kHostLabel,
			kHostText,
			kPortLabel,
			kPortText,
			kNumControls };

	static HWND gStoreControls[kNumControls];
	static NMSInt32 gCommandNumberBase;

	// sjb 19990419 These go with the enum above which enumerates the items we add.
	static const ChildControlInfo gItemsInfo[kNumControls] = {
		{ TEXT("static"), TEXT("Enter the host name and port of the game you wish to join:"),
		  WS_CHILD | WS_VISIBLE | SS_LEFT | WS_GROUP, 10, 10, 240, 20 },
		{ TEXT("static"), TEXT("Host Name:"),
		  WS_CHILD | WS_VISIBLE | SS_RIGHT | WS_GROUP, 10, 30, 90, 40 },
		{ TEXT("edit"), TEXT(""),
		  WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP, 100, 30, 240, 40 },
		{ TEXT("static"), TEXT("Host Port:"),
		  WS_CHILD | WS_VISIBLE | SS_RIGHT | WS_GROUP, 10, 50, 90, 60 },
		{ TEXT("edit"), TEXT(""),
		  WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP, 100, 50, 170, 60 }
	};

static TEXTMETRIC gDialogTextInfo;

static void CreateModuleControls(	HWND dialog,
                                 	const TEXTMETRIC *winMetrics,
									const NMRect *relativeRect,
                                	const int commandNumberBase);

// sjb 19990316 need to specify reserved command IDs

/* Dialog functions */


//----------------------------------------------------------------------------------------
// NMGetRequiredDialogFrame
//----------------------------------------------------------------------------------------

void
NMGetRequiredDialogFrame(
	NMRect *r, 
	NMConfigRef inConfig)
{
	UNUSED_PARAMETER(inConfig)

	*r = gNeededRect;

	r->left /= 4;
	r->top /= 8;
	r->right /= 4;
	r->bottom /= 8;
}

//----------------------------------------------------------------------------------------
// NMSetupDialog
//----------------------------------------------------------------------------------------

NMErr
NMSetupDialog(
	NMDialogPtr 	dialog, 
	NMSInt16	whereFrameItem,
	NMSInt16	inBaseItem, 
	NMConfigRef inConfig)
{
	NMErr err= kNMNoError;
	HDC			windowDrawContext;
	HWND		windowModuleBoxItem;
	NMRect		windowModuleBox;
	NMRect		mainWindowBox;
	
	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",kNMParameterErr);
	op_vassert_return((inConfig->cookie==config_cookie),"inConfig->cookie==config_cookie",kNMParameterErr);

	windowDrawContext = GetDC(dialog);
	GetTextMetrics(windowDrawContext,&gDialogTextInfo);
	ReleaseDC(dialog,windowDrawContext);

// sjb 19990318 this code doesn't draw into the item window that the
// client provides. It draws above it into the parent window instead.
// This is because the test app provides a null rectangle for its
// user item. Maybe I should change this, but I'll worry about it later.

// remember command number base
	gCommandNumberBase = inBaseItem;

// sjb 19990318 get the box item window
	windowModuleBoxItem = GetDlgItem(dialog,whereFrameItem);

// sjb 19990318 now get the box item window frame (in global coords!)
	if (GetWindowRect(windowModuleBoxItem,&windowModuleBox) &&
	    GetWindowRect(dialog,&mainWindowBox))
	{
		NMUInt16		portNum = ntohs(inConfig->hostAddr.sin_port);

	// correct for the location of the parent window.
		windowModuleBox.left -= mainWindowBox.left;
		windowModuleBox.top -= mainWindowBox.top;
		windowModuleBox.right -= mainWindowBox.left;
		windowModuleBox.bottom -= mainWindowBox.top;

		CreateModuleControls(dialog,&gDialogTextInfo,&windowModuleBox,inBaseItem);

	// Now set default item texts
		SetDlgItemText(dialog,inBaseItem+kHostText,inConfig->host_name);
		SetDlgItemInt(dialog,inBaseItem+kPortText,portNum,0);
	}

	return err;
}

// 19990318 sjb this doesn't handle INITDIALOG messages in general since
// those are handled above in NMSetupDialog

//----------------------------------------------------------------------------------------
// NMHandleEvent
//----------------------------------------------------------------------------------------

NMBoolean
NMHandleEvent(
	NMDialogPtr	dialog, 
	NMEvent		*event, 
	NMConfigRef	inConfig)
{
NMBoolean	handled = false;

	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(event)
	UNUSED_PARAMETER(inConfig)

	return handled;
}

//----------------------------------------------------------------------------------------
// CreateModuleControls
//----------------------------------------------------------------------------------------

void 
CreateModuleControls(HWND dialog,
                const TEXTMETRIC *winMetrics,
                const NMRect *relativeRect,
                const int commandNumberBase)
{
// make test control
	LONG		xChar = winMetrics->tmAveCharWidth;
	LONG		yChar = winMetrics->tmHeight + winMetrics->tmExternalLeading;

	NMSInt32	iterItems;

	for (iterItems = 0; iterItems < kNumControls; iterItems++)
	{
		int itemLeft, itemTop;
		int itemWidth, itemHeight;

		itemLeft = (gItemsInfo[iterItems].left * xChar / 4) + relativeRect->left;
		itemTop = (gItemsInfo[iterItems].top * yChar / 8) + relativeRect->top;

		itemWidth = (gItemsInfo[iterItems].right - gItemsInfo[iterItems].left) * xChar / 4;
		itemHeight = (gItemsInfo[iterItems].bottom - gItemsInfo[iterItems].top) * yChar / 8;

		gStoreControls[iterItems] =
			CreateWindow(gItemsInfo[iterItems].className,
							 gItemsInfo[iterItems].controlText,
							 gItemsInfo[iterItems].winStyle,
							 itemLeft,
							 itemTop,
							 itemWidth,
							 itemHeight,
							 dialog,(HMENU) (iterItems+commandNumberBase),
							 application_instance,0);
	}
}

//----------------------------------------------------------------------------------------
// NMHandleItemHit
//----------------------------------------------------------------------------------------

NMErr
NMHandleItemHit(
	NMDialogPtr 	dialog, 
	NMSInt16		inItemHit, 
	NMConfigRef		inConfig)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(inItemHit)
	UNUSED_PARAMETER(inConfig)

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMTeardownDialog
//----------------------------------------------------------------------------------------

NMBoolean
NMTeardownDialog(
	NMDialogPtr dialog,
	NMBoolean  inUpdateConfig, 
	NMConfigRef inConfig)
{
	op_vassert_return((inConfig != NULL),"Config ref is NULL!",true);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",true);
	op_vassert_return((inConfig->cookie==config_cookie),"inConfig->cookie==config_cookie",true);

	if (inUpdateConfig)
	{
	// sjb 19990330 don't resolve the name here
	//              this is probably a bad idea though
	//              since the user gets no feedback that the name is bad.
	//              we don't have a way to display any lookup errors anyway.

	//	Get the host text (is this okay with unicode?)
		GetDlgItemText(dialog,gCommandNumberBase+kHostText,inConfig->host_name,255);

	//	Get the port number as unsigned in network byte order
		inConfig->hostAddr.sin_port = htons((u_short)GetDlgItemInt(dialog,gCommandNumberBase+(NMSInt32)kPortText,NULL,false));
	}

	return true;
}
