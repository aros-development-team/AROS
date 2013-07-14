/*
 * Copyright (C) 2013, Netronome Systems, Inc.
 * All right reserved.
 *
 */
#include <aros/shcommands.h>

#include <proto/dos.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

struct {
    CONST_STRPTR value;
    BOOL result;
} test[] = {
    { "", TRUE },
    { "isfilesystem", TRUE },
    { "*", FALSE },
    { "CONSOLE:", FALSE },
    { "CON:", FALSE },
    { "RAW:", FALSE },
    { "PIPE:", FALSE },
    { "SYS:", TRUE },
    { "RAM:", TRUE },
    { "NIL:", FALSE },
};

AROS_SH0(isfilesystem, 0.0)
{
    AROS_SHCOMMAND_INIT
    BOOL failed = FALSE;
    int i;

    for (i = 0; i < ARRAY_SIZE(test); i++) {
        BOOL res;
        res = IsFileSystem(test[i].value) ? TRUE : FALSE;
        Printf("IsFileSystem(\"%s\") = %s", test[i].value, (test[i].result == res) ? "ok" : "FAIL");
        if (res != test[i].result) {
            Printf(" (expected %s, got %s)\n", test[i].result ? "TRUE" : "FALSE", res ? "TRUE" : "FALSE");
            failed = TRUE;
        } else
            Printf("\n");
    }


    return failed ? RETURN_FAIL : RETURN_OK;
    AROS_SHCOMMAND_EXIT
}

