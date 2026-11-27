/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Message signalled interrupts of live drivers on the CPU they
          were given.

          On SMP the AHCI, NVMe and xHCI drivers ask the PCI device class,
          when they start, to deliver their MSI/MSI-X vector to the least
          busy CPU (HIDD_PCIDevice_SetVectorAttribs(),
          tHidd_PCIVector_Affinity, TASKAFFINITY_ANY). The class has
          kernel.resource choose the CPU and addresses the message to it.
          So:
            - HIDD_PCIDevice_GetVectorAttribs() tells the interrupt and
              the CPU of a vector, and KrnGetIRQA() the same CPU,
            - the handlers of the interrupt run on that CPU and on no
              other,
            - the driver keeps working with its handler there, whichever
              CPUs its requests come from.

          The test changes no vector: they belong to the drivers. It adds
          a counting handler of its own beside the driver's, with
          KrnAddIRQHandler(), to the interrupt of vector 0 of the first
          AHCI, NVMe and xHCI controller, and makes the device work:
            AHCI    files of the boot volume are read (SYS: is taken to
                    be a disk of that controller),
            NVMe    nvme.device unit 0 is read without a filesystem,
            xHCI    usbscsi.device unit 0, the first disk of Poseidon's
                    mass storage class, is read without a filesystem (it
                    is taken to be on that controller). Every request is
                    a command, a data and a status transfer of the
                    controller. Without a USB disk nothing is asked of
                    the controller: what happens to arrive is counted,
                    and parts 3 and 4 are skipped for it.

          Part 1: the controllers are looked for (HIDD_PCI_EnumDevices())
          and vector 0 of each is asked for. A controller that does not
          use MSI is left out. The CPU reported is one that exists.

          Part 2: KrnGetIRQA() reports the CPU the class reports, for
          every vector the device was given.

          Part 3: two seconds of reading, asked for on another CPU than
          the vector's: the handler is called, and every call is on the
          CPU reported. A device that raises no interrupt while it is
          read is INVALID, not a failure.

          Part 4: for each device, fifteen seconds of reading by two
          processes for each CPU (32 at most), one CPU each. Every
          request returns without error, what is read is what it should
          be, something completes in every second, every process ends,
          and every call of the handler is on the CPU reported.
            AHCI    each process reads whole files and compares size and
                    checksum with what was read before the load.
            NVMe, xHCI
                    each process reads extents of 1 sector to 1MB (or
                    the most the unit takes) at random places with
                    CMD_READ, TD_READ64 and NSCMD_TD_READ64, by DoIO()
                    and four at a time by SendIO(). An extent read twice
                    has to read the same. With the argument WRITE each
                    process also writes a pattern made from the block
                    number and its own number into a region of its own
                    at the end of the unit, and reads it back. Without
                    WRITE nothing is written, and nothing is written to
                    a unit that says it is write protected.

          At the start and at the end the entries of log.resource are
          counted, where the drivers put their errors and warnings; the
          new ones are printed. They are for whoever reads the log: no
          verdict is made from them.

          Part 5: the handlers are removed, and are not called any more.

          The handler touches static data and atomic counters only, and
          is removed before the program ends, whatever happened.
*/

#include <aros/config.h>
#include <aros/asmcall.h>
#include <aros/kernel.h>

#include <exec/interrupts.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/rdargs.h>
#include <resources/kernel.h>
#include <resources/task.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>
#include <proto/task.h>

#include <stdio.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

#if defined(__x86_64__)

#include <oop/oop.h>
#include <hidd/pci.h>
#include <hardware/pci.h>
#include <resources/log.h>
#include <proto/oop.h>
#include <proto/log.h>

APTR KernelBase;
APTR TaskResBase;
struct Library *OOPBase;
struct Library *LogResBase;
OOP_AttrBase HiddPCIDeviceAttrBase;

#define STACKSIZE       32768
#define WAIT_TICKS      500             /* 10s */
#define MAXCPUS         64
#define MAXTASKS        32
#define MAXVEC          8               /* vectors of a device that are looked at */

#define SETTLE_TICKS    10
#define P3_TICKS        100             /* part 3: the device is read this long */
#define LOAD_SECS       15              /* part 4, for each device */
#define QUIET_TICKS     25              /* part 5 */
#define MIN_LOAD_HITS   20              /* part 4: fewer interrupts is no load on the device */

#define NOVAL           ((IPTR)0x7FFFFFF0)      /* what a tag is handed with */
#define MAX_IRQ         0x10000UL               /* no port has more interrupts */

/* The controllers */
#define K_AHCI          0
#define K_NVME          1
#define K_XHCI          2
#define KINDS           3

#define MODE_NONE       0
#define MODE_MSI        1
#define MODE_MSIX       2

/* The files read from the boot volume */
#define IO_CHUNK        32768
#define MAXFILES        600
#define MAXPATH         160
#define MAXDEPTH        2
#define MAXFSIZE        (8UL * 1024 * 1024)
#define MIN_FILES       8
#define SCAN_TICKS      400                     /* the checksums are made this long at most */
#define SCAN_BYTES      (48UL * 1024 * 1024)

/* nvme.device: unit = (controller << 12) | (namespace - 1) */
#define NVME_NAME       "nvme.device"
#define NVME_UNIT       0
/* usbscsi.device: the units are numbered from 0 as the mass storage class binds the disks */
#define USB_NAME        "usbscsi.device"
#define USB_UNIT        0
#define USB_WAIT_TICKS  250                     /* the class may still be binding */
#define NV_MAXLEN       (1024UL * 1024)         /* the largest read tried */
#define NV_ASYNC        (256UL * 1024)          /* the largest of the reads sent together */
#define NV_REGION       (256UL * 1024)          /* WRITE: what a process writes into */
#define NV_STIRLEN      65536UL
#define NREQ            4

/* log.resource */
#define LOG_SHOW        8                       /* new errors and warnings that are printed */
#define LOG_TEXT        200

struct Vec
{
    ULONG               v_IRQ;
    ULONG               v_CPU;          /* by the PCI device class */
    int                 v_HasCPU;
};

struct Dev
{
    OOP_Object         *d_Obj;
    ULONG               d_Found;        /* controllers of the kind */
    int                 d_Mode;
    ULONG               d_Count;        /* vectors the device has enabled */
    int                 d_NVec;         /* vectors known; 0: the device is not tested */
    struct Vec          d_Vec[MAXVEC];
    APTR                d_Handle;
    int                 d_Ready;        /* there is something to read */
    char                d_Label[48];
};

struct Snap
{
    ULONG               sn_Hits[MAXCPUS + 1];
    ULONG               sn_Total;
};

struct FileEnt
{
    char                f_Path[MAXPATH];
    ULONG               f_Size;
    ULONG               f_Sum;
    ULONG               f_OK;
};

/* A device read as a whole: a unit of nvme.device or of usbscsi.device */
struct Raw
{
    const char         *r_Name;
    ULONG               r_Unit;
    ULONG               r_Wait;         /* ticks the unit is waited for */
    struct MsgPort     *r_Port;
    struct IOStdReq    *r_Req;
    int                 r_Open;
    UBYTE              *r_Buf;
    ULONG               r_Shift;        /* a sector is 1 << this */
    UQUAD               r_Total;        /* bytes of the unit */
    ULONG               r_Max;          /* the largest read the unit takes */
    UQUAD               r_ReadArea;     /* reads are below this */
    UQUAD               r_WriteBase;    /* WRITE: the regions start here */
    int                 r_Write;        /* WRITE: the unit is written to */
    ULONG               r_Seed;
};

struct Worker
{
    struct Process     *w_Proc;
    ULONG               w_Id;
    ULONG               w_CPU;
    struct Raw         *w_Raw;          /* NVMe, xHCI: the unit read */
    volatile ULONG      w_Stop;
    volatile ULONG      w_Started;
    volatile ULONG      w_Done;
    volatile ULONG      w_SetupFail;    /* 1: no memory, 2: the device did not open */
    volatile ULONG      w_Reqs;         /* requests that returned */
    volatile ULONG      w_Errors;
    volatile ULONG      w_Mismatch;
    volatile ULONG      w_Files;        /* AHCI: files read whole */
    volatile ULONG      w_Writes;       /* NVMe, xHCI: extents written and read back */
    volatile UQUAD      w_Bytes;
    /* The first error and the first mismatch */
    volatile LONG       w_ErrCode;
    volatile ULONG      w_ErrCmd;       /* AHCI: the file's place in the list */
    volatile ULONG      w_ErrLen;
    volatile ULONG      w_ErrActual;
    volatile UQUAD      w_ErrOff;
    volatile ULONG      w_BadLen;
    volatile UQUAD      w_BadOff;       /* AHCI: the file's place in the list */
};

static const char * const kindname[KINDS] = { "AHCI", "NVMe", "xHCI" };
static const char * const modename[3] = { "no MSI", "MSI", "MSI-X" };

static struct Dev       devs[KINDS];

/* All the handler touches. Never freed */
static volatile ULONG   g_Hits[KINDS][MAXCPUS + 1];     /* calls by CPU; last: CPU number too high */
static volatile ULONG   g_In[KINDS];                    /* the handler is inside */
static volatile ULONG   g_Overlap[KINDS];               /* it was entered while it was inside */

/* Never reused by another part: a process that hangs keeps its own */
static struct Worker    workers[KINDS][MAXTASKS];

static cpumask_t       *g_Mask[MAXCPUS];
static OOP_Object      *g_PCI;
static int              g_NCPUs;
static int              hung;                   /* a process did not end */

static struct FileEnt   g_Files[MAXFILES];
static ULONG            g_NFiles;
static ULONG            g_StirIdx;
static UBYTE            io_buf[IO_CHUNK];
static const char * const io_dirs[] = { "SYS:C", "SYS:Libs", "SYS:Devs", "SYS:Classes" };
#define IO_DIRS         (sizeof(io_dirs) / sizeof(io_dirs[0]))

/* By kind; AHCI has none */
static struct Raw       g_Raw[KINDS] =
{
    { NULL,      0,         0              },
    { NVME_NAME, NVME_UNIT, 0              },
    { USB_NAME,  USB_UNIT,  USB_WAIT_TICKS }
};
static int              g_Write;

static ULONG            g_Reads, g_ReadErrs;    /* reads of the main task that returned */

struct LogCount
{
    ULONG               lc_All;
    ULONG               lc_Err;         /* errors and critical ones */
    ULONG               lc_Warn;
    ULONG               lc_Timeout;     /* xHCI: "command timed out" */
    ULONG               lc_NoSpace;     /* xHCI: "NO SPACE ON RING" */
    ULONG               lc_Missing;     /* xHCI: "missing ioreq" */
};

static struct LogCount  g_Log0;
static char             g_LogText[LOG_SHOW][LOG_TEXT];

static int pass, fail, inval, verdict_given;
static char what[224], line[MAXCPUS * 16 + 16];

static void ok(int part, const char *text)
{
    bug("[smpmsiaff] part %d: OK - %s\n", (int)part, text);
    pass++;
}

/* First the verdict where what follows may not come back */
static void verdict(void)
{
    if (!verdict_given)
        bug("[smpmsiaff] DONE: %d PASS, %d FAIL, %d INVALID\n",
            (int)pass, (int)fail, (int)inval);
    verdict_given = 1;
}

static inline ULONG rd(volatile ULONG *p)
{
    return __atomic_load_n(p, __ATOMIC_SEQ_CST);
}

static inline void count(volatile ULONG *p)
{
    __atomic_fetch_add(p, 1, __ATOMIC_SEQ_CST);
}

/*
 * The handler under test, beside the driver's. Runs in interrupt context
 * on the CPU the vector is delivered to. A vector goes to one CPU, so
 * g_In is 0 on entry, or the handler is running on a second CPU.
 */
static void MsiHandler(void *data1, void *data2)
{
    ULONG kind = (ULONG)(IPTR)data1;
    ULONG cpu = KrnGetCPUNumber();

    if (__atomic_exchange_n(&g_In[kind], 1, __ATOMIC_SEQ_CST) != 0)
        count(&g_Overlap[kind]);
    count(&g_Hits[kind][(cpu < MAXCPUS) ? cpu : MAXCPUS]);
    __atomic_store_n(&g_In[kind], 0, __ATOMIC_SEQ_CST);
}

/* Start a process pinned to one CPU; the mask becomes the system's */
static struct Process *spawn(void (*entry)(void), const char *name, int cpu, APTR data)
{
    struct Process *p;
    APTR mask = KrnAllocCPUMask();

    if (!mask)
        return NULL;
    KrnClearCPUMask(mask);
    KrnGetCPUMask(cpu, mask);

    p = CreateNewProcTags(NP_Entry,     (IPTR)entry,
                          NP_Name,      (IPTR)name,
                          NP_StackSize, STACKSIZE,
                          NP_UserData,  (IPTR)data,
                          NP_Affinity,  (IPTR)mask,
                          TAG_DONE);
    if (!p)
        KrnFreeCPUMask(mask);
    return p;
}

/* The calling task on one CPU; cpu < 0: on any again */
static void pin_self(int cpu)
{
    if (!TaskResBase)
        return;
    if (cpu < 0)
        SetTaskAffinity(NULL, (APTR)TASKAFFINITY_ANY);
    else
        SetTaskAffinity(NULL, g_Mask[cpu]);
}

/* The CPU KrnGetIRQA() reports, NOVAL when it leaves the tag alone */
static IPTR irq_cpu(ULONG irq)
{
    struct TagItem tags[2];

    tags[0].ti_Tag  = KERNELTAG_IRQ_AFFINITY;
    tags[0].ti_Data = NOVAL;
    tags[1].ti_Tag  = TAG_DONE;
    tags[1].ti_Data = 0;

    KrnGetIRQA(irq, tags);

    return tags[0].ti_Data;
}

static ULONG now_ticks(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return (ULONG)ds.ds_Days * 4320000UL + (ULONG)ds.ds_Minute * 3000UL + (ULONG)ds.ds_Tick;
}

static int wait_flag(volatile ULONG *flag, int ticks)
{
    int i;

    for (i = 0; i < ticks && !*flag; i++)
        Delay(1);
    return *flag != 0;
}

static inline ULONG rnd(ULONG *seed)
{
    ULONG x = *seed;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *seed = x;
    return x;
}

static ULONG cksum(ULONG sum, const UBYTE *p, LONG n)
{
    while (n-- > 0)
        sum = sum * 31 + *p++;
    return sum;
}

static void snap(int kind, struct Snap *s)
{
    int i;

    s->sn_Total = 0;
    for (i = 0; i <= MAXCPUS; i++)
    {
        s->sn_Hits[i] = rd(&g_Hits[kind][i]);
        s->sn_Total += s->sn_Hits[i];
    }
}

/* The handler calls between two looks, by CPU, into line[] */
static void hits_line(const struct Snap *a, const struct Snap *b)
{
    int i, p = 0;

    line[0] = 0;
    for (i = 0; i <= MAXCPUS && p < (int)sizeof(line) - 24; i++)
    {
        ULONG n = b->sn_Hits[i] - a->sn_Hits[i];

        if (n == 0)
            continue;
        if (i == MAXCPUS)
            p += snprintf(line + p, sizeof(line) - p, " CPU?=%lu", (unsigned long)n);
        else
            p += snprintf(line + p, sizeof(line) - p, " CPU%d=%lu", i, (unsigned long)n);
    }
    if (p == 0)
        snprintf(line, sizeof(line), " none");
}

/*
 * The PCI devices
 */

AROS_UFH3(static void, FindDevice,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct Dev *d = (struct Dev *)hook->h_Data;

    /* The first of a kind is the one tested */
    if (d->d_Found++ == 0)
        d->d_Obj = obj;

    AROS_USERFUNC_EXIT
}

static void find_devices(void)
{
    static const struct TagItem ahci_tags[] =
    {
        { tHidd_PCI_Class,      0x01    },
        { tHidd_PCI_SubClass,   0x06    },
        { TAG_DONE,             0       }
    };
    static const struct TagItem nvme_tags[] =
    {
        { tHidd_PCI_Class,      0x01    },
        { tHidd_PCI_SubClass,   0x08    },
        { TAG_DONE,             0       }
    };
    static const struct TagItem xhci_tags[] =
    {
        { tHidd_PCI_Class,      0x0c    },
        { tHidd_PCI_SubClass,   0x03    },
        { tHidd_PCI_Interface,  0x30    },
        { TAG_DONE,             0       }
    };
    static const struct TagItem * const tags[KINDS] = { ahci_tags, nvme_tags, xhci_tags };
    struct Hook hook;
    int k;

    for (k = 0; k < KINDS; k++)
    {
        memset(&hook, 0, sizeof(hook));
        hook.h_Entry = (APTR)FindDevice;
        hook.h_Data  = &devs[k];

        HIDD_PCI_EnumDevices(g_PCI, &hook, tags[k]);
    }
}

/* What the device has enabled, from its capabilities. Reads only */
static int msi_mode(OOP_Object *obj, ULONG *vectors)
{
    IPTR capmsi = 0, capmsix = 0;
    UWORD flags;

    OOP_GetAttr(obj, aHidd_PCIDevice_CapabilityMSI,  &capmsi);
    OOP_GetAttr(obj, aHidd_PCIDevice_CapabilityMSIX, &capmsix);

    if (capmsix)
    {
        flags = HIDD_PCIDevice_ReadConfigWord(obj, capmsix + PCIMSIX_FLAGS);
        if (flags & PCIMSIXF_ENABLE)
        {
            *vectors = (flags & PCIMSIXF_QSIZE) + 1;
            return MODE_MSIX;
        }
    }
    if (capmsi)
    {
        flags = HIDD_PCIDevice_ReadConfigWord(obj, capmsi + PCIMSI_FLAGS);
        if (flags & PCIMSIF_ENABLE)
        {
            *vectors = 1UL << ((flags & PCIMSIF_MMEN_MASK) >> PCIMSIB_MMEN);
            return MODE_MSI;
        }
    }

    *vectors = 0;
    return MODE_NONE;
}

/* A vector by the class. Returns 0 when it has no interrupt for it */
static int vec_get(OOP_Object *obj, ULONG n, struct Vec *v)
{
    struct TagItem tags[3];

    tags[0].ti_Tag  = tHidd_PCIVector_Int;
    tags[0].ti_Data = NOVAL;
    tags[1].ti_Tag  = tHidd_PCIVector_Affinity;
    tags[1].ti_Data = NOVAL;
    tags[2].ti_Tag  = TAG_DONE;
    tags[2].ti_Data = 0;

    v->v_IRQ    = 0;
    v->v_CPU    = 0;
    v->v_HasCPU = 0;

    HIDD_PCIDevice_GetVectorAttribs(obj, n, tags);

    if (tags[0].ti_Data == NOVAL)
        return 0;

    v->v_IRQ    = (ULONG)tags[0].ti_Data;
    v->v_HasCPU = (tags[1].ti_Data != NOVAL);
    v->v_CPU    = v->v_HasCPU ? (ULONG)tags[1].ti_Data : 0;

    /* An entry of the MSI-X table nothing was written to gives no interrupt */
    return (tags[0].ti_Data < MAX_IRQ);
}

/* Returns the number of devices that have a vector */
static int part1(void)
{
    int k, n, usable = 0;

    for (k = 0; k < KINDS; k++)
    {
        struct Dev *d = &devs[k];
        IPTR bus = 0, dev = 0, sub = 0, vendor = 0, product = 0;
        struct Vec v;

        if (d->d_Obj == NULL)
        {
            bug("[smpmsiaff] part 1: SKIP %s (no such controller)\n", kindname[k]);
            continue;
        }

        OOP_GetAttr(d->d_Obj, aHidd_PCIDevice_Bus, &bus);
        OOP_GetAttr(d->d_Obj, aHidd_PCIDevice_Dev, &dev);
        OOP_GetAttr(d->d_Obj, aHidd_PCIDevice_Sub, &sub);
        OOP_GetAttr(d->d_Obj, aHidd_PCIDevice_VendorID, &vendor);
        OOP_GetAttr(d->d_Obj, aHidd_PCIDevice_ProductID, &product);
        snprintf(d->d_Label, sizeof(d->d_Label), "%s %02x:%02x.%x (%04x:%04x)", kindname[k],
                 (unsigned)bus, (unsigned)dev, (unsigned)sub, (unsigned)vendor, (unsigned)product);

        if (d->d_Found > 1)
            bug("[smpmsiaff] part 1: %lu %s controllers, the first is tested\n",
                (unsigned long)d->d_Found, kindname[k]);

        d->d_Mode = msi_mode(d->d_Obj, &d->d_Count);

        if (!vec_get(d->d_Obj, 0, &v))
        {
            bug("[smpmsiaff] part 1: SKIP %s (%s enabled, the class has no interrupt for "
                "vector 0: not using MSI)\n", d->d_Label, modename[d->d_Mode]);
            continue;
        }

        d->d_Vec[0] = v;
        d->d_NVec = 1;
        usable++;

        /* The others it was given: for part 2. No handler is added to them */
        for (n = 1; n < MAXVEC && (ULONG)n < d->d_Count; n++)
        {
            int i, twice = 0;

            if (!vec_get(d->d_Obj, n, &v))
                break;
            for (i = 0; i < d->d_NVec; i++)
            {
                if (d->d_Vec[i].v_IRQ == v.v_IRQ)
                    twice = 1;
            }
            if (twice)
                break;
            d->d_Vec[d->d_NVec++] = v;
        }

        for (n = 0; n < d->d_NVec; n++)
        {
            if (d->d_Vec[n].v_HasCPU)
                bug("[smpmsiaff] part 1: %s %s vector %d of %lu: IRQ %lu, CPU %lu\n",
                    d->d_Label, modename[d->d_Mode], (int)n, (unsigned long)d->d_Count,
                    (unsigned long)d->d_Vec[n].v_IRQ, (unsigned long)d->d_Vec[n].v_CPU);
            else
                bug("[smpmsiaff] part 1: %s %s vector %d of %lu: IRQ %lu, no CPU reported\n",
                    d->d_Label, modename[d->d_Mode], (int)n, (unsigned long)d->d_Count,
                    (unsigned long)d->d_Vec[n].v_IRQ);
        }

        if (!d->d_Vec[0].v_HasCPU)
        {
            IPTR kcpu = irq_cpu(d->d_Vec[0].v_IRQ);

            bug("[smpmsiaff] part 1: *** FAIL *** %s: the class reports IRQ %lu for vector 0 "
                "and no CPU (tHidd_PCIVector_Affinity left alone)\n",
                d->d_Label, (unsigned long)d->d_Vec[0].v_IRQ);
            fail++;

            /* The rest goes by what kernel.resource says, if it says */
            if (kcpu < (IPTR)g_NCPUs)
                d->d_Vec[0].v_CPU = (ULONG)kcpu;
            else
            {
                d->d_NVec = 0;
                usable--;
            }
        }
        else if (d->d_Vec[0].v_CPU >= (ULONG)g_NCPUs)
        {
            bug("[smpmsiaff] part 1: *** FAIL *** %s: the class reports CPU %lu for vector 0, "
                "there are %d\n", d->d_Label, (unsigned long)d->d_Vec[0].v_CPU, (int)g_NCPUs);
            fail++;
            d->d_NVec = 0;
            usable--;
        }
        else
        {
            snprintf(what, sizeof(what), "%s: vector 0 is IRQ %lu, delivered to CPU %lu",
                     d->d_Label, (unsigned long)d->d_Vec[0].v_IRQ,
                     (unsigned long)d->d_Vec[0].v_CPU);
            ok(1, what);
        }
    }

    return usable;
}

static void part2(void)
{
    int k, n;

    for (k = 0; k < KINDS; k++)
    {
        struct Dev *d = &devs[k];
        int bad = 0, asked = 0;

        for (n = 0; n < d->d_NVec; n++)
        {
            struct Vec *v = &d->d_Vec[n];
            IPTR kcpu;

            if (!v->v_HasCPU)
                continue;
            asked++;

            kcpu = irq_cpu(v->v_IRQ);
            if (kcpu == NOVAL)
            {
                bug("[smpmsiaff] part 2: *** FAIL *** %s vector %d: KrnGetIRQA() reports no "
                    "CPU for IRQ %lu\n", d->d_Label, (int)n, (unsigned long)v->v_IRQ);
                fail++;
                bad = 1;
            }
            else if (kcpu != (IPTR)v->v_CPU)
            {
                bug("[smpmsiaff] part 2: *** FAIL *** %s vector %d, IRQ %lu: the class reports "
                    "CPU %lu, KrnGetIRQA() CPU %lu\n", d->d_Label, (int)n,
                    (unsigned long)v->v_IRQ, (unsigned long)v->v_CPU, (unsigned long)kcpu);
                fail++;
                bad = 1;
            }
        }

        if (asked && !bad)
        {
            snprintf(what, sizeof(what), "%s: KrnGetIRQA() reports the CPU the class reports "
                     "(%d vector%s, vector 0 on CPU %lu)", d->d_Label, asked,
                     (asked == 1) ? "" : "s", (unsigned long)d->d_Vec[0].v_CPU);
            ok(2, what);
        }
    }
}

/*
 * The files of the boot volume
 */

static void walk(const char *dir, int depth)
{
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR lock = Lock((CONST_STRPTR)dir, SHARED_LOCK);
    char path[MAXPATH];

    if (lock && fib && Examine(lock, fib))
    {
        while (g_NFiles < MAXFILES && ExNext(lock, fib))
        {
            if (snprintf(path, sizeof(path), "%s/%s", dir, fib->fib_FileName) >= (int)sizeof(path))
                continue;

            if (fib->fib_DirEntryType >= 0)
            {
                if (depth < MAXDEPTH)
                    walk(path, depth + 1);
            }
            else if (fib->fib_Size > 0 && (ULONG)fib->fib_Size <= MAXFSIZE)
            {
                struct FileEnt *f = &g_Files[g_NFiles++];

                strcpy(f->f_Path, path);
                f->f_Size = (ULONG)fib->fib_Size;
                f->f_Sum  = 0;
                f->f_OK   = 0;
            }
        }
    }

    if (lock)
        UnLock(lock);
    if (fib)
        FreeDosObject(DOS_FIB, fib);
}

static void list_files(void)
{
    ULONG i;

    g_NFiles = 0;
    for (i = 0; i < IO_DIRS; i++)
        walk(io_dirs[i], 0);
    bug("[smpmsiaff] %lu files in SYS:C, Libs, Devs and Classes\n", (unsigned long)g_NFiles);
}

/* A file read whole. Returns 1; 0 when a read failed, -1 when it did not open */
static int sum_file(struct FileEnt *f, ULONG *size, ULONG *sum)
{
    BPTR fh = Open((CONST_STRPTR)f->f_Path, MODE_OLDFILE);
    ULONG s = 0, got = 0;
    LONG n;

    if (!fh)
        return -1;
    while ((n = Read(fh, io_buf, IO_CHUNK)) > 0)
    {
        g_Reads++;
        s = cksum(s, io_buf, n);
        got += n;
    }
    Close(fh);

    *size = got;
    *sum  = s;
    return (n == 0) ? 1 : 0;
}

/*
 * The checksums the load compares with, of as many of the files as a
 * few seconds give, taken from all over the list. The others are
 * dropped from it.
 */
static void scan_files(void)
{
    static const ULONG strides[] = { 127, 61, 31, 13, 7, 3, 1 };
    ULONG start = now_ticks(), stride = 1, i, n, kept = 0, idx = 0, unread = 0;
    UQUAD bytes = 0;

    if (g_NFiles == 0)
        return;

    /* A stride that reaches every file */
    for (i = 0; i < sizeof(strides) / sizeof(strides[0]); i++)
    {
        if (g_NFiles % strides[i] != 0 || strides[i] == 1)
        {
            stride = strides[i];
            break;
        }
    }

    for (n = 0; n < g_NFiles; n++, idx = (idx + stride) % g_NFiles)
    {
        struct FileEnt *f = &g_Files[idx];
        ULONG size = 0, sum = 0;

        if (now_ticks() - start >= SCAN_TICKS || bytes >= SCAN_BYTES)
            break;
        if (sum_file(f, &size, &sum) != 1 || size != f->f_Size)
        {
            unread++;
            continue;
        }
        f->f_Sum = sum;
        f->f_OK  = 1;
        bytes += size;
    }

    for (i = 0; i < g_NFiles; i++)
    {
        if (g_Files[i].f_OK)
        {
            if (kept != i)
                g_Files[kept] = g_Files[i];
            kept++;
        }
    }
    g_NFiles = kept;

    snprintf(what, sizeof(what), "%lu files, %llu bytes, read once for their checksums; "
             "%lu could not be read whole", (unsigned long)kept, (unsigned long long)bytes,
             (unsigned long)unread);
    bug("[smpmsiaff] %s\n", what);
}

/* Files are read for 'ticks', other ones each time */
static void stir_files(ULONG ticks)
{
    ULONG start = now_ticks(), size, sum;

    do
    {
        if (g_NFiles == 0)
        {
            Delay(1);
            continue;
        }
        g_StirIdx = (g_StirIdx + 1) % g_NFiles;
        if (sum_file(&g_Files[g_StirIdx], &size, &sum) == 0)
            g_ReadErrs++;
    } while (now_ticks() - start < ticks);
}

/*
 * nvme.device and usbscsi.device, read as a whole
 */

/* A read or a write of 'len' bytes at 'off' into a request, by one of the commands for it */
static void nv_prep(struct IOStdReq *io, int write, ULONG sel, APTR data, ULONG len, UQUAD off)
{
    io->io_Data   = data;
    io->io_Length = len;
    io->io_Offset = (ULONG)off;
    io->io_Flags  = 0;
    io->io_Error  = 0;

    if ((off + len) <= 0xFFFFFFFFULL && (sel % 3) == 0)
    {
        io->io_Command = write ? CMD_WRITE : CMD_READ;
        io->io_Actual  = 0;
    }
    else
    {
        /* The upper half of the offset goes in io_Actual */
        if (sel & 1)
            io->io_Command = write ? TD_WRITE64 : TD_READ64;
        else
            io->io_Command = write ? NSCMD_TD_WRITE64 : NSCMD_TD_READ64;
        io->io_Actual = (ULONG)(off >> 32);
    }
}

/* A length of whole sectors, one sector to 'maxlen' (a power of two of them) */
static ULONG nv_len(struct Raw *rw, ULONG *seed, ULONG maxlen)
{
    ULONG maxsec = maxlen >> rw->r_Shift, r = rnd(seed), bits = 0, n;

    while ((2UL << bits) <= maxsec)
        bits++;

    if (r & 3)
        n = 1UL << ((r >> 8) % (bits + 1));
    else
        n = 1 + (r >> 8) % maxsec;

    return n << rw->r_Shift;
}

/* Where 'len' bytes fit below 'area', on a sector */
static UQUAD nv_off(struct Raw *rw, ULONG *seed, UQUAD area, ULONG len)
{
    UQUAD secs = (area - len) >> rw->r_Shift;
    UQUAD r = ((UQUAD)rnd(seed) << 32) | rnd(seed);

    return (r % (secs + 1)) << rw->r_Shift;
}

static void nv_close(struct Raw *rw)
{
    if (rw->r_Open)
        CloseDevice((struct IORequest *)rw->r_Req);
    if (rw->r_Req)
        DeleteIORequest((struct IORequest *)rw->r_Req);
    if (rw->r_Port)
        DeleteMsgPort(rw->r_Port);
    if (rw->r_Buf)
        FreeMem(rw->r_Buf, NV_MAXLEN);
    rw->r_Open = 0;
    rw->r_Req  = NULL;
    rw->r_Port = NULL;
    rw->r_Buf  = NULL;
}

/*
 * The unit of a kind opened, its size, and the largest read it takes.
 * Returns 0 when the parts that read it cannot be run; the reason has
 * been printed.
 */
static int nv_open(int kind, int ntasks)
{
    struct Raw *rw = &g_Raw[kind];
    struct DriveGeometry geom;
    ULONG len, secsize, waited = 0;
    LONG err;

    rw->r_Seed = 0x2545F491UL + (ULONG)kind;
    rw->r_Port = CreateMsgPort();
    if (rw->r_Port)
        rw->r_Req = (struct IOStdReq *)CreateIORequest(rw->r_Port, sizeof(struct IOExtTD));
    rw->r_Buf = AllocMem(NV_MAXLEN, MEMF_PUBLIC);
    if (!rw->r_Req || !rw->r_Buf)
    {
        bug("[smpmsiaff] INVALID: no memory for the %s reads\n", kindname[kind]);
        inval++;
        return 0;
    }

    /* A USB disk is not a unit before its class has bound it */
    for (;;)
    {
        err = OpenDevice((CONST_STRPTR)rw->r_Name, rw->r_Unit, (struct IORequest *)rw->r_Req, 0);
        if (err == 0 || waited >= rw->r_Wait)
            break;
        Delay(25);
        waited += 25;
    }
    if (err)
    {
        bug("[smpmsiaff] %s unit %d does not open (error %ld)\n", rw->r_Name, (int)rw->r_Unit,
            (long)err);
        return 0;
    }
    rw->r_Open = 1;

    /* A reader without a medium in it is no disk to read */
    rw->r_Req->io_Command = TD_CHANGESTATE;
    rw->r_Req->io_Data    = NULL;
    rw->r_Req->io_Length  = 0;
    rw->r_Req->io_Offset  = 0;
    rw->r_Req->io_Actual  = 0;
    rw->r_Req->io_Flags   = 0;
    if (DoIO((struct IORequest *)rw->r_Req) == 0 && rw->r_Req->io_Actual != 0)
    {
        bug("[smpmsiaff] %s unit %d has no medium in it\n", rw->r_Name, (int)rw->r_Unit);
        return 0;
    }

    memset(&geom, 0, sizeof(geom));
    rw->r_Req->io_Command = TD_GETGEOMETRY;
    rw->r_Req->io_Data    = &geom;
    rw->r_Req->io_Length  = sizeof(geom);
    rw->r_Req->io_Offset  = 0;
    rw->r_Req->io_Actual  = 0;
    rw->r_Req->io_Flags   = 0;
    err = DoIO((struct IORequest *)rw->r_Req);

    secsize = geom.dg_SectorSize;
    if (err || secsize < 512 || secsize > 65536 || (secsize & (secsize - 1)))
    {
        bug("[smpmsiaff] INVALID: TD_GETGEOMETRY: error %ld, sector size %lu\n", (long)err,
            (unsigned long)secsize);
        inval++;
        return 0;
    }
    for (rw->r_Shift = 0; (1UL << rw->r_Shift) < secsize; rw->r_Shift++)
        ;
    rw->r_Total = (UQUAD)geom.dg_TotalSectors << rw->r_Shift;

    bug("[smpmsiaff] %s unit %d: %lu sectors of %lu bytes\n", rw->r_Name, (int)rw->r_Unit,
        (unsigned long)geom.dg_TotalSectors, (unsigned long)secsize);

    if (rw->r_Total < 64 * (UQUAD)secsize)
    {
        bug("[smpmsiaff] INVALID: the unit is too small to be read at random\n");
        inval++;
        return 0;
    }

    /* The largest read: the driver refuses more than the controller takes at once */
    rw->r_Max = 0;
    for (len = secsize; len <= NV_MAXLEN && len <= rw->r_Total / 4; len <<= 1)
    {
        nv_prep(rw->r_Req, 0, 0, rw->r_Buf, len, 0);
        err = DoIO((struct IORequest *)rw->r_Req);
        if (err == 0 && rw->r_Req->io_Actual == len)
        {
            rw->r_Max = len;
            continue;
        }
        if (err == IOERR_BADLENGTH && rw->r_Max)
        {
            bug("[smpmsiaff] a read of %lu bytes is refused (IOERR_BADLENGTH): the unit "
                "takes %lu at once\n", (unsigned long)len, (unsigned long)rw->r_Max);
            break;
        }

        bug("[smpmsiaff] *** FAIL *** %s: a read of %lu bytes at 0 returned error %ld, "
            "io_Actual %lu\n", rw->r_Name, (unsigned long)len, (long)err,
            (unsigned long)rw->r_Req->io_Actual);
        fail++;
        return 0;
    }
    if (rw->r_Max == 0)
    {
        bug("[smpmsiaff] INVALID: no read of the unit was possible\n");
        inval++;
        return 0;
    }

    rw->r_ReadArea  = rw->r_Total;
    rw->r_WriteBase = 0;
    rw->r_Write     = 0;
    if (g_Write)
    {
        UQUAD warea = (UQUAD)ntasks * NV_REGION;

        rw->r_Req->io_Command = TD_PROTSTATUS;
        rw->r_Req->io_Data    = NULL;
        rw->r_Req->io_Length  = 0;
        rw->r_Req->io_Offset  = 0;
        rw->r_Req->io_Actual  = 0;
        rw->r_Req->io_Flags   = 0;

        if (DoIO((struct IORequest *)rw->r_Req) == 0 && rw->r_Req->io_Actual != 0)
            bug("[smpmsiaff] WRITE: %s unit %d is write protected; nothing is written to it\n",
                rw->r_Name, (int)rw->r_Unit);
        else if (NV_REGION < 2 * secsize || rw->r_Total < 4 * warea)
        {
            bug("[smpmsiaff] INVALID: the unit is too small for WRITE (%d regions of %lu "
                "bytes); nothing is written\n", (int)ntasks, (unsigned long)NV_REGION);
            inval++;
        }
        else
        {
            rw->r_WriteBase = ((rw->r_Total - warea) >> rw->r_Shift) << rw->r_Shift;
            rw->r_ReadArea  = rw->r_WriteBase;
            rw->r_Write     = 1;
            snprintf(what, sizeof(what), "WRITE: %s unit %d is written to from byte %llu on",
                     rw->r_Name, (int)rw->r_Unit, (unsigned long long)rw->r_WriteBase);
            bug("[smpmsiaff] %s\n", what);
        }
    }

    return 1;
}

/* The unit is read for 'ticks', at random places */
static void stir_raw(struct Raw *rw, ULONG ticks)
{
    ULONG start = now_ticks(), maxlen = (rw->r_Max < NV_STIRLEN) ? rw->r_Max : NV_STIRLEN, len;
    UQUAD off;

    do
    {
        len = nv_len(rw, &rw->r_Seed, maxlen);
        off = nv_off(rw, &rw->r_Seed, rw->r_ReadArea, len);
        nv_prep(rw->r_Req, 0, rnd(&rw->r_Seed), rw->r_Buf, len, off);
        if (DoIO((struct IORequest *)rw->r_Req) || rw->r_Req->io_Actual != len)
            g_ReadErrs++;
        g_Reads++;
    } while (now_ticks() - start < ticks);
}

static void stir(int kind, ULONG ticks)
{
    if (kind == K_AHCI && devs[kind].d_Ready)
        stir_files(ticks);
    else if (kind != K_AHCI && devs[kind].d_Ready)
        stir_raw(&g_Raw[kind], ticks);
    else
        Delay(ticks);
}

/*
 * Part 3
 */

static void part3(int kind)
{
    struct Dev *d = &devs[kind];
    ULONG cpu = d->d_Vec[0].v_CPU, irq = d->d_Vec[0].v_IRQ, on, off, reads, errs;
    struct Snap a, b;
    int i, other = -1;

    /* What is asked for is asked on another CPU than the interrupt's */
    pin_self((cpu + 1) % g_NCPUs);

    stir(kind, SETTLE_TICKS);
    snap(kind, &a);
    reads = g_Reads;
    errs  = g_ReadErrs;
    stir(kind, P3_TICKS);
    snap(kind, &b);
    reads = g_Reads - reads;
    errs  = g_ReadErrs - errs;

    pin_self(-1);

    on  = b.sn_Hits[cpu] - a.sn_Hits[cpu];
    off = (b.sn_Total - a.sn_Total) - on;
    for (i = 0; i <= MAXCPUS; i++)
    {
        if (i != (int)cpu && b.sn_Hits[i] != a.sn_Hits[i])
        {
            other = i;
            break;
        }
    }

    hits_line(&a, &b);
    bug("[smpmsiaff] part 3: %s IRQ %lu, reported on CPU %lu: %lu reads from CPU %d, "
        "handler calls:%s\n", d->d_Label, (unsigned long)irq, (unsigned long)cpu,
        (unsigned long)reads, (int)((cpu + 1) % g_NCPUs), line);

    if (errs)
    {
        bug("[smpmsiaff] part 3: *** FAIL *** %s: %lu reads returned an error (%lu did not)\n",
            d->d_Label, (unsigned long)errs, (unsigned long)reads);
        fail++;
    }

    if (off)
    {
        bug("[smpmsiaff] part 3: *** FAIL *** %s: IRQ %lu is reported on CPU %lu, the handler "
            "ran %lu times on other CPUs (CPU %d among them)\n", d->d_Label,
            (unsigned long)irq, (unsigned long)cpu, (unsigned long)off, (int)other);
        fail++;
    }
    else if (on)
    {
        snprintf(what, sizeof(what), "%s: every call of the handler (%lu) was on CPU %lu",
                 d->d_Label, (unsigned long)on, (unsigned long)cpu);
        ok(3, what);
    }
    else if (kind == K_XHCI && !d->d_Ready)
        bug("[smpmsiaff] part 3: SKIP %s (no interrupt arrived; without a USB disk none "
            "can be caused)\n", d->d_Label);
    else
    {
        bug("[smpmsiaff] part 3: INVALID - %s: no interrupt while %lu reads completed "
            "(what was read did not come from the device)\n", d->d_Label,
            (unsigned long)reads);
        inval++;
    }
}

/*
 * Part 4
 */

static struct Worker *myworker(void)
{
    return (struct Worker *)FindTask(NULL)->tc_UserData;
}

/* Whole files, each compared with what the main task read of it */
static void FileWorker(void)
{
    struct Worker *w = myworker();
    UBYTE *buf = AllocMem(IO_CHUNK, MEMF_ANY);
    ULONG idx, sum, got;
    LONG n;
    BPTR fh;
    int whole;

    if (!buf)
    {
        w->w_SetupFail = 1;
        w->w_Started = 1;
        w->w_Done = 1;
        return;
    }

    /* Each from another place in the list */
    idx = (w->w_Id * 37 + 11) % g_NFiles;
    w->w_Started = 1;

    while (!w->w_Stop)
    {
        struct FileEnt *f = &g_Files[idx];

        fh = Open((CONST_STRPTR)f->f_Path, MODE_OLDFILE);
        if (!fh)
        {
            if (w->w_Errors++ == 0)
            {
                w->w_ErrCode = IoErr();
                w->w_ErrCmd  = idx;
                w->w_ErrLen  = 0;
            }
            count(&w->w_Reqs);
            idx = (idx + 1) % g_NFiles;
            continue;
        }

        sum = 0;
        got = 0;
        whole = 1;
        for (;;)
        {
            if (w->w_Stop)
            {
                whole = 0;
                break;
            }
            n = Read(fh, buf, IO_CHUNK);
            count(&w->w_Reqs);
            if (n < 0)
            {
                if (w->w_Errors++ == 0)
                {
                    w->w_ErrCode = IoErr();
                    w->w_ErrCmd  = idx;
                    w->w_ErrLen  = got;
                }
                whole = 0;
                break;
            }
            if (n == 0)
                break;
            sum = cksum(sum, buf, n);
            got += n;
            w->w_Bytes += n;
        }
        Close(fh);

        if (whole)
        {
            if (got != f->f_Size || sum != f->f_Sum)
            {
                if (w->w_Mismatch++ == 0)
                {
                    w->w_BadOff = idx;
                    w->w_BadLen = got;
                }
            }
            w->w_Files++;
        }
        idx = (idx + 1) % g_NFiles;
    }

    FreeMem(buf, IO_CHUNK);
    w->w_Done = 1;
}

/* A request of the load on a unit is back: was it done? */
static int nv_done(struct Worker *w, struct IOStdReq *io, ULONG len, UQUAD off)
{
    count(&w->w_Reqs);

    if (io->io_Error || io->io_Actual != len)
    {
        if (w->w_Errors++ == 0)
        {
            w->w_ErrCode   = io->io_Error;
            w->w_ErrCmd    = io->io_Command;
            w->w_ErrLen    = len;
            w->w_ErrActual = io->io_Actual;
            w->w_ErrOff    = off;
        }
        return 0;
    }

    w->w_Bytes += len;
    return 1;
}

static void nv_differs(struct Worker *w, ULONG len, UQUAD off)
{
    if (w->w_Mismatch++ == 0)
    {
        w->w_BadLen = len;
        w->w_BadOff = off;
    }
}

/* What a process writes: made from the block number and the process */
static void nv_pattern(struct Raw *rw, UBYTE *buf, ULONG len, UQUAD off, ULONG id)
{
    ULONG words = (1UL << rw->r_Shift) / sizeof(ULONG), secs = len >> rw->r_Shift, s, j;
    ULONG *p = (ULONG *)buf;

    for (s = 0; s < secs; s++)
    {
        UQUAD lba = (off >> rw->r_Shift) + s;
        ULONG v = ((ULONG)lba * 2654435761UL) ^ (ULONG)(lba >> 32) ^ ((id + 1) << 24);

        for (j = 0; j < words; j++)
            *p++ = v ^ j;
    }
}

/*
 * Reads all over the unit, by DoIO() and several at once by SendIO().
 * The port, the requests and the device are the process's own.
 */
static void RawWorker(void)
{
    struct Worker *w = myworker();
    struct Raw *rw = w->w_Raw;
    struct MsgPort *port;
    struct IOStdReq *req[NREQ];
    UBYTE *a = NULL, *b = NULL;
    ULONG maxlen = rw->r_Max, alen = 0, bufsz = 0, seed, r, len[NREQ];
    UQUAD off[NREQ];
    int i, opened = 0, good[NREQ];

    for (i = 0; i < NREQ; i++)
        req[i] = NULL;
    seed = (0x9E3779B9UL * (w->w_Id + 1)) ^ now_ticks();
    if (seed == 0)
        seed = 1;

    if ((port = CreateMsgPort()) != NULL)
    {
        for (i = 0; i < NREQ; i++)
            req[i] = (struct IOStdReq *)CreateIORequest(port, sizeof(struct IOExtTD));
    }

    /* Two buffers of the largest read, of smaller reads where memory is short */
    for (;;)
    {
        alen  = (maxlen < NV_ASYNC) ? maxlen : NV_ASYNC;
        bufsz = (maxlen > NREQ * alen) ? maxlen : NREQ * alen;
        a = AllocMem(bufsz, MEMF_PUBLIC);
        b = AllocMem(bufsz, MEMF_PUBLIC);
        if (a && b)
            break;
        if (a)
            FreeMem(a, bufsz);
        if (b)
            FreeMem(b, bufsz);
        a = b = NULL;
        if (maxlen <= 65536 || (maxlen >> 1) < (1UL << rw->r_Shift))
            break;
        maxlen >>= 1;
    }

    if (!port || !req[0] || !req[1] || !req[2] || !req[3] || !a)
        w->w_SetupFail = 1;
    else if (OpenDevice((CONST_STRPTR)rw->r_Name, rw->r_Unit, (struct IORequest *)req[0], 0))
        w->w_SetupFail = 2;
    else
    {
        opened = 1;
        /* The other requests are for the same unit */
        for (i = 1; i < NREQ; i++)
        {
            req[i]->io_Device = req[0]->io_Device;
            req[i]->io_Unit   = req[0]->io_Unit;
        }
    }

    w->w_Started = 1;

    while (opened && !w->w_Stop)
    {
        r = rnd(&seed);

        if (rw->r_Write && (r & 7) == 0)
        {
            /* A pattern into the process's own region, and back */
            ULONG wmax = (maxlen < NV_REGION) ? maxlen : NV_REGION;

            len[0] = nv_len(rw, &seed, wmax);
            off[0] = rw->r_WriteBase + (UQUAD)w->w_Id * NV_REGION +
                     nv_off(rw, &seed, NV_REGION, len[0]);

            nv_pattern(rw, a, len[0], off[0], w->w_Id);
            nv_prep(req[0], 1, r >> 4, a, len[0], off[0]);
            DoIO((struct IORequest *)req[0]);
            if (nv_done(w, req[0], len[0], off[0]))
            {
                nv_prep(req[1], 0, r >> 6, b, len[0], off[0]);
                DoIO((struct IORequest *)req[1]);
                if (nv_done(w, req[1], len[0], off[0]))
                {
                    if (memcmp(a, b, len[0]) != 0)
                        nv_differs(w, len[0], off[0]);
                    w->w_Writes++;
                }
            }
        }
        else if (r & 8)
        {
            /* One extent, and sometimes once more: the same has to be read */
            len[0] = nv_len(rw, &seed, maxlen);
            off[0] = nv_off(rw, &seed, rw->r_ReadArea, len[0]);

            nv_prep(req[0], 0, r >> 4, a, len[0], off[0]);
            DoIO((struct IORequest *)req[0]);
            if (nv_done(w, req[0], len[0], off[0]) && (r & 0x100))
            {
                nv_prep(req[1], 0, r >> 6, b, len[0], off[0]);
                DoIO((struct IORequest *)req[1]);
                if (nv_done(w, req[1], len[0], off[0]) && memcmp(a, b, len[0]) != 0)
                    nv_differs(w, len[0], off[0]);
            }
        }
        else
        {
            /* Several in flight; the first two read the same extent */
            for (i = 0; i < NREQ; i++)
            {
                if (i == 1)
                {
                    len[1] = len[0];
                    off[1] = off[0];
                }
                else
                {
                    len[i] = nv_len(rw, &seed, alen);
                    off[i] = nv_off(rw, &seed, rw->r_ReadArea, len[i]);
                }
                nv_prep(req[i], 0, (r >> 4) + i, a + (ULONG)i * alen, len[i], off[i]);
            }
            for (i = 0; i < NREQ; i++)
                SendIO((struct IORequest *)req[i]);
            for (i = NREQ - 1; i >= 0; i--)
            {
                WaitIO((struct IORequest *)req[i]);
                good[i] = nv_done(w, req[i], len[i], off[i]);
            }
            if (good[0] && good[1] && memcmp(a, a + alen, len[0]) != 0)
                nv_differs(w, len[0], off[0]);
        }
    }

    if (opened)
        CloseDevice((struct IORequest *)req[0]);
    for (i = 0; i < NREQ; i++)
    {
        if (req[i])
            DeleteIORequest((struct IORequest *)req[i]);
    }
    if (port)
        DeleteMsgPort(port);
    if (a)
        FreeMem(a, bufsz);
    if (b)
        FreeMem(b, bufsz);

    w->w_Done = 1;
}

static void part4(int kind, int ntasks)
{
    struct Dev *d = &devs[kind];
    struct Worker *ws = workers[kind], *w;
    ULONG cpu = d->d_Vec[0].v_CPU, irq = d->d_Vec[0].v_IRQ;
    ULONG reqs = 0, errors = 0, mism = 0, files = 0, writes = 0, last, now, on, off, overlap;
    UQUAD bytes = 0;
    struct Snap a, b;
    struct Vec v;
    int i, s, started = 0, nomem = 0, noopen = 0, left = 0, stalled = 0, firsterr = -1,
        firstbad = -1;

    memset(ws, 0, sizeof(workers[kind]));
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));

    for (i = 0; i < ntasks; i++)
    {
        ws[i].w_Id  = i;
        ws[i].w_CPU = i % g_NCPUs;
        ws[i].w_Raw = &g_Raw[kind];
        ws[i].w_Proc = spawn((kind == K_AHCI) ? FileWorker : RawWorker, "smpmsiaff.io",
                             ws[i].w_CPU, &ws[i]);
    }
    for (i = 0; i < ntasks; i++)
    {
        w = &ws[i];
        if (w->w_Proc && wait_flag(&w->w_Started, WAIT_TICKS))
        {
            if (w->w_SetupFail == 1)
                nomem++;
            else if (w->w_SetupFail == 2)
                noopen++;
            else
                started++;
        }
    }

    if (started != ntasks)
    {
        bug("[smpmsiaff] part 4: INVALID - %s: only %d of %d processes started "
            "(%d without memory, %d without the device)\n", d->d_Label, (int)started,
            (int)ntasks, (int)nomem, (int)noopen);
        inval++;
    }
    else
    {
        bug("[smpmsiaff] part 4: %s: %d processes on %d CPUs read for %d seconds, IRQ %lu "
            "on CPU %lu\n", d->d_Label, (int)ntasks, (int)g_NCPUs, (int)LOAD_SECS,
            (unsigned long)irq, (unsigned long)cpu);

        __atomic_store_n(&g_Overlap[kind], 0, __ATOMIC_SEQ_CST);
        snap(kind, &a);
        last = 0;
        for (s = 0; s < LOAD_SECS; s++)
        {
            Delay(50);
            for (i = 0, now = 0; i < ntasks; i++)
                now += rd(&ws[i].w_Reqs);
            if (now == last)
                stalled++;
            last = now;
        }
        snap(kind, &b);
    }

    for (i = 0; i < ntasks; i++)
        ws[i].w_Stop = 1;
    for (i = 0; i < ntasks; i++)
    {
        if (ws[i].w_Proc && !wait_flag(&ws[i].w_Done, WAIT_TICKS))
            left++;
    }
    if (left)
    {
        bug("[smpmsiaff] part 4: *** FAIL *** %s: %d of %d processes did not end: a request "
            "did not return\n", d->d_Label, (int)left, (int)ntasks);
        fail++;
        hung = 1;
    }

    if (started != ntasks)
        return;

    if (!left)
        ok(4, "every process ended");

    for (i = 0; i < ntasks; i++)
    {
        w = &ws[i];
        reqs   += rd(&w->w_Reqs);
        errors += w->w_Errors;
        mism   += w->w_Mismatch;
        files  += w->w_Files;
        writes += w->w_Writes;
        bytes  += w->w_Bytes;
        if (w->w_Errors && firsterr < 0)
            firsterr = i;
        if (w->w_Mismatch && firstbad < 0)
            firstbad = i;
    }

    on      = b.sn_Hits[cpu] - a.sn_Hits[cpu];
    off     = (b.sn_Total - a.sn_Total) - on;
    overlap = rd(&g_Overlap[kind]);

    hits_line(&a, &b);
    if (kind == K_AHCI)
        snprintf(what, sizeof(what), "%lu reads, %llu bytes, %lu files read whole",
                 (unsigned long)reqs, (unsigned long long)bytes, (unsigned long)files);
    else
        snprintf(what, sizeof(what), "%lu requests, %llu bytes, %lu extents written and "
                 "read back", (unsigned long)reqs, (unsigned long long)bytes,
                 (unsigned long)writes);
    bug("[smpmsiaff] part 4: %s: %s; handler calls:%s\n", d->d_Label, what, line);

    if (errors)
    {
        w = &ws[firsterr];
        if (kind == K_AHCI)
            snprintf(what, sizeof(what), "process %d: %s, IoErr() %ld after %lu bytes",
                     firsterr, g_Files[w->w_ErrCmd % MAXFILES].f_Path, (long)w->w_ErrCode,
                     (unsigned long)w->w_ErrLen);
        else
            snprintf(what, sizeof(what), "process %d: command %lu, %lu bytes at %llu: "
                     "io_Error %ld, io_Actual %lu", firsterr, (unsigned long)w->w_ErrCmd,
                     (unsigned long)w->w_ErrLen, (unsigned long long)w->w_ErrOff,
                     (long)w->w_ErrCode, (unsigned long)w->w_ErrActual);
        bug("[smpmsiaff] part 4: *** FAIL *** %s: %lu of %lu requests returned an error "
            "(the first of %s)\n", d->d_Label, (unsigned long)errors, (unsigned long)reqs,
            what);
        fail++;
    }
    else if (reqs)
        ok(4, "every request returned without error");

    if (mism)
    {
        w = &ws[firstbad];
        if (kind == K_AHCI)
            snprintf(what, sizeof(what), "process %d: %s, %lu bytes read", firstbad,
                     g_Files[w->w_BadOff % MAXFILES].f_Path, (unsigned long)w->w_BadLen);
        else
            snprintf(what, sizeof(what), "process %d: %lu bytes at %llu", firstbad,
                     (unsigned long)w->w_BadLen, (unsigned long long)w->w_BadOff);
        bug("[smpmsiaff] part 4: *** FAIL *** %s: what was read was wrong %lu times "
            "(the first of %s)\n", d->d_Label, (unsigned long)mism, what);
        fail++;
    }
    else if (reqs)
        ok(4, (kind == K_AHCI) ? "every file read had the size and checksum it had before"
                               : "every extent read twice read the same");

    if (stalled || reqs == 0)
    {
        bug("[smpmsiaff] part 4: *** FAIL *** %s: nothing completed in %d of the %d seconds\n",
            d->d_Label, (int)stalled, (int)LOAD_SECS);
        fail++;
    }
    else
        ok(4, "requests completed in every second");

    if (overlap)
    {
        bug("[smpmsiaff] part 4: *** FAIL *** %s: the handler was entered on two CPUs at "
            "once (%lu times)\n", d->d_Label, (unsigned long)overlap);
        fail++;
    }

    if (off)
    {
        bug("[smpmsiaff] part 4: *** FAIL *** %s: IRQ %lu is reported on CPU %lu, the handler "
            "ran %lu times on other CPUs\n", d->d_Label, (unsigned long)irq,
            (unsigned long)cpu, (unsigned long)off);
        fail++;
    }
    else if (!vec_get(d->d_Obj, 0, &v) || v.v_IRQ != irq || (v.v_HasCPU && v.v_CPU != cpu))
    {
        bug("[smpmsiaff] part 4: INVALID - %s: vector 0 was changed during the test "
            "(IRQ %lu on CPU %lu now)\n", d->d_Label, (unsigned long)v.v_IRQ,
            (unsigned long)v.v_CPU);
        inval++;
    }
    else if (on < MIN_LOAD_HITS)
    {
        bug("[smpmsiaff] part 4: INVALID - %s: %lu interrupts while %lu requests completed "
            "(what was read did not come from the device)\n", d->d_Label,
            (unsigned long)on, (unsigned long)reqs);
        inval++;
    }
    else
    {
        snprintf(what, sizeof(what), "%s: every call of the handler (%lu) was on CPU %lu",
                 d->d_Label, (unsigned long)on, (unsigned long)cpu);
        ok(4, what);
    }
}

/*
 * log.resource: where the drivers put what they have to say. The entries
 * are counted, and the errors and warnings after the first 'since' of
 * them are printed, LOG_SHOW at most. No verdict is made from them.
 */
static void log_count(struct LogCount *lc, ULONG since, const char *when)
{
    APTR entry = (APTR)LOGEntry_First;
    ULONG shown = 0, i;

    memset(lc, 0, sizeof(*lc));
    if (!LogResBase)
        return;

    logLockEntries(LLF_READ);
    while ((entry = logNextEntry(&entry)) != NULL && entry != (APTR)LOGEntry_Last)
    {
        ULONG flags = 0;
        char *comp = NULL, *text = NULL;
        struct TagItem tags[4];
        int bad;

        tags[0].ti_Tag  = LOGMA_Flags;
        tags[0].ti_Data = (IPTR)&flags;
        tags[1].ti_Tag  = LOGMA_Component;
        tags[1].ti_Data = (IPTR)&comp;
        tags[2].ti_Tag  = LOGMA_Entry;
        tags[2].ti_Data = (IPTR)&text;
        tags[3].ti_Tag  = TAG_DONE;
        tags[3].ti_Data = 0;

        if (logGetEntryAttrs(entry, tags) == 0)
            break;

        bad = 0;
        if (flags & (LOGF_Flag_Type_Error | LOGF_Flag_Type_Crit))
        {
            lc->lc_Err++;
            bad = 1;
        }
        else if (flags & LOGF_Flag_Type_Warn)
        {
            lc->lc_Warn++;
            bad = 1;
        }
        if (text)
        {
            if (strstr(text, "command timed out"))
                lc->lc_Timeout++;
            if (strstr(text, "NO SPACE ON RING"))
                lc->lc_NoSpace++;
            if (strstr(text, "missing ioreq"))
                lc->lc_Missing++;
        }
        if (bad && lc->lc_All >= since && shown < LOG_SHOW)
            snprintf(g_LogText[shown++], LOG_TEXT, "[%s] %s", comp ? comp : "?",
                     text ? text : "");
        lc->lc_All++;
    }
    logUnlockEntries(LLF_READ);

    bug("[smpmsiaff] log.resource %s: %lu entries, %lu errors, %lu warnings; "
        "\"command timed out\" %lu, \"NO SPACE ON RING\" %lu, \"missing ioreq\" %lu\n", when,
        (unsigned long)lc->lc_All, (unsigned long)lc->lc_Err, (unsigned long)lc->lc_Warn,
        (unsigned long)lc->lc_Timeout, (unsigned long)lc->lc_NoSpace,
        (unsigned long)lc->lc_Missing);

    /* Not under the lock: the text may end a line or not */
    for (i = 0; i < shown; i++)
    {
        char *t = g_LogText[i];
        ULONG n = strlen(t);

        while (n > 0 && (t[n - 1] == '\n' || t[n - 1] == ' '))
            t[--n] = 0;
        bug("[smpmsiaff] log.resource: %s\n", t);
    }
}

/*
 * Part 5, and the way out of every other
 */

static void drop_handlers(void)
{
    int k;

    for (k = 0; k < KINDS; k++)
    {
        if (devs[k].d_Handle)
            KrnRemIRQHandler(devs[k].d_Handle);
        devs[k].d_Handle = NULL;
    }
}

static void part5(void)
{
    struct Snap a[KINDS], b[KINDS];
    int k, had = 0, late = 0;

    for (k = 0; k < KINDS; k++)
    {
        if (devs[k].d_Handle)
            had++;
    }

    drop_handlers();
    if (!had)
        return;

    for (k = 0; k < KINDS; k++)
        snap(k, &a[k]);

    /* Not after a hang: a read may not come back */
    if (!hung && devs[K_AHCI].d_NVec && devs[K_AHCI].d_Ready)
        stir_files(QUIET_TICKS);
    else
        Delay(QUIET_TICKS);

    for (k = 0; k < KINDS; k++)
    {
        snap(k, &b[k]);
        if (b[k].sn_Total != a[k].sn_Total)
        {
            bug("[smpmsiaff] part 5: *** FAIL *** %s: the handler was called %lu times after "
                "KrnRemIRQHandler() returned\n", devs[k].d_Label,
                (unsigned long)(b[k].sn_Total - a[k].sn_Total));
            fail++;
            late = 1;
        }
    }
    if (!late)
    {
        snprintf(what, sizeof(what), "%d handler%s removed, and not called any more", had,
                 (had == 1) ? "" : "s");
        ok(5, what);
    }
}

static void cleanup(void)
{
    int i;

    drop_handlers();
    pin_self(-1);
    if (!hung)
    {
        for (i = 0; i < KINDS; i++)
            nv_close(&g_Raw[i]);
    }

    for (i = 0; i < MAXCPUS; i++)
    {
        if (g_Mask[i])
            KrnFreeCPUMask(g_Mask[i]);
        g_Mask[i] = NULL;
    }

    if (g_PCI)
        OOP_DisposeObject(g_PCI);
    g_PCI = NULL;
    if (HiddPCIDeviceAttrBase)
        OOP_ReleaseAttrBase((CONST_STRPTR)IID_Hidd_PCIDevice);
    HiddPCIDeviceAttrBase = 0;
    if (OOPBase)
        CloseLibrary(OOPBase);
    OOPBase = NULL;
}

int main(void)
{
    SIPTR args[1] = { 0 };
    struct RDArgs *rda;
    int i, k, ntasks, usable, moved = 0;

    KernelBase = OpenResource("kernel.resource");
    TaskResBase = OpenResource("task.resource");
    g_NCPUs = KernelBase ? KrnGetCPUCount() : 1;

    bug("[smpmsiaff] start: cpus=%d\n", (int)g_NCPUs);
    if (g_NCPUs < 2)
    {
        bug("[smpmsiaff] only %d CPU online - needs SMP; SKIP\n", (int)g_NCPUs);
        return RETURN_OK;
    }
    if (g_NCPUs > MAXCPUS)
        g_NCPUs = MAXCPUS;

    if ((rda = ReadArgs((CONST_STRPTR)"WRITE/S", args, NULL)) == NULL)
    {
        bug("[smpmsiaff] INVALID: bad arguments (WRITE/S)\n");
        bug("[smpmsiaff] DONE: 0 PASS, 0 FAIL, 1 INVALID\n");
        return RETURN_WARN;
    }
    g_Write = args[0] ? 1 : 0;
    FreeArgs(rda);

    for (i = 0; i < g_NCPUs; i++)
    {
        if ((g_Mask[i] = KrnAllocCPUMask()) == NULL)
            break;
        KrnClearCPUMask(g_Mask[i]);
        KrnGetCPUMask(i, g_Mask[i]);
    }
    if (i < g_NCPUs)
    {
        cleanup();
        bug("[smpmsiaff] INVALID: no CPU masks\n");
        bug("[smpmsiaff] DONE: 0 PASS, 0 FAIL, 1 INVALID\n");
        return RETURN_WARN;
    }

    if ((OOPBase = OpenLibrary((CONST_STRPTR)"oop.library", 0)) != NULL)
    {
        HiddPCIDeviceAttrBase = OOP_ObtainAttrBase((CONST_STRPTR)IID_Hidd_PCIDevice);
        if (HiddPCIDeviceAttrBase)
            g_PCI = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_PCI, NULL);
    }
    if (g_PCI == NULL)
    {
        cleanup();
        bug("[smpmsiaff] SKIP (no PCI on this machine)\n");
        bug("[smpmsiaff] DONE: 0 PASS, 0 FAIL, 0 INVALID\n");
        return RETURN_OK;
    }

    find_devices();
    usable = part1();
    if (usable == 0)
    {
        if (!fail)
            bug("[smpmsiaff] SKIP (the PCI class reports no vector for an AHCI, NVMe or xHCI "
                "controller)\n");
        cleanup();
        verdict();
        return fail ? RETURN_FAIL : RETURN_OK;
    }

    part2();

    /* What the drivers have logged up to here; after the load, what they logged since */
    LogResBase = OpenResource("log.resource");
    log_count(&g_Log0, (ULONG)~0, "at the start");

    for (k = 0; k < KINDS; k++)
    {
        if (devs[k].d_NVec && devs[k].d_Vec[0].v_CPU != 0)
            moved++;
    }
    if (!moved)
        bug("[smpmsiaff] every vector is on the boot CPU: delivery to another CPU is not "
            "seen in this run\n");

    ntasks = 2 * g_NCPUs;
    if (ntasks > MAXTASKS)
        ntasks = MAXTASKS;

    /* What can be read */
    if (devs[K_AHCI].d_NVec)
    {
        list_files();
        if (g_NFiles >= MIN_FILES)
            devs[K_AHCI].d_Ready = 1;
        else
        {
            bug("[smpmsiaff] INVALID: %lu files found on SYS:, too few to read\n",
                (unsigned long)g_NFiles);
            inval++;
        }
    }
    for (k = K_NVME; k < KINDS; k++)
    {
        if (!devs[k].d_NVec)
            continue;
        if (nv_open(k, ntasks))
            devs[k].d_Ready = 1;
        else
            nv_close(&g_Raw[k]);
    }

    /* Our handlers, beside the drivers' */
    for (k = 0; k < KINDS; k++)
    {
        struct Dev *d = &devs[k];

        if (!d->d_NVec || (k != K_XHCI && !d->d_Ready))
            continue;
        for (i = 0; i <= MAXCPUS; i++)
            __atomic_store_n(&g_Hits[k][i], 0, __ATOMIC_SEQ_CST);
        d->d_Handle = KrnAddIRQHandler(d->d_Vec[0].v_IRQ, (APTR)MsiHandler, (APTR)(IPTR)k, NULL);
        if (d->d_Handle == NULL)
        {
            bug("[smpmsiaff] INVALID: %s: IRQ %lu takes no handler\n", d->d_Label,
                (unsigned long)d->d_Vec[0].v_IRQ);
            inval++;
        }
    }

    for (k = 0; k < KINDS; k++)
    {
        struct Dev *d = &devs[k];

        if (d->d_Handle)
            part3(k);
        else if (d->d_NVec && k != K_XHCI && !d->d_Ready)
            bug("[smpmsiaff] part 3: SKIP %s (nothing to read)\n", d->d_Label);
    }

    for (k = 0; k < KINDS; k++)
    {
        struct Dev *d = &devs[k];

        if (!d->d_Handle || !d->d_Ready)
        {
            if (d->d_NVec)
                bug("[smpmsiaff] part 4: SKIP %s (nothing to read)\n", d->d_Label);
            continue;
        }
        if (hung)
        {
            bug("[smpmsiaff] part 4: %s not run after a hang\n", d->d_Label);
            continue;
        }

        if (k == K_AHCI)
        {
            scan_files();
            if (g_NFiles < MIN_FILES)
            {
                bug("[smpmsiaff] part 4: INVALID - %s: %lu files could be read whole, too "
                    "few\n", d->d_Label, (unsigned long)g_NFiles);
                inval++;
                d->d_Ready = 0;
                continue;
            }
        }

        part4(k, ntasks);
    }

    /* After a hang the calls below may not come back: the verdict comes first */
    if (hung)
        verdict();

    part5();

    /* The processes are out of their code before it goes */
    Delay(25);

    {
        struct LogCount lc;

        log_count(&lc, g_Log0.lc_All, "at the end");
    }

    cleanup();
    verdict();

    return fail ? RETURN_FAIL : (inval ? RETURN_WARN : RETURN_OK);
}

#else /* !__x86_64__ */

int main(void)
{
    bug("[smpmsiaff] SKIP (vectors are given a CPU on pc only)\n");
    bug("[smpmsiaff] DONE: 0 PASS, 0 FAIL, 0 INVALID\n");
    return RETURN_OK;
}

#endif

#else
int main(void)
{
    return RETURN_WARN;
}
#endif
