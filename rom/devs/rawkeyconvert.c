/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: console.device function RawKeyConvert()
    Lang: english
*/
#include <exec/libraries.h>
#include <devices/inputevent.h>
#include <devices/keymap.h>
#include <clib/console_protos.h>

extern LONG intui_RawKeyConvert (struct InputEvent *, STRPTR, LONG,
				struct KeyMap *);

/*****************************************************************************

    NAME */
	#include <devices/inputevent.h>
	#include <devices/keymap.h>
	#include <clib/console_protos.h>

	AROS_LH4(LONG, RawKeyConvert,

/*  SYNOPSIS */
	AROS_LHA(struct InputEvent *, events, A0),
	AROS_LHA(STRPTR             , buffer, A1),
	AROS_LHA(LONG               , length, D1),
	AROS_LHA(struct KeyMap     *, keyMap, A2),

/*  LOCATION */
	struct Library *, ConsoleDevice, 8, Console)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from


*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,ConsoleDevice)

    return intui_RawKeyConvert (events, buffer, length, keyMap);

    AROS_LIBFUNC_EXIT
} /* RawKeyConvert */
