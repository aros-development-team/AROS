/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Device and configuration work for the WiFi window.

    Everything that can block lives here: a scan sits in the driver for several
    seconds, and connecting stops and starts the supplicant. The window talks to
    this process with messages only - it never calls into another task's state,
    which is exactly where WirelessManager's own GUI went wrong.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <devices/sana2.h>
#include <devices/sana2wireless.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dostags.h>
#include <devices/newstyle.h>
#include <utility/tagitem.h>

/*
 * The address has to come from the stack, and the stack may not be there: open
 * bsdsocket.library by hand rather than letting the link library open it at
 * startup, or this tool would refuse to run in exactly the case it is wanted.
 */
#define __BSDSOCKET_NOLIBBASE__

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

#include <proto/exec.h>
#include <proto/socket.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wifigui.h"
#include "locale.h"

#define WIRELESS_VAR    "AROSTCP/WirelessDevice"
#define STACK_VAR       "SYS/Packages/AROSTCP"
#define PREFS_ENVARC    "ENVARC:Sys/Wireless.prefs"
#define PREFS_ENV       "ENV:Sys/Wireless.prefs"
#define PUDDLE_SIZE     4096
#define ETH_ALEN        6
#define ASSOC_TIMEOUT   20          /* half-seconds to wait for an association */

struct MsgPort *WifiWorkerPort = NULL;
struct Task *WifiMainTask = NULL;

struct Library *SocketBase = NULL;      /* open only while an address is read */

/* SANA-II expects copy hooks even from an opener that never moves a frame. */
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

/* ------------------------------------------------------------------------- */
/* Device access                                                             */

struct WifiDev
{
    struct MsgPort      *wd_Port;
    struct IOSana2Req   *wd_Req;
    BOOL                 wd_Opened;
};

static void CloseWifiDev(struct WifiDev *dev)
{
    if (dev->wd_Opened)
        CloseDevice((struct IORequest *)dev->wd_Req);
    FreeVec(dev->wd_Req);
    if (dev->wd_Port != NULL)
        DeleteMsgPort(dev->wd_Port);
    memset(dev, 0, sizeof(*dev));
}

static BOOL OpenWifiDev(struct WifiDev *dev, CONST_STRPTR device, LONG unit)
{
    memset(dev, 0, sizeof(*dev));

    dev->wd_Port = CreateMsgPort();
    if (dev->wd_Port == NULL)
        return FALSE;
    dev->wd_Req = AllocVec(sizeof(struct IOSana2Req), MEMF_PUBLIC | MEMF_CLEAR);
    if (dev->wd_Req == NULL)
    {
        CloseWifiDev(dev);
        return FALSE;
    }

    dev->wd_Req->ios2_Req.io_Message.mn_ReplyPort = dev->wd_Port;
    dev->wd_Req->ios2_Req.io_Message.mn_Length = sizeof(struct IOSana2Req);
    dev->wd_Req->ios2_BufferManagement = (APTR)buffer_tags;

    if (OpenDevice(device, unit, (struct IORequest *)dev->wd_Req, 0) != 0)
    {
        CloseWifiDev(dev);
        return FALSE;
    }
    dev->wd_Opened = TRUE;
    return TRUE;
}

/* Split "<device> UNIT <n>", the form the network preferences write. */
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

static BOOL WifiDevice(struct WifiMsg *m)
{
    STRPTR device = NULL;

    if (m->wm_Device[0] != '\0')
        return TRUE;                    /* already resolved once */

    if (GetVar(WIRELESS_VAR, m->wm_Device, sizeof(m->wm_Device), LV_VAR) <= 0)
    {
        strcpy(m->wm_Status, (const char *)_(MSG_ST_NO_DEVICE_VAR));
        return FALSE;
    }

    m->wm_Unit = 0;
    SplitDeviceVar(m->wm_Device, &device, &m->wm_Unit);
    return TRUE;
}

/* Name the protection from the beacon IEs (a UWORD count, then the elements). */
static CONST_STRPTR IEProtection(const UBYTE *ies)
{
    ULONG len, off;
    CONST_STRPTR prot = "open";

    if (ies == NULL)
        return "?";

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

/* ------------------------------------------------------------------------- */
/* Wireless.prefs                                                            */

static STRPTR ReadPrefs(CONST_STRPTR name, LONG *lenp)
{
    BPTR fh = Open(name, MODE_OLDFILE);
    LONG size;
    STRPTR buf = NULL;

    *lenp = 0;
    if (fh == BNULL)
        return NULL;

    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    if (size > 0 && (buf = AllocVec(size + 1, MEMF_ANY | MEMF_CLEAR)) != NULL)
    {
        if (Read(fh, buf, size) == size)
            *lenp = size;
        else
        {
            FreeVec(buf);
            buf = NULL;
        }
    }
    Close(fh);
    return buf;
}

/* TRUE if the stored configuration already has a key for this network. */
static BOOL PrefsHaveNetwork(CONST_STRPTR ssid)
{
    LONG len;
    STRPTR buf = ReadPrefs(PREFS_ENVARC, &len);
    STRPTR p;
    TEXT needle[WIFI_SSID_MAX + 8];
    BOOL found = FALSE;

    if (buf == NULL)
        return FALSE;

    snprintf(needle, sizeof(needle), "ssid=\"%s\"", ssid);
    for (p = buf; (p = strstr(p, needle)) != NULL; p += 1)
    {
        /* A psk belongs to the same block, i.e. before the closing brace. */
        STRPTR end = strstr(p, "}");
        STRPTR psk = strstr(p, "psk=");

        if (psk != NULL && (end == NULL || psk < end))
        {
            found = TRUE;
            break;
        }
    }

    FreeVec(buf);
    return found;
}

/*
 * Write the configuration with this network added or replaced, keeping every
 * other block byte for byte. Rewriting the whole file from a parsed view is
 * what loses the networks a user set up elsewhere.
 */
static BOOL MergePrefs(CONST_STRPTR name, CONST_STRPTR ssid, CONST_STRPTR key)
{
    LONG len;
    STRPTR buf = ReadPrefs(name, &len);
    TEXT needle[WIFI_SSID_MAX + 8];
    BPTR fh;
    STRPTR p;

    snprintf(needle, sizeof(needle), "ssid=\"%s\"", ssid);

    fh = Open(name, MODE_NEWFILE);
    if (fh == BNULL)
    {
        FreeVec(buf);
        return FALSE;
    }

    /* Copy the old file, skipping any block that names the same network. */
    p = buf;
    while (p != NULL && *p != '\0')
    {
        STRPTR blockstart, blockend, hit;

        blockstart = strstr(p, "network={");
        if (blockstart == NULL)
        {
            FPuts(fh, p);
            break;
        }

        if (blockstart > p)
            Write(fh, p, blockstart - p);

        blockend = strstr(blockstart, "}");
        if (blockend == NULL)
            break;                          /* truncated file: drop the rest */
        blockend++;

        hit = (STRPTR)strstr((const char *)blockstart, (const char *)needle);
        if (hit == NULL || hit > blockend)
            Write(fh, blockstart, blockend - blockstart);
        p = blockend;
    }

    /* And append ours. */
    FPuts(fh, "network={\n");
    FPrintf(fh, "\tssid=\"%s\"\n", (IPTR)ssid);
    if (key != NULL && key[0] != '\0')
    {
        FPrintf(fh, "\tpsk=\"%s\"\n", (IPTR)key);
        FPuts(fh, "\tkey_mgmt=WPA-PSK\n");
    }
    else
        FPuts(fh, "\tkey_mgmt=NONE\n");
    FPuts(fh, "}\n\n");

    Close(fh);
    FreeVec(buf);
    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* The supplicant                                                            */

static BOOL StopSupplicant(void)
{
    struct Task *task;
    int tries;

    task = FindTask("C:WirelessManager");
    if (task == NULL)
        return TRUE;

    Signal(task, SIGBREAKF_CTRL_C);
    for (tries = 0; tries < 10; tries++)
    {
        if (FindTask("C:WirelessManager") == NULL)
            return TRUE;
        Delay(25);
    }
    return FALSE;
}

static BOOL StartSupplicant(CONST_STRPTR device, LONG unit)
{
    TEXT command[160];
    struct TagItem tags[] =
    {
        { SYS_Input,    (IPTR)NULL  },
        { SYS_Output,   (IPTR)NULL  },
        { SYS_Error,    (IPTR)NULL  },
        { SYS_Asynch,   (IPTR)TRUE  },
        { TAG_DONE,     0           }
    };
    int tries;

    snprintf(command, sizeof(command),
        "C:WirelessManager \"%s\" UNIT %ld NOGUI\n", device, (long)unit);
    SystemTagList(command, tags);

    for (tries = 0; tries < 20; tries++)
    {
        if (FindTask("C:WirelessManager") != NULL)
            return TRUE;
        Delay(25);
    }
    return FALSE;
}

/* The interfaces file may spell the device in any case. */
static STRPTR FindNoCase(STRPTR haystack, CONST_STRPTR needle)
{
    STRPTR p;

    if (*needle == '\0')
        return haystack;
    for (p = haystack; *p != '\0'; p++)
    {
        CONST_STRPTR n = needle;
        STRPTR q = p;

        while (*n != '\0' && ToUpper(*q) == ToUpper(*n))
        {
            q++;
            n++;
        }
        if (*n == '\0')
            return p;
    }
    return NULL;
}

static BOOL StackDBDir(STRPTR out, ULONG size);
static BOOL ScanInterfaces(CONST_STRPTR path, CONST_STRPTR device,
    STRPTR name, ULONG size, ULONG *used);

/*
 * The stack knows the device as "net0" or whatever the preferences called it,
 * so find the interface whose DEV= names our device.
 */
static BOOL InterfaceName(CONST_STRPTR device, STRPTR name, ULONG size)
{
    TEXT db[192], path[224];

    if (!StackDBDir(db, sizeof(db)))
        return FALSE;
    snprintf((char *)path, sizeof(path), "%s/interfaces", (const char *)db);

    return ScanInterfaces(path, device, name, size, NULL);
}

/* ------------------------------------------------------------------------- */
/* The interface the stack runs IP over                                      */

/*
 * Where the stack will look for its configuration, resolved the way it does
 * itself: ENV:AROSTCP/Config names the directory, and without it the stack
 * derives a path where nothing is installed. So when it is unset, point it at
 * the db directory the package ships - that one is complete.
 */
static BOOL StackDBDir(STRPTR out, ULONG size)
{
    TEXT dir[160];
    BPTR lock;

    if (GetVar("AROSTCP/Config", out, size, GVF_GLOBAL_ONLY) > 0)
    {
        if ((lock = Lock(out, ACCESS_READ)) != BNULL)
        {
            UnLock(lock);
            return TRUE;
        }
    }

    if (GetVar(STACK_VAR, dir, sizeof(dir), LV_VAR) <= 0)
        return FALSE;
    snprintf((char *)out, size, "%s/db", dir);
    if ((lock = Lock(out, ACCESS_READ)) == BNULL)
        return FALSE;
    UnLock(lock);

    /* Both directories may be absent on a machine whose network preferences
     * have never been saved; SetVar cannot create them. */
    UnLock(CreateDir("ENV:AROSTCP"));
    UnLock(CreateDir("ENVARC:AROSTCP"));
    SetVar("AROSTCP/Config", out, -1, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
    return TRUE;
}

/*
 * Both questions the interfaces file answers: which line is ours, and which
 * names are taken. Comments start with '#' or ';' and a line's first word is
 * the interface name.
 */
static BOOL ScanInterfaces(CONST_STRPTR path, CONST_STRPTR device,
    STRPTR name, ULONG size, ULONG *used)
{
    LONG len;
    STRPTR buf = ReadPrefs(path, &len);
    STRPTR line, next;
    BOOL found = FALSE;

    if (buf == NULL)
        return FALSE;

    for (line = buf; line != NULL && *line != '\0'; line = next)
    {
        STRPTR nl = (STRPTR)strchr((const char *)line, '\n');
        ULONG n = 0;
        TEXT first[16];

        next = (nl != NULL) ? nl + 1 : NULL;
        if (nl != NULL)
            *nl = '\0';

        if (*line == '#' || *line == ';' || *line == '\0')
            continue;

        while (line[n] != '\0' && line[n] != ' ' && n < sizeof(first) - 1)
        {
            first[n] = line[n];
            n++;
        }
        first[n] = '\0';
        if (n == 0)
            continue;

        if (used != NULL && Strnicmp(first, (STRPTR)"net", 3) == 0)
        {
            LONG index = atol((const char *)first + 3);

            if (index >= 0 && index < 32)
                *used |= 1UL << index;
        }

        if (!found && FindNoCase(line, device) != NULL)
        {
            snprintf((char *)name, size, "%s", (const char *)first);
            found = TRUE;
            if (used == NULL)
                break;
        }
    }

    FreeVec(buf);
    return found;
}

/*
 * Associating gets us on the air; it takes an interface for the stack to run
 * DHCP over. Rather than requiring a trip through the network preferences for
 * a wireless device that is right here, add the line ourselves - keyed on the
 * device, so a device that already has an interface keeps it untouched.
 */
static BOOL EnsureInterface(struct WifiMsg *m, BOOL *created)
{
    TEXT db[192], path[224], line[288];
    ULONG usednames = 0;
    LONG index;
    BPTR fh;

    *created = FALSE;
    if (!StackDBDir(db, sizeof(db)))
        return FALSE;
    snprintf((char *)path, sizeof(path), "%s/interfaces", (const char *)db);

    if (ScanInterfaces(path, m->wm_Device, m->wm_Interface,
            sizeof(m->wm_Interface), &usednames))
        return TRUE;

    for (index = 0; index < 32; index++)
    {
        if (!(usednames & (1UL << index)))
            break;
    }
    if (index == 32)
        return FALSE;

    snprintf((char *)m->wm_Interface, sizeof(m->wm_Interface), "net%d", index);
    snprintf((char *)line, sizeof(line), "\n%s DEV=%s UNIT=%d IP=DHCP UP\n",
        (const char *)m->wm_Interface, (const char *)m->wm_Device, m->wm_Unit);

    /* Appending keeps whatever else is configured, including the commented-out
     * examples the file ships with. */
    if ((fh = Open(path, MODE_READWRITE)) == BNULL)
        return FALSE;
    Seek(fh, 0, OFFSET_END);
    if (FPuts(fh, line) != 0)
    {
        Close(fh);
        return FALSE;
    }
    Close(fh);

    *created = TRUE;
    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* The TCP/IP stack                                                          */

static BOOL StackRunning(void)
{
    return FindTask("bsdsocket.library") != NULL;
}

/*
 * Associating is not the same as having a network: without the stack nobody
 * runs DHCP. A stack that is already up needs no help - the driver reports the
 * new association and it renews the lease by itself.
 */
static BOOL StartStack(void)
{
    TEXT dir[160], command[224];
    struct TagItem tags[] =
    {
        { SYS_Input,    (IPTR)NULL  },
        { SYS_Output,   (IPTR)NULL  },
        { SYS_Error,    (IPTR)NULL  },
        { SYS_Asynch,   (IPTR)TRUE  },
        { TAG_DONE,     0           }
    };
    int tries;

    if (StackRunning())
        return TRUE;

    if (GetVar(STACK_VAR, dir, sizeof(dir), LV_VAR) <= 0)
        return FALSE;

    snprintf(command, sizeof(command), "\"%s/C/AROSTCP\"\n", dir);
    SystemTagList(command, tags);

    for (tries = 0; tries < 40; tries++)
    {
        if (StackRunning())
            return TRUE;
        Delay(25);
    }
    return FALSE;
}

/* One address out of an ioctl reply, or "" when it is unset. */
static void AddrToText(struct ifreq *ifr, STRPTR out, ULONG size)
{
    const UBYTE *a = (const UBYTE *)
        &((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr;

    out[0] = '\0';
    if (a[0] != 0 || a[1] != 0 || a[2] != 0 || a[3] != 0)
        snprintf(out, size, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
}

static void AskIface(int sock, CONST_STRPTR ifname, ULONG req, STRPTR out,
                     ULONG size)
{
    struct ifreq ifr;

    out[0] = '\0';
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
    if (IoctlSocket(sock, req, (char *)&ifr) == 0)
        AddrToText(&ifr, out, size);
}

/* Copy one whitespace- or delimiter-terminated field. */
static void CopyField(STRPTR out, ULONG size, CONST_STRPTR src)
{
    ULONG n = 0;

    while (src[n] > ' ' && src[n] != ';' && src[n] != ',' && n < size - 1)
    {
        out[n] = src[n];
        n++;
    }
    out[n] = '\0';
}

/*
 * Gateway and name servers do not come back from an ioctl: dhclient installs
 * the route and hands the servers to the resolver, keeping neither anywhere we
 * can ask for. The lease it wrote has both, so read that - and fall back to the
 * preferences for a statically configured interface.
 */
static void FillRouteAndDNS(struct WifiMsg *m)
{
    LONG len;
    STRPTR buf, opt;

    m->wm_Gateway[0] = '\0';
    m->wm_DNS[0] = '\0';

    buf = ReadPrefs("SYS:System/Network/AROSTCP/db/dhclient.leases", &len);
    if (buf != NULL)
    {
        STRPTR p, last = NULL;

        /* The file grows by appending, so the last lease is the live one. */
        for (p = buf; (p = (STRPTR)strstr((const char *)p, "lease {")) != NULL; p++)
            last = p;

        if (last != NULL)
        {
            opt = (STRPTR)strstr((const char *)last, "option routers ");
            if (opt != NULL)
                CopyField(m->wm_Gateway, sizeof(m->wm_Gateway), opt + 15);

            opt = (STRPTR)strstr((const char *)last, "option domain-name-servers ");
            if (opt != NULL)
                CopyField(m->wm_DNS, sizeof(m->wm_DNS), opt + 27);
        }
        FreeVec(buf);
    }

    if (m->wm_Gateway[0] == '\0')
    {
        buf = ReadPrefs("ENV:AROSTCP/db/interfaces", &len);
        if (buf != NULL)
        {
            opt = FindNoCase(buf, "GW=");
            if (opt != NULL)
                CopyField(m->wm_Gateway, sizeof(m->wm_Gateway), opt + 3);
            FreeVec(buf);
        }
    }

    if (m->wm_DNS[0] == '\0')
    {
        buf = ReadPrefs("ENV:AROSTCP/db/netdb-myhost", &len);
        if (buf != NULL)
        {
            opt = FindNoCase(buf, "NAMESERVER ");
            if (opt != NULL)
                CopyField(m->wm_DNS, sizeof(m->wm_DNS), opt + 11);
            FreeVec(buf);
        }
    }
}

/* What the stack has on the interface behind our device, if anything. */
static void FillAddress(struct WifiMsg *m)
{
    int sock;

    m->wm_Address[0] = '\0';
    m->wm_Netmask[0] = '\0';
    m->wm_Broadcast[0] = '\0';
    m->wm_Interface[0] = '\0';
    m->wm_Gateway[0] = '\0';
    m->wm_DNS[0] = '\0';

    /* The name comes from the preferences, so it is known even with the stack
     * down - and telling those two cases apart is the whole point. */
    if (!InterfaceName(m->wm_Device, m->wm_Interface, sizeof(m->wm_Interface)))
        return;
    if (!StackRunning())
        return;

    SocketBase = OpenLibrary("bsdsocket.library", 0);
    if (SocketBase == NULL)
        return;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0)
    {
        AskIface(sock, m->wm_Interface, SIOCGIFADDR,
            m->wm_Address, sizeof(m->wm_Address));
        AskIface(sock, m->wm_Interface, SIOCGIFNETMASK,
            m->wm_Netmask, sizeof(m->wm_Netmask));
        AskIface(sock, m->wm_Interface, SIOCGIFBRDADDR,
            m->wm_Broadcast, sizeof(m->wm_Broadcast));
        CloseSocket(sock);
    }

    CloseLibrary(SocketBase);
    SocketBase = NULL;

    if (m->wm_Address[0] != '\0')
        FillRouteAndDNS(m);
}

/*
 * A driver is wireless if it answers NSCMD_DEVICEQUERY with S2_GETNETWORKS in
 * its command list - the same test wpa_supplicant uses to tell a FullMAC radio
 * from an ethernet card. There is no wire type to go by. Asked on an already
 * open device.
 */
static BOOL ProbeWireless(struct WifiDev *dev)
{
    struct NSDeviceQueryResult nsq;
    BOOL wireless = FALSE;

    memset(&nsq, 0, sizeof(nsq));
    ((struct IOStdReq *)dev->wd_Req)->io_Command = NSCMD_DEVICEQUERY;
    ((struct IOStdReq *)dev->wd_Req)->io_Data = &nsq;
    ((struct IOStdReq *)dev->wd_Req)->io_Length = sizeof(nsq);

    if (DoIO((struct IORequest *)dev->wd_Req) == 0 &&
        nsq.SupportedCommands != NULL)
    {
        const UWORD *cmd;

        for (cmd = nsq.SupportedCommands; *cmd != 0; cmd++)
        {
            if (*cmd == S2_GETNETWORKS)
            {
                wireless = TRUE;
                break;
            }
        }
    }

    return wireless;
}

/* The answer cannot change while the machine is running, and a status request
 * happens up to forty times during one connect. */
static BOOL DeviceIsWireless(struct WifiDev *dev, CONST_STRPTR name)
{
    static TEXT probed[WIFI_DEV_MAX];
    static BOOL probedresult;

    if (probed[0] != '\0' && Stricmp((STRPTR)probed, (STRPTR)name) == 0)
        return probedresult;

    probedresult = ProbeWireless(dev);
    snprintf(probed, sizeof(probed), "%s", name);
    return probedresult;
}

/*
 * Only the radios. DEVS:Networks also holds drivers for hardware that is not
 * in this machine, and the only way to tell one apart is to open it and ask -
 * a driver whose hardware is absent fails the open, which is the answer.
 */
static void DoDevices(struct WifiMsg *m)
{
    struct FileInfoBlock *fib;
    BPTR lock;

    m->wm_DeviceCount = 0;

    lock = Lock("DEVS:Networks", SHARED_LOCK);
    if (lock == BNULL)
    {
        strcpy(m->wm_Status, (const char *)_(MSG_ST_NO_DEVS_NETWORKS));
        return;
    }

    fib = AllocDosObject(DOS_FIB, NULL);
    if (fib != NULL && Examine(lock, fib))
    {
        while (ExNext(lock, fib) && m->wm_DeviceCount < WIFI_MAX_DEVS)
        {
            STRPTR ext = (STRPTR)strrchr((const char *)fib->fib_FileName, '.');
            TEXT name[WIFI_DEV_MAX];
            LONG unit;

            if (fib->fib_DirEntryType > 0)
                continue;
            if (ext == NULL || Stricmp(ext, ".device") != 0)
                continue;

            snprintf(name, sizeof(name), "DEVS:Networks/%s", fib->fib_FileName);

            /* One driver can hold more than one radio, so walk its units until
             * one refuses to open. Capped, because a driver that ignores the
             * unit number would otherwise fill the chooser by itself. */
            for (unit = 0; unit < WIFI_MAX_UNITS &&
                 m->wm_DeviceCount < WIFI_MAX_DEVS; unit++)
            {
                struct WifiDev dev;
                BOOL wireless;

                if (!OpenWifiDev(&dev, name, unit))
                    break;
                wireless = ProbeWireless(&dev);
                CloseWifiDev(&dev);
                if (!wireless)
                    break;      /* wireless is a property of the driver */

                snprintf(m->wm_Devices[m->wm_DeviceCount], WIFI_DEV_MAX, "%s",
                    name);
                m->wm_DeviceUnits[m->wm_DeviceCount] = unit;
                m->wm_DeviceCount++;
            }
        }
    }
    if (fib != NULL)
        FreeDosObject(DOS_FIB, fib);
    UnLock(lock);

    if (m->wm_DeviceCount == 0)
        strcpy(m->wm_Status, (const char *)_(MSG_ST_NO_WIRELESS_DEVICE));
}

/* ------------------------------------------------------------------------- */
/* Commands                                                                  */

/* Fills in wm_Associated/wm_SSID. Returns FALSE only if the device is absent. */
static BOOL DoStatus(struct WifiMsg *m)
{
    struct WifiDev dev;
    APTR pool;

    m->wm_Associated = FALSE;
    m->wm_Wireless = FALSE;
    m->wm_SSID[0] = '\0';
    m->wm_BSSID[0] = '\0';
    m->wm_MAC[0] = '\0';

    if (!WifiDevice(m))
        return FALSE;
    if (!OpenWifiDev(&dev, m->wm_Device, m->wm_Unit))
    {
        snprintf(m->wm_Status, sizeof(m->wm_Status), (const char *)_(MSG_ST_CANNOT_OPEN),
            m->wm_Device, (long)m->wm_Unit);
        return FALSE;
    }

    m->wm_Wireless = DeviceIsWireless(&dev, m->wm_Device);

    pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, PUDDLE_SIZE, PUDDLE_SIZE);
    if (pool != NULL)
    {
        dev.wd_Req->ios2_Req.io_Command = S2_GETSTATIONADDRESS;
        if (DoIO((struct IORequest *)dev.wd_Req) == 0)
        {
            const UBYTE *a = dev.wd_Req->ios2_SrcAddr;

            snprintf(m->wm_MAC, sizeof(m->wm_MAC),
                "%02x:%02x:%02x:%02x:%02x:%02x",
                a[0], a[1], a[2], a[3], a[4], a[5]);
        }

        dev.wd_Req->ios2_Req.io_Command = S2_GETNETWORKINFO;
        dev.wd_Req->ios2_Data = pool;
        dev.wd_Req->ios2_StatData = NULL;
        if (DoIO((struct IORequest *)dev.wd_Req) == 0 &&
            dev.wd_Req->ios2_StatData != NULL)
        {
            STRPTR ssid = (STRPTR)GetTagData(S2INFO_SSID, (IPTR)NULL,
                dev.wd_Req->ios2_StatData);
            UBYTE *bssid = (UBYTE *)GetTagData(S2INFO_BSSID, (IPTR)NULL,
                dev.wd_Req->ios2_StatData);

            m->wm_Associated = TRUE;
            if (ssid != NULL)
                snprintf(m->wm_SSID, sizeof(m->wm_SSID), "%s", ssid);
            if (bssid != NULL)
                snprintf(m->wm_BSSID, sizeof(m->wm_BSSID),
                    "%02x:%02x:%02x:%02x:%02x:%02x",
                    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        }
        DeletePool(pool);
    }

    CloseWifiDev(&dev);

    if (m->wm_Associated)
    {
        FillAddress(m);
        if (m->wm_Address[0] != '\0')
            snprintf(m->wm_Status, sizeof(m->wm_Status),
                (const char *)_(MSG_ST_CONNECTED),
                (const char *)(m->wm_SSID[0] != '\0' ? (CONST_STRPTR)m->wm_SSID
                                                     : _(MSG_ST_A_NETWORK)),
                m->wm_Address);
        else if (m->wm_Interface[0] == '\0')
            snprintf(m->wm_Status, sizeof(m->wm_Status),
                (const char *)_(MSG_ST_CONNECTED_NO_IFACE_PREFS), m->wm_SSID,
                m->wm_Device);
        else if (!StackRunning())
            snprintf(m->wm_Status, sizeof(m->wm_Status),
                (const char *)_(MSG_ST_CONNECTED_NO_STACK), m->wm_SSID);
        else
            snprintf(m->wm_Status, sizeof(m->wm_Status),
                (const char *)_(MSG_ST_CONNECTED_WAITING),
                (const char *)(m->wm_SSID[0] != '\0' ? (CONST_STRPTR)m->wm_SSID
                                                     : _(MSG_ST_A_NETWORK)));
    }
    else if (!m->wm_Wireless)
        snprintf(m->wm_Status, sizeof(m->wm_Status),
            (const char *)_(MSG_ST_NOT_WIRELESS), m->wm_Device);
    else
        strcpy(m->wm_Status, (const char *)_(MSG_ST_NOT_CONNECTED));

    return TRUE;
}

static void DoScan(struct WifiMsg *m)
{
    struct WifiDev dev;
    APTR pool;

    m->wm_Count = 0;

    /* So the window can tick the network we are on while listing the rest. */
    DoStatus(m);

    if (!WifiDevice(m))
        return;
    if (!OpenWifiDev(&dev, m->wm_Device, m->wm_Unit))
    {
        snprintf(m->wm_Status, sizeof(m->wm_Status), (const char *)_(MSG_ST_CANNOT_OPEN),
            m->wm_Device, (long)m->wm_Unit);
        return;
    }

    pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, PUDDLE_SIZE * 8, PUDDLE_SIZE);
    if (pool == NULL)
    {
        strcpy(m->wm_Status, (const char *)_(MSG_ST_OUT_OF_MEMORY));
        CloseWifiDev(&dev);
        return;
    }

    dev.wd_Req->ios2_Req.io_Command = S2_GETNETWORKS;
    dev.wd_Req->ios2_Data = pool;
    dev.wd_Req->ios2_StatData = NULL;
    dev.wd_Req->ios2_DataLength = 0;

    if (DoIO((struct IORequest *)dev.wd_Req) == 0)
    {
        struct TagItem **lists = dev.wd_Req->ios2_StatData;
        ULONG count = dev.wd_Req->ios2_DataLength, i;
        LONG j;

        for (i = 0; i < count && i < WIFI_MAX_NETS && lists != NULL; i++)
        {
            struct TagItem *tl = lists[i];
            struct WifiNet *n = &m->wm_Nets[m->wm_Count];
            STRPTR ssid;

            if (tl == NULL)
                continue;

            ssid = (STRPTR)GetTagData(S2INFO_SSID, (IPTR)"", tl);
            if (ssid[0] == '\0')
                continue;               /* hidden: nothing to click on */

            /*
             * A scan reports one entry per BSS, so a dual-band access point -
             * or a mesh - answers several times under the same name. Keep the
             * strongest of each name: the list is meant to be a list of
             * networks, not of radios.
             */
            for (j = 0; j < m->wm_Count; j++)
            {
                if (strcmp((const char *)m->wm_Nets[j].wn_SSID,
                           (const char *)ssid) == 0)
                    break;
            }
            if (j < m->wm_Count)
            {
                if ((LONG)GetTagData(S2INFO_Signal, 0, tl) <=
                    m->wm_Nets[j].wn_Signal)
                    continue;           /* the one we have is better */
                n = &m->wm_Nets[j];     /* replace it in place */
            }

            snprintf(n->wn_SSID, sizeof(n->wn_SSID), "%s", ssid);
            n->wn_Signal = (LONG)GetTagData(S2INFO_Signal, 0, tl);
            n->wn_Channel = (LONG)GetTagData(S2INFO_Channel, 0, tl);
            snprintf(n->wn_Protection, sizeof(n->wn_Protection), "%s",
                IEProtection((const UBYTE *)
                    GetTagData(S2INFO_InfoElements, (IPTR)NULL, tl)));
            n->wn_Known = PrefsHaveNetwork(n->wn_SSID);
            if (j == m->wm_Count)
                m->wm_Count++;
        }
        /* Strongest first: that is the order they are likely wanted in. */
        for (j = 0; j < m->wm_Count - 1; j++)
        {
            LONG k;

            for (k = 0; k < m->wm_Count - 1 - j; k++)
            {
                if (m->wm_Nets[k].wn_Signal < m->wm_Nets[k + 1].wn_Signal)
                {
                    struct WifiNet t = m->wm_Nets[k];

                    m->wm_Nets[k] = m->wm_Nets[k + 1];
                    m->wm_Nets[k + 1] = t;
                }
            }
        }

        if (m->wm_Count == 1)
            strcpy(m->wm_Status, (const char *)_(MSG_ST_ONE_NETWORK));
        else
            snprintf(m->wm_Status, sizeof(m->wm_Status),
                (const char *)_(MSG_ST_NETWORKS), (long)m->wm_Count);
    }
    else
        strcpy(m->wm_Status, (const char *)_(MSG_ST_SCAN_FAILED));

    DeletePool(pool);
    CloseWifiDev(&dev);
}

/*
 * A failed attempt should leave the stored configuration exactly as it was:
 * the supplicant reads the passphrase from the file, so trying one means
 * writing it, and a typo would otherwise overwrite a passphrase that worked.
 */
struct PrefsBackup
{
    STRPTR  pb_Env;
    LONG    pb_EnvLen;
    STRPTR  pb_Envarc;
    LONG    pb_EnvarcLen;
    BOOL    pb_Taken;
};

static void SavePrefs(struct PrefsBackup *b)
{
    b->pb_Env = ReadPrefs(PREFS_ENV, &b->pb_EnvLen);
    b->pb_Envarc = ReadPrefs(PREFS_ENVARC, &b->pb_EnvarcLen);
    b->pb_Taken = TRUE;
}

static void WriteWhole(CONST_STRPTR name, STRPTR buf, LONG len)
{
    BPTR fh;

    /* No file before the attempt means the network was unknown, and unknown is
     * what it has to go back to - or the next Connect trusts a bad key. */
    if (buf == NULL)
    {
        DeleteFile(name);
        return;
    }
    if ((fh = Open(name, MODE_NEWFILE)) == BNULL)
        return;
    Write(fh, buf, len);
    Close(fh);
}

static void RestorePrefs(struct PrefsBackup *b)
{
    if (!b->pb_Taken)
        return;
    WriteWhole(PREFS_ENV, b->pb_Env, b->pb_EnvLen);
    WriteWhole(PREFS_ENVARC, b->pb_Envarc, b->pb_EnvarcLen);
}

static void FreePrefsBackup(struct PrefsBackup *b)
{
    FreeVec(b->pb_Env);
    FreeVec(b->pb_Envarc);
    b->pb_Env = NULL;
    b->pb_Envarc = NULL;
    b->pb_Taken = FALSE;
}

/*
 * Was this network on the air the last time we looked? What is not there
 * cannot be blamed on the passphrase, and the answer is already in the scan
 * results the window is showing - no need to spend seconds scanning again.
 */
static BOOL SSIDInScan(struct WifiMsg *m)
{
    LONG i;

    for (i = 0; i < m->wm_Count; i++)
    {
        if (strcmp((const char *)m->wm_Nets[i].wn_SSID,
                   (const char *)m->wm_SSID) == 0)
            return TRUE;
    }
    return FALSE;
}

/*
 * The attempt itself, from starting the supplicant to having an address.
 * WRES_NEEDKEY on the way out means the passphrase is what to suspect. The
 * name is passed in because DoStatus() clears wm_SSID whenever the device is
 * not associated, which is every case worth reporting.
 */
static void RunConnect(struct WifiMsg *m, CONST_STRPTR wanted,
    BOOL inrange)
{
    int tries;

    if (!StopSupplicant())
    {
        strcpy(m->wm_Status, (const char *)_(MSG_ST_WM_NO_STOP));
        return;
    }
    if (!StartSupplicant(m->wm_Device, m->wm_Unit))
    {
        strcpy(m->wm_Status, (const char *)_(MSG_ST_WM_NO_START));
        return;
    }

    /* The supplicant now scans, associates and runs the handshake; watch the
     * driver until it reports an association. */
    for (tries = 0; tries < ASSOC_TIMEOUT; tries++)
    {
        Delay(25);
        if (DoStatus(m) && m->wm_Associated)
        {
            BOOL created;

            m->wm_Result = WRES_OK;

            if (!EnsureInterface(m, &created))
            {
                snprintf(m->wm_Status, sizeof(m->wm_Status),
                    (const char *)_(MSG_ST_CONNECTED_NO_IFACE), wanted,
                    m->wm_Device);
                return;
            }

            /* A running stack read the interfaces file when it started, so a
             * line added now means nothing to it until it is restarted. */
            if (created && StackRunning())
            {
                snprintf(m->wm_Status, sizeof(m->wm_Status),
                    (const char *)_(MSG_ST_RESTART_STACK), wanted,
                    m->wm_Interface);
                return;
            }

            if (!StartStack())
            {
                snprintf(m->wm_Status, sizeof(m->wm_Status),
                    (const char *)_(MSG_ST_STACK_NO_START), wanted);
                return;
            }

            /* Associating is quicker than DHCP, so give the lease a moment to
             * arrive - the address is the answer the user is waiting for. */
            for (tries = 0; tries < ASSOC_TIMEOUT; tries++)
            {
                if (m->wm_Address[0] != '\0')
                    return;
                Delay(25);
                DoStatus(m);

                /*
                 * WPA2 lets anybody associate and only then runs the
                 * handshake, so being thrown off again seconds later is what
                 * a wrong passphrase looks like from out here.
                 */
                if (!m->wm_Associated && m->wm_Protected)
                {
                    m->wm_Result = WRES_NEEDKEY;
                    snprintf(m->wm_Status, sizeof(m->wm_Status),
                        (const char *)_(MSG_ST_WRONG_PASSWORD), wanted);
                    return;
                }
            }
            return;
        }
    }

    m->wm_Result = WRES_FAILED;

    /*
     * Never associating at all is not the passphrase's fault either - unless
     * the network was there when we scanned, in which case the handshake
     * failing before we ever caught it associated is the likely story.
     */
    if (m->wm_Protected && inrange)
    {
        m->wm_Result = WRES_NEEDKEY;
        snprintf(m->wm_Status, sizeof(m->wm_Status),
            (const char *)_(MSG_ST_WRONG_PASSWORD), wanted);
        return;
    }

    snprintf(m->wm_Status, sizeof(m->wm_Status),
        (const char *)_(MSG_ST_COULD_NOT_CONNECT), wanted);
}

static void DoConnect(struct WifiMsg *m)
{
    struct PrefsBackup backup = { NULL, 0, NULL, 0, FALSE };
    TEXT wanted[WIFI_SSID_MAX];
    BOOL inrange;

    m->wm_Result = WRES_FAILED;

    if (!WifiDevice(m))
        return;
    if (m->wm_SSID[0] == '\0')
    {
        strcpy(m->wm_Status, (const char *)_(MSG_ST_NO_NETWORK_SELECTED));
        return;
    }

    /* With scan results in hand, a network missing from them is worth saying
     * so about instead of spending half a minute failing to associate. */
    snprintf((char *)wanted, sizeof(wanted), "%s", (const char *)m->wm_SSID);
    inrange = SSIDInScan(m);
    if (m->wm_Count > 0 && !inrange)
    {
        snprintf(m->wm_Status, sizeof(m->wm_Status),
            (const char *)_(MSG_ST_NOT_IN_RANGE), m->wm_SSID);
        return;
    }

    /*
     * A protected network needs a passphrase, but not if we already stored one
     * for it - that is what "remembered" means. Asking the window for it is a
     * separate round trip.
     */
    if (m->wm_Protected && m->wm_Key[0] == '\0' &&
        !PrefsHaveNetwork(m->wm_SSID))
    {
        m->wm_Result = WRES_NEEDKEY;
        snprintf(m->wm_Status, sizeof(m->wm_Status),
            (const char *)_(MSG_ST_ENTER_PASSWORD), m->wm_SSID);
        return;
    }

    /* Only rewrite the stored configuration when we were given something new
     * to put in it, so a reconnect to a remembered network keeps its key. */
    if (!m->wm_Protected || m->wm_Key[0] != '\0')
    {
        SavePrefs(&backup);
        if (!MergePrefs(PREFS_ENVARC, m->wm_SSID, m->wm_Key) ||
            !MergePrefs(PREFS_ENV, m->wm_SSID, m->wm_Key))
        {
            strcpy(m->wm_Status, (const char *)_(MSG_ST_CANNOT_WRITE_PREFS));
            RestorePrefs(&backup);
            FreePrefsBackup(&backup);
            return;
        }
    }

    RunConnect(m, wanted, inrange);

    /* Blaming the passphrase and keeping it are contradictory: put the file
     * back, so an untested passphrase never becomes the remembered one. */
    if (m->wm_Result == WRES_NEEDKEY)
        RestorePrefs(&backup);
    FreePrefsBackup(&backup);
}

/*
 * Leaving means two things: stop the supplicant, or it associates again within
 * seconds, and tell the firmware to drop the link.
 */
static void DoDisconnect(struct WifiMsg *m)
{
    struct WifiDev dev;
    struct TagItem tags[2];

    if (!WifiDevice(m))
        return;

    StopSupplicant();

    if (!OpenWifiDev(&dev, m->wm_Device, m->wm_Unit))
    {
        snprintf(m->wm_Status, sizeof(m->wm_Status), (const char *)_(MSG_ST_CANNOT_OPEN),
            m->wm_Device, (long)m->wm_Unit);
        return;
    }

    tags[0].ti_Tag = S2INFO_Disassociate;
    tags[0].ti_Data = TRUE;
    tags[1].ti_Tag = TAG_DONE;
    tags[1].ti_Data = 0;

    dev.wd_Req->ios2_Req.io_Command = S2_SETOPTIONS;
    dev.wd_Req->ios2_Data = tags;
    DoIO((struct IORequest *)dev.wd_Req);
    CloseWifiDev(&dev);

    DoStatus(m);
    if (!m->wm_Associated)
        strcpy(m->wm_Status, (const char *)_(MSG_ST_DISCONNECTED));
}

/* ------------------------------------------------------------------------- */

VOID WifiWorker(VOID)
{
    struct MsgPort *port = CreateMsgPort();
    struct WifiMsg *m;
    BOOL done = FALSE;

    WifiWorkerPort = port;
    Signal(WifiMainTask, SIGF_SINGLE);       /* port is up (or NULL on failure) */
    if (port == NULL)
        return;

    while (!done)
    {
        WaitPort(port);
        while ((m = (struct WifiMsg *)GetMsg(port)) != NULL)
        {
            m->wm_Result = WRES_OK;
            m->wm_Status[0] = '\0';

            switch (m->wm_Cmd)
            {
            case WCMD_DEVICES:
                DoDevices(m);
                break;

            case WCMD_STATUS:
                DoStatus(m);
                break;

            case WCMD_SCAN:
                DoScan(m);
                break;

            case WCMD_CONNECT:
                DoConnect(m);
                break;

            case WCMD_DISCONNECT:
                DoDisconnect(m);
                break;

            case WCMD_QUIT:
                done = TRUE;
                break;
            }

            ReplyMsg(&m->wm_Msg);
        }
    }

    /* The window waits for this before it frees the port's last message. */
    DeleteMsgPort(port);
    WifiWorkerPort = NULL;
    Signal(WifiMainTask, SIGF_SINGLE);
}
