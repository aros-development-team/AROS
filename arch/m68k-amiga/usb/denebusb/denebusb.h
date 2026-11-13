#ifndef DENEBUSB_H
#define DENEBUSB_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for denebusb.device
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <hodges@in.tum.de>
 *
 * History
 *
 *  26-02-2003  - Initial
 *
 */

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/initializers.h>
#include <hardware/intbits.h>
#include <libraries/configvars.h>

#include <devices/timer.h>
#include <utility/utility.h>
#include <dos/dos.h>
#include <intuition/intuition.h>

#include <devices/usbhardware.h>
#if !defined(__AROS__)
#include <devices/nsd.h>
#else
#include <devices/newstyle.h>
#endif

/* Reply the iorequest with success
*/
#define RC_OK         0

/* Magic cookie, don't set error fields & don't reply the ioreq
*/
#define RC_DONTREPLY  -1

#if defined(__GNUC__) && !defined(__aligned)
#define __aligned __attribute__((__aligned__(4)))
#endif
#define inline __inline

#define LE2BE_L(x) (((x)<<24)|(((x) & 0xff00)<<8)|(((x)>>8) & 0xff00)|(((ULONG) x)>>24))
#define CONSTLE2BE_L(x) (((x)<<24)|(((x) & 0xff00)<<8)|(((x)>>8) & 0xff00)|(((ULONG) x)>>24))

#define ORREG(x,y)    unit->hu_RegBase[x] |= y
#define ANDREG(x,y)   unit->hu_RegBase[x] &= y
#define WRITEREG(x,y) unit->hu_RegBase[x] = y
#define READREG(x)    unit->hu_RegBase[x]

#define WRITEMEM(x,y) WRITEREG(x,y)

#ifndef ZORRO_II
#define WRITEMACH(x,y) unit->hu_HWBase[x] = y
#define READMACH(x)    unit->hu_HWBase[x]
#define MC030FREEZE    //if(unit->hu_FastZorro2) unit->hu_OldCacheBits = CacheControl(CACRF_FreezeD, CACRF_FreezeD)
#define MC030UNFREEZE  //if(unit->hu_FastZorro2) CacheControl(unit->hu_OldCacheBits, CACRF_FreezeD)
#define MC030FLUSH     //if(unit->hu_FastZorro2) CacheClearE(0, 0xffffffff, CACRF_ClearD)
#else
#define WRITEMACH(x,y) ((UWORD *) unit->hu_HWBase)[x] = y
#define READMACH(x)    ((UWORD *) unit->hu_HWBase)[x]
#define MC030FREEZE
#define MC030UNFREEZE
#define MC030FLUSH
#endif

#ifdef __SASC
#define __entry
#endif


/* PTD Transaction node */
#define PTDT_FREE       0x00
#define PTDT_SETUP      0x01
#define PTDT_SDATAIN    0x83
#define PTDT_SDATAOUT   0x05
#define PTDT_STERMIN    0x87
#define PTDT_STERMOUT   0x09
#define PTDT_DATAIN     0x9b
#define PTDT_DATAINDMA  0x9c
#define PTDT_DATAOUT    0x0d
#define PTDT_DATAOUTDMA 0x0e

/* DMA Statemachine */

/*
   State_NOP: DMA disabled
     ? OUT -> Start DMA_LOW  -> Offset == Length -> State_OUT_START_ONE
                                Offset <  Length -> State_OUT_START_MULTIPLE
     ? IN -> (Fire LOW PTD) -> State_IN_WAITING_LOW

   State_OUT_START_ONE: (LOW DMA and activation pending)
     - DMA int  -> State_OUT_DONE_ONE
     x PTD done
     * Activate? Later -> State_OUT_PTD_DONE_HIGH

   State_OUT_START_MULTIPLE: (LOW DMA and activation pending)
     - DMA int  -> Start DMA_HIGH -> State_OUT_PREFETCH_1BUF_DONE
     x PTD done
     * Activate? Later -> State_OUT_PTD_DONE_HIGH

   State_OUT_DONE_ONE: (activation pending, low buffer filled)
     x DMA int
     x PTD done
     * Activate? Now   -> State_NOP

   State_OUT_PREFETCH_1BUF_DONE: (HIGH DMA and activation pending, low buffer filled)
     - DMA int  -> State_OUT_PREFETCH_2BUF_DONE
     x PTD done
     * Activate? Now   -> State_OUT_PREFETCH_HIGH

   State_OUT_PREFETCH_2BUF_DONE: (activation pending, both buffers filled)
     x DMA int
     x PTD done
     * Activate? Now   -> State_OUT_PREFETCH_HIGH_DONE

   -- end of activation stages

   State_OUT_PREFETCH_HIGH: (DMA pending, LOW PTD active)
     - DMA int  -> State_OUT_PREFETCH_HIGH_DONE
     - PTD done -> State_OUT_PTD_DONE_LOW

   State_OUT_PREFETCH_HIGH_DONE: (no DMA active, LOW PTD pending)
     x DMA int
     - PTD done -> FirePTD_HIGH -> Offset == Len   -> State_NOP
                                -> DMASize > Thres -> Start DMA_LOW  -> State_OUT_PREFETCH_LOW
                                -> DMASize < Thres -> CopyQuick LOW  -> State_OUT_PREFETCH_LOW_DONE

   State_OUT_PTD_DONE_LOW: (HIGH DMA pending, no PTD active)
     - DMA int  -> FirePTD_HIGH -> Offset == Len   -> State_NOP
                                -> DMASize > Thres -> Start DMA_LOW  -> State_OUT_PREFETCH_LOW
                                -> DMASize < Thres -> CopyQuick LOW  -> State_OUT_PREFETCH_LOW_DONE
     x PTD done

   State_OUT_PREFETCH_LOW: (LOW DMA pending, HIGH PTD active)
     - DMA int  -> State_OUT_PREFETCH_LOW_DONE
     - PTD done -> State_OUT_PTD_DONE_HIGH

   State_OUT_PREFETCH_LOW_DONE: (no DMA pending, HIGH PTD active)
     x DMA int
     - PTD done -> FirePTD_LOW  -> Offset == Len   -> State_NOP
                                -> DMASize > Thres -> Start DMA_HIGH  -> State_OUT_PREFETCH_HIGH
                                -> DMASize < Thres -> CopyQuick HIGH  -> State_OUT_PREFETCH_HIGH_DONE

   State_OUT_PTD_DONE_HIGH: (LOW DMA pending, no PTD active)
     - DMA int  -> FirePTD_LOW  -> Offset == Len   -> State_NOP
                                -> DMASize > Thres -> Start DMA_HIGH  -> State_OUT_PREFETCH_HIGH
                                -> DMASize < Thres -> CopyQuick HIGH  -> State_OUT_PREFETCH_HIGH_DONE
     x PTD done



   State_IN_WAITING_LOW: (no DMA pending, LOW PTD active)
     x DMA int
     - PTD done -> DMASize > Thres -> Start DMA_LOW  -> Has Next -> FirePTD_HIGH -> State_IN_READING_LOW
                                                     -> Last                     -> State_IN_READING_LOW
                -> DMASize < Thres -> CopyQuick LOW  -> TermPTD  -> State_NOP

   State_IN_READING_LOW: (LOW DMA pending, (HIGH PTD active))
     - DMA int  -> Has Next -> State_IN_WAITING_HIGH
                -> Last     -> TermPTD
     - PTD done -> State_IN_DONE_HIGH

   State_IN_DONE_HIGH: (LOW DMA pending, no PTDs pending)
     - DMA int  -> DMASize > Thres -> Start DMA_HIGH -> Has Next -> FirePTD_LOW  -> State_IN_READING_HIGH
                                                     -> Last                     -> State_IN_READING_HIGH
                -> DMASize < Thres -> CopyQuick HIGH -> TermPTD  -> State_NOP
     x PTD done

   State_IN_WAITING_HIGH: (no DMA pending, HIGH PTD active)
     x DMA int
     - PTD done -> DMASize > Thres -> Start DMA_HIGH -> Has Next -> FirePTD_LOW  -> State_IN_READING_HIGH
                                                     -> Last                     -> State_IN_READING_HIGH
                -> DMASize < Thres -> CopyQuick HIGH -> TermPTD  -> State_NOP

   State_IN_READING_HIGH: (HIGH DMA pending, (LOW PTD active))
     - DMA int  -> Has Next -> State_IN_WAITING_LOW
                -> Last     -> TermPTD
     - PTD done -> State_IN_DONE_LOW

   State_IN_DONE_LOW: (HIGH DMA pending, no PTDs pending)
     - DMA int  -> DMASize > Thres -> Start DMA_LOW  -> Has Next -> FirePTD_HIGH -> State_IN_READING_LOW
                                                     -> Last                     -> State_IN_READING_LOW
                -> DMASize < Thres -> CopyQuick HIGH -> TermPTD  -> State_NOP
     x PTD done

*/

#define DMASM_NOP                    0x00
#define DMASM_OUT_START_ONE          0x01
#define DMASM_OUT_START_MULTIPLE     0x02
#define DMASM_OUT_DONE_ONE           0x03
#define DMASM_OUT_PREFETCH_1BUF_DONE 0x04
#define DMASM_OUT_PREFETCH_2BUF_DONE 0x05
#define DMASM_OUT_PREFETCH_HIGH      0x10
#define DMASM_OUT_PREFETCH_HIGH_DONE 0x11
#define DMASM_OUT_PTD_DONE_LOW       0x12
#define DMASM_OUT_PREFETCH_LOW       0x13
#define DMASM_OUT_PREFETCH_LOW_DONE  0x14
#define DMASM_OUT_PTD_DONE_HIGH      0x15
#define DMASM_IN_WAITING_LOW         0x20
#define DMASM_IN_READING_LOW         0x21
#define DMASM_IN_DONE_HIGH           0x22
#define DMASM_IN_WAITING_HIGH        0x23
#define DMASM_IN_READING_HIGH        0x24
#define DMASM_IN_DONE_LOW            0x25

struct PTDNode
{
    struct MinNode     ptd_Node;             // 00
    UWORD              ptd_Num;              // 08
    UWORD              ptd_Type;             // 0a
    struct IOUsbHWReq *ptd_IOReq;            // 0c
    UWORD              ptd_BufStart;         // 10
    UWORD              ptd_BufLen;           // 12
    ULONG              ptd_AllocMap;         // 14
    ULONG              ptd_NakTimeoutFrame;  // 18
    UWORD              ptd_IntSOFMap;        // 1c
    BOOL               ptd_LastIn;           // 1e
    ULONG              ptd_DW[6];            // 20
    struct RTIsoNode  *ptd_RTIsoNode;        // 38
    ULONG              ptd_Dummy;            // 3c
};

struct RTIsoNode
{
    struct MinNode     rtn_Node;
    struct IOUsbHWRTIso *rtn_RTIso;
    ULONG              rtn_NextPTD;
    struct PTDNode    *rtn_PTDs[2];
    struct IOUsbHWBufferReq rtn_BufferReq;
    struct IOUsbHWReq  rtn_IOReq;
    UWORD              rtn_Dummy;
};


/* The unit node - private */
struct DenebUnit
{
    struct Unit           hu_Unit;
    BOOL                  hu_FastZorro2;
    LONG                  hu_UnitNo;
    struct ConfigDev     *hu_ConfigDev;

    struct DenebDevice   *hu_Device;
    struct MsgPort       *hu_MsgPort;
    struct timerequest   *hu_TimerReq;       /* Timer I/O Request */
    struct timerequest    hu_NakTimeoutReq;
    struct MsgPort        hu_NakTimeoutMsgPort;

    struct MinList        hu_RHIOQueue;
    struct MinList        hu_IntXFerQueue;
    struct MinList        hu_CtrlXFerQueue;
    struct MinList        hu_IsoXFerQueue;
    struct MinList        hu_BulkXFerQueue;
    struct MinList        hu_RTIsoHandlers;
    struct MinList        hu_FreeRTIsoNodes;

    struct Interrupt      hu_Level6Int;
    struct Interrupt      hu_SoftInt;
    struct Interrupt      hu_NakTimeoutInt;

    struct Task           hu_DMATask;

    ULONG                 hu_LargeAreaFreeMap; /* 25 bit word of free 1 KB pages */

    volatile ULONG        hu_ATLDone;
    volatile ULONG        hu_ATLActive;
    volatile ULONG        hu_ATLFree;
    volatile ULONG        hu_ATLBusy;
    ULONG                 hu_ATLNakMap;

    volatile ULONG        hu_IntDone;
    volatile ULONG        hu_IntFree;
    volatile ULONG        hu_IntBusy;
    ULONG                 hu_IntNakMap;

    volatile ULONG        hu_IsoFree;
    volatile ULONG        hu_IsoBusy;
    volatile ULONG        hu_IsoDone;
    ULONG                 hu_IsoRTMask;
    ULONG                 hu_IsoRTDone;

    ULONG                 hu_IntMask;

    UWORD                 hu_LastATL;
    UWORD                 hu_LastInt;
    UWORD                 hu_LastIso;
    BOOL                  hu_SafeATLCheck;

    ULONG                 hu_FrameCounter;

    UWORD                 hu_DMAStateMachine;
    BOOL                  hu_Buster9;
    ULONG                 hu_OldCacheBits;
    struct PTDNode       *hu_DMAPTD;
    UBYTE                *hu_DMAAddr;
    UBYTE                *hu_DMAPhyAddr;
    ULONG                 hu_DMALength;
    ULONG                 hu_DMAOffset;
    ULONG                 hu_ISPAddr;

    volatile ULONG       *hu_HWBase;
    volatile ULONG       *hu_RegBase;
    volatile ULONG       *hu_ReadBaseMem;
    volatile ULONG       *hu_ReadBasePTDs;
    struct PTDNode        hu_ATLPTDs[32];    /* 32 ATL PTD Nodes */
    struct PTDNode        hu_IntPTDs[32];    /* 32 Int PTD Nodes */
    struct PTDNode        hu_IsoPTDs[32];    /* 32 Iso PTD Nodes */
    //ULONG                 hu_DevLastFrm[128*16*2]; /* Contains the frame number of last batch sent */
    struct IOUsbHWReq    *hu_DevBusyReq[128*16*2]; /* pointer to io assigned to the Endpoint */
    UBYTE                 hu_DevDataToggle[128*16*2]; /* Data toggle bit for endpoints */
    struct RTIsoNode      hu_RTIsoNodes[16];
    UBYTE                 hu_DMATaskStack[512];
};

#define MAXUNITS 32

/* The device node - private
*/
struct DenebDevice
{
    struct Library      hd_Library;       /* standard */
    UWORD               hd_Flags;         /* various flags */

    struct ExecBase    *hd_SysBase;       /* cached execbase */
    struct UtilityBase *hd_UtilityBase;   /* for tags etc */
    struct Library     *hd_ExpansionBase;
    struct Library     *hd_DenebBase;

    BPTR                hd_SegList;       /* device seglist */

    struct Device      *hd_TimerBase;     /* timer device base */

	struct DenebUnit   *hd_Units[MAXUNITS];
};

#if !defined(__AROS__)
/* Protos
*/

#include "declgate.h"

/* Library base macros
*/

#define __NOLIBBASE__
#define USE_INLINE_STDARG

#ifndef NOBASEDEF
#define	SysBase		base->hd_SysBase
#define	UtilityBase	base->hd_UtilityBase
#define ExpansionBase base->hd_ExpansionBase
#endif /* NOBASEDEF */
#endif

#include "isp1760.h"
#include "deneb.h"
#include "uhwcmd.h"

#endif /* DENEBUSB_H */
