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

#include "portable_files.h"

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
#include "OPUtils.h"
#include "op_definitions.h"

/* ------------ dialog functions */

 //doxygen instruction:
/** @addtogroup HumanInterface
 * @{
 */

//----------------------------------------------------------------------------------------
// ProtocolSetupDialog
//----------------------------------------------------------------------------------------
/**
	Set up a dialog.
	@brief Set up a dialog.
	@param dialog The dialog object to set up in.
	@param frame the frame..?
	@param base the base..?
	@param config The config to determine base values from.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

/* HWND under windows, DialogPtr on Macs
 * id of ditl item for frame.  Should this be a rect instead?
 * This is NOT a ProtocolConfig-> it's the data hung off the data element
 */
NMErr ProtocolSetupDialog(
	NMDialogPtr dialog, 
	short frame, 
	short base,
	PConfigRef config)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMSetupDialog)
		{
			err= config->NMSetupDialog(dialog, frame, base, (NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolHandleEvent
//----------------------------------------------------------------------------------------
/**
	Tell the config dialog to handle one event.
	@brief Tell the config dialog to handle one event.
	@param dialog The dialog object.
	@param event The event object.
	@param config The configuration the dialog belongs to.
	@return true if the event was handled.\n
	false otherwise.
	\n\n\n\n
 */

/* item_hit is in the DITL's reference space. */
NMBoolean ProtocolHandleEvent(
	NMDialogPtr dialog, 
	NMEvent *event, 
	PConfigRef config)
{
/* sjb 19990317 remove all vestige of returning errors from this function. */
/* This code doesn't return an error, it returns a boolean */
	NMBoolean handled= false;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config && config->NMHandleEvent)
		handled= config->NMHandleEvent(dialog, event, (NMProtocolConfigPriv*)config->configuration_data);

	return handled;
}

//----------------------------------------------------------------------------------------
// ProtocolHandleItemHit
//----------------------------------------------------------------------------------------
/**
	Handle an item hit.
	@brief Handle an item hit.
	@param dialog The dialog object.
	@param inItemHit The item which was hit.
	@param config The configuration the dialog belongs to.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

NMErr ProtocolHandleItemHit(
	NMDialogPtr dialog, 
	short inItemHit,
	PConfigRef config)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMHandleItemHit)
		{
			err = config->NMHandleItemHit(dialog, inItemHit, (NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolDialogTeardown
//----------------------------------------------------------------------------------------
/**
	Dispose of a dialog setup.
	@brief Dispose of a dialog setup.
	@param dialog The dialog object.
	@param update_config Whether or not to update the config with the dialog's values (was "ok" hit?).
	@param config The configuration the dialog belongs to.
	@return false if teardown is not possible (ie invalid parameters).\n
	true otherwise.
	\n\n\n\n
 */

/* returns false if teardown is not possible (ie invalid parameters) */
NMBoolean ProtocolDialogTeardown(
	NMDialogPtr	dialog, 
	NMBoolean 	update_config,
	PConfigRef 	config)
{
	NMBoolean can_teardown= true;
	NMErr	err;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMTeardownDialog)
		{
			can_teardown= config->NMTeardownDialog(dialog, update_config, (NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return can_teardown;
}

//----------------------------------------------------------------------------------------
// ProtocolGetRequiredDialogFrame
//----------------------------------------------------------------------------------------
/**
	Returns the frame size a dialog setup requires.
	@brief Returns the frame size a dialog setup requires.
	@param r Pointer to a NMRect containing the size.
	@param config The configuration the dialog belongs to.
	@return No return value.
	\n\n\n\n
 */

void ProtocolGetRequiredDialogFrame(
	NMRect *r,
	PConfigRef config)
{
	NMErr err= kNMNoError;
	
	if(config)
	{
		if(config->NMGetRequiredDialogFrame)
		{
			config->NMGetRequiredDialogFrame(r, (NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}
}
/** @}*/

 //doxygen instruction:
/** @addtogroup Enumeration
 * @{
 */

//----------------------------------------------------------------------------------------
// ProtocolStartEnumeration
//----------------------------------------------------------------------------------------
/**
	Begin searching for hosts on the network.
	@brief Begin searching for hosts on the network.
	@param config The configuration to use when searching.
	@param inCallback The callback for the enumeration to use.
	@param inContext Custom context to be passed to the enumeration callback.
	@param inActive Active enumeration searches the network for games.  Passive only uses stored addresses, etc.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

NMErr ProtocolStartEnumeration(
	PConfigRef config, 
	NMEnumerationCallbackPtr inCallback, 
	void *inContext, 
	NMBoolean inActive)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMStartEnumeration)
		{
/* 19990124 sjb propagate err from enumeration */
			err = config->NMStartEnumeration((NMProtocolConfigPriv*)config->configuration_data, inCallback, inContext, inActive);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolEndEnumeration
//----------------------------------------------------------------------------------------
/**
	Stop searching for hosts on the network.
	@brief Stop searching for hosts on the network.
	@param config The configuration to end enumeration for.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

NMErr ProtocolEndEnumeration(
	PConfigRef config)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMEndEnumeration)
		{
/* 19990124 sjb propagate err from end enum */
			err = config->NMEndEnumeration((NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolIdleEnumeration
//----------------------------------------------------------------------------------------
/**
	Provide the enumeration process with processing time.  Call this repeatedly while enumerating.
	@brief Provide the enumeration process with processing time.
	@param config The config which is in the process of enumeration.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

NMErr ProtocolIdleEnumeration(
	PConfigRef config)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMIdleEnumeration)
		{
/* 19990124 sjb propagate err from idle */
			err = config->NMIdleEnumeration((NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolBindEnumerationToConfig
//----------------------------------------------------------------------------------------
/**
	Binds a configuration to a host found on the network through enumeration.
	@brief Binds a configuration to a host found on the network.
	@param config The config which is in the process of enumeration.
	@param inID The id of the host to bind to.  The \e id member of an \ref NMEnumItemStruct obtained through enumeration is used.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

NMErr ProtocolBindEnumerationToConfig(
	PConfigRef config, 
	NMHostID inID)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMBindEnumerationItemToConfig)
		{
/* 19990124 sjb propagate err from bind */
			err = config->NMBindEnumerationItemToConfig((NMProtocolConfigPriv*)config->configuration_data, inID);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}


/** @}*/
