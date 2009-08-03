/*
 * GUI
 */

#include "debug.h"
#include "numtostr.h"

#include "hid.class.h"

extern const STRPTR GM_UNIQUENAME(libname);

/* /// "nHIDCtrlGUITask()" */
AROS_UFH0(void, GM_UNIQUENAME(nHIDCtrlGUITask))
{
    AROS_USERFUNC_INIT
    
    struct Task *thistask;
    struct NepHidBase *nh;
    struct NepClassHid *nch;

    thistask = FindTask(NULL);

#undef ps
#define ps nch->nch_HCPsdBase
#undef IntuitionBase
#define IntuitionBase nch->nch_HCIntBase
#undef MUIMasterBase
#define MUIMasterBase nch->nch_HCMUIBase

    nch = thistask->tc_UserData;
    nch->nch_HCGUITask = thistask;
    nh = nch->nch_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    NewList(&nch->nch_HCGUIItems);
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        GM_UNIQUENAME(nHIDCtrlGUITaskCleanup)(nch);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        GM_UNIQUENAME(nHIDCtrlGUITaskCleanup)(nch);
        return;
    }

    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        GM_UNIQUENAME(nHIDCtrlGUITaskCleanup)(nch);
        return;
    }

    nch->nch_HCActionClass = MUI_CreateCustomClass(NULL, MUIC_Area  , NULL, sizeof(struct ActionData), GM_UNIQUENAME(HCActionDispatcher));

    if(!nch->nch_HCActionClass)
    {
        KPRINTF(10, ("Couldn't create ActionClass.\n"));
        GM_UNIQUENAME(nHIDCtrlGUITaskCleanup)(nch);
        return;
    }
    nch->nch_HCApp = ApplicationObject,
        MUIA_Application_Title      , nch->nch_CDC->cdc_HIDCtrlTitle,
        MUIA_Application_Version    , VERSION_STRING,
        MUIA_Application_Copyright  , "©2002-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "HID Device Output Control",
        MUIA_Application_Base       , nch->nch_CDC->cdc_HIDCtrlRexx,
        MUIA_Application_HelpFile   , "HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , MenustripObject,
            Child, MenuObjectT("Project"),
                Child, nch->nch_HCAboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, "About...",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, nch->nch_HCCloseMI = MenuitemObject,
                    MUIA_Menuitem_Title, "Hide",
                    MUIA_Menuitem_Shortcut, "H",
                    End,
                End,
            Child, MenuObjectT("Settings"),
                Child, nch->nch_HCMUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, "MUI Settings",
                    MUIA_Menuitem_Shortcut, "M",
                    End,
                End,
            End,

        SubWindow, nch->nch_HCMainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('H','C','T','L'),
            MUIA_Window_Title, nch->nch_CDC->cdc_HIDCtrlTitle,
            MUIA_HelpNode, GM_UNIQUENAME(libname),

            WindowContents, VGroup,
                Child, nch->nch_HCActionObj = NewObject(nch->nch_HCActionClass->mcc_Class, 0, MUIA_ShowMe, FALSE, TAG_END),
                Child, nch->nch_HCGroupObj = ColGroup(4),
                    End,
                Child, nch->nch_HCCloseObj = TextObject, ButtonFrame,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_CycleChain, 1,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_Text_Contents, "\33c Hide ",
                    End,
                End,
            End,
        End;

    if(!nch->nch_HCApp)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        GM_UNIQUENAME(nHIDCtrlGUITaskCleanup)(nch);
        return;
    }

    {
        struct ActionData *ad = INST_DATA(nch->nch_HCActionClass->mcc_Class, nch->nch_HCActionObj);
        ad->ad_NCH = nch;
    }
    /* add items */
    {
        struct NepHidReport *nhr;
        struct NepHidItem **nhiptr;
        struct NepHidItem *nhi;
        struct NepHidGItem *nhgi;
        UWORD count;
        Object *obj;
        UWORD numobj = 0;

        nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            if((count = nhr->nhr_OutItemCount))
            {
                nhiptr = nhr->nhr_OutItemMap;
                if(nhiptr)
                {
                    do
                    {
                        nhi = *nhiptr++;
                        obj = NULL;
                        nhgi = NULL;
                        if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                        {
                            if(nhi->nhi_Flags & RPF_MAIN_RELATIVE)
                            {
                                if((nhi->nhi_LogicalMin == 0) && (nhi->nhi_LogicalMax == 1))
                                {
                                    if((nhgi = nAllocGHCItem(nch, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* one shot */
                                        obj = VGroup,
                                            Child, VSpace(0),
                                            Child, nhgi->nhgi_GUIObj = TextObject, ButtonFrame,
                                                MUIA_Background, MUII_ButtonBack,
                                                MUIA_CycleChain, 1,
                                                MUIA_InputMode, MUIV_InputMode_RelVerify,
                                                MUIA_Text_PreParse, "\33c",
                                                MUIA_Text_Contents, nhgi->nhgi_Name,
                                                End,
                                            Child, VSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SHOTBUTTON;
                                    }
                                }
                                else if(nhi->nhi_LogicalMin < 0)
                                {
                                    if((nhgi = nAllocGHCItem(nch, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* Horizontal slider */
                                        obj = VGroup,
                                            Child, VSpace(0),
                                            Child, HGroup,
                                                Child, Label(nhgi->nhgi_Name),
                                                Child, nhgi->nhgi_GUIObj = SliderObject, SliderFrame,
                                                    MUIA_Slider_Horiz, TRUE,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_InputMode, MUIV_InputMode_Immediate,
                                                    MUIA_Numeric_Min, nhi->nhi_LogicalMin,
                                                    MUIA_Numeric_Max, nhi->nhi_LogicalMax,
                                                    MUIA_Numeric_Value, nhi->nhi_OldValue,
                                                    End,
                                                End,
                                            Child, VSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SLIDERIMM;
                                    }
                                } else {
                                    if((nhgi = nAllocGHCItem(nch, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* Vertical slider */
                                        obj = HGroup,
                                            Child, HSpace(0),
                                            Child, VGroup,
                                                Child, nhgi->nhgi_GUIObj = SliderObject, SliderFrame,
                                                    MUIA_Slider_Horiz, FALSE,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_InputMode, MUIV_InputMode_Immediate,
                                                    MUIA_Numeric_Min, nhi->nhi_LogicalMin,
                                                    MUIA_Numeric_Max, nhi->nhi_LogicalMax,
                                                    MUIA_Numeric_Value, nhi->nhi_OldValue,
                                                    End,
                                                Child, Label(nhgi->nhgi_Name),
                                                End,
                                            Child, HSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SLIDERIMM;
                                    }
                                }
                            } else {
                                if((nhi->nhi_LogicalMin == 0) && (nhi->nhi_LogicalMax == 1))
                                {
                                    if((nhgi = nAllocGHCItem(nch, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* toggle button */
                                        obj = VGroup,
                                            Child, VSpace(0),
                                            Child, nhgi->nhgi_GUIObj = TextObject, ButtonFrame,
                                                MUIA_Background, MUII_ButtonBack,
                                                MUIA_CycleChain, 1,
                                                MUIA_InputMode, MUIV_InputMode_Toggle,
                                                MUIA_Text_PreParse, "\33c",
                                                MUIA_Text_Contents, nhgi->nhgi_Name,
                                                End,
                                            Child, VSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_TOGGLEBUTTON;
                                    }
                                }
                                else if(nhi->nhi_LogicalMin < 0)
                                {
                                    if((nhgi = nAllocGHCItem(nch, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* Horizontal slider */
                                        obj = VGroup,
                                            Child, VSpace(0),
                                            Child, HGroup,
                                                Child, Label(nhgi->nhgi_Name),
                                                Child, nhgi->nhgi_GUIObj = SliderObject, SliderFrame,
                                                    MUIA_Slider_Horiz, TRUE,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Numeric_Min, nhi->nhi_LogicalMin,
                                                    MUIA_Numeric_Max, nhi->nhi_LogicalMax,
                                                    MUIA_Numeric_Value, nhi->nhi_OldValue,
                                                    End,
                                                End,
                                            Child, VSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SLIDER;
                                    }
                                } else {
                                    if((nhgi = nAllocGHCItem(nch, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* Vertical slider */
                                        obj = HGroup,
                                            Child, HSpace(0),
                                            Child, VGroup,
                                                Child, nhgi->nhgi_GUIObj = SliderObject, SliderFrame,
                                                    MUIA_Slider_Horiz, FALSE,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Numeric_Min, nhi->nhi_LogicalMin,
                                                    MUIA_Numeric_Max, nhi->nhi_LogicalMax,
                                                    MUIA_Numeric_Value, nhi->nhi_OldValue,
                                                    End,
                                                Child, Label(nhgi->nhgi_Name),
                                                End,
                                            Child, HSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SLIDER;
                                    }
                                }
                            }
                        }
                        if(obj)
                        {
                            DoMethod(nch->nch_HCGroupObj, OM_ADDMEMBER, obj);
                            switch(nhgi->nhgi_ObjType)
                            {
                                case NHGIOT_SHOTBUTTON:
                                    DoMethod(nhgi->nhgi_GUIObj, MUIM_Notify, MUIA_Pressed, FALSE,
                                             nch->nch_HCActionObj, 2, MUIM_Action_UpdateHIDCtrl, nhgi);
                                    break;

                                case NHGIOT_TOGGLEBUTTON:
                                    DoMethod(nhgi->nhgi_GUIObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
                                             nch->nch_HCActionObj, 2, MUIM_Action_UpdateHIDCtrl, nhgi);
                                    break;

                                case NHGIOT_SLIDERIMM:
                                    DoMethod(nhgi->nhgi_GUIObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
                                             nch->nch_HCActionObj, 2, MUIM_Action_UpdateHIDCtrl, nhgi);
                                    break;

                                case NHGIOT_SLIDER:
                                    DoMethod(nhgi->nhgi_GUIObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
                                             nch->nch_HCActionObj, 2, MUIM_Action_UpdateHIDCtrl, nhgi);
                                    break;
                            }
                            numobj++;
                        }
                    } while(--count);
                }
            }
            nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
        }
        if(!numobj)
        {
            DoMethod(nch->nch_HCGroupObj, OM_ADDMEMBER, Label("No output items in this interface!"));
        }
    }

    DoMethod(nch->nch_HCMainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             nch->nch_HCActionObj, 1, MUIM_Action_HideHIDControl);
    DoMethod(nch->nch_HCCloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_HCActionObj, 1, MUIM_Action_HideHIDControl);

    DoMethod(nch->nch_HCAboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_HCActionObj, 1, MUIM_Action_About);
    DoMethod(nch->nch_HCCloseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_HCActionObj, 1, MUIM_Action_HideHIDControl);
    DoMethod(nch->nch_HCMUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_HCApp, 2, MUIM_Application_OpenConfigWindow, 0);

    {
        ULONG isopen;
        ULONG iconify;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        if(nch->nch_CDC->cdc_HIDCtrlOpen)
        {
            get(nch->nch_HCApp, MUIA_Application_Iconified, &iconify);
            set(nch->nch_HCMainWindow, MUIA_Window_Open, TRUE);
            get(nch->nch_HCMainWindow, MUIA_Window_Open, &isopen);
            if(!(isopen || iconify))
            {
                GM_UNIQUENAME(nHIDCtrlGUITaskCleanup)(nch);
                return;
            }
        }
        sigmask = 0;
        do
        {
            retid = DoMethod(nch->nch_HCApp, MUIM_Application_NewInput, &sigs);
            if(sigs)
            {
                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C)
                {
                    break;
                }
            }
        } while(retid != MUIV_Application_ReturnID_Quit);
        set(nch->nch_HCMainWindow, MUIA_Window_Open, FALSE);
    }
    GM_UNIQUENAME(nHIDCtrlGUITaskCleanup)(nch);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nHIDCtrlGUITaskCleanup()" */
void GM_UNIQUENAME(nHIDCtrlGUITaskCleanup)(struct NepClassHid *nch)
{
    struct NepHidGItem *nhgi;
    if(nch->nch_HCApp)
    {
        MUI_DisposeObject(nch->nch_HCApp);
        nch->nch_HCApp = NULL;
        nch->nch_HCActionObj = NULL;
    }

    nhgi = (struct NepHidGItem *) nch->nch_HCGUIItems.lh_Head;
    while(nhgi->nhgi_Node.ln_Succ)
    {
        Remove(&nhgi->nhgi_Node);
        psdFreeVec(nhgi->nhgi_Name);
        psdFreeVec(nhgi);
        nhgi = (struct NepHidGItem *) nch->nch_HCGUIItems.lh_Head;
    }
    if(nch->nch_HCActionClass)
    {
        MUI_DeleteCustomClass(nch->nch_HCActionClass);
        nch->nch_HCActionClass = NULL;
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
    if(ps)
    {
        CloseLibrary(ps);
        ps = NULL;
    }
    Forbid();
    nch->nch_HCGUITask = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
    --nch->nch_ClsBase->nh_Library.lib_OpenCnt;
}
/* \\\ */

/* /// "nGetGHCUsageName()" */
STRPTR nGetGHCUsageName(struct NepClassHid *nch, ULONG uid)
{
    STRPTR uname;

    uname = nNumToStr(nch, NTS_USAGEID, uid, NULL);
    if(uname)
    {
        return(psdCopyStr(uname));
    }
    uname = nNumToStr(nch, NTS_USAGEPAGE, uid>>16, "unknown");
    return(psdCopyStrFmt("%s (0x%lx)", uname, uid));
}
/* \\\ */

/* /// "nAllocGHCItem()" */
struct NepHidGItem * nAllocGHCItem(struct NepClassHid *nch, struct NepHidItem *nhi, struct List *actionlist, ULONG usageid)
{
    struct NepHidGItem *nhgi;

    if(!(nhgi = psdAllocVec(sizeof(struct NepHidGItem))))
    {
        return(NULL);
    }
    nhgi->nhgi_Item = nhi;
    nhgi->nhgi_ActionList = actionlist;
    if(usageid)
    {
        nhgi->nhgi_Name = nGetGHCUsageName(nch, usageid);
    }
    AddTail(&nch->nch_HCGUIItems, &nhgi->nhgi_Node);
    return(nhgi);
}
/* \\\ */

/* /// "HCActionDispatcher()" */
AROS_UFH3(IPTR, GM_UNIQUENAME(HCActionDispatcher),
          AROS_UFHA(struct IClass *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    struct ActionData *ad = (struct ActionData *) 0xABADCAFE;
    struct NepClassHid *nch = NULL;
    if(msg->MethodID != OM_NEW)
    {
        ad = INST_DATA(cl, obj);
        nch = ad->ad_NCH;
    }
    switch(msg->MethodID)
    {
        case OM_NEW:
            if(!(obj = (Object *) DoSuperMethodA(cl,obj,msg)))
            {
                return(0);
            }
            return((ULONG) obj);

        case MUIM_Action_UpdateHIDCtrl:
        {
            struct NepHidGItem *nhgi = (struct NepHidGItem *) ((struct opSet *) msg)->ops_AttrList;
            switch(nhgi->nhgi_ObjType)
            {
                case NHGIOT_SHOTBUTTON:
                    nhgi->nhgi_Item->nhi_OldValue = 1;
                    nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                    Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
                    while(nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched)
                    {
                        psdDelayMS(10);
                    }
                    nhgi->nhgi_Item->nhi_OldValue = 0;
                    nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                    nch->nch_OutFeatTouched = TRUE;
                    Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
                    break;

                case NHGIOT_TOGGLEBUTTON:
                    get(nhgi->nhgi_GUIObj, MUIA_Selected, &nhgi->nhgi_Item->nhi_OldValue);
                    nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                    nch->nch_OutFeatTouched = TRUE;
                    Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
                    break;

                case NHGIOT_SLIDER:
                case NHGIOT_SLIDERIMM:
                    get(nhgi->nhgi_GUIObj, MUIA_Numeric_Value, &nhgi->nhgi_Item->nhi_OldValue);
                    nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                    nch->nch_OutFeatTouched = TRUE;
                    Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
                    break;
            }
            return(TRUE);
        }

        case MUIM_Action_About:
            MUI_RequestA(nch->nch_HCApp, nch->nch_HCMainWindow, 0, NULL, "Blimey!", "HID Output Control Window", NULL);
            return(TRUE);

        case MUIM_Action_ShowHIDControl:
            set(nch->nch_HCMainWindow, MUIA_Window_Open, TRUE);
            return(TRUE);

        case MUIM_Action_HideHIDControl:
            set(nch->nch_HCMainWindow, MUIA_Window_Open, FALSE);
            return(TRUE);
    }
    return(DoSuperMethodA(cl,obj,msg));
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

