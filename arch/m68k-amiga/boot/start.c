/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: m68k-amiga bootstrap to exec.
    Lang: english
 */

#define DEBUG 0
#include <aros/debug.h>

#include <aros/kernel.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <hardware/cpu/memory.h>

#include "memory.h"

#include "kernel_romtags.h"
#include "kernel_base.h"

#define __AROS_KERNEL__
#include "exec_intern.h"

#include "amiga_hwreg.h"
#include "amiga_irq.h"

#include "early.h"
#include "debug.h"

#define SS_STACK_SIZE	0x02000

static BOOL iseven(APTR p)
{
    return (((ULONG)p) & 1) == 0;
}

struct BootStruct *GetBootStruct(struct ExecBase *eb)
{
    if (!iseven(eb))
        return NULL;
    ULONG *coldcapture = eb->ColdCapture;
    if (coldcapture && iseven(coldcapture) && coldcapture[1] == AROS_MAKE_ID('F','A','K','E')) {
        struct BootStruct *BootS = (struct BootStruct*)(coldcapture[2]);
        if (BootS->magic == ABS_BOOT_MAGIC)
            return BootS;
    }
    return NULL;
}

#if 0
static void dumpmemory(struct MemHeader *mh)
{
    struct MemChunk *mc = mh->mh_First;
    DEBUGPUTS(("\n"));
    DEBUGPUTS((mh->mh_Node.ln_Name));
    DEBUGPUTS(("\n"));
    DEBUGPUTHEX(("Addr : ", (ULONG)mh));
    DEBUGPUTHEX(("Attrs: ", (ULONG)mh->mh_Attributes));
    DEBUGPUTHEX(("Lower: ", (ULONG)mh->mh_Lower));
    DEBUGPUTHEX(("Upper: ", (ULONG)mh->mh_Upper));
    DEBUGPUTHEX(("Free : ", (ULONG)mh->mh_Free));
    DEBUGPUTHEX(("First: ", (ULONG)mc));
    while (mc) {
        DEBUGPUTHEX(("Next : ", (ULONG)mc->mc_Next));
        DEBUGPUTHEX(("Bytes: ", (ULONG)mc->mc_Bytes));
        mc = mc->mc_Next;
    }
}
static void dumpallmemory(struct ExecBase *SysBase)
{
    struct MemHeader *node;
    ForeachNode (&SysBase->MemList, node) {
        dumpmemory(node);
    }
}
#endif	

extern const struct Resident Exec_resident;
extern struct ExecBase *AbsExecBase;

extern void __attribute__((interrupt)) Exec_Supervisor_Trap (void);
extern void __attribute__((interrupt)) Exec_Supervisor_Trap_00 (void);

#define _AS_STRING(x)	#x
#define AS_STRING(x)	_AS_STRING(x)
	
/* Create a sign extending call stub:
 * foo:
 *   jsr AROS_SLIB_ENTRY(funcname, libname, funcid)
 *     0x4eb9 .... ....
 *   ext.w %d0	// EXT_BYTE only
 *     0x4880	
 *   ext.l %d0	// EXT_BYTE and EXT_WORD
 *     0x48c0
 *   rts
 *     0x4e75
 */
#define EXT_BYTE(lib, libname, funcname, funcid) \
	do { \
		void libname##_##funcname##_Wrapper(void) \
		{ asm volatile ( \
			"jsr " AS_STRING(AROS_SLIB_ENTRY(funcname, libname, funcid)) "\n" \
			"ext.w %d0\n" \
			"ext.l %d0\n" \
			"rts\n"); } \
		/* Insert into the library's jumptable */ \
		__AROS_SETVECADDR(lib, funcid, libname##_##funcname##_Wrapper); \
	} while (0)
#define EXT_WORD(lib, libname, funcname, funcid) \
	do { \
		void libname##_##funcname##_Wrapper(void) \
		{ asm volatile ( \
			"jsr " AS_STRING(AROS_SLIB_ENTRY(funcname, libname, funcid)) "\n" \
			"ext.l %d0\n" \
			"rts\n"); } \
		/* Insert into the library's jumptable */ \
		__AROS_SETVECADDR(lib, funcid, libname##_##funcname##_Wrapper); \
	} while (0)
/*
 * Create a register preserving call stub:
 * foo:
 *   movem.l %d0-%d1/%a0-%a1,%sp@-
 *     0x48e7 0xc0c0
 *   jsr AROS_SLIB_ENTRY(funcname, libname, funcid)
 *     0x4eb9 .... ....
 *   movem.l %sp@+,%d0-%d1/%d0-%a1
 *     0x4cdf 0x0303
 *   rts
 *     0x4e75
 */
#define PRESERVE_ALL(lib, libname, funcname, funcid) \
	do { \
		void libname##_##funcname##_Wrapper(void) \
	        { asm volatile ( \
	        	"movem.l %d0-%d1/%a0-%a1,%sp@-\n" \
	        	"jsr " AS_STRING(AROS_SLIB_ENTRY(funcname, libname, funcid)) "\n" \
	        	"movem.l %sp@+,%d0-%d1/%a0-%a1\n" \
	        	"rts\n" ); } \
		/* Insert into the library's jumptable */ \
		__AROS_SETVECADDR(lib, funcid, libname##_##funcname##_Wrapper); \
	} while (0)
/* Inject arbitrary code into the jump table
 * Used for GetCC and nano-stubs
 */
#define FAKE_IT(lib, libname, funcname, funcid, ...) \
	do { \
		UWORD *asmcall = (UWORD *)__AROS_GETJUMPVEC(lib, funcid); \
		const UWORD code[] = { __VA_ARGS__ }; \
		asmcall[0] = code[0]; \
		asmcall[1] = code[1]; \
		asmcall[2] = code[2]; \
	} while (0)
/* Inject a 'move.w #value,%d0; rts" sequence into the
 * jump table, to fake an private syscall.
 */
#define FAKE_ID(lib, libname, funcname, funcid, value) \
	FAKE_IT(lib, libname, funcname, funcid, 0x303c, value, 0x4e75)

extern void SuperstackSwap(void);
/* This calls the register-ABI library
 * routine Exec/InitCode, for use in NewStackSwap()
 */
static LONG doInitCode(struct BootStruct *BootS)
{
    /* Attempt to allocate a new supervisor stack */
    do {
        APTR ss_stack;
        ULONG ss_stack_size;
        if (BootS && BootS->ss_address) {
            ss_stack = BootS->ss_address;
            ss_stack_size = BootS->ss_size;
            DEBUGPUTS(("Using AROSBootStrap SS Stack"));
        } else {
            ss_stack = AllocMem(SS_STACK_SIZE, MEMF_ANY | MEMF_CLEAR | MEMF_REVERSE);
            if (ss_stack && ((ULONG)ss_stack & (PAGE_SIZE - 1))) {
                 /* Normally ss_stack is page aligned because it is first MEMF_REVERSE
                  * allocation. But we must check it because enabled mungwall or expansion
                  * boot rom code can allocate some memory.
                  */
                FreeMem(ss_stack, SS_STACK_SIZE);
                ss_stack = AllocMem(SS_STACK_SIZE + PAGE_SIZE - 1, MEMF_ANY | MEMF_CLEAR | MEMF_REVERSE);
                ss_stack = (APTR)(((ULONG)ss_stack + PAGE_SIZE - 1) & PAGE_MASK);
            }
            ss_stack_size = SS_STACK_SIZE;
        }
        DEBUGPUTHEX(("SS  lower", (ULONG)ss_stack));
        DEBUGPUTHEX(("SS  upper", (ULONG)ss_stack + ss_stack_size - 1));
        if (ss_stack == NULL) {
            DEBUGPUTS(("PANIC! Can't allocate a new system stack!\n"));
            Early_Alert(CODE_ALLOC_FAIL);
            break;
        }
        SysBase->SysStkLower    = ss_stack;
        SysBase->SysStkUpper    = ss_stack + ss_stack_size;
        SetSysBaseChkSum();

        Supervisor((ULONG_FUNC)SuperstackSwap);
    } while(0);

    /* Move boot tags from temp supervisor stack. MMU debugging will detect access fault later if not moved. */
    if (PrivExecBase(SysBase)->PlatformData.BootMsg && TypeOfMem(PrivExecBase(SysBase)->PlatformData.BootMsg) == 0) {
        UWORD size = 0;
        struct TagItem *bootmsg, *newbootmsg;

        bootmsg = PrivExecBase(SysBase)->PlatformData.BootMsg;
        while (bootmsg[size].ti_Tag)
            size++;
        newbootmsg = AllocMem(sizeof(struct TagItem) * (size + 1), MEMF_CLEAR | MEMF_PUBLIC);
        if (newbootmsg) {
            size = 0;
            bootmsg = PrivExecBase(SysBase)->PlatformData.BootMsg;
             while (bootmsg[size].ti_Tag) {
                 newbootmsg[size].ti_Tag = bootmsg[size].ti_Tag; 
                 newbootmsg[size].ti_Data = bootmsg[size].ti_Data;
                 size++;
             }
        }
        PrivExecBase(SysBase)->PlatformData.BootMsg = newbootmsg;
    }

    InitCode(RTF_COLDSTART, 0);

    return 0;
}

extern BYTE _rom_start;
extern BYTE _rom_end;
extern BYTE _ext_start;
extern BYTE _ext_end;
extern BYTE _ss;
extern BYTE _ss_end;

static BOOL IsSysBaseValidNoVersion(struct ExecBase *sysbase)
{
	if (!iseven(sysbase))
		return FALSE;
    if (sysbase == NULL || (((ULONG)sysbase) & 0x80000001))
        return FALSE;
    if (sysbase->ChkBase != ~(IPTR)sysbase)
        return FALSE;
    return GetSysBaseChkSum(sysbase) == 0xffff;
}

#define KICKMEM_MASK 0xc0000000
#define KICKMEM_ALLOCATED 0x80000000
/* Use following if loaded to memory that is not autoconfig or autodetect */
#define KICMMEM_DONOTALLOCATE 0x40000000

static void resetKickMem(struct BootStruct *bs)
{
    struct MemList *ml;
    WORD i;

    if (bs == NULL)
        return;
    /* Mark as unallocated.
     * This list is guaranteed to be located in memory
     * that is immediately available after reset.
     */
    ForeachNode(bs->mlist, ml) {
        for(i = 0; i < ml->ml_NumEntries; i++) {
            ml->ml_ME[i].me_Length &= ~KICKMEM_ALLOCATED;
        }
    }
}
static void allocKickMem(struct ExecBase *SysBase, struct BootStruct *bs, struct MemHeader *mh, UWORD phase)
{
    struct MemList *ml;
    BOOL panic = FALSE;
    WORD i;
    
    if (phase > 0)
        bs = GetBootStruct(SysBase);
    if (bs == NULL)
        return;
    DEBUGPUTHEX(("KickMem allocation", phase));
    ForeachNode(bs->mlist, ml) {
        DEBUGPUTHEX(("Type   ", ml->ml_Node.ln_Type));
        for(i = 0; i < ml->ml_NumEntries; i++) {
        	WORD ok = -1;
            APTR start = ml->ml_ME[i].me_Addr;
            ULONG len = ml->ml_ME[i].me_Length;
            DEBUGPUTHEX(("Address", (ULONG)start));
            DEBUGPUTHEX(("Length ", len & ~KICKMEM_MASK));
            if (len & KICKMEM_ALLOCATED) {
                DEBUGPUTS(("-> already allocated\n"));
                continue;
            }
            if (len & KICMMEM_DONOTALLOCATE) {
                DEBUGPUTS(("-> pre-reserved\n"));
                continue;
            }
            if (phase > 0) {
                if (TypeOfMem(start) == 0) {
                    if (phase == 2) {
                        DEBUGPUTS(("PANIC! Allocation still not in system pool!\n"));
                        panic = TRUE;
                    } else {
                        DEBUGPUTS(("-> Not in system pool yet\n"));
                    }
                    ok = -2;
                } else {
                    ok = InternalAllocAbs(start, len, SysBase) != 0 ? 1 : 0;
                }
            } else {
                if (start >= mh->mh_Lower && start < mh->mh_Upper) {
                    ok = Early_AllocAbs(mh, start, len) != 0 ? 1 : 0;
                }
            }
            if (ok > 0) {
                DEBUGPUTS(("-> Allocated\n"));
                ml->ml_ME[i].me_Length |= KICKMEM_ALLOCATED;
            } else if (ok == 0) {
                DEBUGPUTS(("PANIC! Early ROM memory allocation failed!\n"));
                DEBUGPUTHEX(("LOWER  ", (ULONG)mh->mh_Lower));
                DEBUGPUTHEX(("UPPER  ", (ULONG)mh->mh_Upper));
                panic = TRUE;
            } else if (ok == -1) {
                DEBUGPUTS(("-> Not yet available\n"));
            }
        }
    }
    if (panic)
        Early_Alert(AT_DeadEnd | AN_MemoryInsane);
    if (phase == 2) {
        DEBUGPUTS(("KickMem allocation complete\n"));
        resetKickMem(bs);
    }
}

static struct MemHeader *addmemoryregion(ULONG startaddr, ULONG size, struct BootStruct *bs, BOOL magicfast)
{
    if (size < 65536)
        return NULL;
    if (magicfast) {
        krnCreateMemHeader("magic fast memory", -8, 
            (APTR)startaddr, size,
            MEMF_FAST | MEMF_PUBLIC | ((startaddr & 0xff000000) == 0 ? MEMF_24BITDMA : 0));
    } else if (startaddr < 0x00c00000) {
        krnCreateMemHeader("chip memory", -10,
            (APTR)startaddr, size,
            MEMF_CHIP | MEMF_KICK | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA);
    } else {
        krnCreateMemHeader("memory", -5, 
            (APTR)startaddr, size,
            MEMF_FAST | MEMF_KICK | MEMF_PUBLIC | MEMF_LOCAL | ((startaddr & 0xff000000) == 0 ? MEMF_24BITDMA : 0));
    }
    DEBUGPUTS(("Added memory header:\n"));
    DEBUGPUTHEX(("Lower", startaddr));
    DEBUGPUTHEX(("Upper", startaddr + size));
    allocKickMem(NULL, bs, (struct MemHeader*)startaddr, 0);
    return (struct MemHeader*)startaddr;
}

/* Called after autoconfig. Autoconfig RAM is now available */
void InitKickMem(struct ExecBase *SysBase)
{
    allocKickMem(SysBase, NULL, NULL, 1);
}
/* Called after diag module. Diag ROM enabled RAM is now available */
void InitKickMemDiag(struct ExecBase *SysBase)
{
    allocKickMem(SysBase, NULL, NULL, 2);
}

static void dumpres(struct Resident **reslist)
{
#if AROS_SERIAL_DEBUG
    while (*reslist) {
        struct Resident *RomTag = *reslist++;
        bug("* %p: %4d %02x %3d \"%s\"\n",
            RomTag,
            RomTag->rt_Pri,
            RomTag->rt_Flags,
            RomTag->rt_Version,
            RomTag->rt_Name);
    }
#endif
}

static ULONG countres(struct Resident **reslist)
{
    ULONG cnt = 0;
    while (*reslist) {
        if (((ULONG)(*reslist)) & RESLIST_NEXT) {
            reslist = (struct Resident**)(((ULONG)(*reslist)) & ~RESLIST_NEXT);
            continue;
        }
        cnt++;
        reslist++;
    }
    return cnt;
}

static struct Resident **copyres(struct Resident **start, struct Resident **newreslist, struct Resident **oldreslist)
{
    while (*oldreslist) {
        struct Resident *r;
        if (((ULONG)(*oldreslist)) & RESLIST_NEXT) {
            oldreslist = (struct Resident**)(((ULONG)(*oldreslist)) & ~RESLIST_NEXT);
            continue;
        }
        r = *oldreslist;
        /* Copy only if Resident appears to be accessible */
        if (r->rt_MatchWord == RTC_MATCHWORD && r->rt_MatchTag == r) {
            /* If same Resident is already in list, select higher version/priority */
            BOOL skip = FALSE;
            ULONG i;
            for (i = 0; start[i] != NULL; i++) {
                struct Resident *r2 = start[i];
                if (!strcmp(r->rt_Name, r2->rt_Name)) {
                    if (r->rt_Version < r2->rt_Version ||
                        (r->rt_Version == r2->rt_Version && r->rt_Pri <= r2->rt_Pri))  {
                        skip = TRUE;
                    } else {
                        *oldreslist = r2;
                        skip = TRUE;
                    }
                }
            }
            if (!skip) {
                *newreslist++ = r;
                *newreslist = NULL;
            }
        }
        oldreslist++;
    }
    *newreslist = NULL;
    return newreslist;
}

/* Find remaining residents after autoconfig because part of our ROM image
 * may have been located in autoconfig RAM that disappeared after reset.
 * Scan all memory lists, even if memory is not yet allocated, all memory
 * is mapped at this point but it may not be in system memory list. Yet.
 */
static void CollectKickResidents(struct BootStruct *BootS, struct ExecBase *SysBase)
{
    ULONG oldtotal, newtotal, total;
    BOOL sorted;
    struct Resident **reslist, **resend;

    newtotal = countres(BootS->reslist);
    if (!newtotal)
        return;
    oldtotal = countres(SysBase->ResModules);
    total = newtotal + oldtotal;
    DEBUGPUTHEX(("OldRes", oldtotal));
    DEBUGPUTHEX(("NewRes", newtotal));
    DEBUGPUTHEX(("Total ", total));
    reslist = AllocMem(sizeof(struct Resident*) * (total + 1), MEMF_PUBLIC);
    if (!reslist)
        return;
    *reslist = NULL;
    resend = copyres(reslist, reslist, SysBase->ResModules);
    resend = copyres(reslist, resend, BootS->reslist);
    total = resend - reslist;
    DEBUGPUTHEX(("Total2", total));
    do
    {
        WORD i;
        sorted = TRUE;
        for (i = 0; i < total - 1; i++) {
            if (reslist[i]->rt_Pri < reslist[i + 1]->rt_Pri) {
                struct Resident *tmp;
                tmp = reslist[i + 1];
                reslist[i + 1] = reslist[i];
                reslist[i] = tmp;
                sorted = FALSE;
            }
        }
    } while (!sorted);
    dumpres(reslist);
    SysBase->ResModules = reslist;
}

#if 0 // debug stuff, do not remove
static ULONG SumKickDataX(struct ExecBase *sb)
{
    ULONG chksum = 0;
    BOOL isdata = FALSE;
    struct ExecBase *sysbase = sb;

    if (sysbase->KickTagPtr) {
        IPTR *list = sysbase->KickTagPtr;
        while(*list)
        {
            chksum += (ULONG)*list;
            DEBUGPUTHEX(("LIST", (ULONG)list));
            DEBUGPUTHEX(("LISTP", (ULONG)*list));
            DEBUGPUTHEX(("CHK", chksum));
            if(*list & RESLIST_NEXT) { list = (IPTR *)(*list & ~RESLIST_NEXT); continue; }
            list++;
            isdata = TRUE;
        }
    }

    if (sysbase->KickMemPtr) {
        struct MemList *ml = (struct MemList*)sysbase->KickMemPtr;
        while (ml) {
            UBYTE i;
            ULONG *p = (ULONG*)ml;
            for (i = 0; i < sizeof(struct MemList) / sizeof(ULONG); i++)
                chksum += p[i];
            DEBUGPUTHEX(("MEM", (ULONG)p));
            DEBUGPUTHEX(("CHK", chksum));
            ml = (struct MemList*)ml->ml_Node.ln_Succ;
            isdata = TRUE;
        }
    }
    if (isdata && !chksum)
        chksum--;
    return chksum;
}
#endif

void doColdCapture(void)
{
    APTR ColdCapture = SysBase->ColdCapture;

    if (ColdCapture == NULL)
        return;
    if (GetBootStruct(SysBase)) {
        /* Fake SysBase installed by AROSBootstrap.
         *
         * In this case, ColdCapture is the trampoline executed
         * by the AOS ROM to get into AROS. We need to keep
         * ColdCapture around in AROS SysBase, but we don't
         * want to execute it (and cause an infinite loop).
         */
        DEBUGPUTS(("[ColdCapture] Ignoring AOS->AROS trampoline\n"));
        return;
    }

    SysBase->ColdCapture = NULL;
    /* ColdCapture calling method is a little
     * strange. It's in supervisor mode, requires
     * the return location in A5, and SysBase in A6.
     */
    asm volatile (
        "move.l %0,%%a0\n"
        "move.l %1,%%a6\n"
        "move.l #0f,%%a5\n"
       "jmp (%%a0)\n"
        "0:\n"
        :
        : "m" (ColdCapture), "m" (SysBase)
        : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
          "a0", "a1", "a2", "a3", "a4", "a5", "a6");
}

static void RomInfo(IPTR rom)
{
#if AROS_SERIAL_DEBUG && (DEBUG > 0)
    APTR ptr = (APTR)rom;
    CONST_STRPTR str;

    if ((*(UWORD *)(ptr + 8) == 0x0000) &&
        (*(UWORD *)(ptr + 10) == 0xffff) &&
        (*(UWORD *)(ptr + 12) == *(UWORD *)(ptr + 16)) &&
        (*(UWORD *)(ptr + 14) == *(UWORD *)(ptr + 18)) &&
        (*(UWORD *)(ptr + 20) == 0xffff) &&
        (*(UWORD *)(ptr + 22) == 0xffff)) {
        DEBUGPUTHEX(("ROM Location", rom));
        DEBUGPUTD(("   Version", *(UWORD *)(ptr + 12)));
        DEBUGPUTD(("  Revision", *(UWORD *)(ptr + 14)));
        str = (ptr + 24);
        DEBUGPUTS(("  ROM Type: ")); DEBUGPUTS((str)); DEBUGPUTS(("\n"));
        str += strlen(str) + 1;
        DEBUGPUTS((" Copyright: ")); DEBUGPUTS((str));
        str += strlen(str) + 1;
        DEBUGPUTS((str));
        str += strlen(str) + 1;
        DEBUGPUTS((str));
        DEBUGPUTS(("\n"));
        str += strlen(str) + 1;
        DEBUGPUTS((" ROM Model: ")); DEBUGPUTS((str));
        DEBUGPUTS(("\n"));
    }
#endif
}

static UWORD GetAttnFlags(ULONG *cpupcr)
{
    /* Convert CPU/FPU flags to AttnFlags */
    UWORD attnflags = cpupcr[0] & 0xffff;
    if (attnflags & (AFF_68030 | AFF_68040 | AFF_68060))
        attnflags |= AFF_ADDR32;
    if (cpupcr[0] & 0xffff0000) {
        attnflags |= AFF_FPU;
        if (attnflags & (AFF_68040 | AFF_68060))
            attnflags |= AFF_FPU40;
            // AFF_68881 | AFF_68882 set only if 040/060 math emulation running
        else if (((cpupcr[0] >> 16) & 0xff) <= 0x1f)
            attnflags |= AFF_68881;
        else
            attnflags |= AFF_68881 | AFF_68882;
    }

#if AROS_SERIAL_DEBUG && (DEBUG > 0)
    DEBUGPUTS(("CPU: "));
    if (attnflags & AFF_68060)
        DEBUGPUTS(("68060"));
    else if (attnflags & AFF_68040)
        DEBUGPUTS(("68040"));
    else if (attnflags & AFF_68030)
        DEBUGPUTS(("68030"));
    else if (attnflags & AFF_68020) {
        if (attnflags & AFF_ADDR32)
            DEBUGPUTS(("68020"));
        else
            DEBUGPUTS(("68EC020"));
    } else if (attnflags & AFF_68010)
        DEBUGPUTS(("68010"));
    else
        DEBUGPUTS(("68000"));
    DEBUGPUTS((" FPU: "));
    if (attnflags & AFF_FPU40) {
        if (attnflags & AFF_68060)
            DEBUGPUTS(("68060"));
        else if (attnflags & AFF_68040)
            DEBUGPUTS(("68040"));
        else
            DEBUGPUTS(("-"));
    } else if (attnflags & AFF_68882)
        DEBUGPUTS(("68882"));
    else if (attnflags & AFF_68881)
        DEBUGPUTS(("68881"));
    else
        DEBUGPUTS(("-"));
    DEBUGPUTS(("\n"));
    if (cpupcr[1])
        DEBUGPUTHEX(("PCR", cpupcr[1]));
#endif
    return attnflags;
}

void exec_boot(ULONG *membanks, ULONG *cpupcr)
{
    struct TagItem bootmsg[] = {
        /* nomonitors - Until we have working m68k PCI support,
         *              attempting to load the monitor drivers
         *              just wastes a lot of time during boot
         */
#if AROS_SERIAL_DEBUG
        { KRN_CmdLine, (IPTR)"nomonitors sysdebug=InitCode" },
//      { KRN_CmdLine, (IPTR)"nomonitors sysdebug=InitCode,debugmmu,mungwall" },
#else
        { KRN_CmdLine, (IPTR)"nomonitors" },
#endif
        { KRN_KernelStackBase, (IPTR)&_ss },
        { KRN_KernelStackSize, (IPTR)(&_ss_end - &_ss) },
        { TAG_END },
    };
    struct TagItem *bootmsgptr = bootmsg;
    volatile APTR *trap;
    int i;
    BOOL wasvalid, arosbootstrapmode;
    UWORD *kickrom[8];
    struct MemHeader *mh;
    LONG oldLastAlert[4];
    ULONG oldmem;
    UWORD attnflags;
    APTR ColdCapture = NULL, CoolCapture = NULL, WarmCapture = NULL;
    APTR KickMemPtr = NULL, KickTagPtr = NULL, KickCheckSum = NULL;
    struct BootStruct *BootS = NULL;
    /* We can't use the global 'SysBase' symbol, since
     * the compiler does not know that PrepareExecBase
     * may change it out from under us.
     */
    struct ExecBase *oldSysBase = *(APTR *)4;
#define SysBase CANNOT_USE_SYSBASE_SYMBOL_HERE

#if AROS_SERIAL_DEBUG
    DebugInit();
#endif

    trap = (APTR *)(NULL);

    /* Set all the exceptions to the Early_Exception */
    for (i = 2; i < 64; i++) {
        if (i != 31) // Do not overwrite NMI
            trap[i] = Early_Exception;
    }

    /* Let the world know we exist */
    DebugInit();
    DEBUGPUTS(("[reset]\n"));
    RomInfo(0xf80000);
    RomInfo(0xe00000);
    RomInfo(0xf00000);

    attnflags = GetAttnFlags(cpupcr);

    /* Zap out old SysBase if invalid */
    arosbootstrapmode = FALSE;
    wasvalid = IsSysBaseValid(oldSysBase);
    if (wasvalid) {
        DEBUGPUTHEX(("[SysBase] was at", (ULONG)oldSysBase));
    } else {
        wasvalid = IsSysBaseValidNoVersion(oldSysBase);
        if (wasvalid) {
            DEBUGPUTHEX(("[SysBase] fakebase at", (ULONG)oldSysBase));
            BootS = GetBootStruct(oldSysBase);
            if (BootS) {
                DEBUGPUTHEX(("BootStruct at", (ULONG)BootS));
                DEBUGPUTHEX(("Original  SysBase at", (ULONG)BootS->RealBase));
                DEBUGPUTHEX(("Secondary SysBase at", (ULONG)BootS->RealBase2));
                bootmsgptr = BootS->kerneltags;
                arosbootstrapmode = TRUE;
            }
            wasvalid = TRUE;
        } else {
            DEBUGPUTHEX(("[SysBase] invalid at", (ULONG)oldSysBase));
            wasvalid = FALSE;
        }
    }
    if (bootmsgptr[0].ti_Tag == KRN_CmdLine) {
        DEBUGPUTS(("[SysBase] kernel commandline '"));
        DEBUGPUTS(((CONST_STRPTR)bootmsgptr[0].ti_Data));
        DEBUGPUTS(("'\n"));
    }

	if (wasvalid) {
        /* Save reset proof vectors */
        ColdCapture  = oldSysBase->ColdCapture;
        CoolCapture  = oldSysBase->CoolCapture;
        WarmCapture  = oldSysBase->WarmCapture;
        KickMemPtr   = oldSysBase->KickMemPtr; 
        KickTagPtr   = oldSysBase->KickTagPtr;
        KickCheckSum = oldSysBase->KickCheckSum;

        /* Mark the oldSysBase as processed */
        oldSysBase = NULL;
    }

    if (BootS && BootS->magicfastmem) {
        /* Add early magic fast ram pool
         * Makes it possible to have execbase and others in fast ram even
         * if fast ram is non-autocnfig diagrom type.
         */
        membanks -= 2;
        membanks[0] = (ULONG)BootS->magicfastmem;
        membanks[1] = BootS->magicfastmemsize;
    }

    /* Adjust to skip the first 1K/4K bytes of
     * Chip RAM. It's reserved for the Trap area.
     */
    for (i = 0; membanks[i + 2 + 1]; i += 2);
    if (arosbootstrapmode || (attnflags & AFF_68030))
        membanks[i + 0] = 0x1000;
    else
        membanks[i + 0] = 0x400;
    membanks[i + 1] -= membanks[i + 0];
 
#if AROS_SERIAL_DEBUG && (DEBUG > 0)
    for (i = 0; membanks[i + 1]; i += 2) {
        ULONG addr = membanks[i + 0];
        ULONG size = membanks[i + 1];
        DEBUGPUTHEX(("RAM lower", addr));
        DEBUGPUTHEX(("RAM upper", addr + size - 1));
    }
#endif

    /* Look for 'HELP' at address 0 - we're recovering
     * from a fatal alert
     */
    if (trap[0] == (APTR)0x48454c50) {
        for (i = 0; i < 4; i++)
            oldLastAlert[i] = (LONG)trap[64 + i];

        DEBUGPUTHEX(("LastAlert Alert", oldLastAlert[0]));
        DEBUGPUTHEX(("LastAlert  Task", oldLastAlert[1]));
    } else {
        oldLastAlert[0] = (LONG)-1;
        oldLastAlert[1] = 0;
        oldLastAlert[2] = 0;
        oldLastAlert[2] = 0;
    }

    /* Clear alert marker */
    trap[0] = 0;

    DEBUGPUTHEX(("SS  lower", (ULONG)&_ss));
    DEBUGPUTHEX(("SS  upper", (ULONG)&_ss_end - 1));

    Early_ScreenCode(CODE_RAM_CHECK);

    if (arosbootstrapmode) {
        /* Scan only first rom image.
         * The rest is in BootStruct resident list
         */
        kickrom[0] = (UWORD*)&_rom_start;
        kickrom[1] = (UWORD*)&_rom_end;
        kickrom[2] = (UWORD*)0x00f00000;
        kickrom[3] = (UWORD*)0x00f80000;
        kickrom[4] = (UWORD*)~0;
        kickrom[5] = (UWORD*)~0;
        resetKickMem (BootS);
    } else {
        kickrom[0] = (UWORD*)&_rom_start;
        kickrom[1] = (UWORD*)&_rom_end;
        kickrom[2] = (UWORD*)0x00f00000;
        kickrom[3] = (UWORD*)0x00f80000;
        kickrom[4] = (UWORD*)&_ext_start;
        kickrom[5] = (UWORD*)&_ext_end;
        kickrom[6] = (UWORD*)~0;
        kickrom[7] = (UWORD*)~0;
    }

    mh = addmemoryregion(membanks[0], membanks[1],
        BootS, BootS && (ULONG)BootS->magicfastmem == membanks[0]);
    if (mh == NULL) {
        DEBUGPUTS(("Can't create initial memory header!\n"));
        Early_Alert(AT_DeadEnd | AG_NoMemory);
    }

#if AROS_SERIAL_DEBUG && (DEBUG > 0)
    for (i = 0; kickrom [i] != (UWORD*)~0; i += 2) {
        DEBUGPUTHEX(("Resident start", (ULONG)kickrom[i]));
        DEBUGPUTHEX(("Resident end  ", (ULONG)kickrom[i + 1]));
    }
#endif

    /*
     * Call the SysBase initialization.
     */
    Early_ScreenCode(CODE_EXEC_CHECK);
    if (!krnPrepareExecBase(kickrom, mh, bootmsgptr))
        Early_Alert(AT_DeadEnd | AG_MakeLib | AO_ExecLib);

    /* From here on, we can reference SysBase */
#undef SysBase
    DEBUGPUTHEX(("[SysBase] at", (ULONG)SysBase));

    PrivExecBase(SysBase)->PlatformData.BootMsg = bootmsgptr;
    SysBase->ThisTask->tc_SPLower = &_ss;
    SysBase->ThisTask->tc_SPUpper = &_ss_end;

    if (wasvalid) {
        SysBase->ColdCapture = ColdCapture;
        SysBase->CoolCapture = CoolCapture;
        SysBase->WarmCapture = WarmCapture;
        SysBase->ChkSum = 0;
        SysBase->ChkSum = GetSysBaseChkSum(SysBase) ^ 0xffff; 
        SysBase->KickMemPtr = KickMemPtr;
        SysBase->KickTagPtr = KickTagPtr;
        SysBase->KickCheckSum = KickCheckSum;
    }

    SysBase->SysStkUpper    = (APTR)&_ss_end;
    SysBase->SysStkLower    = (APTR)&_ss;

    /* Mark what the last alert was */
    for (i = 0; i < 4; i++)
        SysBase->LastAlert[i] = oldLastAlert[i];

    SysBase->AttnFlags = attnflags;

    /* Inject code for GetCC, depending on CPU model */
    if (SysBase->AttnFlags & AFF_68010) {
        /* move.w %ccr,%d0; rts; nop */
        FAKE_IT(SysBase, Exec, GetCC, 88, 0x42c0, 0x4e75, 0x4e71);
    } else {
        /* move.w %sr,%d0; rts; nop */
        FAKE_IT(SysBase, Exec, GetCC, 88, 0x40c0, 0x4e75, 0x4e71);
    }

#ifdef THESE_ARE_KNOWN_SAFE_ASM_ROUTINES
    PRESERVE_ALL(SysBase, Exec, Disable, 20);
    PRESERVE_ALL(SysBase, Exec, Enable, 21);
    PRESERVE_ALL(SysBase, Exec, Forbid, 22);
#endif
    PRESERVE_ALL(SysBase, Exec, Permit, 23);
    PRESERVE_ALL(SysBase, Exec, ObtainSemaphore, 94);
    PRESERVE_ALL(SysBase, Exec, ReleaseSemaphore, 95);
    PRESERVE_ALL(SysBase, Exec, ObtainSemaphoreShared, 113);

    /* Functions that need sign extension */
    EXT_BYTE(SysBase, Exec, SetTaskPri, 50);
    EXT_BYTE(SysBase, Exec, AllocSignal, 55);

    /* Only add the 2 standard ROM locations, since
     * we may get memory at 0x00f00000, or when we
     * are ReKicked, at the rom-in-ram locations.
     */
    krnCreateROMHeader("Kickstart ROM", (APTR)0x00f80000, (APTR)0x00ffffff);
    krnCreateROMHeader("Kickstart ROM", (APTR)0x00e00000, (APTR)0x00e7ffff);

    /* Add remaining memory regions */
    for (i = 2; membanks[i + 1]; i += 2) {
        IPTR  addr = membanks[i];
        ULONG size = membanks[i + 1];

        mh = addmemoryregion(addr, size, BootS, FALSE);
        Enqueue(&SysBase->MemList, &mh->mh_Node);

        /* Adjust MaxLocMem and MaxExtMem as needed */
        if (addr < 0x00200000)
            SysBase->MaxLocMem = (size + 0xffff) & 0xffff0000;
        else if (addr < 0x00d00000)
            SysBase->MaxExtMem = size ? (APTR)(((0xc00000 + (size + 0xffff)) & 0xffff0000)) : 0;
    }

    /* Now that we have a valid SysBase, we can call ColdCapture */
    if (wasvalid)
        doColdCapture();

    /* Seal up SysBase's critical variables */
    SetSysBaseChkSum();

    /* Set privilege violation trap - we
     * need this to support the Exec/Supervisor call
     */
    trap[8] = (SysBase->AttnFlags & AFF_68010) ? Exec_Supervisor_Trap : Exec_Supervisor_Trap_00;

    /* SysBase is complete, now we can enable instruction caches safely. */
    CacheControl(CACRF_EnableI, CACRF_EnableI);
    CacheClearU();

    oldmem = AvailMem(MEMF_FAST);

    /* Ok, let's start the system. We have to
     * do this in Supervisor context, since some
     * expansions ROMs (Cyperstorm PPC) expect it.
     */
    DEBUGPUTS(("[start] InitCode(RTF_SINGLETASK, 0)\n"));
    InitCode(RTF_SINGLETASK, 0);

    /* Autoconfig ram expansions are now configured */

    /* Our original AROS SysBase was in autoconfig RAM?
     * ArosBootStrap needs special handling because AOS autoconfigs all boards
     * before jumping to ColdCapture if SysBase is in autoconfig RAM
     *
     * Technically we don't need this but it saves KickTags and KickMems
     * if someone decides to use them. ArosBootStrap does not anymore need them,
     * it only needs ColdCapture.
     */
    if (BootS) {
        if (BootS->RealBase2) /* SysBase in autoconfig hack */
            BootS->RealBase = BootS->RealBase2;
        if (BootS->RealBase && iseven(BootS->RealBase)) {
            BootS->RealBase->ColdCapture = SysBase->ColdCapture;
            if (IsSysBaseValid(BootS->RealBase)) {
                oldSysBase = BootS->RealBase;
                DEBUGPUTHEX(("[SysBase] Found original at", (ULONG)oldSysBase));
            } else {
                DEBUGPUTHEX(("[SysBase] Found original invalid at", (ULONG)BootS->RealBase));
            }
            BootS->RealBase = NULL;
            BootS->RealBase2 = NULL;
        }
    }

    /* If oldSysBase is not NULL, that means that it
     * (a) wasn't valid before when we only had MEMF_LOCAL
     * ram and (b) could possibly be in the MEMF_KICK memory
     * we just got. Let's check it and find out if we
     * can use it's capture vectors.
     */
    if (oldSysBase && IsSysBaseValidNoVersion(oldSysBase)) {
        /* Save reset proof vectors */
        SysBase->ColdCapture  = oldSysBase->ColdCapture;
        SysBase->CoolCapture  = oldSysBase->CoolCapture;
        SysBase->WarmCapture  = oldSysBase->WarmCapture;
        /* Save KickData */
        SysBase->KickMemPtr   = oldSysBase->KickMemPtr; 
        SysBase->KickTagPtr   = oldSysBase->KickTagPtr;
        SysBase->KickCheckSum = oldSysBase->KickCheckSum;
        doColdCapture();
        /* Re-seal SysBase */
        SetSysBaseChkSum();
        wasvalid = TRUE;
    }

    /* Before we allocate anything else, we need to
     * lock down the mem entries in BootStruct.
     *
     * Diag module does final allocation attempt.
     */
    if (arosbootstrapmode) {
        if (SysBase->KickCheckSum == (APTR)SumKickData()) {
            InitKickMem(SysBase);
            CollectKickResidents(BootS, SysBase);
        } else {
            DEBUGPUTS(("PANIC! KickCheckSum mismatch!\n"));
            Early_Alert(AT_DeadEnd | AN_MemoryInsane);
        }
    }

    if ((AvailMem(MEMF_FAST) > (oldmem + 256 * 1024)) &&
        ((TypeOfMem(SysBase) & MEMF_CHIP) ||
         ((ULONG)SysBase >= 0x00a00000ul && (ULONG)SysBase < 0x01000000ul))) {
        /* Move execbase to real fast if available now */
        SysBase = PrepareExecBaseMove(SysBase);
        AbsExecBase = SysBase;
        DEBUGPUTHEX(("[Sysbase] now at", (ULONG)SysBase));
    }

    /* Initialize IRQ subsystem */
    AmigaIRQInit(SysBase);

    /* Set privilege violation trap again.
     * AmigaIRQInit may have blown it away.
     */
    trap[8] = (SysBase->AttnFlags & AFF_68010) ? Exec_Supervisor_Trap : Exec_Supervisor_Trap_00;

    /* Attempt to allocate a real stack, and switch to it. */
    do {
        const ULONG size = AROS_STACKSIZE;
        IPTR *usp;

        usp = AllocMem(size * sizeof(IPTR), MEMF_PUBLIC);
        if (usp == NULL) {
            DEBUGPUTS(("PANIC! Can't allocate a new stack for Exec!\n"));
            Early_Alert(CODE_ALLOC_FAIL);
        }

        /* Leave supervisor mode, switch power led on */
        asm volatile (
            "or.b	#2,0xbfe001\n"
            "move.l %0,%%usp\n"
            "move.w #0,%%sr\n"
            "move.l %2,%%sp@-\n"
            "pea 0f\n"
            "jmp %1@\n"
            "0:\n"
            :
            : "a" (&usp[size-3]),
              "a" (doInitCode),
                  "a" (BootS)
            :);
    } while (0);

    /* We shouldn't get here */
    DEBUGPUTS(("PANIC! doInitCode() returned!\n"));
    Early_Alert(CODE_EXEC_FAIL);
}
