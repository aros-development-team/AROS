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
#include <libraries/bluetooth.h>

#define MUIMASTER_YES_INLINE_STDARG
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/bluetooth.h>

#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>

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
};

static const char *errlvlstrings[] = { "Failures", "Errors", "Warnings", "All messages", NULL };

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

static void RefreshDevices(struct BtActionData *data)
{
    struct List *hwl, *devl;
    struct Node *bth, *bd;

    set(data->devlist, MUIA_List_Quiet, TRUE);
    DoMethod(data->devlist, MUIM_List_Clear);
    FREELIST(data->deventries);
    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &hwl, TAG_END);
    for(bth = hwl->lh_Head; bth->ln_Succ; bth = bth->ln_Succ) {
        btGetAttrs(BGA_HARDWARE, bth, BHA_DeviceList, &devl, TAG_END);
        for(bd = devl->lh_Head; bd->ln_Succ; bd = bd->ln_Succ) {
            struct DevEntry *e = AllocVec(sizeof(struct DevEntry), MEMF_CLEAR);
            STRPTR name = NULL, addr = NULL;
            IPTR isc = 0, isl = 0, isreg = 0, isb = 0, isconn = 0, isdead = 0, cod = 0, appear = 0;
            LONG rssi = 127;
            if(!e) break;
            btGetAttrs(BGA_DEVICE, bd, BDA_Name, &name, BDA_AddressString, &addr, BDA_RSSI, &rssi,
                       BDA_IsClassic, &isc, BDA_IsLE, &isl, BDA_IsRegistered, &isreg, BDA_IsBonded, &isb,
                       BDA_IsConnected, &isconn, BDA_IsDead, &isdead,
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
            snprintf(e->flags, sizeof(e->flags), "%s%s%s%s",
                     isreg ? "registered " : "", isb ? "bonded " : "",
                     isconn ? "connected " : "", isdead ? "unreachable " : "");
            if(!e->flags[0]) strcpy(e->flags, "discovered");
            AddTail((struct List *)&data->deventries, (struct Node *)e);
            DoMethod(data->devlist, MUIM_List_InsertSingle, e, MUIV_List_Insert_Bottom);
        }
    }
    btUnlockBase();
    set(data->devlist, MUIA_List_Quiet, FALSE);
    if(data->devwin) DoMethod(data->devwin, MUIM_DevWin_Populate);
    if(data->scanwin) DoMethod(data->scanwin, MUIM_ScanWin_Populate);
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
            case BEHMB_DISCOVERYSTART: SetStatus(data, "Discovering..."); RefreshHardware(data); break;
            case BEHMB_DISCOVERYSTOP: SetStatus(data, "Discovery finished."); RefreshHardware(data); break;
            case BEHMB_PAIRINGREQUEST: {
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

    InitHook(&data->navhook, (APTR)NavDisplay, data);
    InitHook(&data->hwhook,  (APTR)HWDisplay,  data);
    InitHook(&data->devhook, (APTR)DevDisplay, data);
    InitHook(&data->clshook, (APTR)ClsDisplay, data);
    InitHook(&data->errhook, (APTR)ErrDisplay, data);

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
                        Child, data->hwinfoobj = SimpleButton("Information"),
                        Child, HSpace(0),
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
                        Child, data->bt_forget = SimpleButton("Forget"),
                        End,
                    End,
                /* --- Classes --- */
                Child, VGroup,
                    Child, Label("Installed Bluetooth classes:"),
                    Child, LISTVIEW(data->clslist, &data->clshook, "BAR,BAR,"),
                    Child, HGroup,
                        Child, data->bt_clsscan = SimpleButton("Class Scan"),
                        Child, HSpace(0),
                        End,
                    End,
                /* --- Options --- */
                Child, VGroup,
                    Child, VGroup, GroupFrameT("Radio"),
                        Child, ColGroup(2),
                            Child, data->opt_discoverable = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Discoverable"),
                            Child, data->opt_connectable  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Connectable"),
                            Child, data->opt_autoconnect  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Auto-connect known devices"),
                            Child, data->opt_popuppairing = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Show pairing pop-ups"),
                            End,
                        End,
                    Child, VGroup, GroupFrameT("Logging"),
                        Child, ColGroup(2),
                            Child, data->opt_loginfo = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Information"),
                            Child, data->opt_logwarn = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Warnings"),
                            Child, data->opt_logerr  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Errors"),
                            Child, data->opt_logfail = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Failures"),
                            End,
                        End,
                    Child, VSpace(0),
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

    DoMethod(data->bt_flush,      MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_FlushLog);
    DoMethod(data->bt_allon,      MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_AllOnline);
    DoMethod(data->bt_alloff,     MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_AllOffline);
    DoMethod(data->bt_restart,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_Restart);
    DoMethod(data->bt_save,       MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_Save);
    DoMethod(data->bt_use,        MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_BtA_Use);

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
        case MUIM_BtA_AddDevice:
            if(data->scanwin) { DoMethod(data->scanwin, MUIM_ScanWin_Refresh); set(data->scanwin, MUIA_Window_Open, TRUE); SetStatus(data,"Scanning for devices..."); }
            return 0;
        case MUIM_BtA_DevInfo:
            { APTR b = SelectedDevice(data); if(b && data->devwin) DoMethod(data->devwin, MUIM_DevWin_Show, b); }
            return 0;
        case MUIM_BtA_FlushLog:    DoFlush(data); return 0;
        case MUIM_BtA_Save:
        case MUIM_BtA_Use:         if(btSaveCfgToDisk(NULL, FALSE)) SetStatus(data,"Configuration saved."); else SetStatus(data,"Saving failed."); return 0;
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
            break;
    }
    return DoSuperMethodA(cl, obj, msg);
    AROS_USERFUNC_EXIT
}
/* \\\ */
