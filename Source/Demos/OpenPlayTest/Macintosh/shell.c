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
#include "log.h"
#include "OpenPlayTest_resources.h"
#include "dialog_lists.h"
#include "dialog_utils.h"
#include "testbed.h"
#include "String_Utils.h"

#if (!OP_PLATFORM_MAC_MACHO)
	#include <TextUtils.h>
#endif

/* ----- structures */
struct new_endpoint_buffer {
	struct endpoint_data *endpoint;
	char title[128];
};

#define MAXIMUM_ENDPOINTS_IN_CACHE (10)

/* -------- globals */
NMBoolean app_done = false;
static NMBoolean quitting= false;
NMBoolean active_enumeration= true;

struct {
	short count;
	struct new_endpoint_buffer endpoints[MAXIMUM_ENDPOINTS_IN_CACHE];
} new_endpoint_cache= { 0 };

extern char gNameString[256];

/* ----------- local prototypes */
static void initialize_application(void);
static void process_event(EventRecord *event);
static NMBoolean process_key(EventRecord *event, short key);
//void do_menu_command(long menuResult) ;
static NMBoolean handle_close(WindowPtr wp);
static void update_window(WindowPtr wp);
static void draw_window_contents(WindowPtr wp);

short get_dialog_item_value(DialogPtr dialog, short item_number);

void append_item_to_popup(DialogPtr dialog, short item, char *cstring);

static pascal Boolean configuration_filter_proc(DialogPtr dialog, EventRecord *event, short *item_hit);
static pascal OSErr  do_quit_apple_event(const AppleEvent *appEvent,AppleEvent *reply,long handlerRefcon);

/* ----------- entry point */
int main(
	void) 
{
	EventRecord event;

	initialize_application();

	while (!quitting) 
	{
		if(WaitNextEvent(everyEvent, &event, 1, NULL))
		{
			/* Process the event.. */
			process_event(&event);
		} else {
			short index;
		
			/* Idling code.. */
			idle();

			/* Do things that can only be done at system time */
			for(index= 0; index<new_endpoint_cache.count; ++index)
			{
				attach_new_endpoint_to_application(new_endpoint_cache.endpoints[index].endpoint,
					new_endpoint_cache.endpoints[index].title, true);
			}
			new_endpoint_cache.count= 0;
		}
	}

	app_done = true;
	return 0;
}

/* ----------- private code */
static void initialize_application(
	void)
{
	Handle menubar;
	StringHandle		userName;

#ifndef OP_PLATFORM_MAC_CARBON_FLAG
	MaxApplZone();
	MoreMasters();
	MoreMasters();
	MoreMasters();
	
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(NULL);
#else
	MoreMasterPointers(192);
#endif // ! OP_PLATFORM_MAC_CARBON_FLAG

	InitCursor();

	FlushEvents(everyEvent, 0);

	menubar= GetNewMBar(rMENU_BAR_ID);
	op_assert(menubar);

	SetMenuBar(menubar);
	DisposeHandle(menubar);
	AppendResMenu(GetMenuHandle(mApple), 'DRVR');
	DrawMenuBar();

	//	Get the user name from the systemÉ
	
	userName = GetString (-16096);
	if (userName == NULL)
		strcpy(gNameString, "OpenPlay Test");
	else
	{
		doCopyP2CStr(*userName,gNameString);
		ReleaseResource ((Handle) userName);
	}

	// ecf - we wanna enable enumeration by default...
	check_menu_item(mSpecial, iActiveEnumeration, active_enumeration);
	
	//install apple event handler for quitting
	AEInstallEventHandler(kCoreEventClass,kAEQuitApplication,NewAEEventHandlerUPP((AEEventHandlerProcPtr)do_quit_apple_event),0L,false);
	
	return;
}

static void process_event(
	EventRecord *event)
{
	WindowPtr window;
	short part_code;
	Rect size_rect;
	long new_size;
	GrafPtr old_port;

	switch(event->what)
	{
		case kHighLevelEvent:
				AEProcessAppleEvent(event);
			break;
	
		case mouseDown:
			/* Did we hit one of the floaters? */
			part_code= FindWindow(event->where, &window);
			
			switch (part_code)
			{
				case inSysWindow:
					break;
					
				case inMenuBar:
					do_menu_command(MenuSelect(event->where));
					break;
					
				case inContent:
					if(window != FrontWindow())
						SelectWindow(window);
					click_in_log_window(window,event->where,(event->modifiers & shiftKey) != 0);
					break;

				case inDrag:
			#ifdef OP_PLATFORM_MAC_CARBON_FLAG
					{
						Rect	tempRect;
						GetRegionBounds(GetGrayRgn(), &tempRect);
						DragWindow(window, event->where, &tempRect);
					}
					#else
						DragWindow(window, event->where, &qd.screenBits.bounds);
					#endif
					break;

				case inGrow:
			#ifdef OP_PLATFORM_MAC_CARBON_FLAG
						GetRegionBounds(GetGrayRgn(), &size_rect);
					#else
						size_rect = qd.screenBits.bounds;
					#endif
					InsetRect(&size_rect, 4, 4);
					new_size= GrowWindow(window, event->where, &size_rect);
					if(new_size) 
					{
						/* Resize the window */
						SizeWindow(window, LoWord(new_size), HiWord(new_size), true);
					}
					break;

				case inGoAway:
					if(TrackGoAway(window, event->where)) 
					{
						/* Close the window... */
						handle_close(window);
					}
					break;

				case inZoomIn:
				case inZoomOut:
					if(TrackBox(window, event->where, part_code)) 
					{
						GetPort(&old_port);
						SetPortWindowPort(window);
					#ifdef OP_PLATFORM_MAC_CARBON_FLAG
						{
							Rect windowBounds;
                                                
							GetWindowPortBounds(window, &windowBounds);
							EraseRect(&windowBounds);
						}
						#else
							EraseRect(&window->portRect);
						#endif
						
						ZoomWindow(window, part_code, true);
						SetPort(old_port);
					}
					break;
			}
			break;
		
		case keyDown:
		case autoKey:
			if(!process_key(event, event->message&charCodeMask))
			{
				/* Not handled by anyone.. */
				SysBeep(-1);
			}
			break;
			
		case updateEvt:
			/* Update the window.. */
			update_window((WindowPtr)event->message);
			break;
			
		case activateEvt:
			/* Activate event->message, event->modifiers&activeFlag.. */
			break;

		case osEvt:
			switch ((event->message>>24) & 0xff)
			{
				case suspendResumeMessage:
					if (event->message&resumeFlag)
					{
						/* resume */
					#ifdef OP_PLATFORM_MAC_CARBON_FLAG
						{
							Cursor		arrowCursor;
							SetCursor(GetQDGlobalsArrow(&arrowCursor));
						}
						#else
							SetCursor(&qd.arrow);
						#endif
					}
					else
					{
						/* suspend */
					}
					break;

				case mouseMovedMessage:
					break;
 			}
			break;
	}

	return;
}

static NMBoolean process_key(
	EventRecord *event,
	short key)
{
	NMBoolean handled= false;

	if(event->modifiers&cmdKey && event->what==keyDown)
	{
		long menu_key= MenuKey(key);
		
		/* If it was a menu key. */
		if(menu_key)
		{
			do_menu_command(menu_key);
			handled= true;
		}
	}
	
	if(!handled)
	{
		handled= true;
	}

	return handled;
}

void handle_quit(
	void)
{
	WindowPtr wp;
	NMBoolean cancelled= false;
	
	while((wp= FrontWindow())!= NULL)
	{
		cancelled= handle_close(wp);
		if(cancelled) break;
	}
	
	if(!cancelled) quitting= true;
}

static void update_window(
	WindowPtr wp)
{
	BeginUpdate(wp);
	draw_window_contents(wp);
	EndUpdate(wp);
	
	return;
}

static void draw_window_contents(
	WindowPtr wp)
{
	GrafPtr old_port;

	GetPort(&old_port);
	SetPortWindowPort(wp);

	/* Any specific updating stuff.... */
	
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
	{
		Rect	windowBounds;
		GetWindowPortBounds(wp, &windowBounds);
		EraseRect(&windowBounds);
	}
	#else
		EraseRect(&wp->portRect);
	#endif
	
	update_log(wp);

	SetPort(old_port);

	return;
}

void close_front_window(
	void)
{
	WindowPtr wp;
	
	if((wp= FrontWindow()) != NULL)
	{
		handle_close(wp);
	}
	
	return;
}

/* ------------------ testbed code inserted here */

/* Return true if the user aborted.. */
static NMBoolean handle_close(
	WindowPtr wp)
{
	struct endpoint_data *endpoint_data;
	
	endpoint_data= (struct endpoint_data *) GetWRefCon(wp);
	if(endpoint_data)
	{
		NMErr err;
	
		err= ProtocolCloseEndpoint(endpoint_data->endpoint, true);
		DEBUG_PRINTonERR("Err %d closing endpoint!", err);
		free(endpoint_data);
	}
	/* Check if it is dirty? */
	dispose_log(wp);
	DisposeWindow(wp);
	
	return false; 
}

void attach_new_endpoint_to_application(
	struct endpoint_data *endpoint,
	char *title,
	NMBoolean system_time)
{
	static long startLeft = 100;
	static long startTop = 100;
	
	WindowPtr wp;
	char window_title[128];
	
	if(system_time)
	{
		Rect boundsRect;
		boundsRect.left = startLeft;
		startLeft += 32;
		boundsRect.right = boundsRect.left + 500;
		boundsRect.top = startTop;
		startTop += 32;
		boundsRect.bottom = boundsRect.top + 300;
		wp = NewCWindow(nil,&boundsRect,"\pWindow", true, noGrowDocProc,(WindowPtr)-1,false,0);
		//wp = NewCWind(
		//wp= GetNewWindow(winDOCUMENT, NULL, (WindowPtr) -1l);
		op_assert(wp);
		
		//add a close box
		//FIXME: i forgot how to do this on classic =)
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
			ChangeWindowAttributes(wp,kWindowCloseBoxAttribute,0);
		#endif
		
		if(wp)
		{
			SetWRefCon(wp, (long) endpoint);

			strcpy(window_title, title);
			c2pstr(window_title);
			SetWTitle(wp, (const unsigned char *) window_title);
			ShowWindow(wp);
		
			new_log(wp);
		}
	} else {
		short new_index= new_endpoint_cache.count++;
	
		// copy into a cache for later servicing.
		op_assert(new_endpoint_cache.count<MAXIMUM_ENDPOINTS_IN_CACHE);
		new_endpoint_cache.endpoints[new_index].endpoint= endpoint;
		strcpy(new_endpoint_cache.endpoints[new_index].title, title);
	}

	return;
}

void detach_endpoint_from_application(
	struct endpoint_data *endpoint)
{
	struct endpoint_data *data= NULL;
	WindowPtr wp;
	NMBoolean removed= false;

	wp= FrontWindow();
	while(wp && !removed)
	{
		data= (struct endpoint_data *) GetWRefCon(wp);

		// get rid of it.
		if(data==endpoint)
		{
			free(endpoint);
			dispose_log(wp);
			DisposeWindow(wp);
			removed= true;
		} else {
			wp= GetNextWindow(wp);
		}
	}
	op_assert(removed);

	return;
}

enum {
	iOK= 1,
	iCancel= 2
};

NMBoolean select_protocol(
	NMType *type)	
{
	DialogPtr dialog;
	short item_hit= NONE;
	short count, index;
	NMErr err;
	NMType module_types[64];
	
	dialog= GetNewDialog(dlogSelectProtocol, NULL, (WindowPtr) -1l);
	op_assert(dialog);

	/* Append them. */
	for(index= 0, err= 0, count= 0; !err && count<(sizeof(module_types)/sizeof(module_types[0])); ++index)
	{
		NMProtocol protocol;
		
		protocol.version= CURRENT_OPENPLAY_VERSION;
		err= GetIndexedProtocol(index, &protocol);
		if(!err)
		{
			append_item_to_popup(dialog, iPROTOCOL_LIST, protocol.name);
			module_types[count++]= protocol.type;
		} else {
			if(err != kNMNoMoreNetModulesErr) DEBUG_PRINTonERR("Err %d in GetIndexedProtocol...", err);
		}
	}
	ShowWindow(GetDialogWindow(dialog));
	
	do {
		ModalDialog(NULL, &item_hit);
	} while(item_hit > iCancel);

	if(item_hit==iOK)
	{
		index= get_dialog_item_value(dialog, iPROTOCOL_LIST)-1;
		op_assert(index>=0 && index<count);
		*type= module_types[index];
	}
	
	DisposeDialog(dialog);

	return (item_hit==iOK);
}

short get_dialog_item_value(
	DialogPtr dialog,
	short item_number)
{
	Rect bounds;
	ControlHandle control;
	short item_type;
	
	GetDialogItem(dialog, item_number, &item_type, (Handle *) &control, &bounds);
	op_assert(control);
	
	return GetControlValue(control);
}

struct endpoint_data *get_current_endpoint_data(
	void)
{
	struct endpoint_data *data= NULL;
	WindowPtr wp;
	
	if((wp= FrontWindow()) != NULL)
	{
		data= (struct endpoint_data *) GetWRefCon(wp);
	}
	
	return data;
}

// stupid silly iterators..
WindowPtr last_endpoint_window= NULL;

struct endpoint_data *get_front_endpoint(
	void)
{
	struct endpoint_data *data= NULL;
	
	if((last_endpoint_window= FrontWindow()) != NULL)
	{
		data= (struct endpoint_data *) GetWRefCon(last_endpoint_window);
	}
	
	return data;
}

struct endpoint_data *get_next_endpoint(
	void)
{
	struct endpoint_data *data= NULL;
	
	if((last_endpoint_window= GetNextWindow(last_endpoint_window)) != NULL)
	{
		data= (struct endpoint_data *) GetWRefCon(last_endpoint_window);
	}

	return data;
}

void cascade_or_tile_windows(
	NMBoolean cascade)
{
	if(cascade)
	{
	} else {
	}

	return;
}

void set_log_window_to_endpoints_window(
	PEndpointRef endpoint)
{
	WindowPtr window= FrontWindow();

	while(window)
	{	
		struct endpoint_data *active_endpoint= (struct endpoint_data *) GetWRefCon(window);

		if(active_endpoint && active_endpoint->endpoint==endpoint)
		{
			set_current_log_window(window);
			break;
		}
		
		window= GetNextWindow(window);
	}

	return;
}

struct {
	DialogPtr dialog;
	PConfigRef config;
} configuration_globals;

NMBoolean configure_protocol(
	PConfigRef config)
{
	DialogPtr dialog;
	short item_hit= NONE;
	NMErr err;
	NMRect min_bounds;
	struct list_item_data list_items;
	ModalFilterUPP configure_protocol_upp;
	short DITLCount;
	NMBoolean success= false;

	configure_protocol_upp= NewModalFilterUPP(configuration_filter_proc);
	op_assert(configure_protocol_upp);

	dialog= GetNewDialog(dlogConfigureProtocol, NULL, (WindowPtr) -1l);
	op_assert(dialog);

	DITLCount = CountDITL(dialog);
	
	list_items.dialog= dialog;
	list_items.count= 0;
	new_list(dialog, iGameList, 0);

	configuration_globals.dialog= dialog;
	configuration_globals.config= config;

	ProtocolGetRequiredDialogFrame(&min_bounds, config);
	DEBUG_PRINT("Req'd DialogFrame: (top left width height) %d %d %d %d!",min_bounds.top, min_bounds.left, RECTANGLE_WIDTH(&min_bounds), RECTANGLE_HEIGHT(&min_bounds));

	err= ProtocolSetupDialog(dialog, iProtocolDataFrame, DITLCount, config);
	DEBUG_PRINTonERR("Err %d in ProtocolSetupDialog!", err);

	// start an enumeration
	err= ProtocolStartEnumeration(config, NMEnumerationCallback, &list_items, active_enumeration);
	DEBUG_PRINTonERR("Err %d in ProtocolStartEnumeration!", err);

	ShowWindow(GetDialogWindow(dialog));
	
	do {
		ModalDialog(configure_protocol_upp, &item_hit);
		if (item_hit > DITLCount)
			err = ProtocolHandleItemHit(dialog, item_hit, config);
	} while(item_hit != iCancel && item_hit != iJoinGame && item_hit != iCreateGame);

	switch(item_hit)
	{
		case iCreateGame:
			success= true;
			break;
			
		case iJoinGame:
			{
				short index;
			
				// ÒAnd one to bind themÉÓ
				index= get_listbox_value(dialog, iGameList);
				op_assert(index>=0 && index<list_items.count);

				if (index>=0 && index<list_items.count)
				{
					err= ProtocolBindEnumerationToConfig(config, list_items.host[index]);
					DEBUG_PRINTonERR("ProtocolEndEnumeration returned %d!", err);
				}
			}
			success= true;
			break;
	}
	
//	End the enumeration AFTER we bind it.  Should probably have a ref for these...
	err= ProtocolEndEnumeration(config);
	DEBUG_PRINTonERR("ProtocolEndEnumeration returned %d!", err);

	if(!ProtocolDialogTeardown(dialog, (item_hit == iCreateGame), config))
	{
		DEBUG_PRINT("Oops, ProtocolDialogTeardown returned false!");
	}

	free_list(dialog, iGameList);
	DisposeDialog(dialog);
	DisposeModalFilterUPP(configure_protocol_upp);
	
	return success;
}

static pascal Boolean
configuration_filter_proc(
	DialogPtr dialog,
	EventRecord *event,
	short *item_hit)
{
	Boolean handled= false;
	NMBoolean config_handled;
	NMErr err;
	
	err= ProtocolIdleEnumeration(configuration_globals.config);
	DEBUG_PRINTonERR("Err %d in ProtocolIdleEnumeration", err);
	
	config_handled= ProtocolHandleEvent(dialog, event, configuration_globals.config);
	if(config_handled)
	{
		handled= true;
	} else {
		switch(event->what)
		{
			case keyDown:
			case autoKey:
				handled= list_handled_key(dialog, item_hit, event->modifiers, event->message&charCodeMask);
				break;
				
			case mouseDown:
				{
					Point where;
					NMBoolean handled;
					GrafPtr old_port;
					
					GetPort(&old_port);
				#ifdef OP_PLATFORM_MAC_CARBON_FLAG
						SetPortDialogPort(dialog);
					#else
						SetPort(dialog);
					#endif
					
					where= event->where;
					GlobalToLocal(&where);
					
					handled= list_handled_click(dialog, where, item_hit, event->modifiers);
					
					SetPort(old_port);
					
					if(handled) return true;
				}
				break;
		}
	}
	
	return handled;
}

NMBoolean get_timeout_value(
	long *timeout)
{
	DialogPtr dialog;
	short item_hit= NONE;
	
	dialog= GetNewDialog(dlogSetTimeout, NULL, (WindowPtr) -1l);
	op_assert(dialog);

	ShowWindow(GetDialogWindow(dialog));
	
	do {
		ModalDialog(NULL, &item_hit);
	} while(item_hit > iCancel);

	if(item_hit==iOK)
	{
		Handle item;
		short type;
		Rect bounds;
		char buffer[257];

		GetDialogItem(dialog, iTimeout, &type, &item, &bounds);
		GetDialogItemText(item, (unsigned char *) buffer);
		p2cstr((unsigned char *) buffer);
		*timeout= atoi(buffer);
	}
	
	DisposeDialog(dialog);

	return (item_hit==iOK);
}

static pascal OSErr  do_quit_apple_event(const AppleEvent *appEvent,AppleEvent *reply,long handlerRefcon)
{
	handle_quit();
	return noErr;
}

