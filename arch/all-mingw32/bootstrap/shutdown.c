#include <stdio.h>
#include <windows.h>

#include "hostlib.h"
#include "sharedmem.h"
#include "shutdown.h"

#define D(x)

static LPTSTR bootstrapname;
static LPTSTR cmdline;

#ifdef UNDER_CE
/* Windows CE has no notion of "current directory" */
#define bootstrapdir NULL

static LPTSTR StrConvert(const char *src)
{
    int len = strlen(src) + 1;
    LPTSTR res = malloc(len * 2);

    if (res)
    {
        if (!MultiByteToWideChar(CP_ACP, 0, src, len, res, len))
        {
            free(res);
            return NULL;
        }
    }
    return NULL;
}
#else
#define StrConvert(x) x
#endif

/* Remember our launch context (bootstrap name and command line) */
void SaveArgs(char **argv)
{
    bootstrapname = StrConvert(argv[0]);
    cmdline       = GetCommandLine();
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
