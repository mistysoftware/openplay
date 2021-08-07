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

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "log.h"

#define EOL_STRING "\r\n"

enum {
	SCROLLBAR_WIDTH= 16,
	STATUS_BAR_HEIGHT= 16,
	TEXT_INSET= 4,
	MAXIMUM_EDIT_LENGTH= 32000
};

struct log_data {
	HWND parent;
	HWND edit;
	char interrupt_safe_buffer[1024];
	char status[256];
	char *edit_buffer;
	struct log_data *next;
};

static struct log_data *first_log= NULL;
static HWND current_log_window= NULL;

static struct log_data *find_log_data_for_window(HWND wp);
static void add_text_to_window(struct log_data *log, char *buffer);

/* -------- code */
void new_log(
	HWND wp)
{
	struct log_data *new_log;
	
	new_log= (struct log_data *) malloc(sizeof(struct log_data));
	if(new_log)
	{
		new_log->parent= wp;
		new_log->edit_buffer= (char *)malloc(MAXIMUM_EDIT_LENGTH);
		new_log->interrupt_safe_buffer[0] = '\0';
		if(new_log->edit_buffer)
		{
			NMRect bounds;
			
			GetClientRect(wp, &bounds);
			new_log->edit= CreateWindow("EDIT", NULL,
				WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL 
				| WS_BORDER | ES_LEFT | ES_MULTILINE |
				ES_AUTOHSCROLL | ES_AUTOVSCROLL, 
				0, 0, RECTANGLE_WIDTH(&bounds), RECTANGLE_HEIGHT(&bounds), 
				wp, NULL, 
				application_instance, NULL);
				
			// attach it.
			new_log->next= first_log;
			first_log= new_log;
		} else {
			free(new_log);
		}
	}

	return;
}

void dispose_log(
	HWND wp)
{
	struct log_data *log= find_log_data_for_window(wp);
	
	if(log)
	{
		
		// remote it from the linked list..
		if(log==first_log)
		{
			first_log= log->next;
		} else {
			struct log_data *previous;

			previous= first_log;
			while(previous->next != log) previous= previous->next;
			op_assert(previous->next==log);
			previous->next= log->next;
		}

		op_assert(log->edit);
		free(log->edit_buffer);
		DestroyWindow(log->edit);
		log->edit= NULL;
		
		free(log);
	}
	
	return;
}

void set_current_log_window(
	HWND wp)
{
	current_log_window= wp;
	
	return;
}

void update_log(
	HWND wp)
{
  UNUSED_PARAMETER(wp)

  return;
}

void set_log_status_text(
	 const char *format,
	...)
{
	va_list arglist;
	struct log_data *log= find_log_data_for_window(current_log_window);
	
	if(log)
	{
		va_start(arglist, format);
		vsprintf(log->status, format, arglist);
		va_end(arglist);
#if 0
	GrafPtr old_port;
	Rect bounds;

	bounds= log_globals.wp->portRect;
	bounds.bottom= bounds.top+STATUS_BAR_HEIGHT;

	/* Make sure it gets redrawn. */	
	GetPort(&old_port);
	SetPort(log_globals.wp);
	InvalRect(&bounds);
	SetPort(old_port);
#endif
	}
	
	return;
}

void idle_log(
	void)
{
	struct log_data *log= first_log;

	while(log)
	{
		if(strlen(log->interrupt_safe_buffer))
		{
			if(log->edit) add_text_to_window(log, log->interrupt_safe_buffer);
			log->interrupt_safe_buffer[0]= '\0';
		}
		log= log->next;
	}
	
	return;
}

void click_in_log_window(
	HWND wp, 
	Point where,
	NMBoolean shiftHeld) 
{
	UNUSED_PARAMETER(wp)
	UNUSED_PARAMETER(where)

	return;
}

int log_text(
	 const char *format,
	...)
{
	struct log_data *log= find_log_data_for_window(current_log_window);

	if(log)
	{
		char buffer[257]; /* [length byte] + [255 string bytes] + [null] */
		va_list arglist;
		int return_value;
		
		va_start(arglist, format);
		return_value= vsprintf(buffer, format, arglist);
		va_end(arglist);
		
		strcat(buffer, EOL_STRING);
		
		op_assert(log->edit);

		add_text_to_window(log, buffer);
	}
	
	return 0;
}

int interrupt_safe_log_text(
	const char *format, ...)
{
	struct log_data *log= find_log_data_for_window(current_log_window);

	if(log)
	{
		va_list arglist;
		int return_value;
		short initial_length;
		
		initial_length= strlen(log->interrupt_safe_buffer);
		if(initial_length)
		{
			//strcat(log->interrupt_safe_buffer, EOL_STRING);
			//initial_length+= strlen(EOL_STRING);
		}
		va_start(arglist, format);
		return_value= vsprintf(&log->interrupt_safe_buffer[initial_length], format, arglist);
		va_end(arglist);
		strcat(log->interrupt_safe_buffer,EOL_STRING);	
	}
	
	return 0;
}

void handle_log_message(
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	struct log_data *log= find_log_data_for_window(hwnd);
	
	UNUSED_PARAMETER(wParam)
	if(log)
	{
		switch(message)
		{
			case WM_SETFOCUS:
				SetFocus(log->edit);
				break;
		
			case WM_SIZE:
				MoveWindow(log->edit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
				break;
		}
	}
	
	return;
}

/* ---------- local code */
static struct log_data *find_log_data_for_window(
	HWND wp)
{
	struct log_data *data;
	
	data= first_log;
	while(data && data->parent != wp) data= data->next;
	
	return data;
}

static void add_text_to_window(
	struct log_data *log,
	char *buffer)
{
	// now add it..
	GetWindowText(log->edit, log->edit_buffer, MAXIMUM_EDIT_LENGTH);
	strcat(log->edit_buffer, buffer);
	SetWindowText(log->edit, log->edit_buffer);

	return;
}
