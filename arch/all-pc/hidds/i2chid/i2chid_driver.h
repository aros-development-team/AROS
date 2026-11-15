/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef I2CHID_DRIVER_H
#define I2CHID_DRIVER_H
#define IH_MOTION 0
#define IH_PRESS 1
#define IH_RELEASE 2
struct ih_sink
{
    void *context;
    int (*event)(void *, int, int, unsigned, unsigned);
    /* Called after hardware initialization. Zero requests cleanup. */
    int (*ready)(void *);
};
int ih_driver_run(const char *, int, int, const struct ih_sink *, const char *);
#endif
