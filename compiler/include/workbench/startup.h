#ifndef WORKBENCH_STARTUP_H
#define WORKBENCH_STARTUP_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Workbench startup handling
    Lang: english
*/

#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

struct WBStartup
{
    struct Message   sm_Message;
    struct MsgPort * sm_Process;
    BPTR	     sm_Segment;
    LONG	     sm_NumArgs;
    char	   * sm_ToolWindow;
    struct WBArg   * sm_ArgList;
};

struct WBArg
{
    BPTR   wa_Lock;
    BYTE * wa_Name;
};

#endif /* WORKBENCH_STARTUP_H */
