#ifndef ECHO_INTERN_H
#define ECHO_INTERN_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: echo.service - library base
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>

#define ECHO_SERVICE_NAME       "Echo"              /* SVCAttrs_Name                    */
#define ECHO_ENTITY_NAME        "Echo Service"      /* the public entity clients use    */

struct EchoBase
{
    struct Library          eb_Lib;
    struct SignalSemaphore  eb_Sem;
    struct Process         *eb_Server;              /* the one server process           */
    struct Task            *eb_Starter;             /* waits for the server to report   */
    ULONG                   eb_StartResult;         /* 0 = server failed to come up     */
    ULONG                   eb_Clients;             /* StartServiceA() calls so far     */
};

#endif /* ECHO_INTERN_H */
