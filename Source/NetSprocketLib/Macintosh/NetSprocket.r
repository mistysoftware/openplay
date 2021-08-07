#include <MacTypes.r>
#include <Dialogs.r>


resource 'DITL' (2500, "Game Setup", purgeable)
{
	{
	/* ok */
		{ 260, 310, 280, 375 },
		Button
		{
			enabled,
			"OK"
		},
	/* cancel */
		{ 260, 210, 280, 275 },
		Button
		{
			enabled,
			"Cancel"
		},
	/* kIPJoinInfoLabel */
		{ 50, 10, 70, 365 },
		StaticText
		{
			disabled,
			"Enter host name and port of the game you wish to join:"
		},
		
	/* kIPHostInfoLabel */
		{ 50, 10, 70, 365 },
		StaticText
		{
			disabled,
			"Enter host name and port for the game you wish to host:"
		},
		
		
		
		
	/* kIPHostLabel */
		{ 85, 10, 105, 90 },
		StaticText
		{
			disabled,
			"Host Name:"
		},
	/* kIPHostText */
		{ 85, 100, 105, 360 },
		EditText
		{
			enabled,
			"127.0.0.1"
		},
		
		
	/* kIPPortLabel */
		{ 115, 10, 135, 90 },
		StaticText
		{
			disabled,
			"Host Port:"
		},
	/* kIPPortText */
		{ 115, 100, 135, 170 },
		EditText
		{
			enabled,
			"8762"
		},
		
		
		

	/* kATJoinInfoLabel */
		{ 50, 10, 70, 390 },
		StaticText
		{
			disabled,
			"Enter name of the game you wish to join:"
		},	
		
		
	/* kATHostInfoLabel */
		{ 50, 10, 70, 390 },
		StaticText
		{
			disabled,
			"Enter name of the game you wish to host:"
		},	
		
		
	/* kATGameNameLabel */
		{ 85, 10, 105, 90 },
		StaticText
		{
			disabled,
			"Game Name:"
		},
	/* kATGameNameText */
		{ 85, 100, 105, 390 },
		EditText
		{
			enabled,
			"House of Clarus"
		},		
		
		

	/* kPlayerNameLabel */
		{ 170, 10, 190, 95 },
		StaticText
		{
			disabled,
			"Player Name:"
		},
	/* kPlayerNameText */
		{ 170, 100, 190, 320 },
		EditText
		{
			enabled,
			"Sir Fleem"
		},		
		
		
		
	/* kPasswordLabel */
		{ 200, 10, 220, 90 },
		StaticText
		{
			disabled,
			"Password:"
		},
	/* kPasswordText */
		{ 200, 100, 220, 200 },
		EditText
		{
			enabled,
			"Meatloaf"
		},
		
		
	/* kProtocolLabel */
		{ 15, 10, 35, 90 },
		StaticText
		{
			disabled,
			"Protocol:"
		},

	/* kIPProtocolButton */
		{ 13, 90, 33, 160 },
		RadioButton
		{
			enabled,
			"TCP/IP"
		},

	/* kAppleTalkProtocolButton */
		{ 13, 170, 33, 260 },
		RadioButton
		{
			enabled,
			"AppleTalk"
		}
	}
};


resource 'DLOG' (2500, purgeable)
{
	{ 200, 150, 500, 560 },
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	2500,
	"Game Setup",
	centerParentWindow
};


resource 'vers' (1) {
		0x02, 
		0x02, 
		release, 
		0x1,
		verUS,
		"2.2r2",
		"2.2r2, © Copyright 1999-2004 Apple Computer"
	};


resource 'vers' (2) {
		0x02, 
		0x02, 
		release, 
		0x1,
		verUS,
		"2.2r2",
		"OpenPlay, Apple Open Source"
	};
