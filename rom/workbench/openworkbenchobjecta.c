/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Open a drawer or launch a program.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <proto/utility.h>

#include <string.h>

#include "workbench_intern.h"
#include "support.h"
#include "handler.h"
#include "handler_support.h"

/*** Prototypes *************************************************************/
BOOL   __CLI_LaunchProgram(CONST_STRPTR command, struct TagItem *tags, struct WorkbenchBase *WorkbenchBase);
STRPTR __CLI_BuildCommandLine(CONST_STRPTR command, struct TagItem *tags, struct WorkbenchBase *WorkbenchBase);
BOOL   __WB_LaunchProgram(BPTR lock, CONST_STRPTR name, struct TagItem *tags, struct WorkbenchBase *WorkbenchBase);
BOOL   __WB_BuildArguments(struct WBStartup *startup, BPTR lock, CONST_STRPTR name, struct TagItem *tags, struct WorkbenchBase *WorkbenchBase);

/*** Macros *****************************************************************/
#define CLI_LaunchProgram(name, tags) (__CLI_LaunchProgram((name), (tags), WorkbenchBase))
#define CLI_BuildCommandLine(name, tags) (__CLI_BuildCommandLine((name), (tags), WorkbenchBase))
#define WB_LaunchProgram(lock, name, tags) (__WB_LaunchProgram((lock), (name), (tags), WorkbenchBase))
#define WB_BuildArguments(startup, lock, name, tags) (__WB_BuildArguments((startup), (lock), (name), (tags), WorkbenchBase))

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH2(BOOL, OpenWorkbenchObjectA,

/*  SYNOPSIS */
        AROS_LHA(STRPTR,           name, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 16, Workbench)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    BOOL                  success = FALSE;
    struct FileInfoBlock  fib     = { 0 };
    BPTR                  lock    = NULL;
    struct DiskObject    *icon    = NULL;
    
    /* Test whether the named path exist, as-is. */
    if ((lock = Lock(name, ACCESS_READ)))
    {
        /* Find out some info about the file. */
        if (Examine(lock, &fib))
        {
            if (fib.fib_DirEntryType > 0)
            {
                /* 
                    Since it's a directory, tell the Workbench Application
                    to open the corresponding drawer. 
                */
                // FIXME
            }
            else if (~fib.fib_Protection & FIBF_EXECUTE)
            {
                /*
                    It's an executable. Before I launch it, I must check
                    whether it is a Workbench program or a CLI program.
                */
                icon = GetIconTags(name, TAG_DONE);
                if (icon != NULL)
                {
                    /* It's a Workbench program */
                    success = WB_LaunchProgram(ParentDir(lock), FilePart(name), tags);
                    if (!success)
                    {
                        /*
                            Fallback to launching it as a CLI program. Most 
                            likely it will also fail, but we might get lucky.
                        */
                        success = CLI_LaunchProgram(name, tags);
                    }
                    
                    FreeDiskObject(icon);
                } 
                else
                {
                    /* It's a CLI program */
                    success = CLI_LaunchProgram(name, tags);
                }
            }
            #if 0 /* [ach] script bit not handled properly in AROS */
            else if (fib.fib_Protection & FIBF_SCRIPT)
            {
                /* 
                    It's a script. Launch it as an CLI program as such:
                    C:Execute <filename> <args>, building the arguments as
                    for a normal CLI program.
                */
                // FIXME
                
            }
            #endif
            else
            {
                /*
                    Since it's not a directory nor an executable, it must be
                    a plain data file. Test whether it has an icon, and if
                    so try to launch it's default tool (if it has one). 
                */
                
                icon = GetIconTags
                (
                    name, ICONGETA_FailIfUnavailable, FALSE, TAG_DONE
                );
                
                if (icon != NULL)
                {
                    /* Test whether it has an valid default tool. */
                    if
                    (
                           icon->do_DefaultTool != NULL 
                        && strlen(icon->do_DefaultTool) > 0
                    )
                    {
                        BPTR parent = ParentDir(lock);
                        
                        success = OpenWorkbenchObject
                        (
                            icon->do_DefaultTool,
                            WBOPENA_ArgLock, (IPTR) parent,
                            WBOPENA_ArgName, (IPTR) FilePart(name), 
                            TAG_DONE
                        );
                    }
                    else
                    {
                        // FIXME: default to global identify hook
                    }
                    
                    FreeDiskObject(icon);
                } 
                else
                {
                    // FIXME
                    // global identify hook
                }
            }
        }
        
        UnLock(lock);
    }
    else
    {
        /*
            Locking the named path failed, and therefore we need to search the
            default search path for an executable of that name and launch 
            it. This only makes sense if the name is *not* an (absolute or 
            relative) path: that is, it must not contain any ':' or '/'.
        */
        
        // FIXME: links in strpbrk from host libc?!?! -> crash
        //if (strpbrk(name, '/:') == NULL)
        {
            struct CommandLineInterface *cli = Cli();
            if (cli != NULL)
            {
                BPTR *paths;          /* Path list */
                BOOL  running = TRUE;
                
                /* Iterate over all paths in the path list */
                for
                (
                    paths = (BPTR *) BADDR(cli->cli_CommandDir);
                    running == TRUE && paths != NULL;
                    paths = (BPTR *) BADDR(paths[0]) /* next path */
                )
                {
                    BPTR cd   = CurrentDir(paths[1]);
                    BPTR lock = Lock(name, SHARED_LOCK);
                    
                    if (lock != NULL)
                    {
                        success = OpenWorkbenchObjectA(name, tags);
                        running = FALSE;
                        
                        UnLock(lock);
                    }
                    
                    CurrentDir(cd);
                }
            }
        }
    }
    
    return success;

    AROS_LIBFUNC_EXIT
} /* OpenWorkbenchObjectA() */

STRPTR __CLI_BuildCommandLine
(
    CONST_STRPTR command, struct TagItem *tags,
    struct WorkbenchBase *WorkbenchBase
)
{
    struct TagItem *tstate   = tags;
    struct TagItem *tag      = NULL;
    BPTR            lastLock = NULL;
    STRPTR          buffer   = NULL;
    ULONG           length   = strlen(command) + 3 /* NULL + 2 '"' */;
    
    /*-- Calculate length of resulting string ------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case WBOPENA_ArgLock:
                lastLock = (BPTR) tag->ti_Data;
                break;
                
            case WBOPENA_ArgName:
                if (lastLock != NULL)
                {
                    BPTR cd   = CurrentDir(lastLock);
                    BPTR lock = Lock((STRPTR) tag->ti_Data, ACCESS_READ);
                    if (lock != NULL)
                    {
                        STRPTR path = AllocateNameFromLock(lock);
                        if (path != NULL)
                        {
                            length += 1 /* space */ + strlen(path);
                            FreeVec(path);
                        }
                        
                        UnLock(lock);
                    }
                    
                    CurrentDir(cd);
                }
                break;
        }
    }
    
    /*-- Allocate space for command line string ----------------------------*/
    buffer = AllocVec(length, MEMF_ANY);
    
    if (buffer != NULL)
    {
        buffer[0] = '\0';
        
        /*-- Build command line --------------------------------------------*/
        strcat(buffer, "\"");
        strcat(buffer, command);
        strcat(buffer, "\"");
        
        tstate = tags; lastLock = NULL;
        while ((tag = NextTagItem(&tstate)) != NULL )
        {
            switch (tag->ti_Tag)
            {
                case WBOPENA_ArgLock:
                    lastLock = (BPTR) tag->ti_Data;
                    break;
                
                case WBOPENA_ArgName:
                    if (lastLock != NULL)
                    {
                        BPTR cd   = CurrentDir(lastLock);
                        BPTR lock = Lock((STRPTR) tag->ti_Data, ACCESS_READ);
                        if (lock != NULL)
                        {
                            STRPTR path = AllocateNameFromLock(lock);
                            if (path != NULL)
                            {
                                strcat(buffer, " ");
                                strcat(buffer, path);
                                FreeVec(path);
                            }
                            
                            UnLock(lock);
                        }
                        
                        CurrentDir(cd);
                    }
                    break;
            }
        }
    }
    else
    {
        SetIoErr(ERROR_NO_FREE_STORE);
    }
    
    return buffer;
}

BOOL __CLI_LaunchProgram
(
    CONST_STRPTR command, struct TagItem *tags, 
    struct WorkbenchBase *WorkbenchBase
)
{
    BPTR   input       = NULL;
    STRPTR commandline = NULL;
    
    input = Open("CON:////Output Window/CLOSE/AUTO/WAIT", MODE_OLDFILE);
    if (input == NULL) goto error;
    
    commandline = CLI_BuildCommandLine(command, tags);
    if (commandline == NULL) goto error;
    
    if
    (
        /*
            Launch the program. Note that there is no advantage of doing this
            in the handler, since we need to wait for the return value anyway
            (and thus we cannot cut down the blocking time by returning before
            the program is loaded from disk).
        */
        
        SystemTags
        (
            commandline,
            SYS_Asynch,          TRUE,
            SYS_Input,    (IPTR) input,
            SYS_Output,   (IPTR) NULL,
            SYS_Error,    (IPTR) NULL, 
            NP_StackSize,        WorkbenchBase->wb_DefaultStackSize,
            TAG_DONE
        ) == -1
    )
    {
        goto error;
    }
    FreeVec(commandline);
    
    return TRUE;
    
error:
    if (input != NULL) Close(input);
    if (commandline != NULL) FreeVec(commandline);
    
    return FALSE;
}

BOOL __WB_BuildArguments
(
    struct WBStartup *startup, BPTR lock, CONST_STRPTR name, struct TagItem *tags, 
    struct WorkbenchBase *WorkbenchBase
)
{
    struct TagItem *tstate   = tags,
                   *tag      = NULL;
    BPTR            lastLock = NULL;
    LONG            numArgs  = 1;
    struct WBArg   *args;
    
    /*-- Calculate the number of arguments ---------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case WBOPENA_ArgLock:
                lastLock = (BPTR) tag->ti_Data;
                break;
                
            case WBOPENA_ArgName:
                if (lastLock != NULL) numArgs++;
                break;
        }
    }
    
    /*-- Allocate memory for the arguments ---------------------------------*/
    args = AllocMem(sizeof(struct WBArg) * numArgs, MEMF_ANY | MEMF_CLEAR);
    if (args != NULL)
    {
        /*-- Build the argument list ---------------------------------------*/
        LONG i      = 0;
        BOOL error  = FALSE;
        
        if
        (
               (args[i].wa_Lock = lock)         == NULL
            || (args[i].wa_Name = StrDup(name)) == NULL
        )
        {
            goto error;
        }
        i++;
        
        tstate = tags; lastLock = NULL;
        while ((tag = NextTagItem(&tstate)) != NULL && !error)
        {
            switch (tag->ti_Tag)
            {
                case WBOPENA_ArgLock:
                    lastLock = (BPTR) tag->ti_Data;
                    break;
                    
                case WBOPENA_ArgName:
                    if (lastLock != NULL)
                    {
                        STRPTR name = (STRPTR) tag->ti_Data;
                        
                        /* Duplicate the lock and the name */
                        if
                        (
                               (args[i].wa_Lock = DupLock(lastLock)) == NULL
                            || (args[i].wa_Name = StrDup(name))      == NULL
                        )
                        {
                            D(bug("workbench.library: WB_BuildArguments: Failed to duplicate lock or string\n"));
                            error = TRUE;
                            break;
                        }
                        i++;
                    }
                    break;
            }
        }
        
error:
        if (error)
        {
            D(bug("workbench.library: WB_BuildArguments: Freeing resources after error...\n"));
            /* Free allocated resources */
            for (i = 0; i < numArgs; i++)
            {
                if (args[i].wa_Lock != NULL) UnLock(args[i].wa_Lock);
                if (args[i].wa_Name != NULL) FreeVec(args[i].wa_Name);
            }
            
            return FALSE;
        }
        
        startup->sm_NumArgs = numArgs;
        startup->sm_ArgList = args;
        
        return TRUE;
    }
    
    D(bug("workbench.library: WB_BuildArguments: Failed to allocate memory for argument array\n"));
    
    return FALSE;
}

BOOL __WB_LaunchProgram
(
    BPTR lock, CONST_STRPTR name, struct TagItem *tags,
    struct WorkbenchBase *WorkbenchBase
)
{
    struct WBStartup     *startup = NULL;
    struct LaunchMessage *message = NULL;
       
    /*-- Allocate memory for messages --------------------------------------*/
    startup = AllocMem(sizeof(struct WBStartup), MEMF_PUBLIC | MEMF_CLEAR);
    if (startup == NULL) 
    {
        D(bug("workbench.library: WB_LaunchProgram: Failed to allocate memory for startup message\n"));
        SetIoErr(ERROR_NO_FREE_STORE); 
        goto error;
    }
    MESSAGE(startup)->mn_Length = sizeof(struct WBStartup);
    
    message = AllocMem(sizeof(struct LaunchMessage), MEMF_PUBLIC | MEMF_CLEAR);
    if (message == NULL)
    {
        D(bug("workbench.library: WB_LaunchProgram: Failed to allocate memory for launch message\n"));
        SetIoErr(ERROR_NO_FREE_STORE);
        goto error;
    }
    MESSAGE(message)->mn_Length = sizeof(struct LaunchMessage);
    
    /*-- Build the arguments array -----------------------------------------*/
    if (!WB_BuildArguments(startup, lock, name, tags))
    {
        D(bug("workbench.library: WB_LaunchProgram: Failed to build arguments\n"));
        goto error;
    }

    /*-- Send message to handler -------------------------------------------*/
    D(bug("workbench.library: WB_LaunchProgram: Setting up message\n"));
    message->lm_HandlerMessage.hm_Type = HM_TYPE_LAUNCH;
    message->lm_StartupMessage         = startup; 
    
    /* The handler will deallocate the memory! */
    D(bug("workbench.library: WB_LaunchProgram: Sending message\n"));
    PutMsg(&(WorkbenchBase->wb_HandlerPort), MESSAGE(message));
    
    D(bug("workbench.library: WB_LaunchProgram: Success\n"));
    
    return TRUE;

error:
    if (startup != NULL) FreeMem(startup, sizeof(struct WBStartup));
    if (message != NULL) FreeMem(message, sizeof(struct LaunchMessage));
    
    D(bug("workbench.library: WB_LaunchProgram: Failure\n"));
    
    return FALSE;
}
