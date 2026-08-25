/*
** ActionClass - main content class for the Bluetooth prefs (subclass of
** Group.mui). See ActionClass.h. This follows Trident's structure: the whole
** panel and all logic live here, exposed as MUI methods.
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <libraries/bluetooth.h>

#define MUIMASTER_YES_INLINE_STDARG
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/bluetooth.h>
#include <proto/btclass.h>       /* btcGetAttrs/btcDoMethod on a class base (BtClsBase) */

#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "bluetoothprefs.h"
#include "ActionClass.h"
#include "IconListClass.h"
#include "ScanWinClass.h"
#include "DevWinClass.h"
#include "bluzinglogo.h"
#include "debug.h"

/*
 * This file is one large MUI object tree: tag-list values are Object* and
 * string literals passed where the macros expect IPTR/STRPTR. Under gcc 16
 * those benign conversions are hard errors, so relax exactly those two here.
 */
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#define FREELIST(l) { struct MinNode *n; while((n = (struct MinNode *)RemHead((struct List *)&(l)))) FreeVec(n); }

/* left navigation entries */
static struct BtNavEntry naventries[] =
{
    { "General",  BTPAGE_GENERAL,  0 },
    { "Hardware", BTPAGE_HARDWARE, 1 },
    { "Devices",  BTPAGE_DEVICES,  2 },
    { "Classes",  BTPAGE_CLASSES,  3 },
    { "Options",  BTPAGE_OPTIONS,  4 },
    { "Config",   BTPAGE_CONFIG,   4 },
};

static const char *errlvlstrings[] = { "Failures", "Errors", "Warnings", "All messages", NULL };
/* BGCA_PopupDeviceNew: BGCP_NEVER .. BGCP_ALWAYS */
static const char *popupnewstrings[] = { "Never", "When seen for the first time", "When registered without a binding", "Always", NULL };

/* *** display hooks ***
 * The first column of each list is rendered with a leading icon using the
 * MUI "\33O[<hex image ptr>] text" escape. The image handles belong to the
 * IconListClass instance of the list, reached through h->h_Data (the
 * BtActionData) and the list object. */

#define ICONCOL(buf, imgs, idx, text) \
    (snprintf((buf), sizeof(buf), "\33O[%08lx] %s", (unsigned long)(IPTR)(imgs)[idx], (text)), (buf))

AROS_UFH3(LONG, NavDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct BtNavEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    struct BtActionData *ad = (struct BtActionData *)h->h_Data;
    static char b0[48];
    if(e) *a = ICONCOL(b0, ICONLIST_IMAGES(ad->navlst), e->icon, e->label);
    else  *a = "";
    return 0;
    AROS_USERFUNC_EXIT
}

AROS_UFH3(LONG, HWDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct HWEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    struct BtActionData *ad = (struct BtActionData *)h->h_Data;
    static char b0[96];
    if(e) { *a++ = ICONCOL(b0, ICONLIST_IMAGES(ad->hwlist), e->icon, e->name); *a++ = e->addr; *a++ = e->state; *a++ = e->prod; *a = e->info; }
    else  { *a++ = "Unit"; *a++ = "Address"; *a++ = "State"; *a++ = "Product"; *a = "Features"; }
    return 0;
    AROS_USERFUNC_EXIT
}

AROS_UFH3(LONG, DevDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct DevEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    struct BtActionData *ad = (struct BtActionData *)h->h_Data;
    static char b0[64], b1[80], rssibuf[12];
    if(e) {
        if(e->rssi != 127) snprintf(rssibuf, sizeof(rssibuf), "%ld", (long)e->rssi); else strcpy(rssibuf, "-");
        /* Address column shows the online/offline LED; Name column shows the
         * device-class icon before the name. The bearer type is a per-service
         * property (a device may support several), shown in the Information
         * window's service list, not on the device row. */
        *a++ = ICONCOL(b0, ICONLIST_IMAGES(ad->devlist), e->statusicon, e->addr);
        *a++ = ICONCOL(b1, ICONLIST_IMAGES(ad->devlist), e->icon, e->name);
        *a++ = rssibuf; *a = e->flags;
    } else { *a++ = "Address"; *a++ = "Name"; *a++ = "RSSI"; *a = "Status"; }
    return 0;
    AROS_USERFUNC_EXIT
}

AROS_UFH3(LONG, ClsDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct ClsEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    struct BtActionData *ad = (struct BtActionData *)h->h_Data;
    static char b0[64];
    if(e) { *a++ = ICONCOL(b0, ICONLIST_IMAGES(ad->clslist), e->icon, e->name); *a++ = e->use; *a = e->path; }
    else  { *a++ = "Class"; *a++ = "Bindings"; *a = "Path"; }
    return 0;
    AROS_USERFUNC_EXIT
}

AROS_UFH3(LONG, ErrDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct ErrEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    if(e) { *a++ = e->level; *a++ = e->origin; *a = e->msg; }
    else  { *a++ = "Lvl"; *a++ = "Origin"; *a = "Message"; }
    return 0;
    AROS_USERFUNC_EXIT
}

AROS_UFH3(LONG, CfgDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct CfgEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    static char sizebuf[16];
    if(e) { snprintf(sizebuf, sizeof(sizebuf), "%lu", (unsigned long)e->size); *a++ = e->type; *a++ = e->desc; *a++ = e->owner; *a = sizebuf; }
    else  { *a++ = "Type"; *a++ = "Description"; *a++ = "Owner"; *a = "Size"; }
    return 0;
    AROS_USERFUNC_EXIT
}

/* *** selection helpers *** */

static APTR SelectedDevice(struct BtActionData *data)
{
    struct DevEntry *e = NULL;
    DoMethod(data->devlist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
    return e ? e->bd : NULL;
}

static void SetStatus(struct BtActionData *data, CONST_STRPTR s)
{
    set(data->statustxt, MUIA_Text_Contents, (IPTR)s);
}

static void RefreshHardware(struct BtActionData *data);   /* defined below */

static APTR SelectedHardware(struct BtActionData *data)
{
    struct HWEntry *e = NULL;
    DoMethod(data->hwlist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
    return e ? e->bth : NULL;
}

/* Manually bring up an HCI transport at runtime - the GUI equivalent of the
   AddBTHardware shell command (mirrors Trident's hardware panel). */
static void DoHwAdd(struct BtActionData *data)
{
    STRPTR name = NULL;
    IPTR unit = 0;
    APTR bth;

    get(data->hwdevobj, MUIA_String_Contents, &name);
    get(data->hwunitobj, MUIA_String_Integer, &unit);
    if(!name || !name[0]) {
        SetStatus(data, "Enter a device driver name first.");
        return;
    }
    SetStatus(data, "Adding radio...");
    if((bth = btAddHardware(name, (ULONG)unit))) {
        btEnumerateHardware(bth);
        btClassScan();
        RefreshHardware(data);
        SetStatus(data, "Radio added.");
    } else {
        SetStatus(data, "Could not add that radio.");
    }
}

static void DoHwRemove(struct BtActionData *data)
{
    APTR bth = SelectedHardware(data);
    if(bth) {
        btRemHardware(bth);
        RefreshHardware(data);
        SetStatus(data, "Radio removed.");
    } else {
        SetStatus(data, "Select a radio to remove.");
    }
}

/* *** class settings windows ***
 * A class may offer a settings GUI for its defaults (BCCA_HasClassCfgGUI) and
 * one per binding (BCCA_HasBindingCfgGUI) - bthid.class does, for key
 * mappings and actions. Trident opens them with the class methods
 * UCM_OpenCfgWindow/UCM_OpenBindingCfgWindow; the BCM_ ones are the same. */

static APTR SelectedClass(struct BtActionData *data)
{
    struct ClsEntry *e = NULL;
    DoMethod(data->clslist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
    return e ? e->bc : NULL;
}

/* does the class offer that settings window? (attr = BCCA_HasClassCfgGUI / BCCA_HasBindingCfgGUI) */
static BOOL ClassHasGUI(APTR bc, ULONG attr)
{
    struct Library *BtClsBase = NULL;
    IPTR has = FALSE;
    if(!bc) return FALSE;
    btGetAttrs(BGA_BTCLASS, bc, BCA_ClassBase, &BtClsBase, TAG_END);
    if(!BtClsBase) return FALSE;
    btcGetAttrs(BCGA_CLASS, NULL, attr, &has, TAG_END);
    return has ? TRUE : FALSE;
}

static BOOL OpenBindingWindow(APTR bc, APTR binding)
{
    struct Library *BtClsBase = NULL;
    if(!bc || !binding) return FALSE;
    btGetAttrs(BGA_BTCLASS, bc, BCA_ClassBase, &BtClsBase, TAG_END);
    if(!BtClsBase) return FALSE;
    return btcDoMethod(BCM_OpenBindingCfgWindow, binding) ? TRUE : FALSE;
}

/* the device's own binding, or any of its services' bindings, whose class
 * has a binding settings window */
static BOOL DeviceHasSettings(APTR bd)
{
    APTR binding = NULL, bc = NULL;
    struct List *svcl;
    struct Node *bsv;
    BOOL has = FALSE;
    if(!bd) return FALSE;
    btLockReadBase();
    btGetAttrs(BGA_DEVICE, bd, BDA_Binding, &binding, BDA_BindingClass, &bc, BDA_ServiceList, &svcl, TAG_END);
    if(binding && ClassHasGUI(bc, BCCA_HasBindingCfgGUI)) {
        has = TRUE;
    } else {
        for(bsv = svcl->lh_Head; bsv->ln_Succ && !has; bsv = bsv->ln_Succ) {
            binding = NULL; bc = NULL;
            btGetAttrs(BGA_SERVICE, bsv, BSVA_Binding, &binding, BSVA_BindingClass, &bc, TAG_END);
            if(binding && ClassHasGUI(bc, BCCA_HasBindingCfgGUI)) has = TRUE;
        }
    }
    btUnlockBase();
    return has;
}

/* open the settings window(s) of every binding on the device (Trident's Action_Dev_Configure) */
static ULONG OpenDeviceSettings(APTR bd)
{
    APTR binding = NULL, bc = NULL;
    struct List *svcl;
    struct Node *bsv;
    ULONG opened = 0;
    if(!bd) return 0;
    btLockReadBase();
    btGetAttrs(BGA_DEVICE, bd, BDA_Binding, &binding, BDA_BindingClass, &bc, BDA_ServiceList, &svcl, TAG_END);
    if(binding && bc) {
        if(OpenBindingWindow(bc, binding)) opened++;
    } else {
        for(bsv = svcl->lh_Head; bsv->ln_Succ; bsv = bsv->ln_Succ) {
            binding = NULL; bc = NULL;
            btGetAttrs(BGA_SERVICE, bsv, BSVA_Binding, &binding, BSVA_BindingClass, &bc, TAG_END);
            if(binding && bc && OpenBindingWindow(bc, binding)) opened++;
        }
    }
    btUnlockBase();
    return opened;
}

static void UpdateDevButtons(struct BtActionData *data)
{
    set(data->bt_devsettings, MUIA_Disabled, !DeviceHasSettings(SelectedDevice(data)));
}

static void UpdateClsButtons(struct BtActionData *data)
{
    set(data->bt_clscfg, MUIA_Disabled, !ClassHasGUI(SelectedClass(data), BCCA_HasClassCfgGUI));
}

static void DoClsConfigure(struct BtActionData *data)
{
    APTR bc = SelectedClass(data);
    struct Library *BtClsBase = NULL;
    if(!bc) { SetStatus(data, "Select a class first."); return; }
    btGetAttrs(BGA_BTCLASS, bc, BCA_ClassBase, &BtClsBase, TAG_END);
    if(BtClsBase && btcDoMethod(BCM_OpenCfgWindow)) {
        SetStatus(data, "Class settings window opened.");
    } else {
        SetStatus(data, "This class has no settings window.");
    }
}

static void DoDevSettings(struct BtActionData *data)
{
    APTR bd = SelectedDevice(data);
    if(!bd) { SetStatus(data, "Select a device first."); return; }
    if(OpenDeviceSettings(bd)) {
        SetStatus(data, "Device settings window opened.");
    } else {
        SetStatus(data, "No class with a settings window is bound to this device.");
    }
}

/* *** refresh *** */

/* pick a device-class icon from the classic Class-of-Device or the LE Appearance */
ULONG DeviceIconFor(IPTR cod, IPTR appearance, IPTR isclassic)
{
    if(isclassic && cod) {
        ULONG major = (cod >> 8) & 0x1f;
        ULONG minor = (cod >> 2) & 0x3f;
        switch(major) {
            case 0x01: return ICON_DEV_COMPUTER;
            case 0x02: return ICON_DEV_PHONE;
            case 0x04: return ICON_DEV_HEADSET;          /* audio / video */
            case 0x05:                                    /* peripheral */
                switch((minor >> 4) & 0x03) {
                    case 1: return ICON_DEV_KEYBOARD;
                    case 2: return ICON_DEV_MOUSE;
                    case 3: return ICON_DEV_KEYBOARD;     /* keyboard+pointing combo */
                }
                return ICON_DEV_GENERIC;
        }
    }
    if(appearance) {
        switch((appearance >> 6) & 0x3ff) {              /* GAP appearance category */
            case 1:  return ICON_DEV_PHONE;              /* Phone */
            case 2:  return ICON_DEV_COMPUTER;           /* Computer */
            case 15:                                      /* HID */
                if(appearance == 961) return ICON_DEV_KEYBOARD;
                if(appearance == 962) return ICON_DEV_MOUSE;
                return ICON_DEV_GENERIC;
        }
    }
    return ICON_DEV_GENERIC;
}


/* *** incremental list update ***
 * The stack raises a device event for every advertising report during a scan,
 * so rebuilding a list on each event tears it down many times a second - the
 * list flickers and the selection is lost. Instead merge a freshly built set
 * of entries into the live list: update rows in place (redrawing only those),
 * insert new ones, remove vanished ones. */

static LONG ListIndexOf(Object *list, APTR entry)
{
    LONG i = 0;
    APTR p;
    for(;;) {
        DoMethod(list, MUIM_List_GetEntry, i, &p);
        if(!p) return -1;
        if(p == entry) return i;
        i++;
    }
}

#define DEVENTRY_DATA(e) (((UBYTE *)(e)) + offsetof(struct DevEntry, icon))
#define DEVENTRY_DATALEN (sizeof(struct DevEntry) - offsetof(struct DevEntry, icon))

void MergeDevList(Object *list, struct MinList *entries, struct MinList *fresh)
{
    struct DevEntry *e, *next, *f;
    BOOL structural = FALSE;

    for(e = (struct DevEntry *)entries->mlh_Head; (next = (struct DevEntry *)e->node.mln_Succ); e = next) {
        for(f = (struct DevEntry *)fresh->mlh_Head; f->node.mln_Succ; f = (struct DevEntry *)f->node.mln_Succ) {
            if(f->bd == e->bd) break;
        }
        if(!f->node.mln_Succ) {
            /* gone */
            LONG pos = ListIndexOf(list, e);
            if(!structural) { set(list, MUIA_List_Quiet, TRUE); structural = TRUE; }
            if(pos >= 0) DoMethod(list, MUIM_List_Remove, pos);
            Remove((struct Node *)e);
            FreeVec(e);
            continue;
        }
        if(memcmp(DEVENTRY_DATA(e), DEVENTRY_DATA(f), DEVENTRY_DATALEN)) {
            LONG pos;
            memcpy(DEVENTRY_DATA(e), DEVENTRY_DATA(f), DEVENTRY_DATALEN);
            if(!structural && ((pos = ListIndexOf(list, e)) >= 0)) {
                DoMethod(list, MUIM_List_Redraw, pos);
            }
        }
        Remove((struct Node *)f);
        FreeVec(f);
    }
    /* whatever is left in fresh is new */
    while((f = (struct DevEntry *)RemHead((struct List *)fresh))) {
        if(!structural) { set(list, MUIA_List_Quiet, TRUE); structural = TRUE; }
        AddTail((struct List *)entries, (struct Node *)f);
        DoMethod(list, MUIM_List_InsertSingle, f, MUIV_List_Insert_Bottom);
    }
    if(structural) set(list, MUIA_List_Quiet, FALSE);
}

static void RefreshDevices(struct BtActionData *data)
{
    struct List *hwl, *devl;
    struct Node *bth, *bd;
    struct MinList fresh;

    NewList((struct List *)&fresh);
    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &hwl, TAG_END);
    for(bth = hwl->lh_Head; bth->ln_Succ; bth = bth->ln_Succ) {
        btGetAttrs(BGA_HARDWARE, bth, BHA_DeviceList, &devl, TAG_END);
        for(bd = devl->lh_Head; bd->ln_Succ; bd = bd->ln_Succ) {
            struct DevEntry *e = AllocVec(sizeof(struct DevEntry), MEMF_CLEAR);
            STRPTR name = NULL, addr = NULL;
            IPTR isc = 0, isl = 0, isreg = 0, isb = 0, isconn = 0, isdead = 0, cod = 0, appear = 0, pstate = 0;
            LONG rssi = 127;
            if(!e) break;
            btGetAttrs(BGA_DEVICE, bd, BDA_Name, &name, BDA_AddressString, &addr, BDA_RSSI, &rssi,
                       BDA_IsClassic, &isc, BDA_IsLE, &isl, BDA_IsRegistered, &isreg, BDA_IsBonded, &isb,
                       BDA_IsConnected, &isconn, BDA_IsDead, &isdead, BDA_PairingState, &pstate,
                       BDA_ClassOfDevice, &cod, BDA_Appearance, &appear, TAG_END);
            /* the Devices page lists only devices we actually use (connected,
             * registered or bonded); freshly discovered ones live in the
             * "Add Device" window instead. */
            if(!(isreg || isb || isconn)) { FreeVec(e); continue; }
            e->bd = bd;
            e->icon = DeviceIconFor(cod, appear, isc);
            e->statusicon = isdead ? ICON_LED_RED : (isconn ? ICON_LED_GREEN : ICON_LED_GRAY);
            e->rssi = rssi;
            strncpy(e->addr, addr ? addr : "?", sizeof(e->addr)-1);
            strncpy(e->name, name ? name : "?", sizeof(e->name)-1);
            strcpy(e->type, (isc && isl) ? "dual" : (isl ? "LE" : "BR/EDR"));
            /* a device that is connected only because it is being paired says so */
            if(pstate == BDPS_WAITUSER)
                snprintf(e->flags, sizeof(e->flags), "pairing - enter the passkey");
            else if(pstate == BDPS_INPROGRESS)
                snprintf(e->flags, sizeof(e->flags), "pairing...");
            else
                snprintf(e->flags, sizeof(e->flags), "%s%s%s%s",
                         isreg ? "registered " : "", isb ? "bonded " : "",
                         isconn ? "connected " : "", isdead ? "unreachable " : "");
            if(!e->flags[0]) strcpy(e->flags, "discovered");
            AddTail((struct List *)&fresh, (struct Node *)e);
        }
    }
    btUnlockBase();
    MergeDevList(data->devlist, &data->deventries, &fresh);
    if(data->devwin) DoMethod(data->devwin, MUIM_DevWin_Populate);
    if(data->scanwin) DoMethod(data->scanwin, MUIM_ScanWin_Populate);
    if(data->bt_devsettings) UpdateDevButtons(data);
}

static void RefreshHardware(struct BtActionData *data)
{
    struct List *hwl;
    struct Node *bth;

    set(data->hwlist, MUIA_List_Quiet, TRUE);
    DoMethod(data->hwlist, MUIM_List_Clear);
    FREELIST(data->hwentries);
    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &hwl, TAG_END);
    for(bth = hwl->lh_Head; bth->ln_Succ; bth = bth->ln_Succ) {
        struct HWEntry *e = AllocVec(sizeof(struct HWEntry), MEMF_CLEAR);
        STRPTR name = NULL, prod = NULL, addr = NULL;
        IPTR unit = 0, state = 0, isc = 0, isl = 0, disc = 0, num = 0;
        if(!e) break;
        btGetAttrs(BGA_HARDWARE, bth, BHA_DeviceName, &name, BHA_DeviceUnit, &unit, BHA_ProductName, &prod,
                   BHA_AddressString, &addr, BHA_State, &state, BHA_IsClassic, &isc, BHA_IsLE, &isl,
                   BHA_IsDiscovering, &disc, BHA_NumDevices, &num, TAG_END);
        e->bth = bth;
        e->icon = disc ? ICON_LED_ORANGE : ICON_LED_GREEN;
        snprintf(e->name, sizeof(e->name), "%s/%ld", name ? FilePart(name) : "?", (long)unit);
        strncpy(e->addr, addr ? addr : "?", sizeof(e->addr)-1);
        strncpy(e->state, btNumToStr(BNTS_HWSTATE, state, "?"), sizeof(e->state)-1);
        strncpy(e->prod, prod ? prod : "?", sizeof(e->prod)-1);
        snprintf(e->info, sizeof(e->info), "%s%s%s, %ld dev%s", isc ? "BR/EDR" : "", (isc&&isl) ? "+" : "",
                 isl ? "LE" : "", (long)num, disc ? ", scanning" : "");
        AddTail((struct List *)&data->hwentries, (struct Node *)e);
        DoMethod(data->hwlist, MUIM_List_InsertSingle, e, MUIV_List_Insert_Bottom);
    }
    btUnlockBase();
    set(data->hwlist, MUIA_List_Quiet, FALSE);
}

static void RefreshClasses(struct BtActionData *data)
{
    struct List *cl;
    struct Node *bc;

    set(data->clslist, MUIA_List_Quiet, TRUE);
    DoMethod(data->clslist, MUIM_List_Clear);
    FREELIST(data->clsentries);
    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_ClassList, &cl, TAG_END);
    for(bc = cl->lh_Head; bc->ln_Succ; bc = bc->ln_Succ) {
        struct ClsEntry *e = AllocVec(sizeof(struct ClsEntry), MEMF_CLEAR);
        STRPTR name = NULL, path = NULL;
        IPTR use = 0;
        if(!e) break;
        btGetAttrs(BGA_BTCLASS, bc, BCA_ClassName, &name, BCA_FullPath, &path, BCA_UseCount, &use, TAG_END);
        e->bc = bc;
        e->icon = ICON_CLASSES;
        strncpy(e->name, name ? name : "?", sizeof(e->name)-1);
        snprintf(e->use, sizeof(e->use), "%ld", (long)use);
        strncpy(e->path, path ? path : "?", sizeof(e->path)-1);
        AddTail((struct List *)&data->clsentries, (struct Node *)e);
        DoMethod(data->clslist, MUIM_List_InsertSingle, e, MUIV_List_Insert_Bottom);
    }
    btUnlockBase();
    set(data->clslist, MUIA_List_Quiet, FALSE);
    if(data->bt_clscfg) UpdateClsButtons(data);
}

static void RefreshErrors(struct BtActionData *data)
{
    struct List *el;
    struct Node *bem;

    set(data->errlist, MUIA_List_Quiet, TRUE);
    DoMethod(data->errlist, MUIM_List_Clear);
    FREELIST(data->errentries);
    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_ErrorMsgList, &el, TAG_END);
    for(bem = el->lh_Head; bem->ln_Succ; bem = bem->ln_Succ) {
        struct ErrEntry *e = AllocVec(sizeof(struct ErrEntry), MEMF_CLEAR);
        IPTR level = 0;
        STRPTR origin = NULL, msg = NULL;
        if(!e) break;
        btGetAttrs(BGA_ERRORMSG, bem, BEMA_Level, &level, BEMA_Origin, &origin, BEMA_Msg, &msg, TAG_END);
        snprintf(e->level, sizeof(e->level), "%ld", (long)level);
        strncpy(e->origin, origin ? origin : "?", sizeof(e->origin)-1);
        strncpy(e->msg, msg ? msg : "", sizeof(e->msg)-1);
        AddTail((struct List *)&data->errentries, (struct Node *)e);
        DoMethod(data->errlist, MUIM_List_InsertSingle, e, MUIV_List_Insert_Bottom);
    }
    btUnlockBase();
    set(data->errlist, MUIA_List_Quiet, FALSE);
    set(data->errlist, MUIA_List_Active, MUIV_List_Active_Bottom);
}

/* *** config page ***
 * Lists every form of the stack's configuration the way Trident's Config
 * panel does: the global stack config, one entry per known device (with its
 * registration/bond state), per-device and per-class private prefs, forced
 * bindings. Built from the IFF image btWriteCfg() produces. */

static ULONG RdBE(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3];
}

/* direct child chunk of an IFF FORM: pointer to its data (after the 8 byte header) */
static const UBYTE *FindChunkData(const UBYTE *form, ULONG id)
{
    const UBYTE *p = form + 12;
    const UBYTE *end = form + 8 + RdBE(form + 4);
    while(p + 8 <= end) {
        ULONG clen = RdBE(p + 4);
        if(RdBE(p) == id) return p + 8;
        p += 8 + ((clen + 1) & ~1UL);
    }
    return NULL;
}

static void AddCfgEntry(struct BtActionData *data, ULONG formid, ULONG parentid, ULONG size,
                        CONST_STRPTR type, CONST_STRPTR desc, CONST_STRPTR owner, CONST_STRPTR devid)
{
    struct CfgEntry *e = AllocVec(sizeof(struct CfgEntry), MEMF_CLEAR);
    if(!e) return;
    e->formid = formid;
    e->parentid = parentid;
    e->size = size;
    strncpy(e->type, type, sizeof(e->type)-1);
    strncpy(e->desc, desc, sizeof(e->desc)-1);
    strncpy(e->owner, owner ? owner : "Bluetooth", sizeof(e->owner)-1);
    strncpy(e->devid, devid ? devid : "", sizeof(e->devid)-1);
    AddTail((struct List *)&data->cfgentries, (struct Node *)e);
    DoMethod(data->cfglist, MUIM_List_InsertSingle, e, MUIV_List_Insert_Bottom);
}

static void WalkCfgForm(struct BtActionData *data, const UBYTE *form, ULONG parentid, ULONG depth, CONST_STRPTR devid)
{
    ULONG formid = RdBE(form + 8);
    ULONG size = RdBE(form + 4) + 8;
    const UBYTE *p, *end, *s;
    char desc[96];
    char mydevid[32];
    CONST_STRPTR owner = (CONST_STRPTR)FindChunkData(form, IFFCHNK_OWNER);

    switch(formid) {
        case IFFFORM_BTCFG:
            break;   /* the root itself is not listed, its children are */
        case IFFFORM_BTSTACKCFG:
            AddCfgEntry(data, formid, parentid, size, "Stack", "Global stack configuration (radios, classes, options)", "Bluetooth", NULL);
            break;
        case IFFFORM_BTDEVICECFG: {
            const UBYTE *dreg = FindChunkData(form, IFFCHNK_REGDEVICE);
            const UBYTE *keys = FindChunkData(form, IFFCHNK_KEYS);
            CONST_STRPTR name = (CONST_STRPTR)FindChunkData(form, IFFCHNK_NAME);
            s = FindChunkData(form, IFFCHNK_DEVID);
            strncpy(mydevid, s ? (const char *)s : "?", sizeof(mydevid)-1);
            mydevid[sizeof(mydevid)-1] = 0;
            devid = mydevid;
            snprintf(desc, sizeof(desc), "%s (%s)%s%s%s%s%s", name ? name : mydevid, mydevid,
                     dreg ? " - registered" : "",
                     keys ? ", bonded:" : "",
                     (keys && (keys[0] & BDKF_LINKKEY)) ? " link key" : "",
                     (keys && (keys[0] & BDKF_LTK)) ? ((keys[0] & BDKF_SC) ? " LE key (SC)" : " LE key") : "",
                     (keys && (keys[0] & BDKF_IRK)) ? " IRK" : "");
            AddCfgEntry(data, formid, parentid, size, "Device", desc, "Bluetooth", mydevid);
            break;
        }
        case IFFFORM_BTCLASSCFG:
            snprintf(desc, sizeof(desc), "Default prefs for %s", owner ? owner : "?");
            AddCfgEntry(data, formid, parentid, size, "Class", desc, owner, NULL);
            break;
        case IFFFORM_BTDEVCFGDATA:
            snprintf(desc, sizeof(desc), "%s prefs for this device", owner ? owner : "?");
            AddCfgEntry(data, formid, parentid, size, "  Device prefs", desc, owner, devid);
            break;
        case IFFFORM_BTSVCCFGDATA:
            s = FindChunkData(form, IFFCHNK_SVCID);
            snprintf(desc, sizeof(desc), "%s prefs for service %s", owner ? owner : "?", s ? (const char *)s : "?");
            AddCfgEntry(data, formid, parentid, size, "  Service prefs", desc, owner, devid);
            break;
        case IFFFORM_BTCLASSDATA:
        case IFFFORM_BTDEVCLSDATA:
        case IFFFORM_BTSVCCLSDATA:
            snprintf(desc, sizeof(desc), "%s private data", owner ? owner : "?");
            AddCfgEntry(data, formid, parentid, size, "  Private data", desc, owner, devid);
            break;
        default:
            snprintf(desc, sizeof(desc), "Unknown form %c%c%c%c", (int)form[8], (int)form[9], (int)form[10], (int)form[11]);
            AddCfgEntry(data, formid, parentid, size, "Unknown", desc, owner, devid);
            break;
    }

    if(depth >= 3) return;
    p = form + 12;
    end = form + 8 + RdBE(form + 4);
    while(p + 8 <= end) {
        ULONG cid = RdBE(p), clen = RdBE(p + 4);
        if(cid == ID_FORM) {
            WalkCfgForm(data, p, formid, depth + 1, devid);
        } else if((cid == IFFCHNK_FORCEDBIND) && (formid == IFFFORM_BTDEVICECFG)) {
            snprintf(desc, sizeof(desc), "Forced binding to %s", (const char *)(p + 8));
            AddCfgEntry(data, cid, formid, clen + 8, "  Binding", desc, (CONST_STRPTR)(p + 8), devid);
        }
        p += 8 + ((clen + 1) & ~1UL);
    }
}

static void RefreshConfig(struct BtActionData *data)
{
    IPTR oldpos = 0, saved = 0, curr = 0;
    UBYTE *buf;

    set(data->cfglist, MUIA_List_Quiet, TRUE);
    get(data->cfglist, MUIA_List_Active, &oldpos);
    DoMethod(data->cfglist, MUIM_List_Clear);
    FREELIST(data->cfgentries);
    if((buf = btWriteCfg(NULL))) {
        if(RdBE(buf) == ID_FORM) WalkCfgForm(data, buf, 0, 0, NULL);
        btFreeVec(buf);
    }
    set(data->cfglist, MUIA_List_Active, oldpos);
    set(data->cfglist, MUIA_List_Quiet, FALSE);
    btGetAttrs(BGA_STACK, NULL, BSA_CurrConfigHash, &curr, BSA_SavedConfigHash, &saved, TAG_END);
    set(data->bt_save, MUIA_Disabled, (curr == saved));
}

static struct CfgEntry *SelectedCfg(struct BtActionData *data)
{
    struct CfgEntry *e = NULL;
    DoMethod(data->cfglist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
    return e;
}

/* the library's form for a list entry, or NULL */
static APTR CfgFormFor(struct CfgEntry *e)
{
    APTR pic;
    switch(e->formid) {
        case IFFFORM_BTSTACKCFG:
            return btFindCfgForm(NULL, IFFFORM_BTSTACKCFG);
        case IFFFORM_BTDEVICECFG:
            for(pic = btFindCfgForm(NULL, IFFFORM_BTDEVICECFG); pic; pic = btNextCfgForm(pic))
                if(btMatchStringChunk(pic, IFFCHNK_DEVID, e->devid)) return pic;
            return NULL;
        case IFFFORM_BTCLASSCFG:
            return btGetClsCfg(e->owner);
        case IFFFORM_BTDEVCFGDATA:
            return btGetDevCfg(e->owner, e->devid, NULL);
    }
    return NULL;
}

static void DoCfgActive(struct BtActionData *data)
{
    struct CfgEntry *e = SelectedCfg(data);
    BOOL canremove = FALSE, canexport = FALSE;
    if(e) {
        switch(e->formid) {
            case IFFFORM_BTSTACKCFG:  canexport = TRUE; break;
            case IFFFORM_BTDEVICECFG:
            case IFFFORM_BTCLASSCFG:
            case IFFFORM_BTDEVCFGDATA: canexport = canremove = TRUE; break;
        }
    }
    set(data->bt_cfgremove, MUIA_Disabled, !canremove);
    set(data->bt_cfgexport, MUIA_Disabled, !canexport);
}

static void DoCfgRemove(struct BtActionData *data)
{
    struct CfgEntry *e = SelectedCfg(data);
    APTR pic;
    if(!e) return;
    if(e->formid == IFFFORM_BTDEVICECFG) {
        APTR bd;
        if(!MUI_Request(_app(data->cfglist), _win(data->cfglist), 0, NULL, "Remove|Cancel",
                        "Remove the stored configuration of device\n%s?\n\nIts registration and pairing keys are dropped;\nit has to be paired again to be used.", e->desc))
            return;
        /* a live device object must let go of the registration too, or the
           stack writes the record straight back */
        btLockReadBase();
        bd = btFindDevice(NULL, BDA_IDString, (IPTR)e->devid, TAG_END);
        btUnlockBase();
        if(bd) { btUnregisterDevice(bd); btUnpairDevice(bd); }
    } else if(!MUI_Request(_app(data->cfglist), _win(data->cfglist), 0, NULL, "Remove|Cancel",
                           "Remove the configuration entry\n%s?", e->desc)) {
        return;
    }
    if((pic = CfgFormFor(e))) {
        btRemCfgForm(pic);
        SetStatus(data, "Configuration entry removed.");
    } else {
        SetStatus(data, "Could not find that configuration entry.");
    }
    RefreshConfig(data);
    RefreshDevices(data);
}

static void DoCfgExport(struct BtActionData *data)
{
    struct CfgEntry *e = SelectedCfg(data);
    struct FileRequester *aslreq;
    struct TagItem asltags[] = {
        { ASLFR_InitialDrawer, (IPTR) "SYS:" },
        { ASLFR_InitialFile,   (IPTR) "bluetooth-export.prefs" },
        { ASLFR_DoSaveMode,    (IPTR) TRUE },
        { ASLFR_TitleText,     (IPTR) "Export configuration entry" },
        { TAG_END,             (IPTR) NULL }
    };
    APTR pic;
    UBYTE *buf;
    char path[256];
    BPTR fh;

    if(!e || !(pic = CfgFormFor(e))) { SetStatus(data, "Select an entry to export."); return; }
    if(!(aslreq = (struct FileRequester *) MUI_AllocAslRequest(ASL_FileRequest, asltags))) return;
    if(MUI_AslRequest(aslreq, TAG_END)) {
        strncpy(path, aslreq->fr_Drawer, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
        AddPart((STRPTR) path, aslreq->fr_File, sizeof(path));
        if((buf = btWriteCfg(pic))) {
            if((fh = Open((STRPTR) path, MODE_NEWFILE))) {
                Write(fh, buf, (RdBE(buf + 4) + 9) & ~1UL);
                Close(fh);
                SetStatus(data, "Configuration entry exported.");
            } else {
                SetStatus(data, "Could not open the file for writing.");
            }
            btFreeVec(buf);
        }
    }
    MUI_FreeAslRequest(aslreq);
}

static void DoCfgImport(struct BtActionData *data)
{
    struct FileRequester *aslreq;
    struct TagItem asltags[] = {
        { ASLFR_InitialDrawer, (IPTR) "SYS:" },
        { ASLFR_TitleText,     (IPTR) "Import configuration" },
        { TAG_END,             (IPTR) NULL }
    };
    char path[256];
    BPTR fh;

    if(!(aslreq = (struct FileRequester *) MUI_AllocAslRequest(ASL_FileRequest, asltags))) return;
    if(MUI_AslRequest(aslreq, TAG_END)) {
        strncpy(path, aslreq->fr_Drawer, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
        AddPart((STRPTR) path, aslreq->fr_File, sizeof(path));
        if((fh = Open((STRPTR) path, MODE_OLDFILE))) {
            UBYTE head[12];
            if((Read(fh, head, 12) == 12) && (RdBE(head) == ID_FORM)) {
                ULONG len = RdBE(head + 4);
                UBYTE *buf = AllocVec(len + 8, MEMF_ANY);
                if(buf) {
                    memcpy(buf, head, 12);
                    if(Read(fh, buf + 12, len - 4) == (LONG)(len - 4)) {
                        switch(RdBE(head + 8)) {
                            case IFFFORM_BTCFG:
                                /* a whole prefs file: replaces the running config */
                                if(btReadCfg(NULL, buf)) { btParseCfg(); SetStatus(data, "Configuration imported."); }
                                else SetStatus(data, "The file could not be read as a Bluetooth configuration.");
                                break;
                            case IFFFORM_BTDEVICECFG:
                            case IFFFORM_BTCLASSCFG:
                                if(btAddCfgEntry(NULL, buf)) SetStatus(data, "Configuration entry imported.");
                                else SetStatus(data, "The entry could not be added.");
                                break;
                            default:
                                SetStatus(data, "Not a Bluetooth configuration, device or class entry.");
                                break;
                        }
                    } else {
                        SetStatus(data, "The file is truncated.");
                    }
                    FreeVec(buf);
                }
            } else {
                SetStatus(data, "Not an IFF configuration file.");
            }
            Close(fh);
        } else {
            SetStatus(data, "Could not open that file.");
        }
    }
    MUI_FreeAslRequest(aslreq);
    RefreshConfig(data);
    RefreshDevices(data);
}

/* *** options page ***
 * The gadgets mirror the stack's global config (BGA_STACKCFG); every change
 * is pushed straight into the library (in memory), Save/Use write it out,
 * as in Trident. */

static void LoadOptions(struct BtActionData *data)
{
    IPTR disc = 0, conn = 0, autoc = 0, popp = 0, li = 0, lw = 0, le = 0, lf = 0;
    IPTR dtime = 12, pnew = 0, pgone = 0, pdelay = 5, pact = 0, pfront = 0;
    STRPTR lname = NULL;

    btGetAttrs(BGA_STACKCFG, NULL,
               BGCA_Discoverable, &disc, BGCA_Connectable, &conn, BGCA_AutoConnect, &autoc,
               BGCA_PopupPairing, &popp, BGCA_LogInfo, &li, BGCA_LogWarning, &lw,
               BGCA_LogError, &le, BGCA_LogFailure, &lf, BGCA_DiscoveryTime, &dtime,
               BGCA_PopupDeviceNew, &pnew, BGCA_PopupDeviceGone, &pgone, BGCA_PopupCloseDelay, &pdelay,
               BGCA_PopupActivateWin, &pact, BGCA_PopupWinToFront, &pfront, BGCA_LocalName, &lname,
               TAG_END);
    data->optloading = TRUE;
    nnset(data->opt_discoverable, MUIA_Selected, disc ? TRUE : FALSE);
    nnset(data->opt_connectable,  MUIA_Selected, conn ? TRUE : FALSE);
    nnset(data->opt_autoconnect,  MUIA_Selected, autoc ? TRUE : FALSE);
    nnset(data->opt_popuppairing, MUIA_Selected, popp ? TRUE : FALSE);
    nnset(data->opt_loginfo, MUIA_Selected, li ? TRUE : FALSE);
    nnset(data->opt_logwarn, MUIA_Selected, lw ? TRUE : FALSE);
    nnset(data->opt_logerr,  MUIA_Selected, le ? TRUE : FALSE);
    nnset(data->opt_logfail, MUIA_Selected, lf ? TRUE : FALSE);
    nnset(data->opt_disctime, MUIA_Numeric_Value, dtime);
    nnset(data->opt_popupnew, MUIA_Cycle_Active, (pnew <= BGCP_ALWAYS) ? pnew : 0);
    nnset(data->opt_popupgone, MUIA_Selected, pgone ? TRUE : FALSE);
    nnset(data->opt_popupdelay, MUIA_Numeric_Value, pdelay);
    nnset(data->opt_popupactivate, MUIA_Selected, pact ? TRUE : FALSE);
    nnset(data->opt_popuptofront, MUIA_Selected, pfront ? TRUE : FALSE);
    nnset(data->opt_localname, MUIA_String_Contents, (IPTR)(lname ? lname : (STRPTR)""));
    data->optloading = FALSE;
}

static void ApplyOptions(struct BtActionData *data)
{
    IPTR disc = 0, conn = 0, autoc = 0, popp = 0, li = 0, lw = 0, le = 0, lf = 0;
    IPTR dtime = 12, pnew = 0, pgone = 0, pdelay = 5, pact = 0, pfront = 0;
    STRPTR lname = NULL;
    struct List *hwl;
    struct Node *bth;

    if(data->optloading) return;
    get(data->opt_discoverable, MUIA_Selected, &disc);
    get(data->opt_connectable,  MUIA_Selected, &conn);
    get(data->opt_autoconnect,  MUIA_Selected, &autoc);
    get(data->opt_popuppairing, MUIA_Selected, &popp);
    get(data->opt_loginfo, MUIA_Selected, &li);
    get(data->opt_logwarn, MUIA_Selected, &lw);
    get(data->opt_logerr,  MUIA_Selected, &le);
    get(data->opt_logfail, MUIA_Selected, &lf);
    get(data->opt_disctime, MUIA_Numeric_Value, &dtime);
    get(data->opt_popupnew, MUIA_Cycle_Active, &pnew);
    get(data->opt_popupgone, MUIA_Selected, &pgone);
    get(data->opt_popupdelay, MUIA_Numeric_Value, &pdelay);
    get(data->opt_popupactivate, MUIA_Selected, &pact);
    get(data->opt_popuptofront, MUIA_Selected, &pfront);
    get(data->opt_localname, MUIA_String_Contents, &lname);
    btSetAttrs(BGA_STACKCFG, NULL,
               BGCA_Discoverable, disc, BGCA_Connectable, conn, BGCA_AutoConnect, autoc,
               BGCA_PopupPairing, popp, BGCA_LogInfo, li, BGCA_LogWarning, lw,
               BGCA_LogError, le, BGCA_LogFailure, lf, BGCA_DiscoveryTime, dtime,
               BGCA_PopupDeviceNew, pnew, BGCA_PopupDeviceGone, pgone, BGCA_PopupCloseDelay, pdelay,
               BGCA_PopupActivateWin, pact, BGCA_PopupWinToFront, pfront,
               BGCA_LocalName, (IPTR)((lname && lname[0]) ? lname : NULL),
               TAG_END);
    /* the scan modes take effect on the radios right away */
    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &hwl, TAG_END);
    for(bth = hwl->lh_Head; bth->ln_Succ; bth = bth->ln_Succ) {
        btSetAttrs(BGA_HARDWARE, bth, BHA_Discoverable, disc, BHA_Connectable, conn, TAG_END);
    }
    btUnlockBase();
    RefreshConfig(data);
}

/* *** actions *** */

static void DoForget(struct BtActionData *data)
{
    APTR bd = SelectedDevice(data);
    if(bd) { btUnregisterDevice(bd); btUnpairDevice(bd); btFreeDevice(bd); RefreshDevices(data); SetStatus(data, "Device forgotten."); }
}
static void DoFlush(struct BtActionData *data)
{
    struct List *el;
    btLockWriteBase();
    btGetAttrs(BGA_STACK, NULL, BSA_ErrorMsgList, &el, TAG_END);
    while(el->lh_Head->ln_Succ) btRemErrorMsg(el->lh_Head);
    btUnlockBase();
    RefreshErrors(data);
}
/* Save the whole message log to a file the user picks (default the boot volume,
   so it lands on the USB key). Mirrors Trident's "Save Log". */
static void DoSaveLog(struct BtActionData *data)
{
    struct FileRequester *aslreq;
    struct TagItem asltags[] = {
        { ASLFR_InitialDrawer, (IPTR) "SYS:" },
        { ASLFR_InitialFile,   (IPTR) "BluetoothLog.txt" },
        { ASLFR_DoSaveMode,    (IPTR) TRUE },
        { ASLFR_TitleText,     (IPTR) "Save Bluetooth message log" },
        { TAG_END,             (IPTR) NULL }
    };
    char path[256];
    BPTR fh;

    if((aslreq = (struct FileRequester *) MUI_AllocAslRequest(ASL_FileRequest, asltags)))
    {
        if(MUI_AslRequest(aslreq, TAG_END))
        {
            strncpy(path, aslreq->fr_Drawer, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
            AddPart((STRPTR) path, aslreq->fr_File, sizeof(path));
            if((fh = Open((STRPTR) path, MODE_NEWFILE)))
            {
                struct List *el;
                struct Node *bem;
                char line[400];
                btLockReadBase();
                btGetAttrs(BGA_STACK, NULL, BSA_ErrorMsgList, &el, TAG_END);
                for(bem = el->lh_Head; bem->ln_Succ; bem = bem->ln_Succ)
                {
                    IPTR level = 0;
                    STRPTR origin = NULL, msg = NULL;
                    btGetAttrs(BGA_ERRORMSG, bem, BEMA_Level, &level, BEMA_Origin, &origin, BEMA_Msg, &msg, TAG_END);
                    snprintf(line, sizeof(line), "%ld %s: %s\n", (long) level, origin ? origin : "?", msg ? msg : "");
                    FPuts(fh, (STRPTR) line);
                }
                btUnlockBase();
                Close(fh);
                SetStatus(data, "Message log saved.");
            } else {
                SetStatus(data, "Could not open the log file for writing.");
            }
        }
        MUI_FreeAslRequest(aslreq);
    }
}
static void DoPairReply(struct BtActionData *data, BOOL yes)
{
    if(data->pairdev) {
        if(data->pairtype == BPRT_PASSKEYENTRY) btPairingReply(data->pairdev, BPRA_Passkey, 0, TAG_END);
        else btPairingReply(data->pairdev, BPRA_Confirm, yes, TAG_END);
    }
    data->pairdev = NULL;
    set(data->pairwin, MUIA_Window_Open, FALSE);
}

/* *** live events *** */

static void HandleEvents(struct BtActionData *data)
{
    struct Message *m;
    while((m = GetMsg(data->eventport))) {
        IPTR ev = 0;
        APTR p1 = NULL, p2 = NULL;
        btGetAttrs(BGA_EVENTNOTE, m, BENA_EventID, &ev, BENA_Param1, &p1, BENA_Param2, &p2, TAG_END);
        switch(ev) {
            case BEHMB_ADDHARDWARE: case BEHMB_REMHARDWARE:
                RefreshHardware(data); RefreshDevices(data); break;
            case BEHMB_ADDDEVICE: case BEHMB_REMDEVICE: case BEHMB_DEVICEUPDATE:
            case BEHMB_DEVICEREGISTERED: case BEHMB_DEVICEUNREGISTERED:
            case BEHMB_DEVICECONNECTED: case BEHMB_DEVICEDISCONNECTED:
            case BEHMB_DEVICEDEAD:
                RefreshDevices(data); break;
            case BEHMB_SERVICESCHG:
                if(data->devwin) DoMethod(data->devwin, MUIM_DevWin_Populate); break;
            case BEHMB_ADDCLASS: case BEHMB_REMCLASS: case BEHMB_ADDBINDING: case BEHMB_REMBINDING:
                RefreshClasses(data); RefreshDevices(data); break;
            case BEHMB_ADDERRORMSG:
                RefreshErrors(data); break;
            case BEHMB_CONFIGCHG:
                /* the stack's config changed (a pairing was stored, options
                 * set elsewhere, a save): mirror it */
                LoadOptions(data); RefreshConfig(data); break;
            case BEHMB_DISCOVERYSTART: SetStatus(data, "Discovering..."); RefreshHardware(data); break;
            case BEHMB_DISCOVERYSTOP: SetStatus(data, "Discovery finished."); RefreshHardware(data); break;
            case BEHMB_PAIRINGREQUEST: {
                RefreshDevices(data);
                STRPTR name = NULL;
                IPTR passkey = 0;
                char buf[160];
                if(!data->pairwin) break;   /* pairing popup not built yet */
                btLockReadBase();
                btGetAttrs(BGA_DEVICE, p1, BDA_Name, &name, BDA_PairingPasskey, &passkey, TAG_END);
                data->pairdev = p1;
                data->pairtype = (ULONG)(IPTR)p2;
                switch(data->pairtype) {
                    case BPRT_NUMERICCOMPARE:
                        snprintf(buf, sizeof(buf), "Pair with '%s'?\nConfirm the code matches: %06lu", name ? name : "device", (unsigned long)passkey);
                        break;
                    case BPRT_PASSKEYDISPLAY:
                        snprintf(buf, sizeof(buf), "Enter this passkey on '%s':\n%06lu", name ? name : "device", (unsigned long)passkey);
                        break;
                    case BPRT_CONSENT:
                    default:
                        snprintf(buf, sizeof(buf), "Accept pairing with '%s'?", name ? name : "device");
                        break;
                }
                btUnlockBase();
                set(data->pairtext, MUIA_Text_Contents, (IPTR)buf);
                set(data->pairwin, MUIA_Window_Open, TRUE);
                break;
            }
            case BEHMB_PAIRINGDONE:
                if(data->pairwin) set(data->pairwin, MUIA_Window_Open, FALSE);
                data->pairdev = NULL;
                RefreshDevices(data);
                if((IPTR)p2) {
                    SetStatus(data, "Pairing failed.");
                } else {
                    /* a device paired successfully: it now belongs on the
                     * Devices page, so dismiss the Add Device window. */
                    if(data->scanwin) set(data->scanwin, MUIA_Window_Open, FALSE);
                    SetStatus(data, "Pairing complete.");
                }
                RefreshDevices(data);
                break;
        }
        ReplyMsg(m);
    }
}

/* *** GUI helpers *** */

/* an icon-capable list (IconListClass) wrapped in a listview */
#define LISTVIEW(store, hookp, fmt) \
    ListviewObject, MUIA_Listview_List, (store = NewObject(IconListClass->mcc_Class, NULL, \
        InputListFrame, MUIA_List_MinLineHeight, 18, \
        MUIA_List_Format, (IPTR)(fmt), MUIA_List_Title, TRUE, \
        MUIA_List_DisplayHook, (IPTR)(hookp), TAG_END)), End

static void InitHook(struct Hook *h, APTR func, APTR d)
{
    h->h_Entry = (APTR)func;
    h->h_SubEntry = NULL;
    h->h_Data = d;
}

/* /// "mNew()" */
static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct BtActionData *data;
    Object *navlst, *mainobj;
    ULONG i;

    struct TagItem tags[] = { { TAG_MORE, (IPTR)msg->ops_AttrList } };
    struct opSet supermsg = { OM_NEW, tags, msg->ops_GInfo };

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)&supermsg);
    if(!obj) return 0;

    data = INST_DATA(cl, obj);
    memset(data, 0, sizeof(*data));

    NewList((struct List *)&data->hwentries);
    NewList((struct List *)&data->deventries);
    NewList((struct List *)&data->clsentries);
    NewList((struct List *)&data->errentries);
    NewList((struct List *)&data->cfgentries);

    InitHook(&data->navhook, (APTR)NavDisplay, data);
    InitHook(&data->hwhook,  (APTR)HWDisplay,  data);
    InitHook(&data->devhook, (APTR)DevDisplay, data);
    InitHook(&data->clshook, (APTR)ClsDisplay, data);
    InitHook(&data->errhook, (APTR)ErrDisplay, data);
    InitHook(&data->cfghook, (APTR)CfgDisplay, data);

    data->scanwin = NewObject(ScanWinClass->mcc_Class, NULL, TAG_END);
    data->devwin  = NewObject(DevWinClass->mcc_Class, NULL, TAG_END);

    /* the left navigation list (drives the page group below) */
    navlst = NewObject(IconListClass->mcc_Class, NULL,
                InputListFrame,
                MUIA_List_MinLineHeight, 18,
                MUIA_List_DisplayHook, &data->navhook,
                TAG_END);
    data->navlst = navlst;

    mainobj = VGroup,
        Child, HGroup,
            /* left: navigation column */
            Child, VGroup,
                MUIA_HorizWeight, 28,
                Child, ListviewObject,
                    MUIA_Listview_List, navlst,
                    End,
                End,
            /* draggable divider between the navigation column and the pages */
            Child, BalanceObject, End,
            /* right: one page per navigation entry */
            Child, data->pagegrp = VGroup,
                MUIA_HorizWeight, 72,
                MUIA_Group_PageMode, TRUE,
                /* --- General --- */
                Child, VGroup,
                    Child, VSpace(0),
                    Child, HGroup,
                        Child, HSpace(0),
                        Child, BodychunkObject,
                            MUIA_FixWidth,               BLUZING_WIDTH,
                            MUIA_FixHeight,              BLUZING_HEIGHT,
                            MUIA_Bitmap_Width,           BLUZING_WIDTH,
                            MUIA_Bitmap_Height,          BLUZING_HEIGHT,
                            MUIA_Bodychunk_Depth,        BLUZING_DEPTH,
                            MUIA_Bodychunk_Body,         (IPTR)bluzing_body,
                            MUIA_Bodychunk_Compression,  0,
                            MUIA_Bodychunk_Masking,      2,
                            MUIA_Bitmap_Transparent,     BLUZING_TRANSPARENT,
                            MUIA_Bitmap_SourceColors,    (IPTR)bluzing_colors,
                            MUIA_Bitmap_UseFriend,       TRUE,
                            End,
                        Child, HSpace(0),
                        End,
                    Child, VSpace(6),
                    Child, CLabel("An AROS front end for bluetooth.library."),
                    Child, CLabel("Inspect the radios, devices and classes, and change"),
                    Child, CLabel("the stack options, from the pages on the left."),
                    Child, VSpace(0),
                    End,
                /* --- Hardware --- */
                Child, VGroup,
                    Child, Label("Bluetooth radios in the system:"),
                    Child, LISTVIEW(data->hwlist, &data->hwhook, "BAR,BAR,BAR,BAR,"),
                    Child, HGroup,
                        Child, Label("Driver:"),
                        Child, PopaslObject,
                            MUIA_Popstring_String, data->hwdevobj = StringObject,
                                StringFrame,
                                MUIA_String_AdvanceOnCR, TRUE,
                                End,
                            MUIA_Popstring_Button, PopButton(MUII_PopFile),
                            ASLFR_TitleText, (IPTR)"Select a Bluetooth HCI device driver",
                            ASLFR_InitialDrawer, (IPTR)"DEVS:",
                            End,
                        Child, Label("Unit:"),
                        Child, data->hwunitobj = StringObject,
                            StringFrame,
                            MUIA_HorizWeight, 8,
                            MUIA_String_AdvanceOnCR, TRUE,
                            MUIA_String_Integer, 0,
                            MUIA_String_Accept, (IPTR)"0123456789",
                            End,
                        End,
                    Child, HGroup, MUIA_Group_SameWidth, TRUE,
                        Child, data->bt_hwadd = SimpleButton("Add"),
                        Child, data->bt_hwremove = SimpleButton("Remove"),
                        Child, data->hwinfoobj = SimpleButton("Information"),
                        End,
                    End,
                /* --- Devices --- */
                Child, VGroup,
                    Child, Label("Your Bluetooth devices:"),
                    Child, LISTVIEW(data->devlist, &data->devhook, "BAR,BAR,BAR,"),
                    Child, HGroup, MUIA_Group_SameWidth, TRUE,
                        Child, data->bt_adddev = SimpleButton("Add Device"),
                        Child, data->bt_connect = SimpleButton("Connect"),
                        Child, data->bt_disconnect = SimpleButton("Disconnect"),
                        Child, data->bt_info = SimpleButton("Information"),
                        Child, data->bt_devsettings = SimpleButton("Settings"),
                        Child, data->bt_forget = SimpleButton("Forget"),
                        End,
                    End,
                /* --- Classes --- */
                Child, VGroup,
                    Child, Label("Installed Bluetooth classes:"),
                    Child, LISTVIEW(data->clslist, &data->clshook, "BAR,BAR,"),
                    Child, HGroup,
                        Child, data->bt_clsscan = SimpleButton("Class Scan"),
                        Child, data->bt_clscfg = SimpleButton("Configure"),
                        Child, HSpace(0),
                        End,
                    End,
                /* --- Options --- */
                Child, VGroup,
                    Child, HGroup,
                        Child, VGroup, GroupFrameT("Radio"),
                            Child, ColGroup(2),
                                Child, data->opt_discoverable = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Discoverable"),
                                Child, data->opt_connectable  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Connectable"),
                                Child, data->opt_autoconnect  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Reconnect bonded devices automatically"),
                                End,
                            Child, ColGroup(2),
                                Child, Label("Local name:"),
                                Child, data->opt_localname = StringObject, StringFrame, MUIA_String_MaxLen, 63, MUIA_String_AdvanceOnCR, TRUE, End,
                                Child, Label("Discovery time:"),
                                Child, data->opt_disctime = SliderObject, MUIA_Numeric_Min, 5, MUIA_Numeric_Max, 60, MUIA_Numeric_Format, "%ld s", End,
                                End,
                            End,
                        Child, VGroup, GroupFrameT("Logging"),
                            Child, ColGroup(2),
                                Child, data->opt_loginfo = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Information"),
                                Child, data->opt_logwarn = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Warnings"),
                                Child, data->opt_logerr  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Errors"),
                                Child, data->opt_logfail = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Failures"),
                                End,
                            Child, VSpace(0),
                            End,
                        End,
                    Child, VGroup, GroupFrameT("Pop-ups"),
                        Child, ColGroup(2),
                            Child, data->opt_popuppairing  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Ask on pairing requests"),
                            Child, data->opt_popupgone     = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Announce devices going away"),
                            Child, data->opt_popupactivate = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Activate the pop-up window"),
                            Child, data->opt_popuptofront  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Bring the pop-up to the front on changes"),
                            End,
                        Child, ColGroup(2),
                            Child, Label("New device:"),
                            Child, data->opt_popupnew = CycleObject, MUIA_Cycle_Entries, popupnewstrings, End,
                            Child, Label("Close after:"),
                            Child, data->opt_popupdelay = SliderObject, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 30, MUIA_Numeric_Format, "%ld s", End,
                            End,
                        End,
                    Child, VSpace(0),
                    End,
                /* --- Config --- */
                Child, VGroup,
                    Child, Label("Configuration held by the stack (Save writes it to ENVARC:Sys/bluetooth.prefs):"),
                    Child, ListviewObject,
                        MUIA_Listview_List, (data->cfglist = ListObject,
                            InputListFrame,
                            MUIA_List_Format, "BAR,BAR,BAR,",
                            MUIA_List_Title, TRUE,
                            MUIA_List_DisplayHook, &data->cfghook,
                            End),
                        End,
                    Child, HGroup, MUIA_Group_SameWidth, TRUE,
                        Child, data->bt_cfgexport = SimpleButton("Export..."),
                        Child, data->bt_cfgimport = SimpleButton("Import..."),
                        Child, data->bt_cfgremove = SimpleButton("Remove"),
                        End,
                    End,
                End,
            End,

        /* --- always visible message log --- */
        Child, BalanceObject, End,
        Child, VGroup, GroupFrameT("Message Log"),
            MUIA_VertWeight, 25,
            Child, HGroup,
                Child, Label("Information level:"),
                Child, data->errlvl = CycleObject,
                    MUIA_Cycle_Entries, errlvlstrings,
                    MUIA_Cycle_Active, 3,
                    End,
                Child, HSpace(0),
                Child, data->bt_savelog = SimpleButton("Save log..."),
                Child, data->bt_flush = SimpleButton("Flush all"),
                End,
            Child, data->errlist = ListviewObject,
                MUIA_Listview_Input, FALSE,
                MUIA_Listview_List, ListObject,
                    ReadListFrame,
                    MUIA_List_Format, "BAR,BAR,",
                    MUIA_List_DisplayHook, &data->errhook,
                    End,
                End,
            End,

        /* --- status line + bottom button bar --- */
        Child, HGroup,
            Child, data->statustxt = TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_Text_Contents, "Ready.",
                MUIA_Text_PreParse, "\33l",
                End,
            Child, data->bt_allon = SimpleButton("All Online"),
            Child, data->bt_alloff = SimpleButton("All Offline"),
            Child, data->bt_restart = SimpleButton("Restart"),
            Child, HSpace(0),
            Child, data->bt_save = SimpleButton("Save"),
            Child, data->bt_use = SimpleButton("Use"),
            End,
        End;

    if(!mainobj)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    DoMethod(obj, OM_ADDMEMBER, mainobj);

    /* populate the navigation list */
    set(navlst, MUIA_List_Quiet, TRUE);
    for(i = 0; i < BTPAGE_COUNT; i++)
        DoMethod(navlst, MUIM_List_InsertSingle, &naventries[i], MUIV_List_Insert_Bottom);
    set(navlst, MUIA_List_Quiet, FALSE);
    set(navlst, MUIA_List_Active, 0);

    /* navigation switches the page group */
    DoMethod(navlst, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             data->pagegrp, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);

    /* double click a device -> open its information window */
    DoMethod(data->devlist, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, obj, 1, MUIM_BtA_DevInfo);

    /* action buttons */
    DoMethod(data->bt_adddev,     MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_AddDevice);
    DoMethod(data->bt_connect,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_Connect);
    DoMethod(data->bt_disconnect, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_Disconnect);
    DoMethod(data->bt_info,       MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_DevInfo);
    DoMethod(data->bt_forget,     MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_Forget);
    DoMethod(data->bt_clsscan,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_ClsScan);
    /* class/binding settings windows (enabled per selection) */
    DoMethod(data->bt_devsettings, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_DevSettings);
    DoMethod(data->bt_clscfg,     MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_ClsConfigure);
    DoMethod(data->devlist,       MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MUIM_BtA_DevActive);
    DoMethod(data->clslist,       MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MUIM_BtA_ClsActive);
    set(data->bt_devsettings, MUIA_Disabled, TRUE);
    set(data->bt_clscfg, MUIA_Disabled, TRUE);

    DoMethod(data->bt_hwadd,      MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_HwAdd);
    DoMethod(data->bt_hwremove,   MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_HwRemove);
    /* pressing Return in the unit field adds the radio too */
    DoMethod(data->hwunitobj,     MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 1, MUIM_BtA_HwAdd);

    DoMethod(data->bt_savelog,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_SaveLog);
    DoMethod(data->bt_flush,      MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_FlushLog);
    DoMethod(data->bt_allon,      MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_AllOnline);
    DoMethod(data->bt_alloff,     MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_AllOffline);
    DoMethod(data->bt_restart,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_Restart);
    DoMethod(data->bt_save,       MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_Save);
    DoMethod(data->bt_use,        MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_Use);

    /* options: any change goes straight into the stack's global config */
    {
        Object *checks[] = { data->opt_discoverable, data->opt_connectable, data->opt_autoconnect,
                             data->opt_popuppairing, data->opt_popupgone, data->opt_popupactivate,
                             data->opt_popuptofront, data->opt_loginfo, data->opt_logwarn,
                             data->opt_logerr, data->opt_logfail, NULL };
        for(i = 0; checks[i]; i++)
            DoMethod(checks[i], MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 1, MUIM_BtA_OptChanged);
    }
    DoMethod(data->opt_localname,  MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 1, MUIM_BtA_OptChanged);
    DoMethod(data->opt_disctime,   MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, obj, 1, MUIM_BtA_OptChanged);
    DoMethod(data->opt_popupdelay, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, obj, 1, MUIM_BtA_OptChanged);
    DoMethod(data->opt_popupnew,   MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, obj, 1, MUIM_BtA_OptChanged);

    /* config page */
    DoMethod(data->cfglist,       MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MUIM_BtA_CfgActive);
    DoMethod(data->bt_cfgexport,  MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_CfgExport);
    DoMethod(data->bt_cfgimport,  MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_CfgImport);
    DoMethod(data->bt_cfgremove,  MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_CfgRemove);
    set(data->bt_cfgexport, MUIA_Disabled, TRUE);
    set(data->bt_cfgremove, MUIA_Disabled, TRUE);

    return (IPTR)obj;
}
/* \\\ */

/* /// "ActionDispatcher()" */
AROS_UFH3(IPTR, ActionDispatcher,
          AROS_UFHA(struct IClass *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
    struct BtActionData *data;

    if(msg->MethodID == OM_NEW)
        return mNew(cl, obj, (struct opSet *)msg);

    data = INST_DATA(cl, obj);

    switch(msg->MethodID)
    {
        case MUIM_BtA_Register:    { APTR b = SelectedDevice(data); if(b && btRegisterDevice(b)) SetStatus(data,"Device registered."); } return 0;
        case MUIM_BtA_Unregister:  { APTR b = SelectedDevice(data); if(b && btUnregisterDevice(b)) SetStatus(data,"Device unregistered."); } return 0;
        case MUIM_BtA_Pair:        { APTR b = SelectedDevice(data); if(b && btPairDevice(b, TAG_END)) SetStatus(data,"Pairing started..."); else SetStatus(data,"Pairing failed to start."); } return 0;
        case MUIM_BtA_Connect:     { APTR b = SelectedDevice(data); SetStatus(data,"Connecting..."); if(b) { if(btConnectDevice(b)) SetStatus(data,"Connected."); else SetStatus(data,"Connection failed."); } } return 0;
        case MUIM_BtA_Disconnect:  { APTR b = SelectedDevice(data); if(b) btDisconnectDevice(b); SetStatus(data,"Disconnected."); } return 0;
        case MUIM_BtA_Forget:      DoForget(data); return 0;
        case MUIM_BtA_ClsScan:     btClassScan(); SetStatus(data,"Class scan requested."); return 0;
        case MUIM_BtA_HwAdd:       DoHwAdd(data); return 0;
        case MUIM_BtA_HwRemove:    DoHwRemove(data); return 0;
        case MUIM_BtA_AddDevice:
            if(data->scanwin) { DoMethod(data->scanwin, MUIM_ScanWin_Refresh); set(data->scanwin, MUIA_Window_Open, TRUE); SetStatus(data,"Scanning for devices..."); }
            return 0;
        case MUIM_BtA_DevInfo:
            { APTR b = SelectedDevice(data); if(b && data->devwin) DoMethod(data->devwin, MUIM_DevWin_Show, b); }
            return 0;
        case MUIM_BtA_FlushLog:    DoFlush(data); return 0;
        case MUIM_BtA_SaveLog:     DoSaveLog(data); return 0;
        case MUIM_BtA_Save:
            /* ENVARC: and ENV:, like Trident's Save */
            if(btSaveCfgToDisk(NULL, FALSE)) SetStatus(data,"Configuration saved to ENVARC:Sys/bluetooth.prefs."); else SetStatus(data,"Saving failed.");
            RefreshConfig(data);
            return 0;
        case MUIM_BtA_Use:
            /* ENV: only: in use until the next reboot */
            if(btSaveCfgToDisk("ENV:Sys/bluetooth.prefs", FALSE)) SetStatus(data,"Configuration in use (ENV: only, not saved permanently)."); else SetStatus(data,"Writing ENV:Sys/bluetooth.prefs failed.");
            RefreshConfig(data);
            return 0;
        case MUIM_BtA_OptChanged:  ApplyOptions(data); return 0;
        case MUIM_BtA_DevActive:   UpdateDevButtons(data); return 0;
        case MUIM_BtA_ClsActive:   UpdateClsButtons(data); return 0;
        case MUIM_BtA_DevSettings: DoDevSettings(data); return 0;
        case MUIM_BtA_ClsConfigure: DoClsConfigure(data); return 0;
        case MUIM_BtA_CfgActive:   DoCfgActive(data); return 0;
        case MUIM_BtA_CfgExport:   DoCfgExport(data); return 0;
        case MUIM_BtA_CfgImport:   DoCfgImport(data); return 0;
        case MUIM_BtA_CfgRemove:   DoCfgRemove(data); return 0;
        case MUIM_BtA_AllOnline:   RefreshHardware(data); return 0;
        case MUIM_BtA_AllOffline:  RefreshHardware(data); return 0;
        case MUIM_BtA_Restart:     RefreshHardware(data); RefreshDevices(data); RefreshClasses(data); RefreshErrors(data); SetStatus(data,"Refreshed."); return 0;
        case MUIM_BtA_HandleEvents: HandleEvents(data); return 0;
        case MUIM_BtA_PairReply:   DoPairReply(data, ((struct MUIP_BtA_Reply *)msg)->yes); return 0;

        case MUIM_Setup:
            if(!DoSuperMethodA(cl, obj, msg)) return FALSE;
            if(data->scanwin) DoMethod(_app(obj), OM_ADDMEMBER, data->scanwin);
            if(data->devwin)  DoMethod(_app(obj), OM_ADDMEMBER, data->devwin);
            if((data->eventport = CreateMsgPort())) {
                data->eventhandler = btAddEventHandler(data->eventport, ~0);
                data->ihnode.ihn_Object = obj;
                data->ihnode.ihn_Method = MUIM_BtA_HandleEvents;
                data->ihnode.ihn_Signals = 1UL << data->eventport->mp_SigBit;
                data->ihnode.ihn_Flags = 0;
                DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->ihnode);
                data->ihadded = TRUE;
            }
            RefreshHardware(data);
            RefreshDevices(data);
            RefreshClasses(data);
            RefreshErrors(data);
            LoadOptions(data);
            RefreshConfig(data);
            return TRUE;

        case MUIM_Cleanup:
            if(data->ihadded) {
                DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->ihnode);
                data->ihadded = FALSE;
            }
            if(data->eventhandler) { btRemEventHandler(data->eventhandler); data->eventhandler = NULL; }
            if(data->eventport) { DeleteMsgPort(data->eventport); data->eventport = NULL; }
            if(data->scanwin) DoMethod(_app(obj), OM_REMMEMBER, data->scanwin);
            if(data->devwin)  DoMethod(_app(obj), OM_REMMEMBER, data->devwin);
            break;

        case OM_DISPOSE:
            if(data->scanwin) { MUI_DisposeObject(data->scanwin); data->scanwin = NULL; }
            if(data->devwin)  { MUI_DisposeObject(data->devwin);  data->devwin = NULL; }
            FREELIST(data->hwentries);
            FREELIST(data->deventries);
            FREELIST(data->clsentries);
            FREELIST(data->errentries);
            FREELIST(data->cfgentries);
            break;
    }
    return DoSuperMethodA(cl, obj, msg);
    AROS_USERFUNC_EXIT
}
/* \\\ */
