/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the LoadResource program, which is distributed under
    the terms of version 2 of the GNU General Public License.

    FIXME:
    + Implement LOCK and UNLOCK options. Argument template should then be
      changed to "NAME/M,LOCK/S,UNLOCK/S"; if the user doesn't provide NAME,
      then all currently locked resources shall be listed.
    + Implement support for loading devices, fonts and catalogs.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include "locale.h"

enum
{
    ARG_NAME,
    ARG_COUNT
};

#define TEMPLATE     "NAME/M/A"
#define ERROR_HEADER "LoadResource"

BOOL process(CONST_STRPTR name);

int main(void)
{
    int            rc              = RETURN_OK;
    struct RDArgs *rdargs          = NULL;
    IPTR           args[ARG_COUNT] = { 0 };
    
    if ((rdargs = ReadArgs(TEMPLATE, args, NULL)) != NULL)
    {
        if (args[ARG_NAME] != 0)
        {
            CONST_STRPTR  *names = (CONST_STRPTR *) args[ARG_NAME],
                           name  = NULL;
            
            while ((name = *names++) != NULL)
            {
                if (!process(name))
                {
                    rc = RETURN_WARN;
                }
            }
        }
        else
        {
            // FIXME: List currently locked resources.
        }
        
        FreeArgs(rdargs);
    }
    else
    {
        PrintFault(IoErr(), ERROR_HEADER);
        rc = RETURN_FAIL;
    }
    
    return rc;
}

BOOL process(CONST_STRPTR name)
{
    struct Library *lb = OpenLibrary(name, 0L);
    
    if (lb != NULL)
    {
        CloseLibrary(lb);
        return TRUE;
    }
    else
    {
        PutStr(ERROR_HEADER": ");
        Printf(_(MSG_ERROR_OPEN_LIBRARY), name);
        PutStr("\n");
        return FALSE;
    }
}
