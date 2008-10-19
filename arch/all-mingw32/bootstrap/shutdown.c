#include <windows.h>
#include "shutdown.h"

#define D(x) x

void Host_Shutdown(unsigned long action)
{
    STARTUPINFO runinfo;
    PROCESS_INFORMATION ProcInfo;

    switch (action) {
    case SD_ACTION_POWEROFF:
        D(printf("[Shutdown] POWER OFF request\n"));
        ExitProcess(0);
    	break;
    case SD_ACTION_COLDREBOOT:
        D(printf("[Shutdown] Cold reboot, dir: %s, name: %s, command line: %s\n", bootstrapdir, bootstrapname, cmdline));
    	FillMemory(&runinfo, sizeof(runinfo), 0);
        runinfo.cb = sizeof(runinfo);
        if (CreateProcess(bootstrapname, cmdline, NULL, NULL, FALSE, 0, NULL, bootstrapdir, &runinfo, &ProcInfo)) {
            D(printf("[Shutdown] AROS re-run\n"));   
            CloseHandle(ProcInfo.hProcess);
            CloseHandle(ProcInfo.hThread);
            ExitProcess(0);
        }
        D(printf("[Shutdown] Unable to re-run AROS\n"));
    }
}
