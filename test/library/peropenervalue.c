#include "peropenerbase.h"

void PeropenerSetValue(int value)
{
    struct PeropenerBase *PeropenerBase = __GM_GetBase();

    PeropenerBase->value = value;
}


int PeropenerGetValue(void)
{
    struct PeropenerBase *PeropenerBase = __GM_GetBase();

    return PeropenerBase->value;
}
