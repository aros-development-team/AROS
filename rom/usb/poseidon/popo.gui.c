/*
 * GUI
 */

#include "debug.h"
#include "poseidon.library.h"

#include <libraries/mui.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <datatypes/soundclass.h>

#define __NOLIBBASE__
#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/usbclass.h>

#include "numtostr.h"

#define NewList NEWLIST

#include "popo.gui.h"

#include <stdarg.h>

#define MUIMasterBase po->po_MUIBase
#define DataTypesBase po->po_DTBase
#define IntuitionBase po->po_IntBase

extern struct ExecBase *SysBase;

/* /// "pPoPoGUITask()" */
AROS_UFH0(void, pPoPoGUITask)
{
    AROS_USERFUNC_INIT
    struct Task *thistask;
    LIBBASETYPEPTR ps;
    struct PsdPoPo *po;
    struct List *scrlist;
    struct PubScreenNode *pubscr;
    BOOL hasworkbench = FALSE;

    thistask = FindTask(NULL);

    ps = thistask->tc_UserData;
    SetTaskPri(thistask, 0);
    po = &ps->ps_PoPo;

    NewList(&po->po_GadgetList);
    NewList(&po->po_Sounds);

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }

    scrlist = LockPubScreenList();
    pubscr = (struct PubScreenNode *) scrlist->lh_Head;
    while(pubscr->psn_Node.ln_Succ)
    {
        if(strcmp(pubscr->psn_Node.ln_Name, "Workbench") == 0)
        {
            if(pubscr->psn_Screen)
            {
                hasworkbench = TRUE;
            }
            break;
        }
        pubscr = (struct PubScreenNode *) pubscr->psn_Node.ln_Succ;
    }
    UnlockPubScreenList();

    if(!hasworkbench)
    {
        KPRINTF(10, ("No screen open yet.\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }

    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }

    if(!(DataTypesBase = OpenLibrary("datatypes.library", 39)))
    {
        KPRINTF(10, ("Couldn't open datatypes.library.\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }

    if(!(po->po_MsgPort = CreateMsgPort()))
    {
        KPRINTF(10, ("Couldn't create MsgPort.\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }

    po->po_EventHandler = psdAddEventHandler(po->po_MsgPort, EHMF_ADDDEVICE|EHMF_REMDEVICE|EHMF_ADDBINDING|EHMF_CONFIGCHG|EHMF_DEVICEDEAD|EHMF_DEVICELOWPW);
    if(!po->po_EventHandler)
    {
        KPRINTF(10, ("Couldn't add EventHandler.\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }

    if(!(po->po_TimerMsgPort = CreateMsgPort()))
    {
        KPRINTF(10, ("Couldn't create MsgPort.\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }

    if(!(po->po_TimerIOReq = (struct timerequest *) CreateIORequest(po->po_TimerMsgPort, sizeof(struct timerequest))))
    {
        KPRINTF(10, ("Couldn't create TimerIOReq.\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }

    if(OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *) po->po_TimerIOReq, 0))
    {
        KPRINTF(10, ("Couldn't open timer.device.\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }
    po->po_TimerIOReq->tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    po->po_TimerIOReq->tr_node.io_Message.mn_Node.ln_Name = "PoPo";
    po->po_TimerIOReq->tr_node.io_Command = TR_ADDREQUEST;

    if(!(po->po_PoPoClass = MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct PoPoData), PoPoDispatcher)))
    {
        KPRINTF(10, ("Couldn't create PoPo Class\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }
    po->po_PoPoClass->mcc_Class->cl_UserData = (IPTR) ps;

    po->po_AppObj = ApplicationObject,
        MUIA_Application_Title      , (IPTR)"PoPo -- Poseidon Popup Provider",
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)"©2004-2009 Chris Hodges",
        MUIA_Application_Author     , (IPTR)"Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"Opens annoying windows",
        MUIA_Application_Base       , (IPTR)"POPO",
        MUIA_Application_HelpFile   , (IPTR)"HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , (IPTR)MenustripObject,
            Child, (IPTR)MenuObjectT((IPTR)"Project"),
                Child, (IPTR)(po->po_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"About...",
                    MUIA_Menuitem_Shortcut, (IPTR)"?",
                    End),
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                    End,
                Child, (IPTR)(po->po_CloseMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Close",
                    MUIA_Menuitem_Shortcut, (IPTR)"Q",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                Child, (IPTR)(po->po_TridentMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Open Trident",
                    MUIA_Menuitem_Shortcut, (IPTR)"O",
                    End),
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                    End,
                Child, (IPTR)(po->po_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR)"M",
                    End),
                End,
            End,

        SubWindow, (IPTR)(po->po_WindowObj = WindowObject,
            MUIA_Window_ID   , MAKE_ID('P','O','P','O'),
            MUIA_Window_Title, (IPTR)"Poseidon Popups",
            MUIA_HelpNode, (IPTR)"main",

            WindowContents, (IPTR)VGroup,
                Child, (IPTR)(po->po_PoPoObj = NewObject(po->po_PoPoClass->mcc_Class, 0, MUIA_ShowMe, FALSE, TAG_END)),
                Child, (IPTR)(po->po_GroupObj = VGroup,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)HSpace(0),
                        Child, (IPTR)Label("Messages"),
                        Child, (IPTR)HSpace(0),
                        End,
                    End),
                Child, (IPTR)HGroup,
                    Child, (IPTR)(po->po_StickyObj = ImageObject, ImageButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_Toggle,
                        MUIA_Image_Spec, MUII_CheckMark,
                        MUIA_Image_FreeVert, TRUE,
                        MUIA_Selected, FALSE,
                        MUIA_ShowSelState, FALSE,
                        End),
                    Child, (IPTR)Label("Hold on"),
                    Child, (IPTR)HGroup,
                        MUIA_Group_SameWidth, TRUE,
                        Child, (IPTR)(po->po_SaveObj = TextObject, ButtonFrame,
                            MUIA_Disabled, (ps->ps_SavedConfigHash == ps->ps_ConfigHash),
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, (IPTR)"\33c Save Prefs ",
                            End),
                        Child, (IPTR)(po->po_CloseObj = TextObject, ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, (IPTR)"\33c Close ",
                            End),
                        End,
                    End,
                End,
            End),
        End;

    if(!po->po_AppObj)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        pPoPoGUITaskCleanup(ps);
        return;
    }

    DoMethod(po->po_WindowObj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             po->po_AppObj, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(po->po_SaveObj, MUIM_Notify, MUIA_Pressed, FALSE,
             po->po_PoPoObj, 1, MUIM_PoPo_SavePrefs);
    DoMethod(po->po_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             po->po_AppObj, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(po->po_StickyObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             po->po_PoPoObj, 1, MUIM_PoPo_Sticky);

    DoMethod(po->po_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             po->po_PoPoObj, 1, MUIM_PoPo_About);
    DoMethod(po->po_CloseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             po->po_AppObj, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(po->po_TridentMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             po->po_PoPoObj, 1, MUIM_PoPo_OpenTrident);
    DoMethod(po->po_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             po->po_AppObj, 2, MUIM_Application_OpenConfigWindow, 0);


    {
        BOOL isopen = FALSE;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;
        struct PsdPoPoGadgets *pog;
        struct timeval currtime;

        po->po_Task = thistask;
        Forbid();
        if(po->po_ReadySigTask)
        {
            Signal(po->po_ReadySigTask, 1L<<po->po_ReadySignal);
        }
        Permit();

        //set(po->po_WindowObj, MUIA_Window_Open, TRUE);
        po->po_TimerIOReq->tr_time.tv_micro = 500*1000;
        SendIO(&po->po_TimerIOReq->tr_node);
        sigmask = (1UL<<po->po_MsgPort->mp_SigBit)|(1UL<<po->po_TimerMsgPort->mp_SigBit);
        do
        {
            retid = DoMethod(po->po_AppObj, MUIM_Application_NewInput, &sigs);
            if(sigs)
            {
                pEventHandler(ps);
                if(CheckIO(&po->po_TimerIOReq->tr_node))
                {
                    WaitIO(&po->po_TimerIOReq->tr_node);
                    po->po_TimerIOReq->tr_time.tv_micro = 500*1000;
                    SendIO(&po->po_TimerIOReq->tr_node);
                }
                CurrentTime(&currtime.tv_secs, &currtime.tv_micro);
                pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
                while(pog->pog_Node.ln_Succ)
                {
                    if((currtime.tv_secs > pog->pog_TimeoutTime.tv_secs) ||
                       ((currtime.tv_secs == pog->pog_TimeoutTime.tv_secs) &&
                        (currtime.tv_micro > pog->pog_TimeoutTime.tv_micro)))
                    {
                        pFreePoPoGadgets(ps, pog);
                        pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
                    } else {
                        pog = (struct PsdPoPoGadgets *) pog->pog_Node.ln_Succ;
                    }
                }

                if(po->po_GadgetList.lh_Head->ln_Succ)
                {
					if(po->po_OpenRequest)
                    {
                        if(!isopen)
                        {
                            if(ps->ps_GlobalCfg->pgc_PopupActivateWin)
                            {
                                set(po->po_WindowObj, MUIA_Window_Activate, TRUE);
                            } else {
                                set(po->po_WindowObj, MUIA_Window_Activate, FALSE);
                            }
                            set(po->po_StickyObj, MUIA_Selected, FALSE);
                            set(po->po_WindowObj, MUIA_Window_Open, TRUE);
                            isopen = TRUE;
                        } else {
                            if(ps->ps_GlobalCfg->pgc_PopupWinToFront)
                            {
                                DoMethod(po->po_WindowObj, MUIM_Window_ToFront);
                            }
                        }
                        po->po_OpenRequest = FALSE;
                    }
                } else {
                    if(isopen)
                    {
                        set(po->po_WindowObj, MUIA_Window_Open, FALSE);
                        isopen = FALSE;
                    }
                }

                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C)
                {
                    break;
                }
            }
            if(retid == MUIV_Application_ReturnID_Quit)
            {
                set(po->po_WindowObj, MUIA_Window_Open, FALSE);
                /* remove all thingies */
                pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
                while(pog->pog_Node.ln_Succ)
                {
                    pFreePoPoGadgets(ps, pog);
                    pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
                }
            }
            if((ps->ps_GlobalCfg->pgc_PopupDeviceNew == PGCP_NEVER) &&
               (!ps->ps_GlobalCfg->pgc_PopupDeviceDeath) &&
               (!ps->ps_GlobalCfg->pgc_PopupDeviceGone))
            {
                psdAddErrorMsg0(RETURN_OK, (STRPTR) "PoPo", "PoPo has been abandoned (is it you, stuntzi?).");
                break;
            }

        } while(TRUE);
        set(po->po_WindowObj, MUIA_Window_Open, FALSE);
    }
    pPoPoGUITaskCleanup(ps);
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "pPoPoGUITaskCleanup()" */
void pPoPoGUITaskCleanup(LIBBASETYPEPTR ps)
{
    struct PsdPoPo *po = &ps->ps_PoPo;
    struct PsdPoPoGadgets *pog;
    struct PsdPoPoSound *pps;

    pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
    while(pog->pog_Node.ln_Succ)
    {
        pFreePoPoGadgets(ps, pog);
        pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
    }

    pps = (struct PsdPoPoSound *) po->po_Sounds.lh_Head;
    while(pps->pps_Node.ln_Succ)
    {
        pPoPoFreeSound(ps, pps);
        pps = (struct PsdPoPoSound *) po->po_Sounds.lh_Head;
    }

    if(po->po_TimerIOReq)
    {
        if(po->po_TimerIOReq->tr_node.io_Device)
        {
            AbortIO(&po->po_TimerIOReq->tr_node);
            WaitIO(&po->po_TimerIOReq->tr_node);
            CloseDevice((struct IORequest *) po->po_TimerIOReq);
        }
        DeleteIORequest((struct IORequest *) po->po_TimerIOReq);
        po->po_TimerIOReq = NULL;
    }
    if(po->po_TimerMsgPort)
    {
        DeleteMsgPort(po->po_TimerMsgPort);
        po->po_TimerMsgPort = NULL;
    }

    if(po->po_EventHandler)
    {
        psdRemEventHandler(po->po_EventHandler);
        po->po_EventHandler = NULL;
    }
    if(po->po_MsgPort)
    {
        DeleteMsgPort(po->po_MsgPort);
        po->po_MsgPort = NULL;
    }
    if(po->po_AppObj)
    {
        MUI_DisposeObject(po->po_AppObj);
        po->po_AppObj = NULL;
    }
    if(po->po_PoPoClass)
    {
        MUI_DeleteCustomClass(po->po_PoPoClass);
        po->po_PoPoClass = NULL;
    }
    if(DataTypesBase)
    {
        CloseLibrary(DataTypesBase);
        DataTypesBase = NULL;
    }
    if(MUIMasterBase)
    {
        CloseLibrary(MUIMasterBase);
        MUIMasterBase = NULL;
    }
    if(IntuitionBase)
    {
        CloseLibrary(IntuitionBase);
        IntuitionBase = NULL;
    }
    Forbid();
    po->po_Task = NULL;
    if(po->po_ReadySigTask)
    {
        Signal(po->po_ReadySigTask, 1L<<po->po_ReadySignal);
    }
}
/* \\\ */

/* /// "pPoPoLoadSound()" */
struct PsdPoPoSound * pPoPoLoadSound(LIBBASETYPEPTR ps, STRPTR name)
{
    struct PsdPoPo *po = &ps->ps_PoPo;
    struct PsdPoPoSound *pps;
    if((pps = psdAllocVec(sizeof(struct PsdPoPoSound))))
    {
        if((pps->pps_Node.ln_Name = psdCopyStr(name)))
        {
            AddTail(&po->po_Sounds, &pps->pps_Node);
            pps->pps_DTHandle = NewDTObject(name,
                                        DTA_SourceType, DTST_FILE,
                                        DTA_GroupID, GID_SOUND,
                                        SDTA_Cycles, 1L,
                                        TAG_END);
            if(!pps->pps_DTHandle)
            {
                psdAddErrorMsg(RETURN_ERROR, "PoPo", "Could not load soundfile '%s'.", name);
            }
            return(pps);
        }
        psdFreeVec(pps);
    }
    return(NULL);
}
/* \\\ */

/* /// "pPoPoPlaySound()" */
BOOL pPoPoPlaySound(LIBBASETYPEPTR ps, STRPTR name)
{
    struct PsdPoPo *po = &ps->ps_PoPo;
    struct PsdPoPoSound *pps;
    struct dtTrigger playmsg;

    pps = (struct PsdPoPoSound *) FindName(&po->po_Sounds, name);
    if(!pps)
    {
        pps = pPoPoLoadSound(ps, name);
    }
    if(!pps)
    {
        return(FALSE);
    }
    if(!pps->pps_DTHandle)
    {
        return(FALSE);
    }

    SetAttrs(pps->pps_DTHandle,
             SDTA_Volume, 64,
             TAG_END);
    playmsg.MethodID     = DTM_TRIGGER;
    playmsg.dtt_GInfo    = NULL;
    playmsg.dtt_Function = STM_PLAY;
    playmsg.dtt_Data     = NULL;
    DoMethodA(pps->pps_DTHandle, (Msg) &playmsg);
    return(TRUE);
}
/* \\\ */

/* /// "pPoPoFreeSound()" */
void pPoPoFreeSound(LIBBASETYPEPTR ps, struct PsdPoPoSound *pps)
{
    struct PsdPoPo *po = &ps->ps_PoPo;
    Remove(&pps->pps_Node);
    if(pps->pps_DTHandle)
    {
        DisposeDTObject(pps->pps_DTHandle);
    }
    psdFreeVec(pps->pps_Node.ln_Name);
    psdFreeVec(pps);
}
/* \\\ */

/* /// "pIsDeviceStillValid()" */
BOOL pIsDeviceStillValid(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    struct PsdDevice *tmppd = NULL;
    while((tmppd = psdGetNextDevice(tmppd)))
    {
        if(tmppd == pd)
        {
            return(TRUE);
        }
    }
    return(FALSE);
}
/* \\\ */

/* /// "pIsClassStillValid()" */
BOOL pIsClassStillValid(LIBBASETYPEPTR ps, struct Library *ucb)
{
    struct PsdUsbClass *puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
    while(puc->puc_Node.ln_Succ)
    {
        if(puc->puc_ClassBase == ucb)
        {
            return(TRUE);
        }
        puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
    }
    return(FALSE);
}
/* \\\ */

/* /// "pRemoveOldBox()" */
void pRemoveOldBox(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    struct PsdPoPo *po = &ps->ps_PoPo;
    struct PsdPoPoGadgets *pog;
    // try to remove adding message, if it still existing
    pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
    while(pog->pog_Node.ln_Succ)
    {
        if(pog->pog_Device == pd)
        {
            pFreePoPoGadgets(ps, pog);
            pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
        } else {
            pog = (struct PsdPoPoGadgets *) pog->pog_Node.ln_Succ;
        }
    }
}
/* \\\ */

/* /// "pGenerateAddBox()" */
struct PsdPoPoGadgets * pGenerateAddBox(LIBBASETYPEPTR ps, struct PsdDevice *pd, struct PsdPoPoGadgets *pog)
{
    struct PsdPoPo *po = &ps->ps_PoPo;
    STRPTR body = NULL;
    STRPTR bindingsstr;
    STRPTR bindinginfo;
    STRPTR bodyinfo;
    STRPTR wholebodyinfo;
    STRPTR gad[4] = { NULL, NULL, NULL, NULL };
    STRPTR lpwarn;
    STRPTR deadwarn;
    ULONG  configstate;
    BOOL   lowpower = FALSE;
    BOOL   isdead = FALSE;
    BOOL   updatetime = FALSE;
    STRPTR oldbody = NULL;
    UWORD  cnt;
    ULONG  methods[4] = { MUIM_PoPo_ConfigureBinding, MUIM_PoPo_ConfigureClass, MUIM_PoPo_ShutUp, MUIM_PoPo_RemInfo };

    bindingsstr = pBindingsString(ps, pd);
    configstate = pCheckConfigurable(ps, pd);

    lowpower = pd->pd_Flags & PDFF_LOWPOWER;
    isdead = pd->pd_Flags & PDFF_DEAD;

    body = "A new USB device was detected:\n\n\33b%s\33n\n\n";
    if(lowpower)
    {
        lpwarn = "\n\n\33bWARNING: Detected low power situation!";
    } else {
        lpwarn = "";
    }
    if(isdead)
    {
        deadwarn = "\n\n\33bWARNING: Device seems to be dead!";
    } else {
        deadwarn = "";
    }

    if(bindingsstr)
    {
        bindinginfo = psdCopyStrFmt("It is bound to \33b%s\33n", bindingsstr);
    } else {
        bindinginfo = psdCopyStr("No class seems responsible (yet)!");
    }
    if(!bindinginfo)
    {
        psdFreeVec(bindingsstr);
        return(NULL);
    }
    if(!(bodyinfo = psdCopyStrFmt(body, pd->pd_ProductStr)))
    {
        psdFreeVec(bindingsstr);
        psdFreeVec(bindinginfo);
        return(NULL);
    }
    if(!(wholebodyinfo = psdCopyStrFmt("%s%s%s%s", bodyinfo, bindinginfo, lpwarn, deadwarn)))
    {
        psdFreeVec(bindingsstr);
        psdFreeVec(bindinginfo);
        psdFreeVec(bodyinfo);
        return(NULL);
    }
    if(configstate & PPF_HasBindingGUI)
    {
        gad[0] = "Set Device Prefs";
    }
    if(configstate & PPF_HasBindConfig)
    {
        gad[0] = "Change Device Prefs";
    }
    if(configstate & PPF_HasClassGUI)
    {
        gad[1] = "Set Class Prefs";
    }
    if(configstate & PPF_HasClsConfig)
    {
        gad[1] = "Change Class Prefs";
    }
    if(isdead || lowpower)
    {
        gad[0] = "Disable Port";
        methods[0] = MUIM_PoPo_DisablePort;
        gad[1] = NULL;
    }
    if(isdead && (!lowpower))
    {
        gad[1] = "Powercycle Port";
        methods[1] = MUIM_PoPo_PowerCyclePort;
    }
    gad[2] = "No PopUps!";
    gad[3] = "Continue";
    if(!pog)
    {
        pog = pAllocPoPoGadgets(ps, wholebodyinfo, gad);
        if(pog)
        {
            pog->pog_Device = pd;
        }
    } else {
        get(pog->pog_BodyObj, MUIA_Text_Contents, &oldbody);
        if(strcmp(oldbody, wholebodyinfo))
        {
            set(pog->pog_BodyObj, MUIA_Text_Contents, wholebodyinfo);
            updatetime = TRUE;
        }
        for(cnt = 0; cnt < 4; cnt++)
        {
            if(gad[cnt])
            {
                set(pog->pog_GadgetObj[cnt], MUIA_Text_Contents, gad[cnt]);
                set(pog->pog_GadgetObj[cnt], MUIA_ShowMe, TRUE);
            } else {
                set(pog->pog_GadgetObj[cnt], MUIA_ShowMe, FALSE);
            }
        }
    }
    for(cnt = 0; cnt < 4; cnt++)
    {
        DoMethod(pog->pog_GadgetObj[cnt], MUIM_KillNotify, MUIA_Pressed);
    	if(gad[cnt])
    	{
            DoMethod(pog->pog_GadgetObj[cnt], MUIM_Notify, MUIA_Pressed, FALSE,
                     po->po_AppObj, 5, MUIM_Application_PushMethod, po->po_PoPoObj, 2, methods[cnt], pog);
    	}
    }

    psdFreeVec(bindingsstr);
    psdFreeVec(bindinginfo);
    psdFreeVec(bodyinfo);
    psdFreeVec(wholebodyinfo);

    /* update delay */
    if(updatetime && pog)
    {
        CurrentTime(&pog->pog_TimeoutTime.tv_secs, &pog->pog_TimeoutTime.tv_micro);
        if(ps->ps_GlobalCfg->pgc_PopupCloseDelay)
        {
            pog->pog_TimeoutTime.tv_secs += ps->ps_GlobalCfg->pgc_PopupCloseDelay;
        } else {
            pog->pog_TimeoutTime.tv_secs += 60*60*24;
        }
    }

    if((ps->ps_GlobalCfg->pgc_PopupDeviceNew >= PGCP_ISNEW) && pd->pd_IsNewToMe)
    {
        pog->pog_ShowMe = TRUE;
    }
    if((ps->ps_GlobalCfg->pgc_PopupDeviceNew >= PGCP_ERROR) && (isdead || lowpower))
    {
        pog->pog_ShowMe = TRUE;
    }
    switch(ps->ps_GlobalCfg->pgc_PopupDeviceNew)
    {
        case PGCP_NEVER:
        case PGCP_ERROR:
        case PGCP_ISNEW:
            break;

        case PGCP_NOBINDING:
            if(configstate & PPF_HasBinding)
            {
                break;
            }
            // break missing on purpose
        case PGCP_HASBINDING:
            if(configstate & PPF_HasBinding)
            {
                pog->pog_ShowMe = TRUE;
            }
            break;

        case PGCP_ASKCONFIG:
            if(configstate & PPF_HasBindingGUI)
            {
                if(!(configstate & PPF_HasBindConfig))
                {
                    pog->pog_ShowMe = TRUE;
                }
                break; // don't ask, if there is an unset default class config
            }
            if(configstate & PPF_HasClassGUI)
            {
                if(!(configstate & PPF_HasClsConfig))
                {
                    pog->pog_ShowMe = TRUE;
                }
            }
            break;

        case PGCP_CANCONFIG:
            if(configstate & (PPF_HasClassGUI|PPF_HasBindingGUI))
            {
                pog->pog_ShowMe = TRUE;
            }
            break;

        case PGCP_ALWAYS:
            pog->pog_ShowMe = TRUE;
            break;

        default:
            break;
    }
    return(pog);
}
/* \\\ */

/* /// "pEventHandler()" */
void pEventHandler(LIBBASETYPEPTR ps)
{
    struct PsdPoPo *po = &ps->ps_PoPo;
    struct PsdDevice *pd;
    struct PsdEventNote *pen;
    struct PsdPoPoGadgets *pog;
    ULONG  eventmask;
    STRPTR body = NULL;
    BOOL   addsound = FALSE;
    BOOL   remsound = FALSE;
    BOOL   cfgchanged = FALSE;

    /* delete removed devices first */
    psdLockReadPBase();

    pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
    while(pog->pog_Node.ln_Succ)
    {
        if(pog->pog_Device)
        {
            /* verify that device is still there */
            if(!pIsDeviceStillValid(ps, pog->pog_Device))
            {
                /* huh, gone! */
                pFreePoPoGadgets(ps, pog);
                pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
                continue;
            }
        }
        pog = (struct PsdPoPoGadgets *) pog->pog_Node.ln_Succ;
    }

    eventmask = 0;
    while((pen = (struct PsdEventNote *) GetMsg(po->po_MsgPort)))
    {
        eventmask |= (1L<<pen->pen_Event);
        switch(pen->pen_Event)
        {
            case EHMB_ADDDEVICE:
            {
                pd = (struct PsdDevice *) pen->pen_Param1;
                if(pIsDeviceStillValid(ps, pd))
                {
                    pRemoveOldBox(ps, pd);
                    if(pd->pd_PoPoCfg.poc_InhibitPopup)
                    {
                        break;
                    }
                    addsound = TRUE;
                    pog = pGenerateAddBox(ps, pd, NULL);
                    if(!pog)
                    {
                        break;
                    }
                    pog->pog_Device = pd;
                    if(pog->pog_ShowMe && (!pog->pog_WaitBinding))
                    {
                        po->po_OpenRequest = TRUE;
                    }
                }
                break;
            }

            case EHMB_DEVICEDEAD:
            case EHMB_DEVICELOWPW:
            {
                BOOL lowpower = FALSE;
                BOOL isdead = FALSE;
                BOOL autodis = FALSE;
                STRPTR body;
                STRPTR devname;
                STRPTR gads[4] = { NULL, NULL, NULL, "Ignore" };
                STRPTR autodismsg = "";

                pd = (struct PsdDevice *) pen->pen_Param1;
                if(!pIsDeviceStillValid(ps, pd))
                {
                    break;
                }
                lowpower = pd->pd_Flags & PDFF_LOWPOWER;
                isdead = pd->pd_Flags & PDFF_DEAD;
                if(!(lowpower || isdead))
                {
                    break;
                }
                if((ps->ps_GlobalCfg->pgc_AutoDisableLP && lowpower) ||
                   (ps->ps_GlobalCfg->pgc_AutoDisableDead && isdead) ||
                   (ps->ps_GlobalCfg->pgc_AutoRestartDead && isdead))
                {
                    struct Library *UsbClsBase;
                    struct PsdDevice *hubpd = pd->pd_Hub;
                    Forbid();
                    if(hubpd->pd_DevBinding)
                    {
                        UsbClsBase = hubpd->pd_ClsBinding->puc_ClassBase;
                        if(pIsClassStillValid(ps, UsbClsBase))
                        {
                            if(ps->ps_GlobalCfg->pgc_AutoRestartDead && isdead)
                            {
                                usbDoMethod((ULONG) UCM_HubPowerCyclePort, hubpd, pd->pd_HubPort);
                                psdAddErrorMsg(RETURN_WARN, ps->ps_Library.lib_Node.ln_Name,
                                               "Automatically powercycling port for '%s' due to death event.",
                                               pd->pd_ProductStr);
                                autodismsg = "\n\n\33bAutomatically powercycling port!\33n";
                            } else {
                                usbDoMethod((ULONG) UCM_HubDisablePort, hubpd, pd->pd_HubPort);
                                psdAddErrorMsg(RETURN_WARN, ps->ps_Library.lib_Node.ln_Name,
                                               "Automatically disabling port for '%s' due lowpower/death event.",
                                               pd->pd_ProductStr);
                                autodismsg = "\n\n\33bAutomatically disabling port!\33n";
                            }
                            autodis = TRUE;
                        }
                    }
                    Permit();
                }
                if(pd->pd_PoPoCfg.poc_InhibitPopup)
                {
                    break;
                }

                remsound = TRUE;
                if((ps->ps_GlobalCfg->pgc_PopupDeviceNew < PGCP_ERROR) && (!ps->ps_GlobalCfg->pgc_PopupDeviceDeath))
                {
                    break;
                }

                pRemoveOldBox(ps, pd);
                gads[0] = "Disable Port";
                devname = pd->pd_ProductStr;
                if(!devname)
                {
                    devname = "Unknown Soldier";
                }
                if(lowpower && isdead)
                {
                    body = psdCopyStrFmt("\33bWARNING: Detected LOW POWER situation\33n\n\nfor USB device\n\n\33b%s\33n\n\nand it has \33bdropped dead already\33n!%s", devname, autodismsg);
                }
                else if(lowpower)
                {
                    body = psdCopyStrFmt("\33bWARNING: Detected LOW POWER situation\33n\n\nfor USB device\n\n\33b%s\33n\n\nand this might cause failures!%s", devname, autodismsg);
                } else {
                    body = psdCopyStrFmt("\33bWARNING: DEAD DEVICE!\33n\n\nUSB device\n\n\33b%s\33n\n\ndropped dead for no apparent reason!%s", devname, autodismsg);
                    gads[1] = "Powercycle Port";
                }
                pog = pAllocPoPoGadgets(ps, body, gads);
                psdFreeVec(body);
                if(!pog)
                {
                    break;
                }

                if(autodis)
                {
                    set(pog->pog_GadgetObj[0], MUIA_Disabled, TRUE);
                    pog->pog_Device = NULL; // to keep the message on the screen
                } else {
                    pog->pog_Device = pd;
                }
                DoMethod(pog->pog_GadgetObj[0], MUIM_Notify, MUIA_Pressed, FALSE,
                         po->po_AppObj, 5, MUIM_Application_PushMethod, po->po_PoPoObj, 2, MUIM_PoPo_DisablePort, pog);
                if(gads[1])
                {
                    DoMethod(pog->pog_GadgetObj[1], MUIM_Notify, MUIA_Pressed, FALSE,
                             po->po_AppObj, 5, MUIM_Application_PushMethod, po->po_PoPoObj, 2, MUIM_PoPo_PowerCyclePort, pog);
                }
                DoMethod(pog->pog_GadgetObj[3], MUIM_Notify, MUIA_Pressed, FALSE,
                         po->po_AppObj, 5, MUIM_Application_PushMethod, po->po_PoPoObj, 2, MUIM_PoPo_RemInfo, pog);
                po->po_OpenRequest = TRUE;
                break;
            }

            case EHMB_REMDEVICE:
            {
                pd = (struct PsdDevice *) pen->pen_Param1;
                pRemoveOldBox(ps, pd);
                if(pd->pd_PoPoCfg.poc_InhibitPopup)
                {
                    break;
                }
                remsound = TRUE;
                if(!ps->ps_GlobalCfg->pgc_PopupDeviceGone)
                {
                    break;
                }
                if(pd->pd_ProductStr)
                {
                    body = psdCopyStrFmt("The USB device\n\n\33b%s\33n\n\nhas been removed!", pd->pd_ProductStr);
                    /* late free */
                    //psdFreeVec(pd->pd_ProductStr);
                    //pd->pd_ProductStr = NULL;
                } else {
                    body = psdCopyStr("An USB device has been removed,\nbut I cannot recall its name.");
                }
                if(body)
                {
                    STRPTR gads[4] = { NULL, NULL, NULL, "Bye bye!" };
                    pog = pAllocPoPoGadgets(ps, body, gads);
                    psdFreeVec(body);
                    if(!pog)
                    {
                        break;
                    }
                    DoMethod(pog->pog_GadgetObj[3], MUIM_Notify, MUIA_Pressed, FALSE,
                             po->po_AppObj, 5, MUIM_Application_PushMethod, po->po_PoPoObj, 2, MUIM_PoPo_RemInfo, pog);
                    po->po_OpenRequest = TRUE;
                }
                break;
            }

            case EHMB_ADDBINDING:
            case EHMB_REMBINDING:
            {
                pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
                while(pog->pog_Node.ln_Succ)
                {
                    if(pog->pog_Device == (struct PsdDevice *) pen->pen_Param1)
                    {
                        if(!pIsDeviceStillValid(ps, pog->pog_Device))
                        {
                            break;
                        }
                        if(pog->pog_Device->pd_PoPoCfg.poc_InhibitPopup)
                        {
                            break;
                        }
                        pGenerateAddBox(ps, pog->pog_Device, pog);
                        if(pog->pog_ShowMe)
                        {
                            po->po_OpenRequest = TRUE;
                        }
                        break;
                    }
                    pog = (struct PsdPoPoGadgets *) pog->pog_Node.ln_Succ;
                }
                break;
            }

            case EHMB_CONFIGCHG:
                if(!cfgchanged)
                {
                    psdDelayMS(100);
                    cfgchanged = TRUE;
                }
                break;

        }
        ReplyMsg(&pen->pen_Msg);
    }
    psdUnlockPBase();
    if(addsound)
    {
        if(po->po_InsertSndFile)
        {
            if(*po->po_InsertSndFile)
            {
                pPoPoPlaySound(ps, po->po_InsertSndFile);
            }
        }
    }
    if(remsound)
    {
        if(po->po_RemoveSndFile)
        {
            if(*po->po_RemoveSndFile)
            {
                pPoPoPlaySound(ps, po->po_RemoveSndFile);
            }
        }
    }
    if(cfgchanged)
    {
        set(po->po_SaveObj, MUIA_Disabled, (ps->ps_SavedConfigHash == ps->ps_ConfigHash));
    }
}
/* \\\ */

/* /// "pBindingsString()" */
STRPTR pBindingsString(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    STRPTR oldstr = NULL;
    STRPTR newstr = NULL;
    struct PsdConfig *pc;
    struct PsdInterface *pif;

    if(pd->pd_DevBinding)
    {
        if(pd->pd_Flags & PDFF_APPBINDING)
        {
            return(psdCopyStr(((struct PsdAppBinding *) pd->pd_DevBinding)->pab_Task->tc_Node.ln_Name));
        } else {
            return(psdCopyStr(pd->pd_ClsBinding->puc_ClassBase->lib_Node.ln_Name));
        }
    } else {
        pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
        while(pc->pc_Node.ln_Succ)
        {
            pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
            while(pif->pif_Node.ln_Succ)
            {
                if(pif->pif_IfBinding)
                {
                    if(oldstr)
                    {
                        newstr = psdCopyStrFmt("%s, %s", oldstr, pif->pif_ClsBinding->puc_ClassBase->lib_Node.ln_Name);
                        psdFreeVec(oldstr),
                        oldstr = newstr;
                    } else {
                        oldstr = psdCopyStr(pif->pif_ClsBinding->puc_ClassBase->lib_Node.ln_Name);
                    }
                }
                pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
            }
            pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
        }
    }
    return(oldstr);
}
/* \\\ */

/* /// "pCheckConfigurable()" */
ULONG pCheckConfigurable(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    ULONG  hasclassgui;
    ULONG  hasbindinggui;
    ULONG  noconfig;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    struct Library *UsbClsBase;
    ULONG result = 0;

    psdLockReadPBase();
    if(pd->pd_DevBinding)
    {
        if(!(pd->pd_Flags & PDFF_APPBINDING))
        {
            UsbClsBase = pd->pd_ClsBinding->puc_ClassBase;
            noconfig = FALSE;
            hasclassgui = FALSE;
            hasbindinggui = FALSE;
            if(pIsClassStillValid(ps, UsbClsBase))
            {
                usbGetAttrs(UGA_CLASS, NULL,
                            UCCA_HasClassCfgGUI, &hasclassgui,
                            UCCA_HasBindingCfgGUI, &hasbindinggui,
                            UCCA_UsingDefaultCfg, &noconfig,
                            TAG_END);
                result |= PPF_HasBinding;
                if(hasclassgui)
                {
                    result |= PPF_HasClassGUI;
                    if(!noconfig)
                    {
                        result |= PPF_HasClsConfig;
                    }
                }
                if(hasbindinggui)
                {
                    result |= PPF_HasBindingGUI;
                    usbGetAttrs(UGA_BINDING, pd->pd_DevBinding,
                                UCBA_UsingDefaultCfg, &noconfig,
                                TAG_END);
                    if(!noconfig)
                    {
                        result |= PPF_HasBindConfig;
                    }
                }
            }
        }
    } else {
        pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
        while(pc->pc_Node.ln_Succ)
        {
            pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
            while(pif->pif_Node.ln_Succ)
            {
                if(pif->pif_IfBinding)
                {
                    UsbClsBase = pif->pif_ClsBinding->puc_ClassBase;
                    noconfig = FALSE;
                    hasclassgui = FALSE;
                    hasbindinggui = FALSE;
                    if(pIsClassStillValid(ps, UsbClsBase))
                    {
                        usbGetAttrs(UGA_CLASS, NULL,
                                    UCCA_HasClassCfgGUI, &hasclassgui,
                                    UCCA_HasBindingCfgGUI, &hasbindinggui,
                                    UCCA_UsingDefaultCfg, &noconfig,
                                    TAG_END);
                        result |= PPF_HasBinding;
                        if(hasclassgui)
                        {
                            result |= PPF_HasClassGUI;
                            if(!noconfig)
                            {
                                result |= PPF_HasClsConfig;
                            }
                        }
                        if(hasbindinggui)
                        {
                            result |= PPF_HasBindingGUI;
                            usbGetAttrs(UGA_BINDING, pif->pif_IfBinding,
                                        UCBA_UsingDefaultCfg, &noconfig,
                                        TAG_END);
                            if(!noconfig)
                            {
                                result |= PPF_HasBindConfig;
                            }
                        }
                    }
                }
                pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
            }
            pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
        }
    }
    psdUnlockPBase();
    return(result);
}
/* \\\ */

/* /// "pOpenBindingsConfigGUI()" */
void pOpenBindingsConfigGUI(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    struct Library *UsbClsBase;
    ULONG hascfggui;
    psdLockReadPBase();
    if(!pIsDeviceStillValid(ps, pd))
    {
        psdUnlockPBase();
        return;
    }
    if(pd->pd_DevBinding)
    {
        if(!(pd->pd_Flags & PDFF_APPBINDING))
        {
            UsbClsBase = pd->pd_ClsBinding->puc_ClassBase;
            if(pIsClassStillValid(ps, UsbClsBase))
            {
                usbGetAttrs(UGA_CLASS, NULL,
                            UCCA_HasBindingCfgGUI, &hascfggui,
                            TAG_END);
                if(hascfggui)
                {
                    usbDoMethod(UCM_OpenBindingCfgWindow, pd->pd_DevBinding);
                }
            }
        }
    } else {
        pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
        while(pc->pc_Node.ln_Succ)
        {
            pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
            while(pif->pif_Node.ln_Succ)
            {
                if(pif->pif_IfBinding)
                {
                    UsbClsBase =  pif->pif_ClsBinding->puc_ClassBase;
                    if(pIsClassStillValid(ps, UsbClsBase))
                    {
                        usbGetAttrs(UGA_CLASS, NULL,
                                    UCCA_HasBindingCfgGUI, &hascfggui,
                                    TAG_END);
                        if(hascfggui)
                        {
                            usbDoMethod(UCM_OpenBindingCfgWindow, pif->pif_IfBinding);
                        }
                    }
                }
                pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
            }
            pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
        }
    }
    psdUnlockPBase();
}
/* \\\ */

/* /// "pOpenClassesConfigGUI()" */
void pOpenClassesConfigGUI(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    struct Library *UsbClsBase;
    ULONG hascfggui;

    psdLockReadPBase();
    if(!pIsDeviceStillValid(ps, pd))
    {
        psdUnlockPBase();
        return;
    }

    if(pd->pd_DevBinding)
    {
        if(!(pd->pd_Flags & PDFF_APPBINDING))
        {
            UsbClsBase = pd->pd_ClsBinding->puc_ClassBase;
            if(pIsClassStillValid(ps, UsbClsBase))
            {
                usbGetAttrs(UGA_CLASS, NULL,
                            UCCA_HasClassCfgGUI, &hascfggui,
                            TAG_END);
                if(hascfggui)
                {
                    usbDoMethod(UCM_OpenCfgWindow);
                }
            }
        }
    } else {
        pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
        while(pc->pc_Node.ln_Succ)
        {
            pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
            while(pif->pif_Node.ln_Succ)
            {
                if(pif->pif_IfBinding)
                {
                    UsbClsBase =  pif->pif_ClsBinding->puc_ClassBase;
                    if(pIsClassStillValid(ps, UsbClsBase))
                    {
                        usbGetAttrs(UGA_CLASS, NULL,
                                    UCCA_HasClassCfgGUI, &hascfggui,
                                    TAG_END);
                        if(hascfggui)
                        {
                            usbDoMethod(UCM_OpenCfgWindow);
                        }
                    }
                }
                pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
            }
            pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
        }
    }
    psdUnlockPBase();
}
/* \\\ */

/* /// "pDisableDevicePopup()" */
void pDisableDevicePopup(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    if(!pIsDeviceStillValid(ps, pd))
    {
        return;
    }
    psdSetAttrs(PGA_DEVICE, pd, DA_InhibitPopup, TRUE, TAG_END);
}
/* \\\ */

/* /// "pAllocPoPoGadgets()" */
struct PsdPoPoGadgets * pAllocPoPoGadgets(LIBBASETYPEPTR ps, STRPTR body, STRPTR *gad)
{
    struct PsdPoPo *po = &ps->ps_PoPo;
    struct PsdPoPoGadgets *pog;
    if((pog = (struct PsdPoPoGadgets *) psdAllocVec(sizeof(struct PsdPoPoGadgets))))
    {
        pog->pog_GroupObj =
            VGroup,
                Child, (IPTR)(pog->pog_BodyObj = TextObject,
                    MUIA_Frame, MUIV_Frame_Text,
                    MUIA_Background, MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)body,
                    End),
                Child, (IPTR)(HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(pog->pog_GadgetObj[0] = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_PreParse, (IPTR)"\33c",
                        MUIA_Text_Contents, (IPTR)(gad[0] ? gad[0] : (STRPTR) ""),
                        MUIA_ShowMe, (IPTR)gad[0],
                        End),
                    Child, (IPTR)(pog->pog_GadgetObj[1] = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_PreParse, (IPTR)"\33c",
                        MUIA_Text_Contents, (IPTR)(gad[1] ? gad[1] : (STRPTR) ""),
                        MUIA_ShowMe, (IPTR)gad[1],
                        End),
                    Child, (IPTR)(pog->pog_GadgetObj[2] = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_PreParse, (IPTR)"\33c",
                        MUIA_Text_Contents, (IPTR)(gad[2] ? gad[2] : (STRPTR) ""),
                        MUIA_ShowMe, (IPTR)gad[2],
                        End),
                    Child, (IPTR)(pog->pog_GadgetObj[3] = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_PreParse, (IPTR)"\33c",
                        MUIA_Text_Contents, (IPTR)(gad[3] ? gad[3] : (STRPTR) ""),
                        MUIA_ShowMe, (IPTR)gad[3],
                        End),
                    End,
                //Child, VSpace(0),
                Child, (IPTR)BalanceObject,
                    End),
                //Child, VSpace(0),
                End;

        if(!pog->pog_GroupObj)
        {
            psdFreeVec(pog);
            return(NULL);
        }
        DoMethod(po->po_GroupObj, MUIM_Group_InitChange);
        DoMethod(po->po_GroupObj, OM_ADDMEMBER, pog->pog_GroupObj);
        DoMethod(po->po_GroupObj, MUIM_Group_ExitChange);
        AddTail(&po->po_GadgetList, &pog->pog_Node);
        CurrentTime(&pog->pog_TimeoutTime.tv_secs, &pog->pog_TimeoutTime.tv_micro);
        if(ps->ps_GlobalCfg->pgc_PopupCloseDelay && (!po->po_Sticky))
        {
            pog->pog_TimeoutTime.tv_secs += ps->ps_GlobalCfg->pgc_PopupCloseDelay;
        } else {
            pog->pog_TimeoutTime.tv_secs += 60*60*24;
        }
    }
    return(pog);
}
/* \\\ */

/* /// "pFreePoPoGadgets()" */
void pFreePoPoGadgets(LIBBASETYPEPTR ps, struct PsdPoPoGadgets *pog)
{
    struct PsdPoPo *po = &ps->ps_PoPo;
    Remove(&pog->pog_Node);
    DoMethod(po->po_GroupObj, MUIM_Group_InitChange);
    DoMethod(po->po_GroupObj, OM_REMMEMBER, pog->pog_GroupObj);
    DoMethod(po->po_GroupObj, MUIM_Group_ExitChange);
    MUI_DisposeObject(pog->pog_GroupObj);
    psdFreeVec(pog);
}
/* \\\ */

/* /// "PoPoDispatcher()" */
AROS_UFH3(IPTR, PoPoDispatcher,
                 AROS_UFHA(struct IClass *, cl, A0),
                 AROS_UFHA(Object *, obj, A2),
                 AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
    struct PsdPoPoGadgets *pog;
    struct PsdPoPoGadgets *tmppog;
    LIBBASETYPEPTR ps = (LIBBASETYPEPTR) cl->cl_UserData;
    struct PsdPoPo *po = &ps->ps_PoPo;
    ULONG sticky = 0;

    switch(msg->MethodID)
    {
        case OM_NEW:
            if(!(obj = (Object *)DoSuperMethodA(cl,obj,msg)))
                return(0);
            return((IPTR)obj);

        case MUIM_PoPo_SavePrefs:
            psdSaveCfgToDisk(NULL, FALSE);
            set(po->po_SaveObj, MUIA_Disabled, (ps->ps_SavedConfigHash == ps->ps_ConfigHash));
            return(0);

        case MUIM_PoPo_Sticky:
            get(po->po_StickyObj, MUIA_Selected, &sticky);
            po->po_Sticky = sticky;
            pog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
            while(pog->pog_Node.ln_Succ)
            {
                CurrentTime(&pog->pog_TimeoutTime.tv_secs, &pog->pog_TimeoutTime.tv_micro);
                if(ps->ps_GlobalCfg->pgc_PopupCloseDelay && (!po->po_Sticky))
                {
                    pog->pog_TimeoutTime.tv_secs += ps->ps_GlobalCfg->pgc_PopupCloseDelay;
                } else {
                    pog->pog_TimeoutTime.tv_secs += 60*60*24;
                }
                pog = (struct PsdPoPoGadgets *) pog->pog_Node.ln_Succ;
            }
            return(0);

        case MUIM_PoPo_ConfigureBinding:
            pog = (struct PsdPoPoGadgets *) ((IPTR *) msg)[1];
            //KPRINTF(20, ("Config Po=%08lx, Pog=%08lx, PS=%08lx\n", po, pog, ps));
            pog->pog_TimeoutTime.tv_secs += 60*60*24;
            pOpenBindingsConfigGUI(ps, pog->pog_Device);
            return(0);

        case MUIM_PoPo_ConfigureClass:
            pog = (struct PsdPoPoGadgets *) ((IPTR *) msg)[1];
            //KPRINTF(20, ("Config Po=%08lx, Pog=%08lx, PS=%08lx\n", po, pog, ps));
            pog->pog_TimeoutTime.tv_secs += 60*60*24;
            pOpenClassesConfigGUI(ps, pog->pog_Device);
            return(0);

        case MUIM_PoPo_ShutUp:
            pog = (struct PsdPoPoGadgets *) ((IPTR *) msg)[1];
            pDisableDevicePopup(ps, pog->pog_Device);
            pRemoveOldBox(ps, pog->pog_Device);
            return(0);

        case MUIM_PoPo_RemInfo:
            pog = (struct PsdPoPoGadgets *) ((IPTR *) msg)[1];
            //KPRINTF(20, ("RemInfo Po=%08lx, Pog=%08lx, PS=%08lx\n", po, pog, ps));
            tmppog = (struct PsdPoPoGadgets *) po->po_GadgetList.lh_Head;
            while(tmppog->pog_Node.ln_Succ)
            {
                if(tmppog == pog)
                {
                    pFreePoPoGadgets(ps, pog);
                    break;
                }
                tmppog = (struct PsdPoPoGadgets *) tmppog->pog_Node.ln_Succ;
            }
            return(0);

        case MUIM_PoPo_DisablePort:
        case MUIM_PoPo_PowerCyclePort:
        {
            struct PsdDevice *pd;
            struct Library *UsbClsBase;
            BOOL disable = (msg->MethodID == MUIM_PoPo_DisablePort);

            pog = (struct PsdPoPoGadgets *) ((IPTR *) msg)[1];
            pd = pog->pog_Device;
            set(pog->pog_GadgetObj[0], MUIA_Disabled, TRUE);
            if(pog->pog_GadgetObj[1] && (!disable))
            {
                set(pog->pog_GadgetObj[1], MUIA_Disabled, TRUE);
            }
            Forbid();
            if(pIsDeviceStillValid(ps, pd))
            {
                struct PsdDevice *hubpd = pd->pd_Hub;
                if(hubpd->pd_DevBinding)
                {
                    UsbClsBase = hubpd->pd_ClsBinding->puc_ClassBase;
                    if(pIsClassStillValid(ps, UsbClsBase))
                    {
                        usbDoMethod((ULONG) (disable ? UCM_HubDisablePort : UCM_HubPowerCyclePort), hubpd, pd->pd_HubPort);
                    }
                }
            }
            Permit();
            pog->pog_TimeoutTime.tv_secs += 60*60*24;

            return(0);
        }

        case MUIM_PoPo_About:
            MUI_RequestA(po->po_AppObj, po->po_WindowObj, 0, NULL, "Blimey!", "PoPo -- The Poseidon Popup Provider\n\nWritten by Chris Hodges.\n\nWichtig ist, was hinten rauskommt (Helmut Kohl).", NULL);
            return(0);

        case MUIM_PoPo_OpenTrident:
        {
            struct Library *DOSBase;
            if((DOSBase = OpenLibrary("dos.library", 39)))
            {
                BPTR fhandle;
                if((fhandle = Open("NIL:", MODE_READWRITE)))
                {
                    if(SystemTags("Trident",
                                  NP_StackSize, 32*1024,
                                  SYS_Input, fhandle,
                                  SYS_Output, NULL,
                                  SYS_Asynch, TRUE,
                                  TAG_END))
                    {
                        Close(fhandle);
                        MUI_RequestA(po->po_AppObj, po->po_WindowObj, 0, NULL, "Oh no!", "Bugger!\n\nI tried hard to load Trident,\nbut there was Cryptonite somewhere!", NULL);
                    }
                }
                CloseLibrary(DOSBase);
            }
            return(0);
        }
    }
    return(DoSuperMethodA(cl,obj,msg));
    AROS_USERFUNC_EXIT
}
/* \\\ */
