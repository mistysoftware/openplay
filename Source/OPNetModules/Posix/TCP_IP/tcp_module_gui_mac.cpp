/* 
 *-------------------------------------------------------------
 * Description:
 *   Functions which handle user interface interaction
 *
 *------------------------------------------------------------- 
 *   Author: Kevin Holbrook
 *  Created: June 23, 1999
 *
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

// so we dont get the standard posix dialog types
#define OP_POSIX_USE_CARBON_TYPES 1 

#include <Carbon/Carbon.h>
#include "OpenPlay.h"
#include "OPUtils.h"
#include "DebugPrint.h"
#include "NetModule.h"
#include "tcp_module.h"
#include "Exceptions.h"


static NMSInt16			gBaseItem;

enum
{
	kInfoStrings		= 2000,
	kDITLID				= 2000
};

enum
{
	kJoinTitle			= 1,
	kHostLabel,
	kHostText,
	kPortLabel,
	kPortText
};

static const Rect	kFrameSize = {0, 0, 100, 430};


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

/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMSetupDialog(	NMDialogPtr 		dialog, 
						NMSInt16 			frame, 
						NMSInt16			inBaseItem, 
						NMConfigRef			config)
{
	DEBUG_ENTRY_EXIT("NMSetupDialog");

	NMErr				status = kNMNoError;
	Handle				ourDITL;
	NMProtocolConfigPriv		*theConfig = (NMProtocolConfigPriv *) config;
	//SetupLibraryState	duh;	// Make sure we're accessing the right resource fork
	Str255				hostName;
	Str255				portText;

	NMSInt16			kind;
	Handle				h;
	Rect				r; 
	
	//FIXME
	//SetTempPort			port(dialog);
	
	op_vassert_return((theConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",kNMParameterErr);
		
	gBaseItem = inBaseItem;

	//	Try to load in our DITL.  If we fail, we should bail
	ourDITL = Get1Resource('DITL', kDITLID);
	//ThrowIfNil_(ourDITL);
	if (ourDITL == NULL){
		DEBUG_PRINT("couldnt get our dialog. argh");
		return kNMResourceErr;
	}
	else
		DEBUG_PRINT("got our dialog");

	//	Append our DITL relative to the frame by passing the negative of the frame's id
	AppendDITL(dialog, ourDITL, -frame);
	ReleaseResource(ourDITL);

	//FIXME
	//	Setup our dialog info.
	/*if (theConfig->address.fHost != 0)
	{
		//	Try to get the canonical name
		status = OTUtils::MakeInetNameFromAddress(theConfig->address.fHost, (char *) hostName);
		
		//	if that fails, just use the string version of the dotted quad
		if (status != kNMNoError)
			OTInetHostToString(theConfig->address.fHost, (char *) hostName);

		c2pstr((char *) hostName);				
	}
	else*/
	{
		unsigned char defaultString[] = "\p0.0.0.0";
		memcpy(hostName,defaultString,defaultString[0] + 1);
	}

	//FIXME
	//	get the port
	//NumToString(theConfig->address.fPort, portText);
	{
		unsigned char defaultString[] = "\p1234";
		memcpy(portText,defaultString,defaultString[0] + 1);
	}
	
	
	GetDialogItem(dialog, gBaseItem + kHostText, &kind, &h, &r);
	SetDialogItemText(h, hostName);

	GetDialogItem(dialog, gBaseItem + kPortText, &kind, &h, &r);
	SetDialogItemText(h, portText);

	return kNMNoError;
} /* NMSetupDialog */



/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMBoolean NMHandleEvent(	NMDialogPtr			dialog, 
							NMEvent *			event, 
							NMConfigRef 		inConfig)
{
	return false;
} /* NMHandleEvent */



/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMHandleItemHit(	NMDialogPtr			dialog, 
						NMSInt16			inItemHit, 
						NMConfigRef 		inConfig)
{
	DEBUG_ENTRY_EXIT("NMHandleItemHit");

NMProtocolConfigPriv	*theConfig = (NMProtocolConfigPriv *) inConfig;
NMSInt16		theRealItem = inItemHit - gBaseItem;

//FIXME
//SetTempPort		port(dialog);

	op_vassert_return((theConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",kNMParameterErr);
	
	switch (theRealItem)
	{
		case kPortText:
		case kHostText:
		default:
			break;
	}

	return kNMNoError;
} /* NMHandleItemHit */


/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */


NMBoolean NMTeardownDialog(	NMDialogPtr 		dialog, 
							NMBoolean			inUpdateConfig, 
							NMConfigRef 		ioConfig)
{
	DEBUG_ENTRY_EXIT("NMTeardownDialog");

	NMProtocolConfigPriv	*theConfig = (NMProtocolConfigPriv *) ioConfig;
	NMErr		status;
	//InetAddress		addr;
	Str255			hostText;
	Str255			portText;
	
	NMSInt16		kind;
	Rect 			r;
	Handle 			h;
	NMSInt32		theVal;
	
	//FIXME
	//SetTempPort		port(dialog);

	op_vassert_return((theConfig != NULL),"Config ref is NULL!",true);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",true);

	if (inUpdateConfig)
	{
		// 	Get the host text	
		GetDialogItem(dialog, gBaseItem + kHostText, &kind, &h, &r);
		GetDialogItemText(h, hostText);

		doP2CStr((char*)hostText);

		//	resolve it into a host addr
		
		//FIXME
		status = 1;
		//status = OTUtils::MakeInetAddressFromString((const char *)hostText, &addr);
		//op_warn(status == kNMNoError);
		
		if (status != kNMNoError)
			return false;
		
		//FIXME
		//theConfig->address = addr;
		
		//	Get the port text
		GetDialogItem(dialog, gBaseItem + kPortText, &kind, &h, &r);
		GetDialogItemText(h, portText);
		StringToNum(portText, &theVal);

		//FIXME
		//if (theVal != 0)
		//	theConfig->address.fPort = theVal;
	}
	
	return true;
} /* NMTeardownDialog */



/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

void NMGetRequiredDialogFrame(	NMRect *		r, 
								NMConfigRef 	inConfig)
{
	DEBUG_ENTRY_EXIT("NMGetRequiredDialogFrame");

	//	I don't know of any reason why the config is going to affect the size of the frame
	*r = kFrameSize;
} /* NMGetRequiredDialogFrame */


