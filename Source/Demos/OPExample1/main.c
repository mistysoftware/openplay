/*
 * Copyright (c) 2002 Lane Roathe. All rights reserved.
 *	Developed in cooperation with Freeverse, Inc.
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

/* NOTES:

	- This example is designed so that all networking code is in a single file, meaning that
		there is no networking code in this file :) The purpose is to make it easier for a programmer
		to study the basic use of OpenPlay's NetSprocket API and not to demonstrate good programming
		style or OOP practices.

	- There is only rudimentary game play here; there is no scoring, and there are lots of conditions
		that confuse the "game", esp. with players leaving and entering at will. However, it does enough
		to actually have some fun playing the game, and watching it work in an "open" connect model.

	- WINDOWS: at this time the game will not work on windows because the input mechanism for answering
		(and quitting) is not implemented. If someone can provide that code, it would be great!

*/

/* ----------------------------------------------------------- Includes */

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "network.h"
#include "main.h"

#if __MWERKS__
	#if (OP_PLATFORM_MAC_CFM)
		#define __USE_SIOUX__ 1		/* define to non-zero to use MetroWerks standard output library */
	#endif
#endif

#if __USE_SIOUX__
		#include <sioux.h>
		#include <siouxGlobals.h>
#endif

/* ----------------------------------------------------------- Macro definitions */

/* default values, can be overriden by user */

#define kGamePort		25710
#define kLocalHostIP	"127.0.0.1"
extern char 			*GameNameStr;

/* ----------------------------------------------------------- Type definitions */

/* ----------------------------------------------------------- Local Variables */

/* Define our questions */

static Question_t _questions[] =
{
	{"What color are golden eagles?\n\na) white\nb) brown\nc) silver", 'b'},
	{"Where do bears fish?\n\na) streams\nb) rivers\nc) lakes\nd) aquariums\ne) anywhere they wish", 'e'},
	{"OpenPlay is...\n\na) Apple's cross-platform network abstraction library\nb) a wrestling event\nc) an earthquake monitoring service", 'a'},
	{"The Creative Nomad is a...\n\na) ingenious sand dweller\nb) 1st person shooter\nc) portable MP3 player\nd) none of the above", 'c'},
	{"Equilibrium makes which product?\n\na) Debabilizer\nb) Theatre of war\nc) Colin's Classic Cards", 'a'},
	{"Wolves howl to...\n\na) communicate\nb) because they want to\nc) find a mate\nd) all the above", 'd'},
	{"MacTank provides what service?\n\na) privacy\nb) security\nc) beer reviews\nd) tech support\ne) none of the above", 'd'},
	{"The S2000 is what?\n\na) a sports car\nb) a fireware drive\nc) a stereo reciever\nd) an electoscope", 'a'},
	{"Wingnuts is what?\n\na) specialty bolt nuts\nb) an arcade game\nc) a dance troupe\nd) none of the above", 'b'},
	{"NetSprocket is...\n\na) A high-level interface in Apple's OpenPlay library\nb) a fishing apparatus\nc) a weapon in quake 3", 'a'},
	{"When somebody asks you if you're a god you say:\n\na) YES!\nb) no\nc) maybe..\nd) what do you mean - African or European?", 'a'},
	{"How many people were going to St. Ives?\n\na) 14\nb) 49\nc) 2401\nd) 1\ne) 0", 'd'},
	{"What do you do if you're on fire?\n\na) stop, drop, and roll\nb) shake, rattle, and roll\nc) take a photo\nd) slam-dunk from the three-point line", 'd'},
	{"The meaning of life is?\n\na) 42\nb) 65536\nc) 0\nd) -3", 'a'},
	{ NULL, 0 }
};

/* Some local variables, we have accessor functions where needed */

static int _questionCount;	/* Number of questions we find available */
static int _gameOver;		/* true when game is over */
static int _gameStarted;	/* true once game (ie, server) has started */

static int _server;			/* true if server/host, false if player/client */

static int _numPlayers;		/* number of players for this question */
static int _numAnswers;		/* current # of answers back on a question */
static int _newQuestion;	/* true if a new question should be asked */

static QuestionPtr _theQuestion;	/* ptr to current question record */

static NMUInt32 _askTime;	/* time that last question was asked */

static int _sendAnswer;		/* true if server wants an answer form the player (only one answer per question!) */

static tGameType _gameType;	/* type of game (client/server) */

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
#if __MWERKS__
	#pragma mark -
	#pragma mark È Local Routines
	#pragma mark -
#endif


/* --------------------------------------------------------------------------------
	Get the game ready to play

	 EXIT:	none
*/

static NMErr _startServer( void )
{
	int v;
	unsigned char gameNameStr[32], playerNameStr[32];
	char gameName[32], playerName[32];
	NMUInt32 port;
	int players;

	/* Get port # to use, defaulting if no entry or bad value */

	printf( "Listen for clients on port? (0 == %d): ", kGamePort );
	fflush(stdout);
	scanf( "%d", &v );
	port = (NMUInt32)v;
	if( port < 1000 || port > 32767 )
	{
		printf( "   <using default port>\n" );
		fflush(stdout);
		port = kGamePort;
	}

	/* Get max # of players to allow on server */

	printf( "Maximum number of players to allow? (0 == 16): " );
	fflush(stdout);
	scanf( "%d", &v );
	players = (NMUInt32)v;
	if( players < 1 || players > 32 )
	{
		players = 16;
		printf( "   <using default max players>\n");
		fflush(stdout);
	}

	/* Get the name of the server to return to connecting clients */

	printf( "What is the name of your server? (0 == '%s'): ", GameNameStr );
	fflush(stdout);
	scanf( "%31s", gameName );
	if( !gameName[0] || ('0' == gameName[0] && !gameName[1]) )
	{
		printf( "   <using default game name>\n" );
		fflush(stdout);
		strcpy( gameName, GameNameStr );
	}

	/* Get the name this player wishes to go by */

	printf( "Enter your player name or alias: " );
	fflush(stdout);
	scanf( "%s", playerName );

	/* convert names from C to Pascal */

	GameC2PStr( gameName, gameNameStr );
	GameC2PStr( playerName, playerNameStr );

	_gameType = kGameHost;

	/* finally, we're ready to start the server! */

	return( NetworkStartServer( (NMUInt16)port, players, gameNameStr, playerNameStr ) );
}

/* --------------------------------------------------------------------------------
	Get the game ready to play

	 EXIT:	none
*/

static NMErr _startClient( void )
{
	int v;
	unsigned char playerNameStr[32];
	char theIPAddress[256];
	char thePlayer[256];
	char thePort[32];
	NMUInt32 port;

	/* Get IP # to look for the server on */

	printf( "Enter domain name or IP # for host (0 == %s): ", kLocalHostIP );
	fflush(stdout);
	scanf( "%s", theIPAddress );
	if( !theIPAddress[0] || ('0' == theIPAddress[0] && !theIPAddress[1]) )
	{
		printf( "   <using localhost IP>\n" );
		fflush(stdout);
		strcpy( theIPAddress, kLocalHostIP );
	}

	/* Get port # to use, defaulting if no entry or bad value */

	printf( "Talk to server on which port? (0 == %d): ", kGamePort );
	fflush(stdout);
	scanf( "%d", &v );
	port = (NMUInt32)v;
	if( port < 1000 || port > 32767 )
	{
		printf( "   <using default port>\n" );
		fflush(stdout);
		port = kGamePort;
	}
	sprintf( thePort, "%li", port );

	/* Get the name this player wishes to go by */

	printf( "Enter your player name or alias: " );
	fflush(stdout);
	scanf( "%s", thePlayer );

	GameC2PStr( thePlayer, playerNameStr );

	_gameType = kGameClient;

	/* finally, we're ready to start the client! */

	return( NetworkStartClient( theIPAddress, thePort, playerNameStr ) );
}

/* --------------------------------------------------------------------------------
	Get the game ready to play

	 EXIT:	none
*/

static void _gameStartup( void )
{
	/* Tell user what they have run */

	printf( "\n=== %s ===\n\n", GameNameStr );
	fflush(stdout);

	/* if using SIOUX, set some of it's preferences */

#if __USE_SIOUX__
	SIOUXSettings.asktosaveonclose = false;
	SIOUXSettings.autocloseonquit = false;
	SIOUXSettings.standalone = true;
#endif

	/* init the random # routine */
	{
		time_t theTime;
		time(&theTime);
		srand( (unsigned int)theTime );
	}

	/* Determine number of questions we have available. */

	_questionCount = 0;

	while( _questions[_questionCount].question != NULL )
		_questionCount++;

	/* set new game state */

	_gameOver = false;
	_sendAnswer = false;
	_newQuestion = false;
}

/* --------------------------------------------------------------------------------
	Release any game resources

	 EXIT:	none
*/

static void _gameShutdown( void )
{
	/* init game state */
	
	_gameOver = true;

	/* Tell user the game is over */

	printf( "--- GAME OVER ---" );
	fflush(stdout);
}


/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
#if __MWERKS__
	#pragma mark -
	#pragma mark È Global Routines
	#pragma mark -
#endif


/* ================================================================================
	Set game over state to true (ie, end game)

	 EXIT:	none
*/

void GameOver( void )
{
	_gameOver = true;
	_gameStarted = false;
}

/* ================================================================================
	Return whether or not the game is the host (server)

	 EXIT:	true == host (kGameHost), false = client or observer, etc.
*/

int GameIsHost( void )
{
	return( kGameHost == _gameType );
}

/* ================================================================================
	Check to see if the answer is correct

	NOTE: should only be called on the server!!!

	EXIT: true if player answered correctly, false otherwise
*/

int GameCheckAnswer
(
	char answer			/* answer to check against current question */
)
{
	if( answer == _theQuestion->answer )
	{
		_newQuestion = true;	/* player got it right, time for next question! */
		return( true );
	}

	/* wrong answer, if all players answered incorrectly, ask a new question */

	_numAnswers++;
	if( _numAnswers >= _numPlayers )
		_newQuestion = true;

	return( false );
}

/* ================================================================================
	See if it is time to ask a new question (time expired or previous question answered)

	NOTE: this is a server only function!

	EXIT: ptr to new question, or NULL if not time
*/

QuestionPtr GameGetNewQuestion( void )
{
	time_t seconds;
	time(&seconds);

	/* only valid to check on running games on a server */

	if( _gameStarted && _server )
	{
		/* first, see if we have timed out and forcibly ask a new question */

		if( !_newQuestion )
		{
			NMUInt32 now;

			now = seconds;

			if( now - _askTime > 60 )	/* time for a new question every X seconds */
				_newQuestion = true;
		}

		/* if it's time to get a new question, do so */

		if( _newQuestion )
		{
			_theQuestion = &_questions[rand() % _questionCount];
			_newQuestion = false;

			_askTime = seconds;		/* reset the time-out clock */

			_numPlayers = (int)NetworkGetPlayerCount();			/* we get this count here to avoid long waits when new players enter! */
			_numAnswers = 0;

			return( _theQuestion );
		}
	}
	return( NULL );
}

/* ================================================================================
	Simply returns the current question - if one has not been asked yet, we do so

	NOTE: this is a server only function!

	EXIT: ptr to current question
*/

QuestionPtr GameGetCurrentQuestion( void )
{
	if( !_theQuestion )
		_theQuestion = GameGetNewQuestion();

	return _theQuestion;
}

/* ================================================================================
	Display the question we got from the server

	 EXIT:	none
*/

void GameDisplayQuestion
(
	char *question		/* question string to display to local console */
)
{
	printf( "==========================================\nQUESTION: %s\n\nAnswer:\n\n", question );
	fflush( stdout );

	_sendAnswer = true;		/* once we display a new question, we are set to send an answer! */
}

/* ================================================================================
	Allow the system some time, and get update events, etc.
	Also, checks for the 'Quit' flag (q key) and sets the game to over if found

	 EXIT:	none
*/

void GameHandleEvents( void )
{
	QuestionPtr qp;
	char key = 0;

	/* first check for any network messages we need to handle */
	
	NetworkHandleMessage();

	/* now get and handle the system/application event messages */
	
#if OP_PLATFORM_WINDOWS
	/*{
		MSG msg;

		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if( WM_CHAR == msg.message )
				key = msg.wParam & 0xFF;

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}*/
#elif OP_PLATFORM_MAC_CFM
	{
		EventRecord event;

		if( WaitNextEvent( everyEvent, &event, 1, NULL ) )
		{
			if( keyDown == event.what )
				key = (char)(event.message & charCodeMask);

			#if (__USE_SIOUX__)
				SIOUXHandleOneEvent( &event );
			#endif
		}
	}
#endif

	/* check for special key presses */

	if( 'q' == key
	
	/* see if sioux has been quit so we can exit gracefully
	alas, this is only available on mac */
	#if (__USE_SIOUX__)
		#if (OP_PLATFORM_MAC_CFM)
			|| SIOUXQuitting
		#endif
	#endif
	)
			{
				GameOver();
				key = 0;
			}
	
	/* Send any non-special keys to the server */

	if( _sendAnswer && key )
	{
		NetworkSendAnswer( key );	/* once player answers the question, they can't do so again! */
		_sendAnswer = false;
	}

	/* See if it's time to send a new question to the players */

	qp = GameGetNewQuestion();
	if( qp )
		NetworkSendQuestion( qp->question );

}

/*============================================================================
	Convert a string from Pascal to C format -- NOTE: dest buffer ASSUMED large enough!

	EXIT:	pointer to resulting text
*/
char *GameP2CStr
(
	unsigned char *s,	/* source Pascal string */
	char *d				/* buffer to hold converted C string */
)
{
	if( s && d )
	{
		memcpy( d, s+1, *s );	/* pretty simple */
		d[*s] = 0;
	}
	return( d );
}

/* ================================================================================
	Convert a string from C to Pascal -- NOTE: dest buffer ASSUMED large enough!

	EXIT:	ptr to converted string
*/

unsigned char *GameC2PStr
(
	char *s,			/* source C string */
	unsigned char *d	/* destination buffer for Pascal string */
)
{
	int c = 0;

	if( s && d )	/* make sure we got got inputs */
	{
		while( *s )
			d[++c] = (unsigned char)*s++;	/* no blockmove, copy & get length in one shot */
	}
	*d = (unsigned char)c;

	return( d );
}

/* ================================================================================
	Entry point to the game

	 EXIT:	0 == no error, else error code to system
*/

int main
(
	int argc,			/* number of arguments */
	char *argv[]		/* argument list */
)
{
#pragma unused(argc,argv)

	char c;
	NMErr err = kNMNoError;

	/* Startup the game, allocating any needed resources, etc. */

	_gameStartup();

	/* Warn them if we don't fully work on this platform */
	#if (!__USE_SIOUX__)
		printf("\nNOTE: non-blocking input hasn't been added to this version of the app.");
		printf("\nYou won't be able to submit answers for this reason. (you can still host or join though)\n\n");
		fflush(stdout);
	#endif
	
	/* Startup networking, exiting on an error (startup routine reports error to user */

	err = NetworkStartup();
	if( !err )
	{
		/* Now, ask user if they want to run a server or a client */

		printf( "Do you want to run a server or a client? (s/c): " );
		fflush(stdout);
		c = (char)tolower( getchar() );

		if( 's' == c )
		{
			err = _startServer();		/* initialize server */
			if( !err )
			{
				printf( "\n\n[Server Started -- press 'q' to quit serving]\n\n" );
				fflush(stdout);

				_server = true;
			}
			else
			{
				printf( "!!! ERROR !!!  ===> Server startup failed with error %d\n", (int)err );
				fflush(stdout);
			}
		}
		else
		{
			err = _startClient();		/* initialize client */
			if( !err )
			{
				printf( "waiting for next question...press 'q' to quit the game\n\n" );
				fflush(stdout);

				_server = false;
			}
			else
			{
				printf( "!!! ERROR !!!  ===> Client startup failed with error %d\n", (int)err );
				fflush(stdout);
			}
		}

		/* Now, if we didn't get an error, run the game until the user quits */

		if( !err )
		{
			_gameStarted = true;

			do
			{
				GameHandleEvents();

			}while( !_gameOver );
		}

		/* end game, release any resources, etc. */

		NetworkShutdown();
	}
	else
	{
		printf( "!!! ERROR !!!  ===> Networking startup failed with error %d\n", (int)err );
		fflush(stdout);
	}

	_gameShutdown();

	/* exit nicely */

	return( (int)err );
}
