/***********************************************************************
#
#		App.h
#
#		This file contains the constants and structure definitions.
#
#		Author: Michael Marinkovich
#				Apple Developer Technical Support
#
#
#		Modification History: 
#
#			6/4/95		MWM 	Initial coding					 
#			10/12/95	MWM		cleaned up
#			3/2/02		ECF		Carbon/OpenPlay port				 
#
#		Copyright © 1992-95 Apple Computer, Inc., All Rights Reserved
#
#
***********************************************************************/
#ifndef APP_H
#define APP_H

#if (!OP_PLATFORM_MAC_MACHO)
	#include <Controls.h>
	#include <QDOffscreen.h>
#endif

//---------------------------------------------------------------------
//
//	Macros
//
//---------------------------------------------------------------------

#define MIN(x,y)			( ((x)<(y)) ? (x) : (y) )
#define MAX(x,y)			( ((x)>(y)) ? (x) : (y) )
#define TopLeft( r )		( *(Point *) &(r).top )
#define BotRight( r )		( *(Point *) &(r).bottom )


//---------------------------------------------------------------------
//
//	General
//
//---------------------------------------------------------------------

#define kMinHeap			300 * 1024
#define kMinSpace			300 * 1024


//---------------------------------------------------------------------
//
//	Menus
//
//---------------------------------------------------------------------

#define rMBarID				128

enum {
 	mApple					= 128,
	iAbout					= 1,

	mFile					= 129,
	iHost					= 1,
	iJoin					= 2,
	iLeave					= 3,
	iQuit					= 5
};


//---------------------------------------------------------------------
//
//	Window stuff
//
//---------------------------------------------------------------------

// doc types
enum {
	kDocKind				= 94,
	kDialogKind				= 95,
	kFloatKind				= 96,
	kAboutKind				= 97
};	

#define kFudgeFactor		4		// fudge factor for boundary around window
#define kTitleBarHeight		18		// title bar height

// scroll values
#define kScrollWidth		15
#define kScrollDelta		16


//---------------------------------------------------------------------
//
//	General resource ID's
//
//---------------------------------------------------------------------

//#define rAboutPictID		3000 // about picture
#define rAboutAlert			128


//---------------------------------------------------------------------
//
//	Alert Error ID's
//
//---------------------------------------------------------------------

#define rErrorDlg			129	 // main error dialog


//---------------------------------------------------------------------
//
//	Custom Event Proc stuff
//
//---------------------------------------------------------------------

enum {
	kIdleProc			= 1,
	kMenuProc,
	kInContentProc,
	kInGoAwayProc,
	kInZoomProc,
	kInGrowProc,
	kMUpProc,
	kKeyProc,
	kActivateProc,
	kUpdateProc
};
	

//---------------------------------------------------------------------
//
//	Typedefs
//
//---------------------------------------------------------------------

// event handling proc
typedef void (*CustomProc)(WindowRef window, void *refCon);


// we just use the basic events for the callback procs

struct DocRec
{	
	CustomProc					idleProc;			// custom idle proc
	CustomProc					mMenuProc;			// custom menu proc
	CustomProc					inContentProc;		// custom content click Proc
	CustomProc					inGoAwayProc;		// custom inGoAway proc
	CustomProc					inZoomProc;			// custom inZoom proc
	CustomProc					inGrowProc;			// custom inGrow proc
	CustomProc					mUpProc;			// custom mouseUp proc
	CustomProc					keyProc;			// custom autoKey-keyDown proc
	CustomProc					activateProc;		// custom activate window proc
	CustomProc					updateProc;			// custom window update proc
	ControlHandle				hScroll;			// horz scroll bar
	ControlHandle				vScroll;			// vert scroll bar
	GWorldPtr					world;				// offscreen for pict imaging
	PicHandle					pict;				// windows picture
//	THPrint						printer;			// apps print record - inited at window int
	Boolean						dirty;				// document needs saving
};

typedef struct DocRec DocRec;
typedef DocRec *DocPtr, **DocHnd;



#endif














