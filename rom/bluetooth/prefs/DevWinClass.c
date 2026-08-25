/*
** DevWinClass - per-device information window. See DevWinClass.h.
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
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/bluetooth.h>
#include <proto/btclass.h>

#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>

#include "bluetoothprefs.h"
#include "ActionClass.h"     /* struct SvcEntry */
#include "DevWinClass.h"
#include "IconListClass.h"   /* ICONLIST_IMAGES */
#include "icons.h"
#include "debug.h"

#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#define FREELIST(l) { struct MinNode *n; while((n = (struct MinNode *)RemHead((struct List *)&(l)))) FreeVec(n); }

AROS_UFH3(LONG, SvcDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct SvcEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    static char bindbuf[64];
    if(e) {
        *a++ = e->name; *a++ = e->type; *a++ = e->uuid; *a++ = e->proto;
        if(e->binding[0] && strcmp(e->binding, "-") && h->h_Data) {
            /* show the bound class with its icon, like Trident's bindings */
            snprintf(bindbuf, sizeof(bindbuf), "\33O[%08lx] %s",
                     (unsigned long)(IPTR)ICONLIST_IMAGES((Object *)h->h_Data)[ICON_CLASSES], e->binding);
            *a = bindbuf;
        } else {
            *a = e->binding[0] ? e->binding : "-";
        }
    } else {
        *a++ = "Service"; *a++ = "Type"; *a++ = "UUID"; *a++ = "Protocol"; *a = "Binding";
    }
    return 0;
    AROS_USERFUNC_EXIT
}
static struct Hook svchook = { { NULL, NULL }, (APTR)SvcDisplay };

/* /// "Populate()" */
static void Populate(struct DevWinData *data)
{
    struct List *svcl;
    struct Node *bsv;
    STRPTR name = NULL, addr = NULL, origname = NULL;
    IPTR isc = 0, isl = 0, isreg = 0, isb = 0, isconn = 0, isdead = 0, keys = 0;
    IPTR inhibitpopup = 0, noclassbind = 0, autoconnect = 0, trusted = 0;
    APTR devbindcls = NULL;
    STRPTR devbindname = NULL;
    char buf[96];

    if(!data->device) return;

    btLockReadBase();
    btGetAttrs(BGA_DEVICE, data->device, BDA_Name, &name, BDA_OrigName, &origname, BDA_AddressString, &addr,
               BDA_IsClassic, &isc, BDA_IsLE, &isl, BDA_IsRegistered, &isreg, BDA_IsBonded, &isb,
               BDA_IsConnected, &isconn, BDA_IsDead, &isdead, BDA_BindingClass, &devbindcls,
               BDA_BondFlags, &keys, BDA_InhibitPopup, &inhibitpopup, BDA_InhibitClassBind, &noclassbind,
               BDA_AutoConnect, &autoconnect, BDA_Trusted, &trusted, TAG_END);
    if(devbindcls) btGetAttrs(BGA_BTCLASS, devbindcls, BCA_ClassName, &devbindname, TAG_END);
    set(data->bindtxt, MUIA_Text_Contents, (IPTR)(devbindname ? devbindname : "-"));

    set(data->nametxt, MUIA_Text_Contents, (IPTR)(name ? name : "?"));
    set(data->addrtxt, MUIA_Text_Contents, (IPTR)(addr ? addr : "?"));
    set(data->typetxt, MUIA_Text_Contents, (IPTR)((isc && isl) ? "Dual mode (BR/EDR + LE)" : (isl ? "Low Energy" : "BR/EDR")));
    snprintf(buf, sizeof(buf), "%s%s%s%s", isconn ? "connected " : "", isreg ? "registered " : "",
             isb ? "bonded " : "", isdead ? "unreachable " : "");
    if(!buf[0]) strcpy(buf, "-");
    set(data->statetxt, MUIA_Text_Contents, (IPTR)buf);
    /* which keys the bond holds (never the keys themselves) */
    snprintf(buf, sizeof(buf), "%s%s%s%s%s", (keys & BDKF_LINKKEY) ? "BR/EDR link key " : "",
             (keys & BDKF_LTK) ? ((keys & BDKF_SC) ? "LE key (Secure Connections) " : "LE key (legacy) ") : "",
             (keys & BDKF_IRK) ? "IRK " : "", (keys & BDKF_CSRK) ? "CSRK " : "",
             keys ? "" : "none");
    set(data->keystxt, MUIA_Text_Contents, (IPTR)buf);

    /* settings: only the gadgets, not the stack (see MUIM_DevWin_SettingChg) */
    data->loading = TRUE;
    nnset(data->cwname, MUIA_String_Contents, (IPTR)(name ? name : (STRPTR)""));
    nnset(data->chk_inhibitpopup, MUIA_Selected, inhibitpopup ? TRUE : FALSE);
    nnset(data->chk_noclassbind,  MUIA_Selected, noclassbind ? TRUE : FALSE);
    nnset(data->chk_autoconnect,  MUIA_Selected, autoconnect ? TRUE : FALSE);
    nnset(data->chk_trusted,      MUIA_Selected, trusted ? TRUE : FALSE);
    set(data->resetnamebtn, MUIA_Disabled, !(origname && name && strcmp(origname, name)));
    data->loading = FALSE;

    set(data->svclist, MUIA_List_Quiet, TRUE);
    DoMethod(data->svclist, MUIM_List_Clear);
    FREELIST(data->svcentries);
    btGetAttrs(BGA_DEVICE, data->device, BDA_ServiceList, &svcl, TAG_END);
    for(bsv = svcl->lh_Head; bsv->ln_Succ; bsv = bsv->ln_Succ) {
        struct SvcEntry *e;
        STRPTR sname = NULL, bindname = NULL;
        APTR bindcls = NULL;
        IPTR uuid16 = 0, proto = 0, psm = 0, chan = 0, sh = 0, eh = 0;
        if(!(e = AllocVec(sizeof(struct SvcEntry), MEMF_CLEAR))) break;
        btGetAttrs(BGA_SERVICE, bsv, BSVA_Name, &sname, BSVA_UUID16, &uuid16, BSVA_Protocol, &proto,
                   BSVA_PSM, &psm, BSVA_RFCOMMChannel, &chan, BSVA_StartHandle, &sh, BSVA_EndHandle, &eh,
                   BSVA_BindingClass, &bindcls, TAG_END);
        if(bindcls) btGetAttrs(BGA_BTCLASS, bindcls, BCA_ClassName, &bindname, TAG_END);
        strncpy(e->binding, bindname ? bindname : "-", sizeof(e->binding)-1);
        e->bsv = bsv;
        strncpy(e->name, sname ? sname : "?", sizeof(e->name)-1);
        snprintf(e->uuid, sizeof(e->uuid), "0x%04lx", (unsigned long)uuid16);
        switch(proto) {
            case BSVP_L2CAP: snprintf(e->proto, sizeof(e->proto), "L2CAP PSM 0x%04lx", (unsigned long)psm); break;
            case BSVP_RFCOMM: snprintf(e->proto, sizeof(e->proto), "RFCOMM ch %ld", (long)chan); break;
            case BSVP_ATT: snprintf(e->proto, sizeof(e->proto), "GATT 0x%04lx-0x%04lx", (unsigned long)sh, (unsigned long)eh); break;
            default: strcpy(e->proto, "-"); break;
        }
        /* the bearer the service lives on: GATT/ATT is LE, everything else BR/EDR */
        strcpy(e->type, (proto == BSVP_ATT) ? "LE" : "BR/EDR");
        AddTail((struct List *)&data->svcentries, (struct Node *)e);
        DoMethod(data->svclist, MUIM_List_InsertSingle, e, MUIV_List_Insert_Bottom);
    }
    btUnlockBase();
    set(data->svclist, MUIA_List_Quiet, FALSE);
}
/* \\\ */

/* /// "mNew()" */
static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct DevWinData *data;
    Object *nametxt, *addrtxt, *typetxt, *statetxt, *bindtxt, *keystxt, *svclist, *rescanbtn, *cfgbtn, *contents;
    Object *cwname, *setnamebtn, *resetnamebtn, *chk_inhibitpopup, *chk_noclassbind, *chk_autoconnect, *chk_trusted;

    contents = VGroup,
        Child, ColGroup(2), GroupFrameT("Device"),
            Child, Label("Name:"),        Child, nametxt  = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            Child, Label("Address:"),     Child, addrtxt  = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            Child, Label("Type:"),        Child, typetxt  = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            Child, Label("Status:"),      Child, statetxt = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            Child, Label("Bound class:"), Child, bindtxt  = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            Child, Label("Stored keys:"), Child, keystxt  = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            End,
        /* the persistent per-device settings, kept in the device's config
           form (Save/Use in the main window writes them to disk) */
        Child, VGroup, GroupFrameT("Settings"),
            Child, HGroup,
                Child, Label("Custom name:"),
                Child, cwname = StringObject, StringFrame, MUIA_String_MaxLen, 63, MUIA_String_AdvanceOnCR, TRUE, End,
                Child, setnamebtn = SimpleButton("Set"),
                Child, resetnamebtn = SimpleButton("Reset"),
                End,
            /* checkmarks and labels are fixed size: the spacer lets the group
               (and so the window) be resized wider than its contents */
            Child, HGroup,
                Child, ColGroup(4),
                    Child, chk_autoconnect  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Auto-reconnect"),
                    Child, chk_trusted      = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("Trusted (may connect to us)"),
                    Child, chk_noclassbind  = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("No class binding"),
                    Child, chk_inhibitpopup = MUI_MakeObject(MUIO_Checkmark, NULL), Child, LLabel("No pop-ups"),
                    End,
                Child, HSpace(0),
                End,
            End,
        Child, VGroup, GroupFrameT("Services and their bindings"),
            Child, ListviewObject,
                MUIA_Listview_List, (svclist = NewObject(IconListClass->mcc_Class, NULL,
                    InputListFrame,
                    MUIA_List_MinLineHeight, 18,
                    MUIA_List_Format, "BAR,BAR,BAR,BAR,",
                    MUIA_List_Title, TRUE,
                    MUIA_List_DisplayHook, &svchook,
                    TAG_END)),
                End,
            End,
        Child, HGroup,
            Child, rescanbtn = SimpleButton("Rescan services"),
            Child, cfgbtn = SimpleButton("Configure"),
            Child, HSpace(0),
            End,
        End;

    {
        struct TagItem tags[] = {
            { MUIA_Window_Title,  (IPTR)"Device Information" },
            { MUIA_Window_ID,     MAKE_ID('B','T','D','I') },
            { MUIA_Window_Width,  MUIV_Window_Width_MinMax(30) },
            { MUIA_Window_Height, MUIV_Window_Height_MinMax(40) },
            { WindowContents,     (IPTR)contents },
            { TAG_MORE,           (IPTR)msg->ops_AttrList },
        };
        struct opSet supermsg = { OM_NEW, tags, msg->ops_GInfo };
        obj = (Object *)DoSuperMethodA(cl, obj, (Msg)&supermsg);
    }
    if(!obj) return 0;

    data = INST_DATA(cl, obj);
    memset(data, 0, sizeof(*data));
    data->nametxt = nametxt;
    data->addrtxt = addrtxt;
    data->typetxt = typetxt;
    data->statetxt = statetxt;
    data->bindtxt = bindtxt;
    data->keystxt = keystxt;
    data->svclist = svclist;
    data->rescanbtn = rescanbtn;
    data->cfgbtn = cfgbtn;
    set(cfgbtn, MUIA_Disabled, TRUE);
    data->cwname = cwname;
    data->setnamebtn = setnamebtn;
    data->resetnamebtn = resetnamebtn;
    data->chk_inhibitpopup = chk_inhibitpopup;
    data->chk_noclassbind = chk_noclassbind;
    data->chk_autoconnect = chk_autoconnect;
    data->chk_trusted = chk_trusted;
    NewList((struct List *)&data->svcentries);

    /* let the services display hook reach this list's icon images */
    svchook.h_Data = svclist;

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);
    DoMethod(rescanbtn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_DevWin_Rescan);
    DoMethod(cfgbtn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_DevWin_Configure);
    DoMethod(svclist, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MUIM_DevWin_SvcActive);
    DoMethod(setnamebtn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_DevWin_SetName);
    DoMethod(cwname, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 1, MUIM_DevWin_SetName);
    DoMethod(resetnamebtn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_DevWin_ResetName);
    DoMethod(chk_inhibitpopup, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 1, MUIM_DevWin_SettingChg);
    DoMethod(chk_noclassbind,  MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 1, MUIM_DevWin_SettingChg);
    DoMethod(chk_autoconnect,  MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 1, MUIM_DevWin_SettingChg);
    DoMethod(chk_trusted,      MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 1, MUIM_DevWin_SettingChg);

    return (IPTR)obj;
}
/* \\\ */

/* /// "DevWinDispatcher()" */
AROS_UFH3(IPTR, DevWinDispatcher,
          AROS_UFHA(struct IClass *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
    struct DevWinData *data;

    if(msg->MethodID == OM_NEW)
        return mNew(cl, obj, (struct opSet *)msg);

    data = INST_DATA(cl, obj);

    switch(msg->MethodID)
    {
        case MUIM_DevWin_Show:
            data->device = ((struct MUIP_DevWin_Show *)msg)->device;
            Populate(data);
            DoMethod(obj, MUIM_DevWin_SvcActive);
            set(obj, MUIA_Window_Open, TRUE);
            return 0;

        case MUIM_DevWin_Populate:
            Populate(data);
            DoMethod(obj, MUIM_DevWin_SvcActive);
            return 0;

        case MUIM_DevWin_Rescan:
            if(data->device) btEnumerateServices(data->device);
            Populate(data);
            return 0;

        case MUIM_DevWin_SvcActive:
        case MUIM_DevWin_Configure: {
            /* the selected service's binding (or, if none is selected, the
               device binding): does its class offer a settings window, open it */
            struct SvcEntry *e = NULL;
            APTR binding = NULL, bc = NULL;
            struct Library *BtClsBase = NULL;
            IPTR has = FALSE;
            if(!data->device) return 0;
            DoMethod(data->svclist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
            btLockReadBase();
            if(e) btGetAttrs(BGA_SERVICE, e->bsv, BSVA_Binding, &binding, BSVA_BindingClass, &bc, TAG_END);
            if(!binding) btGetAttrs(BGA_DEVICE, data->device, BDA_Binding, &binding, BDA_BindingClass, &bc, TAG_END);
            if(binding && bc) btGetAttrs(BGA_BTCLASS, bc, BCA_ClassBase, &BtClsBase, TAG_END);
            btUnlockBase();
            if(BtClsBase) btcGetAttrs(BCGA_CLASS, NULL, BCCA_HasBindingCfgGUI, &has, TAG_END);
            if(msg->MethodID == MUIM_DevWin_SvcActive) {
                set(data->cfgbtn, MUIA_Disabled, !has);
            } else if(has) {
                btcDoMethod(BCM_OpenBindingCfgWindow, binding);
            }
            return 0;
        }

        case MUIM_DevWin_SettingChg: {
            IPTR inhibitpopup = 0, noclassbind = 0, autoconnect = 0, trusted = 0;
            if(data->loading || !data->device) return 0;
            get(data->chk_inhibitpopup, MUIA_Selected, &inhibitpopup);
            get(data->chk_noclassbind,  MUIA_Selected, &noclassbind);
            get(data->chk_autoconnect,  MUIA_Selected, &autoconnect);
            get(data->chk_trusted,      MUIA_Selected, &trusted);
            /* into the device's config form (in memory); Save/Use persists it */
            btSetAttrs(BGA_DEVICE, data->device,
                       BDA_InhibitPopup, inhibitpopup, BDA_InhibitClassBind, noclassbind,
                       BDA_AutoConnect, autoconnect, BDA_Trusted, trusted, TAG_END);
            return 0;
        }

        case MUIM_DevWin_SetName: {
            STRPTR newname = NULL;
            if(data->loading || !data->device) return 0;
            get(data->cwname, MUIA_String_Contents, &newname);
            if(newname && newname[0]) {
                btSetAttrs(BGA_DEVICE, data->device, BDA_Name, (IPTR)newname, TAG_END);
                Populate(data);
            }
            return 0;
        }

        case MUIM_DevWin_ResetName: {
            STRPTR origname = NULL;
            if(!data->device) return 0;
            btGetAttrs(BGA_DEVICE, data->device, BDA_OrigName, &origname, TAG_END);
            if(origname && origname[0]) {
                btSetAttrs(BGA_DEVICE, data->device, BDA_Name, (IPTR)origname, TAG_END);
                Populate(data);
            }
            return 0;
        }

        case OM_DISPOSE:
            FREELIST(data->svcentries);
            break;
    }
    return DoSuperMethodA(cl, obj, msg);
    AROS_USERFUNC_EXIT
}
/* \\\ */
