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
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#if (!OP_PLATFORM_MAC_MACHO)
	#include <ControlDefinitions.h> // For Universal Headers 3.3+ support
#endif

#include "log.h"

enum
{
	SCROLLBAR_WIDTH= 16,
	SHORT_MAX= 32767
};

const unsigned char LOG_FONT_NAME[] = "\pMonaco";

enum {
	STATUS_BAR_HEIGHT= 16,
	LOG_FONT_SIZE= 9,
	TEXT_INSET= 4,
	INTERRUPT_SAFE_BUFFER_SIZE= 4*KILO,
	MAXIMUM_LOG_BUFFER_SIZE= 24*KILO
};

struct log_data {
	WINDOWPTR wp;
	TEHandle te;
	char interrupt_safe_buffer[INTERRUPT_SAFE_BUFFER_SIZE];
	ControlHandle scrollbar;
	char status[256];
	NMBoolean autoscroll;
	struct log_data *next;
};

static struct log_data *first_log= NULL;
static struct log_data *current_log= NULL;

/* -------- local prototypes */
static void create_scrollbar(WindowPtr wp, struct log_data *log);
static pascal void track_scroll(ControlHandle control, short part_code);
static short scroll_text_window(ControlHandle control, short part);
static short calculate_visible_line_count(TEHandle te, short *line_height);
static int add_text_to_window(struct log_data *log, char *buffer);
static void sync_scrollbar_with_log(struct log_data *log);

static void attach_log_to_list(struct log_data *new_log);
static struct log_data *log_from_wp(WINDOWPTR wp);
static void remove_log_from_list(struct log_data *log);

/* -------- code */
void new_log(
	WindowPtr wp)
{
	Rect destRect, viewRect;
	GrafPtr old_port;
	struct log_data *new_log;
	
	GetPort(&old_port);
	SetPortWindowPort(wp);

#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		GetWindowPortBounds(wp, &viewRect);
	#else
		viewRect= wp->portRect;
	#endif
	
	viewRect.right-= SCROLLBAR_WIDTH; /* Scrollbar */
	viewRect.top+= STATUS_BAR_HEIGHT;
	destRect= viewRect;
	InsetRect(&destRect, TEXT_INSET, 0);

	new_log= (struct log_data *) malloc(sizeof(struct log_data));
	if(new_log)
	{	
		memset(new_log, 0, sizeof(struct log_data));
		new_log->te= TEStyleNew(&destRect, &viewRect);
		new_log->wp= wp;
		new_log->interrupt_safe_buffer[0]= '\0';
		new_log->autoscroll= true;
		if(new_log->te)
		{
			TextStyle	style;
			short		fontNum = 0;			// default font is system

			GetFNum(LOG_FONT_NAME,&fontNum);

			style.tsFont= fontNum;
			style.tsSize= LOG_FONT_SIZE;

			/* Try to set the null text */
			//TESetSelect(0, 0, new_log->te);
			//TESetStyle(doFont | doSize | doColor, &style, false, new_log->te);		

			//add a space, change its style, and delete the space - probably not the best way to do this, but whatever...
			TEInsert(" ",1,new_log->te);
			TESetSelect(0, 100,new_log->te);
			TESetStyle(doSize | doFont,&style,false,new_log->te);
			TEDelete(new_log->te);
			TESetSelect(0,0,new_log->te);

			// match the window font size.
			TextFont(style.tsFont);
			TextSize(style.tsSize);

			create_scrollbar(wp, new_log);
			
			attach_log_to_list(new_log);
			if(!current_log) current_log= new_log;
		} else {
			free(new_log);
		}
	}

	SetPort(old_port);
	
	return;
}

void dispose_log(
	WindowPtr wp)
{
	struct log_data *log= log_from_wp(wp);
	if(log)
	{
		remove_log_from_list(log);
		op_assert(log->te);	
		TEDispose(log->te);
		log->te= NULL;
		free(log);
	}
	
	return;
}

void update_log(
	WindowPtr wp)
{
	struct log_data *log= log_from_wp(wp);

	if(log)
	{
		op_assert(log->te);
		EraseRect(&(**(log->te)).viewRect);

#ifdef OP_PLATFORM_MAC_CARBON_FLAG
	{
		Rect		windowRect;
		GetWindowPortBounds(wp, &windowRect);
		TEUpdate(&windowRect, log->te);
		DrawControls(wp);
		MoveTo(windowRect.left, windowRect.top+STATUS_BAR_HEIGHT-1);
		LineTo(windowRect.right, windowRect.top+STATUS_BAR_HEIGHT-1);
		MoveTo(windowRect.left, windowRect.top+STATUS_BAR_HEIGHT-3);
		LineTo(windowRect.right, windowRect.top+STATUS_BAR_HEIGHT-3);
		// Draw the status bar text.
		MoveTo(windowRect.left+TEXT_INSET, windowRect.top+STATUS_BAR_HEIGHT-3-2);
		DrawText(log->status, 0, strlen(log->status));
	}
	#else
		// pre-carbon code
		TEUpdate(&wp->portRect, log->te);
		UpdateControls(wp, wp->visRgn); 
		MoveTo(wp->portRect.left, wp->portRect.top+STATUS_BAR_HEIGHT-1);
		LineTo(wp->portRect.right, wp->portRect.top+STATUS_BAR_HEIGHT-1);
		MoveTo(wp->portRect.left, wp->portRect.top+STATUS_BAR_HEIGHT-3);
		LineTo(wp->portRect.right, wp->portRect.top+STATUS_BAR_HEIGHT-3);
		// Draw the status bar text.
		MoveTo(wp->portRect.left+TEXT_INSET, wp->portRect.top+STATUS_BAR_HEIGHT-3-2);
		DrawText(log->status, 0, strlen(log->status));
	#endif
		
	}

	return;
}

void set_log_status_text(
	 const char *format,
	...)
{
	struct log_data *log= current_log;
	
	if(log)
	{
		va_list arglist;
		GrafPtr old_port;
		Rect bounds;
		int return_value;
		
		va_start(arglist, format);
		return_value= vsprintf(log->status, format, arglist);
		va_end(arglist);
		op_assert(return_value<sizeof(log->status));

#ifdef OP_PLATFORM_MAC_CARBON_FLAG
			GetWindowPortBounds(log->wp, &bounds);
		#else
			bounds= log->wp->portRect;
		#endif

		bounds.bottom= bounds.top+STATUS_BAR_HEIGHT;

		/* Make sure it gets redrawn. */	
		GetPort(&old_port);
		SetPortWindowPort(log->wp);
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		InvalWindowRect(log->wp, &bounds);
#else
		InvalRect(&bounds);
#endif // OP_PLATFORM_MAC_CARBON_FLAG
		SetPort(old_port);
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
			/* Don't call log_text, because it uses a smaller buffer */
			if(log->te) add_text_to_window(log, log->interrupt_safe_buffer);
			log->interrupt_safe_buffer[0]= '\0';
		}
		log= log->next;
	}
	
	return;
}

void click_in_log_window(
	WindowPtr wp, 
	Point where,
	NMBoolean shiftHeld) 
{
	struct log_data *log= log_from_wp(wp);
	static ControlActionUPP control_action_upp= NULL;
			
	if(!control_action_upp)
	{
		control_action_upp= NewControlActionUPP(track_scroll);
		op_assert(control_action_upp);
	}

	if(log)
	{
		GrafPtr oldPort;
		short cntrl_loc;
		ControlHandle control;
		short current;
		TEHandle te;
		short lineheight;

		GetPort(&oldPort);
		SetPortWindowPort(wp);
		GlobalToLocal(&where);

		te= log->te;
		lineheight= TEGetHeight(0, 0, te);
		cntrl_loc= FindControl(where, wp, &control);
		if(cntrl_loc != 0)  /* clicked in the scroll bar */
		{
			DEBUG_PRINT("in scroll");
			current= GetControlValue(control); /* To allow for proper updating */
			if(cntrl_loc==kControlIndicatorPart) 
			{
				cntrl_loc= TrackControl(control, where, nil);
				if(cntrl_loc) 
				{
					TEScroll(0, lineheight*(current-GetControlValue(control)), te);
				}
			} else {
				TrackControl(control, where, control_action_upp);
			}
		} else {
			TEClick(where, shiftHeld, te);  /* FUTURE FEATURE! */
		}
		SetPort(oldPort);
	}
	
	return;
}

int log_text(
	 const char *format,
	...)
{
	struct log_data *log= current_log;
	
	if(current_log)
	{
// 19990131 sjb add newline to all log items
		char buffer[258]; /* [length byte] + [255 string bytes] + nl + [null] */
		
		va_list arglist;
		int return_value;
		
		va_start(arglist, format);
		return_value= vsprintf(buffer, format, arglist);
		va_end(arglist);
		
		strcat(buffer, "\r");
		
		op_assert(return_value<sizeof(buffer));
		
		add_text_to_window(log, buffer);
	}
	
	return 0;
}

int interrupt_safe_log_text(
	const char *format, ...)
{
	struct log_data *log= current_log;
	
	if(current_log)
	{
		va_list arglist;
		int return_value;
		short initial_length;
		char temporary_copy[256];
		
		initial_length= strlen(log->interrupt_safe_buffer);
		if(initial_length && initial_length+1<INTERRUPT_SAFE_BUFFER_SIZE)
		{
			//strcat(log->interrupt_safe_buffer, "\n");
			//initial_length+= 1;
		}

		va_start(arglist, format);
		return_value= vsprintf(temporary_copy, format, arglist);
		va_end(arglist);

		strcat(temporary_copy,"\r");

		if(initial_length+1+return_value<INTERRUPT_SAFE_BUFFER_SIZE)
		{
			strcat(log->interrupt_safe_buffer, temporary_copy);
		} else {
			DEBUG_PRINT("Dropping text;g");
		}
	}
	
	return 0;
}

void set_current_log_window(
	WINDOWPTR wp)
{
	struct log_data *log= log_from_wp(wp);
	
	current_log= log;
	
	return;
}

/* ----------- local code */
static void create_scrollbar(
	WindowPtr wp,
	struct log_data *log)
{
	Rect scrollRect;
	
	/* Add the scrollbar */
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		GetWindowPortBounds(wp, &scrollRect);
	#else
		scrollRect= wp->portRect;
	#endif

	InsetRect(&scrollRect, -1, -1);
	scrollRect.top+= STATUS_BAR_HEIGHT;
	scrollRect.left= scrollRect.right-SCROLLBAR_WIDTH;

	log->scrollbar= NewControl(wp, &scrollRect, "\p", true, 0, 0, 0, scrollBarProc, 
		(long) (ProcPtr) scroll_text_window); /* Store the scrolling functions */

	return;
}

// stored in the control's refcon
static short scroll_text_window(
	ControlHandle control, 
	short part)
{
	short page_distance;
	TEHandle te;
	short delta= 0;
	WindowPtr wp;
	short current, new_value;
	short lineheight;
	struct log_data *log;

	#if OP_PLATFORM_MAC_CARBON_FLAG
		wp = GetControlOwner(control);
	#else
		wp= (**control).contrlOwner;
	#endif
	
	log= log_from_wp(wp);
	op_assert(log);
	te= log->te;
	op_assert(te);

	page_distance= calculate_visible_line_count(te, &lineheight);
	switch(part) 
	{
		case kControlUpButtonPart:
			delta= -1;
			break;
		case kControlDownButtonPart:
			delta= 1;
			break;
		case kControlPageUpPart:
			delta= -page_distance;
			break;
		case kControlPageDownPart:
			delta= page_distance;
			break;			
	}
	
	current= GetControlValue(control);
	new_value= current+delta;
	if(new_value<0) new_value= 0;
	if(new_value>GetControlMaximum(control)) new_value= GetControlMaximum(control);
	
	TEScroll(0, lineheight*(current-new_value), te);

	return (delta);
}

static pascal void
track_scroll(
	ControlHandle control, 
	short part_code) 
{
	short current, max;
	ProcPtr scrolling_func;
	short delta;
	
	current= GetControlValue(control);
	max= GetControlMaximum(control);
	scrolling_func= (ProcPtr) GetControlReference(control);

	if(scrolling_func) 
	{	
		if((delta= scrolling_func(control, part_code)) != 0) 
		{
			current+= delta;
			if(current<0) current= 0;
			if(current>max) current= max;
		
			SetControlValue(control, current);
		}
	}

	return;
}

static short calculate_visible_line_count(
	TEHandle te,
	short *line_height)
{
	short lineheight;
	short lines_per_page;

	lineheight= TEGetHeight(0, 0, te);						
	lines_per_page= ((*te)->viewRect.bottom-(*te)->viewRect.top)/lineheight;
	
	if(line_height) *line_height= lineheight;
	
	return lines_per_page;
}

static int add_text_to_window(
	struct log_data *log,
	char *buffer)
{
	GrafPtr old_port;
	short scrollmax;
	long size;
	
	op_assert(log);
	op_assert(log->te);

	// now add it..
	GetPort(&old_port);
	SetPortWindowPort(log->wp);
	TEInsert(buffer, strlen(buffer), log->te);

	// don't fill up the 32k buffer.
	size= (*(log->te))->teLength;
	if(size>MAXIMUM_LOG_BUFFER_SIZE)
	{
		CharsHandle text_handle;
		char *start;
		long end_index= 0;
		SInt8 state;
		short lines_deleted_count= 0;

		// grab the text and copy it..	
		text_handle= TEGetText(log->te);
		op_assert(text_handle);
		
		state= HGetState((Handle) text_handle);
		HLock((Handle) text_handle);
		
		// a buffer filled without \n's will fuck us.
		start= *text_handle; end_index= 0;
		while(size>MAXIMUM_LOG_BUFFER_SIZE/2)
		{
			while(*start != '\n' && size>0) 
			{
				lines_deleted_count++;
				start++; size--; end_index++;
			}
			
			// now we really don't want the \r...
			if(size>0) // found a \n
			{
				start++; size--; end_index++;
			}
		}

		HUnlock((Handle) text_handle); // probably unecessary.
		HSetState((Handle) text_handle, state);

		// now delete the text.
		TESetSelect(0, end_index, log->te);
		TEDelete(log->te);
		TESetSelect(SHORT_MAX, SHORT_MAX, log->te);
		
		sync_scrollbar_with_log(log);
	}
	SetPort(old_port);

	/* Determine the lineheight! */
	scrollmax= (*log->te)->nLines- calculate_visible_line_count(log->te, NULL);
	if(scrollmax>0)	SetControlMaximum(log->scrollbar, scrollmax);
	
	if(log->autoscroll && log->scrollbar)
	{
		track_scroll(log->scrollbar, kControlPageDownPart);
	}
	
	return 0;
}

static void sync_scrollbar_with_log(
	struct log_data *log)
{
	int scrollmax, new_scrollmax;
	short line_height;

	scrollmax= GetControlMaximum(log->scrollbar);
	new_scrollmax= (*(log->te))->nLines- calculate_visible_line_count(log->te, &line_height);
	
	TEScroll(0, (scrollmax-new_scrollmax)*line_height, log->te);

	if(new_scrollmax>0)	
	{
		SetControlMaximum(log->scrollbar, new_scrollmax);
		if(GetControlValue(log->scrollbar)>new_scrollmax)
		{
			SetControlValue(log->scrollbar, new_scrollmax);
		}
	}
	
	return;
}

static void attach_log_to_list(
	struct log_data *new_log)
{
	new_log->next= first_log;
	first_log= new_log;
	
	return;
}

static struct log_data *log_from_wp(
	WINDOWPTR wp)
{
	struct log_data *log= first_log;
	
	while(log)
	{
		if(log->wp==wp) break;
		log= log->next;
	}
	
	return log;
}

static void remove_log_from_list(
	struct log_data *log)
{
	if(first_log==log)
	{
		first_log= log->next;
	} else {
		struct log_data *previous= first_log;
		
		while(previous && previous->next != log) previous= previous->next;
		if(previous)
		{
			previous->next= log->next;
		}
	}
	
	if(current_log==log) current_log= first_log;
	
	return;
}
