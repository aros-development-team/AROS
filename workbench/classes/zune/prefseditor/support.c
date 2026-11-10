/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.
    This file is part of the PrefsEditor class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/exec.h>

#include <stdio.h>
#include <aros/debug.h>

#include "support.h"

/* Note: buffer *MUST* be large enough for atleast strlen(prefix) + 8 + 1 */
/* Create a temporary file using prefix followed by GetUniqueID formatted as
 * eight hexadecimal digits. Require a writable buffer large enough for the
 * prefix, digits and terminator, and a readable prefix. Return an open DOS
 * handle, leaving the generated name in buffer, or BNULL for an Open error
 * other than ERROR_OBJECT_EXISTS. Retry that error without a retry limit.
 * MODE_NEWFILE is not an exclusive-create guarantee and can truncate an
 * existing destination. Caller owns closing the handle and deleting the
 * file.
 */
BPTR CreateTemporary(STRPTR buffer, CONST_STRPTR prefix)
{
    BPTR fh;
    
    while (TRUE)
    {
        sprintf(buffer, "%s%08lx", prefix, (long)GetUniqueID());
        fh = Open(buffer, MODE_NEWFILE);
        
        if (fh != BNULL)
        {
            return fh;
        }
        else if (IoErr() != ERROR_OBJECT_EXISTS)
        {
            return BNULL;
        }
    }
}

/* Create the directory at the readable DOS path and release its returned
 * lock. Return TRUE if CreateDir succeeds, otherwise FALSE, including a
 * failed attempt on an existing directory. Do not retain the path or return
 * a lock.
 */
BOOL MakeDir(CONST_STRPTR path)
{
    BPTR lock = CreateDir(path);
    
    if (lock != BNULL)
    {
        UnLock(lock);
        return TRUE;
    }
    
    return FALSE;
}

/* Walk a writable NUL-terminated pathname and temporarily terminate it at
 * each slash. Accept a successfully locked prefix or try to create it,
 * releasing every acquired lock. Restore each slash before continuing or
 * returning FALSE on failure. Return TRUE when all prefixes succeed,
 * including paths without slashes. The final component is not created;
 * directories already created are not rolled back. The input spelling is
 * restored on return.
 */
BOOL MakeDirs(STRPTR path)
{
    STRPTR position;
    BOOL   success = FALSE;
    BPTR   lock    = BNULL;
    
    for (position = path; *position != '\0'; position++)
    {
        if (*position == '/')
        {
            *position = '\0';
            
            if ((lock = Lock(path, SHARED_LOCK)) != BNULL)
            {
                UnLock(lock);
                success = TRUE;
            }
            else
            {
                success = MakeDir(path);
            }
            
            *position = '/';
            
            if (!success) return FALSE;
        }
    }
    
    return TRUE;
}
