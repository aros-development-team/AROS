#include <exec/libraries.h>

struct PertaskBase
{
    struct Library lib;
    int value;
};

APTR __GM_GetBase();
