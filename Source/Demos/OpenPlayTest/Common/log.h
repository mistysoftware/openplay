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

#ifndef __LOG__
#define __LOG__

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
    
#if defined(OP_PLATFORM_MAC_CFM) || defined(OP_PLATFORM_MAC_MACHO)
typedef WindowPtr WINDOWPTR;
#elif defined(OP_PLATFORM_WINDOWS)
typedef HWND WINDOWPTR;
typedef struct point2d {
	short x;
	short y;
} Point;
#endif

void new_log(WINDOWPTR wp);
void dispose_log(WINDOWPTR wp);

void update_log(WINDOWPTR wp);
void idle_log(void);

void click_in_log_window(WINDOWPTR wp, Point where, NMBoolean shiftHeld);

void set_log_status_text(const char *format, ...);

int log_text(const char *format, ...);
int interrupt_safe_log_text(const char *format, ...);

void set_current_log_window(WINDOWPTR wp);

#ifdef OP_PLATFORM_WINDOWS
void handle_log_message(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif

#endif
