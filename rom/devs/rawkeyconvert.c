/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/11/08 11:27:54  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.2  1996/10/24 15:50:22  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/08/23 17:32:24  digulla
    Implementation of the console.device


    Desc:
    Lang: english
*/
#include <exec/libraries.h>
#include <devices/inputevent.h>
#include <devices/keymap.h>

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
