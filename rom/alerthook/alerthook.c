/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Alert.Hook for AROS
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/config.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <aros/system.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

struct Errors
{
    ULONG   number;
    STRPTR  string;
};

static const UBYTE Alerthook_name[];
static const UBYTE Alerthook_version[];
static AROS_UFP3(ULONG, Alerthook_init,
    AROS_UFPA(void *,		    dummy1,  D0),
    AROS_UFPA(BPTR,		    dummy2,  A0),
    AROS_UFPA(struct ExecBase *,    SysBase, A6)
);

STRPTR getGuruString(ULONG, STRPTR);

extern void Alerthook_end(void);

const struct Resident Alerthook_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Alerthook_resident,
    (APTR)&Alerthook_end,
    RTF_COLDSTART,
    41,
    NT_UNKNOWN,
    -45,
    (UBYTE *)Alerthook_name,
    (UBYTE *)&Alerthook_version[6],
    (APTR)&Alerthook_init,
};

static const UBYTE Alerthook_name[] = "alert.hook\r\n";
static const UBYTE Alerthook_version[] = "$VER: alert.hook 41.8 " ADATE;
static UBYTE *const nomem = "\x38\x0f" "Not Enough Memory! ";
static UBYTE *const sfail = "\x38\x0f" "Software Failure! ";
static UBYTE *const recov = "\x38\x0f" "Recoverable Alert! ";
static UBYTE *const mouse = "\x01\x50\x0f" "Press mouse button to continue.";
static UBYTE *const fmtstring = "\xa8\x2a" "Task:   %08lx - ";
static UBYTE *const errstring = "\x1e" "Error  %04x %04x - ";
static UBYTE *const tasknotfound = "--task not found--";

/* This is the callback for RawDoFmt() */
AROS_UFH2(void, putChProc,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(STRPTR *, buf, A3))
{
    AROS_LIBFUNC_INIT

    **buf = chr;
    *buf += 1;

    AROS_LIBFUNC_EXIT
}

/* This function copies a string, but also returns a pointer to the
   '\0' at the end of the string. This way I can concat the strings
   which have '\0' in them for other reasons.

   Note len == -1 is equivalent to strcpy(dest,src).
*/
static STRPTR
mystrcpy(STRPTR dest, STRPTR src, LONG len)
{
    while(len && *src)
    {
        *dest++ = *src++;
        len--;
    }
    *dest++ = 0;
    return dest;
}

static AROS_UFH3(ULONG, Alerthook_init,
    AROS_UFHA(void *,		    dummy1,  D0),
    AROS_UFHA(BPTR,		    dummy2,  A0),
    AROS_UFHA(struct ExecBase *,    SysBase, A6)
)
{
    AROS_LIBFUNC_INIT

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    /*
	Clear memory location zero (a cookie for the delayed guru after reset).
	Some machines will keep putting a guru up during every reset if I don't
	do this here.
    */
    ULONG *location_zero = (void *)0;

    *location_zero = 0;
#endif

    D(bug("AlertHook: *******************\n"));
    if(SysBase->LastAlert[0] != -1)
    {
        struct IntuitionBase *IntuitionBase;
        struct Task *task;
        UBYTE buffer[256], *buf, *tname;

        D(bug("alert.hook: LastAlert[0] = 0x%lx\n", SysBase->LastAlert[0]));

        buffer[0] = 0;
        buf = &buffer[1];

        if(SysBase->LastAlert[0] & AG_NoMemory)
            buf = mystrcpy(buf, nomem, -1);
        else if(SysBase->LastAlert[0] & AT_DeadEnd)
            buf = mystrcpy(buf, sfail, -1);
        else
            buf = mystrcpy(buf, recov, -1);

        *buf++ = 1;

        buf = mystrcpy(buf, mouse, -1);
        *buf++ = 1; *buf++ = 0;

        /* Find out the task name. The node type must be correct. */
        task = (struct Task *)SysBase->LastAlert[1];
        if( (
                  (task->tc_Node.ln_Type == NT_TASK)
               || (task->tc_Node.ln_Type == NT_PROCESS)
            )
            && (task->tc_Node.ln_Name != NULL)
          )
            tname = task->tc_Node.ln_Name;
        else
            tname = tasknotfound;

        RawDoFmt(fmtstring, &SysBase->LastAlert[1], (void *)putChProc, &buf);
        buf = mystrcpy(buf - 1, tname, 30);
        *buf++ = 1; *buf++ = 0;

        /* Use this variable to hold the current address */
        tname = buf++;
        RawDoFmt(errstring, &SysBase->LastAlert[0], (void *)putChProc, &buf);

        buf = getGuruString(SysBase->LastAlert[0], &buf[-1]);
        *buf++ = 0;

        /* This rather strange contraption will centre the string. */
        *((UBYTE *)tname) = (82 - (buf - tname)) << 2;

        IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",0);

        if(IntuitionBase)
        {
            if(IntuitionBase->LibNode.lib_Version >= 39)
                TimedDisplayAlert(SysBase->LastAlert[0] & AT_DeadEnd, buffer, 0x38, 500);
            else
                DisplayAlert(SysBase->LastAlert[0] & AT_DeadEnd, buffer, 0x38);

            CloseLibrary((struct Library *)IntuitionBase);
        } else {
            D(bug("alert.hook: No intuition, dumping message\n"));
            D(bug("alert.hook: %s\n", buffer));
        }

        SysBase->LastAlert[0] = -1;
    }
#if 0
    SysBase->LastAlert[0] = old;
    SysBase->LastAlert[1] = NULL;
#endif
    return FALSE;

    AROS_LIBFUNC_EXIT
}

/* Get a string from an array of type Errors. */
STRPTR getString(STRPTR buffer, ULONG alertnum, const struct Errors *errs)
{
    while((errs->number) && (errs->number != alertnum))
    {
        errs++;
    }
    return mystrcpy(buffer, errs->string, -1);
}

static const struct Errors cpustrings[] =
{
    { ACPU_AbsExecBase, "Write attempted to AbsExecBase" },
    { ACPU_BusErr,      "Hardware bus fault/address error" },
    { ACPU_AddressErr,  "Illegal address access (odd)" },
    { ACPU_InstErr,     "Illegal instruction" },
    { ACPU_DivZero,     "Division by zero" },
    { ACPU_CHK,         "CHK instruction error" },
    { ACPU_TRAPV,       "TRAPV instruction error" },
    { ACPU_PrivErr,     "Priviledge violation error" },
    { ACPU_Trace,       "Trace error" },
    { ACPU_LineA,       "Line 1010 (A) E mulator error" },
    { ACPU_LineF,       "Line 1111 (F) Emulator/Coprocessor error" },
    { ACPU_Format,      "Stack frame format error" },
    { ACPU_Spurious,    "Spurious interrupt error" },
    { 0, "Unknown CPU error" }
};

static const struct Errors subsystems[] =
{
    { 0x01,     "exec.library " },
    { 0x02,     "graphics.library " },
    { 0x03,     "layers.library " },
    { 0x04,     "intuition.library " },
    { 0x05,     "math.library " },
    { 0x07,     "dos.library " },
    { 0x08,     "ramlib " },
    { 0x09,     "icon.library " },
    { 0x0a,     "expansion.library " },
    { 0x0b,     "diskfont.library " },
    { 0x10,     "audio.device " },
    { 0x11,     "console.device " },
    { 0x12,     "gameport.device " },
    { 0x13,     "keyboard.device " },
    { 0x14,     "trackdisk.device " },
    { 0x15,     "timer.device " },
    { 0x20,     "cia.resource " },
    { 0x21,     "disk.resource " },
    { 0x22,     "misc.resource " },
    { 0x30,     "bootstrap " },
    { 0x31,     "workbench " },
    { 0x32,     "diskcopy " },
    { 0x33,     "gadtools " },
    { 0x34,     "utility " },

    { 0x40,	"aros " },
    { 0x41,	"oop " },
    { 0x42,	"hidd " },

    /* This takes in 0x35 as well... */
    { 0x00,     "unknown " }
};

static const struct Errors types[] =
{
    { 0x01,     "no memory for " },
    { 0x02,     "could not make library " },
    { 0x03,     "could not open library " },
    { 0x04,     "could not open device " },
    { 0x05,     "could not open resource " },
    { 0x06,     "IO error with " },
    { 0x07,     "no signal for/from " },
    { 0x08,     "bad parameter for/from " },
    { 0x09,     "close library error with " },
    { 0x0a,     "close device error with " },
    { 0x0b,     "process creating failure with " },
    { 0x00,     "unknown problem with "}
};

static const struct Errors execstrings[] =
{
    { AN_ExcptVect,     "MC68k Exception vector checksum" },
    { AN_BaseChkSum,    "ExecBase checksum" },
    { AN_LibChkSum,     "Library checksum failure" },
    { AN_MemCorrupt,    "Corrupt memory list detected" },
    { AN_IntrMem,       "No memory for interrupt servers" },
    { AN_InitAPtr,      "(obs) InitStruct of an APTR" },
    { AN_SemCorrupt,    "Semaphore in an illegal state" },
    { AN_FreeTwice,     "Memory freed twice" },
    { AN_BogusExcpt,    "Illegal mc68k exception taken" },
    { AN_IOUsedTwice,   "Attempt to reuse active IORequest" },
    { AN_MemoryInsane,  "Sanity check on memory list failed" },
    { AN_IOAfterClose,  "Attempt to use IORequest after close" },
    { AN_StackProbe,    "Stack extends out of range" },
    { AN_BadFreeAddr,   "Memory header not located" },
    { AN_BadSemaphore,  "Attempt to use the old message semaphore" },
    { 0, "unknown exec.library error" }
};

static const struct Errors gfxstrings[] =
{
    { AN_GfxNoMem,      "Graphics out of memory" },
    { AN_GfxNoMemMspc,  "No memory to allocate MonitorSpec" },
    { AN_LongFrame,     "No memory for long frame" },
    { AN_ShortFrame,    "No memory for short frame" },
    { AN_TextTmpRas,    "Mo memory for TmpRas" },
    { AN_BltBitMap,     "No memory for BltBitMap" },
    { AN_RegionMemory,  "No memory for Region" },
    { AN_MakeVPort,     "No memory for MakeVPort" },
    { AN_GfxNewError,   "Error in GfxNew()" },
    { AN_GfxFreeError,  "Error in GfxFree()" },
    { AN_GfxNoLCM,      "Emergency memory not available" },
    { AN_ObsoleteFont,  "Unsupported font description used" },
    { 0, "unknown graphics.library error" }
};

static const struct Errors unknownstrings[] =
{
    { 0, "unknown error" }
};

static const struct Errors layersstrings[] =
{
    { AN_LayersNoMem,   "layers: no memory" },
    { 0, "unknown layers.library error" }
};

static const struct Errors intuistrings[] =
{
    { AN_GadgetType,    "intuition: unknown gadget type" },
    { AN_CreatePort,    "intuition couldn't create port, no memory" },
    { AN_ItemAlloc,     "no memory for menu item" },
    { AN_SubAlloc,      "no memory for menu subitem" },
    { AN_PlaneAlloc,    "no memory for bitplane" },
    { AN_ItemBoxTop,    "top of item box < RelZero" },
    { AN_OpenScreen,    "no memory for OpenScreen()" },
    { AN_OpenScrnRast,  "no memory for OpenScreen() raster" },
    { AN_SysScrnType,   "unknown type of system screen" },
    { AN_AddSWGadget,   "add SW gadgets, no memory" },
    { AN_OpenWindow,    "no memory for OpenWindow()" },
    { AN_BadState,      "bad state return entering intuition" },
    { AN_BadMessage,    "bad message received by IDCMP" },
    { AN_WeirdEcho,     "weird echo causing incomprehension" },
    { AN_NoConsole,     "couldn't open the console.device" },
    { AN_NoISem,        "intuition skipped obtaining a semaphore" },
    { AN_ISemOrder,     "intuition got a semaphore in wrong order" },
    { 0, "unknown intuition.library error" }
};

static const struct Errors mathstrings[] =
{
    { 0, "unknown math library error" }
};

static const struct Errors dosstrings[] =
{
    { AN_StartMem,      "no memory at startup" },
    { AN_EndTask,       "EndTask did not end task" },
    { AN_QPktFail,      "QPkt failure" },
    { AN_AsyncPkt,      "unexpected DOS packet received" },
    { AN_FreeVec,       "freevec failed" },
    { AN_DiskBlkSeq,    "disk block sequence error" },
    { AN_BitMap,        "disk bitmap corrupt" },
    { AN_KeyFree,       "disk key already free" },
    { AN_BadChkSum,     "disk checksum bad" },
    { AN_DiskError,     "disk error" },
    { AN_KeyRange,      "disk key out of range" },
    { AN_BadOverlay,    "bad overlay" },
    { AN_BadInitFunc,   "invalid initialization packet for cli/shell" },
    { AN_FileReclosed,  "filehandle closed more than once" },
    { 0, "unknown dos.library error" }
};

static const struct Errors ramlibstrings[] =
{
    { AN_BadSegList,    "bad library seglist" },
    { 0, "unknown ramlib/lddemon error" }
};

static const struct Errors iconstrings[] =
{
    { 0, "unknown icon.library error" }
};

static const struct Errors expanstrings[] =
{
    { AN_BadExpansionFree,  "expansion freeing region already freed"},
    { 0, "unknown expansion.library error" }
};

static const struct Errors utilitystrings[] =
{
    {0, "unknown utility.library error" }
};

static const struct Errors keymapstrings[] =
{
    {0, "unknown keymap error" }
};

static const struct Errors dfontstrings[] =
{
    { 0, "unknown diskfont.library error" }
};

static const struct Errors audiostrings[] =
{
    { 0, "unknown audio.device error" }
};

static const struct Errors consolestrings[] =
{
    { AN_NoWindow, "can't open initial console window" },
    { 0, "unknown console.device error" }
};

static const struct Errors gameportstrings[] =
{
    { 0, "unknown gameport.device error" }
};

static const struct Errors keyboardstrings[] =
{
    { 0, "unknown keyboard.device error" }
};

static const struct Errors trackdiskstrings[] =
{
    { AN_TDCalibSeek,   "trackdisk calibrate seek error" },
    { 0, "unknown trackdisk.device error" }
};

static const struct Errors timerstrings[] =
{
    { AN_TMBadReq,      "bad timer request" },
    { AN_TMBadSupply,   "bad timer powersupply frequency" },
    { 0, "unknown timer.device error" }
};

static const struct Errors ciastrings[] =
{
    { 0, "unknown cia resource error" }
};

static const struct Errors diskstrings[] =
{
    { AN_DRHasDisk, "get disk unit, already has disk" },
    { AN_DRIntNoAct,"disk interrupt, no active unit" },
    { 0, "unknown disk.resource error" }
};

static const struct Errors miscstrings[] =
{
    { 0, "unknown misc.resource error" }
};

static const struct Errors bootstrings[] =
{
    { AN_BootError, "boot code returned an error" },
    { 0, "unknown bootstrap error" }
};

static const struct Errors workbenchstrings[] =
{
    { AN_NoFonts,           "no fonts for workbench" },
    { AN_WBBadStartupMsg1,  "bad startup message 1 for workbench" },
    { AN_WBBadStartupMsg2,  "bad startup message 2 for workbench" },
    { AN_WBBadIOMsg,        "bad IO message for workbench" },
    { AN_WBReLayoutToolMenu, "error with layout on tools menu" },
    { 0, "unknown workbench error" }
};

static const struct Errors diskcopystrings[] =
{
    {0, "unknown diskcopy error" }
};

static const struct Errors gadtoolsstrings[] =
{
    {0, "unknown gadtools.library error" }
};

static const struct Errors arosstrings[] =
{
    {0, "unknown aros.library error" }
};

static const struct Errors oopstrings[] =
{
    {0, "unknown oop.library error" }
};

static const struct Errors hiddstrings[] =
{
    {0, "unknown Hidd system error" }
};

static const struct Errors *const stringlist[] =
{
    /* 0x00 */
    unknownstrings,
    execstrings,
    gfxstrings,
    layersstrings,
    intuistrings,
    mathstrings,
    unknownstrings,
    dosstrings,
    ramlibstrings,
    iconstrings,
    expanstrings,
    dfontstrings,
    utilitystrings,
    keymapstrings,
    unknownstrings,
    unknownstrings,

    /* 0x10 */
    audiostrings,
    consolestrings,
    gameportstrings,
    keyboardstrings,
    trackdiskstrings,
    timerstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,

    /* 0x20 */
    ciastrings,
    diskstrings,
    miscstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,

    /* 0x30 */
    bootstrings,
    workbenchstrings,
    diskcopystrings,
    gadtoolsstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,
    unknownstrings,

    /* 0x40 */
    arosstrings,
    oopstrings,
    hiddstrings
};

/* Decode the alert number, and try and work out what string to get */
STRPTR getGuruString(ULONG alertnum, STRPTR buf)
{
    /* Is this a CPU alert? */
    if((alertnum & 0x7f008000) == 0)
    {
        /* Yes */
        buf = getString(buf, alertnum, cpustrings);
    }
    /* Is this a General alert */
    else if((alertnum & 0x8000) == 0x8000)
    {
        UBYTE type = (alertnum & 0x00FF0000) >> 16;
        UWORD obj  = (alertnum & 0x7fff);
        UBYTE subsys = (alertnum & 0x7f000000) >> 24;

        buf = getString(buf, obj, subsystems);
        buf = getString(&buf[-1], type, types);
        buf = getString(&buf[-1], subsys, subsystems);
    }
    /* This must be a specific alert */
    else
    {
        UBYTE subsys = (alertnum & 0x7f000000) >> 24;

        if(subsys < 0x80)
            buf = getString(buf, alertnum, stringlist[subsys]);
        else
            buf = mystrcpy(buf, "unknown error", -1);
    }

    return buf;
}
