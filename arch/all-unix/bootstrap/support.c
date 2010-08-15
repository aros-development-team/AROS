#include <stdio.h>
#include <windows.h>

#include "support.h"

char *DefaultConfig = "boot/AROSBootstrap.conf";
OSVERSIONINFO winver;

char *getosversion(void)
{
    char *BootLoader_Name;

    uname(&sysinfo);
    nameparts[0] = sysinfo.sysname;
    nameparts[1] = sysinfo.machine;
    nameparts[2] = sysinfo.release;
    nameparts[3] = sysinfo.version;
    BootLoader_Name = join_string(4, nameparts);
    
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
