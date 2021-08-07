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

#include "OPUtils.h"
#include "NetModule.h"


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
						NMConfigRef			inConfig)
{
	UNUSED_PARAMETER(dialog);
	UNUSED_PARAMETER(frame);
	UNUSED_PARAMETER(inBaseItem);
	UNUSED_PARAMETER(inConfig);

	return kNMInternalErr;
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
	UNUSED_PARAMETER(dialog);
	UNUSED_PARAMETER(event);
	UNUSED_PARAMETER(inConfig);

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
	UNUSED_PARAMETER(dialog);
	UNUSED_PARAMETER(inItemHit);
	UNUSED_PARAMETER(inConfig);

	return kNMInternalErr;
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
	UNUSED_PARAMETER(dialog);
	UNUSED_PARAMETER(inUpdateConfig);
	UNUSED_PARAMETER(ioConfig);

	return false;
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
	r->left = 0;
	r->right = 0;
	r->top = 0;
	r->bottom = 0;
	UNUSED_PARAMETER(inConfig);

} /* NMGetRequiredDialogFrame */

