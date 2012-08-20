#include <exec/libraries.h>

struct PertaskBase
{
    struct Library lib;
    int value;
};

APTR __aros_getbase();
