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
#include "dialog_lists.h"

#define SCROLL_BAR_WIDTH (15)
#define MAX_TYPEAHEAD (32)

/* ---------- listbox functions */
#define MAXIMUM_LIST_HANDLES (16) // if a list item value is >= this, this will fail.

/* ------------- local prototypes */
/* ------------- code */
NMBoolean new_list(
	NMDialogPtr dialog, 
	short item_number,
	NMUInt16 flags)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(item_number)
	UNUSED_PARAMETER(flags)
	
	return FALSE;
}

void free_list(
	NMDialogPtr dialog,
	short item)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(item)
	
	return;		
}

/* swiped from tag_interface.c */
short get_listbox_value(
	NMDialogPtr dialog,
	short item)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(item)

	return NONE;
}

void set_listbox_doubleclick_itemhit(
	NMDialogPtr dialog,
	short item,
	short double_click_item)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(item)
	UNUSED_PARAMETER(double_click_item)

	return;
}

void select_list_item(
	NMDialogPtr dialog,
	short item,
	short row)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(item)
	UNUSED_PARAMETER(row)
	return;
}

short add_text_to_list(
	NMDialogPtr dialog, 
	short item, 
	char *text)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(item)
	UNUSED_PARAMETER(text)
	return NONE;
}

short find_text_in_list(
	NMDialogPtr dialog, 
	short item, 
	char *text)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(item)
	UNUSED_PARAMETER(text)
	return NONE;
}

void delete_from_list(
	NMDialogPtr dialog, 
	short item, 
	short list_index)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(item)
	UNUSED_PARAMETER(list_index)
	return;
}

void empty_list(
	NMDialogPtr dialog,
	short item)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(item)
	return;
}

/* LR -- no prototype
void activate_dialog_lists(
	NMDialogPtr dialog,
	NMBoolean becoming_active)
{
	UNUSED_PARAMETER(dialog)
	UNUSED_PARAMETER(becoming_active)
	return;
}
*/
