/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Find a working Ethernet device and write an AROSTCP interfaces
          line for it.

    Nothing populates ENVARC:AROSTCP on a fresh install, and a Live CD has
    no writable ENVARC: at all, so ${AROSTCP/AutoRun} is empty and the stack
    never starts - the machine boots with a browser and no network. The
    interfaces file has to name one specific device, and DEVS:Networks ships
    around eighteen of them, so the right one has to be found at boot.

    Listing them all is not an option: AROSTCP's iface_make() puts up a
    "Fatal error in NetDB file interfaces ... Memory exhausted" requester for
    every entry whose device does not open.

    So probe exactly the way iface_make() does - OpenDevice(), S2_DEVICEQUERY,
    S2_GETSTATIONADDRESS - and only write an entry for a device that answers
    all three. A device that passes here is one AROSTCP can bring up.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <devices/sana2.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>

#include <oop/oop.h>
#include <hidd/pci.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>

#include <string.h>

/* utility.library's Stricmp is not pulled in by proto/dos.h, and this is the
   only case-insensitive compare needed here. */
static int NameEquals(CONST_STRPTR a, CONST_STRPTR b)
{
    while (*a != '\0' && *b != '\0')
    {
        TEXT ca = *a++, cb = *b++;

        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb)
            return 1;
    }

    return (*a == *b) ? 0 : 1;
}

const TEXT version[] = "$VER: netdetect 1.0 (09.09.2026)";

#define NETWORKS_DIR    "DEVS:Networks"
#define DEFAULT_IFACES  "ENV:AROSTCP/db/interfaces"
#define DEFAULT_IFNAME  "eth0"

#define ARG_TEMPLATE    "INTERFACES/K,IFNAME/K,DEVICE/K,QUIET/S"

enum
{
    ARG_INTERFACES,
    ARG_IFNAME,
    ARG_DEVICE,
    ARG_QUIET,
    ARG_COUNT
};

/* A driver copies packets through these and defaults them to NULL, so an
   empty list leaves a live opener with null hooks. Never hand it one. */
static BOOL CopyToBuff(APTR to, APTR from, ULONG len)
{
    CopyMem(from, to, len);
    return TRUE;
}

static BOOL CopyFromBuff(APTR to, APTR from, ULONG len)
{
    CopyMem(from, to, len);
    return TRUE;
}

static struct TagItem buffer_management[] =
{
    { S2_CopyToBuff,   (IPTR)CopyToBuff   },
    { S2_CopyFromBuff, (IPTR)CopyFromBuff },
    { TAG_END,         0                  }
};

/* Drivers that cannot serve as a plain DHCP Ethernet interface at boot:
   the _debug variants flood the serial log (and livelock at DEBUG=1),
   ppp is not Ethernet, and the wireless ones need an association before
   they carry traffic. S2_DEVICEQUERY's wire type catches the last two as
   well; the names are here so we do not even open them. */
static CONST_STRPTR skip_devices[] =
{
    "ppp.device",
    "atheros5000.device",
    "prism2.device",
    "realtek8180.device",
    NULL
};

static BOOL IsSkipped(CONST_STRPTR name)
{
    ULONG i;

    if (strstr(name, "_debug") != NULL)
        return TRUE;

    for (i = 0; skip_devices[i] != NULL; i++)
    {
        if (NameEquals(name, skip_devices[i]) == 0)
            return TRUE;
    }

    return FALSE;
}

/* TRUE if the device <path> (a file path, or the name of a device already
   in the system list) opens as an Ethernet device and answers the two
   queries AROSTCP makes before it will use an interface. */
static BOOL ProbeDeviceAt(CONST_STRPTR path)
{
    struct MsgPort *port;
    struct IOSana2Req *req;
    struct Sana2DeviceQuery query;
    BOOL usable = FALSE;

    port = CreateMsgPort();
    if (port == NULL)
        return FALSE;

    req = (struct IOSana2Req *)CreateIORequest(port, sizeof(struct IOSana2Req));
    if (req != NULL)
    {
        req->ios2_BufferManagement = (APTR)buffer_management;

        /* SANA2OPF_MINE: a device somebody else already owns must come back
           IOERR_UNITBUSY and be left completely alone. e1000.device (at
           least) re-runs its hardware init on every open, so probing a NIC
           that the stack is already using resets it underneath the stack and
           hangs the machine. Exclusive open both avoids that and asks the
           right question - autoconfig only wants a card nobody is using. */
        if (OpenDevice(path, 0, (struct IORequest *)req, SANA2OPF_MINE) == 0)
        {
            req->ios2_Req.io_Command = S2_DEVICEQUERY;
            req->ios2_StatData = &query;
            memset(&query, 0, sizeof(query));
            query.SizeAvailable = sizeof(query);
            query.DevQueryFormat = 0;
            DoIO((struct IORequest *)req);

            if (req->ios2_Req.io_Error == 0
                && query.HardwareType == S2WireType_Ethernet)
            {
                req->ios2_Req.io_Command = S2_GETSTATIONADDRESS;
                req->ios2_StatData = NULL;
                DoIO((struct IORequest *)req);

                if (req->ios2_Req.io_Error == 0)
                    usable = TRUE;
            }

            CloseDevice((struct IORequest *)req);
        }

        DeleteIORequest((struct IORequest *)req);
    }

    DeleteMsgPort(port);

    return usable;
}

/* As ProbeDeviceAt() for a driver in DEVS:Networks. */
static BOOL ProbeDevice(CONST_STRPTR name)
{
    TEXT path[256];

    strlcpy(path, NETWORKS_DIR "/", sizeof(path));
    strlcat(path, name, sizeof(path));
    return ProbeDeviceAt(path);
}

/* ------------------------------------------------------------------------
   USB network adapters.

   The USB network classes (RNDIS - which phones, hotspots and KVM gadgets
   use for tethering - CDC Ethernet, ASIX, ...) do not live in
   DEVS:Networks: each class adds its SANA-II device to the system list
   when Poseidon loads it, and a unit only opens while an adapter is bound.
   Only devices already in the list are tried, so nothing is loaded from
   disk, and only a unit that really opens is written, as an interfaces
   entry whose device does not open puts up a requester.
   ------------------------------------------------------------------------ */

static CONST_STRPTR usb_devices[] =
{
    "usbrndis.device",
    "usbcdceth.device",
    "usbasixeth.device",
    "usblan78xx.device",
    "usbmoschipeth.device",
    "usbpegasus.device",
    NULL
};

static BOOL FindDeviceByUSB(STRPTR found, ULONG size, BOOL quiet)
{
    ULONG i;

    for (i = 0; usb_devices[i] != NULL; i++)
    {
        BOOL present;

        Forbid();
        present = FindName(&SysBase->DeviceList, usb_devices[i]) != NULL;
        Permit();
        if (!present)
            continue;

        if (ProbeDeviceAt(usb_devices[i]))
        {
            strlcpy(found, usb_devices[i], size);
            if (!quiet)
                Printf("netdetect: USB adapter on %s\n", (IPTR)usb_devices[i]);
            return TRUE;
        }
    }
    if (!quiet)
        PutStr("netdetect: no USB network adapter\n");
    return FALSE;
}

/* Writes "<ifname> DEV=DEVS:Networks/<device> UNIT=0 IP=DHCP UP", or
   DEV=<device> for a device already in the system list (USB adapters).
   Exactly one interface: dhclient, built on the plain socket API (no
   SO_BINDTODEVICE), exits with "can only support hosts with a single
   network interface" when given a second one. */
static BOOL WriteInterfaces(CONST_STRPTR file, CONST_STRPTR ifname,
    CONST_STRPTR device, BOOL resident)
{
    BPTR fh;
    BOOL ok = FALSE;

    fh = Open(file, MODE_NEWFILE);
    if (fh != BNULL)
    {
        if (resident)
            ok = FPrintf(fh, "%s DEV=%s UNIT=0 IP=DHCP UP\n",
                         ifname, device) >= 0;
        else
            ok = FPrintf(fh, "%s DEV=%s/%s UNIT=0 IP=DHCP UP\n",
                         ifname, NETWORKS_DIR, device) >= 0;
        Close(fh);
    }

    return ok;
}

/* First usable device in DEVS:Networks, or NULL. The name is copied into
   <found>. */
/* ------------------------------------------------------------------------
   PCI-based detection.

   Opening a card to find out whether it is there costs more than it looks:
   e1000's Close() does not stop the unit (its own FIXME), so a probe's
   open/close cycle consumes the driver's one-time bring-up and the stack that
   opens afterwards inherits an unconfigured unit and moves no traffic.

   The card can be identified without touching the driver at all. Every AROS
   network driver finds its own hardware by enumerating PCI and matching
   vendor/product IDs; do the same here and map the match to the driver that
   claims it. No OpenDevice, so nothing is consumed and AROSTCP remains the
   card's first opener.

   A product of 0 matches any device from that vendor whose PCI class says
   network controller, which is what lets one entry cover a whole family. -->
   ------------------------------------------------------------------------ */

struct NicMatch
{
    UWORD vendor;
    UWORD product;      /* 0 = any network-class device from this vendor */
    CONST_STRPTR driver;
};

static const struct NicMatch nic_table[] =
{
    { 0x10ec, 0x8139, "rtl8139.device"     },  /* Realtek 8139 (QEMU rtl8139) */
    { 0x10ec, 0x8136, "rtl816x.device"     },  /* Realtek 8101E/8102E */
    { 0x10ec, 0x8167, "rtl816x.device"     },
    { 0x10ec, 0x8168, "rtl816x.device"     },  /* Realtek 8111/8168 */
    { 0x10ec, 0x8169, "rtl816x.device"     },
    { 0x1022, 0x2000, "pcnet32.device"     },  /* AMD PCnet (QEMU pcnet) */
    { 0x1106, 0x3106, "rhine.device"       },  /* VIA Rhine */
    { 0x1039, 0x0900, "sis900.device"      },  /* SiS 900 */
    { 0x14e4, 0x4401, "broadcom4400.device"},  /* Broadcom 440x */
    { 0x14e4, 0x4400, "broadcom4400.device"},
    { 0x10b7, 0x0000, "etherlink3.device"  },  /* 3Com */
    { 0x10de, 0x0000, "nvidianet.device"   },  /* nForce */
    { 0x8086, 0x1229, "intelpro100.device" },  /* Intel PRO/100 */
    { 0x8086, 0x0000, "e1000.device"       },  /* Intel gigabit (QEMU e1000) */
    { 0x0000, 0x0000, NULL                 }
};

struct PciScan
{
    OOP_AttrBase   devattr;
    CONST_STRPTR   driver;      /* set when a match is found */
    UWORD          vendor;
    UWORD          product;
};

AROS_UFH3(void, NicEnumerator,
    AROS_UFHA(struct Hook *,  hook,      A0),
    AROS_UFHA(OOP_Object *,   pciDevice, A2),
    AROS_UFHA(APTR,           message,   A1))
{
    AROS_USERFUNC_INIT

    struct PciScan *scan = (struct PciScan *)hook->h_Data;
    IPTR vendor = 0, product = 0;
    int i;

    /* No early return: AROS_USERFUNC_EXIT closes the function's scope, so it
       has to appear exactly once, at the end. First match wins. */
    if (scan->driver == NULL)
    {
        OOP_GetAttr(pciDevice, scan->devattr + aoHidd_PCIDevice_VendorID, &vendor);
        OOP_GetAttr(pciDevice, scan->devattr + aoHidd_PCIDevice_ProductID, &product);

        for (i = 0; nic_table[i].driver != NULL; i++)
        {
            if (nic_table[i].vendor != (UWORD)vendor)
                continue;
            if (nic_table[i].product != 0 && nic_table[i].product != (UWORD)product)
                continue;

            scan->driver  = nic_table[i].driver;
            scan->vendor  = (UWORD)vendor;
            scan->product = (UWORD)product;
            break;
        }
    }

    AROS_USERFUNC_EXIT
}

/* Returns TRUE and fills `found` when a known NIC is present in PCI and its
   driver exists in DEVS:Networks. */
static BOOL FindDeviceByPCI(STRPTR found, ULONG size, BOOL quiet)
{
    struct Library *OOPBase;
    OOP_Object *pci;
    struct PciScan scan = { 0, NULL, 0, 0 };
    BOOL got = FALSE;

    OOPBase = OpenLibrary("oop.library", 0);
    if (OOPBase == NULL)
    {
        if (!quiet)
            PutStr("netdetect: no oop.library\n");
        return FALSE;
    }

    scan.devattr = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (scan.devattr != 0)
    {
        pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
        if (pci != NULL)
        {
            struct Hook FindHook;
            /* Class 0x02 is "network controller"; restricting the enumeration
               keeps us away from every other device on the bus. */
            struct TagItem reqs[] =
            {
                { tHidd_PCI_Class, 0x02 },
                { TAG_DONE,        0    }
            };

            memset(&FindHook, 0, sizeof(FindHook));
            FindHook.h_Entry = (IPTR (*)())NicEnumerator;
            FindHook.h_Data  = &scan;

            HIDD_PCI_EnumDevices(pci, &FindHook, reqs);
            OOP_DisposeObject(pci);
        }
        else if (!quiet)
            PutStr("netdetect: no PCI subsystem\n");

        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    }

    if (scan.driver != NULL)
    {
        TEXT path[256];

        /* A driver named in the table still has to be installed. */
        strlcpy(path, NETWORKS_DIR, sizeof(path));
        strlcat(path, "/", sizeof(path));
        strlcat(path, scan.driver, sizeof(path));

        if (!IsSkipped(scan.driver))
        {
            BPTR lock = Lock(path, SHARED_LOCK);
            if (lock != BNULL)
            {
                UnLock(lock);
                strlcpy(found, scan.driver, size);
                got = TRUE;
                if (!quiet)
                {
                    Printf("netdetect: PCI %04lx:%04lx -> %s\n",
                           (IPTR)scan.vendor, (IPTR)scan.product,
                           (IPTR)scan.driver);
                }
            }
            else if (!quiet)
            {
                Printf("netdetect: PCI %04lx:%04lx wants %s, which is not installed\n",
                       (IPTR)scan.vendor, (IPTR)scan.product, (IPTR)scan.driver);
            }
        }
    }
    else if (!quiet)
        PutStr("netdetect: no known network controller on PCI\n");

    CloseLibrary(OOPBase);

    return got;
}

static BOOL FindDevice(STRPTR found, ULONG size, BOOL quiet)
{
    struct FileInfoBlock *fib;
    BPTR dir;
    BOOL got = FALSE;

    dir = Lock(NETWORKS_DIR, SHARED_LOCK);
    if (dir == BNULL)
    {
        if (!quiet)
            PutStr("netdetect: no " NETWORKS_DIR "\n");
        return FALSE;
    }

    fib = AllocDosObject(DOS_FIB, NULL);
    if (fib != NULL)
    {
        if (Examine(dir, fib))
        {
            while (!got && ExNext(dir, fib))
            {
                ULONG len;

                if (fib->fib_DirEntryType > 0)
                    continue;

                len = strlen(fib->fib_FileName);
                if (len < 8 || NameEquals(fib->fib_FileName + len - 7, ".device") != 0)
                    continue;

                if (IsSkipped(fib->fib_FileName))
                    continue;

                if (ProbeDevice(fib->fib_FileName))
                {
                    strlcpy(found, fib->fib_FileName, size);
                    got = TRUE;
                }
            }
        }

        FreeDosObject(DOS_FIB, fib);
    }

    UnLock(dir);

    return got;
}

int main(void)
{
    struct RDArgs *rdargs;
    IPTR args[ARG_COUNT] = { 0 };
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldwindowptr;
    TEXT device[128];
    CONST_STRPTR file, ifname;
    BOOL quiet;
    BOOL resident = FALSE;
    int rc = RETURN_WARN;

    rdargs = ReadArgs(ARG_TEMPLATE, args, NULL);
    if (rdargs == NULL)
    {
        PrintFault(IoErr(), "netdetect");
        return RETURN_FAIL;
    }

    file = args[ARG_INTERFACES] ? (CONST_STRPTR)args[ARG_INTERFACES]
                                : (CONST_STRPTR)DEFAULT_IFACES;
    ifname = args[ARG_IFNAME] ? (CONST_STRPTR)args[ARG_IFNAME]
                              : (CONST_STRPTR)DEFAULT_IFNAME;
    quiet = args[ARG_QUIET] != 0;

    /* Probing a driver whose hardware is absent must not put a requester on
       the screen during the boot sequence. */
    oldwindowptr = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;

    if (args[ARG_DEVICE] != 0)
    {
        /* Explicit device: still probed, so a wrong guess writes nothing. */
        if (ProbeDevice((CONST_STRPTR)args[ARG_DEVICE]))
        {
            strlcpy(device, (CONST_STRPTR)args[ARG_DEVICE], sizeof(device));
            rc = RETURN_OK;
        }
    }
    else if (FindDeviceByUSB(device, sizeof(device), quiet))
    {
        /* A USB adapter wins over the onboard port: the port is always
           there, cabled or not, while a tethered phone, a hotspot or a USB
           dongle was plugged in to get online. */
        resident = TRUE;
        rc = RETURN_OK;
    }
    else if (FindDeviceByPCI(device, sizeof(device), quiet))
    {
        rc = RETURN_OK;
    }

    me->pr_WindowPtr = oldwindowptr;

    if (rc == RETURN_OK)
    {
        if (!WriteInterfaces(file, ifname, device, resident))
        {
            if (!quiet)
            {
                IPTR data[1];

                data[0] = (IPTR)file;
                VPrintf("netdetect: cannot write %s\n", (RAWARG)data);
            }
            rc = RETURN_FAIL;
        }
        else if (!quiet)
        {
            IPTR data[2];

            data[0] = (IPTR)device;
            data[1] = (IPTR)ifname;
            VPrintf("netdetect: %s -> %s\n", (RAWARG)data);
        }
    }
    else if (!quiet)
    {
        PutStr("netdetect: no usable Ethernet device found\n");
    }

    FreeArgs(rdargs);

    return rc;
}
