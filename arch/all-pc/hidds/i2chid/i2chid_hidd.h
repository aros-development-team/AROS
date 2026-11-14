/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef I2CHID_HIDD_H
#define I2CHID_HIDD_H
#include <exec/libraries.h>
#include <exec/tasks.h>
#include <oop/oop.h>
#include <hidd/mouse.h>

struct i2chid_base
{
    struct Library library;
    struct Library *oop;
    OOP_Class *mouseclass;
    OOP_Object *mousehw, *mouse, *registration;
    OOP_AttrBase mouseAB;
    OOP_MethodID hwBase, inputBase;
    const char *status;
    ULONG probes;
};
struct i2chid_mouse
{
    struct i2chid_base *base;
    OOP_Object *self;
    struct Task *worker, *waiter;
    ULONG signal;
    BOOL ready, stopping;
};
#endif
