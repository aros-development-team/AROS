#include <proto/peropener.h>
#include "peropenerbase.h"

void PeropenerSetValueStack(int value)
{
    struct PeropenerBase *PeropenerBase = (struct PeropenerBase *)__aros_getbase_PeropenerBase();

    PeropenerBase->value = value;
}


int PeropenerGetValueStack(void)
{
    struct PeropenerBase *PeropenerBase = (struct PeropenerBase *)__aros_getbase_PeropenerBase();

    return PeropenerBase->value;
}

AROS_LH1(void, PeropenerSetValueReg,
AROS_LHA(int, value, D0),
struct Library *, PeropenerBase, 7, Peropener)
{
    AROS_LIBFUNC_INIT

    struct PeropenerBase *PeropenerBase = (struct PeropenerBase *)__aros_getbase_PeropenerBase();

    PeropenerBase->value = value;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(int, PeropenerGetValueReg,
struct Library *, PeropenerBase, 8, Peropener)
{
    AROS_LIBFUNC_INIT

    struct PeropenerBase *PeropenerBase = (struct PeropenerBase *)__aros_getbase_PeropenerBase();

    return PeropenerBase->value;

    AROS_LIBFUNC_EXIT
}

