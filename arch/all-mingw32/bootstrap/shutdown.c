#include <stdio.h>
#include <windows.h>

#include "hostlib.h"
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

void __aros Host_Shutdown(unsigned char warm)
{
    char var[SHARED_RAM_LEN];
    STARTUPINFO runinfo;
    PROCESS_INFORMATION ProcInfo;

    if (warm)
	sprintf(var, "%s=%p:%p", SHARED_RAM_VAR, RAM_Handle, RAM_Address);
    else
	strcpy(var, SHARED_RAM_VAR "=");

    D(printf("[Shutdown] Dir: %s, Name: %s, RAM: %s, Command line: %s\n", bootstrapdir, bootstrapname, var, cmdline));
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

