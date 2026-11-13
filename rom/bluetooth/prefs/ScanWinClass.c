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
#include <proto/dos.h>
#include <dos/dostags.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/bluetooth.h>

#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>

#include "bluetoothprefs.h"
#include "ActionClass.h"     /* struct DevEntry */
#include "IconListClass.h"   /* ICONLIST_IMAGES */
#include "icons.h"
#include "ScanWinClass.h"
#include "debug.h"

#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#define FREELIST(l) { struct MinNode *n; while((n = (struct MinNode *)RemHead((struct List *)&(l)))) FreeVec(n); }
#define ICONCOL(buf,imgs,idx,text) (snprintf((buf),sizeof(buf),"\33O[%08lx] %s",(unsigned long)(IPTR)(imgs)[idx],(text)),(buf))

/* display hook for the discovery list; h_Data is the inner IconList object */
AROS_UFH3(LONG, ScanDisplay, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(char **, a, A2), AROS_UFHA(struct DevEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    static char rssibuf[12];
    static char addrbuf[64];
    if(e) {
        if(e->rssi != 127) snprintf(rssibuf, sizeof(rssibuf), "%ld", (long)e->rssi); else strcpy(rssibuf, "-");
        *a++ = h->h_Data ? ICONCOL(addrbuf, ICONLIST_IMAGES(h->h_Data), e->icon, e->addr) : e->addr;
        *a++ = e->name; *a = rssibuf;
    } else { *a++ = "Address"; *a++ = "Name"; *a = "RSSI"; }
    return 0;
    AROS_USERFUNC_EXIT
}
static struct Hook scanhook = { { NULL, NULL }, (APTR)ScanDisplay };

/* The connect/pair calls block until the device answers (or times out) and
 * pairing waits on the user, so they must not run on the GUI task. A small
 * helper process does the sequence; success registers the device (the
 * library does that on pairing completion), failure leaves it alone. */
AROS_UFH0(void, ScanConnectProc)
{
    AROS_USERFUNC_INIT
    APTR bd = FindTask(NULL)->tc_UserData;
    struct Library *BluetoothBase = OpenLibrary("bluetooth.library", 1);
    if(BluetoothBase) {
        if(btConnectDevice(bd)) {
            if(!btPairDevice(bd, TAG_END)) {
                btDisconnectDevice(bd);
            }
        }
        CloseLibrary(BluetoothBase);
    }
    AROS_USERFUNC_EXIT
}

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
    struct MinList fresh;

    NewList((struct List *)&fresh);
    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &hwl, TAG_END);
    for(bth = hwl->lh_Head; bth->ln_Succ; bth = bth->ln_Succ) {
        btGetAttrs(BGA_HARDWARE, bth, BHA_DeviceList, &devl, TAG_END);
        for(bd = devl->lh_Head; bd->ln_Succ; bd = bd->ln_Succ) {
            struct DevEntry *e;
            STRPTR name = NULL, addr = NULL;
            IPTR isc = 0, isl = 0, isreg = 0, isb = 0, isconn = 0, cod = 0, appear = 0;
            LONG rssi = 127;
            btGetAttrs(BGA_DEVICE, bd, BDA_Name, &name, BDA_AddressString, &addr, BDA_RSSI, &rssi,
                       BDA_IsClassic, &isc, BDA_IsLE, &isl, BDA_IsRegistered, &isreg,
                       BDA_IsBonded, &isb, BDA_IsConnected, &isconn,
                       BDA_ClassOfDevice, &cod, BDA_Appearance, &appear, TAG_END);
            /* only devices that are not yet known/connected */
            if(isreg || isb || isconn) continue;
            if(!(e = AllocVec(sizeof(struct DevEntry), MEMF_CLEAR))) break;
            e->bd = bd;
            e->icon = DeviceIconFor(cod, appear, isc);
            e->rssi = rssi;
            strncpy(e->addr, addr ? addr : "?", sizeof(e->addr)-1);
            strncpy(e->name, name ? name : "?", sizeof(e->name)-1);
            strcpy(e->type, (isc && isl) ? "dual" : (isl ? "LE" : "BR/EDR"));
            AddTail((struct List *)&fresh, (struct Node *)e);
        }
    }
    btUnlockBase();
    MergeDevList(data->list, &data->entries, &fresh);
}
/* \\\ */

/* /// "mNew()" */
static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct ScanWinData *data;
    Object *list, *innerlist, *refreshbtn, *connectbtn, *contents;

    list = ListviewObject,
        MUIA_Listview_List, innerlist = NewObject(IconListClass->mcc_Class, NULL,
            InputListFrame,
            MUIA_List_MinLineHeight, 18,
            MUIA_List_Format, (IPTR)"BAR,BAR,",
            MUIA_List_Title, TRUE,
            MUIA_List_DisplayHook, (IPTR)&scanhook,
            TAG_END),
        End;
    scanhook.h_Data = innerlist;

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
    data->list = innerlist;
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
                if(data->busy) {
                    return 0;           /* a connect/pair is already running (these calls block) */
                }
                DoMethod(data->list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);
                if(e && e->bd) {
                    /* connect and pair in the background; the device shows up on
                     * the Devices page as it connects, and gets registered by the
                     * library once pairing succeeds. Close now so a key from a
                     * freshly bound device cannot trigger another connect here. */
                    if(CreateNewProcTags(NP_Entry, (IPTR)ScanConnectProc, NP_Name, (IPTR)"Bluetooth pairing",
                                         NP_UserData, (IPTR)e->bd, NP_Priority, 0, TAG_DONE)) {
                        set(obj, MUIA_Window_Open, FALSE);
                        return 0;
                    }
                }
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
