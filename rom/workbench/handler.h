#ifndef __WORKBENCH_HANDLER_H__
#define __WORKBENCH_HANDLER_H__

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Headers for the Workbench Handler.
*/

#include <aros/asmcall.h>
#include <exec/types.h>
#include <exec/ports.h>

/*** Messages ***************************************************************/
struct HandlerMessage
{
    struct Message hm_Message;
    ULONG          hm_Type;     /* see below */
};

#define HM_TYPE_UNKNOWN (0)
#define HM_TYPE_LAUNCH  (1)     /* launch a program */
#define HM_TYPE_DRAWER  (2)     /* open a drawer */

struct LaunchMessage
{
    struct HandlerMessage  lm_HandlerMessage;
    struct WBStartup      *lm_StartupMessage;
};

struct DrawerMessage
{
    struct HandlerMessage  dm_HandlerMessage;
    struct WBArg           dm_Drawer;
};

/*** Errors *****************************************************************/
#define HE_NONE         (0) /* no error */
#define HE_MSGPORT      (1) /* could not allocate message port */
#define HE_MEMORY       (2) /* could not allocate memory */
#define HE_LIBRARY      (3) /* could not open library */

/*** Prototypes *************************************************************/
AROS_UFP3
(
    LONG, WorkbenchHandler,
    AROS_UFPA(STRPTR,            args,       A0),
    AROS_UFPA(ULONG,             argsLength, D0),
    AROS_UFPA(struct ExecBase *, SysBase,    A6)
);

#endif /* __WORKBENCH_HANDLER_H__ */
