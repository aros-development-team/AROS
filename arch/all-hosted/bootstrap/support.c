#include "support.h"

char *DefaultConfig = "boot/AROSBootstrap.conf";

char *getosversion(const char *version)
{
    char *parts[] =
    {
        version,
        "/",
        "Unknown operating system"
    }
    
    return join_strings(3, parts);
}

char *namepart(char *name)
{
    while (*name)
	name++;

    while((name[-1] != ':') && (name[-1] != '\\') && (name[-1] != '/'))
	name--;

    return name;
}
