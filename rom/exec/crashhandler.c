#include <exec/alerts.h>
#include <proto/exec.h>

#include "etask.h"

VOID Exec_CrashHandler(void)
{
    ULONG alertNum;
    struct Task *task = FindTask(NULL);
    struct IntETask *iet = GetIntETask(task);
    
    /* Pick up the alert code given us by trap handler */
    alertNum = iet->iet_LastAlert[0];
    /* Remove supervisor-level crash indication (see traphandler.c) */
    iet->iet_LastAlert[0] = 0;
    
    Alert(alertNum);
}
