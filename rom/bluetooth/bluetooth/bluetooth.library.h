/*
 *----------------------------------------------------------------------------
 *                 Internal protos for bluetooth.library
 *----------------------------------------------------------------------------
 */

#ifndef BLUETOOTH_LIBRARY_H
#define BLUETOOTH_LIBRARY_H

#define RELEASEVERSION 0x20260818

#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

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

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "bluetooth_intern.h"

#include <libraries/btclass.h>
#include <proto/bluetooth.h>

struct BtRawDoFmt
{
    ULONG rdf_Len;
    STRPTR rdf_Buf;
};

extern const char GM_UNIQUENAME(libname)[];

/* Protos (bluetooth.library.c) */

void bInitSem(struct BtBase *BluetoothBase, struct BtLockSem *bls, STRPTR name);
void bDeleteSem(struct BtBase *BluetoothBase, struct BtLockSem *bls);
void bLockSemExcl(struct BtBase *BluetoothBase, struct BtLockSem *bls);
void bLockSemShared(struct BtBase *BluetoothBase, struct BtLockSem *bls);
void bUnlockSem(struct BtBase *BluetoothBase, struct BtLockSem *bls);

BOOL bOpenDOS(struct BtBase *BluetoothBase);
BOOL bHaveDOS(struct BtBase *BluetoothBase);

void bGarbageCollectEvents(struct BtBase *BluetoothBase);
BOOL bStartEventHandler(struct BtBase *BluetoothBase);

/* PoPo-style pairing popup handled inside the library */
void bShowPairingPopup(struct BtBase *BluetoothBase, struct BtDevice *bd, ULONG type, ULONG passkey);
void bStopPopup(struct BtBase *BluetoothBase);

void bStripString(struct BtBase *BluetoothBase, STRPTR str);
struct Node * bFindName(struct BtBase *BluetoothBase, struct List *list, STRPTR name);
struct BtHardware * bFindHardware(struct BtBase *BluetoothBase, STRPTR name, ULONG unit);

void bFreeBindings(struct BtBase *BluetoothBase, struct BtDevice *bd);
void bFreeDevice(struct BtBase *BluetoothBase, struct BtDevice *bd);
void bFreeService(struct BtBase *BluetoothBase, struct BtService *bsv);
struct BtService * bAllocService(struct BtBase *BluetoothBase, struct BtDevice *bd);
void bFreeEndpoint(struct BtBase *BluetoothBase, struct BtEndpoint *bep);
struct BtEndpoint * bAllocEndpoint(struct BtBase *BluetoothBase, struct BtService *bsv);
void bDeviceClassScan(struct BtBase *BluetoothBase, struct BtDevice *bd);
void bApplyDevConfig(struct BtBase *BluetoothBase, struct BtDevice *bd);
/* persist: registration/bond state changed - also write the config to disk
   (done by the event handler process, which has DOS) */
void bStoreDevConfig(struct BtBase *BluetoothBase, struct BtDevice *bd, BOOL persist);
ULONG bRestoreDevices(struct BtBase *BluetoothBase, struct BtHardware *bth);
void bRekeyDevice(struct BtBase *BluetoothBase, struct BtDevice *bd, const UBYTE *addr, UBYTE addrtype);
void bReplyChannel(struct BtBase *BluetoothBase, struct BtChannel *bch, LONG error, ULONG actual);
BOOL bSubmitCtrl(struct BtBase *BluetoothBase, struct BtHardware *bth, struct BtDevice *bd,
                 UWORD request, UWORD val, UWORD idx, APTR data, ULONG len, LONG *error);

/* Protos (config.c) */

ULONG bGetFormLength(struct BtIFFContext *pic);
struct BtIFFContext * bAllocForm(struct BtBase *BluetoothBase, struct BtIFFContext *parent, ULONG formid);
void bFreeForm(struct BtBase *BluetoothBase, struct BtIFFContext *pic);
ULONG * bInternalWriteForm(struct BtIFFContext *pic, ULONG *buf);
struct BtIFFContext * bAddCfgChunk(struct BtBase *BluetoothBase, struct BtIFFContext *pic, APTR chunk);
STRPTR bGetStringChunk(struct BtBase *BluetoothBase, struct BtIFFContext *pic, ULONG chunkid);
BOOL bMatchStringChunk(struct BtBase *BluetoothBase, struct BtIFFContext *pic, ULONG chunkid, CONST_STRPTR str);
BOOL bRemCfgChunk(struct BtBase *BluetoothBase, struct BtIFFContext *pic, ULONG chnkid);
BOOL bAddStringChunk(struct BtBase *BluetoothBase, struct BtIFFContext *pic, ULONG chunkid, CONST_STRPTR str);
void bUpdateGlobalCfg(struct BtBase *BluetoothBase, struct BtIFFContext *pic);
APTR bFindCfgChunk(struct BtBase *BluetoothBase, struct BtIFFContext *pic, ULONG chnkid);
BOOL bCheckCfgChanged(struct BtBase *BluetoothBase);
void bSyncStackCfg(struct BtBase *BluetoothBase);

/* Protos (hwtask.c) */

void bHandleChannel(struct BtBase *BluetoothBase, struct BtHardware *bth, struct BtChannel *bch, BOOL direct);

#define btAddErrorMsg0(level, origin, fmtstr) btAddErrorMsgA(level, origin, fmtstr, NULL)

AROS_UFP0(void, bHWTask);
AROS_UFP0(void, bEventHandlerTask);

AROS_UFP2(void, bPutChar,
                   AROS_UFPA(char, ch, D0),
                   AROS_UFPA(struct BtRawDoFmt *, rdf, A3));

AROS_UFP2(void, bRawFmtLength,
                   AROS_UFPA(char, ch, D0),
                   AROS_UFPA(ULONG *, len, A3));

#endif /* BLUETOOTH_LIBRARY_H */
