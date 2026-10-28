/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#define DEVNAME "serial.device"

static int failures;

static BOOL mount(CONST_STRPTR name, BPTR startup)
{
    struct DosList *aux, *dl;

    if (!(dl = MakeDosEntry(name, DLT_DEVICE)))
        return FALSE;

    aux = LockDosList(LDF_DEVICES | LDF_WRITE);
    if (!(aux = FindDosEntry(aux, "AUX", LDF_DEVICES)))
    {
        UnLockDosList(LDF_DEVICES | LDF_WRITE);
        FreeDosEntry(dl);
        return FALSE;
    }
    ((struct DeviceNode *)dl)->dn_Handler   = ((struct DeviceNode *)aux)->dn_Handler;
    ((struct DeviceNode *)dl)->dn_SegList   = ((struct DeviceNode *)aux)->dn_SegList;
    ((struct DeviceNode *)dl)->dn_StackSize = ((struct DeviceNode *)aux)->dn_StackSize;
    ((struct DeviceNode *)dl)->dn_Priority  = ((struct DeviceNode *)aux)->dn_Priority;
    ((struct DeviceNode *)dl)->dn_GlobalVec = (BPTR)-1;
    ((struct DeviceNode *)dl)->dn_Startup   = startup;
    AddDosEntry(dl);
    UnLockDosList(LDF_DEVICES | LDF_WRITE);

    return TRUE;
}

static void writeto(CONST_STRPTR path, const char *what)
{
    static const char text[] = "auxstartup: written through AUX\n";
    BPTR fh;
    LONG written;

    if (!(fh = Open(path, MODE_NEWFILE)))
    {
        printf("FAIL: %s: Open(\"%s\") error %ld\n", what, path, (long)IoErr());
        failures++;
        return;
    }
    written = Write(fh, text, sizeof(text) - 1);
    Close(fh);

    if (written != sizeof(text) - 1)
    {
        printf("FAIL: %s: Write() returned %ld\n", what, (long)written);
        failures++;
        return;
    }
    printf("PASS: %s\n", what);
}

int main(void)
{
    struct FileSysStartupMsg *fssm = NULL;
    STRPTR device;
    IPTR addr;

    for (addr = 0x7fff0000; addr >= 0x40000000; addr -= 0x10000)
        if ((fssm = AllocAbs(sizeof(*fssm), (APTR)addr)))
            break;
    device = AllocVec(AROS_BSTR_MEMSIZE4LEN(sizeof(DEVNAME)), MEMF_PUBLIC | MEMF_CLEAR);

    if (fssm && device)
    {
        memset(fssm, 0, sizeof(*fssm));
        CopyMem(DEVNAME, AROS_BSTR_ADDR(MKBADDR(device)), sizeof(DEVNAME) - 1);
        AROS_BSTR_setstrlen(MKBADDR(device), sizeof(DEVNAME) - 1);
        fssm->fssm_Device = MKBADDR(device);

        if (mount("TAUXHI", MKBADDR(fssm)))
            writeto("TAUXHI:", "startup at an address with bit 30 set");
        else
        {
            printf("FAIL: AUX: is not mounted\n");
            failures++;
        }
    }
    else
        printf("SKIP: no free memory with address bit 30 set\n");

    if (mount("TAUXDEF", BNULL))
        writeto("TAUXDEF:", "no startup, default device");
    else
    {
        printf("FAIL: AUX: is not mounted\n");
        failures++;
    }

    printf("%s\n", failures ? "AUXSTARTUP FAIL" : "AUXSTARTUP PASS");
    return failures ? RETURN_FAIL : RETURN_OK;
}
