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


/*
	File:		LinkedList.h

	Contains:

*/

#ifndef __LINKEDLIST__
#define __LINKEDLIST__


#include <stddef.h>
#include "OPUtils.h"

#if defined(OP_PLATFORM_UNIX) || defined(OP_PLATFORM_MAC_MACHO)
#include <pthread.h>
class OSCriticalSection
{
	public:
		OSCriticalSection() {pthread_mutex_init(&theMutex,NULL);}
		~OSCriticalSection() {pthread_mutex_destroy(&theMutex);}
		NMBoolean	acquire()	{	int error = pthread_mutex_trylock(&theMutex);
									if (error) return false; else return true;}
		void	release()	{	pthread_mutex_unlock(&theMutex);}
		private:
		pthread_mutex_t theMutex;
};
#elif (OP_PLATFORM_MAC_CFM)
	class OSCriticalSection
	{
		public:
			OSCriticalSection() {OTClearLock(&theLock);}
			~OSCriticalSection() {}
			NMBoolean	acquire() {	return OTAcquireLock(&theLock);}
			void	release() 	{OTClearLock(&theLock);}
		private:
			OTLock theLock;
	};
#elif (OP_PLATFORM_WINDOWS)
	// ECF 010928 win32 syncronization routines for safe critical sections.  We can't simply use TryEnterCriticalSection()
	// to return a true/false value because it's not available in win95 and 98
	class OSCriticalSection
	{
		public:
			OSCriticalSection() {	locked = false; InitializeCriticalSection(&theCriticalSection);}
			~OSCriticalSection() {	DeleteCriticalSection(&theCriticalSection);}
			NMBoolean	acquire()	{	if (locked == true) return false; //dont have to wait in this case	
									EnterCriticalSection(&theCriticalSection);
									if (locked) {LeaveCriticalSection(&theCriticalSection); return false;}
									else {locked = true; LeaveCriticalSection(&theCriticalSection); return true;}}
			void	release()	{	EnterCriticalSection(&theCriticalSection);
									locked = false; LeaveCriticalSection(&theCriticalSection);}
		private:
			NMBoolean locked;
			CRITICAL_SECTION theCriticalSection;
	};
#else
	#error "Linked list OSCriticalSection undefined"
#endif

/*	-------------------------------------------------------------------------
	** NMLIFO
	**
	** These are functions to implement a LIFO list that is interrupt-safe.
	** The only function which is not is NMReverseList.  Normally, you create
	** a LIFO list, populate it at interrupt time, and then use NMLIFOStealList
	** to atomically remove the list, and NMReverseList to flip the list so that
	** it is a FIFO list, which tends to be more useful.
	------------------------------------------------------------------------- */

	class NMLink;
	class NMLIFO;

	class NMLink
	{
		public:
			NMLink*	fNext;
			void	Init()
						{ fNext = NULL; }
	};

	class NMLIFO
	{
		public:
			OSCriticalSection theLock;
			NMLink*	fHead;
		
			void	Init()	
					{ fHead = NULL; }
		
			void	Enqueue(NMLink* link)
							{ 
								while (true) {if (theLock.acquire()) break;}
								link->fNext = fHead;
								fHead = link;	
								theLock.release();	
							}
					

			NMLink*	Dequeue()
							{
								while (true) {if (theLock.acquire()) break;}						
								NMLink *origHead = fHead;
								if (fHead) /* check for empty list */
									fHead = fHead->fNext;
								theLock.release();									
								return origHead;
							}

						
			NMLink*	StealList()
							{	
								while (true) {if (theLock.acquire()) break;}
								NMLink *origHead = fHead;
								fHead = NULL;
								theLock.release();
								return origHead;
							}
						
						
			NMBoolean	IsEmpty()
							{
								return fHead == NULL;
							}
	};

/*	-------------------------------------------------------------------------
	** NMList
	**
	** An NMList is a non-interrupt-safe list, but has more features than the
	** NMLIFO list. It is a standard singly-linked list.
	------------------------------------------------------------------------- */

	typedef struct NMList	NMList;

		typedef NMBoolean (*NMListSearchProcPtr)(const void* ref, NMLink* linkToCheck);
		//
		// Remove the last link from the list
		//
	extern NMLink*		NMRemoveLast(NMList* pList);
		//
		// Return the first link from the list
		//
	extern NMLink*		NMGetFirst(NMList* pList);
		//
		// Return the last link from the list
		//
	extern NMLink*		NMGetLast(NMList* pList);
		//
		// Return true if the link is present in the list
		//
	extern NMBoolean		NMIsInList(NMList* pList, NMLink* link);
		//
		// Find a link in the list which matches the search criteria
		// established by the search proc and the refPtr.  This is done
		// by calling the search proc, passing it the refPtr and each
		// link in the list, until the search proc returns true.
		// NULL is returned if the search proc never returned true.
		//
	extern NMLink*		NMFindLink(NMList* pList, NMListSearchProcPtr proc, const void* refPtr);
		//
		// Remove the specified link from the list, returning true if it was found
		//
	extern NMBoolean		NMRemoveLink(NMList*, NMLink*);
		//
		// Similar to NMFindLink, but it also removes it from the list.
		//
	extern NMLink*		NMFindAndRemoveLink(NMList* pList, NMListSearchProcPtr proc, const void* refPtr);
		//
		// Return the "index"th link in the list
		//
	extern NMLink*		NMGetIndexedLink(NMList* pList, size_t index);

	struct NMList
	{
		NMLink*		fHead;
		
		void		Init()	
						{ fHead = NULL; }
		
		NMBoolean		IsEmpty()
						{ return fHead == NULL; }
						
		void		AddFirst(NMLink* link)
							{ 
								link->fNext = fHead;
								fHead = link;			
							}

		void		AddLast(NMLink* link)
							{
								link->fNext = NULL;

								if( !fHead )
								{
									fHead = link;
								}
								else
								{
									NMLink *current = fHead;
									
									while( current->fNext )
										current = current->fNext;
										
									current->fNext = link;
								}
							}

		
		NMLink*		GetFirst()
						{ return NMGetFirst(this); }
		
		NMLink*		GetLast()
						{ return NMGetLast(this); }
		
		NMLink*		RemoveFirst()
							{
								NMLink *origHead = fHead;
								fHead = fHead->fNext;
								return origHead;
							}
								
		NMLink*		RemoveLast()
						{ return NMRemoveLast(this); }
						
		NMBoolean		IsInList(NMLink* link)
						{ return NMIsInList(this, link); }
						
		NMLink*		FindLink(NMListSearchProcPtr proc, const void* ref)
						{ return NMFindLink(this, proc, ref); }
						
		NMBoolean		RemoveLink(NMLink* link)
						{ return NMRemoveLink(this, link); }
						
		NMLink*		RemoveLink(NMListSearchProcPtr proc, const void* ref)
						{ return NMFindAndRemoveLink(this, proc, ref); }
						
		NMLink*		GetIndexedLink(size_t index)
						{ return NMGetIndexedLink(this, index); }
	};
	
	
//ecf 020619 changed to a non-recursive method so large lists won't cause problems.

static NMLink* NMReverseList(NMLink *headRef)
{

	NMLink *oldNext;
	NMLink *newNext = NULL;

	if( !headRef )
		return( NULL );

	oldNext = headRef->fNext;
	
	while (oldNext)
	{
		headRef->fNext = newNext;
		newNext = headRef;
		headRef = oldNext;
		oldNext = headRef->fNext;
	}
	headRef->fNext = newNext;

	return headRef;
}

	#define NMGetLinkObject(link, struc, field)	\
		((struc*)((char*)(link) - offsetof(struc, field)))

#endif	/* __LINKEDLIST__ */

