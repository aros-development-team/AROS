/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <devices/inputevent.h>
#include <devices/keymap.h>
#include "cxintern.h"
#include <proto/keymap.h>
#define __NOLIBBASE__ 1
#include <proto/timer.h>
#undef __NOLIBBASE__
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/commodities.h>

    AROS_LH3(BOOL, InvertKeyMap,

/*  SYNOPSIS */

	AROS_LHA(ULONG              , ansiCode , D0),
	AROS_LHA(struct InputEvent *, event    , A0),
	AROS_LHA(struct KeyMap     *, km       , A1),

/*  LOCATION */

	struct Library *, CxBase, 29, Commodities)

/*  FUNCTION

    Translate a given ANSI character code to an InputEvent. The InputEvent
    pointed to by 'event' is initialized to match the 'ansiCode'. The
    translation is done using the keymap 'km'. If 'km' is NULL, the default
    system keymap is used.

    INPUTS

    ansiCode  -  the ANSI character code to be translated
    event     -  the inputevent that will contain the translation
    km        -  keymap used for the translation (if 'km' is NULL the system
                 default keymap is used).

    RESULT

    TRUE if the translation was successful, otherwise FALSE.

    NOTES

    EXAMPLE

    BUGS

    Only one-deep dead keys are handled, for instance <alt f>e. It doesn't
    look up the high key map (keycodes with scan codes greater than 0x40).

    SEE ALSO

    cx_lib/InvertString()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    char buf[4];
    char str[2] = { 0, 0 };
    LONG k;

    event->ie_SubClass = 0;

    GetSysTime(&event->ie_TimeStamp);

    event->ie_EventAddress = NULL;
    event->ie_Class = IECLASS_RAWKEY;
    str[0] = ansiCode;
    k = MapANSI((STRPTR) &str, 1, (STRPTR) &buf, 3, km);

    switch(k)
    {
    case 1 :   /* One code / qualifier */
	event->ie_Prev1DownCode = 0;
	event->ie_Prev1DownQual = 0;
	D(bug("Buf 0: %i\n", buf[0]));
	event->ie_Code = (WORD)(buf[0]);
	event->ie_Qualifier = (WORD) buf[1];

	return TRUE;
	
    case 2 :   /* Two codes / qualifiers */
	event->ie_Prev1DownCode = buf[0];
	event->ie_Prev1DownQual = buf[1];
	event->ie_Code = (WORD) buf[2];
	event->ie_Qualifier = (WORD) buf[3];
	
	return TRUE;
	
    default :
	return FALSE; /* Error mapping ANSI */
    }
    
    AROS_LIBFUNC_EXIT
} /* InvertKeyMap */
