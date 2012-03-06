#ifndef HID_CLASS_H
#define HID_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for HID class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/rawkeycodes.h>

#include <datatypes/soundclass.h>

#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/lowlevel_ext.h>
#include <graphics/layers.h>

#include <devices/usb_hid.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "hid.h"

/* Protos */

struct NepClassHid * GM_UNIQUENAME(usbAttemptInterfaceBinding)(struct NepHidBase *nh, struct PsdInterface *pif);
struct NepClassHid * GM_UNIQUENAME(usbForceInterfaceBinding)(struct NepHidBase *nh, struct PsdInterface *pif);
void GM_UNIQUENAME(usbReleaseInterfaceBinding)(struct NepHidBase *nh, struct NepClassHid *nch);

extern UBYTE usbkeymap[];

BOOL GM_UNIQUENAME(nLoadClassConfig)(struct NepHidBase *nh);
BOOL GM_UNIQUENAME(nLoadBindingConfig)(struct NepClassHid *nch, BOOL gui);
LONG GM_UNIQUENAME(nOpenBindingCfgWindow)(struct NepHidBase *nh, struct NepClassHid *nch);

void nInstallLLPatch(struct NepHidBase *nh);
struct NepClassHid * GM_UNIQUENAME(nAllocHid)(void);
void GM_UNIQUENAME(nFreeHid)(struct NepClassHid *nch);

struct NepHidItem * nFindItemID(struct NepClassHid *nch, UWORD id, UWORD itype, ULONG *pos);
UWORD nFindItemUsage(struct NepClassHid *nch, ULONG usage, UWORD itype);
BOOL nFindCollID(struct NepClassHid *nch, struct NepHidCollection *nhc, ULONG collidmin, ULONG collidmax);
BOOL nDetectDefaultAction(struct NepClassHid *nch, struct NepHidItem *nhi, struct List *lst, struct NepHidCollection *nhc, ULONG uid);
BOOL nCheckForDefaultAction(struct NepClassHid *nch,  struct NepHidItem *nhi, struct List *lst, struct NepHidCollection *nhc, ULONG uid);
struct NepHidAction * nAllocAction(struct NepClassHid *nch, struct List *lst, UWORD utype);

void nCheckReset(struct NepClassHid *nch);

BOOL nProcessItem(struct NepClassHid *nch, struct NepHidItem *nhi, UBYTE *buf);
BOOL nDoAction(struct NepClassHid *nch, struct NepHidAction *nha, struct NepHidItem *nhi, ULONG uid, LONG value, BOOL downevent);
void nFlushEvents(struct NepClassHid *nch);
STRPTR nGetUsageName(struct NepClassHid *nch, ULONG uid);
void nCleanCollection(struct NepClassHid *nch, struct NepHidCollection *nhc);
void nSendRawKey(struct NepClassHid *nch, UWORD key);

void nFreeReport(struct NepClassHid *nch, struct NepHidReport *nhr);
BOOL nReadReports(struct NepClassHid *nch);
BOOL nParseReport(struct NepClassHid *nch, struct NepHidReport *nhrptr);
void nLoadActionConfig(struct NepClassHid *nch);
BOOL nAddExtraReport(struct NepClassHid *nch);

BOOL nParseWacom(struct NepClassHid *nch, UBYTE *buf, ULONG len);
BOOL nDetectWacom(struct NepClassHid *nch);
BOOL nAddUsage(struct NepClassHid *nch, struct List *list, ULONG umin, ULONG umax);

void nGenerateOutReport(struct NepClassHid *nch, struct NepHidReport *nhr, UBYTE *buf);
void nGenerateFeatReport(struct NepClassHid *nch, struct NepHidReport *nhr, UBYTE *buf);
void nEncodeItemBuffer(struct NepClassHid *nch, struct NepHidItem *nhi, UBYTE *buf);

void nInstallLastActionHero(struct NepClassHid *nch);

void GM_UNIQUENAME(nGUITaskCleanup)(struct NepClassHid *nch);

struct NepHidGItem * nAllocGOutItem(struct NepClassHid *nch, struct NepHidItem *nhi, struct List *actionlist, ULONG usageid);
struct NepHidGItem * nAllocGItem(struct NepClassHid *nch, struct NepHidItem *nhi, struct List *actionlist, ULONG usageid);
void nFreeGItem(struct NepClassHid *nch, struct NepHidGItem *nhgi);

BOOL nLoadItem(struct NepClassHid *nch, struct PsdIFFContext *rppic, struct List *lst, UWORD idbase);
struct PsdIFFContext * nSaveItem(struct NepClassHid *nch, struct PsdIFFContext *rppic, struct List *lst, UWORD idbase);

struct InputEvent *nInvertString(struct NepHidBase *nh, STRPTR str, struct KeyMap *km);
void nFreeIEvents(struct NepHidBase *nh, struct InputEvent *event);
BOOL nSendKeyString(struct NepHidBase *nh, STRPTR str);

void nLastActionHero(struct NepHidBase *nh);

void nDebugReport(struct NepClassHid *nch, struct NepHidReport *nhr);

struct NepHidSound * nLoadSound(struct NepHidBase *nh, STRPTR name);
BOOL nPlaySound(struct NepHidBase *nh, struct NepHidAction *nha);
void nFreeSound(struct NepHidBase *nh, struct NepHidSound *nhs);

LONG nEasyRequestA(struct NepHidBase *nh, STRPTR body, STRPTR gadgets, ULONG *params);

// FIXME
LONG nEasyRequest(struct NepHidBase *nh, STRPTR body, STRPTR gadgets, ...);

AROS_UFP0(void, GM_UNIQUENAME(nHidTask));
AROS_UFP0(void, GM_UNIQUENAME(nGUITask));
AROS_UFP0(void, GM_UNIQUENAME(nHIDCtrlGUITask));
AROS_UFP0(void, GM_UNIQUENAME(nDispatcherTask));

AROS_LD1(ULONG, nReadJoyPort,
         AROS_LDA(ULONG, port, D0),
         struct Library *, LowLevelBase, 5, hid);
          
AROS_LD2(ULONG, nSetJoyPortAttrsA,
         AROS_LDA(ULONG, port, D0),
         AROS_LDA(struct TagItem *, tags, A1),
         struct Library *, LowLevelBase, 22, hid);

AROS_UFP3(LONG, GM_UNIQUENAME(USBKeyListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct HidUsageIDMap *, hum, A1));

AROS_UFP3(LONG, GM_UNIQUENAME(ReportListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct NepHidCollection *, nhc, A1));
          
AROS_UFP3(LONG, GM_UNIQUENAME(ItemListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct NepHidGItem *, nhgi, A1));
          
AROS_UFP3(LONG, GM_UNIQUENAME(ActionListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct NepHidAction *, nha, A1));

AROS_UFP3(IPTR, GM_UNIQUENAME(ActionDispatcher),
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, GM_UNIQUENAME(HCActionDispatcher),
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

void GM_UNIQUENAME(nHIDCtrlGUITaskCleanup)(struct NepClassHid *nch);
struct NepHidGItem * nAllocGHCItem(struct NepClassHid *nch, struct NepHidItem *nhi, struct List *actionlist, ULONG usageid);

static inline UWORD GET_WTYPE(struct List *list)
{
    UWORD *w = (UWORD *)(&list->lh_Type);
    return *w;
}

static inline void SET_WTYPE(struct List *list, UWORD val)
{
    UWORD *w = (UWORD *)(&list->lh_Type);
    *w = val;
}


#endif /* HID_CLASS_H */
