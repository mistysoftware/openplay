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
#include "dialog_lists.h"
#include "dialog_utils.h"

#if (!OP_PLATFORM_MAC_MACHO)
	#include <StringCompare.h>
#endif

#define SCROLL_BAR_WIDTH (15)
#define MAX_TYPEAHEAD (32)

/* ---------- listbox functions */
#define MAXIMUM_LIST_HANDLES (16) // if a list item value is >= this, this will fail.

struct list_data {
	DialogPtr dialog;
	short item;
	UInt16 flags;
	NMBoolean selected;
	short double_click_item;
	ListHandle list;
	struct list_data *next;
};

struct dialog_list_data {
	UserItemUPP list_drawing_upp;
	ListSearchUPP match_next_alphabetically_upp;
	struct list_data *first;
	long ticks_at_last_key;
	char typed_selection[MAX_TYPEAHEAD];
};

static struct dialog_list_data dialog_lists= {
	0, NULL, 0l
};

/* ------------- local prototypes */
static void handle_tab(NMDialogPtr dialog, NMBoolean forward, short *item_hit);
static struct list_data *find_active_list(NMDialogPtr dialog);
static NMBoolean should_draw_border_on_dialog_list(NMDialogPtr dialog);
static pascal void draw_list(NMDialogPtr dialog, short which_item);
static struct list_data *find_dialog_list_data(NMDialogPtr dialog, short item);
static void make_cell_visible(ListHandle list, Cell cell);
static void mark_list_for_redraw(NMDialogPtr dialog, short item);
static short find_selected_list_or_edittext(NMDialogPtr dialog);
static void select_list(NMDialogPtr dialog, short item);
static pascal short match_next_alphabetically(Ptr cellDataPtr,  Ptr searchDataPtr,  short cellDataLen,  short searchDataLen);
static NMBoolean dialog_has_edit_items(NMDialogPtr dialog);

/* ------------- code */
NMBoolean new_list(
	NMDialogPtr dialog, 
	short item_number,
	NMUInt16 flags)
{
	Point cell_size;
	Rect list_bounds, data_bounds;
	short item_type;
	Handle item_handle;
	NMBoolean success= false;
	struct list_data *list_data;

	// don't create it twice;
	op_assert(!find_dialog_list_data(dialog, item_number));
	op_assert(item_number>0&&item_number <= CountDITL(dialog));
//	op_assert(item_number>0&&item_number<=GET_DIALOG_ITEM_COUNT(dialog));

	/* First time, we allocate it. */
	if(!dialog_lists.list_drawing_upp) 
	{
		dialog_lists.list_drawing_upp = NewUserItemUPP(draw_list); 
		op_assert(dialog_lists.list_drawing_upp != NULL);
	}
	
	if(!dialog_lists.match_next_alphabetically_upp)
	{
		dialog_lists.match_next_alphabetically_upp= NewListSearchUPP(match_next_alphabetically);
		op_assert(dialog_lists.match_next_alphabetically_upp != NULL);
	}
	GetDialogItem(dialog, item_number, &item_type, &item_handle, &list_bounds);
	SetDialogItem(dialog, item_number, item_type, (Handle) dialog_lists.list_drawing_upp, &list_bounds);

	// Make a list
	list_data= (struct list_data *) malloc(sizeof(struct list_data));
	if(list_data)
	{
		ListHandle list;

		GetDialogItem(dialog, item_number, &item_type, &item_handle, &list_bounds);
		list_bounds.right -= SCROLL_BAR_WIDTH;
		cell_size.h = cell_size.v = 0;
		SetRect(&data_bounds, 0, 0, 1, 0);
		list = LNew(&list_bounds, &data_bounds, cell_size, 0, GetDialogWindow(dialog), true, false, false, true);
		if(list)
		{
			/* Setup the queue'd element */
			list_data->dialog= dialog;
			list_data->item= item_number;
			list_data->selected= false;
			list_data->double_click_item= NONE;
			list_data->list= list;
			list_data->flags= flags;

			(*list)->selFlags = lOnlyOne;
			success= true;

			/* Reset the typeahead */
			dialog_lists.typed_selection[0]= 0;
			dialog_lists.ticks_at_last_key= 0l;
			
			list_data->next= dialog_lists.first;
			dialog_lists.first= list_data;
		} else {
			free(list_data);
		}
	}

	return success;
}

void free_list(
	NMDialogPtr dialog,
	short item)
{
	struct list_data *list_data= dialog_lists.first;
	struct list_data **previous= &dialog_lists.first;

	while(list_data)
	{
		if(list_data->dialog==dialog && list_data->item==item)
		{
			LDispose(list_data->list);
			list_data->list= NULL;
			
			*previous= list_data->next;
			free(list_data);
			break;
		}
		previous= &list_data->next;
		list_data= list_data->next;
	}	
	
	return;		
}

/* swiped from tag_interface.c */
short get_listbox_value(
	NMDialogPtr dialog,
	short item)
{
	Cell cell;
	short item_index= NONE;
	struct list_data *list_data;
	
	list_data= find_dialog_list_data(dialog, item);
	op_assert(list_data);

	cell.h = cell.v = 0;
	if(LGetSelect(true, &cell, list_data->list))
	{
		item_index= cell.v;
	}
	
	return item_index;
}

void set_listbox_doubleclick_itemhit(
	NMDialogPtr dialog,
	short item,
	short double_click_item)
{
	struct list_data *list_data;

	list_data= find_dialog_list_data(dialog, item);
	op_assert(list_data);
	
	list_data->double_click_item= double_click_item;
	
	return;
}

void select_list_item(
	NMDialogPtr dialog,
	short item,
	short row)
{
	struct list_data *list_data;
	Cell cell= {0, 0};
	Cell cell_to_select;

	list_data= find_dialog_list_data(dialog, item);
	op_assert(list_data && list_data->list);
	
	cell_to_select.h= 0, cell_to_select.v= row;
	
	/* Deselect all others */
	while(LGetSelect(true, &cell, list_data->list))
	{
		if(cell_to_select.h != cell.h || cell_to_select.v != cell.v)
		{
			LSetSelect(false, cell, list_data->list);
		} else {
			LNextCell(true, true, &cell, list_data->list);
		}
	}
	LSetSelect(true, cell_to_select, list_data->list);
	
	return;
}

short add_text_to_list(
	NMDialogPtr dialog, 
	short item, 
	char *text)
{
	struct list_data *list_data;
	Cell cell;
	
	list_data= find_dialog_list_data(dialog, item);
	op_assert(list_data);
	
	if(list_data->flags & _list_is_alphabetical)
	{
		short row_count= RECTANGLE_HEIGHT(&((*list_data->list)->dataBounds));
		Handle international_resource= GetIntlResource(2);
		Ptr cell_data;

		HLockHi((Handle) (*list_data->list)->cells);
		cell_data= *(*list_data->list)->cells;
		cell.h= 0;
		for(cell.v= 0; cell.v<row_count; cell.v++)
		{
			short cellDataOffset, cellDataLength;
			
			LGetCellDataLocation(&cellDataOffset, &cellDataLength, cell, list_data->list);
			if(CompareText(text, cell_data+cellDataOffset, 
				strlen(text), cellDataLength, international_resource)== -1)
			{
				break;
			}
		}
		HUnlock((Handle) (*list_data->list)->cells);
		
		/* Now add it at whatever row is.. */
		cell.v= LAddRow(1, cell.v, list_data->list);
		LSetCell((unsigned char *) text, strlen(text), cell, list_data->list);
	} else {
		cell.h= 0;
		cell.v= (*list_data->list)->dataBounds.bottom;
		cell.v= LAddRow(1, cell.v, list_data->list);
		LSetCell((unsigned char *) text, strlen(text), cell, list_data->list);
	}
	
	return cell.v;
}

short find_text_in_list(
	NMDialogPtr dialog, 
	short item, 
	char *text)
{
	Cell matching_cell= {0, 0};
	struct list_data *list_data;
	
	list_data= find_dialog_list_data(dialog, item);
	op_assert(list_data);

	/* If we found it.. */
	if(!LSearch(text, strlen(text), NULL, &matching_cell, list_data->list))
	{
		matching_cell.v= NONE;
	}
	
	return matching_cell.v;
}

void delete_from_list(
	NMDialogPtr dialog, 
	short item, 
	short list_index)
{
	struct list_data *list_data;

	list_data= find_dialog_list_data(dialog, item);
	op_assert(list_data);
	LDelRow(1, list_index, list_data->list);

	return;
}

void empty_list(
	NMDialogPtr dialog,
	short item)
{
	struct list_data *list_data;

	list_data= find_dialog_list_data(dialog, item);
	op_assert(list_data);

	/* untested */
	LDelRow(RECTANGLE_HEIGHT(&((*list_data->list)->dataBounds)), 0, list_data->list);

	return;
}

void activate_dialog_lists(
	NMDialogPtr dialog,
	NMBoolean becoming_active)
{
	struct list_data *list_data= dialog_lists.first;

	while(list_data)
	{
		if(list_data->dialog==dialog)
		{
			LActivate(becoming_active, list_data->list);
		}
		list_data= list_data->next;
	}	

	return;
}

NMBoolean list_handled_click(
	NMDialogPtr dialog, 
	Point where,  // in local coords
	short *item_hit, 
	short modifiers)
{
	struct list_data *list_data= dialog_lists.first;
	NMBoolean handled= false;

	while(list_data && !handled)
	{
		if(list_data->dialog==dialog)
		{
			Rect list_bounds;
		
			// account for the scroll bars..	
			list_bounds= (*list_data->list)->rView;
			if((*list_data->list)->hScroll)
			{
				list_bounds.bottom+= SCROLL_BAR_WIDTH;
			}

			if((*list_data->list)->vScroll)
			{			
				list_bounds.right+= SCROLL_BAR_WIDTH;
			}
		
			if(PtInRect(where, &list_bounds))
			{
				select_dialog_list_or_edittext(dialog, list_data->item);
				if(LClick(where, modifiers, list_data->list))
				{
					/* Double click */
					if(list_data->double_click_item != NONE)
					{
						*item_hit= list_data->double_click_item;
					} else {
						*item_hit= list_data->item;
					}
				} else {
					/* Single click */
					*item_hit= list_data->item;
				}
				handled= true;
			}
		}
		list_data= list_data->next;
	}	
	
	return handled;
}

static NMBoolean dialog_has_edit_items(
	NMDialogPtr dialog)
{
	short item_count, item;
	short item_type;
	Handle item_handle;
	Rect item_rectangle;
	
	item_count= CountDITL(dialog);
//	item_count= GET_DIALOG_ITEM_COUNT(dialog);
	for (item=1;item<=item_count;++item)
	{
		GetDialogItem(dialog, item, &item_type, &item_handle, &item_rectangle);
		if (item_type==editText)
		{
			return true;
		}
	}
	
	return false;
}

NMBoolean list_handled_key(
	NMDialogPtr dialog,
	short *item_hit,
	short modifiers,
	short key)
{
	NMBoolean handled= false;
	struct list_data *list_data;
	
	list_data= find_active_list(dialog);
	if(list_data || dialog_has_edit_items(dialog))
	{
		Cell old_cell= {0, 0};

		if(key==kTAB)
		{
			// do tab processing..
			handle_tab(dialog, (modifiers & shiftKey) ? false : true, item_hit);
			handled= true;
		} 
		else if(list_data && LGetSelect(true, &old_cell, list_data->list))
		{
			short column_count, row_count;
			
			column_count= RECTANGLE_WIDTH(&((*list_data->list)->dataBounds));
			row_count= RECTANGLE_HEIGHT(&((*list_data->list)->dataBounds));
	
			switch(key)
			{
				case kUP_ARROW: if(old_cell.v!=0) old_cell.v--; handled= true; break;
				case kDOWN_ARROW: if(old_cell.v!=row_count-1) old_cell.v++; handled= true; break;
				case kLEFT_ARROW: if(old_cell.h!=0) old_cell.h--; handled= true; break;
				case kRIGHT_ARROW: if(old_cell.h!=column_count-1) old_cell.h++; handled= true; break;
				case kHOME: if(old_cell.v!=0) old_cell.v= 0; handled= true; break;
				case kEND: if(old_cell.v!=row_count-1) old_cell.v= row_count-1; handled= true; break;
				default: 
					{
						long max_key_thresh= 2*LMGetKeyThresh();
						short length= strlen(dialog_lists.typed_selection);
						long when= TickCount();
					
						if(max_key_thresh>2*MACHINE_TICKS_PER_SECOND) max_key_thresh= 2*MACHINE_TICKS_PER_SECOND;

						/* Reset if we should.. */
						if(when-dialog_lists.ticks_at_last_key>=max_key_thresh || 
							length+1>=MAX_TYPEAHEAD)
						{
							dialog_lists.typed_selection[0]= 0;
							length= 0;
						}
						
						/* Add it.. */
						dialog_lists.typed_selection[length++]= key;
						dialog_lists.typed_selection[length]= 0;
						dialog_lists.ticks_at_last_key= when;
						
						/* Find it */
						old_cell.v= old_cell.h= 0;
						if(LSearch(dialog_lists.typed_selection, length, dialog_lists.match_next_alphabetically_upp, &old_cell, list_data->list))
						{
							/* Select it.. */
							handled= true;
						}
					}
					break;
			}
			
			if(handled)
			{
				/* Select it, and make sure it is visible.. */
				select_list_item(list_data->dialog, list_data->item, old_cell.v);
				make_cell_visible(list_data->list, old_cell);
			}
		}
	}

	return handled;
}

static void select_list(
	NMDialogPtr dialog,
	short item)
{
	struct list_data *list_data= dialog_lists.first;
	
	while(list_data)
	{
		if(list_data->dialog==dialog)
		{
			if(list_data->item==item) 
			{
				if(!list_data->selected) mark_list_for_redraw(dialog, list_data->item);
				list_data->selected= true;
			} else {
				if(list_data->selected) mark_list_for_redraw(dialog, list_data->item);
				list_data->selected= false;
			}
		}
		list_data= list_data->next;
	}

	return;
}

static void mark_list_for_redraw(
	NMDialogPtr dialog,
	short item)
{
	short item_type;
	Rect item_rectangle;
	Handle item_handle;
	GrafPtr old_port;

	GetPort(&old_port);
	
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		SetPortDialogPort(dialog);
	#else
		SetPort(dialog);
	#endif
	
	GetDialogItem(dialog, item, &item_type, &item_handle, &item_rectangle);
	InsetRect(&item_rectangle, -4, -4);
#ifndef OP_PLATFORM_MAC_CARBON_FLAG
	InvalRect(&item_rectangle);
#else
	InvalWindowRect(GetDialogWindow(dialog), &item_rectangle);
#endif

	SetPort(old_port);
	
	return;
}

static short find_selected_list_or_edittext(
	NMDialogPtr dialog)
{
	short old_field;
	struct list_data *list_data= find_active_list(dialog);
	
	if(list_data)
	{
		/* Lists take priority.. */
		old_field= list_data->item;
	} else {
		old_field = GetDialogKeyboardFocusItem(dialog);
//		old_field= ((DialogPeek)dialog)->editField+1;
	}
	
	return old_field;
}

static void handle_tab(
	NMDialogPtr dialog,
	NMBoolean forward,
	short *item_hit)
{
	short delta, index, item_count, old_field;
	NMBoolean selected_item= false;
	
	delta= (forward) ? 1 : -1;
	item_count= CountDITL(dialog);
//	item_count= GET_DIALOG_ITEM_COUNT(dialog);
	old_field= find_selected_list_or_edittext(dialog);
	
	for(index= 1; !selected_item && index<=item_count; ++index)
	{	
		short item_type, item;
		Handle item_handle;
		Rect item_rectangle;

		// we do voodoo
		item= (old_field+item_count+(index*delta))%item_count;

		GetDialogItem(dialog, item, &item_type, &item_handle, &item_rectangle);
		if (item_type==editText)
		{
			select_dialog_list_or_edittext(dialog, item);
			*item_hit= item;
			selected_item= true;
		} else {
			struct list_data *list_data= find_dialog_list_data(dialog, item);
			if(list_data)
			{
				select_dialog_list_or_edittext(dialog, item);
				*item_hit= item;
				selected_item= true;
			}
		}
	}
	
	return;
}

void select_dialog_list_or_edittext(
	NMDialogPtr dialog,
	short item)
{
	short item_type, old_item_type, old_item;
	Handle item_handle;
	Rect item_rectangle;

	old_item= find_selected_list_or_edittext(dialog);
	GetDialogItem(dialog, old_item, &old_item_type, &item_handle, &item_rectangle);
	GetDialogItem(dialog, item, &item_type, &item_handle, &item_rectangle);

	if(old_item_type==editText)
	{
		/* Deselect the old one. */
		SelectDialogItemText(dialog, old_item, 0, 0);
	}
	
	if(item_type==editText)
	{
		SelectDialogItemText(dialog, item, 0, 32767);
		select_list(dialog, NONE);
	} else {
		select_list(dialog, item);
	}

	/* Reset typeahead */
	dialog_lists.typed_selection[0]= 0;

	return;
}
	
static struct list_data *find_active_list(
	NMDialogPtr dialog)
{
	struct list_data *active_list= NULL;
	struct list_data *list_data= dialog_lists.first;
	
	while(list_data)
	{
		if(list_data->dialog==dialog)
		{
			/* If there was no active list, or this one was selected and the previous one wasn't.. */
			if(list_data->selected)
			{
				active_list= list_data;
			}
		}
		list_data= list_data->next;
	}
	
	return active_list;
}

static NMBoolean should_draw_border_on_dialog_list(
	NMDialogPtr dialog)
{
	struct list_data *list_data= dialog_lists.first;
	short count= 0;
	NMBoolean should_draw= false;

	while(list_data)
	{
		if(list_data->dialog==dialog)
		{
			count++;
		}
		list_data= list_data->next;
	}	

	if(count)
	{
		if(count>1)
		{
			should_draw= true;
		} else {
			/* Only if we have an edit text */
			should_draw= dialog_has_edit_items(dialog);
		}
	}
	
	return should_draw;
}

static pascal void draw_list(
	NMDialogPtr dialog, 
	short which_item)
{
	struct list_data *list_data;
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		Pattern		thePattern;
	#endif
	
	list_data= find_dialog_list_data(dialog, which_item);
	op_assert(list_data);
	if(list_data)
	{
		Rect     item_box;
		short    item_type;
		Handle   item_handle;
		GrafPtr		old_port;
		PenState	current_pen;

		GetPort(&old_port);
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
			SetPortDialogPort(dialog);
		#else
			SetPort(dialog);
		#endif
		
		GetPenState(&current_pen);
		
		GetDialogItem(dialog, which_item, &item_type, &item_handle, &item_box);
		InsetRect(&item_box, -1, -1);
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
			PenPat(GetQDGlobalsBlack(&thePattern));
		#else
			PenPat(&qd.black);
		#endif
		FrameRect(&item_box);

#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		{
			RgnHandle	windowRgn = NewRgn();
			GetPortVisibleRegion(GetDialogPort(dialog), windowRgn);
			LUpdate(windowRgn, list_data->list);
			DisposeRgn(windowRgn);
		}
		#else
			LUpdate(dialog->visRgn, list_data->list);
		#endif

		/* Draw the 2 pixel border if it is selected.. */
		if(should_draw_border_on_dialog_list(dialog))
		{
			Rect outline;
			
			outline= (*list_data->list)->rView;
			if((*list_data->list)->vScroll)
			{
				outline.right= outline.right+SCROLL_BAR_WIDTH;
			}
			
			if((*list_data->list)->hScroll)
			{
				outline.bottom= outline.bottom+SCROLL_BAR_WIDTH;
			}
			
			InsetRect(&outline, -4, -4); // 2 pixel outline, 3 pixels from border
			
			/* Only if this one is selected... */
			if((*list_data->list)->lActive && list_data->selected)
			{
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
				PenPat(GetQDGlobalsBlack(&thePattern));
#else
				PenPat(&qd.black);
#endif
			} else {
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
					PenPat(GetQDGlobalsWhite(&thePattern));
				#else
					PenPat(&qd.white);
				#endif
			}
			PenSize(2, 2);
			FrameRect(&outline);
		}
		
		SetPenState(&current_pen);
		SetPort(old_port);
	}

	return;
}

static struct list_data *find_dialog_list_data(
	NMDialogPtr dialog,
	short item)
{
	struct list_data *list_data= dialog_lists.first;

	while(list_data)
	{
		if(list_data->dialog==dialog && list_data->item==item)
		{
			break;
		}
		list_data= list_data->next;
	}	
	
	return list_data;
}

static void make_cell_visible(
	ListHandle list,
	Cell cell)
{
	Rect visibleRect;
	
	visibleRect= (*list)->visible;

	if (!PtInRect(cell, &visibleRect))
	{
		short delta_cols, delta_rows;
		
		if(cell.h>visibleRect.right-1) 
		{
			delta_cols= cell.h-visibleRect.right+1;
		} 
		else if(cell.h<visibleRect.left) 
		{
			delta_cols= cell.h-visibleRect.left;
		}
		
		if(cell.v>visibleRect.bottom-1) 
		{
			delta_rows= cell.v-visibleRect.bottom+1;
		} else if(cell.v<visibleRect.top) {
			delta_rows= cell.v-visibleRect.top;
		}

		LScroll(delta_cols, delta_rows, list);		
	}
	
	return;
}

static pascal short match_next_alphabetically(
	Ptr cellDataPtr, 
	Ptr searchDataPtr, 
	short cellDataLen, 
	short searchDataLen)
{
	short return_value= 1;
	
	if(cellDataLen>0)
	{
		if (IdenticalText(cellDataPtr, searchDataPtr,  searchDataLen, searchDataLen, nil) == 0)
		{
			return_value= 0; // strings are equal
		} 
		else if (IdenticalText(cellDataPtr, searchDataPtr, cellDataLen, searchDataLen, nil) == 1)
		{
			return_value= 0; // search data is after this cell.
		}
	}
	
	return return_value;
}
