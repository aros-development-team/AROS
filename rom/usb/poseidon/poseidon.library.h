#ifndef POSEIDON_LIBRARY_H
#define POSEIDON_LIBRARY_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for poseidon.library
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#define RELEASEVERSION 0x20090807

#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <sys/time.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/alerts.h>
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

#include <devices/timer.h>
#include <utility/utility.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "poseidon_intern.h"

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/usb_hid.h>
#include <devices/usb_massstorage.h>
#include <devices/usbhardware.h>
#include <libraries/usbclass.h>
#include <proto/poseidon.h>

struct PsdRawDoFmt
{
    ULONG rdf_Len;
    STRPTR rdf_Buf;
};

/* Protos */

void pFreeEndpoint(struct PsdEndpoint *pep);
struct PsdEndpoint * pAllocEndpoint(struct PsdInterface *pif);

void pFreeInterface(struct PsdInterface *pif);
struct PsdInterface * pAllocInterface(struct PsdConfig *pc);

void pFreeConfig(struct PsdConfig *pc);
struct PsdConfig * pAllocConfig(struct PsdDevice *pd);

void pCheckForDeadlock(struct PsdBase *ps, struct PsdLockSem *pls, BOOL excl);
void pInitSem(struct PsdBase *ps, struct PsdLockSem *pls, STRPTR name);
void pDeleteSem(struct PsdBase *ps, struct PsdLockSem *pls);
void pLockSemExcl(struct PsdBase *ps, struct PsdLockSem *pls);
void pLockSemShared(struct PsdBase *ps, struct PsdLockSem *pls);
void pUnlockSem(struct PsdBase *ps, struct PsdLockSem *pls);

BOOL pOpenDOS(struct PsdBase *ps);

UWORD pAllocDevAddr(struct PsdDevice *pd);

BOOL pFixBrokenConfig(struct PsdPipe *pp);
BOOL pGetDevConfig(struct PsdPipe *pp);

ULONG pGetFormLength(struct PsdIFFContext *pic);

struct PsdIFFContext * pAllocForm(struct PsdBase *ps, struct PsdIFFContext *parent, ULONG formid);
void pFreeForm(struct PsdBase *ps, struct PsdIFFContext *pic);
ULONG * pInternalWriteForm(struct PsdIFFContext *pic, ULONG *buf);
struct PsdIFFContext * pAddCfgChunk(struct PsdBase *ps, struct PsdIFFContext *pic, APTR chunk);
STRPTR pGetStringChunk(struct PsdBase *ps, struct PsdIFFContext *pic, ULONG chunkid);
BOOL pMatchStringChunk(struct PsdBase *ps, struct PsdIFFContext *pic, ULONG chunkid, STRPTR str);

BOOL pRemCfgChunk(struct PsdBase *ps, struct PsdIFFContext *pic, ULONG chnkid);
BOOL pAddStringChunk(struct PsdBase *ps, struct PsdIFFContext *pic, ULONG chunkid, STRPTR str);
void pUpdateGlobalCfg(struct PsdBase *ps, struct PsdIFFContext *pic);
APTR pFindCfgChunk(struct PsdBase *ps, struct PsdIFFContext *pic, ULONG chnkid);

BOOL pGetDevConfig(struct PsdPipe *pp);

void pClassScan(struct PsdBase *ps);
void pReleaseBinding(struct PsdBase *ps, struct PsdDevice *pd, struct PsdInterface *pif);
void pReleaseDevBinding(struct PsdBase *ps, struct PsdDevice *pd);
void pReleaseIfBinding(struct PsdBase *ps, struct PsdInterface *pif);

void pGarbageCollectEvents(struct PsdBase *ps);
BOOL pStartEventHandler(struct PsdBase *ps);

BOOL pCheckCfgChanged(struct PsdBase *ps);

ULONG pPowerRecurseDrain(struct PsdBase *ps, struct PsdDevice *pd);
void pPowerRecurseSupply(struct PsdBase *ps, struct PsdDevice *pd);

void pStripString(struct PsdBase *ps, STRPTR str);
struct Node * pFindName(struct PsdBase *ps, struct List *list, STRPTR name);

#define psdAddErrorMsg0(level, origin, fmtstr) psdAddErrorMsgA(level, origin, fmtstr, NULL)

AROS_UFP0(void, pDeviceTask);
AROS_UFP0(void, pPoPoGUITask);
AROS_UFP0(void, pEventHandlerTask);

AROS_UFP2(void, pPutChar,
                   AROS_UFPA(char, ch, D0),
                   AROS_UFPA(struct PsdRawDoFmt *, rdf, A3));

AROS_UFP2(void, pRawFmtLength,
                   AROS_UFPA(char, ch, D0),
                   AROS_UFPA(ULONG *, len, A3));

AROS_UFP1(void, pQuickForwardRequest,
                   AROS_UFPA(struct MsgPort *, msgport, A1));

AROS_UFP1(void, pQuickReplyRequest,
                   AROS_UFPA(struct MsgPort *, msgport, A1));

#endif /* POSEIDON_LIBRARY_H */
