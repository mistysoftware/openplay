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
#include "log.h"
#include "openplaytest_resources.h"
#include "dialog_lists.h"
#include "testbed.h"

#define CACHE_ENDPOINT_COUNT (10)

struct cache_data {
	short count;
	struct endpoint_data *endpoint_cache[CACHE_ENDPOINT_COUNT];
};

extern NMBoolean app_done;
NMBoolean accepting_connections= false;

struct cache_data endpoint_cache= { 0 };
const char *gEnumData = "Mary had a little lamb";
NMUInt32 gEnumDataLen = 24;
char gNameString[256];

/* ------ local code */
static void dump_config_string(PConfigRef config);
static void send_packet(void);
static void send_stream(void);
static void is_alive(void);
static void set_timeout(void);
static void report_info(void);
// 19990123 sjb removed tristate_to_name, NMTriState is unused now
//static char *tristate_to_name(NMTriState state);
static char *boolean_to_name(NMBoolean state);
static struct endpoint_data *create_endpoint_from_config(PConfigRef config, NMBoolean active);
static void callback_function(PEndpointRef inEndpoint, void* inContext,
	NMCallbackCode inCode, NMErr inError, void* inCookie);
static void handle_open(NMBoolean active, NMBoolean use_ui);

/* ----- code */
void idle(
	void)
{
	struct endpoint_data *endpoint_data;

	idle_log();

	// this resets the count..	
	endpoint_data= get_front_endpoint();
	while(endpoint_data)
	{
		struct endpoint_data *next_endpoint= get_next_endpoint();
	
		// maybe should only call if it requires idle time?
		if (endpoint_data->closed)
		{
			// the endpoint is closed, now remove from local lists, etc.
			detach_endpoint_from_application(endpoint_data);
		} 
		else if (endpoint_data->requires_idle_time)
		{
			NMErr err;
		
			err= ProtocolIdle(endpoint_data->endpoint);
			if (err) log_text("ProtocolIdle Error: %d", err);
		}
		
		endpoint_data= next_endpoint;
	}

	// refill the endpoint cache.
	while(endpoint_cache.count<CACHE_ENDPOINT_COUNT)
	{
		endpoint_cache.endpoint_cache[endpoint_cache.count]= (struct endpoint_data*)malloc(sizeof(struct endpoint_data));
		endpoint_cache.count+= 1;
	}

	return;
}

void
NMEnumerationCallback(
	void *inContext,
	NMEnumerationCommand inCommand, 
	NMEnumerationItem *item)
{
	struct list_item_data *list_data= (struct list_item_data *) inContext;
	short index;

	switch(inCommand)
	{
		case kNMEnumAdd:
			for(index= 0; index<list_data->count; ++index)
			{
				if (list_data->host[index]==item->id) DEBUG_PRINT("Got multiple adds for id %d", item->id);
			}
			
			if (index==list_data->count)
			{
				list_data->host[list_data->count++]= item->id;
				index= add_text_to_list(list_data->dialog, iGameList, item->name);
				op_assert(index==list_data->count-1);
			}
			break;
			
		case kNMEnumDelete:
		{
			NMBoolean found = false;
			for(index= 0; index<list_data->count; ++index)
			{
				if (list_data->host[index]==item->id) 
				{
					found = true;
					// remove from list.
					delete_from_list(list_data->dialog, iGameList, index);
					
					// from cache
					memmove(&list_data->host[index], &list_data->host[index+1], 
						list_data->count-index-1);
					list_data->count--;
					break;
				}
			}
			
			if (!found)
			{
				DEBUG_PRINT("Got a remove for something I don't have in my list?? (%d %s)",item->id, item->name);
			}
			break;
		}
		case kNMEnumClear:
			empty_list(list_data->dialog, iGameList);
			list_data->count= 0;
			break;
			
		default:
			DEBUG_PRINT("Command %d unrecognized in NMEnumerationCallback", inCommand);
			break;

	}
	
	return;
}

void adjust_menus(
	void)
{
	struct endpoint_data *endpoint_data= get_current_endpoint_data();

	set_menu_enabled(mFile, iClose, endpoint_data ? true : false);
	set_menu_enabled(mSpecial, iSendPacket, endpoint_data ? true : false);
	set_menu_enabled(mSpecial, iSendStream, endpoint_data ? true : false);
	set_menu_enabled(mSpecial, iProtocolAlive, endpoint_data ? true : false);
	set_menu_enabled(mSpecial, iSetTimeout, endpoint_data ? true : false);
	set_menu_enabled(mSpecial, iGetInfo, endpoint_data ? true : false);
	set_menu_enabled(mSpecial, iGetConfigString, false);
	
	return;
}

void do_menu_command(
	long menuResult) 
{
	short menuID;
	short menuItem;

	menuID= GET_MENU_ID(menuResult);
	menuItem= GET_MENU_ITEM(menuResult);
	
	switch (menuID) 
	{
		case mApple:
			switch (menuItem) 
			{
				case iAbout:
					break;
				default:
#if defined(OP_PLATFORM_MAC_CFM) && !defined(OP_PLATFORM_MAC_CARBON_FLAG)
					{
						Str255 daName;
						
						GetMenuItemText(GetMenuHandle(mApple), menuItem, daName);
						OpenDeskAcc(daName);
					}
#endif
					break;
			}
			break;

		case mFile:
			switch (menuItem) 
			{
				case iOpen:
				case iOpenPassive:
					handle_open(menuItem==iOpen, false);
					break;
					
				case iOpenActiveUI:
				case iOpenPassiveUI:
					handle_open(menuItem==iOpenActiveUI, true);
					break;
					
				case iClose:
					close_front_window();
					break;
					
				case iQuit:
					handle_quit();
					break;
			}
			break;
			
		case mSpecial:
			switch(menuItem)
			{
				case iSendPacket:
					send_packet();
					break;
				
				case iSendStream:
					send_stream();
					break;
					
				case iProtocolAlive:
					is_alive();
					break;
					
				case iAcceptingConnections:
					accepting_connections= !accepting_connections;
					check_menu_item(menuID, menuItem, accepting_connections);
					break;

				case iActiveEnumeration:
					active_enumeration= !active_enumeration;
					check_menu_item(menuID, menuItem, active_enumeration);
					break;
					
				case iSetTimeout:
					set_timeout();
					break;
					
				case iGetInfo:
					report_info();
					break;
					
				case iGetConfigString:
//					get_config_string();
					break;
			}
			break;
			
		case mWindowMenu:
			switch(menuItem)
			{
				case iCascade:
				case iTile:
					cascade_or_tile_windows(menuItem==iCascade);
					break;
			}
	}
#if OP_PLATFORM_MAC_CFM || OP_PLATFORM_MAC_MACHO
	HiliteMenu(0);
#endif
	adjust_menus();

	return;
}

static void dump_config_string(
	PConfigRef config)
{
	struct endpoint_data *endpoint_data;

	endpoint_data= get_current_endpoint_data();
	if (endpoint_data && endpoint_data->endpoint)
	{
		NMErr err;
		char default_config_string[1024];
	
		/* Echo the default config string. */
		err= ProtocolGetConfigString(config, default_config_string, sizeof(default_config_string));
		if (!err)
		{
			short function_length;
			
			log_text("Created endpoint has configuration: **%s**", default_config_string);
			err= ProtocolGetConfigStringLen(config, &function_length);
			if (!err)
			{
				log_text("Actual Length: %d", strlen(default_config_string));
				log_text("ProtocolGetConfigStringLen: %d",function_length);
			} else {
				log_text("Err: %d in ProtocolGetConfigStringLen", err);
			}
		} else {
			log_text("Err %d on ProtocolGetConfigString", err);
		}
	}
	
	return;
}



/* ------- Miscellaneous Menu Commands -------------- */
void set_menu_enabled(
	short menu_id,
	short item,
	NMBoolean enabled)
{
#if defined(OP_PLATFORM_MAC_CFM) || defined(OP_PLATFORM_MAC_MACHO)
	MenuHandle menu= GetMenuHandle(menu_id);

#ifndef OP_PLATFORM_MAC_CARBON_FLAG
	if (enabled)
	{
		EnableItem(menu, item);
	} else {
		DisableItem(menu, item);
	}
#else
	if (enabled) {
		EnableMenuItem(menu, item);
	} else {
		DisableMenuItem(menu, item);
	}
#endif // OP_PLATFORM_MAC_CARBON_FLAG

#elif defined(OP_PLATFORM_WINDOWS)
	HMENU menu= GetMenu(screen_window);

	op_assert(menu);
	EnableMenuItem(menu, MAKE_MENU_COMMAND(menu_id, item), 
		enabled ? MF_ENABLED : MF_GRAYED);
#endif

	return;
}

void check_menu_item(
	short menu,
	short item,
	NMBoolean checked)
{
#if OP_PLATFORM_MAC_CFM || OP_PLATFORM_MAC_MACHO
	#ifndef OP_PLATFORM_MAC_CARBON_FLAG
	CheckItem(GetMenuHandle(menu), item, checked);
#else
	CheckMenuItem(GetMenuHandle(menu), item, checked);
	#endif
#elif defined(OP_PLATFORM_WINDOWS)
	CheckMenuItem(GetMenu(screen_window), MAKE_MENU_COMMAND(menu, item), MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
#endif
	return;
}



/* ------------- local code */
static void send_packet(
	void)
{
	struct endpoint_data *endpoint_data= get_current_endpoint_data();

	if (endpoint_data && endpoint_data->endpoint)
	{
		NMErr err;
		char data[]= "This is a packet!";
		long length= strlen(data);

		err= ProtocolSendPacket(endpoint_data->endpoint, data, length, 0);
		DEBUG_PRINTonERR("Err %d on ProtocolSendPacket!", err);
	}
	
	return;
}

static void send_stream(
	void)
{
	struct endpoint_data *endpoint_data= get_current_endpoint_data();

	if (endpoint_data && endpoint_data->endpoint)
	{
		NMErr err;
		char data[]= "This is a stream!";
		long length= strlen(data);
		
		err = ProtocolSend(endpoint_data->endpoint, data, length, 0);
		if (err != length) DEBUG_PRINTonERR("Err %d on ProtocolSend!", err);
	}
	
	return;
}

static void is_alive(
	void)
{
	struct endpoint_data *endpoint_data;

	endpoint_data= get_current_endpoint_data();
	if (endpoint_data && endpoint_data->endpoint)
	{
// 19990123 sjb changed NMTriState below to a NMBoolean, NMTriStates are gone
		NMBoolean state;
		
		state= ProtocolIsAlive(endpoint_data->endpoint);
// 19990123 sjb print result as yes or no below
		log_text("Alive: %s", boolean_to_name(state));
	}
	
	return;
}

static void set_timeout(
	void)
{
	struct endpoint_data *endpoint_data= get_current_endpoint_data();
	
	if (endpoint_data && endpoint_data->endpoint)
	{
		NMErr err;
		long timeout;
		
		if (get_timeout_value(&timeout))
		{
			err= ProtocolSetTimeout(endpoint_data->endpoint, timeout);
			if (err) log_text("ProtocolSetTimeout Err: %d", err);
		}
	}
	
	return;
}

static void report_info(
	void)
{
	struct endpoint_data *endpoint_data= get_current_endpoint_data();
	
	if (endpoint_data && endpoint_data->endpoint)
	{
		NMErr err;
		NMModuleInfo info;
	
		/* Echo the data to the screen. */
		err= ProtocolGetEndpointInfo(endpoint_data->endpoint, &info);
		if (!err)
		{
			log_text("--- NMModuleInfo ----");
			log_text("Type: %c%c%c%c", (info.type>>24) & 0xff, (info.type>>16) & 0xff,
				(info.type>>8) & 0xff, (info.type) & 0xff);
			log_text("Name: %s", info.name);
			log_text("Copyright: %s", info.copyright);
			log_text("Max Packet Size: %d", info.maxPacketSize);
// 19990123 sjb some old code here appears to be obsolete format
#if 1
			log_text("stream: %s", boolean_to_name(info.flags & kNMModuleHasStream));
			log_text("datagram: %s", boolean_to_name(info.flags & kNMModuleHasDatagram));
			log_text("expedited: %s", boolean_to_name(info.flags & kNMModuleHasExpedited));
			log_text("requiresIdleTime: %s", boolean_to_name(info.flags & kNMModuleRequiresIdleTime));
#else
// 19990123 sjb below appears to be obsolete format
			log_text("stream: %s", tristate_to_name(info.stream));
			log_text("datagram: %s", tristate_to_name(info.datagram));
			log_text("expedited: %s", tristate_to_name(info.expedited));
			log_text("requiresIdleTime: %s", tristate_to_name(info.requiresIdleTime));
#endif
			log_text("--- End NMModuleInfo ----");
		} else {
			log_text("Err %d on ProtocolGetEndpointInfo", err);
		}
	}
	
	return;
}

// 19990123 sjb removed tristate_to_name as tristates appear to be dead
// adding boolean_to_name
#if 0
static char *tristate_to_name(
	NMTriState state)
{
	char *name;

	switch(state)
	{
		case yes: name= "yes"; break;
		case no: name= "no"; break;
		case unknown: name= "unknown"; break;
		default:  op_halt(); break;
	}
	
	return name;
}
#endif

static char *boolean_to_name(NMBoolean state)
{
	return state ? "yes" : "no";
}

static struct endpoint_data *create_endpoint_from_config(
	PConfigRef config,
	NMBoolean active)
{
	NMOpenFlags flags;
	PEndpointRef endpoint;
	NMErr err;
	struct endpoint_data *endpoint_data= NULL;

	op_assert(config);
	if (active)
	{
		flags= kOpenActive;
	} else {
		flags= kOpenNone;
	}

	endpoint_data= (struct endpoint_data *) malloc(sizeof(struct endpoint_data));
	if (endpoint_data)
	{
		op_assert(endpoint_data);

		// save the endpoint..
		endpoint_data->endpoint= NULL;
		endpoint_data->requires_idle_time= false;
		endpoint_data->closed= false;

		err= ProtocolOpenEndpoint(config, callback_function, endpoint_data, &endpoint, flags);
		if (!err)
		{
			NMModuleInfo info;
			
			//ecf need to start advertising to show up in dialogs
			ProtocolStartAdvertising(endpoint);
			
			// 19990129 sjb endpoint info
			// can't get endpoint info without checking size of info
			info.size = sizeof(info);

			endpoint_data->endpoint= endpoint;

			/* Echo the data to the screen. */
			err= ProtocolGetEndpointInfo(endpoint, &info);
			if (!err)
			{
				endpoint_data->requires_idle_time= (info.flags & kNMModuleRequiresIdleTime) ? true : false;
			}
		} else {
			DEBUG_PRINT("err %d opening endpoint", err);
		}
	} else {
		DEBUG_PRINT("unable to allocate memory for the endpoint data");
	}

	return (err) ? NULL : endpoint_data;
}

static void
callback_function(
	PEndpointRef inEndpoint, 
	void* inContext,
	NMCallbackCode inCode, 
	NMErr inError, 
	void* inCookie)	
{
	static int private_connection_count= 1;
	struct endpoint_data *endpoint_data= (struct endpoint_data *) inContext;

	//ECF 011107 if we've shut down the app, we don't process OP messages anymore
	//a more graceful way would be to make sure we've gotten all our close complete messages
	//and whatnot, but this works...
	if (app_done)
		return;

	set_log_window_to_endpoints_window(inEndpoint);
	switch(inCode)
	{
		case kNMConnectRequest:
			{
				NMErr err;

				interrupt_safe_log_text("Got a connection request!");

				if (accepting_connections)
				{
					struct endpoint_data *new_endpoint;
					
// 19990123 sjb this assertion makes a change to endpoint_cache.out
//              however, sometimes assert is defined to ignore its input
//              so this code works differently in the debug and non-debug
//              cases. But, which is correct? I suspect the count should
//              not be decremented here.
//					assert((endpoint_cache.count-=1)>=0);
// 19990123 sjb replaced above assert with the below one which works same
//              whether debug is on or off
					op_assert(endpoint_cache.count >= 1);
// 19990123 sjb changed post decrement to pre, since the array goes 0 to
//              count-1 and so the count entry is past the end of the array
					new_endpoint= endpoint_cache.endpoint_cache[--endpoint_cache.count];
				
					new_endpoint->endpoint= (PEndpointRef) inCookie;
					new_endpoint->requires_idle_time= false;
					new_endpoint->closed= false;
					err= ProtocolAcceptConnection(inEndpoint, inCookie, 
						callback_function, new_endpoint);
					if (!err)
					{
						interrupt_safe_log_text("Accepting connection...");
					} else {
						interrupt_safe_log_text("Failed connection request: Err: %d", err);
					}
				} else {
					err= ProtocolRejectConnection(inEndpoint, inCookie);
					if (!err)
					{
						interrupt_safe_log_text("Rejected Connection Cookie: 0x%x", inCookie);
					} else {
						interrupt_safe_log_text("Failed rejection request: Err: %d", err);
					}
				}
			}
			break;
			
		case kNMDatagramData:
			{
				NMErr err;
				char buffer[1024];
				unsigned long data_length= sizeof(buffer);
				NMFlags flags;
				
				buffer[0] = 0;
				err= ProtocolReceivePacket(inEndpoint, buffer, &data_length, &flags);
				interrupt_safe_log_text("Got kNMDatagramData: Err %d Data: 0x%x Size: 0x%x flags: 0x%x",
					err, buffer, data_length, flags);
				buffer[data_length]= 0;
				interrupt_safe_log_text("Got kNMDatagramData: **%s**", buffer);
			}
			break;
			
		case kNMStreamData:
			{
				NMErr err;
				char buffer[1024];
				unsigned long data_length= sizeof(buffer);
				NMFlags flags;
				
				buffer[0] = 0;
				err= ProtocolReceive(inEndpoint, buffer, &data_length, &flags);
				interrupt_safe_log_text("Got kNMStreamData: Err %d Data: 0x%x Size: 0x%x flags: 0x%x",
					err, buffer, data_length, flags);
				buffer[data_length]= 0;
				interrupt_safe_log_text("Got kNMStreamData: **%s**", buffer);
			}
			break;

		case kNMFlowClear:
			interrupt_safe_log_text("Got kNMFlowClear");
			break;

		case kNMAcceptComplete:
			interrupt_safe_log_text("Accepted connection: 0x%x Error: %d", inCookie, inError);
			{
				char title[100];
			
//				endpoint_data->endpoint= (PEndpointRef) inCookie;
 				endpoint_data->endpoint= (PEndpointRef) inEndpoint;
				sprintf(title, "Client #%d (0x%x) on host 0x%x", private_connection_count++,inEndpoint, inCookie);
				attach_new_endpoint_to_application(endpoint_data, title, false);
			}
			break;

		case kNMHandoffComplete:
			interrupt_safe_log_text("Got a kHandoffComplete message: Cookie: 0x%x", inCookie);
			break;
			
		case kNMEndpointDied:
			interrupt_safe_log_text("Got an endpoint died message");
			{
				NMErr err;
				err= ProtocolCloseEndpoint(inEndpoint, false);
				if (err) DEBUG_PRINT("Err %d closing endpoint! due to endpoint died", err);
			}
			break;

		case kNMCloseComplete:
			interrupt_safe_log_text("Got a kNMCloseComplete");
			endpoint_data->closed= true;
			break;
			
		default:
			interrupt_safe_log_text("Got unknown inCode in the callback: %d", inCode);
			break;
	}
	
	return;
}

static void handle_open(
	NMBoolean active,
	NMBoolean use_ui)
{
	NMType type;
	
	if (select_protocol(&type))
	{
		PConfigRef config;
		NMErr err;
		NMBoolean create= true;
		
		err= ProtocolCreateConfig(type, 'UbTs', gNameString, gEnumData, gEnumDataLen, NULL, &config);
		if (!err)
		{
			if (use_ui)
			{
				create= configure_protocol(config);
			}

			if (create)
			{
				struct endpoint_data *new_endpoint_data;

				new_endpoint_data= create_endpoint_from_config(config, active);
				if (new_endpoint_data)
				{
					char title[128];
					NMType actual_type= type;
					
					sprintf(title, "%s: 0x%x; Type: '%c%c%c%c'; %s;",
						active ? "Client(Active)" : "Host(Passive)",					
						new_endpoint_data->endpoint,
						(actual_type>>24) & 0xff, (actual_type>>16) & 0xff,	(actual_type>>8) & 0xff, actual_type & 0xff,
						use_ui ? "UI Configured" :  "Default Configured");
				
					attach_new_endpoint_to_application(new_endpoint_data, title, true);
					dump_config_string(config);
				}
			}
			err= ProtocolDisposeConfig(config);
			DEBUG_PRINTonERR("Err %d disposing the config", err);
		} else {
			DEBUG_PRINTonERR("Didn't get config: %d", err);
		}
	}
	
	return;
}
