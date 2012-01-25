/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
        'str' with the keymap 'km'.

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
        An example string: "Hello <shift alt a>\n"

    BUGS

    SEE ALSO
        commodities.library/ParseIX(), FreeIEvents()

    INTERNALS
        Ought to have an extra \< for < not starting an IX expression.

    HISTORY

******************************************************************************/
{
    struct InputEvent *ieChain = NULL;
    struct InputEvent *ie;
    struct InputEvent *first = NULL;
    UBYTE                  ansiCode;
    UBYTE                 *start;

    while(*str != '\0')
    {
        ie = AllocMem(sizeof(struct InputEvent), MEMF_PUBLIC | MEMF_CLEAR);
        if (!ie)
        {
            if (first) FreeIEvents(first);
            return NULL;
        }
        
        if (!first) first = ie;
        
        if(ieChain != NULL)
            ieChain->ie_NextEvent = ie;            
        
        ieChain = ie;
        
        ie->ie_Class = IECLASS_RAWKEY;
        ie->ie_EventAddress = NULL;
        
        switch(*str)
        {
        case '\\' :
            str++;
            switch(*str)
            {
            case 't':
                ansiCode = '\t';
                break;
            case 'r':
            case 'n':
                ansiCode = '\n';
                break;
            case '\\':
                ansiCode = '\\';
                break;
            default  :
                /* FIXME: What to do if "\x" comes? */
                /* stegerg: This? */
                ansiCode = *str;
                break;
            }

            if(InvertKeyMap(ansiCode, ie, km) == FALSE)
            {
                FreeIEvents(first);
                return NULL;
            }

            break;
            
        case '<' :
            start = ++str;

            while(*str && (*str != '>')) str++;
                        
            {
                IX ix = {0};
                LONG err;
                
                *str = '\0';            
                err = ParseIX(start, &ix);
                *str++ = '>';
                
                if (err < 0)
                {
                    FreeIEvents(first);
                    return NULL;
                }

                    ie->ie_Class     = ix.ix_Class;
                    ie->ie_Code      = ix.ix_Code;
                    ie->ie_Qualifier = ix.ix_Qualifier;
                }
            break;
            
        default :
            if(InvertKeyMap(*str++, ie, km) == FALSE)
            {
                FreeIEvents(first);
                return NULL;
            }
            
            break;
        }
        
    }

    return first;
} /* InvertString */
