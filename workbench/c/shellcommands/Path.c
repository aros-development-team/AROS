/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

*/

/*****************************************************************************

    NAME

        Path [{<dir>}] [ADD] [SHOW] [RESET] [REMOVE] [QUIET] [HEAD]

    SYNOPSIS

        PATH/M,ADD/S,SHOW/S,RESET/S,REMOVE/S,QUIET/S,HEAD/S

    LOCATION

        C:

    FUNCTION
        
        Changes the search path for commands. Without arguments it shows the path.
        
    INPUTS

        PATH    -- path  
        ADD     -- adds path
        SHOW    -- shows path
        RESET   -- removes existing path and replaces it by new path
        REMOVE  -- removes the given path
        QUIET   -- suppresses dialog when a path is not found
        HEAD    -- inserts path at beginning of path list
        
    RESULT

    NOTES

    EXAMPLE

        path dh0:work add
        
    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/

#include <exec/memory.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/dos.h>
#include "dos_commanderrors.h"

#include <aros/shcommands.h>

#include <aros/debug.h>

typedef struct CommandLineInterface* CommandLineInterfacePtr;

typedef struct
{
    BPTR next;
    BPTR lock;
} PathEntry, *PathEntryPtr;

#define PE(x) ((PathEntry *)(BADDR(x)))

static PathEntryPtr FindPathEntry(CommandLineInterfacePtr cli, STRPTR pathName,
    PathEntryPtr* predStorage, APTR DOSBase);

static PathEntryPtr InsertPathEntry(PathEntryPtr predecessor, STRPTR pathName, APTR SysBase, APTR DOSBase);

static BOOL IsDirectory(BPTR lock, APTR DOSBase);

AROS_SH7(Path, 45.4,
AROS_SHA(STRPTR *, ,PATH,/M,NULL),
AROS_SHA(BOOL, ,ADD,/S,NULL),
AROS_SHA(BOOL, ,SHOW,/S,NULL),
AROS_SHA(BOOL, ,RESET,/S,NULL),
AROS_SHA(BOOL, ,REMOVE,/S,NULL),
AROS_SHA(BOOL, ,QUIET,/S,NULL),
AROS_SHA(BOOL, ,HEAD,/S,NULL))
{
    AROS_SHCOMMAND_INIT

        CommandLineInterfacePtr
    cli = Cli();

    if(!cli)
    {
        PrintFault(ERROR_SCRIPT_ONLY, "Path");

        return RETURN_ERROR;
    }

    if (SHArg(RESET))
    {
            PathEntryPtr
        cur = PE(cli->cli_CommandDir);

        while (cur)
        {
                PathEntryPtr
            next = PE(cur->next);

            UnLock(cur->lock);

            FreeVec(BADDR(cur->next));

            cur = next;
        }
        
        cli->cli_CommandDir = BNULL;
    }

        STRPTR*
    names = SHArg(PATH);

    if (names && *names)
    {
            int
        idx;

        if (SHArg(REMOVE) && !SHArg(RESET))
        {
                PathEntryPtr
            pred = NULL,
            entry = NULL;

            for (idx = 0; names[idx]; ++idx)
            {
                entry = FindPathEntry(cli, names[idx], &pred, DOSBase);

/* free the path entry */
                if (NULL != entry)
                {
                    if (NULL != pred)
                    {
                        pred->next = entry->next;
                    }
                    else
                    {
                        cli->cli_CommandDir = entry->next;
                    }

                    if (BNULL != entry->lock)
                    {
                        UnLock(entry->lock);
                    }
                
                    FreeVec(entry);
                }
            }
        }
        else    /* add */
        {
                PathEntryPtr
            insertAfter = (PathEntryPtr)&cli->cli_CommandDir;
            
            if (!SHArg(HEAD))
            {
                /* Search last entry */
                while (BNULL != insertAfter->next)
                {
                    insertAfter = PE(insertAfter->next);
                }
            }

            for (idx = 0; names[idx]; ++idx)
            {
                if (NULL != FindPathEntry(cli, names[idx], NULL, DOSBase))
                {
/* don't add if already in path */
                    continue;
                }
                else
                {
                    insertAfter = InsertPathEntry(insertAfter, names[idx], SysBase, DOSBase);
                }
            }
        }
    }
    else
    {
        SHArg(SHOW) = SHArg(RESET) == 0;
    }

    if (SHArg(SHOW))
    {
        UBYTE Buffer[2048];

        PutStr("Current Directory\n");

            PathEntryPtr
        cur = NULL;

    	for (cur = PE(cli->cli_CommandDir); cur; cur = PE(cur->next))
    	{
            NameFromLock (cur->lock, Buffer, sizeof (Buffer));
            
            PutStr(Buffer);
            PutStr("\n");
    	}

        PutStr("C:\n");
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}


/** find the specfied path name in the specified CLI's path list.
 * 
 * @autodoc:function if the path specified by pathName is in the path list of
 * the specified CLI the corresponding path entry is returned. the path entry's
 * predecessor is stored in the specified location if the storage pointer is
 * not NULL.
 * 
 */
static PathEntryPtr
FindPathEntry(CommandLineInterfacePtr cli, STRPTR pathName,
    PathEntryPtr* predStorage, APTR DOSBase)
{
        PathEntryPtr
    entry = NULL;
    
    if (NULL != cli && BNULL != cli->cli_CommandDir && NULL != pathName)
    {
            BPTR
        pathLock = Lock(pathName, ACCESS_READ);

        if (BNULL != pathLock)
        {
                PathEntryPtr
            pred = NULL,
            curr = PE(cli->cli_CommandDir);
    
            while (NULL != curr)
            {
                    LONG
                value = SameLock(pathLock, curr->lock);
                
                if (LOCK_SAME == value)
                {
                    entry = curr;
                    
                    if (NULL != predStorage)
                    {
                        *predStorage = pred;
                    }
                    break;
                }
                else
                {
                    pred = curr;
                    curr = PE(curr->next);
                }
            }

            UnLock(pathLock);
        }
        else
            PrintFault(IoErr(), pathName);
    }
    
    return(entry);        
}


/* insert a path entry for the specified path
 * 
 * create and insert a path entry for the specified path name. the new path
 * entry is inserted after the specified predecessor.
 * 
 * returns the path entry or NULL for failure.
 */
static PathEntryPtr
InsertPathEntry(PathEntryPtr predecessor, STRPTR pathName, APTR SysBase, APTR DOSBase)
{
        PathEntryPtr
    pathEntry = NULL;

    {
            PathEntryPtr
        newEntry = AllocVec(sizeof(PathEntry), MEMF_ANY);
        
            BPTR
        lock = Lock(pathName, SHARED_LOCK);

            BOOL
        isDirectory = (BNULL != lock)
            ? IsDirectory(lock, DOSBase)
            : FALSE;
        
        if (newEntry != NULL && lock != BNULL && isDirectory)
        {
            newEntry->lock = lock;
            newEntry->next = predecessor->next;
            
            predecessor->next = MKBADDR(newEntry);

            pathEntry = newEntry;
        }
        else
        {
            if (lock != BNULL)
            {
                if (!isDirectory)
                {
                    PrintFault(ERROR_OBJECT_WRONG_TYPE, pathName);
                }
            
                UnLock(lock);
            }

            if (newEntry != NULL)
            {
                FreeVec(newEntry);
            }
        }
    }

    return(pathEntry);
}


/* check if the object specified is a directory */
static BOOL
IsDirectory(BPTR lock, APTR DOSBase)
{
        BOOL
    isDirectory = FALSE;
    
        struct FileInfoBlock*
    fib = AllocDosObject(DOS_FIB, NULL);
    
    if (Examine(lock, fib))
    {
            LONG
        entryType = fib->fib_EntryType;
        
        if (entryType >= ST_ROOT && entryType <= ST_LINKDIR)
        {
            if (entryType != ST_SOFTLINK)
            {
                isDirectory = TRUE;
            }
            else
            {
                    BPTR
                dupLock = DupLock(lock);
                
                if (BNULL != dupLock)
                {
                        BPTR
                    file = OpenFromLock(dupLock);
                    
                    if (BNULL != file)
                    {
/* lock was on a file. dupLock is relinquished by OpenFromLock */
                        Close(file);
                        
                        isDirectory = FALSE;
                    }
                    else
                    {
                        UnLock(dupLock);
                    }
                }
                else
                {
                    isDirectory = FALSE;
                }
            }
        }

        FreeDosObject(DOS_FIB, fib);
    }
    
    return(isDirectory);
}
