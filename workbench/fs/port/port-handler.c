/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <string.h>

#include <aros/debug.h>

#include <intuition/preferences.h>      /* DEVNAME_SIZE */
#include <devices/serial.h>
#include <devices/printer.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#define NT_PORTARGS     (NT_USER - 1)

#define UtilityBase     (pb->pb_UtilityBase)

struct portBase {
    TEXT  pb_DeviceName[DEVNAME_SIZE];
    LONG  pb_DeviceFlags;
    enum { PORT_SERIAL, PORT_PARALLEL, PORT_PRINTER, PORT_STREAM } pb_Mode;
    struct Library *pb_UtilityBase;

    /* Per-open settable arguments */
    struct portArgs {
        struct Node pa_Node;
        IPTR  pa_DeviceUnit;
        union {
            struct {
                ULONG ps_Baud;
                UBYTE ps_LenBits;
                UBYTE ps_StopBits;
                UBYTE ps_SerFlags;
                UBYTE ps_ExtFlags;
            } pa_Serial;
            struct {
                enum { PRT_COOKED, PRT_RAW, PRT_TRANSPARENT } pr_Type;
            } pa_Printer;
        };
        
        /* Only used in the per-open instances */
        struct MsgPort *pa_IOMsg;
        union {
            struct IOStdReq std;
            struct IOExtSer ser;
        } *pa_IO;
    } pb_Defaults;

    struct List pb_Files;
};

/* Decode the following flags:
 * Any type:
 *  UNIT=n
 * PORT_PRINTER:
 *  TRANSPARENT
 *  RAW
 * PORT_SERIAL:
 *  9600 (and other baud rates)
 *  [78][NOEMS][12]
 */
static SIPTR decodeArgs(struct portBase *pb, struct portArgs *pa, BSTR args)
{
    int loc, len = AROS_BSTR_strlen(args);
    CONST_STRPTR cp = AROS_BSTR_ADDR(args);

    /* Skip any VOL: prefix */
    for (loc = 0; loc < len; loc++) {
        if (cp[loc] == ':')
            break;
    }
    if (loc < len)
        loc++;
    else
        loc=0;

    while (loc < len) {
        int slen;       /* length of section */

        /* Advance to next section */
        cp = &cp[loc];
        len -= loc;

        /* Find next section */
        for (loc = 0; loc < len; loc++) {
            if (cp[loc] == '/')
                break;
        }
        slen = loc;

        if (loc < len)
            loc++;

        /* Check for matches.. */
        if (slen >= 5 && (0 == Strnicmp(cp, "UNIT=", 5))) {
            CONST_STRPTR unit = cp + 5;
            int ulen = slen - 5;
            int i;
            SIPTR uval = 0;

            for (i = 0; i < ulen; i++) {
                if (unit[i] < '0' || unit[i] > '9')
                    break;
                uval *= 10;
                uval += unit[i] - '0';
            }

            if (i == ulen)
                pa->pa_DeviceUnit = uval;

        } else if (pb->pb_Mode == PORT_PARALLEL) {
            /* No parallel.device options to process */
        } else if (pb->pb_Mode == PORT_PRINTER) {
            if (slen == 3 && Strnicmp(cp, "RAW", 3) == 0) {
                pa->pa_Printer.pr_Type = PRT_RAW;
            } else if (slen == 11 && Strnicmp(cp, "TRANSPARENT", 11) == 0) {
                pa->pa_Printer.pr_Type = PRT_TRANSPARENT;
            } 
        } else if (pb->pb_Mode == PORT_SERIAL) {
            int i;
            ULONG baud = 0;

            /* Check for all-numeric */
            for (i = 0; i < slen; i++) {
                if (cp[i] < '0' || cp[i] > '9')
                    break;
                baud *= 10;
                baud += cp[i] - '0';
            }
            if (i == slen) {
                pa->pa_Serial.ps_Baud = baud;
            } else if (slen == 3) {
                TEXT bits, mode, stop;
                bits = cp[0];
                mode = ToUpper(cp[1]);
                stop = cp[2];

                if ((bits == '7' || bits == '8') &&
                    (mode == 'N' || mode == 'O' || mode == 'E' ||
                                    mode == 'M' || mode == 'S') &&
                    (stop == '1' || stop == '2')) {
                    pa->pa_Serial.ps_StopBits = stop - '0';
                    pa->pa_Serial.ps_LenBits  = bits - '0';
                    switch (mode) {
                    case 'N': pa->pa_Serial.ps_SerFlags = 0;
                              pa->pa_Serial.ps_ExtFlags = 0;
                              break;
                    case 'O': pa->pa_Serial.ps_SerFlags = SERF_PARTY_ON | SERF_PARTY_ODD;
                              pa->pa_Serial.ps_ExtFlags = 0;
                              break;
                    case 'E': pa->pa_Serial.ps_SerFlags = SERF_PARTY_ON;
                              pa->pa_Serial.ps_ExtFlags = 0;
                              break;
                    case 'M': pa->pa_Serial.ps_SerFlags = SERF_PARTY_ON;
                              pa->pa_Serial.ps_ExtFlags = SEXTF_MSPON | SEXTF_MARK;
                              break;
                    case 'S': pa->pa_Serial.ps_SerFlags = SERF_PARTY_ON;
                              pa->pa_Serial.ps_ExtFlags = SEXTF_MSPON;
                              break;
                    }
                }
            }
        }
    }


    return RETURN_OK;
}

static void portSerialDefaults(struct portArgs *pa)
{
    /* 9600, 8N1 */
    pa->pa_Serial.ps_StopBits = 1;
    pa->pa_Serial.ps_Baud = 9600;
    pa->pa_Serial.ps_SerFlags = 0;
    pa->pa_Serial.ps_ExtFlags = 0;
}

/* Decode the startup message
 */
static SIPTR decodeStartup(struct portBase *pb, BPTR startup)
{
    int len;
    struct FileSysStartupMsg *fssm;
    struct DosEnvec *env;
    struct portArgs *pa = &pb->pb_Defaults;
    SIPTR ret = RETURN_OK;

    switch ((SIPTR)startup) {
    case 0:
        pb->pb_Mode = PORT_SERIAL;
        strcpy(pb->pb_DeviceName, "serial.device");
        pa->pa_DeviceUnit = 0;
        pb->pb_DeviceFlags = 0;
        portSerialDefaults(pa);
        break;
    case 1:
        pb->pb_Mode = PORT_PARALLEL;
        strcpy(pb->pb_DeviceName, "parallel.device");
        pa->pa_DeviceUnit = 0;
        pb->pb_DeviceFlags = 0;
        break;
    case 2:
        pb->pb_Mode = PORT_PRINTER;
        pa->pa_Printer.pr_Type = PRT_COOKED;
        strcpy(pb->pb_DeviceName, "printer.device");
        pa->pa_DeviceUnit = 0;
        pb->pb_DeviceFlags = 0;
        break;
    default:
        fssm = BADDR(startup);
        if (fssm->fssm_Device == BNULL) {
            ret = ERROR_NO_DISK;
            break;
        }

        len = AROS_BSTR_strlen(fssm->fssm_Device);
        if (len > sizeof(pb->pb_DeviceName)-1) {
            ret = ERROR_NO_DISK;
            break;
        }

        CopyMem(AROS_BSTR_ADDR(fssm->fssm_Device), pb->pb_DeviceName, len);
        pb->pb_DeviceName[len] = 0;

        if (0 == Stricmp(pb->pb_DeviceName, "serial.device")) {
            pb->pb_Mode = PORT_SERIAL;
            portSerialDefaults(pa);
        } else if (0 == Stricmp(pb->pb_DeviceName, "parallel.device")) {
            pb->pb_Mode = PORT_PARALLEL;
        } else if (0 == Stricmp(pb->pb_DeviceName, "printer.device")) {
            pb->pb_Mode = PORT_PRINTER;
        } else {
            pb->pb_Mode = PORT_STREAM;
        }
        pa->pa_DeviceUnit = fssm->fssm_Unit;
        pb->pb_DeviceFlags = fssm->fssm_Flags;
        if ((env = BADDR(fssm->fssm_Environ)) &&
            (env->de_TableSize > DE_CONTROL) &&
            ((BSTR)env->de_Control != BNULL)) {
            ret = decodeArgs(pb, pa, (BSTR)env->de_Control);
        }
        break;
    }

    return ret;
}

/* Open the device for IO
 */
static struct portArgs *portOpen(struct portBase *pb, struct portArgs *pa, BPTR name, SIPTR *err)
{
    int len = AROS_BSTR_strlen(name);

    if ((pa = AllocVec(sizeof(*pa) + len + 1, MEMF_ANY))) {
        CopyMem(&pb->pb_Defaults, pa, sizeof(*pa));

        pa->pa_Node.ln_Type = NT_PORTARGS;
        pa->pa_Node.ln_Name = (APTR)(&pa[1]);
        CopyMem(AROS_BSTR_ADDR(name), pa->pa_Node.ln_Name, len);
        pa->pa_Node.ln_Name[len] = 0;

        decodeArgs(pb, pa, name);

        if ((pa->pa_IOMsg = CreateMsgPort())) {
            if ((pa->pa_IO = (APTR)CreateIORequest(pa->pa_IOMsg, sizeof(*pa->pa_IO)))) {
                if (0 == OpenDevice(pb->pb_DeviceName, pa->pa_DeviceUnit, (struct IORequest *)pa->pa_IO, pb->pb_DeviceFlags)) {
                    *err = 0;
                    if (pb->pb_Mode != PORT_SERIAL) {
                        AddTail(&pb->pb_Files, &pa->pa_Node);
                        return pa;
                    }

                    pa->pa_IO->ser.IOSer.io_Command = SDCMD_SETPARAMS;
                                         /* xON xOFF ENQ ACK */
                    pa->pa_IO->ser.io_CtlChar  = SER_DEFAULT_CTLCHAR;
                    pa->pa_IO->ser.io_RBufLen  = 64;
                    pa->pa_IO->ser.io_SerFlags = pa->pa_Serial.ps_SerFlags;
                    pa->pa_IO->ser.io_ExtFlags = pa->pa_Serial.ps_ExtFlags;
                    pa->pa_IO->ser.io_Baud     = pa->pa_Serial.ps_Baud;
                    pa->pa_IO->ser.io_BrkTime  = 250000;
                    pa->pa_IO->ser.io_TermArray.TermArray0 = 0;
                    pa->pa_IO->ser.io_TermArray.TermArray1 = 1;
                    pa->pa_IO->ser.io_ReadLen  = pa->pa_Serial.ps_LenBits;
                    pa->pa_IO->ser.io_WriteLen = pa->pa_Serial.ps_LenBits;
                    pa->pa_IO->ser.io_StopBits = pa->pa_Serial.ps_StopBits;

                    if (0 == DoIO((struct IORequest *)pa->pa_IO)) {
                        AddTail(&pb->pb_Files, &pa->pa_Node);
                        return pa;
                    }
                    CloseDevice((struct IORequest *)pa->pa_IO);
                }
                DeleteIORequest((struct IORequest *)pa->pa_IO);
            }
            DeleteMsgPort(pa->pa_IOMsg);
        }
        FreeVec(pa);
    }

    *err = ERROR_NO_DISK;
    return NULL;
}

static void portClose(struct portArgs *pa)
{
    Remove(&pa->pa_Node);
    CloseDevice((struct IORequest *)pa->pa_IO);
    DeleteIORequest((struct IORequest *)pa->pa_IO);
    DeleteMsgPort(pa->pa_IOMsg);
    FreeVec(pa);
}

__startup void _main(void)
{
    struct DosPacket *dp;
    struct MsgPort *mp;
    BOOL dead = FALSE;
    SIPTR res;
    struct FileHandle *fh;
    struct portBase pb = {};
    struct portArgs *pa;

    NEWLIST(&pb.pb_Files);

    mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;

    WaitPort(mp);

    dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);

    res = RETURN_FAIL;
    if ((pb.pb_UtilityBase = OpenLibrary("utility.library", 0))) {
        res = decodeStartup(&pb, (BPTR)dp->dp_Arg2);
    }

    dp->dp_Res2 = res;
    dp->dp_Res1 = (dp->dp_Res2 == 0) ? DOSTRUE : DOSFALSE;

    do {
        PutMsg (dp->dp_Port, dp->dp_Link);
        WaitPort(mp);
        dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);

        switch (dp->dp_Type) {
        case ACTION_FINDINPUT:
        case ACTION_FINDOUTPUT:
        case ACTION_FINDUPDATE:
            pa = portOpen(&pb, pa, (BSTR)dp->dp_Arg3, &dp->dp_Res2);
            if (dp->dp_Res2 == RETURN_OK) {
                fh = BADDR(dp->dp_Arg1);
                fh->fh_Arg1 = (SIPTR)pa;
                fh->fh_Type = mp;
            }
            dp->dp_Res1 = (dp->dp_Res2 == 0) ? DOSTRUE : DOSFALSE;
            break;
        case ACTION_READ:
            if ((pa = (struct portArgs *)dp->dp_Arg1)) {
                pa->pa_IO->std.io_Command = CMD_READ;
                pa->pa_IO->std.io_Data = (APTR)dp->dp_Arg2;
                pa->pa_IO->std.io_Length = dp->dp_Arg3;
                pa->pa_IO->std.io_Actual = 0;
                pa->pa_IO->std.io_Offset = 0;
                pa->pa_IO->std.io_Message.mn_Length = sizeof(pa->pa_IO->std);
                res = DoIO((struct IORequest *)pa->pa_IO);
                if (res == 0) {
                    dp->dp_Res1 = pa->pa_IO->std.io_Actual;
                    dp->dp_Res2 = 0;
                } else {
                    dp->dp_Res1 = DOSFALSE;
                    dp->dp_Res2 = ERROR_READ_PROTECTED;
                }
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
            }
            break;
        case ACTION_WRITE:
            if ((pa = (struct portArgs *)dp->dp_Arg1)) {
                if (pb.pb_Mode == PORT_PRINTER &&
                    pa->pa_Printer.pr_Type == PRT_RAW)
                    pa->pa_IO->std.io_Command = PRD_RAWWRITE;
                else
                    pa->pa_IO->std.io_Command = CMD_WRITE;
                pa->pa_IO->std.io_Data = (APTR)dp->dp_Arg2;
                pa->pa_IO->std.io_Length = dp->dp_Arg3;
                pa->pa_IO->std.io_Actual = 0;
                pa->pa_IO->std.io_Offset = 0;
                pa->pa_IO->std.io_Message.mn_Length = sizeof(pa->pa_IO->std);
                res = DoIO((struct IORequest *)pa->pa_IO);
                if (res == 0) {
                    dp->dp_Res1 = pa->pa_IO->std.io_Actual;
                    dp->dp_Res2 = 0;
                } else {
                    dp->dp_Res1 = DOSFALSE;
                    dp->dp_Res2 = ERROR_READ_PROTECTED;
                }
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
            }
            break;
        case ACTION_END:
            if ((pa = (struct portArgs *)dp->dp_Arg1)) {
                portClose(pa);
                dp->dp_Res1 = DOSTRUE;
                dp->dp_Res2 = 0;
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
            }
            break;
        case ACTION_DIE:
            if (IsListEmpty(&pb.pb_Files)) {
                dp->dp_Res1 = DOSTRUE;
                dp->dp_Res2 = 0;
                dead = TRUE;
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_IN_USE;
            }
            break;
        default:
            dp->dp_Res1 = DOSFALSE;
            dp->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
            break;
        }
    } while (!dead);

/* ACTION_DIE ends up here... */
    PutMsg (dp->dp_Port, dp->dp_Link);

    CloseLibrary(pb.pb_UtilityBase);
}
