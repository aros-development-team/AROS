/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/23 17:32:24  digulla
    Implementation of the console.device


    Desc:
    Lang: english
*/
#include <exec/libraries.h>
#include <devices/inputevent.h>
#include <devices/keymap.h>

extern LONG intui_RawKeyConvert (struct InputEvent *, STRPTR, long,
				struct KeyMap *);

/*****************************************************************************

    NAME */
	#include <devices/inputevent.h>
	#include <devices/keymap.h>
	#include <clib/console_protos.h>

	__AROS_LH4(LONG, RawKeyConvert,

/*  SYNOPSIS */
	__AROS_LHA(struct InputEvent *, events, A0),
	__AROS_LHA(STRPTR             , buffer, A1),
	__AROS_LHA(long               , length, D1),
	__AROS_LHA(struct KeyMap     *, keyMap, A2),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct Library *,ConsoleDevice)

    return intui_RawKeyConvert (events, buffer, length, keyMap);

    __AROS_FUNC_EXIT
} /* RawKeyConvert */
