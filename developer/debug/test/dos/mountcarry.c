/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

static int failures;

static BOOL writefile(CONST_STRPTR name, CONST_STRPTR text)
{
    BPTR fh = Open(name, MODE_NEWFILE);
    LONG len = strlen(text);
    BOOL ok;

    if (!fh)
        return FALSE;
    ok = Write(fh, text, len) == len;
    Close(fh);
    return ok;
}

static void checkdevice(CONST_STRPTR name, BOOL wantstarted, SIPTR wantstartup)
{
    struct DosList *dl;
    BOOL found = FALSE, started = FALSE;
    SIPTR startup = 0;
    char what[80];

    dl = LockDosList(LDF_DEVICES | LDF_READ);
    if ((dl = FindDosEntry(dl, name, LDF_DEVICES)))
    {
        found = TRUE;
        started = dl->dol_Task != NULL;
        startup = (SIPTR)((struct DeviceNode *)dl)->dn_Startup;
    }
    UnLockDosList(LDF_DEVICES | LDF_READ);

    snprintf(what, sizeof(what), "%s: mounted", name);
    printf("%s: %s\n", found ? "PASS" : "FAIL", what);
    if (!found)
    {
        failures++;
        return;
    }

    snprintf(what, sizeof(what), "%s: startup %ld (want %ld)", name, (long)startup, (long)wantstartup);
    printf("%s: %s\n", startup == wantstartup ? "PASS" : "FAIL", what);
    if (startup != wantstartup)
        failures++;

    snprintf(what, sizeof(what), "%s: %s (want %s)", name,
        started ? "started" : "not started", wantstarted ? "started" : "not started");
    printf("%s: %s\n", started == wantstarted ? "PASS" : "FAIL", what);
    if (started != wantstarted)
        failures++;
}

int main(void)
{
    BPTR dir;

    dir = CreateDir("T:mountcarry");
    if (dir)
        UnLock(dir);

    if (!writefile("T:mountcarry/CARRYA",
                   "Handler = L:pipe-handler\nStartup = 1\nActivate = 1\n") ||
        !writefile("T:mountcarry/CARRYB",
                   "Handler = L:pipe-handler\n") ||
        !writefile("T:mountcarry.list",
                   "CARRYC:\n Handler = L:pipe-handler\n Startup = 1\n Activate = 1\n#\n"
                   "CARRYD:\n Handler = L:pipe-handler\n#\n"))
    {
        printf("FAIL: cannot write the mountlists to T:\n");
        return RETURN_FAIL;
    }

    if (SystemTags("Mount T:mountcarry/CARRY#?", TAG_DONE) != 0 ||
        SystemTags("Mount CARRYC: CARRYD: FROM T:mountcarry.list", TAG_DONE) != 0)
        printf("WARN: Mount returned an error\n");

    checkdevice("CARRYA", TRUE, 1);
    checkdevice("CARRYB", FALSE, 0);
    checkdevice("CARRYC", TRUE, 1);
    checkdevice("CARRYD", FALSE, 0);

    printf("%s\n", failures ? "MOUNTCARRY FAIL" : "MOUNTCARRY PASS");
    return failures ? RETURN_FAIL : RETURN_OK;
}
