#ifndef ___EXITFUNC_H
#define ___EXITFUNC_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/lists.h>
#include <exec/nodes.h>

struct AtExitNode
{
    struct Node node;
    union
    {
        void (*fn)(void);
        struct
        {
            void (*fn)(int status, void *arg);
            void *arg;
        } on;
        struct
        {
            void (*fn)(void *arg, int status);
            void *arg;
            void *dsoh;
        } cxa;
    } func;
};

#define AEN_VOID 0
#define AEN_ON 1
#define AEN_CXA 2

int __addexitfunc(struct AtExitNode *aen);
void __callexitfuncs(void);

int __add_quick_exit_func(struct AtExitNode *aen);
void __call_quick_exit_funcs(void);

#endif
