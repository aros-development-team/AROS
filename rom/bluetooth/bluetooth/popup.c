/*
** popup.c - a PoPo-style pairing popup handled inside bluetooth.library.
**
** When a pairing request needs user interaction and popups are enabled
** (bgc_PopupPairing), the library spawns a small GUI task that opens
** muimaster.library and shows a Zune window asking the user to confirm the
** pairing, compare a number, or enter a passkey/PIN. The user's answer is fed
** straight back through btPairingReply(). This mirrors how poseidon.library's
** PoPo shows popups for USB from within the library itself.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "debug.h"
#include "bluetooth.library.h"
#include "popupicon.h"

#include <stdio.h>
#include <string.h>

/* the inline muimaster stubs use this base */
#define MUIMasterBase BluetoothBase->bt_Popup.bp_MUIMasterBase

/* one large MUI object tree: relax the gcc 16 tag-list conversions */
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

static ULONG bStrToPasskey(CONST_STRPTR s)
{
    ULONG v = 0;
    if(s) while(*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v;
}

/* /// "bPopupTask()" */
AROS_UFH0(void, bPopupTask)
{
    AROS_USERFUNC_INIT
    struct BtBase *BluetoothBase = (struct BtBase *) FindTask(NULL)->tc_UserData;
    struct BtPopupTask *bp = &BluetoothBase->bt_Popup;
    struct Library *IntuitionBase = NULL;
    struct MsgPort *port = NULL;
    Object *app = NULL, *win = NULL, *bodytxt = NULL, *inputstr = NULL, *acceptbtn = NULL, *rejectbtn = NULL;
    struct BtDevice *curdev = NULL;
    ULONG curtype = BPRT_NONE;

    enum { ID_ACCEPT = 1, ID_REJECT };

    IntuitionBase = OpenLibrary("intuition.library", 39);
    MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN);
    port = CreateMsgPort();

    if(IntuitionBase && MUIMasterBase && port)
    {
        bp->bp_Port = port;

        app = ApplicationObject,
            MUIA_Application_Title,     (IPTR)"Bluetooth",
            MUIA_Application_Base,      (IPTR)"BTPAIRING",
            MUIA_Application_SingleTask, TRUE,
            SubWindow, win = WindowObject,
                MUIA_Window_Title,    (IPTR)"Bluetooth pairing",
                MUIA_Window_ID,       MAKE_ID('B','T','P','P'),
                MUIA_Window_Activate, TRUE,
                WindowContents, VGroup,
                    MUIA_Group_Spacing, 4,
                    Child, HGroup,
                        MUIA_Group_Spacing, 8,
                        Child, VGroup,
                            Child, BodychunkObject,
                                MUIA_FixWidth,              POPUPICON_WIDTH,
                                MUIA_FixHeight,             POPUPICON_HEIGHT,
                                MUIA_Bitmap_Width,          POPUPICON_WIDTH,
                                MUIA_Bitmap_Height,         POPUPICON_HEIGHT,
                                MUIA_Bodychunk_Depth,       POPUPICON_DEPTH,
                                MUIA_Bodychunk_Body,        (IPTR)popupicon_body,
                                MUIA_Bodychunk_Compression, 0,
                                MUIA_Bodychunk_Masking,     2,
                                MUIA_Bitmap_Transparent,    POPUPICON_TRANSPARENT,
                                MUIA_Bitmap_SourceColors,   (IPTR)popupicon_colors,
                                MUIA_Bitmap_UseFriend,      TRUE,
                                End,
                            Child, VSpace(0),
                            End,
                        Child, VGroup, GroupFrameT("Pairing request"),
                            Child, VSpace(0),
                            Child, bodytxt = TextObject,
                                MUIA_Text_PreParse, (IPTR)"\33c",
                                MUIA_Text_Contents, (IPTR)"",
                                End,
                            Child, inputstr = StringObject,
                                StringFrame,
                                MUIA_String_MaxLen, 20,
                                End,
                            Child, VSpace(0),
                            End,
                        End,
                    Child, HGroup,
                        MUIA_Group_SameWidth, TRUE,
                        Child, HSpace(0),
                        Child, acceptbtn = SimpleButton("_Accept"),
                        Child, rejectbtn = SimpleButton("_Reject"),
                        Child, HSpace(0),
                        End,
                    End,
                End,
            End;
    }

    if(app)
    {
        ULONG sigs = 0, portsig = 1UL << port->mp_SigBit;
        BOOL run = TRUE;

        DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, ID_REJECT);
        DoMethod(acceptbtn, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, ID_ACCEPT);
        DoMethod(rejectbtn, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, ID_REJECT);
        DoMethod(inputstr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, app, 2, MUIM_Application_ReturnID, ID_ACCEPT);

        bp->bp_Task = FindTask(NULL);
        Forbid();
        if(bp->bp_ReadySigTask) Signal(bp->bp_ReadySigTask, 1L << bp->bp_ReadySignal);
        Permit();

        while(run)
        {
            ULONG id = DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs);
            struct BtPopupMsg *pm;

            switch(id)
            {
                case MUIV_Application_ReturnID_Quit:
                    run = FALSE;
                    break;

                case ID_ACCEPT:
                case ID_REJECT:
                    if(curdev)
                    {
                        BOOL accept = (id == ID_ACCEPT);
                        struct TagItem tags[2];
                        STRPTR s = NULL;
                        switch(curtype)
                        {
                            case BPRT_PASSKEYENTRY:
                                get(inputstr, MUIA_String_Contents, &s);
                                tags[0].ti_Tag = BPRA_Passkey;
                                tags[0].ti_Data = accept ? bStrToPasskey(s) : 0;
                                break;
                            case BPRT_PINCODE:
                                get(inputstr, MUIA_String_Contents, &s);
                                tags[0].ti_Tag = BPRA_PINCode;
                                tags[0].ti_Data = (IPTR)(accept && s ? s : (STRPTR)"");
                                break;
                            default: /* CONSENT, NUMERICCOMPARE */
                                tags[0].ti_Tag = BPRA_Confirm;
                                tags[0].ti_Data = accept;
                                break;
                        }
                        tags[1].ti_Tag = TAG_END;
                        btPairingReplyA(curdev, tags);
                    }
                    set(win, MUIA_Window_Open, FALSE);
                    curdev = NULL;
                    break;
            }

            if(!run) break;

            while((pm = (struct BtPopupMsg *) GetMsg(port)))
            {
                if(pm->bpm_Type == BPRT_NONE)
                {
                    /* the pairing this popup belongs to has finished (or failed) */
                    if(!curdev || (curdev == pm->bpm_Device))
                    {
                        set(win, MUIA_Window_Open, FALSE);
                        curdev = NULL;
                    }
                    btFreeVec(pm);
                    continue;
                }
                STRPTR name = NULL;
                char buf[200];
                BOOL needinput = FALSE;

                curdev  = pm->bpm_Device;
                curtype = pm->bpm_Type;

                btLockReadBase();
                btGetAttrs(BGA_DEVICE, curdev, BDA_Name, &name, TAG_END);
                switch(curtype)
                {
                    case BPRT_NUMERICCOMPARE:
                        snprintf(buf, sizeof(buf), "Pair with '%s'?\nConfirm this code is shown on the device:\n\33c\33b%06lu\33n",
                                 name ? name : "device", (unsigned long) pm->bpm_Passkey);
                        break;
                    case BPRT_PASSKEYDISPLAY:
                        snprintf(buf, sizeof(buf), "Enter this passkey on '%s':\n\33c\33b%06lu\33n",
                                 name ? name : "device", (unsigned long) pm->bpm_Passkey);
                        break;
                    case BPRT_PASSKEYENTRY:
                        snprintf(buf, sizeof(buf), "Enter the passkey shown on '%s':", name ? name : "device");
                        needinput = TRUE;
                        break;
                    case BPRT_PINCODE:
                        snprintf(buf, sizeof(buf), "Enter the PIN for '%s':", name ? name : "device");
                        needinput = TRUE;
                        break;
                    case BPRT_CONSENT:
                    default:
                        snprintf(buf, sizeof(buf), "Accept pairing with '%s'?", name ? name : "device");
                        break;
                }
                btUnlockBase();

                set(bodytxt, MUIA_Text_Contents, (IPTR)buf);
                /* a displayed passkey is confirmed on the device (Enter), not here */
                set(acceptbtn, MUIA_ShowMe, (curtype != BPRT_PASSKEYDISPLAY));
                set(inputstr, MUIA_ShowMe, needinput);
                set(inputstr, MUIA_String_Contents, (IPTR)"");
                set(win, MUIA_Window_Open, TRUE);
                if(needinput) set(win, MUIA_Window_ActiveObject, (IPTR)inputstr);

                btFreeVec(pm);
            }

            if(sigs)
            {
                sigs = Wait(sigs | portsig | SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C) run = FALSE;
            }
        }

        set(win, MUIA_Window_Open, FALSE);
    }

    /* drain any pending requests so senders don't leak them */
    if(port)
    {
        struct BtPopupMsg *pm;
        bp->bp_Port = NULL;   /* stop new messages arriving here */
        while((pm = (struct BtPopupMsg *) GetMsg(port))) btFreeVec(pm);
    }

    if(app) MUI_DisposeObject(app);
    if(port) DeleteMsgPort(port);
    if(MUIMasterBase) { CloseLibrary(MUIMasterBase); MUIMasterBase = NULL; }
    if(IntuitionBase) CloseLibrary(IntuitionBase);

    Forbid();
    bp->bp_Task = NULL;
    if(bp->bp_ReadySigTask) Signal(bp->bp_ReadySigTask, 1L << bp->bp_ReadySignal);
    /* Permit() is implicit at task exit */

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "bStartPopup()" */
static BOOL bStartPopup(struct BtBase *BluetoothBase)
{
    struct BtPopupTask *bp = &BluetoothBase->bt_Popup;

    ObtainSemaphore(&BluetoothBase->bt_ReentrantLock);
    if(bp->bp_Task || bp->bp_ReadySigTask)   /* running or being started */
    {
        ReleaseSemaphore(&BluetoothBase->bt_ReentrantLock);
        return(bp->bp_Task != NULL);
    }
    bp->bp_ReadySignal = SIGB_SINGLE;
    bp->bp_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    /* Do NOT hold the lock across the wait - the popup task takes library
       locks while starting up (as the event broadcaster does). */
    ReleaseSemaphore(&BluetoothBase->bt_ReentrantLock);

    if(btSpawnSubTask("Bluetooth Pairing Popup", bPopupTask, BluetoothBase))
        Wait(1UL << bp->bp_ReadySignal);

    bp->bp_ReadySigTask = NULL;
    return(bp->bp_Task != NULL);
}
/* \\\ */

/* /// "bShowPairingPopup()" */
void bShowPairingPopup(struct BtBase *BluetoothBase, struct BtDevice *bd, ULONG type, ULONG passkey)
{
    struct BtPopupMsg *pm;

    if(!bd || !BluetoothBase->bt_GlobalCfg->bgc_PopupPairing)
        return;
    if((type == BPRT_NONE) && !BluetoothBase->bt_Popup.bp_Port)
        return;                          /* nothing open to close */
    if(!bStartPopup(BluetoothBase))
        return;

    if((pm = btAllocVec(sizeof(struct BtPopupMsg))))
    {
        pm->bpm_Msg.mn_Node.ln_Type = NT_MESSAGE;
        pm->bpm_Msg.mn_Length       = sizeof(struct BtPopupMsg);
        pm->bpm_Msg.mn_ReplyPort    = NULL;   /* the popup task frees it */
        pm->bpm_Device  = bd;
        pm->bpm_Type    = type;
        pm->bpm_Passkey = passkey;
        Forbid();
        if(BluetoothBase->bt_Popup.bp_Port)
            PutMsg(BluetoothBase->bt_Popup.bp_Port, (struct Message *) pm);
        else
            btFreeVec(pm);
        Permit();
    }
}
/* \\\ */

/* /// "bStopPopup()" */
void bStopPopup(struct BtBase *BluetoothBase)
{
    struct BtPopupTask *bp = &BluetoothBase->bt_Popup;

    if(bp->bp_Task)
    {
        bp->bp_ReadySignal = SIGB_SINGLE;
        bp->bp_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        Signal(bp->bp_Task, SIGBREAKF_CTRL_C);
        while(bp->bp_Task)
            Wait(1UL << bp->bp_ReadySignal);
        bp->bp_ReadySigTask = NULL;
    }
}
/* \\\ */
