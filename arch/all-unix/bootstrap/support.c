#include <stdio.h>
#include <sys/utsname.h>

#include "support.h"

char *DefaultConfig = ARCH "/AROSBootstrap.conf";

char *getosversion(const char *bsversion)
{
    char *BootLoader_Name;
    struct utsname sysinfo;
    char *nameparts[6];

    uname(&sysinfo);
    nameparts[0] = (char *)bsversion;
    nameparts[1] = "/";
    nameparts[2] = sysinfo.sysname;
    nameparts[3] = sysinfo.machine;
    nameparts[4] = sysinfo.release;
    nameparts[5] = sysinfo.version;
    BootLoader_Name = join_string(6, nameparts);

    return BootLoader_Name;
}

char *namepart(char *name)
{
    while (*name)
	name++;

    while (name[-1] != '/')
	name--;

    return name;
}
