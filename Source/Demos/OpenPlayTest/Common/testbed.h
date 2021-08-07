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

struct endpoint_data { // stored off the windows RefCon
	PEndpointRef endpoint;
	NMBoolean requires_idle_time;
	NMBoolean closed;
};

struct list_item_data {
	NMDialogPtr dialog;
	short count;
	NMHostID host[200];
};

extern NMBoolean active_enumeration;

/* -------- from shell.c/main.c */
struct endpoint_data *get_current_endpoint_data(void);
void close_front_window(void);
void handle_quit(void);

NMBoolean get_timeout_value(long *timeout);
NMBoolean configure_protocol(PConfigRef config);
NMBoolean select_protocol(NMType *type);

void attach_new_endpoint_to_application(struct endpoint_data *endpoint, char *title, NMBoolean system_time);
void detach_endpoint_from_application(struct endpoint_data *endpoint);

void cascade_or_tile_windows(NMBoolean cascade);

void set_log_window_to_endpoints_window(PEndpointRef endpoint);

struct endpoint_data *get_front_endpoint(void);
struct endpoint_data *get_next_endpoint(void);

/* -------- from testbed.c */
void NMEnumerationCallback(void *inContext, NMEnumerationCommand inCommand, 
	NMEnumerationItem *item);

void idle(void);

void do_menu_command(long menuResult);
void adjust_menus(void);

/* -------- should go into the library */

#if defined(OP_PLATFORM_MAC_CFM) || defined(OP_PLATFORM_MAC_MACHO)
	#define MENU_UPSHIFT (16)
#elif defined(OP_PLATFORM_WINDOWS)
	#define MENU_UPSHIFT (8)
#endif
#define GET_MENU_ID(x) ((x)>>MENU_UPSHIFT)
#define GET_MENU_ITEM(x) ((x)&((1<<MENU_UPSHIFT)-1))
#define MAKE_MENU_COMMAND(menu, item) (((menu)<<MENU_UPSHIFT)|(item))

void set_menu_enabled(short menu_id, short item, NMBoolean enabled);
void check_menu_item(short menu, short item, NMBoolean checked);
