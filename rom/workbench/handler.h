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

#include <workbench/handler.h>

/*** Messages ***************************************************************/
enum WBCM_Type
{
    WBCM_TYPE_LAUNCH,  /* Launch a program */
    WBCM_TYPE_RELAY    /* Relay a message to the workbench application */
};

struct WBCommandMessage
{
    struct Message wbcm_Message;
    enum WBCM_Type wbcm_Type;
    
    union
    {
        struct
        {
            struct WBStartup        *Startup;
        } Launch;
        
        struct
        {
            struct WBHandlerMessage *Message;
        } Relay;
    } wbcm_Data;
    struct TagItem *wbcm_Tags;
};

#define WBCM_SIZE (sizeof(struct WBCommandMessage))
#define WBCM(msg) ((struct WBCommandMessage *) (msg))

/*** Prototypes *************************************************************/
AROS_UFP3
(
    LONG, WorkbenchHandler,
    AROS_UFPA(STRPTR,            args,       A0),
    AROS_UFPA(ULONG,             argsLength, D0),
    AROS_UFPA(struct ExecBase *, SysBase,    A6)
);

#endif /* __WORKBENCH_HANDLER_H__ */
