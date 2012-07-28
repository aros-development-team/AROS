/*
 * This function is in the static link lib.
 * It uses the genmodule provided Pertask_GetLibbase() function so it can be
 * used both in a library that uses pertask_rel.a or a program that just uses
 * pertask.a.
 * It does not call a function in pertask.library so that a good optimizing
 * compiler with link time function inlining to optimize this very good.
 */
#include "pertaskbase.h"

struct PertaskBase *Pertask_GetLibbase(void);

int *__pertask_getvalueptr(void)
{
    return &(Pertask_GetLibbase()->value);
}
