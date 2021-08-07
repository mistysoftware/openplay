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
//	NSp_InterruptSafeList.h	
//
//  Uses C++ STL container class at its core.
//
//  WARNING:  Metrowerks' STL implementation is thread-safe for atomic 
//  operations, other IDE vendors' implementations MAY NOT BE! If porting
//  to a new IDE, be sure to investigate the issue.	
//
// ===========================================================================



#ifndef NSp_InterruptSafeList_H
#define NSp_InterruptSafeList_H


	#undef min	// Name collision otherwise....
	#undef max

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif

	// C++ STL Headers...
	#include <list>

	using namespace std;



	class NSp_InterruptSafeListMember;
	class NSp_InterruptSafeListIterator;

// ===========================================================================
//	NSp_InterruptSafeList
// ===========================================================================
//	Wrapper for an STL list.

	class NSp_InterruptSafeList {

	public:
								NSp_InterruptSafeList();
		virtual					~NSp_InterruptSafeList();

		virtual void			Append( NSp_InterruptSafeListMember* inItem );
		virtual NMBoolean		Remove( NSp_InterruptSafeListMember* inItem );
		virtual NMBoolean		IsEmpty( void ) const;

	private:
		list< NSp_InterruptSafeListMember* >     *mQueue;
//		list< NSp_InterruptSafeListIterator* >   *mIteratorQueue;

								NSp_InterruptSafeList( NSp_InterruptSafeList& );

		friend class NSp_InterruptSafeListIterator;

	};

// ===========================================================================
//	NSp_InterruptSafeListMember
// ===========================================================================
//	Base class for items which can be added to NSp_InterruptSafeList.

	class NSp_InterruptSafeListMember {

	public:
								NSp_InterruptSafeListMember();
								~NSp_InterruptSafeListMember();

	private:
		NSp_InterruptSafeList*		mParentList;

		friend class NSp_InterruptSafeList;
		friend class NSp_InterruptSafeListIterator;

	};

// ===========================================================================
//	NSp_InterruptSafeListIterator
// ===========================================================================
//	An iterator for NSp_InterruptSafeList. But that was probably obvious... ;-)

	class NSp_InterruptSafeListIterator {

	public:
								NSp_InterruptSafeListIterator( NSp_InterruptSafeList& inList );
								~NSp_InterruptSafeListIterator();

		void					Reset();

		NMBoolean				Current( NSp_InterruptSafeListMember** outItem );
		NMBoolean				Next( NSp_InterruptSafeListMember** outItem );
		
	private:
		void					ListDied();	

		list<NSp_InterruptSafeListMember*>::iterator  mListIterator;
		NSp_InterruptSafeList&	mList;
		NMBoolean					mListDied;

		NSp_InterruptSafeListMember* mCurrentEntry;

								NSp_InterruptSafeListIterator( void );	// do not use
								NSp_InterruptSafeListIterator( NSp_InterruptSafeListIterator& );

		friend class NSp_InterruptSafeList;

	};
	
#endif // NSp_InterruptSafeList_H

// EOF
