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
 
#include "OpenPlay.h"
#include "OPUtils.h"
#include "dialog_utils.h"
#include <string.h>
//#include <assert.h>
#include "String_Utils.h"

#ifdef OP_PLATFORM_WINDOWS
	#include <windowsx.h>
#endif //OP_PLATFORM_WINDOWS

#if defined(OP_PLATFORM_MAC_CFM)

	#include <ControlDefinitions.h>	// For Universal Headers 3.3+ support

#elif defined(OP_PLATFORM_WINDOWS)

	static NMBoolean dialog_item_class_is(HWND target, char *test_class_name);
	static char temporary[256];

#endif


#if defined(OP_PLATFORM_MAC_CFM) || defined(OP_PLATFORM_MAC_MACHO)

static MenuHandle get_popup_menu_handle(DialogPtr dialog, short item);

void
append_item_to_popup(
	DialogPtr	dialog, 
	short		item,
	char		*cstring)
{
MenuHandle	menu;
short		item_type;
Handle		control;
Rect		bounds;
char		temporary_copy[256];

	/* Get the menu */
	menu = get_popup_menu_handle(dialog, item);

	AppendMenu(menu, "\p ");
	strcpy(temporary_copy, cstring);
	c2pstr(temporary_copy);
	SetMenuItemText(menu, CountMenuItems(menu), (const UInt8 *) temporary_copy);
//	SetMenuItemText(menu, CountMItems(menu), (const UInt8 *) temporary_copy);
	
	/* Set the max value */
	GetDialogItem(dialog, item, &item_type, (Handle *) &control, &bounds);
	SetControlMaximum((ControlHandle) control, CountMenuItems(menu));
//	SetControlMaximum((ControlHandle) control, CountMItems(menu));

	/* Make sure the value is okay */
	SetControlMinimum((ControlHandle) control, 1);

	if	(GetControlValue((ControlHandle) control) < 1)
		SetControlValue((ControlHandle) control, 1);
}

static MenuHandle
get_popup_menu_handle(
	DialogPtr	dialog,
	short		item)
{
MenuHandle				menu;
short					item_type;
ControlHandle			control;
Rect					bounds;

	/* Add the maps.. */
	GetDialogItem(dialog, item, &item_type, (Handle *) &control, &bounds);

#ifdef OP_PLATFORM_MAC_CARBON_FLAG
	menu = GetControlPopupMenuHandle(control);
#else
	{
		struct PopupPrivateData	**privateHndl;
	
		// I don't know how to assert that it is a popup control... <sigh>
		privateHndl= (PopupPrivateData **) ((*control)->contrlData);
		op_assert(privateHndl);

		menu= (*privateHndl)->mHandle;
	}
#endif

	op_assert(menu);

	return menu;
}

#elif defined(OP_PLATFORM_WINDOWS)


long
extract_number_from_text_item(
	HWND	dialog, 
	short	item_number)
{
	op_assert(dialog);
	SendDlgItemMessage(dialog, item_number, WM_GETTEXT, 256, (LPARAM) temporary);
	
	return atoi(temporary);
}



void
append_item_to_popup(
	HWND	dialog,
	short	item,
	char	*cstring)
{
#define TEMPORARY_COPY_LENGTH (80)
HWND	combobox;
LONG	window_style;
	
	combobox= GetDlgItem(dialog, item);
	op_assert(combobox);
	op_assert(dialog_item_class_is(combobox, "ComboBox"));

	/* Don't add the preceding E if it isn't an ownerdraw window. */
	window_style = GetWindowLong(combobox, GWL_STYLE);

	if (window_style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))
	{
	char	temporary_copy[TEMPORARY_COPY_LENGTH];

		op_assert(strlen(cstring)<TEMPORARY_COPY_LENGTH);
		temporary_copy[0]= 'E'; // initially enabled.
		strcpy(&temporary_copy[1], cstring);
	
		ComboBox_AddString(combobox, temporary_copy);
	}
	else
	{
		ComboBox_AddString(combobox, cstring);

		if (ComboBox_GetCurSel(combobox) == -1)
		{
			ComboBox_SetCurSel(combobox, 0);
		}
	}
}

#define CLASS_NAME_LENGTH (32)



short
get_dialog_item_value(
	HWND	dialog, 
	short	control)
{
short	selected_index;
HWND	window = GetDlgItem(dialog, control);
char	class_name[CLASS_NAME_LENGTH];
	
	op_vassert(window, csprintf(temporary, "Dialog doesn't have control: %d", control));
	GetClassName(window, class_name, CLASS_NAME_LENGTH);

	if (strcmp(class_name, "ComboBox") == 0)
	{
		selected_index= ComboBox_GetCurSel(window)+1; //+1 to make it match the mac.
		op_assert(selected_index != NONE);
	} 
	else if (strcmp(class_name, "Button") == 0)
	{
		selected_index = Button_GetCheck(window);
	}
	else
	{
		op_vhalt(csprintf(temporary, "What is class %s (%d)?", class_name, control));
	}
	
	return selected_index;
}



static NMBoolean
dialog_item_class_is(
	HWND	target,
	char	*test_class_name)
{
char		class_name[CLASS_NAME_LENGTH];
NMBoolean	classes_match = false;

	op_assert(target);
	GetClassName(target, class_name, CLASS_NAME_LENGTH);
	
	if (strcmp(class_name, test_class_name) == 0)
		classes_match = true;
	
	return classes_match;
}



#elif defined(OP_PLATFORM_UNIX)

 /* FIXME - add code here*/

#else
  #error "Porting error - Need to write dialog utility functions."
#endif
