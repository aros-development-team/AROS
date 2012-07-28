#include <exec/libraries.h>

struct PeropenerBase
{
    struct Library lib;
    int value;
};

APTR __GM_GetBase();
