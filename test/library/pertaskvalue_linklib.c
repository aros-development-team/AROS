/*
 * This function is in the static link lib.
 * It uses the genmodule provided __GM_GetBase_PertaskBase() function so it can be
 * used both in a library that uses pertask_rel.a or a program that just uses
 * pertask.a.
 * It does not call a function in pertask.library so that a good optimizing
 * compiler with link time function inlining can optimize this well.
 */
#include "pertaskbase.h"

struct PertaskBase *__aros_getbase_PertaskBase(void);

int *__pertask_getvalueptr(void)
{
    return &(__aros_getbase_PertaskBase()->value);
}
