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

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
#include "OPUtils.h"
#include "portable_files.h"
#include "find_files.h"

/* --------- local prototypes */
static int alphabetical_names(const FileDesc *a, const FileDesc *b);

static NMErr enumerate_files(find_file_pb *param_block, NMSInt32 directory_id);
static NMErr get_directories_parID(FSSpec *directory, NMSInt32 *parID);
static NMErr get_search_parID(FSSpec *directory, NMSInt32 *parID);
	
/* ---------------- Parameter Block Version */

//----------------------------------------------------------------------------------------
// find_files
//----------------------------------------------------------------------------------------

NMErr find_files(
	find_file_pb *param_block)
{
  NMErr err= kNMNoError;


  op_assert(param_block);

  /* If we need to create the destination buffer */
  if(param_block->flags & _ff_create_buffer) 
  {
    op_assert(param_block->search_type ==_fill_buffer);
    param_block->buffer = (FileDesc *) new_pointer(sizeof(FileDesc)*param_block->max);
  }

  /* Assert that we got it. */
  op_assert(param_block->search_type == _callback_only || param_block->buffer);
  op_assert(param_block->version == 0);

  /* Set the variables */
  param_block->count = 0;

  {
    NMSInt32 directory_id;

    err = get_search_parID((FSSpec *) &param_block->start_search_from, &directory_id);
    if(!err)
    {
      err = enumerate_files(param_block, directory_id);
    }
  }

	/* Alphabetical */
	if(param_block->flags & _ff_alphabetical)
	{
		op_assert(param_block->search_type==_fill_buffer);
		qsort(param_block->buffer, param_block->count,sizeof(FileDesc), (int (*)(const void *, const void *))alphabetical_names);
	}

	/* If we created the buffer, make it the exact right size */
	if(param_block->flags & _ff_create_buffer)
	{
		op_assert(param_block->search_type==_fill_buffer);
		param_block->buffer= (FileDesc *)realloc(param_block->buffer, sizeof(FileDesc)*param_block->count);
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// equal_filedescs
//----------------------------------------------------------------------------------------

NMBoolean equal_filedescs(
	FileDesc *a, 
	FileDesc *b)
{
  NMBoolean equal = false;
	
  if(a->vRefNum == b->vRefNum && a->parID == b->parID && 
     EqualString(a->name, b->name, false, false))
  {
    equal = true;
  }
	
	return equal;
}

/* --------- Local Code --------- */

//----------------------------------------------------------------------------------------
// enumerate_files
//----------------------------------------------------------------------------------------

static NMErr enumerate_files(
	find_file_pb *param_block, 
	NMSInt32 directory_id) /* Because it is recursive.. */
{
	static CInfoPBRec pb; /* static to prevent stack overflow.. */
	static FileDesc temp_file;
	static NMErr err;
	NMSInt16 index;

	machine_mem_zero(&pb, sizeof(CInfoPBRec));
	
	pb.hFileInfo.ioVRefNum= param_block->start_search_from.vRefNum;
	pb.hFileInfo.ioNamePtr= temp_file.name;
			
	for(err= kNMNoError, index=1; param_block->count<param_block->max && err==kNMNoError; index++) 
	{
		pb.hFileInfo.ioDirID= directory_id;
		pb.hFileInfo.ioFDirIndex= index;

		err= PBGetCatInfoSync(&pb);
		if(!err)
		{
			if ((pb.hFileInfo.ioFlAttrib & 16) && (param_block->flags & _ff_recurse))
			{
				/* Recurse, if you really want to... */
				err= enumerate_files(param_block, pb.dirInfo.ioDrDirID);
			} else {
				/* Add.. */
				if(param_block->type_to_find==WILDCARD_TYPE || 
					pb.hFileInfo.ioFlFndrInfo.fdType==param_block->type_to_find)
				{
					/* Gotta set these here, because otherwise they will be wrong. */
					temp_file.vRefNum= pb.hFileInfo.ioVRefNum;
					temp_file.parID= directory_id;
					
					/* Only add if there isn't a callback or it returns true */
					switch(param_block->search_type)
					{
						case _fill_buffer:
							if(!param_block->callback || param_block->callback(&temp_file, param_block->user_data))
							{
								/* Copy it in.. */
								BlockMove(&temp_file, &param_block->buffer[param_block->count++], 
									sizeof(FSSpec));
							}
							break;
							
						case _callback_only:
							op_assert(param_block->callback);
							if(param_block->flags & _ff_callback_with_catinfo)
							{
								param_block->callback(&temp_file, &pb);
							} else {
								param_block->callback(&temp_file, param_block->user_data);
							}
							break;
							
						default:
							op_halt();
							break;
					}
				}
			}
		} else {
			/* We ran out of files.. */
		}
	}

	/* If we got a fnfErr, it was because we indexed too far. */	
	return (err==fnfErr) ? (kNMNoError) : err;
}

//----------------------------------------------------------------------------------------
// get_directories_parID
//----------------------------------------------------------------------------------------

static NMErr get_directories_parID(
	FSSpec *directory, 
	NMSInt32 *parID)
{
	CInfoPBRec pb;
	NMErr error;

	/* Clear it.  Always a good thing */
	machine_mem_zero(&pb,sizeof(pb));
	pb.dirInfo.ioNamePtr= directory->name;
	pb.dirInfo.ioVRefNum= directory->vRefNum;
	pb.dirInfo.ioDrDirID= directory->parID;
	pb.dirInfo.ioFDirIndex= 0; /* use ioNamePtr and ioDirID */
	error= PBGetCatInfoSync(&pb);
	
	*parID = pb.dirInfo.ioDrDirID;
	op_assert(!error && (pb.hFileInfo.ioFlAttrib & 0x10));

	return error;
}

//----------------------------------------------------------------------------------------
// get_search_parID
//----------------------------------------------------------------------------------------

static NMErr get_search_parID(
	FSSpec *directory, 
	NMSInt32 *parID)
{
	CInfoPBRec pb;
	NMErr error;

	/* Clear it.  Always a good thing */
	machine_mem_zero(&pb, sizeof(pb));
	pb.dirInfo.ioNamePtr= directory->name;
	pb.dirInfo.ioVRefNum= directory->vRefNum;
	pb.dirInfo.ioDrDirID= directory->parID;
	pb.dirInfo.ioFDirIndex= 0; /* use ioNamePtr and ioDirID */
	error= PBGetCatInfoSync(&pb);
	
	if(pb.hFileInfo.ioFlAttrib & 0x10)
	{
		*parID = pb.dirInfo.ioDrDirID;
	} else {
		*parID= directory->parID;
	}

	return error;
}

//----------------------------------------------------------------------------------------
// alphabetical_names
//----------------------------------------------------------------------------------------

static int alphabetical_names(
	const FileDesc *a, 
	const FileDesc *b)
{
  return (CompareString(a->name, b->name, NULL));
}
