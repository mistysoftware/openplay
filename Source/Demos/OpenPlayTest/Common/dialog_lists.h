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

#ifndef __DIALOG_LISTS__
#define __DIALOG_LISTS__

enum {
	_list_is_alphabetical= 0x01
};

NMBoolean new_list(NMDialogPtr dialog, short item_number, NMUInt16 flags);
void free_list(NMDialogPtr dialog, short item);

void select_dialog_list_or_edittext(NMDialogPtr dialog, short item);

short get_listbox_value(NMDialogPtr dialog, short item);
void set_listbox_doubleclick_itemhit(NMDialogPtr dialog, short item, short double_click_item);
short add_text_to_list(NMDialogPtr dialog, short item, char *text);
void delete_from_list(NMDialogPtr dialog, short item, short list_index);
void empty_list(NMDialogPtr dialog, short item);
void select_list_item(NMDialogPtr dialog, short item,	short row);
short find_text_in_list(NMDialogPtr dialog, short item, char *text);

#ifdef OP_PLATFORM_MAC_CFM
// --- these are only used by the general_filter_proc.
void activate_dialog_lists(DialogPtr dialog, NMBoolean becoming_active);
NMBoolean list_handled_key(DialogPtr dialog, short *item_hit, short modifiers, short key);
// where should be in local coords
NMBoolean list_handled_click(DialogPtr dialog, Point where, short *item_hit, short modifiers);
#endif

#endif
