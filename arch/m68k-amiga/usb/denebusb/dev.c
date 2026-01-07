/* dev.c - denebusb.device by Chris Hodges
*/

#include "debug.h"

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/utility.h>

#if defined(__SASC) || defined(__MORPHOS__)
#include <clib/alib_protos.h>
#endif

#include "denebusb.h"

#if !defined(__AROS__)
#undef	SysBase
#undef	UtilityBase
#undef	ExpansionBase
#else
#include LC_LIBDEFS_FILE
#endif

#ifndef ZORRO_II
#ifndef NODMA
#define DEVNAME             "denebdmausb.device"
#else
#define DEVNAME             "denebusb.device"
#endif
#else
#define DEVNAME             "denebz2usb.device"
#endif

#if !defined(__AROS__)
struct initstruct
{
    const ULONG libsize;
    const void  *functable;
    const void  *datatable;
    void  (*initfunc)(void);
};


/* MorphOS m68k->ppc gate functions
*/
DECLGATE(static const, uhwSoftInt, LIBNR)
DECLGATE(static const, uhwLevel6Int, LIBNR)

/* local protos
*/

#ifdef __MORPHOS__

struct DenebDevice *devInit(struct DenebDevice *base,
                         BPTR seglist,
                         struct ExecBase *SysBase);
#else /* __MORPHOS__ */

struct DenebDevice *NATDECLFUNC_3(devInit,
                               d0, struct DenebDevice *, base,
                               a0, BPTR, seglist,
                               a6, struct ExecBase *, SysBase);
#endif /* __MORPHOS__ */

struct DenebDevice *NATDECLFUNC_4(devOpen,
                               a1, struct IOUsbHWReq *, ioreq,
                               d0, LONG, unit,
                               d1, ULONG, flags,
                               a6, struct DenebDevice *, base);

BPTR NATDECLFUNC_2(devClose,
                   a1, struct IOUsbHWReq *, ioreq,
                   a6, struct DenebDevice *, base);

BPTR NATDECLFUNC_1(devExpunge,
                   a6, struct DenebDevice *, base);

BPTR i_devExpunge(struct DenebDevice *base);

ULONG devReserved(void);

void NATDECLFUNC_2(devBeginIO,
                   a1, struct IOUsbHWReq *, ioreq,
                   a6, struct DenebDevice *, base);

void NATDECLFUNC_2(devAbortIO,
                   a1, struct IOUsbHWReq *, ioreq,
                   a6, struct DenebDevice *, base);



/* This is a device, not executable
*/
__entry static
int fakemain(void)
{
    return -1;
}

void callfake(void)
{
    fakemain();
}


/* Important! Magic cookie for MorphOS
*/
#ifdef __MORPHOS__
const ULONG __amigappc__ = 1;
#endif /* __MORPHOS__ */


const char devname[];
const char devidstring[];
static
const struct initstruct deviceinitstruct;


__entry static
const struct Resident ROMTag =
{
    RTC_MATCHWORD,
    (struct Resident *) &ROMTag,
    (struct Resident *) (&ROMTag + 1),

#ifdef __MORPHOS__

    RTF_PPC | RTF_AUTOINIT | RTF_COLDSTART,

#else /* __MORPHOS__ */

    RTF_AUTOINIT | RTF_COLDSTART,

#endif /* __MORPHOS__ */

    VERSION,
    NT_DEVICE,
    48, /* Behind timer.device */
    (char *) devname,
    (char *) devidstring,
    (APTR) &deviceinitstruct
};


/* Static data
*/
#endif

const char devname[]     = DEVNAME;
#if !defined(__AROS__)
const char devidstring[] = VSTRING;
__entry const char devverstag[]  = VERSTAG;

static
const ULONG FuncTable[] =
{
#ifdef __MORPHOS__
    FUNCARRAY_32BIT_NATIVE,
#endif /* __MORPHOS__ */
    (ULONG) devOpen,
    (ULONG) devClose,
    (ULONG) devExpunge,
    (ULONG) devReserved,
    (ULONG) devBeginIO,
    (ULONG) devAbortIO,
  ~0
};



static
const struct initstruct deviceinitstruct =
{
    sizeof(struct DenebDevice),
    FuncTable,
    NULL,
    (void (*)(void)) devInit
};
#endif

/*
 *===========================================================
 * devInit(base, seglist, SysBase)
 *===========================================================
 *
 * This is the the DEV_INIT function.
 *
 */

#ifdef __MORPHOS__

struct DenebDevice *devInit(struct DenebDevice *base,
                         BPTR seglist,
                         struct ExecBase *SysBase)
{

#elif defined(__AROS__)
static int devInit(LIBBASETYPEPTR base)
{
#else

struct DenebDevice *NATDECLFUNC_3(devInit,
                               d0, struct DenebDevice *, base,
                               a0, BPTR, seglist,
                               a6, struct ExecBase *, SysBase)
{
    DECLARG_3(d0, struct DenebDevice *, base,
              a0, BPTR, seglist,
              a6, struct ExecBase *, SysBase)

#endif /* __MORPHOS__ */

    KPRINTF(20, ("devInit base: 0x%08lx seglist: 0x%08lx SysBase: 0x%08lx\n",
                 base, seglist, SysBase));

    base->hd_UtilityBase = (struct UtilityBase *) OpenLibrary("utility.library", 39);

#define	UtilityBase	base->hd_UtilityBase

      if(UtilityBase)
      {
        base->hd_ExpansionBase = OpenLibrary("expansion.library", 39);

#define	ExpansionBase	base->hd_ExpansionBase

        if(ExpansionBase)
        {
#if !defined(__AROS__)
          /*base->hd_MemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,
                                        PUDDLESIZE, THRESHSIZE);
          if (base->hd_MemPool)
          { */

            /* Initialize device node & library struct
            */
            base->hd_Library.lib_Node.ln_Type = NT_DEVICE;
            base->hd_Library.lib_Node.ln_Name = (char *) devname;
            base->hd_Library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
            base->hd_Library.lib_Version      = VERSION;
            base->hd_Library.lib_Revision     = REVISION;
            base->hd_Library.lib_IdString     = (char *) devidstring;

            /* Store sysbase & segment
            */
            base->hd_SysBase = SysBase;
            base->hd_SegList = seglist;
#endif
            KPRINTF(10, ("devInit: Ok\n"));
          /*} else {
            KPRINTF(10, ("devInit: CreatePool() failed!\n"));
            CloseLibrary((struct Library *) UtilityBase);
            base = NULL;
          } */
        } else {
          KPRINTF(10, ("devInit: OpenLibrary(\"expansion.library\", 39) failed!\n"));
          base = NULL;
        }
      } else {
        KPRINTF(10, ("devInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
        base = NULL;
      }

  KPRINTF(10, ("devInit: openCnt = %ld\n", base->hd_Library.lib_OpenCnt));

#if defined(__AROS__)
    return base ? TRUE : FALSE;
#else
  return base;
#endif
}

/*
 *===========================================================
 * devOpen(ioreq, unit, flags, base)
 *===========================================================
 *
 * This is the the DEV_OPEN function.
 *
 */
#if defined(__AROS__)
static int devOpen(LIBBASETYPEPTR base, struct IOUsbHWReq *ioreq, ULONG unit, ULONG flags)
{
#else
struct DenebDevice *NATDECLFUNC_4(devOpen,
                               a1, struct IOUsbHWReq *, ioreq,
                               d0, LONG, unit,
                               d1, ULONG, flags,
                               a6, struct DenebDevice *, base)
{
  DECLARG_4(a1, struct IOUsbHWReq *, ioreq,
            d0, LONG, unit,
            d1, ULONG, flags,
            a6, struct DenebDevice *, base)
#endif
  /*struct ExecBase *SysBase = base->dd_SysBase;
  */

  KPRINTF(20, ("devOpen ioreq: 0x%08lx unit: %ld flags: 0x%08lx base: 0x%08lx\n",
               ioreq, unit, flags, base));

  ++base->hd_Library.lib_OpenCnt;
  base->hd_Library.lib_Flags &= ~LIBF_DELEXP;

  KPRINTF(20, ("devOpen: openCnt = %ld\n", base->hd_Library.lib_OpenCnt));

  if(ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReqObsolete))
  {
    KPRINTF(20, ("devOpen: invalid MN_LENGTH!\n"));

    ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
  } else {

    /* Default to open failure.
    */
    ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;

    ioreq->iouh_Req.io_Unit = Open_Unit(ioreq, unit, base);
    if(!ioreq->iouh_Req.io_Unit)
    {
      KPRINTF(20, ("devOpen: could not open unit!\n"));
    } else {

      /* Opended ok!
      */
      ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
      ioreq->iouh_Req.io_Error                   = 0;

#if defined(__AROS__)
      return TRUE;
#else
      return base;
#endif
    }
  }

  ioreq->iouh_Req.io_Unit   = (APTR) -1;
  ioreq->iouh_Req.io_Device = (APTR) -1;
  base->hd_Library.lib_OpenCnt--;

#if defined(__AROS__)
  return FALSE;
#else
  return NULL;
#endif
}


/*
 *===========================================================
 * devClose(ioreq, base)
 *===========================================================
 *
 * This is the the DEV_EXPUNGE function.
 *
 */
#if defined(__AROS__)
static int devExpunge(LIBBASETYPEPTR base);

static int devClose(LIBBASETYPEPTR base, struct IOUsbHWReq *ioreq)
{
#else
BPTR NATDECLFUNC_2(devClose,
                   a1, struct IOUsbHWReq *, ioreq,
                   a6, struct DenebDevice *, base)
{
  DECLARG_2(a1, struct IOUsbHWReq *, ioreq,
            a6, struct DenebDevice *, base)
#endif
  BPTR ret;

  KPRINTF(20, ("devClose ioreq: 0x%08lx base: 0x%08lx\n", ioreq, base));

  ret = BNULL;

  Close_Unit(base, (struct DenebUnit *) ioreq->iouh_Req.io_Unit, ioreq);

  ioreq->iouh_Req.io_Unit   = (APTR) -1;
  ioreq->iouh_Req.io_Device = (APTR) -1;

  if (--base->hd_Library.lib_OpenCnt == 0)
  {

    /*KPRINTF(5, ("devClose: devclosedown\n"));
    devclosedown(base);*/

    if (base->hd_Library.lib_Flags & LIBF_DELEXP)
    {

      KPRINTF(5, ("devClose: calling expunge...\n"));

#if defined(__AROS__)
      ret = devExpunge(base);
#else
      ret = i_devExpunge(base);
#endif
    }
  }

  KPRINTF(5, ("devClose: lib_OpenCnt = %ld\n", base->hd_Library.lib_OpenCnt));

#if defined(__AROS__)
  return (ret) ? TRUE : FALSE;
#else
  return ret;
#endif
}

#if defined(__AROS__)
static int devExpunge(LIBBASETYPEPTR base)
{
#else
BPTR i_devExpunge(struct DenebDevice *base)
{
#endif
  BPTR ret;

  KPRINTF(20, ("devExpunge base: 0x%08lx\n", base));

  ret = BNULL;

  if(base->hd_Library.lib_OpenCnt == 0)
  {
    struct ExecBase *SysBase;

    KPRINTF(5, ("devExpunge: Unloading...\n"));

    SysBase = base->hd_SysBase;

    ret = base->hd_SegList;

    /*
    KPRINTF(5, ("devExpunge: devclosedown\n"));
    devclosedown(base);
    */

    KPRINTF(5, ("devExpunge: closelibrary utilitybase 0x%08lx\n",
                UtilityBase));
    CloseLibrary((struct Library *) UtilityBase);

    KPRINTF(5, ("devExpunge: closelibrary expansionbase 0x%08lx\n",
                ExpansionBase));
    CloseLibrary(ExpansionBase);

    KPRINTF(5, ("devExpunge: removing device node 0x%08lx\n",
                &base->hd_Library.lib_Node));
    Remove(&base->hd_Library.lib_Node);

    KPRINTF(5, ("devExpunge: FreeMem()...\n"));
    FreeMem((char *) base - base->hd_Library.lib_NegSize,
            (ULONG) (base->hd_Library.lib_NegSize + base->hd_Library.lib_PosSize));

    KPRINTF(5, ("devExpunge: Unloading done! denebusb.device expunged!\n\n"));

#if defined(__AROS__)
    return (ret) ? TRUE : FALSE;
#else
    return ret;
#endif
  }
  else
  {
    KPRINTF(5, ("devExpunge: Could not expunge, LIBF_DELEXP set!\n"));
    base->hd_Library.lib_Flags |= LIBF_DELEXP;
  }

#if defined(__AROS__)
  return FALSE;
#else
  return NULL;
#endif
}

#if !defined(__AROS__)
/*
 *===========================================================
 * devExpunge(base)
 *===========================================================
 *
 * This is the the LIB_EXPUNGE function.
 *
 */

BPTR NATDECLFUNC_1(devExpunge,
                   a6, struct DenebDevice *, base)
{
    DECLARG_1(a6, struct DenebDevice *, base)

    return(i_devExpunge(base));
}



/*
 *===========================================================
 * devReserved(void)
 *===========================================================
 *
 * This is the the reserved function. It must return 0.
 *
 */

ULONG devReserved(void)
{
  return 0;
}
#else
ADD2INITLIB(devInit, 0)
ADD2OPENDEV(devOpen, 0)
ADD2CLOSEDEV(devClose, 0)
ADD2EXPUNGELIB(devExpunge, 0)
#endif


/*
 *===========================================================
 * devBeginIO(ioreq, base)
 *===========================================================
 *
 * This is the DEV_BEGINIO vector of the device.
 *
 */
#if defined(__AROS__)
AROS_LH1(void, devBeginIO,
         AROS_LHA(struct IOUsbHWReq *, ioreq, A1),
	     LIBBASETYPEPTR, base, 5, deneb)
{
    AROS_LIBFUNC_INIT
#else
void NATDECLFUNC_2(devBeginIO,
                   a1, struct IOUsbHWReq *, ioreq,
                   a6, struct DenebDevice *, base)
{
    DECLARG_2(a1, struct IOUsbHWReq *, ioreq,
              a6, struct DenebDevice *, base)
#endif
    struct DenebUnit *unit = (struct DenebUnit *) ioreq->iouh_Req.io_Unit;
    WORD ret;

    KPRINTF(1, ("devBeginIO ioreq: 0x%08lx base: 0x%08lx cmd: %lu\n", ioreq, base, ioreq->iouh_Req.io_Command));


    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error                   = UHIOERR_NO_ERROR;

    if (ioreq->iouh_Req.io_Command < NSCMD_DEVICEQUERY)
    {
        switch (ioreq->iouh_Req.io_Command)
        {

            case CMD_RESET:
                ret = cmdReset(ioreq, unit, base);
                break;

            case CMD_FLUSH:
                ret = cmdFlush(ioreq, unit, base);
                break;

            case UHCMD_QUERYDEVICE:
                ret = cmdQueryDevice(ioreq, unit, base);
                break;

            case UHCMD_USBRESET:
                ret = cmdUsbReset(ioreq, unit, base);
                break;

            case UHCMD_USBRESUME:
                ret = cmdUsbResume(ioreq, unit, base);
                break;

            case UHCMD_USBSUSPEND:
                ret = cmdUsbSuspend(ioreq, unit, base);
                break;

            case UHCMD_USBOPER:
                ret = cmdUsbOper(ioreq, unit, base);
                break;

            case UHCMD_CONTROLXFER:
                ret = cmdControlXFer(ioreq, unit, base);
                break;

            case UHCMD_BULKXFER:
                ret = cmdBulkXFer(ioreq, unit, base);
                break;

            case UHCMD_INTXFER:
                ret = cmdIntXFer(ioreq, unit, base);
                break;

            case UHCMD_ISOXFER:
                ret = cmdIsoXFer(ioreq, unit, base);
                break;

            case UHCMD_ADDISOHANDLER:
                ret = cmdAddIsoHandler(ioreq, unit, base);
                break;

            case UHCMD_REMISOHANDLER:
                ret = cmdRemIsoHandler(ioreq, unit, base);
                break;

            case UHCMD_STARTRTISO:
                ret = cmdStartRTIso(ioreq, unit, base);
                break;

            case UHCMD_STOPRTISO:
                ret = cmdStopRTIso(ioreq, unit, base);
                break;

            default:
                ret = IOERR_NOCMD;
                break;
        }
    }
    else
    {
        switch(ioreq->iouh_Req.io_Command)
        {
            case NSCMD_DEVICEQUERY:
                ret = cmdNSDeviceQuery((struct IOStdReq *) ioreq, unit, base);
                break;

            default:
                ret = IOERR_NOCMD;
                break;
        }
    }

    if(ret != RC_DONTREPLY)
    {
        KPRINTF(1, ("TermIO\n"));
        if(ret != RC_OK)
        {
            /* Set error codes */
            ioreq->iouh_Req.io_Error = ret & 0xff;
        }
        /* Terminate the iorequest */
        TermIO(ioreq, base);
    }
#if defined(__AROS__)
    AROS_LIBFUNC_EXIT
#endif
}




/*
 *===========================================================
 * devAbortIO(ioreq, base)
 *===========================================================
 *
 * This is the DEV_ABORTIO vector of the device. It abort
 * the given iorequest, and set
 *
 */
#if defined(__AROS__)
AROS_LH1(LONG, devAbortIO,
         AROS_LHA(struct IOUsbHWReq *, ioreq, A1),
	     LIBBASETYPEPTR, base, 6, deneb)
{
    AROS_LIBFUNC_INIT
#else
void NATDECLFUNC_2(devAbortIO,
                   a1, struct IOUsbHWReq *, ioreq,
                   a6, struct DenebDevice *, base)
{
    DECLARG_2(a1, struct IOUsbHWReq *, ioreq,
              a6, struct DenebDevice *, base)

    struct ExecBase *SysBase;
#endif
    KPRINTF(1, ("devAbortIO ioreq: 0x%08lx\n", ioreq));
#if !defined(__AROS__)
    SysBase = base->hd_SysBase;
#endif
    /* Is it pending?
    */
    if(ioreq->iouh_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE)
    {
        if(cmdAbortIO(ioreq, base))
        {
            ioreq->iouh_Req.io_Error = IOERR_ABORTED;
            TermIO(ioreq, base);
#if defined(__AROS__)
            return(0);
#endif
        }
    }
#if defined(__AROS__)
    return(-1);
    AROS_LIBFUNC_EXIT
#endif
}

