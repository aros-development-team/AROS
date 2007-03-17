/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Open a drawer or launch a program.
*/
#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <string.h>

#include "workbench_intern.h"
#include "support.h"
#include "support_messages.h"
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

    BOOL                  success       = FALSE;
    LONG                  isDefaultIcon = 42;

    struct DiskObject    *icon          = GetIconTags
    (
        name,
        ICONGETA_FailIfUnavailable,        FALSE,
        ICONGETA_IsDefaultIcon,     (IPTR) &isDefaultIcon,
        TAG_DONE
    );

    ASSERT_VALID_PTR(name);
    ASSERT_VALID_PTR_OR_NULL(tags);

    D(bug("OpenWorkbenchObject: name = %s\n", name));
    D(bug("OpenWorkbenchObject: isDefaultIcon = %ld\n", isDefaultIcon));

    if (icon != NULL)
    {
        switch (icon->do_Type)
        {
            case WBDISK:
            case WBDRAWER:
            case WBGARBAGE:
                /*
                    Since it's a directory or volume, tell the Workbench
                    Application to open the corresponding drawer.
                */

                D(bug("OpenWorkbenchObject: it's a DISK, DRAWER or GARGABE\n"));

                {
                    struct WBCommandMessage *wbcm     = NULL;
                    struct WBHandlerMessage *wbhm     = NULL;
                    CONST_STRPTR             namecopy = NULL;

                    if
                    (
                           (wbcm     = CreateWBCM(WBCM_TYPE_RELAY)) != NULL
                        && (wbhm     = CreateWBHM(WBHM_TYPE_OPEN))  != NULL
                        && (namecopy = StrDup(name))                != NULL

                    )
                    {
                        wbhm->wbhm_Data.Open.Name     = namecopy;
                        wbcm->wbcm_Data.Relay.Message = wbhm;

                        PutMsg(&(WorkbenchBase->wb_HandlerPort), (struct Message *) wbcm);
                    }
                    else
                    {
                        FreeVec((STRPTR)namecopy);
                        DestroyWBHM(wbhm);
                        DestroyWBCM(wbcm);
                    }
                }

                break;

            case WBTOOL:
                /*
                    It's an executable. Before I launch it, I must check
                    whether it is a Workbench program or a CLI program.
                */

                D(bug("OpenWorkbenchObject: it's a TOOL\n"));

                if
                (
                       !isDefaultIcon
                    && FindToolType(icon->do_ToolTypes, "CLI") == NULL
                )
                {
                    /* It's a Workbench program */
                    BPTR lock = Lock(name, ACCESS_READ);

                    D(bug("OpenWorkbenchObject: it's a WB program\n"));

                    if (lock != NULL)
                    {
                        BPTR parent = ParentDir(lock);

                        if (parent != NULL)
                        {
                            success = WB_LaunchProgram
                            (
                                parent, FilePart(name), tags
                            );

                            if (!success)
                            {
                                /*
                                    Fallback to launching it as a CLI program.
                                    Most likely it will also fail, but we
                                    might get lucky.
                                */
                                success = CLI_LaunchProgram(name, tags);
                            }

                            UnLock(parent);
                        }

                        UnLock(lock);
                    }
                }
                else
                {
                    /* It's a CLI program */

                    D(bug("OpenWorkbenchObject: it's a CLI program\n"));

                    success = CLI_LaunchProgram(name, tags);
                }
                break;

            case WBPROJECT:
                /* It's a project; try to launch it via it's default tool. */

                D(bug("OpenWorkbenchObject: it's a PROJECT\n"));
                D(bug("OpenWorkbenchObject: default tool: %s\n", icon->do_DefaultTool));

                if
                (
                       icon->do_DefaultTool != NULL
                    && strlen(icon->do_DefaultTool) > 0
                )
                {
                    BPTR lock = (BPTR)NULL, lock2 = (BPTR)NULL,
                        parent = (BPTR)NULL, parent2 = (BPTR)NULL;
                    
                    lock = Lock(name, ACCESS_READ);
                    if (lock != (BPTR)NULL)
                        parent = ParentDir(lock);
                    if (parent != (BPTR)NULL)
                    {
                        struct TagItem tags[] =
                        {
                            {WBOPENA_ArgLock, (IPTR) parent},
                            {WBOPENA_ArgName, (IPTR) FilePart(name)},
                            {TAG_DONE, (IPTR)NULL}
                        };
                        
                        if (FindToolType(icon->do_ToolTypes, "CLI") == NULL)
                        {
                            BPTR lock2 = (BPTR)NULL, parent2 = (BPTR)NULL;

                            lock2 = Lock(icon->do_DefaultTool, ACCESS_READ);
                            if (lock2 != (BPTR)NULL)
                                parent2 = ParentDir(lock2);
                            if (parent2 != NULL)
                            {
                                success = WB_LaunchProgram
                                (
                                    parent2, FilePart(icon->do_DefaultTool), tags
                                );
                            }
                            UnLock(parent2);
                            UnLock(lock2);
                        }
                        else
                        {
                            success = CLI_LaunchProgram
                            (
                                icon->do_DefaultTool, tags
                            );
                        }

                        UnLock(parent);
                        UnLock(lock);
                    }
                }

                if (!success)
                {
                    // FIXME: open execute command?
                }
                break;
        }

        FreeDiskObject(icon);
    }
    else
    {
        /*
            Getting (a possibly default) icon for the path failed, and
            therefore there is no such file. We need to search the default
            search path for an executable of that name and launch it. This
            only makes sense if the name is *not* an (absolute or relative)
            path: that is, it must not contain any ':' or '/'.
        */

        if (strpbrk(name, "/:") == NULL)
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

    D(bug("OpenWorkbenchObject: success = %d\n", success));

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
                            length += 3 /* space + 2 '"' */ + strlen(path);
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
                                strcat(buffer, " \"");
                                strcat(buffer, path);
                                strcat(buffer, "\"");
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
    struct WBArg   *args     = NULL;

    /*-- Calculate the number of arguments ---------------------------------*/
    startup->sm_NumArgs = 1;
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case WBOPENA_ArgLock:
                lastLock = (BPTR) tag->ti_Data;
                break;

            case WBOPENA_ArgName:
                /*
                    Filter out args where both lock AND name are NULL, since
                    they are completely worthless to the application.
                */
                if (lastLock != NULL || (STRPTR) tag->ti_Data != NULL)
                {
                    startup->sm_NumArgs++;
                }
                break;
        }
    }

    /*-- Allocate memory for the arguments ---------------------------------*/
    args = AllocMem(sizeof(struct WBArg) * startup->sm_NumArgs, MEMF_ANY | MEMF_CLEAR);
    if (args != NULL)
    {
        LONG i = 0;

        startup->sm_ArgList = args;

        /*-- Build the argument list ---------------------------------------*/
        if
        (
               (args[i].wa_Lock = DupLock(lock)) == NULL
            || (args[i].wa_Name = StrDup(name))  == NULL
        )
        {
            goto error;
        }
        i++;

        tstate = tags; lastLock = NULL;
        while ((tag = NextTagItem(&tstate)) != NULL)
        {
            switch (tag->ti_Tag)
            {
                case WBOPENA_ArgLock:
                    lastLock = (BPTR) tag->ti_Data;
                    break;

                case WBOPENA_ArgName:
                    /*
                        Filter out args where both lock AND name are NULL,
                        since they are completely worthless to the application.
                    */
                    if (lastLock != NULL || (STRPTR) tag->ti_Data != NULL)
                    {
                        STRPTR name = (STRPTR) tag->ti_Data;

                        /* Duplicate the lock */
                        if (lastLock != NULL)
                        {
                            args[i].wa_Lock = DupLock(lastLock);

                            if (args[i].wa_Lock == NULL)
                            {
                                D(bug("workbench.library: WB_BuildArguments: Failed to duplicate lock!\n"));
                                goto error;
                                break;
                            }
                        }
                        else
                        {
                            args[i].wa_Lock = NULL;
                        }

                        /* Duplicate the name */
                        if (name != NULL)
                        {
                            args[i].wa_Name = StrDup(name);

                            if (args[i].wa_Name == NULL)
                            {
                                D(bug("workbench.library: WB_BuildArguments: Failed to duplicate string!\n"));
                                goto error;
                                break;
                            }
                        }
                        else
                        {
                            args[i].wa_Name = NULL;
                        }

                        i++;
                    }
                    break;
            }
        }

        return TRUE;
    }
    else
    {
        D(bug("workbench.library: WB_BuildArguments: Failed to allocate memory for argument array\n"));
    }

error:
    D(bug("workbench.library: WB_BuildArguments: Freeing resources after error...\n"));
    /* Free allocated resources */
    if (args != NULL)
    {
        int i;

        for (i = 0; i < startup->sm_NumArgs; i++)
        {
            if (args[i].wa_Lock != NULL) UnLock(args[i].wa_Lock);
            if (args[i].wa_Name != NULL) FreeVec(args[i].wa_Name);
        }

        FreeMem(args, sizeof(struct WBArg) * startup->sm_NumArgs);
    }

    startup->sm_NumArgs = 0;
    startup->sm_ArgList = NULL;

    return FALSE;
}

BOOL __WB_LaunchProgram
(
    BPTR lock, CONST_STRPTR name, struct TagItem *tags,
    struct WorkbenchBase *WorkbenchBase
)
{
    struct WBStartup        *startup = NULL;
    struct WBCommandMessage *message = NULL;

    /*-- Allocate memory for messages --------------------------------------*/
    startup = CreateWBS();
    if (startup == NULL)
    {
        D(bug("workbench.library: WB_LaunchProgram: Failed to allocate memory for startup message\n"));
        SetIoErr(ERROR_NO_FREE_STORE);
        goto error;
    }

    message = CreateWBCM(WBCM_TYPE_LAUNCH);
    if (message == NULL)
    {
        D(bug("workbench.library: WB_LaunchProgram: Failed to allocate memory for launch message\n"));
        SetIoErr(ERROR_NO_FREE_STORE);
        goto error;
    }

    /*-- Build the arguments array -----------------------------------------*/
    if (!WB_BuildArguments(startup, lock, name, tags))
    {
        D(bug("workbench.library: WB_LaunchProgram: Failed to build arguments\n"));
        goto error;
    }

    /*-- Send message to handler -------------------------------------------*/
    /* NOTE: The handler will deallocate all resources of this message! */
    message->wbcm_Data.Launch.Startup = startup;

    PutMsg(&(WorkbenchBase->wb_HandlerPort), (struct Message *) message);

    D(bug("workbench.library: WB_LaunchProgram: Success\n"));

    return TRUE;

error:
    if (startup != NULL) DestroyWBS(startup);
    if (message != NULL) DestroyWBCM(message);

    D(bug("workbench.library: WB_LaunchProgram: Failure\n"));

    return FALSE;
}
