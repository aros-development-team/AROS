#include <exec/alerts.h>
#include <proto/exec.h>

#include "etask.h"

VOID Exec_CrashHandler(void)
{
    struct Task *task = FindTask(NULL);
    struct IntETask *iet = GetIntETask(task);
    ULONG alertNum = iet->iet_AlertCode;

    iet->iet_AlertCode = 0;	/* Makes Alert() attempting to bring up Intuition requester */
    Alert(alertNum);
}
