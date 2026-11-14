/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <proto/dos.h>
#include "i2chid_driver.h"
int main(void)
{
    SIPTR args[4] = {0};
    struct RDArgs *rda = ReadArgs((CONST_STRPTR) "DEVICE/K,RUN/S,CHECK/S,REPORT/K", args, NULL);
    if (!rda)
    {
        return RETURN_FAIL;
    }
    int result =
        ih_driver_run((const char *)args[0], !!args[1], !!args[2], NULL, (const char *)args[3]);
    FreeArgs(rda);
    return result;
}
