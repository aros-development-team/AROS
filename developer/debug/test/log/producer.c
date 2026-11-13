/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/log.h>

#include <resources/log.h>
#include <exec/ports.h>

#include <stdio.h>

#include "logtest.h"

struct Library *LogResBase = NULL;

int main(void)
{
    LogResBase = OpenResource("log.resource");
    if (LogResBase)
    {
        APTR LogRHandle;
        struct LogProviderNode testProvider;
        testProvider.lpn_Node.ln_Name = LOGTEST_COMPONENT;

        LogRHandle = logInitialise(&testProvider);

        printf("creating log.resource entries...\n");
#define LOGLVL_UNUSED 20  
        logAddEntry((LOGF_Flag_Type_Debug | LOGLVL_UNUSED),
            LogRHandle, "Test", __func__, 0,
            "Test Debug Entry\nThis is test entry #%u\nYou can safely ignore this entry", 1);
        logAddEntry((LOGF_Flag_Type_Information | LOGLVL_UNUSED),
            LogRHandle, "Test", __func__, 0,
            "Test Information %s\nYou can safely ignore this entry", "Entry");
        logAddEntry((LOGF_Flag_Type_Warn | LOGLVL_UNUSED),
            LogRHandle, "Test", __func__, 0,
            "Test Warning Entry\nYou can safely ignore this entry");
        logAddEntry((LOGF_Flag_Type_Error | LOGLVL_UNUSED),
            LogRHandle, "Test", __func__, 0,
            "Test Error Entry\nYou can safely ignore this entry");
        printf("entries created\n");
    }

    return 20;
}
