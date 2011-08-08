#include <stdio.h>
#include <windows.h>

#include "support.h"

char *DefaultConfig = "boot\\AROSBootstrap.conf";
OSVERSIONINFO winver;

char *getosversion(const char *bsver)
{
    static char SystemVersion[512];

    winver.dwOSVersionInfoSize = sizeof(winver);
    GetVersionEx(&winver);
    sprintf(SystemVersion, "%s / Windows %lu.%lu build %lu %s", bsver, winver.dwMajorVersion, winver.dwMinorVersion, winver.dwBuildNumber, winver.szCSDVersion);
    
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
