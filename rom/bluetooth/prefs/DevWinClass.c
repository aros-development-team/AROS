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

#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>

#include "bluetoothprefs.h"
#include "ActionClass.h"     /* struct SvcEntry */
#include "DevWinClass.h"
#include "debug.h"

#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#define FREELIST(l) { struct MinNode *n; while((n = (struct MinNode *)RemHead((struct List *)&(l)))) FreeVec(n); }

AROS_UFH3(LONG, SvcDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct SvcEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    if(e) { *a++ = e->name; *a++ = e->uuid; *a = e->proto; }
    else  { *a++ = "Service"; *a++ = "UUID"; *a = "Protocol"; }
    return 0;
    AROS_USERFUNC_EXIT
}
static struct Hook svchook = { { NULL, NULL }, (APTR)SvcDisplay };

/* /// "Populate()" */
static void Populate(struct DevWinData *data)
{
    struct List *svcl;
    struct Node *bsv;
    STRPTR name = NULL, addr = NULL;
    IPTR isc = 0, isl = 0, isreg = 0, isb = 0, isconn = 0, isdead = 0;
    char buf[96];

    if(!data->device) return;

    btLockReadBase();
    btGetAttrs(BGA_DEVICE, data->device, BDA_Name, &name, BDA_AddressString, &addr,
               BDA_IsClassic, &isc, BDA_IsLE, &isl, BDA_IsRegistered, &isreg, BDA_IsBonded, &isb,
               BDA_IsConnected, &isconn, BDA_IsDead, &isdead, TAG_END);

    set(data->nametxt, MUIA_Text_Contents, (IPTR)(name ? name : "?"));
    set(data->addrtxt, MUIA_Text_Contents, (IPTR)(addr ? addr : "?"));
    set(data->typetxt, MUIA_Text_Contents, (IPTR)((isc && isl) ? "Dual mode (BR/EDR + LE)" : (isl ? "Low Energy" : "BR/EDR")));
    snprintf(buf, sizeof(buf), "%s%s%s%s", isconn ? "connected " : "", isreg ? "registered " : "",
             isb ? "bonded " : "", isdead ? "unreachable " : "");
    if(!buf[0]) strcpy(buf, "-");
    set(data->statetxt, MUIA_Text_Contents, (IPTR)buf);

    set(data->svclist, MUIA_List_Quiet, TRUE);
    DoMethod(data->svclist, MUIM_List_Clear);
    FREELIST(data->svcentries);
    btGetAttrs(BGA_DEVICE, data->device, BDA_ServiceList, &svcl, TAG_END);
    for(bsv = svcl->lh_Head; bsv->ln_Succ; bsv = bsv->ln_Succ) {
        struct SvcEntry *e;
        STRPTR sname = NULL;
        IPTR uuid16 = 0, proto = 0, psm = 0, chan = 0, sh = 0, eh = 0;
        if(!(e = AllocVec(sizeof(struct SvcEntry), MEMF_CLEAR))) break;
        btGetAttrs(BGA_SERVICE, bsv, BSVA_Name, &sname, BSVA_UUID16, &uuid16, BSVA_Protocol, &proto,
                   BSVA_PSM, &psm, BSVA_RFCOMMChannel, &chan, BSVA_StartHandle, &sh, BSVA_EndHandle, &eh, TAG_END);
        e->bsv = bsv;
        strncpy(e->name, sname ? sname : "?", sizeof(e->name)-1);
        snprintf(e->uuid, sizeof(e->uuid), "0x%04lx", (unsigned long)uuid16);
        switch(proto) {
            case BSVP_L2CAP: snprintf(e->proto, sizeof(e->proto), "L2CAP PSM 0x%04lx", (unsigned long)psm); break;
            case BSVP_RFCOMM: snprintf(e->proto, sizeof(e->proto), "RFCOMM ch %ld", (long)chan); break;
            case BSVP_ATT: snprintf(e->proto, sizeof(e->proto), "GATT 0x%04lx-0x%04lx", (unsigned long)sh, (unsigned long)eh); break;
            default: strcpy(e->proto, "-"); break;
        }
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
    Object *nametxt, *addrtxt, *typetxt, *statetxt, *svclist, *rescanbtn, *contents;

    contents = VGroup,
        Child, ColGroup(2), GroupFrameT("Device"),
            Child, Label("Name:"),    Child, nametxt  = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            Child, Label("Address:"), Child, addrtxt  = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            Child, Label("Type:"),    Child, typetxt  = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            Child, Label("Status:"),  Child, statetxt = TextObject, MUIA_Text_Contents, (IPTR)"", End,
            End,
        Child, VGroup, GroupFrameT("Services"),
            Child, svclist = ListviewObject,
                MUIA_Listview_List, ListObject,
                    InputListFrame,
                    MUIA_List_Format, "BAR,BAR,",
                    MUIA_List_Title, TRUE,
                    MUIA_List_DisplayHook, &svchook,
                    End,
                End,
            End,
        Child, HGroup,
            Child, rescanbtn = SimpleButton("Rescan services"),
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
    data->svclist = svclist;
    data->rescanbtn = rescanbtn;
    NewList((struct List *)&data->svcentries);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);
    DoMethod(rescanbtn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_DevWin_Rescan);

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
            set(obj, MUIA_Window_Open, TRUE);
            return 0;

        case MUIM_DevWin_Populate:
            Populate(data);
            return 0;

        case MUIM_DevWin_Rescan:
            if(data->device) btEnumerateServices(data->device);
            Populate(data);
            return 0;

        case OM_DISPOSE:
            FREELIST(data->svcentries);
            break;
    }
    return DoSuperMethodA(cl, obj, msg);
    AROS_USERFUNC_EXIT
}
/* \\\ */
