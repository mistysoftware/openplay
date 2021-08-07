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
 
#ifndef __DIALOG_UTILS_H__
#define __DIALOG_UTILS_H__


#if defined(OP_PLATFORM_MAC_CFM) || defined(OP_PLATFORM_MAC_MACHO)

    #define kUP_ARROW     0x1e
    #define kLEFT_ARROW   0x1c
    #define kRIGHT_ARROW  0x1d
    #define kDOWN_ARROW   0x1f

    #define kTAB          0x9

    #define kPAGE_UP      0xb
    #define kPAGE_DOWN    0xc
    #define kHOME         0x1
    #define kEND          0x4

    #define GET_DIALOG_ITEM_COUNT(dialog) \
              (* ((short *) * ((DialogPeek)(dialog))->items) + 1)


#elif defined(OP_PLATFORM_WINDOWS)

    #define kUP_ARROW     VK_UP
    #define kLEFT_ARROW   VK_LEFT
    #define kRIGHT_ARROW  VK_RIGHT
    #define kDOWN_ARROW   VK_DOWN

    #define kTAB          VK_TAB

    #define kPAGE_UP      VK_PRIOR
    #define kPAGE_DOWN    VK_NEXT
    #define kHOME         VK_HOME
    #define kEND          VK_END

#elif defined(OP_PLATFORM_UNIX)

  /* FIXME - what goes here? */

#else
    #error "This file needs porting."
#endif


#define RECTANGLE_HEIGHT(b)  ((b)->bottom - (b)->top)
#define RECTANGLE_WIDTH(b)   ((b)->right - (b)->left)


#ifdef __cplusplus
extern "C" {
#endif


extern void append_item_to_popup(NMDialogPtr dialog, short item, char *cstring);

#if defined(OP_PLATFORM_WINDOWS)

  extern HINSTANCE application_instance;
  extern HWND screen_window;

  extern long extract_number_from_text_item(NMDialogPtr dialog, short item_number);

  extern short get_dialog_item_value(NMDialogPtr dialog, short control);

#endif


#ifdef __cplusplus
}
#endif


#endif
