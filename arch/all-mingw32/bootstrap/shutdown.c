#include <stdio.h>
#include <windows.h>

#include "sharedmem.h"
#include "shutdown.h"

#define D(x)

static char *bootstrapname;
static char *cmdline;

void SaveArgs(char **argv)
{
    bootstrapname = argv[0];
    cmdline = GetCommandLine();
}

static void Restart(char *var)
{
    STARTUPINFO runinfo;
    PROCESS_INFORMATION ProcInfo;

    D(printf("[Restart] Dir: %s, Name: %s, RAM: %s, Command line: %s\n", bootstrapdir, bootstrapname, var, cmdline));
    putenv(var);
    SetCurrentDirectory(bootstrapdir);
    FillMemory(&runinfo, sizeof(runinfo), 0);
    runinfo.cb = sizeof(runinfo);

    /* If we create new process without CREATE_NEW_CONSOLE, strange thing will happen if we start AROS
     * from within command line processor. Looks like it's Windows bug/misdesign. Well, let's reopen the console every time. */
    if (CreateProcess(bootstrapname, cmdline, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, bootstrapdir, &runinfo, &ProcInfo))
    {
        D(printf("[Shutdown] AROS re-run\n"));   
        CloseHandle(ProcInfo.hProcess);
        CloseHandle(ProcInfo.hThread);
        exit(0);
    }
    D(printf("[Shutdown] Unable to re-run AROS\n"));
}

void Host_Shutdown(unsigned long action)
{
    char buf[SHARED_RAM_LEN];

    switch (action) {
    case SD_ACTION_POWEROFF:
        D(printf("[Shutdown] POWER OFF request\n"));
        exit(0);
    	break;

    case SD_ACTION_WARMREBOOT:
	sprintf(buf, "%s=%lx:%lx", SHARED_RAM_VAR, RAM_Handle, RAM_Address);
	Restart(buf);
	break;

    case SD_ACTION_COLDREBOOT:
	Restart(SHARED_RAM_VAR "=");
	break;
    }
}
