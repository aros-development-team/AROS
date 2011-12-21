#include <stdio.h>
#include <windows.h>

#include "hostlib.h"
#include "sharedmem.h"
#include "shutdown.h"

#define D(x)

/* Windows CE has no notion of "current directory" */
#ifdef UNDER_CE
#define bootstrapdir NULL
#endif

static LPTSTR bootstrapname;
static LPTSTR cmdline;

void SaveArgs(char **argv)
{
    /* FIXME: Adapt to WinCE by converting argv[0] to Unicode */
    bootstrapname = argv[0];
    cmdline = GetCommandLine();
}

void __aros Host_Shutdown(unsigned char warm)
{
    STARTUPINFO runinfo;
    PROCESS_INFORMATION ProcInfo;
#ifndef UNDER_CE
    char var[SHARED_RAM_LEN];

    if (warm)
        sprintf(var, "%s=%p:%p", SHARED_RAM_VAR, RAM_Handle, RAM_Address);
    else
        strcpy(var, SHARED_RAM_VAR "=");

    D(printf("[Shutdown] Dir: %s, Name: %s, RAM: %s, Command line: %s\n", bootstrapdir, bootstrapname, var, cmdline));
    putenv(var);
    SetCurrentDirectory(bootstrapdir);
#endif
    FillMemory(&runinfo, sizeof(runinfo), 0);
    runinfo.cb = sizeof(runinfo);

    /*
     * If we create new process without CREATE_NEW_CONSOLE, strange thing will happen if we start AROS
     * from within command line processor. Looks like it's Windows bug/misdesign. Well, let's reopen the console every time.
     */
    if (CreateProcess(bootstrapname, cmdline, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, bootstrapdir, &runinfo, &ProcInfo))
    {
        D(printf("[Shutdown] AROS re-run\n"));

        CloseHandle(ProcInfo.hProcess);
        CloseHandle(ProcInfo.hThread);
        exit(0);
    }

    D(printf("[Shutdown] Unable to re-run AROS\n"));
}
