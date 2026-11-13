#ifndef WORKBENCH_HANDLER_H
#define WORKBENCH_HANDLER_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved. 
    $Id$
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <workbench/startup.h>

enum WBHM_Type
{
    WBHM_TYPE_SHOW,   /* Open all windows */
    WBHM_TYPE_HIDE,   /* Close all windows */
    WBHM_TYPE_OPEN,   /* Open a drawer */
    WBHM_TYPE_UPDATE  /* Update an object */
};

struct WBHandlerMessage
{
    struct Message    wbhm_Message;
    enum   WBHM_Type  wbhm_Type;       /* Type of message (see above) */
 
    union
    {
        struct
        {
            CONST_STRPTR      Name;    /* Name of drawer */
        } Open;
       
        struct
        {
            CONST_STRPTR      Name;    /* Name of object */
            LONG              Type;    /* Type of object (WBDRAWER, WBPROJECT, ...) */
        } Update;
    } wbhm_Data;
};

#define WBHM_SIZE (sizeof(struct WBHandlerMessage))
#define WBHM(msg) ((struct WBHandlerMessage *) (msg))

#endif /* WORKBENCH_HANDLER_H */
