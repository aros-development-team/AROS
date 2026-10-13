/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: minimal, non-interactive filesystem formatter for test scripts:
          inhibits the device and sends ACTION_FORMAT to its handler.

          FSformat DRIVE/A,NAME/A,DOSTYPE/K   (DOSTYPE as hex, e.g. 0x50465301)
*/

#include <aros/debug.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
    struct RDArgs *rda;
    IPTR args[3] = { 0, 0, 0 };
    LONG rc = RETURN_FAIL;

    if (!(rda = ReadArgs("DRIVE/A,NAME/A,DOSTYPE/K", args, NULL)))
    {
        PrintFault(IoErr(), "FSformat");
        return RETURN_FAIL;
    }
    else
    {
        STRPTR drive = (STRPTR)args[0];
        STRPTR name = (STRPTR)args[1];
        ULONG dostype = args[2] ? (ULONG)strtoul((char *)args[2], NULL, 0) : 0;
        struct DosList *dl;
        struct MsgPort *port = NULL;
        UBYTE devname[64];
        int len = strlen(drive);

        strncpy(devname, drive, sizeof(devname) - 1);
        devname[sizeof(devname) - 1] = '\0';
        if (len && devname[len - 1] == ':')
            devname[len - 1] = '\0';

        {
            /* GetDeviceProc() starts the handler if it is not running yet
             * (a device node's dol_Task is NULL until first use) */
            struct DevProc *dvp = GetDeviceProc(drive, NULL);

            if (dvp)
            {
                port = dvp->dvp_Port;
                FreeDeviceProc(dvp);
            }
            else
            {
                struct DosList *head = LockDosList(LDF_DEVICES | LDF_READ);
                if ((dl = FindDosEntry(head, devname, LDF_DEVICES)))
                    port = dl->dol_Task;
                UnLockDosList(LDF_DEVICES | LDF_READ);
            }
        }

        if (!port)
            Printf("FSformat: unknown device %s\n", drive);
        else if (!Inhibit(drive, DOSTRUE))
            PrintFault(IoErr(), "FSformat: Inhibit");
        else
        {
            UBYTE bname[64];
            BSTR bstr = MKBADDR(bname);
            ULONG nlen = strlen(name);
            SIPTR res;

            /* build the BSTR the way this port lays them out (length byte or
             * plain C string, see <dos/bptr.h>) */
            if (nlen > 60) nlen = 60;
            CopyMem(name, AROS_BSTR_ADDR(bstr), nlen);
            AROS_BSTR_setstrlen(bstr, nlen);

            res = DoPkt(port, ACTION_FORMAT, (SIPTR)bstr, (SIPTR)dostype, 0, 0, 0);
            if (res == DOSFALSE)
                PrintFault(IoErr(), "FSformat: ACTION_FORMAT");
            else
            {
                Printf("FSformat: %s formatted as '%s'\n", drive, name);
                rc = RETURN_OK;
            }
            Inhibit(drive, DOSFALSE);
        }
        FreeArgs(rda);
    }
    return rc;
}
