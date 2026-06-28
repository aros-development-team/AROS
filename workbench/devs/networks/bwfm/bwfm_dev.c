/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    bwfm.device - SANA-II network device for the Broadcom FullMAC WiFi chip.

    Phase 3 skeleton: opens bwfm.resource (chip bring-up), manages units and
    openers, and answers the SANA-II query commands. The actual datapath
    (CMD_READ/WRITE) and 802.11 management arrive once firmware download +
    SDPCM are wired up; for now those commands report the unit offline.

    Genmodule device structure modelled on the AROS tap.device (APL).
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <exec/errors.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <devices/sana2.h>
#include <devices/sana2wireless.h>
#include <devices/newstyle.h>
#include <devices/timer.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/bwfm.h>

#include "bwfm.h"

APTR BWFMBase __attribute__((used)) = NULL;     /* bwfm.resource (proto/bwfm.h base) */
struct DosLibrary *DOSBase __attribute__((used)) = NULL;
static LIBBASETYPEPTR PumpBase = NULL;          /* device base for the RX pump */

#define BWFM_FW_DIR     "DEVS:Firmware/"
#define BWFM_C_UP       2               /* firmware "interface up" ioctl */
#define BWFM_C_GET_BSSID 23             /* firmware "get associated BSSID" */

#define ETH_ALEN        6
#define ETH_HLEN        14
#define ETH_FRAME_LEN   1518

/*
 * Arm the SDIO card interrupt (1) or poll only (0). The BCM2835 Arasan card
 * interrupt fires once but does not reliably re-edge per frame (the chip-side
 * re-arm is a known quirk - michalsc's WiFiPi.device polls for the same reason),
 * so the RX pump uses adaptive-backoff polling instead and leaves this off.
 */
#define BWFM_USE_IRQ    0

/* RX poll backoff window: poll fast while frames flow, ease off when idle. */
#define BWFM_POLL_MIN   1000            /* 1 ms  - busy */
#define BWFM_POLL_MAX   100000          /* 100 ms - idle */

/* 802.3 frame header (matches the on-wire layout the firmware hands us). */
struct bwfm_ethhdr
{
    UBYTE   h_dest[ETH_ALEN];
    UBYTE   h_source[ETH_ALEN];
    UWORD   h_proto;
};

/* One scan result, layout shared with bwfm.resource's BWFMScan() output
 * (struct bwfm_scanresult in bwfm_scan.h - the resource header is not on our
 * include path, so the layout is mirrored here; clean-build, no ABI concerns). */
#define BWFM_MAX_SSID_LEN       32
#define BWFM_MAX_SCAN           40      /* networks BWFMScan() may return */

struct bwfm_scanresult
{
    UBYTE   bssid[ETH_ALEN];
    UBYTE   ssid_len;
    UBYTE   ssid[BWFM_MAX_SSID_LEN];
    WORD    rssi;
    UWORD   chanspec;
};

static const ULONG rx_tags[] = { S2_CopyToBuff, S2_CopyToBuff16 };
static const ULONG tx_tags[] = { S2_CopyFromBuff, S2_CopyFromBuff16, S2_CopyFromBuff32 };

static const UWORD supported_commands[] =
{
    CMD_READ, CMD_WRITE, CMD_FLUSH,
    S2_DEVICEQUERY, S2_GETSTATIONADDRESS, S2_CONFIGINTERFACE,
    S2_ONLINE, S2_OFFLINE,
    S2_BROADCAST, S2_MULTICAST,
    S2_TRACKTYPE, S2_UNTRACKTYPE, S2_GETTYPESTATS, S2_GETGLOBALSTATS,
    S2_ADDMULTICASTADDRESS, S2_DELMULTICASTADDRESS,
    /* SANA-II wireless extensions (drives wpa_supplicant / WirelessManager).
     * Listing S2_GETNETWORKS makes wpa_supplicant treat us as a hard-MAC
     * (FullMAC) device, which matches the chip. */
    S2_GETNETWORKS, S2_GETCRYPTTYPES, S2_ONEVENT, S2_SETOPTIONS, S2_SETKEY,
    S2_GETNETWORKINFO,
    NSCMD_DEVICEQUERY,
    0
};

/* ----------------------------------------------------------------------- */

static int GM_UNIQUENAME(init)(LIBBASETYPEPTR LIBBASE)
{
    int i;

    BWFMBase = OpenResource("bwfm.resource");
    if (BWFMBase)
        D(bug("[bwfm.device] bwfm.resource present (chip brought up on first open)\n"));
    else
        D(bug("[bwfm.device] bwfm.resource not available\n"));

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);

    for (i = 0; i < BWFM_MAX_UNITS; i++)
    {
        LIBBASE->unit[i].num = i;
        InitSemaphore(&LIBBASE->unit[i].lock);
    }

    return TRUE;
}

/* ----------------------------------------------------------------------- */
/* Firmware loading from disk (DEVS:Firmware), then hand to bwfm.resource   */

static void str_append(char **p, const char *s)
{
    while (*s)
        *(*p)++ = *s++;
}

/* Build "DEVS:Firmware/brcmfmac<id>-sdio<ext>" for the detected chip. */
static int fw_filename(char *out, ULONG chip, const char *ext)
{
    const char *id;
    char *p = out;

    switch (chip)
    {
    case 43430:     id = "43430"; break;        /* Pi 3 / 3B (BCM43430) */
    case 0x4345:    id = "43455"; break;        /* Pi 3B+ / 4 (BCM43455) */
    default:        return -1;
    }

    str_append(&p, BWFM_FW_DIR "brcmfmac");
    str_append(&p, id);
    str_append(&p, "-sdio");
    str_append(&p, ext);
    *p = '\0';
    return 0;
}

/* Read a whole file into a freshly AllocVec'd buffer. */
static APTR read_file(const char *name, ULONG *lenp)
{
    BPTR fh;
    LONG size;
    APTR buf;

    fh = Open((CONST_STRPTR)name, MODE_OLDFILE);
    if (!fh)
        return NULL;

    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);       /* returns the end position */
    if (size <= 0)
    {
        Close(fh);
        return NULL;
    }

    buf = AllocVec(size, MEMF_PUBLIC);
    if (buf && Read(fh, buf, size) != size)
    {
        FreeVec(buf);
        buf = NULL;
    }
    Close(fh);

    if (buf)
        *lenp = size;
    return buf;
}

/*
 * Convert a brcmfmac nvram text file into the binary form the firmware
 * expects: comment/blank lines dropped, '\n' -> '\0', padded to a 4-byte
 * boundary and terminated with a (count/4) size token. Ported from
 * OpenBSD bwfm_nvram_convert (no device-tree MAC append).
 */
static APTR nvram_convert(const UBYTE *src, ULONG size, ULONG *newlenp)
{
    const UBYTE *end = src + size;
    UBYTE *dst, *newbuf;
    ULONG count, pad;
    int skip = 0;
    uint32_t token;

    newbuf = AllocVec(size + 64, MEMF_PUBLIC | MEMF_CLEAR);
    if (!newbuf)
        return NULL;
    dst = newbuf;
    count = 0;

    for (; src != end; ++src)
    {
        if (*src == '\n')
        {
            if (count > 0)
                *dst++ = '\0';
            count = 0;
            skip = 0;
            continue;
        }
        if (skip)
            continue;
        if (*src == '#' && count == 0)
        {
            skip = 1;
            continue;
        }
        if (*src == '\r')
            continue;
        *dst++ = *src;
        ++count;
    }

    count = dst - newbuf;
    pad = (((count + 1) + 3) & ~3UL) - count;   /* roundup(count+1,4) - count */
    dst += pad;
    count += pad;                               /* pad bytes already zeroed */

    token = (count / 4) & 0xffff;
    token |= (~token) << 16;
    *(uint32_t *)dst = AROS_LONG2LE(token);
    count += sizeof(token);

    *newlenp = count;
    return newbuf;
}

/* ----------------------------------------------------------------------- */
/* Async SDPCM RX pump + SANA-II datapath                                  */

/*
 * Deliver one received 802.3 frame to whichever opener has a matching pending
 * CMD_READ request. Modelled on tap.device's tap_receive().
 */
static void rx_deliver(LIBBASETYPEPTR LIBBASE, struct bwfm_unit *unit,
                       UBYTE *buf, ULONG len)
{
    struct bwfm_ethhdr *eth = (struct bwfm_ethhdr *)buf;
    struct bwfm_opener *opener, *onext;
    struct IOSana2Req *req, *rnext;
    WORD packet_type;
    UBYTE *dst;
    BOOL bcast = FALSE, mcast = FALSE;

    if (len < ETH_HLEN)
        return;

    packet_type = AROS_BE2WORD(eth->h_proto);
    dst = eth->h_dest;
    if (*(ULONG *)dst == 0xffffffff && *(UWORD *)(dst + 4) == 0xffff)
        bcast = TRUE;
    else if (dst[0] & 0x01)
        mcast = TRUE;

    /* Held across the whole walk so a concurrent close()/CMD_FLUSH/AbortIO on
     * another CPU cannot free a list node we are iterating over (SMP). */
    ObtainSemaphore(&unit->lock);
    ForeachNodeSafe(&unit->openers, opener, onext)
    {
        ForeachNodeSafe(&opener->read_pending.mp_MsgList, req, rnext)
        {
            UBYTE *packet;

            if (req->ios2_PacketType != (ULONG)packet_type)
                continue;

            req->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
            if (bcast)
                req->ios2_Req.io_Flags |= SANA2IOF_BCAST;
            if (mcast)
                req->ios2_Req.io_Flags |= SANA2IOF_MCAST;

            CopyMem(eth->h_source, req->ios2_SrcAddr, ETH_ALEN);
            CopyMem(eth->h_dest, req->ios2_DstAddr, ETH_ALEN);

            if (req->ios2_Req.io_Flags & SANA2IOF_RAW)
            {
                req->ios2_DataLength = len;
                packet = buf;
            }
            else
            {
                req->ios2_DataLength = len - ETH_HLEN;
                packet = buf + ETH_HLEN;
            }

            /* opener packet filter: drop the frame for this request if it says no */
            if (opener->filter != NULL &&
                !CallHookPkt(opener->filter, req, packet))
                continue;

            if (opener->rx == NULL ||
                !opener->rx(req->ios2_Data, packet, req->ios2_DataLength))
            {
                req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
                req->ios2_WireError = S2WERR_BUFF_ERROR;
            }

            Remove((struct Node *)req);
            ReplyMsg(&req->ios2_Req.io_Message);
            break;          /* one read per opener; keep offering to the rest */
        }
    }
    ReleaseSemaphore(&unit->lock);
}

/*
 * RX pump process: drains SDPCM frames from the chip and dispatches them.
 * Data frames go to rx_deliver(); firmware events are logged for now (link /
 * assoc handling arrives with join). Runs for the device's lifetime; the chip
 * buffers frames, so when idle we poll on a coarse timer.
 */
static void bwfm_pump(void)
{
    LIBBASETYPEPTR LIBBASE = PumpBase;
    struct bwfm_unit *unit = &LIBBASE->unit[0];
    static UBYTE rxbuf[ETH_FRAME_LEN];
    struct MsgPort *tport;
    struct timerequest *tr = NULL;
    ULONG irqsig, irqmask = 0, timersig = 0, woke = 0, delay = BWFM_POLL_MIN;
    int use_irq = 0, idle = 0;

    /* Card interrupt wakes us via BWFMSetRxSignal; the handler masks it in
     * INT_ENABLE, we clear the chip's SDPCM int + re-arm after draining. */
    irqsig = (ULONG)-1;
    if (BWFM_USE_IRQ)
        irqsig = AllocSignal(-1);
    if (irqsig != (ULONG)-1)
    {
        irqmask = 1UL << irqsig;
        BWFMSetRxSignal(FindTask(NULL), irqmask);
        use_irq = 1;
    }

    /* Safety timeout so a missed interrupt can never stall RX. */
    tport = CreateMsgPort();
    if (tport)
        tr = (struct timerequest *)CreateIORequest(tport, sizeof(struct timerequest));
    if (tr && OpenDevice((CONST_STRPTR)"timer.device", UNIT_MICROHZ,
                         (struct IORequest *)tr, 0) == 0)
        timersig = 1UL << tport->mp_SigBit;

    D(bug("[bwfm.device] RX pump running (irq %s, timer %s)\n",
          use_irq ? "on" : "off", timersig ? "on" : "off"));

    for (;;)
    {
        ULONG info, len;
        int got = 0;

        /* Drain everything in F2 (and the glom queue), THEN ack the chip's card
         * interrupt: clearing HMB_FRAME_IND with F2 already empty deasserts DAT1,
         * so the next frame re-triggers the edge-sensitive Arasan card interrupt
         * (true per-frame IRQ rather than a one-shot + timer poll). */
        while ((len = (ULONG)BWFMRxFrame(rxbuf, sizeof(rxbuf), &info)) > 0)
        {
            if (info & BWFM_RX_EVENT)
            {
                D(bug("[bwfm.device] RX event type %lu\n", (info >> 16) & 0xffff));
                BWFMEventPost(rxbuf, len);  /* hand to a waiting join/scan */
            }
            else if (unit->online)
                rx_deliver(LIBBASE, unit, rxbuf, len);
            got = 1;
        }

        if (use_irq)
            BWFMClearInt();

        /*
         * Spin guard: if the card interrupt keeps waking us with nothing to
         * read (chip int not clearing), stop using it and fall back to timer
         * polling - never worse than the old behaviour, never a busy loop.
         */
        if (use_irq && (woke & irqmask) && !got)
        {
            if (++idle >= 8)
            {
                use_irq = 0;
                D(bug("[bwfm.device] card IRQ not self-clearing - polling\n"));
            }
        }
        else
            idle = 0;

        /* Adaptive backoff: reset to the fast interval whenever a frame turned
         * up, else double the wait up to the idle ceiling - low latency under
         * load, low CPU when quiet (mirrors WiFiPi.device's poll strategy). */
        if (got)
            delay = BWFM_POLL_MIN;
        else if ((delay <<= 1) > BWFM_POLL_MAX)
            delay = BWFM_POLL_MAX;

        if (timersig)
        {
            tr->tr_node.io_Command = TR_ADDREQUEST;
            tr->tr_time.tv_secs = 0;
            tr->tr_time.tv_micro = delay;
            SendIO((struct IORequest *)tr);
            if (use_irq)
                BWFMAckRx();
            woke = Wait(irqmask | timersig);
            if (!CheckIO((struct IORequest *)tr))
                AbortIO((struct IORequest *)tr);
            WaitIO((struct IORequest *)tr);
        }
        else
        {
            Delay(1);
            woke = 0;
        }
    }
}

/* Run a queued associate (defined below, near report_events). */
static void do_associate(struct bwfm_unit *unit);

/*
 * Control worker: runs the blocking BWFMJoin for associate requests off the
 * caller's task, so it never freezes wpa_supplicant's event loop (and so the RX
 * pump - a separate task - stays free to feed BWFMJoin its association events).
 * Woken by S2_SETOPTIONS via Signal(); also checks on entry in case a job was
 * queued before this task armed its signal.
 */
static void bwfm_ctrl(void)
{
    LIBBASETYPEPTR LIBBASE = PumpBase;
    struct bwfm_unit *unit = &LIBBASE->unit[0];
    ULONG sig = AllocSignal(-1);

    LIBBASE->ctrl_sig = (sig == (ULONG)-1) ? 0 : (1UL << sig);
    LIBBASE->ctrl_task = FindTask(NULL);

    D(bug("[bwfm.device] ctrl worker running (sig %s)\n",
          LIBBASE->ctrl_sig ? "on" : "poll"));

    for (;;)
    {
        do_associate(unit);             /* drain any pending job */
        if (LIBBASE->ctrl_sig)
            Wait(LIBBASE->ctrl_sig);
        else
            Delay(5);                   /* signal alloc failed: poll fallback */
    }
}

/* Spawn the RX pump + control worker once the chip firmware is up. */
static void start_pump(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->pump != NULL)
        return;

    PumpBase = LIBBASE;
    LIBBASE->pump = CreateNewProcTags(
        NP_Entry, (IPTR)bwfm_pump,
        NP_Name,  (IPTR)"bwfm.rxpump",
        NP_Priority, 5,
        TAG_DONE);

    /* Async associate worker (see bwfm_ctrl). */
    LIBBASE->ctrl = CreateNewProcTags(
        NP_Entry, (IPTR)bwfm_ctrl,
        NP_Name,  (IPTR)"bwfm.ctrl",
        NP_Priority, 4,
        TAG_DONE);

    /* The pump + ctrl worker reference LIBBASE for their whole life - keep the
     * device from being expunged after the last CloseDevice by counting them
     * as openers. */
    if (LIBBASE->pump != NULL)
        LIBBASE->device.dd_Library.lib_OpenCnt++;
    if (LIBBASE->ctrl != NULL)
        LIBBASE->device.dd_Library.lib_OpenCnt++;
}

/*
 * Transmit one SANA-II write request: build the 802.3 header (unless RAW),
 * copy the caller's payload through its tx buffer-management hook, and hand
 * the frame to bwfm.resource. Modelled on tap.device's tap_send().
 */
static void tx_request(struct bwfm_unit *unit, struct IOSana2Req *req)
{
    struct bwfm_opener *opener = (struct bwfm_opener *)req->ios2_BufferManagement;
    UBYTE txbuf[ETH_FRAME_LEN];
    struct bwfm_ethhdr *eth = (struct bwfm_ethhdr *)txbuf;
    UBYTE *packet;
    ULONG framelen;

    if (!unit->online)
    {
        req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        req->ios2_WireError = S2WERR_UNIT_OFFLINE;
        return;
    }

    if (req->ios2_Req.io_Flags & SANA2IOF_RAW)
    {
        packet = txbuf;
        framelen = req->ios2_DataLength;
    }
    else
    {
        CopyMem(unit->hwaddr, eth->h_source, ETH_ALEN);
        CopyMem(req->ios2_DstAddr, eth->h_dest, ETH_ALEN);
        eth->h_proto = AROS_WORD2BE(req->ios2_PacketType);
        packet = txbuf + ETH_HLEN;
        framelen = ETH_HLEN + req->ios2_DataLength;
    }

    if (framelen > sizeof(txbuf) ||
        opener->tx == NULL ||
        !opener->tx(packet, req->ios2_Data, req->ios2_DataLength))
    {
        req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        req->ios2_WireError = S2WERR_BUFF_ERROR;
        return;
    }

    if (BWFMTxData(txbuf, framelen) != 0)
    {
        req->ios2_Req.io_Error = S2ERR_TX_FAILURE;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
    }
}

/* One-time chip firmware bring-up. Returns TRUE on success. */
static int load_firmware(LIBBASETYPEPTR LIBBASE)
{
    char name[64];
    APTR fw = NULL, nvtxt = NULL, nvbin = NULL;
    ULONG fwlen = 0, nvtxtlen = 0, nvbinlen = 0;
    ULONG chip;
    int ok = FALSE;

    if (LIBBASE->fw_loaded)
        return TRUE;
    if (!BWFMBase || !DOSBase)
        return FALSE;

    /* Lazy chip bring-up: power the WiFi chip + enumerate it now (first open).
     * No-op if a prior open already attached. */
    if (!BWFMAttach())
    {
        D(bug("[bwfm.device] chip attach failed (no WiFi chip?)\n"));
        return FALSE;
    }

    chip = BWFMChipID();
    if (chip == 0 || fw_filename(name, chip, ".bin"))
    {
        D(bug("[bwfm.device] no firmware mapping for chip %x\n", chip));
        return FALSE;
    }

    fw = read_file(name, &fwlen);
    if (!fw)
    {
        D(bug("[bwfm.device] cannot read firmware '%s'\n", name));
        goto out;
    }

    if (fw_filename(name, chip, ".txt") == 0)
    {
        nvtxt = read_file(name, &nvtxtlen);
        if (nvtxt)
            nvbin = nvram_convert(nvtxt, nvtxtlen, &nvbinlen);
    }

    D(bug("[bwfm.device] starting firmware (%u bytes, nvram %u bytes)\n",
          fwlen, nvbinlen));

    if (BWFMStartFirmware(fw, fwlen, nvbin, nvbinlen) == 0)
    {
        LIBBASE->fw_loaded = TRUE;
        ok = TRUE;

        /* Firmware is running: read the chip MAC into every unit. */
        if (BWFMGetMAC(LIBBASE->unit[0].hwaddr) == 0)
            D(bug("[bwfm.device] MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                  LIBBASE->unit[0].hwaddr[0], LIBBASE->unit[0].hwaddr[1],
                  LIBBASE->unit[0].hwaddr[2], LIBBASE->unit[0].hwaddr[3],
                  LIBBASE->unit[0].hwaddr[4], LIBBASE->unit[0].hwaddr[5]));
        else
            D(bug("[bwfm.device] failed to read chip MAC\n"));

        /* Load the regulatory CLM blob if one ships for this chip. Without it
         * the radio has no valid channels and scanning fails (NOTUP). The
         * Pi 3 (43430) firmware has built-in regulatory and ships no blob, so
         * a missing file is not an error. */
        if (fw_filename(name, chip, ".clm_blob") == 0)
        {
            ULONG clmlen = 0;
            APTR clm = read_file(name, &clmlen);

            if (clm)
            {
                D(bug("[bwfm.device] loading CLM blob (%u bytes)\n", clmlen));
                BWFMLoadCLM(clm, clmlen);
                FreeVec(clm);
            }
            else
                D(bug("[bwfm.device] no CLM blob at %s (IoErr %ld) - using"
                      " built-in regulatory\n", name, IoErr()));
        }

        /* Bring the firmware MAC layer up */
        if (BWFMIoctl(BWFM_C_UP, 1, NULL, 0) == 0)
            D(bug("[bwfm.device] interface up\n"));
        else
            D(bug("[bwfm.device] BWFM_C_UP failed\n"));

        /* Start the async RX pump now that the chip can deliver frames. */
        start_pump(LIBBASE);
    }

out:
    FreeVec(fw);
    FreeVec(nvtxt);
    FreeVec(nvbin);
    return ok;
}

/*
 * WiFi credentials for auto-join live in DEVS:bwfm.prefs: the first non-blank,
 * non-comment line is the SSID, the second (optional) is the WPA2 passphrase
 * (omit it for an open network). Whole-line values so SSIDs may contain spaces.
 * Returns TRUE when at least an SSID was read.
 */
static int load_wifi_prefs(UBYTE *ssid, ULONG *ssidlen, UBYTE *key, ULONG *keylen)
{
    ULONG flen = 0, i = 0, line = 0;
    UBYTE *buf = read_file("DEVS:bwfm.prefs", &flen);

    *ssidlen = 0;
    *keylen = 0;
    if (buf == NULL)
        return FALSE;

    while (i < flen && line < 2)
    {
        ULONG s = i, e, n;

        while (i < flen && buf[i] != '\n' && buf[i] != '\r')
            i++;
        e = i;
        while (i < flen && (buf[i] == '\n' || buf[i] == '\r'))
            i++;
        if (e == s || buf[s] == '#')
            continue;                       /* blank line or comment */

        if (line == 0)
        {
            n = e - s;
            if (n > 32)
                n = 32;
            CopyMem(buf + s, ssid, n);
            *ssidlen = n;
        }
        else
        {
            n = e - s;
            if (n > 63)
                n = 63;
            CopyMem(buf + s, key, n);
            *keylen = n;
        }
        line++;
    }

    FreeVec(buf);
    return (*ssidlen > 0);
}

/* Associate to the configured network once, when the interface goes online. */
static void try_join(struct bwfm_unit *unit)
{
    UBYTE ssid[33], key[64];
    ULONG ssidlen = 0, keylen = 0;

    if (unit->joined)
        return;
    if (!load_wifi_prefs(ssid, &ssidlen, key, &keylen))
    {
        D(bug("[bwfm.device] no DEVS:bwfm.prefs - not auto-joining\n"));
        return;
    }

    ssid[ssidlen] = '\0';
    D(bug("[bwfm.device] auto-join \"%s\" (%s)\n", ssid, keylen ? "WPA2" : "open"));
    if (BWFMJoin(ssid, ssidlen, keylen ? key : NULL, keylen) == 0)
    {
        unit->joined = TRUE;
        D(bug("[bwfm.device] auto-join OK\n"));
    }
    else
        D(bug("[bwfm.device] auto-join failed\n"));
}

/* SANA-II per-packet-type tracking (used by AROSTCP at interface setup). */
static void track_type(struct bwfm_unit *unit, struct IOSana2Req *req)
{
    struct bwfm_tracker *t, *tn;
    ULONG type = req->ios2_PacketType;

    /* Walk + insert under the lock so two concurrent S2_TRACKTYPE callers
     * cannot both miss and then add a duplicate tracker (SMP). */
    ObtainSemaphore(&unit->lock);
    ForeachNodeSafe(&unit->trackers, t, tn)
        if (t->packet_type == type)
        {
            t->refcount++;
            ReleaseSemaphore(&unit->lock);
            return;
        }

    t = AllocVec(sizeof(struct bwfm_tracker), MEMF_PUBLIC | MEMF_CLEAR);
    if (t == NULL)
    {
        ReleaseSemaphore(&unit->lock);
        req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        return;
    }
    t->refcount = 1;
    t->packet_type = type;
    AddTail((struct List *)&unit->trackers, (struct Node *)t);
    ReleaseSemaphore(&unit->lock);
}

static void untrack_type(struct bwfm_unit *unit, struct IOSana2Req *req)
{
    struct bwfm_tracker *t, *tn;
    ULONG type = req->ios2_PacketType;

    ObtainSemaphore(&unit->lock);
    ForeachNodeSafe(&unit->trackers, t, tn)
        if (t->packet_type == type)
        {
            if (--t->refcount == 0)
            {
                Remove((struct Node *)t);
                ReleaseSemaphore(&unit->lock);
                FreeVec(t);
                return;
            }
            ReleaseSemaphore(&unit->lock);
            return;
        }
    ReleaseSemaphore(&unit->lock);
}

static void get_type_stats(struct bwfm_unit *unit, struct IOSana2Req *req)
{
    struct bwfm_tracker *t, *tn;
    ULONG type = req->ios2_PacketType;

    if (req->ios2_StatData == NULL)
    {
        req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        req->ios2_WireError = S2WERR_BAD_STATDATA;
        return;
    }

    ObtainSemaphore(&unit->lock);
    ForeachNodeSafe(&unit->trackers, t, tn)
        if (t->packet_type == type)
        {
            CopyMem(&t->stats, req->ios2_StatData, sizeof(struct Sana2PacketTypeStats));
            ReleaseSemaphore(&unit->lock);
            return;
        }
    ReleaseSemaphore(&unit->lock);

    req->ios2_Req.io_Error = S2ERR_BAD_STATE;
    req->ios2_WireError = S2WERR_NOT_TRACKED;
}

static int GM_UNIQUENAME(open)(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *req,
                               ULONG unitnum, ULONG flags)
{
    struct TagItem *tags;
    struct bwfm_unit *unit;
    struct bwfm_opener *opener = NULL;
    BYTE error = 0;
    int i;

    req->ios2_Req.io_Unit = NULL;

    tags = req->ios2_BufferManagement;
    req->ios2_BufferManagement = NULL;

    if (unitnum >= BWFM_MAX_UNITS)
        error = IOERR_OPENFAIL;
    else
        unit = &LIBBASE->unit[unitnum];

    if (error == 0)
    {
        opener = AllocVec(sizeof(struct bwfm_opener), MEMF_PUBLIC | MEMF_CLEAR);
        if (opener == NULL)
            error = IOERR_OPENFAIL;
        req->ios2_BufferManagement = (APTR)opener;
    }

    if (error == 0)
    {
        NEWLIST(&opener->read_pending.mp_MsgList);
        opener->read_pending.mp_Flags = PA_IGNORE;

        for (i = 0; i < 2; i++)
            opener->rx = (BOOL (*)(APTR, APTR, ULONG))
                GetTagData(rx_tags[i], (IPTR)opener->rx, tags);
        for (i = 0; i < 3; i++)
            opener->tx = (BOOL (*)(APTR, APTR, ULONG))
                GetTagData(tx_tags[i], (IPTR)opener->tx, tags);
        opener->filter = (struct Hook *)GetTagData(S2_PacketFilter, 0, tags);
    }

    /* First open of this unit: initialise its SANA-II description */
    if (error == 0 && unit->refcount == 0)
    {
        NEWLIST((struct List *)&unit->openers);
        NEWLIST((struct List *)&unit->trackers);
        NEWLIST((struct List *)&unit->event_pending);

        unit->info.SizeAvailable = sizeof(struct Sana2DeviceQuery);
        unit->info.SizeSupplied = sizeof(struct Sana2DeviceQuery);
        unit->info.DevQueryFormat = 0;
        unit->info.DeviceLevel = 0;
        unit->info.AddrFieldSize = 8 * ETHER_ADDR_LEN;
        unit->info.MTU = 1500;
        unit->info.BPS = 0;                     /* unknown until associated */
        unit->info.HardwareType = S2WireType_Ethernet;

        /* hwaddr stays zero until firmware reports the chip MAC */

        /* Bring the chip firmware up (one-time). A failure is logged but
         * does not fail the open yet - query commands still work; the
         * datapath is stubbed until firmware + SDPCM are in place. */
        if (!load_firmware(LIBBASE))
            D(bug("[bwfm.device] firmware not loaded (chip offline)\n"));
    }

    if (error == 0)
    {
        req->ios2_Req.io_Unit = (APTR)unit;
        ObtainSemaphore(&unit->lock);
        AddTail((struct List *)&unit->openers, (struct Node *)opener);
        unit->refcount++;
        ReleaseSemaphore(&unit->lock);
    }
    else
        FreeVec(opener);

    req->ios2_Req.io_Error = error;
    return (error == 0) ? TRUE : FALSE;
}

static int GM_UNIQUENAME(close)(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *req)
{
    struct bwfm_unit *unit = (struct bwfm_unit *)req->ios2_Req.io_Unit;
    struct bwfm_opener *opener = (struct bwfm_opener *)req->ios2_BufferManagement;

    if (unit && opener)
    {
        struct IOSana2Req *r, *rnext;

        /* Reply any IO still queued against this opener before it is freed:
         * otherwise the owner's WaitIO() blocks forever and the RX pump /
         * CMD_FLUSH would later walk freed memory. Held under the unit lock so
         * the pump cannot be mid-delivery to this opener on another CPU. */
        ObtainSemaphore(&unit->lock);

        while ((r = (struct IOSana2Req *)
                RemHead(&opener->read_pending.mp_MsgList)) != NULL)
        {
            r->ios2_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&r->ios2_Req.io_Message);
        }

        ForeachNodeSafe(&unit->event_pending, r, rnext)
        {
            if (r->ios2_BufferManagement != (APTR)opener)
                continue;
            Remove((struct Node *)r);
            r->ios2_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&r->ios2_Req.io_Message);
        }

        Remove((struct Node *)opener);
        unit->refcount--;
        ReleaseSemaphore(&unit->lock);

        FreeVec(opener);
    }
    else
    {
        if (unit)
            unit->refcount--;
        if (opener)
            FreeVec(opener);
    }

    req->ios2_Req.io_Unit = NULL;
    req->ios2_BufferManagement = NULL;
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(init), 0)
ADD2OPENDEV(GM_UNIQUENAME(open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(close), 0)

/* ----------------------------------------------------------------------- */

static void TermIO(struct IOSana2Req *req)
{
    if (!(req->ios2_Req.io_Flags & IOF_QUICK))
        ReplyMsg(&req->ios2_Req.io_Message);
}

/* ----------------------------------------------------------------------- */
/* SANA-II wireless extensions (scan / crypto query for WirelessManager)    */

/*
 * S2_GETNETWORKS: run a scan and return the results as an array of tag-lists,
 * one per network, allocated from the caller-supplied memory pool (ios2_Data).
 * Mirrors what wpa_supplicant's driver_sana2 get_scan_results() reads back.
 *
 * NOTE: synchronous (BWFMScan blocks ~5s, fed by the RX pump). Fine for a
 * DoIO() caller; wpa_supplicant SendIO's this expecting an async reply, so a
 * later step must move the scan to a worker. Good enough to bring the device
 * side up and test via WiFiTest.
 */
static void get_networks(struct bwfm_unit *unit, struct IOSana2Req *req)
{
    static struct bwfm_scanresult results[BWFM_MAX_SCAN];
    APTR pool = req->ios2_Data;
    struct TagItem **lists;
    int n, i;

    if (pool == NULL)
    {
        req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
        return;
    }

    req->ios2_DataLength = 0;        /* set to the network count only on success */
    req->ios2_StatData = NULL;

    n = BWFMScan(results, BWFM_MAX_SCAN);
    if (n < 0)
        n = 0;

    lists = AllocPooled(pool, sizeof(struct TagItem *) * (n > 0 ? n : 1));
    if (lists == NULL)
    {
        req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        return;
    }

    for (i = 0; i < n; i++)
    {
        UBYTE iebuf[256];
        ULONG ielen;
        struct TagItem *tl = AllocPooled(pool, sizeof(struct TagItem) * 7);
        UBYTE *bssid = AllocPooled(pool, ETH_ALEN);
        UBYTE *ssid = AllocPooled(pool, results[i].ssid_len + 1);
        int t = 0;

        if (tl == NULL || bssid == NULL || ssid == NULL)
        {
            req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
            return;
        }

        CopyMem(results[i].bssid, bssid, ETH_ALEN);
        CopyMem(results[i].ssid, ssid, results[i].ssid_len);
        ssid[results[i].ssid_len] = '\0';

        tl[t].ti_Tag = S2INFO_SSID;    tl[t++].ti_Data = (IPTR)ssid;
        tl[t].ti_Tag = S2INFO_BSSID;   tl[t++].ti_Data = (IPTR)bssid;
        tl[t].ti_Tag = S2INFO_Channel; tl[t++].ti_Data = results[i].chanspec & 0xff;
        tl[t].ti_Tag = S2INFO_Signal;  tl[t++].ti_Data = (IPTR)(LONG)results[i].rssi;
        tl[t].ti_Tag = S2INFO_Noise;   tl[t++].ti_Data = (IPTR)(LONG)-95;

        /* Beacon IEs (incl. the RSN/WPA element) as S2INFO_InfoElements: a
         * u16 byte-count followed by the IE bytes, which wpa_supplicant parses
         * to match a WPA2 network. Without this it sees the BSS as open. */
        ielen = (ULONG)BWFMScanIE(i, iebuf, sizeof(iebuf));
        if (ielen > 0)
        {
            UBYTE *ie = AllocPooled(pool, 2 + ielen);
            if (ie != NULL)
            {
                *(UWORD *)ie = (UWORD)ielen;
                CopyMem(iebuf, ie + 2, ielen);
                tl[t].ti_Tag = S2INFO_InfoElements; tl[t++].ti_Data = (IPTR)ie;
            }
        }

        tl[t].ti_Tag = TAG_DONE;       tl[t].ti_Data = 0;
        lists[i] = tl;
    }

    req->ios2_StatData = lists;
    req->ios2_DataLength = n;
}

/* S2_GETCRYPTTYPES: report the cipher suites the firmware can do, as a byte
 * array allocated from the caller's pool (ios2_Data). */
static void get_crypttypes(struct IOSana2Req *req)
{
    APTR pool = req->ios2_Data;
    UBYTE *list;

    if (pool == NULL)
    {
        req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        return;
    }
    list = AllocPooled(pool, 3);
    if (list == NULL)
    {
        req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        return;
    }
    list[0] = S2ENC_WEP;
    list[1] = S2ENC_TKIP;
    list[2] = S2ENC_CCMP;
    req->ios2_StatData = list;
    req->ios2_DataLength = 3;
}

/*
 * Reply every queued S2_ONEVENT request whose wanted-event mask overlaps the
 * events that just occurred. While queued, ios2_WireError holds the caller's
 * mask; on reply it is narrowed to the events that fired. The matched requests
 * are unlinked under the unit lock, then replied after the lock is dropped.
 */
static void report_events(struct bwfm_unit *unit, ULONG events)
{
    struct MinList done;
    struct IOSana2Req *req, *next;
    ULONG consumed = 0, pend_mask_or = 0;
    int matched = 0, pend_count = 0;

    NEWLIST((struct List *)&done);

    ObtainSemaphore(&unit->lock);
    ForeachNodeSafe(&unit->event_pending, req, next)
    {
        ULONG hit;

        pend_count++;                       /* DIAGNOSTIC: who is listening */
        pend_mask_or |= req->ios2_WireError; /* and for which events */

        hit = req->ios2_WireError & events;
        if (hit)
        {
            Remove((struct Node *)req);
            req->ios2_WireError = hit;
            req->ios2_Req.io_Error = 0;
            AddTail((struct List *)&done, (struct Node *)req);
            consumed |= hit;
            matched++;
        }
    }
    /* Latch edge events (CONNECT/DISCONNECT) that no parked listener took, so a
     * listener arming slightly later still sees them. The synchronous scan/
     * associate blocks wpa_supplicant's event loop, so its S2_ONEVENT is often
     * not parked at the instant the join fires CONNECT - without this the event
     * is lost and wpa_supplicant times out and re-scans forever. */
    unit->pending_events |= (events & ~consumed) &
                            (S2EVENT_CONNECT | S2EVENT_DISCONNECT);
    ReleaseSemaphore(&unit->lock);

    /* DIAGNOSTIC: if CONNECT (0x200) fired but no parked mask wants it,
     * wpa_supplicant is in soft-MAC mode => hard-MAC detection failed. */
    D(bug("[bwfm.device] report_events 0x%lx -> %ld/%ld listener(s) masks 0x%lx latched 0x%lx\n",
          (unsigned long)events, (long)matched, (long)pend_count,
          (unsigned long)pend_mask_or, (unsigned long)unit->pending_events));

    while ((req = (struct IOSana2Req *)RemHead((struct List *)&done)) != NULL)
        ReplyMsg(&req->ios2_Req.io_Message);
}

/*
 * S2_SETOPTIONS: WirelessManager (wpa_supplicant) drives association here. The
 * request carries S2INFO_SSID + (with WPA_DRIVER_FLAGS_4WAY_HANDSHAKE)
 * S2INFO_Passphrase. The join (BWFMJoin) takes seconds, so we MUST NOT run it
 * inline: that blocks the caller's event loop (wpa_supplicant's), and the
 * connect notification ends up lost (no S2_ONEVENT listener armed) -> auth
 * timeout + endless re-scan. Instead copy the params, hand them to the
 * bwfm.ctrl worker, and reply S2_SETOPTIONS at once (= "association initiated");
 * the worker runs the join and raises S2EVENT_CONNECT on success, by which time
 * the caller's loop is free and its S2_ONEVENT listener is armed. A non-associate
 * S2_SETOPTIONS (no SSID) is just acknowledged.
 */
static void queue_associate(struct bwfm_unit *unit, struct IOSana2Req *req)
{
    struct TagItem *tags = (struct TagItem *)req->ios2_Data;
    UBYTE *ssid, *pass;
    ULONG ssidlen = 0, passlen = 0;

    if (tags == NULL)
        return;

    ssid = (UBYTE *)GetTagData(S2INFO_SSID, (IPTR)NULL, tags);
    if (ssid == NULL || ssid[0] == '\0')
        return;                         /* not an associate request - just ack */

    while (ssid[ssidlen] && ssidlen < sizeof(unit->assoc_ssid) - 1)
        ssidlen++;
    pass = (UBYTE *)GetTagData(S2INFO_Passphrase, (IPTR)NULL, tags);
    if (pass)
        while (pass[passlen] && passlen < sizeof(unit->assoc_pass) - 1)
            passlen++;

    /* Copy the params out of the caller's tag list (freed once we return) and
     * publish the job, then wake the worker. */
    ObtainSemaphore(&unit->lock);
    CopyMem(ssid, unit->assoc_ssid, ssidlen);
    unit->assoc_ssid[ssidlen] = '\0';
    unit->assoc_ssidlen = ssidlen;
    if (passlen)
        CopyMem(pass, unit->assoc_pass, passlen);
    unit->assoc_pass[passlen] = '\0';
    unit->assoc_passlen = passlen;
    unit->assoc_pending = TRUE;
    ReleaseSemaphore(&unit->lock);

    D(bug("[bwfm.device] queued associate \"%s\" (%s) for ctrl worker\n",
          unit->assoc_ssid, passlen ? "WPA2-PSK" : "open"));

    if (PumpBase != NULL && PumpBase->ctrl_task != NULL)
        Signal(PumpBase->ctrl_task, PumpBase->ctrl_sig);
}

/*
 * Worker side of an associate (runs in the bwfm.ctrl task): pull the queued
 * params and run the blocking BWFMJoin, then raise S2EVENT_CONNECT on success.
 * Because this is off the caller's task, the RX pump stays free to feed the
 * join its association events, and the caller's S2_ONEVENT listener is armed
 * when CONNECT fires.
 */
static void do_associate(struct bwfm_unit *unit)
{
    UBYTE ssid[33], pass[64];
    ULONG ssidlen, passlen;

    ObtainSemaphore(&unit->lock);
    if (!unit->assoc_pending)
    {
        ReleaseSemaphore(&unit->lock);
        return;
    }
    unit->assoc_pending = FALSE;
    ssidlen = unit->assoc_ssidlen;
    passlen = unit->assoc_passlen;
    CopyMem(unit->assoc_ssid, ssid, ssidlen + 1);
    CopyMem(unit->assoc_pass, pass, passlen + 1);
    ReleaseSemaphore(&unit->lock);

    D(bug("[bwfm.device] ctrl worker: associate \"%s\" (%s)\n",
          ssid, passlen ? "WPA2-PSK" : "open"));

    if (BWFMJoin(ssid, ssidlen, passlen ? pass : NULL, passlen) == 0)
    {
        unit->joined = TRUE;
        report_events(unit, S2EVENT_CONNECT);
    }
    else
        D(bug("[bwfm.device] ctrl worker: associate failed\n"));
}

/*
 * S2_GETNETWORKINFO: report the current association (at least the BSSID, which
 * wpa_supplicant's get_bssid needs to confirm the connection and settle into
 * COMPLETED instead of rescanning). The tag-list is allocated from the caller's
 * pool (ios2_Data). Returns an error if not associated.
 */
static void get_networkinfo(struct bwfm_unit *unit, struct IOSana2Req *req)
{
    APTR pool = req->ios2_Data;
    struct TagItem *tl;
    UBYTE *bssid;

    if (pool == NULL)
    {
        req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        return;
    }
    tl = AllocPooled(pool, sizeof(struct TagItem) * 2);
    bssid = AllocPooled(pool, ETH_ALEN);
    if (tl == NULL || bssid == NULL)
    {
        req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        return;
    }

    if (BWFMIoctl(BWFM_C_GET_BSSID, 0, bssid, ETH_ALEN) != 0)
    {
        req->ios2_Req.io_Error = S2ERR_BAD_STATE;       /* not associated */
        req->ios2_WireError = S2WERR_UNIT_OFFLINE;
        return;
    }

    tl[0].ti_Tag = S2INFO_BSSID; tl[0].ti_Data = (IPTR)bssid;
    tl[1].ti_Tag = TAG_DONE;     tl[1].ti_Data = 0;
    req->ios2_StatData = tl;
}

/* Reply every queued CMD_READ across all openers (CMD_FLUSH, going offline). */
static void flush_reads(struct bwfm_unit *unit, BYTE err, ULONG werr)
{
    struct bwfm_opener *opener, *onext;
    struct IOSana2Req *r;

    ObtainSemaphore(&unit->lock);
    ForeachNodeSafe(&unit->openers, opener, onext)
    {
        while ((r = (struct IOSana2Req *)
                RemHead(&opener->read_pending.mp_MsgList)) != NULL)
        {
            r->ios2_Req.io_Error = err;
            r->ios2_WireError = werr;
            ReplyMsg(&r->ios2_Req.io_Message);
        }
    }
    ReleaseSemaphore(&unit->lock);
}

static void handle_request(struct IOSana2Req *req)
{
    struct bwfm_unit *unit = (struct bwfm_unit *)req->ios2_Req.io_Unit;
    ULONG wanted = req->ios2_WireError;     /* S2_ONEVENT passes the wanted-event
                                             * mask in ios2_WireError (input); save
                                             * it before we clear the field for the
                                             * common (output) case below. */

    req->ios2_Req.io_Error = 0;
    req->ios2_WireError = 0;

    switch (req->ios2_Req.io_Command)
    {
    case S2_DEVICEQUERY:
    {
        struct Sana2DeviceQuery *q = (struct Sana2DeviceQuery *)req->ios2_StatData;
        ULONG n;

        if (q == NULL)
        {
            req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
            req->ios2_WireError = S2WERR_BAD_STATDATA;
            break;
        }
        n = q->SizeAvailable;
        if (n > sizeof(struct Sana2DeviceQuery))
            n = sizeof(struct Sana2DeviceQuery);
        CopyMem(&unit->info, q, n);
        q->SizeSupplied = n;
        break;
    }

    case S2_GETSTATIONADDRESS:
        CopyMem(unit->hwaddr, req->ios2_SrcAddr, ETHER_ADDR_LEN);
        CopyMem(unit->hwaddr, req->ios2_DstAddr, ETHER_ADDR_LEN);
        break;

    case S2_CONFIGINTERFACE:
        /* SANA-II: configure the address AND bring the interface online. Don't
         * associate here - that's S2_ONLINE's auto-join (AROSTCP) or
         * S2_SETOPTIONS (wpa_supplicant). wpa_supplicant relies on this to go
         * online (it only sends S2_ONLINE if we'd returned IS_CONFIGURED). */
        CopyMem(req->ios2_SrcAddr, unit->hwaddr, ETHER_ADDR_LEN);
        unit->online = TRUE;
        report_events(unit, S2EVENT_ONLINE);
        break;

    case S2_ONLINE:
        try_join(unit);                 /* associate to DEVS:bwfm.prefs network */
        unit->online = TRUE;
        report_events(unit, S2EVENT_ONLINE);
        break;

    case S2_OFFLINE:
        unit->online = FALSE;
        /* Reads queued while online can never complete now (the pump only
         * delivers while online), so abort them instead of hanging WaitIO. */
        flush_reads(unit, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE);
        report_events(unit, S2EVENT_OFFLINE);
        break;

    case S2_ONEVENT:
    {
        /* S2EVENT_ONLINE/OFFLINE reflect the current state - reply at once if
         * the wanted event already holds; otherwise queue the request until
         * report_events() fires (e.g. CONNECT/DISCONNECT from association). */
        const ULONG supported = S2EVENT_ONLINE | S2EVENT_OFFLINE |
                                S2EVENT_CONNECT | S2EVENT_DISCONNECT;
        ULONG mask = wanted;
        ULONG cur = unit->online ? S2EVENT_ONLINE : S2EVENT_OFFLINE;
        ULONG latched, ready;

        /* A request for events this device can never raise can never complete;
         * reject it now instead of parking it forever (SANA-II S2WERR_BAD_EVENT). */
        if (mask & ~supported)
        {
            req->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
            req->ios2_WireError = S2WERR_BAD_EVENT;
            break;
        }

        /* Deliver immediately for: ONLINE/OFFLINE that already holds (derivable
         * from unit->online), OR a CONNECT/DISCONNECT edge that fired earlier
         * with no listener parked (latched in report_events). Consuming the
         * latched bit once means no spin even though wpa_supplicant keeps CONNECT
         * in its mask permanently. */
        ObtainSemaphore(&unit->lock);
        latched = mask & unit->pending_events;
        unit->pending_events &= ~latched;
        ReleaseSemaphore(&unit->lock);
        ready = (mask & cur) | latched;

        D(bug("[bwfm.device] S2_ONEVENT mask 0x%lx (online=%d joined=%d) -> %s 0x%lx\n",
              (unsigned long)mask, unit->online, unit->joined,
              ready ? "immediate" : "queued", (unsigned long)ready));

        if (ready)
        {
            req->ios2_WireError = ready;
            break;                      /* immediate reply via TermIO */
        }
        req->ios2_Req.io_Flags &= ~IOF_QUICK;
        req->ios2_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
        req->ios2_WireError = mask;     /* restore wanted mask: report_events()
                                         * matches the queued request on it */
        ObtainSemaphore(&unit->lock);
        AddTail((struct List *)&unit->event_pending,
                &req->ios2_Req.io_Message.mn_Node);
        ReleaseSemaphore(&unit->lock);
        return;                         /* replied later by report_events() */
    }

    case S2_TRACKTYPE:
        track_type(unit, req);
        break;

    case S2_UNTRACKTYPE:
        untrack_type(unit, req);
        break;

    case S2_GETTYPESTATS:
        get_type_stats(unit, req);
        break;

    case S2_GETGLOBALSTATS:
        if (req->ios2_StatData == NULL)
        {
            req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
            req->ios2_WireError = S2WERR_BAD_STATDATA;
            break;
        }
        CopyMem(&unit->stats, req->ios2_StatData, sizeof(struct Sana2DeviceStats));
        break;

    case S2_ADDMULTICASTADDRESS:
    case S2_DELMULTICASTADDRESS:
        /* Accept: in managed mode the firmware passes multicast through, so we
         * don't program a hardware filter (yet). Stops AROSTCP's in6_addmulti
         * from erroring and lets IPv4/IPv6 multicast through. */
        break;

    case S2_GETNETWORKS:
        get_networks(unit, req);
        break;

    case S2_GETCRYPTTYPES:
        get_crypttypes(req);
        break;

    case S2_SETOPTIONS:
        queue_associate(unit, req);     /* hands the join to bwfm.ctrl; the
                                         * request is replied now (TermIO) */
        break;

    case S2_GETNETWORKINFO:
        get_networkinfo(unit, req);
        break;

    case S2_SETKEY:
        /* The firmware does the 4-way handshake (driver advertises
         * WPA_DRIVER_FLAGS_4WAY_HANDSHAKE), so host-installed PTK/GTK keys are
         * a no-op here - just succeed so wpa_supplicant proceeds. */
        break;

    case NSCMD_DEVICEQUERY:
    {
        /* NewStyle command: the result buffer is in the IOStdReq io_Data field,
         * NOT ios2_Data (a different struct offset). Reading ios2_Data wrote the
         * result to a stale pointer and corrupted memory. */
        struct IOStdReq *std = (struct IOStdReq *)req;
        struct NSDeviceQueryResult *nsq =
            (struct NSDeviceQueryResult *)std->io_Data;

        if (nsq == NULL)
        {
            req->ios2_Req.io_Error = IOERR_BADADDRESS;
            break;
        }
        nsq->DevQueryFormat = 0;
        nsq->SizeAvailable = sizeof(struct NSDeviceQueryResult);
        nsq->DeviceType = NSDEVTYPE_SANA2;
        nsq->DeviceSubType = 0;
        nsq->SupportedCommands = (UWORD *)supported_commands;
        std->io_Actual = sizeof(struct NSDeviceQueryResult);
        /* DIAGNOSTIC: wpa_supplicant calls this to detect hard-MAC (it scans
         * SupportedCommands for S2_GETNETWORKS). If this never logs, the
         * hard-MAC probe is not reaching us. */
        D(bug("[bwfm.device] NSCMD_DEVICEQUERY -> %ld cmds (hard-MAC probe)\n",
              (long)(sizeof(supported_commands)/sizeof(supported_commands[0]) - 1)));
        break;
    }

    case CMD_READ:
    {
        struct bwfm_opener *opener = (struct bwfm_opener *)req->ios2_BufferManagement;

        if (!unit->online)
        {
            D(bug("[bwfm.device] CMD_READ rejected: offline\n"));
            req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
            req->ios2_WireError = S2WERR_UNIT_OFFLINE;
            break;                          /* reply immediately with the error */
        }

        /* Queue the read; the RX pump replies it when a matching frame lands.
         * SendIO() leaves ln_Type = 0, so mark the request NT_MESSAGE here or
         * CheckIO() would treat the still-pending request as already done. */
        req->ios2_Req.io_Flags &= ~IOF_QUICK;
        req->ios2_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
        ObtainSemaphore(&unit->lock);
        AddTail(&opener->read_pending.mp_MsgList,
                &req->ios2_Req.io_Message.mn_Node);
        ReleaseSemaphore(&unit->lock);
        return;
    }

    case S2_BROADCAST:
    {
        static const UBYTE bcast[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        CopyMem((APTR)bcast, req->ios2_DstAddr, ETH_ALEN);
    }
    /* fall through */
    case CMD_WRITE:
    case S2_MULTICAST:
        tx_request(unit, req);
        break;

    case CMD_FLUSH:
        flush_reads(unit, IOERR_ABORTED, 0);
        break;

    default:
        req->ios2_Req.io_Error = IOERR_NOCMD;
        break;
    }

    TermIO(req);
}

AROS_LH1(void, begin_io, AROS_LHA(struct IOSana2Req *, req, A1),
         LIBBASETYPEPTR, LIBBASE, 5, bwfm_device)
{
    AROS_LIBFUNC_INIT
    handle_request(req);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, abort_io, AROS_LHA(struct IOSana2Req *, req, A1),
         LIBBASETYPEPTR, LIBBASE, 6, bwfm_device)
{
    AROS_LIBFUNC_INIT

    struct bwfm_unit *unit = (struct bwfm_unit *)req->ios2_Req.io_Unit;

    /* Pull a still-queued CMD_READ / S2_ONEVENT off its list and reply it
     * aborted, so a caller's WaitIO() returns instead of blocking forever.
     * Re-test ln_Type under the unit lock: the RX pump may have replied (and
     * unlinked) the request between the caller's AbortIO() and us. */
    if (unit)
    {
        ObtainSemaphore(&unit->lock);
        if (req->ios2_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE)
        {
            Remove((struct Node *)req);
            ReleaseSemaphore(&unit->lock);
            req->ios2_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&req->ios2_Req.io_Message);
            return 0;
        }
        ReleaseSemaphore(&unit->lock);
    }
    return -1;          /* not queued / already completed - nothing to abort */

    AROS_LIBFUNC_EXIT
}
