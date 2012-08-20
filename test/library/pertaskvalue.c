#include "pertaskbase.h"

void PertaskSetValue(int value)
{
    struct PertaskBase *PertaskBase = __aros_getbase();

    PertaskBase->value = value;
}


int PertaskGetValue(void)
{
    struct PertaskBase *PertaskBase = __aros_getbase();

    return PertaskBase->value;
}
