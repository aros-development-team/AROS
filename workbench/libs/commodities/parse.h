#ifndef COMMODITIES_PARSE_H
#define COMMODITIES_PARSE_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/****************************************************************************/

typedef struct paType
{
    STRPTR name;
    UWORD  value;
} pix_S;


pix_S pix_Class[]=
    {
	{"DISKINSERTED",
	IECLASS_DISKINSERTED},

	{"DISKREMOVED",
	IECLASS_DISKREMOVED},

	{"EVENT",
	IECLASS_EVENT},

	{"NEWPREFS",
	IECLASS_NEWPREFS},

	{"POINTERPOS",
	IECLASS_POINTERPOS},

	{"RAWKEY",
	IECLASS_RAWKEY},

	{"RAWMOUSE",
	IECLASS_RAWMOUSE},

	{"TIMER",
	IECLASS_TIMER},

	{NULL,
	0}
    };

pix_S pix_IEvent[]=
    {
	{"CAPSLOCK",
	IEQUALIFIER_CAPSLOCK},

	{"CAPS_LOCK",
	IEQUALIFIER_CAPSLOCK},

	{"CONTROL",
	IEQUALIFIER_CONTROL},

	{"CTRL",
	IEQUALIFIER_CONTROL},

	{"LALT",
	IEQUALIFIER_LALT},

	{"LAMIGA",
	IEQUALIFIER_LCOMMAND},

	{"LBUTTON",
	IEQUALIFIER_LEFTBUTTON},

	{"LCOMMAND",
	IEQUALIFIER_LCOMMAND},

	{"LEFTBUTTON",
	IEQUALIFIER_LEFTBUTTON},

	{"LEFT_ALT",
	IEQUALIFIER_LALT},

	{"LEFT_AMIGA",
	IEQUALIFIER_LCOMMAND},

	{"LEFT_BUTTON",
	IEQUALIFIER_LEFTBUTTON},

	{"LEFT_COMMAND",
	IEQUALIFIER_LCOMMAND},

	{"LEFT_SHIFT",
	IEQUALIFIER_LSHIFT},

	{"LSHIFT",
	IEQUALIFIER_LSHIFT},

	{"MBUTTON",
	IEQUALIFIER_MIDBUTTON},

	{"MIDBUTTON",
	IEQUALIFIER_MIDBUTTON},

	{"MIDDLEBUTTON",
	IEQUALIFIER_MIDBUTTON},

	{"MIDDLE_BUTTON",
	IEQUALIFIER_MIDBUTTON},

	{"NUMERICPAD",
	IEQUALIFIER_NUMERICPAD},

	{"NUMERIC_PAD",
	IEQUALIFIER_NUMERICPAD},

	{"NUMPAD",
	IEQUALIFIER_NUMERICPAD},

	{"NUM_PAD",
	IEQUALIFIER_NUMERICPAD},

	{"RALT",
	IEQUALIFIER_RALT},

	{"RAMIGA",
	IEQUALIFIER_RCOMMAND},

	{"RBUTTON",
	IEQUALIFIER_RBUTTON},

	{"RCOMMAND",
	IEQUALIFIER_RCOMMAND},

	{"RELATIVEMOUSE",
	IEQUALIFIER_RELATIVEMOUSE},

	{"REPEAT",
	IEQUALIFIER_REPEAT},

	{"RIGHTBUTTON",
	IEQUALIFIER_RBUTTON},

	{"RIGHT_ALT",
	IEQUALIFIER_RALT},

	{"RIGHT_AMIGA",
	IEQUALIFIER_RCOMMAND},

	{"RIGHT_BUTTON",
	IEQUALIFIER_RBUTTON},

	{"RIGHT_COMMAND",
	IEQUALIFIER_RCOMMAND},

	{"RIGHT_SHIFT",
	IEQUALIFIER_RSHIFT},

	{"RSHIFT",
	IEQUALIFIER_RSHIFT},

	{NULL,
	0}
    };

pix_S pix_Synonyms[]=
    {
	{"ALT",
	IXSYM_ALT},

	{"CAPS",
	IXSYM_CAPS},

	{"SHIFT",
	IXSYM_SHIFT},

	{NULL,
	0}
    };

pix_S pix_Upstroke[]=
    {
	{"UPSTROKE",
	0x1},

	{NULL,
	0}
    };


pix_S pix_Highmap[]=
    {
	{"BACKSPACE",
	0x41},

	{"BREAK",
	0x6e},

	{"COMMA",
	0x38},

	{"CURSOR_DOWN",
	0x4d},

	{"CURSOR_LEFT",
	0x4f},

	{"CURSOR_RIGHT",
	0x4e},

	{"CURSOR_UP",
	0x4c},

	{"DEL",
	0x46},

	{"DELETE",
	0x46},

	{"DOWN",
	0x4d},

	{"END",
	0x71},

	{"ENTER",
	0x43},

	{"ESC",
	0x45},

	{"ESCAPE",
	0x45},

	/* The keys F10 to F12 must appear before F1 to F9
	   in this table or the parsing will fail */
	{"F10",
	0x59},

	{"F11",
	0x4b},

	{"F12",
	0x6f},

	{"F1",
	0x50},

	{"F2",
	0x51},

	{"F3",
	0x52},

	{"F4",
	0x53},

	{"F5",
	0x54},

	{"F6",
	0x55},

	{"F7",
	0x56},

	{"F8",
	0x57},

	{"F9",
	0x58},

	{"HELP",
	0x5f},

	{"HOME",
	0x70},

	{"INSERT",
	0x47},

	{"LEFT",
	0x4f},

	{"NUMLOCK",
	0x6d},

	{"PAGE_DOWN",
	0x49},

	{"PAGE_UP",
	0x48},

	{"PAUSE",
	0x6e},

	{"PRTSCR",
	0x6c},

	{"RETURN",
	0x44},

	{"RIGHT",
	0x4e},

	{"SCRLOCK",
	0x6b},

	{"SPACE",
	0x40},

	{"SPACEBAR",
	0x40},

	{"TAB",
	0x42},

	{"UP",
	0x4C},

/*	{NULL,
	0}
    };

pix_S pix_Extra[]=
    {
*/
	{"MOUSE_LEFTPRESS",
	0x68},

	{"MOUSE_MIDDLEPRESS",
	0x6a},

	{"MOUSE_RIGHTPRESS",
	0x69},

	{"(",
	0x5a},

	{")",
	0x5b},

	{"*",
	0x5d},

	{"+",
	0x5e},

	{"-",
	0x4a},

	{".",
	0x3c},

	{"/",
	0x5c},

	{"0",
	0x0f},

	{"1",
	0x1d},

	{"2",
	0x1e},

	{"3",
	0x1f},

	{"4",
	0x2d},

	{"5",
	0x2e},

	{"6",
	0x2f},

	{"7",
	0x3d},

	{"8",
	0x3e},

	{"9",
	0x3f},

	{NULL,
	0}
    };

#endif /* COMMODITIES_PARSE_H */
