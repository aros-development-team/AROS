#include <stdio.h>
#include <windows.h>

#include "support.h"

char *DefaultConfig = "boot\\AROSBootstrap.conf";
OSVERSIONINFO winver;

char *getosversion(void)
{
    static char SystemVersion[256];

    winver.dwOSVersionInfoSize = sizeof(winver);
    GetVersionEx(&winver);
    sprintf(SystemVersion, "Windows %lu.%lu build %lu %s", winver.dwMajorVersion, winver.dwMinorVersion, winver.dwBuildNumber, winver.szCSDVersion);
    
    return SystemVersion;
}

char *namepart(char *name)
{
    while (*name)
	name++;

    while((name[-1] != ':') && (name[-1] != '\\') && (name[-1] != '/'))
	name--;

    return name;
}
