#include "pertaskbase.h"

void PertaskSetValue(int value)
{
    struct PertaskBase *PertaskBase = __GM_GetBase();

    PertaskBase->value = value;
}


int PertaskGetValue(void)
{
    struct PertaskBase *PertaskBase = __GM_GetBase();

    return PertaskBase->value;
}
