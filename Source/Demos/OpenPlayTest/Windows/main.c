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

#include "testbed.h"
#include "OpenPlayTest_resources.h"
#include "log.h"

#include "dialog_lists.h"


#define szClass TEXT(__FILE__)
#define szTitle TEXT("OpenPlayTest App")
#define szEndpointClass TEXT("EndpointClass")
#define szEndpointTitle TEXT("Endpoint")

/* -------- structures */
struct new_endpoint_buffer {
	struct endpoint_data *endpoint;
	char title[128];
};

#define MAXIMUM_ENDPOINTS_IN_CACHE (10)

struct {
	short count;
	struct new_endpoint_buffer endpoints[MAXIMUM_ENDPOINTS_IN_CACHE];
} new_endpoint_cache= { 0 };

/* -------- globals */
HINSTANCE application_instance= NULL;
HWND screen_window= NULL;
HWND hWndMDIClient= NULL;

#define IDM_WINDOWCHILD MAKE_MENU_COMMAND(200, 0)

NMBoolean app_done;
NMBoolean accepting_connections = false;
NMBoolean active_enumeration = false;

/* -------- local prototypes */
static BOOL InitApplication(HANDLE hInstance);
static BOOL InitInstance(HANDLE hInstance, int nCmdShow);

static LONG APIENTRY MainWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam);
static LONG APIENTRY EndpointWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam);

static LRESULT CALLBACK select_protocol_proc(HWND dialog, UINT uMessage, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK set_timeout_proc(HWND dialog, UINT uMessage, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK configure_protocol_proc(HWND dialog, UINT uMessage, WPARAM wParam, 
	LPARAM lParam);
	
/* ------- entry point */
int APIENTRY WinMain(
	HINSTANCE hInstance, 
	HINSTANCE hPrevInstance, 
	LPSTR lpCmdLine, 
	int nCmdShow)
{
  MSG msg;

  UNUSED_PARAMETER(hPrevInstance)
  UNUSED_PARAMETER(lpCmdLine)

	app_done = false;

	InitApplication(hInstance);

	if(InitInstance(hInstance, nCmdShow))
	{
		// ecf - set the initial menu state in case we enable this by default
		check_menu_item(mSpecial, iActiveEnumeration, active_enumeration);
	
		while(GetMessage(&msg, 0, 0, 0)) 
		{
			short index;
		
			if(!TranslateMDISysAccel(hWndMDIClient, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				idle(); // this isn't really idle, as you don't get time if nothing is going on.
			}

			/* Do things that can only be done at system time */
			for(index= 0; index<new_endpoint_cache.count; ++index)
			{
				attach_new_endpoint_to_application(new_endpoint_cache.endpoints[index].endpoint,
					new_endpoint_cache.endpoints[index].title, TRUE);
			}
			new_endpoint_cache.count= 0;
		}
	}

	app_done = true;
	return 0;
}

/* --------- local code */
static BOOL InitApplication(
	HANDLE hInstance)     
{
	WNDCLASS  wc;
	BOOL success= FALSE;

	memset(&wc, 0, sizeof(wc));
	wc.style= 0;
	wc.lpfnWndProc= (WNDPROC)MainWndProc;
	wc.cbClsExtra= 0;
	wc.cbWndExtra= sizeof(struct endpoint_data *);
	wc.hInstance= (HINSTANCE)hInstance;
	wc.hIcon= LoadIcon((HINSTANCE)hInstance, NULL) ;
	wc.hCursor= LoadCursor(NULL, NULL);
	wc.hbrBackground= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName= MAKEINTRESOURCE(rMENU_BAR_ID);
	wc.lpszClassName= szClass;

	if(RegisterClass(&wc))
	{
		wc.style= 0;
		wc.lpfnWndProc= (WNDPROC)EndpointWndProc;
		wc.cbClsExtra= 0;
		wc.cbWndExtra= sizeof(struct endpoint_data *);
		wc.hInstance= (HINSTANCE)hInstance;
		wc.hIcon= LoadIcon((HINSTANCE)hInstance, NULL) ;
		wc.hCursor= LoadCursor(NULL, NULL);
		wc.hbrBackground= (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName= NULL;
		wc.lpszClassName= szEndpointClass;

		success= RegisterClass(&wc);
	}
	
	return success;
}

static BOOL InitInstance(
	HANDLE hInstance, 
	int nCmdShow)
{
    DWORD dwFlags = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN;
    
    /* save the instance handle in a global variable */
    application_instance = (HINSTANCE)hInstance;

    /* Create the main window */
    screen_window = CreateWindow(szClass, szTitle, dwFlags, 
		10, 10, 640, 600, (HWND)NULL, NULL, application_instance, NULL) ;

    if (!screen_window)  return (FALSE);

    ShowWindow(screen_window, nCmdShow) ;
    UpdateWindow(screen_window);

    return (TRUE);
}

static LONG APIENTRY MainWndProc(
	HWND hWnd, 
	UINT message, 
	UINT wParam, 
	LONG lParam)
{
	LONG return_value= 0;

	switch (message)
	{
		case WM_CREATE:
			{
			    CLIENTCREATESTRUCT ccs;
			    
			    ccs.hWindowMenu= GetSubMenu(GetMenu(hWnd), 3);
			    ccs.idFirstChild= IDM_WINDOWCHILD;
			    
			    /* Create the new subwindow window */
			    /* A mdi window is invisible, and is just a container? */
			    hWndMDIClient = CreateWindow("MDICLIENT", NULL, 
			    	WS_CHILD | WS_CLIPCHILDREN, 
					0, 0, 0, 0, hWnd, NULL, application_instance, (LPSTR) &ccs) ;
				ShowWindow(hWndMDIClient, SW_SHOW);
			}
			break;
	
		case WM_COMMAND:
			do_menu_command(LOWORD(wParam));
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
			
		default:
			return_value= DefFrameProc(hWnd, hWndMDIClient, message, wParam, lParam);
			break ;
	}

    return return_value;
}

/* ------ functions required */
void cascade_or_tile_windows(
	NMBoolean cascade)
{
	if(cascade)
	{
		SendMessage(hWndMDIClient, WM_MDICASCADE, 0, 0);
	} else {
		SendMessage(hWndMDIClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
	}
	
	return;
}

struct endpoint_data *get_current_endpoint_data(
	void)
{
	struct endpoint_data *active_endpoint= NULL;
	HWND top_window= GetTopWindow(hWndMDIClient);
		
	if(top_window)
	{
		active_endpoint= (struct endpoint_data *) GetWindowLong(top_window, 0);
		set_current_log_window(top_window);
	}

	return active_endpoint;
}

// gross hack, i am tired and don't want to write iterator functions
HWND last_endpoint_window;

struct endpoint_data *get_front_endpoint(
	void)
{
	struct endpoint_data *active_endpoint= NULL;
	
	last_endpoint_window= GetTopWindow(hWndMDIClient);
	if(last_endpoint_window)
	{
		active_endpoint= (struct endpoint_data *) GetWindowLong(last_endpoint_window, 0);
	}

	return active_endpoint;
}

struct endpoint_data *get_next_endpoint(
	void)
{
	struct endpoint_data *active_endpoint= NULL;

	op_assert(last_endpoint_window);
	last_endpoint_window= GetNextWindow(last_endpoint_window, GW_HWNDNEXT);
	if(last_endpoint_window)
	{
		active_endpoint= (struct endpoint_data *) GetWindowLong(last_endpoint_window, 0);
	}
	
	return active_endpoint;
}

void attach_new_endpoint_to_application(
	struct endpoint_data *endpoint,
	char *title,
	NMBoolean system_task_time)
{
	HWND new_window;
    DWORD dwFlags = 0;
  
    /* Create the new subwindow window */
    if(system_task_time)
    {
	    op_assert(hWndMDIClient);
	    new_window= CreateMDIWindow(szEndpointClass, title, dwFlags, 
	    	CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	    	hWndMDIClient, application_instance, 0l);
	    op_assert(new_window);
		new_log(new_window);
		SetWindowLong(new_window, 0, (long) endpoint);
		ShowWindow(new_window, SW_SHOW);
	} else {		
		// copy into a cache for later servicing.
		op_assert(new_endpoint_cache.count<MAXIMUM_ENDPOINTS_IN_CACHE);
		new_endpoint_cache.endpoints[new_endpoint_cache.count].endpoint= endpoint;
		strcpy(new_endpoint_cache.endpoints[new_endpoint_cache.count].title, title);
		new_endpoint_cache.count++;
	}
	
	return;
}

void detach_endpoint_from_application(
	struct endpoint_data *endpoint)
{
	struct endpoint_data *data= NULL;
	HWND wp;
	NMBoolean removed= FALSE;

	wp= GetTopWindow(hWndMDIClient);
	while(wp && !removed)
	{
		data= (struct endpoint_data *) GetWindowLong(wp, 0);

		// get rid of it.
		if(data==endpoint)
		{
			free(endpoint);
			dispose_log(wp);
			DestroyWindow(wp);
			removed= TRUE;
		} else {
			wp= GetNextWindow(wp, GW_HWNDNEXT);
		}
	}
	op_assert(removed);

	return;
}

void set_log_window_to_endpoints_window(
	PEndpointRef endpoint)
{
	HWND window= GetTopWindow(hWndMDIClient);

	while(window)
	{	
		struct endpoint_data *active_endpoint= (struct endpoint_data *) GetWindowLong(window, 0);

		if(active_endpoint && active_endpoint->endpoint==endpoint)
		{
			set_current_log_window(window);
			break;
		}
		
		window= GetNextWindow(window, GW_HWNDNEXT);
	}
	
	return;
}

void close_front_window(
	void)
{
	HWND top_window= GetTopWindow(hWndMDIClient);

	if(top_window)
	{	
		SendMessage(top_window, WM_CLOSE, 0, 0);
	}
	
	return;
}

static void close_window(
	HWND window)
{
	struct endpoint_data *endpoint;
	NMErr err;
	
	endpoint= (struct endpoint_data *) GetWindowLong(window, 0);

	err= ProtocolCloseEndpoint(endpoint->endpoint, TRUE);
	DEBUG_PRINTonERR("Err %d closing endpoint!", err);
	
	free(endpoint);
	endpoint= NULL;
	
	// destroy the window
	SendMessage(hWndMDIClient, WM_MDIDESTROY, (WPARAM) window, 0l);

	return;
}

void handle_quit(
	void)
{
	PostQuitMessage(0);
	
	return;
}

enum {
	iOK= 1,
	iCancel
};

NMBoolean get_timeout_value(
	long *timeout)
{
	FARPROC farproc;
	short item_hit;
	
	/* Build the dialog */
	farproc= MakeProcInstance((FARPROC) set_timeout_proc, application_instance);
	item_hit= DialogBoxParam(application_instance, MAKEINTRESOURCE(dlogSetTimeout),
		screen_window, (DLGPROC) farproc, (LPARAM) timeout);

	return (item_hit==iOK);
}

NMBoolean select_protocol(
	NMType *type)
{
	FARPROC farproc;
	short item_hit;
	
	/* Build the dialog */
	farproc= MakeProcInstance((FARPROC) select_protocol_proc, application_instance);
	item_hit= DialogBoxParam(application_instance, MAKEINTRESOURCE(dlogSelectProtocol),
		screen_window, (DLGPROC) farproc, (LPARAM) type);

	return (item_hit==iOK);
}

NMBoolean configure_protocol(
	PConfigRef config)
{
	FARPROC farproc;
	short item_hit;
	
	/* Build the dialog */
	farproc= MakeProcInstance((FARPROC) configure_protocol_proc, application_instance);
	item_hit= DialogBoxParam(application_instance, MAKEINTRESOURCE(dlogConfigureProtocol),
		screen_window, (DLGPROC) farproc, (LPARAM) config);

	return item_hit;
}

/* -------- callbacks */
static LRESULT CALLBACK set_timeout_proc(
	HWND dialog, 
	UINT uMessage,
	WPARAM wParam, 
	LPARAM lParam)
{
	LRESULT returnhand= 0;				// sjb 19990311 yes, this was handled by default. I don't like this
	static long *timeout;

	switch(uMessage)
	{
		case WM_INITDIALOG:
		{
			timeout= (long*) lParam;
			// we didn't set the focus
		}
		break;
			
		case WM_COMMAND:
		{
			WORD id= LOWORD(wParam);

			switch(id)
			{
				case iOK:
					*timeout= extract_number_from_text_item(dialog, iTimeout);
				// fall through
				case iCancel:
					EndDialog(dialog, id);
				break;

				default:
				break;
			}
		}
		break;
			
		default:
			returnhand = DefWindowProc(dialog,uMessage,wParam,lParam);
		break;
	}
	
	return returnhand;
}


static LRESULT CALLBACK select_protocol_proc(
	HWND dialog, 
	UINT uMessage,
	WPARAM wParam, 
	LPARAM lParam)
{
	LRESULT returnhand= 0;				// sjb 19990311 yes, this was handled by default. I don't like this
	short index;
	static short count;
	static NMType module_types[64];
	static NMType *type;

	switch(uMessage)
	{
		case WM_INITDIALOG:
		{
			NMErr err;
		
			type= (NMType*)lParam;
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
					if(err != kNMNoMoreNetModulesErr) DEBUG_PRINT("Err %d in GetIndexedProtocol...", err);
				}
			}
			// we didn't set the focus
		}
		break;

		case WM_COMMAND:
		{
			WORD id= LOWORD(wParam);

			switch(id)
			{
				case iOK:
					index= get_dialog_item_value(dialog, iPROTOCOL_LIST)-1;
					op_assert(index>=0 && index<count);
					*type= module_types[index];
				// fall through
				case iCancel:
					EndDialog(dialog, id);
				break;

				default:
				break;
			}
		}
		break;
			
		default:
			returnhand = DefWindowProc(dialog,uMessage,wParam,lParam);
		break;
	}
	
	return returnhand;
}

enum {
	IDLE_TIME_TIMER= 20,
	IDLE_TIME_TASK_PER_SECOND= 10
};

static LRESULT CALLBACK configure_protocol_proc(
	HWND dialog, 
	UINT uMessage,
	WPARAM wParam, 
	LPARAM lParam)
{
	LRESULT returnhand= 0;				// sjb 19990311 yes, this was handled by default. I don't like this
	static struct list_item_data list_items;
	NMEvent event;
	NMErr err;
	BOOLEAN handled;
	static PConfigRef config;

	// sjb 19990317 first things first, memorize the config parameter in a create
	// this is necessary as we use it to call ProtocolHandleEvent before we run
	// the WM_INITDIALOG handler in the switch statement below
	if (WM_INITDIALOG == uMessage)
	{
		config = (PConfigRef) lParam;
	}

	// build the event	
	event.hWnd= dialog;
	event.msg = uMessage;
	event.wParam= wParam;
	event.lParam= lParam;

// sjb 19990311 what to do about this?
	handled = ProtocolHandleEvent(dialog, &event, config);
	if(!handled)
	{
		switch(uMessage)
		{
			case WM_INITDIALOG:
			{
				NMRect min_bounds;
			
				list_items.dialog= dialog;
				list_items.count= 0;
				new_list(dialog, iGameList, 0);

				ProtocolGetRequiredDialogFrame(&min_bounds, config);
				DEBUG_PRINT("Req'd DialogFrame: (top left width height) %d %d %d %d!", 
					min_bounds.top, min_bounds.left, RECTANGLE_WIDTH(&min_bounds), RECTANGLE_HEIGHT(&min_bounds));

				err= ProtocolSetupDialog(dialog, iProtocolDataFrame, iFirstOpenPlayDialogItem, config);
				DEBUG_PRINTonERR("Err %d in ProtocolSetupDialog!", err);

				// start an enumeration
				err= ProtocolStartEnumeration(config, NMEnumerationCallback, &list_items, active_enumeration);
				DEBUG_PRINTonERR("Err %d in ProtocolStartEnumeration!", err);

				SetTimer(dialog, IDLE_TIME_TIMER, 1000/IDLE_TIME_TASK_PER_SECOND, NULL);

				// we didn't set the focus
			}
			break;

			case WM_TIMER:
				err= ProtocolIdleEnumeration(config);
				DEBUG_PRINTonERR("Err %d in ProtocolIdleEnumeration", err);
			break;
				
			case WM_COMMAND:
			{
				WORD id= LOWORD(wParam);
				NMBoolean success= FALSE;

				switch(id)
				{
					case iCreateGame:
					// fallthrough to join game
					case iJoinGame:
						success= TRUE;
						if (id==iJoinGame)
						{
							short index;
						
							// ÒAnd one to bind themÉÓ
							index= get_listbox_value(dialog, iGameList);
							op_assert(index>=0 && index<list_items.count);
							
							err= ProtocolBindEnumerationToConfig(config, list_items.host[index]);
							DEBUG_PRINTonERR("ProtocolEndEnumeration returned %d!", err);
						}
					// fallthrough
					case iCancel:
						//	End the enumeration AFTER we bind it.  Should probably have a ref for these...
						err= ProtocolEndEnumeration(config);
						DEBUG_PRINTonERR("ProtocolEndEnumeration returned %d!", err);

						if (!ProtocolDialogTeardown(dialog, (iCreateGame == id), config))
						{
							DEBUG_PRINT("Oops, ProtocolDialogTeardown returned FALSE!");
						}
						free_list(dialog, iGameList);
						EndDialog(dialog, (iCancel == id) ? 0 : 1);
					break;

					default:
						returnhand = ProtocolHandleItemHit(dialog, id, config);
					break;
				}
			}
			break;

			case WM_DESTROY:
				KillTimer(dialog, IDLE_TIME_TIMER);
			break;
				
			default:
				returnhand = DefWindowProc(dialog,uMessage,wParam,lParam);
			break;
		}
	}
	
	return returnhand;
}

static LONG APIENTRY EndpointWndProc(
	HWND hWnd, 
	UINT message, 
	UINT wParam, 
	LONG lParam)
{
	LONG return_value= 0;

	handle_log_message(hWnd, message, wParam, lParam);
	switch (message)
	{
		case WM_CLOSE:
			close_window(hWnd);
			break;
		default:
			return_value= DefMDIChildProc(hWnd, message, wParam, lParam);
			break ;
	}

    return return_value;
}
