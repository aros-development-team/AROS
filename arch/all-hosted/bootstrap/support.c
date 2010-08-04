#include "support.h"

char *DefaultConfig = "boot/AROSBootstrap.conf";

char *getosversion(void)
{   
    return "Unknown operating system";
}

char *namepart(char *name)
{
    while (*name)
	name++;

    while((name[-1] != ':') && (name[-1] != '\\') && (name[-1] != '/'))
	name--;

    return name;
}
