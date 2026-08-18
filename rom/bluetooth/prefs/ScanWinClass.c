/*
** ScanWinClass - the "Add Device" window. See ScanWinClass.h.
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
#include "ActionClass.h"     /* struct DevEntry */
#include "ScanWinClass.h"
#include "debug.h"

#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#define FREELIST(l) { struct MinNode *n; while((n = (struct MinNode *)RemHead((struct List *)&(l)))) FreeVec(n); }

/* display hook for the discovery list */
AROS_UFH3(LONG, ScanDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct DevEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    static char rssibuf[12];
    if(e) {
        if(e->rssi != 127) snprintf(rssibuf, sizeof(rssibuf), "%ld", (long)e->rssi); else strcpy(rssibuf, "-");
        *a++ = e->addr; *a++ = e->name; *a++ = e->type; *a = rssibuf;
    } else { *a++ = "Address"; *a++ = "Name"; *a++ = "Type"; *a = "RSSI"; }
    return 0;
    AROS_USERFUNC_EXIT
}
static struct Hook scanhook = { { NULL, NULL }, (APTR)ScanDisplay };

static APTR DefaultRadio(void)
{
    struct List *l;
    btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &l, TAG_END);
    if(l->lh_Head->ln_Succ) return l->lh_Head;
    return NULL;
}

/* /// "Populate()" */
static void Populate(struct ScanWinData *data)
{
    struct List *hwl, *devl;
    struct Node *bth, *bd;

    set(data->list, MUIA_List_Quiet, TRUE);
    DoMethod(data->list, MUIM_List_Clear);
    FREELIST(data->entries);

    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &hwl, TAG_END);
    for(bth = hwl->lh_Head; bth->ln_Succ; bth = bth->ln_Succ) {
        btGetAttrs(BGA_HARDWARE, bth, BHA_DeviceList, &devl, TAG_END);
        for(bd = devl->lh_Head; bd->ln_Succ; bd = bd->ln_Succ) {
            struct DevEntry *e;
            STRPTR name = NULL, addr = NULL;
            IPTR isc = 0, isl = 0, isreg = 0, isb = 0, isconn = 0;
            LONG rssi = 127;
            btGetAttrs(BGA_DEVICE, bd, BDA_Name, &name, BDA_AddressString, &addr, BDA_RSSI, &rssi,
                       BDA_IsClassic, &isc, BDA_IsLE, &isl, BDA_IsRegistered, &isreg,
                       BDA_IsBonded, &isb, BDA_IsConnected, &isconn, TAG_END);
            /* only devices that are not yet known/connected */
            if(isreg || isb || isconn) continue;
            if(!(e = AllocVec(sizeof(struct DevEntry), MEMF_CLEAR))) break;
            e->bd = bd;
            e->rssi = rssi;
            strncpy(e->addr, addr ? addr : "?", sizeof(e->addr)-1);
            strncpy(e->name, name ? name : "?", sizeof(e->name)-1);
            strcpy(e->type, (isc && isl) ? "dual" : (isl ? "LE" : "BR/EDR"));
            AddTail((struct List *)&data->entries, (struct Node *)e);
            DoMethod(data->list, MUIM_List_InsertSingle, e, MUIV_List_Insert_Bottom);
        }
    }
    btUnlockBase();
    set(data->list, MUIA_List_Quiet, FALSE);
}
/* \\\ */

/* /// "mNew()" */
static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct ScanWinData *data;
    Object *list, *refreshbtn, *connectbtn, *contents;

    list = ListviewObject,
        MUIA_Listview_List, ListObject,
            InputListFrame,
            MUIA_List_Format, "BAR,BAR,BAR,",
            MUIA_List_Title, TRUE,
            MUIA_List_DisplayHook, &scanhook,
            End,
        End;

    contents = VGroup,
        Child, Label("Discovered devices:"),
        Child, list,
        Child, HGroup,
            MUIA_Group_SameWidth, TRUE,
            Child, refreshbtn = SimpleButton("Refresh"),
            Child, HSpace(0),
            Child, connectbtn = SimpleButton("Connect"),
            End,
        End;

    {
        struct TagItem tags[] = {
            { MUIA_Window_Title,     (IPTR)"Add Bluetooth Device" },
            { MUIA_Window_ID,        MAKE_ID('B','T','A','D') },
            { MUIA_Window_Width,     MUIV_Window_Width_MinMax(30) },
            { MUIA_Window_Height,    MUIV_Window_Height_MinMax(40) },
            { WindowContents,        (IPTR)contents },
            { TAG_MORE,              (IPTR)msg->ops_AttrList },
        };
        struct opSet supermsg = { OM_NEW, tags, msg->ops_GInfo };
        obj = (Object *)DoSuperMethodA(cl, obj, (Msg)&supermsg);
    }
    if(!obj) return 0;

    data = INST_DATA(cl, obj);
    memset(data, 0, sizeof(*data));
    data->list = list;
    data->refreshbtn = refreshbtn;
    data->connectbtn = connectbtn;
    NewList((struct List *)&data->entries);

    /* close gadget just cancels */
    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);
    DoMethod(refreshbtn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScanWin_Refresh);
    DoMethod(connectbtn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScanWin_Connect);
    DoMethod(list, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, obj, 1, MUIM_ScanWin_Connect);

    return (IPTR)obj;
}
/* \\\ */

/* /// "ScanWinDispatcher()" */
AROS_UFH3(IPTR, ScanWinDispatcher,
          AROS_UFHA(struct IClass *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
    struct ScanWinData *data;

    if(msg->MethodID == OM_NEW)
        return mNew(cl, obj, (struct opSet *)msg);

    data = INST_DATA(cl, obj);

    switch(msg->MethodID)
    {
        case MUIM_ScanWin_Refresh:
            data->radio = DefaultRadio();
            if(data->radio) btStartDiscovery(data->radio, BDSA_Duration, 10, TAG_END);
            Populate(data);
            return 0;

        case MUIM_ScanWin_Populate:
            Populate(data);
            return 0;

        case MUIM_ScanWin_Connect:
            {
                struct DevEntry *e = NULL;
                DoMethod(data->list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
                if(e && e->bd) btConnectDevice(e->bd);
                Populate(data);
            }
            return 0;

        case OM_DISPOSE:
            FREELIST(data->entries);
            break;
    }
    return DoSuperMethodA(cl, obj, msg);
    AROS_USERFUNC_EXIT
}
/* \\\ */
