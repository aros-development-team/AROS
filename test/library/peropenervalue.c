#include "peropenerbase.h"

void PeropenerSetValue(int value)
{
    struct PeropenerBase *PeropenerBase = __aros_getbase();

    PeropenerBase->value = value;
}


int PeropenerGetValue(void)
{
    struct PeropenerBase *PeropenerBase = __aros_getbase();

    return PeropenerBase->value;
}
