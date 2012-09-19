#include <proto/peropener.h>
#include "peropenerbase.h"

void PeropenerSetValue(int value)
{
    struct PeropenerBase *PeropenerBase = (struct PeropenerBase *)__aros_getbase_PeropenerBase();

    PeropenerBase->value = value;
}


int PeropenerGetValue(void)
{
    struct PeropenerBase *PeropenerBase = (struct PeropenerBase *)__aros_getbase_PeropenerBase();

    return PeropenerBase->value;
}
