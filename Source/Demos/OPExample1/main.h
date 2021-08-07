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

#ifdef __cplusplus
	extern "C"{
#endif

/* ----------------------------------------------------------- Includes */


/* ----------------------------------------------------------- Macro definitions */

/* If true/false are not defined, do so */

#ifndef true
	#define true 1
	#define false !true
#endif


/* ----------------------------------------------------------- Type definitions */

typedef enum{ kGameClient = 0, kGameHost = 1 } tGameType;	/* type of game to be played */

/* Questions are defined in the following structure */

typedef struct
{
	const char * question;	/* string with question and answers */
	char answer;			/* char matching appr. answer from above string */

}Question_t, *QuestionPtr;


/* ----------------------------------------------------------- Function Prototypes */

int GameIsHost( void );

char *GameP2CStr
(
	unsigned char *s,	/* source Pascal string */
	char *d				/* buffer to hold converted C string */
);

unsigned char *GameC2PStr
(
	char *s,			/* source C string */
	unsigned char *d	/* destination buffer for Pascal string */
);

int GameCheckAnswer
(
	char answer			/* answer to check against current question */
);

void GameDisplayQuestion
(
	char *question		/* question string to display to local console */
);

void GameOver( void );
void GameHandleEvents( void );
QuestionPtr GameGetNewQuestion( void );
QuestionPtr GameGetCurrentQuestion( void );

#ifdef __cplusplus
	}
#endif
