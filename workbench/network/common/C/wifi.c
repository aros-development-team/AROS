/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Report the wireless state of a SANA-II device.
*/

/******************************************************************************

    NAME

        wifi

    SYNOPSIS

        DEVICE,UNIT/K/N,SCAN/S

    LOCATION

        C:

    FUNCTION

        Show whether a wireless SANA-II device is associated, and to which
        network. With SCAN, list the networks it can currently see.

        Without DEVICE the wireless device configured in the network
        preferences is used (ENV:AROSTCP/WirelessDevice).

    INPUTS

        DEVICE  --  SANA-II device to query, e.g. DEVS:Networks/bwfm.device
        UNIT    --  device unit, default 0
        SCAN    --  scan for visible networks (takes a few seconds)

    RESULT

        Standard DOS return codes.

    NOTES

        Nothing here needs a TCP/IP stack: the driver is asked directly, so
        this also works while the network is down. Use ifconfig for addresses.

    EXAMPLE

        wifi
        wifi SCAN
        wifi DEVS:Networks/bwfm.device UNIT 0

    SEE ALSO

        ifconfig

******************************************************************************/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <devices/sana2.h>
#include <devices/sana2wireless.h>
#include <dos/dos.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <stdlib.h>
#include <string.h>

#define WIRELESS_VAR    "AROSTCP/WirelessDevice"
#define PUDDLE_SIZE     4096
#define ETH_ALEN        6

/*
 * SANA-II hands frames through caller-supplied copy functions. This tool never
 * moves a frame, but a driver is entitled to expect the hooks to be there.
 */
static BOOL copy_buff(UBYTE *dst, UBYTE *src, ULONG size)
{
    CopyMem(src, dst, size);
    return TRUE;
}

static const struct TagItem buffer_tags[] =
{
    { S2_CopyToBuff,    (IPTR)copy_buff },
    { S2_CopyFromBuff,  (IPTR)copy_buff },
    { TAG_DONE,         0               }
};

static void PrintMAC(CONST_STRPTR prefix, const UBYTE *addr)
{
    Printf("%s%02lx:%02lx:%02lx:%02lx:%02lx:%02lx", (IPTR)prefix,
        (IPTR)addr[0], (IPTR)addr[1], (IPTR)addr[2],
        (IPTR)addr[3], (IPTR)addr[4], (IPTR)addr[5]);
}

/*
 * Name the protection from the beacon IEs (S2INFO_InfoElements: a UWORD byte
 * count followed by the elements). RSN (id 48) is WPA2 or newer; the vendor
 * element with the Microsoft OUI and type 1 is the original WPA.
 */
static CONST_STRPTR IEProtection(const UBYTE *ies)
{
    ULONG len, off;
    CONST_STRPTR prot = "open";

    if (ies == NULL)
        return "-";

    len = *(const UWORD *)ies;
    ies += 2;

    for (off = 0; off + 2 <= len; off += 2 + ies[off + 1])
    {
        UBYTE id = ies[off], ielen = ies[off + 1];

        if (off + 2 + ielen > len)
            break;

        if (id == 48)
            return "WPA2";
        if (id == 221 && ielen >= 4 &&
            ies[off + 2] == 0x00 && ies[off + 3] == 0x50 &&
            ies[off + 4] == 0xf2 && ies[off + 5] == 0x01)
            prot = "WPA";
    }

    return prot;
}

/* Split "<device> UNIT <n>" as written by the network preferences. */
static void SplitDeviceVar(STRPTR buf, STRPTR *device, LONG *unit)
{
    STRPTR p = buf;

    *device = buf;
    while (*p != '\0' && *p != ' ')
        p++;
    if (*p == '\0')
        return;
    *p++ = '\0';

    while (*p == ' ')
        p++;
    if (Strnicmp(p, "UNIT", 4) == 0)
    {
        p += 4;
        while (*p == ' ')
            p++;
        if (*p >= '0' && *p <= '9')
            *unit = atol(p);
    }
}

#define ARG_DEVICE      0
#define ARG_UNIT        1
#define ARG_SCAN        2

static const TEXT template[] = "DEVICE,UNIT/K/N,SCAN/S";

int main(void)
{
    struct MsgPort *port = NULL;
    struct IOSana2Req *req = NULL;
    struct RDArgs *rda = NULL;
    APTR pool = NULL;
    IPTR args[3] = { (IPTR)NULL, (IPTR)NULL, 0 };
    TEXT devbuf[128];
    STRPTR device;
    LONG unit = 0, rc = RETURN_FAIL;
    BOOL opened = FALSE;

    rda = ReadArgs(template, args, NULL);
    if (rda == NULL)
    {
        PrintFault(IoErr(), "wifi");
        return RETURN_FAIL;
    }

    device = (STRPTR)args[ARG_DEVICE];
    if (args[ARG_UNIT] != (IPTR)NULL)
        unit = *(LONG *)args[ARG_UNIT];

    if (device == NULL)
    {
        if (GetVar(WIRELESS_VAR, devbuf, sizeof(devbuf), LV_VAR) <= 0)
        {
            PutStr("No wireless device configured. Set one in the network"
                   " preferences, or name it on the command line.\n");
            goto cleanup;
        }
        SplitDeviceVar(devbuf, &device, &unit);
    }

    port = CreateMsgPort();
    if (port != NULL)
        req = AllocVec(sizeof(struct IOSana2Req), MEMF_PUBLIC | MEMF_CLEAR);
    pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, PUDDLE_SIZE, PUDDLE_SIZE);
    if (port == NULL || req == NULL || pool == NULL)
    {
        PrintFault(ERROR_NO_FREE_STORE, "wifi");
        goto cleanup;
    }

    req->ios2_Req.io_Message.mn_ReplyPort = port;
    req->ios2_Req.io_Message.mn_Length = sizeof(*req);
    req->ios2_BufferManagement = (APTR)buffer_tags;

    if (OpenDevice(device, unit, (struct IORequest *)req, 0) != 0)
    {
        Printf("Cannot open %s unit %ld\n", (IPTR)device, (IPTR)unit);
        goto cleanup;
    }
    opened = TRUE;

    Printf("%s unit %ld\n", (IPTR)device, (IPTR)unit);

    req->ios2_Req.io_Command = S2_GETSTATIONADDRESS;
    if (DoIO((struct IORequest *)req) == 0)
    {
        PrintMAC("  Address     ", req->ios2_SrcAddr);
        PutStr("\n");
    }

    /* Association state. A driver that is up but not joined answers with an
     * error, which is the interesting case to report plainly. */
    req->ios2_Req.io_Command = S2_GETNETWORKINFO;
    req->ios2_Data = pool;
    req->ios2_StatData = NULL;
    if (DoIO((struct IORequest *)req) == 0 && req->ios2_StatData != NULL)
    {
        struct TagItem *tl = req->ios2_StatData;
        STRPTR ssid = (STRPTR)GetTagData(S2INFO_SSID, (IPTR)NULL, tl);
        UBYTE *bssid = (UBYTE *)GetTagData(S2INFO_BSSID, (IPTR)NULL, tl);

        Printf("  Network     %s\n",
            (IPTR)(ssid != NULL ? (CONST_STRPTR)ssid : (CONST_STRPTR)"(unknown)"));
        if (bssid != NULL)
        {
            PrintMAC("  Access point ", bssid);
            PutStr("\n");
        }
        PutStr("  State       associated\n");
    }
    else
        PutStr("  State       not associated\n");

    rc = RETURN_OK;

    if (args[ARG_SCAN])
    {
        struct TagItem **lists;
        ULONG count, i;

        PutStr("\nScanning...\n");
        req->ios2_Req.io_Command = S2_GETNETWORKS;
        req->ios2_Data = pool;
        req->ios2_StatData = NULL;
        req->ios2_DataLength = 0;
        if (DoIO((struct IORequest *)req) != 0)
        {
            Printf("Scan failed (error %ld, wire error %ld)\n",
                (IPTR)req->ios2_Req.io_Error, (IPTR)req->ios2_WireError);
            rc = RETURN_ERROR;
            goto cleanup;
        }

        count = req->ios2_DataLength;
        lists = req->ios2_StatData;
        Printf("%-32s %-17s %4s %6s %s\n", (IPTR)"Network", (IPTR)"Access point",
            (IPTR)"Ch", (IPTR)"Signal", (IPTR)"Protection");

        for (i = 0; i < count && lists != NULL; i++)
        {
            struct TagItem *tl = lists[i];
            STRPTR ssid;
            UBYTE *bssid;

            if (tl == NULL)
                continue;

            ssid = (STRPTR)GetTagData(S2INFO_SSID, (IPTR)"", tl);
            bssid = (UBYTE *)GetTagData(S2INFO_BSSID, (IPTR)NULL, tl);

            Printf("%-32s ", (IPTR)(ssid[0] != '\0' ? (CONST_STRPTR)ssid
                                             : (CONST_STRPTR)"(hidden)"));
            if (bssid != NULL)
                PrintMAC("", bssid);
            else
                PutStr("                 ");
            Printf(" %4ld %6ld %s\n",
                (IPTR)GetTagData(S2INFO_Channel, 0, tl),
                (IPTR)(LONG)GetTagData(S2INFO_Signal, 0, tl),
                (IPTR)IEProtection((const UBYTE *)
                    GetTagData(S2INFO_InfoElements, (IPTR)NULL, tl)));
        }

        if (count == 0)
            PutStr("No networks found.\n");
    }

cleanup:
    if (opened)
        CloseDevice((struct IORequest *)req);
    if (pool != NULL)
        DeletePool(pool);
    FreeVec(req);
    if (port != NULL)
        DeleteMsgPort(port);
    if (rda != NULL)
        FreeArgs(rda);

    return rc;
}
