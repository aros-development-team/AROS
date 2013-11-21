#include <stdio.h>
#include <windows.h>

#include "support.h"

char *DefaultConfig = ARCH "\\AROSBootstrap.conf";
OSVERSIONINFO winver;

#ifdef _UNICODE
#define VERSION_FORMAT "%s / Windows %lu.%lu build %lu %S"
#else
#define VERSION_FORMAT "%s / Windows %lu.%lu build %lu %s"
#endif

char *getosversion(const char *bsver)
{
    static char SystemVersion[512];

    winver.dwOSVersionInfoSize = sizeof(winver);
    GetVersionEx(&winver);
    sprintf(SystemVersion, VERSION_FORMAT, bsver, winver.dwMajorVersion, winver.dwMinorVersion, winver.dwBuildNumber, winver.szCSDVersion);

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

#ifdef _UNICODE

LPTSTR StrConvert(const char *src)
{
    int len = strlen(src) + 1;
    LPTSTR res = malloc(len * 2);

    if (res)
    {
        if (!MultiByteToWideChar(CP_ACP, 0, src, -1, res, len))
        {
            free(res);
            return NULL;
        }
    }
    return res;
}

#endif
