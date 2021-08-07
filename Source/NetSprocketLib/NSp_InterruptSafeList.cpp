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
 */


// ===========================================================================
//	NSp_InterruptSafeList.cp
//
//  Uses C++ STL (Standard Template Library). In Metrowerks Codewarrior,
//  the STL is reentrant for atomic operations in a container class. This
//  may NOT be true for other vendors' STL implementations.
//
// ===========================================================================

#include "OPUtils.h"
#include "NSp_InterruptSafeList.h"


//----------------------------------------------------------------------------------------
// NSp_InterruptSafeList::NSp_InterruptSafeList 
//----------------------------------------------------------------------------------------
//	Constructor

NSp_InterruptSafeList::NSp_InterruptSafeList()
{
	mQueue = new list<NSp_InterruptSafeListMember*>;
//	mIteratorQueue = new list<NSp_InterruptSafeListIterator*>;
}


//----------------------------------------------------------------------------------------
// NSp_InterruptSafeList::~NSp_InterruptSafeList 
//----------------------------------------------------------------------------------------
//	Destructor

NSp_InterruptSafeList::~NSp_InterruptSafeList()
{
	// Memory leak in here?
	
//	list< NSp_InterruptSafeListIterator* >::iterator  itr;
//	
//	if ( mIteratorQueue != NULL ) {
//	
//	 	itr = mIteratorQueue->begin();
//
//		for ( ; itr != mIteratorQueue->end(); itr++ ) {
//		
//			( *itr )->ListDied();
//		}
//	}

	delete mQueue;
//	delete mIteratorQueue;
}

//----------------------------------------------------------------------------------------
// NSp_InterruptSafeList::Append 
//----------------------------------------------------------------------------------------
//	Call to add an item to the end of a list. Note that there is currently no
//	way to insert an item at any other position in a list.
//
//	May be called at interrupt time.

void
NSp_InterruptSafeList::Append(
	NSp_InterruptSafeListMember* inItem)
{
	if ( inItem->mParentList != NULL )
		return;				// not allowed

	inItem->mParentList = this;
	
	mQueue->push_back( inItem );
}


//----------------------------------------------------------------------------------------
// NSp_InterruptSafeList::Remove 
//----------------------------------------------------------------------------------------
//	Call to remove an item from a list. 
//
//	May be called at interrupt time.

NMBoolean
NSp_InterruptSafeList::Remove(	NSp_InterruptSafeListMember* inItem)
{
	NSp_InterruptSafeList* parentList = inItem->mParentList;

	if (parentList != this)
		return false;				// not allowed

	mQueue->remove( inItem );
	
	inItem->mParentList = NULL;

	return true;
}


//----------------------------------------------------------------------------------------
// NSp_InterruptSafeList::IsEmpty 
//----------------------------------------------------------------------------------------
//	Returns true if there are no entries in this list.

NMBoolean
NSp_InterruptSafeList::IsEmpty() const
{
	if ( mQueue != NULL ) {

		return( mQueue->empty() );
	}
	
	return( false );
}


// ===========================================================================


//----------------------------------------------------------------------------------------
// NSp_InterruptSafeListIterator::NSp_InterruptSafeListIterator 
//----------------------------------------------------------------------------------------
//	Constructor

NSp_InterruptSafeListIterator::NSp_InterruptSafeListIterator( 
		NSp_InterruptSafeList& inList ) : mList( inList )
{
//	mList = inList;  // Correct?
	mListDied = false;
	
	Reset();
		
//	mList.mIteratorQueue->push_back( this );
}


//----------------------------------------------------------------------------------------
// NSp_InterruptSafeListIterator::~NSp_InterruptSafeListIterator 
//----------------------------------------------------------------------------------------
//	Destructor

NSp_InterruptSafeListIterator::~NSp_InterruptSafeListIterator()
{
//	if ( !mListDied )
//	
//		mList.mIteratorQueue->remove( this );
}


//----------------------------------------------------------------------------------------
// NSp_InterruptSafeListIterator::Reset 
//----------------------------------------------------------------------------------------
//	Returns the iterator to the beginning of the list.
//	May be called at interrupt time.

void
NSp_InterruptSafeListIterator::Reset( void )
{
	if ( !mListDied ) {

		mListIterator = mList.mQueue->begin();
	}
}


//----------------------------------------------------------------------------------------
// NSp_InterruptSafeListIterator::Current 
//----------------------------------------------------------------------------------------
//	Sets outItem to the address of the current item. Returns true if the item
//	pointer is valid.
//
//	May be called at interrupt time.

NMBoolean
NSp_InterruptSafeListIterator::Current(
										NSp_InterruptSafeListMember** outItem)
{
	NMBoolean status = false;
	
	if ( !mListDied && mList.mQueue->size() > 0 ) {
	
		*outItem = *mListIterator;
		status = true;
	}
	
	return( status );
}

//----------------------------------------------------------------------------------------
// NSp_InterruptSafeListIterator::Next 
//----------------------------------------------------------------------------------------
//	Sets outItem to the address of the next item in this list. Returns true if the
//	next item pointer is valid, false if the end of the list had been reached.
//
//	May be called at interrupt time.

NMBoolean
NSp_InterruptSafeListIterator::Next(
	NSp_InterruptSafeListMember** outItem)
{
	if ( mListDied || mListIterator == mList.mQueue->end() ) {
	
		return( false );
	}
	
	*outItem = *mListIterator;
	mListIterator++;

	return ( true );
}


//----------------------------------------------------------------------------------------
//	NSp_InterruptSafeListIterator::ListDied				[private]
//----------------------------------------------------------------------------------------
//	Should be called only by NSp_InterruptSafeList when the list is deleted.

void
NSp_InterruptSafeListIterator::ListDied()
{
	mListDied = true;
}


// ===========================================================================



// ---------------------------------------------------------------------------
//	NSp_InterruptSafeListMember
// ---------------------------------------------------------------------------
//	Constructor

NSp_InterruptSafeListMember::NSp_InterruptSafeListMember()
{
	mParentList = NULL;
}


// ---------------------------------------------------------------------------
//	~NSp_InterruptSafeListMember
// ---------------------------------------------------------------------------
//	Destructor

NSp_InterruptSafeListMember::~NSp_InterruptSafeListMember()
{
	if ( mParentList )
		mParentList->Remove( this );
}

// EOF
