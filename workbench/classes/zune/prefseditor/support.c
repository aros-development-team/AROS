/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the PrefsEditor class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/exec.h>

#include <stdio.h>
#include <aros/debug.h>

#include "support.h"

/* Note: buffer *MUST* be large enough for atleast strlen(prefix) + 8 + 1 */
BPTR CreateTemporary(STRPTR buffer, CONST_STRPTR prefix)
{
    BPTR fh;
    
    while (TRUE)
    {
        sprintf(buffer, "%s%08lx", prefix, GetUniqueID());
        fh = Open(buffer, MODE_NEWFILE);
        
        if (fh != NULL)
        {
            return fh;
        }
        else if (IoErr() != ERROR_OBJECT_EXISTS)
        {
            return NULL;
        }
    }
}
