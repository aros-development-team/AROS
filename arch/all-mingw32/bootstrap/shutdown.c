#include <stdlib.h>
#include <windows.h>
#include "shutdown.h"

#define D(x)

void Host_Shutdown(unsigned long action)
{
    STARTUPINFO runinfo;
    PROCESS_INFORMATION ProcInfo;

    switch (action) {
    case SD_ACTION_POWEROFF:
        D(printf("[Shutdown] POWER OFF request\n"));
        exit(0);
    	break;
    case SD_ACTION_COLDREBOOT:
        D(printf("[Shutdown] Cold reboot, dir: %s, name: %s, command line: %s\n", bootstrapdir, bootstrapname, cmdline));
    	FillMemory(&runinfo, sizeof(runinfo), 0);
        runinfo.cb = sizeof(runinfo);
        /* If we create new process without CREATE_NEW_CONSOLE, strange thing will happen if we start AROS
         * from within command line processor. Looks like it's Windows bug/misdesign. Well, let's reopen the console every time. */
        if (CreateProcess(bootstrapname, cmdline, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, bootstrapdir, &runinfo, &ProcInfo)) {
            D(printf("[Shutdown] AROS re-run\n"));   
            CloseHandle(ProcInfo.hProcess);
            CloseHandle(ProcInfo.hThread);
            exit(0);
        }
        D(printf("[Shutdown] Unable to re-run AROS\n"));
    }
}
