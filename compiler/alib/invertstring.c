/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/commodities.h>
#include <proto/commodities.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <exec/memory.h>
#include <devices/keymap.h>
#include <devices/inputevent.h>

extern struct Library *CxBase;

/*****************************************************************************

    NAME */

    struct InputEvent *InvertString(

/*  SYNOPSIS */

        STRPTR str,
        struct KeyMap *km
                     )
/*  FUNCTION
        Return a linked list of input events which would produce the string
        'str' in the reverse order, with the keymap 'km'. To get a list of
        events in the right order, just use InvertStringForwd(). InverString()
        is only there to provide compatibility with the original function,
        which indeed reversed the order of the supplied string.

    INPUTS
        str  --  pointer to a (NULL-terminated) string that may contain
                 * ANSI character codes
                 * backslash-escaped characters:
                   \n    --  carriage return
                   \r    --  carriage return
                   \t    --  tab
                   \\    --  backslash
                 * a description of an input event a la ParseIX() surrounded
                   by angle brackets

        km   --  keymap to use for the conversion or NULL to use the default
                 keymap

    RESULT
        A linked list of input events or NULL if something went wrong.

    NOTES

    EXAMPLE
        An example string: "\n<shift alt a> olleH"

    BUGS

    SEE ALSO
        commodities.library/ParseIX(), InvertStringForwd(), FreeIEvents()

    INTERNALS
        Ought to have an extra \< for < not starting an IX expression.

    HISTORY

******************************************************************************/
{
    struct InputEvent *first, *second, *third, *fourth;
    if ((first = InvertStringForwd(str, km)))
    {
        fourth = first;
        third = first->ie_NextEvent;
        while (third)
        {
            second = first;
            first = third;
            third = first->ie_NextEvent;
            first->ie_NextEvent = second;
        }
        fourth->ie_NextEvent = NULL;
    }
    return first;
} /* InvertString */

