/*
 * GUI
 */

#include <aros/isoascii.h>
#include "debug.h"
#include "numtostr.h"

#include "bthid.h"

extern const STRPTR GM_UNIQUENAME(libname);

/* /// "bHIDCtrlGUITask()" */
AROS_UFH0(void, GM_UNIQUENAME(bHIDCtrlGUITask))
{
    AROS_USERFUNC_INIT
    
    struct Task *thistask;
    struct BTHidBase *nh;
    struct BTHidBinding *nhb;

    thistask = FindTask(NULL);

#undef BluetoothBase
#define BluetoothBase nhb->nhb_HCBtBase
#undef IntuitionBase
#define IntuitionBase nhb->nhb_HCIntBase
#undef MUIMasterBase
#define MUIMasterBase nhb->nhb_HCMUIBase

    nhb = thistask->tc_UserData;
    nhb->nhb_HCGUITask = thistask;
    nh = nhb->nhb_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    NewList(&nhb->nhb_HCGUIItems);
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        GM_UNIQUENAME(bHIDCtrlGUITaskCleanup)(nhb);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        GM_UNIQUENAME(bHIDCtrlGUITaskCleanup)(nhb);
        return;
    }

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        KPRINTF(10, ("Couldn't open bluetooth.library.\n"));
        GM_UNIQUENAME(bHIDCtrlGUITaskCleanup)(nhb);
        return;
    }

    nhb->nhb_HCActionClass = MUI_CreateCustomClass(NULL, MUIC_Area  , NULL, sizeof(struct ActionData), GM_UNIQUENAME(HCActionDispatcher));

    if(!nhb->nhb_HCActionClass)
    {
        KPRINTF(10, ("Couldn't create ActionClass.\n"));
        GM_UNIQUENAME(bHIDCtrlGUITaskCleanup)(nhb);
        return;
    }
    nhb->nhb_HCApp = ApplicationObject,
        MUIA_Application_Title      , (IPTR)nhb->nhb_CDC->cdc_HIDCtrlTitle,
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)ISOASCII_COPYRIGHT "2002-2009 Chris Hodges",
        MUIA_Application_Author     , (IPTR)"Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"HID Device Output Control",
        MUIA_Application_Base       , (IPTR)nhb->nhb_CDC->cdc_HIDCtrlRexx,
        MUIA_Application_Menustrip  , (IPTR)MenustripObject,
            Child, (IPTR)MenuObjectT((IPTR)"Project"),
                Child, (IPTR)(nhb->nhb_HCAboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"About...",
                    MUIA_Menuitem_Shortcut, (IPTR)"?",
                    End),
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                    End,
                Child, (IPTR)(nhb->nhb_HCCloseMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Hide",
                    MUIA_Menuitem_Shortcut, (IPTR)"H",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                Child, (IPTR)(nhb->nhb_HCMUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR)"M",
                    End),
                End,
            End,

        SubWindow, (IPTR)(nhb->nhb_HCMainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('H','C','T','L'),
            MUIA_Window_Title, (IPTR)nhb->nhb_CDC->cdc_HIDCtrlTitle,
            MUIA_HelpNode, (IPTR)GM_UNIQUENAME(libname),

            WindowContents, (IPTR)VGroup,
                Child, (IPTR)(nhb->nhb_HCActionObj = NewObject(nhb->nhb_HCActionClass->mcc_Class, 0, MUIA_ShowMe, FALSE, TAG_END)),
                Child, (IPTR)(nhb->nhb_HCGroupObj = ColGroup(4),
                    End),
                Child, (IPTR)(nhb->nhb_HCCloseObj = TextObject, ButtonFrame,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_CycleChain, 1,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_Text_Contents, (IPTR)"\33c Hide ",
                    End),
                End,
            End),
        End;

    if(!nhb->nhb_HCApp)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        GM_UNIQUENAME(bHIDCtrlGUITaskCleanup)(nhb);
        return;
    }

    {
        struct ActionData *ad = INST_DATA(nhb->nhb_HCActionClass->mcc_Class, nhb->nhb_HCActionObj);
        ad->ad_NCH = nhb;
    }
    /* add items */
    {
        struct BtHidReport *nhr;
        struct BtHidItem **nhiptr;
        struct BtHidItem *nhi;
        struct BtHidGItem *nhgi;
        UWORD count;
        Object *obj;
        UWORD numobj = 0;

        nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
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
                                    if((nhgi = bAllocGHCItem(nhb, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* one shot */
                                        obj = VGroup,
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)(nhgi->nhgi_GUIObj = TextObject, ButtonFrame,
                                                MUIA_Background, MUII_ButtonBack,
                                                MUIA_CycleChain, 1,
                                                MUIA_InputMode, MUIV_InputMode_RelVerify,
                                                MUIA_Text_PreParse, (IPTR)"\33c",
                                                MUIA_Text_Contents, (IPTR)nhgi->nhgi_Name,
                                                End),
                                            Child, (IPTR)VSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SHOTBUTTON;
                                    }
                                }
                                else if(nhi->nhi_LogicalMin < 0)
                                {
                                    if((nhgi = bAllocGHCItem(nhb, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* Horizontal slider */
                                        obj = VGroup,
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR)nhgi->nhgi_Name),
                                                Child, (IPTR)(nhgi->nhgi_GUIObj = SliderObject, SliderFrame,
                                                    MUIA_Slider_Horiz, TRUE,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_InputMode, MUIV_InputMode_Immediate,
                                                    MUIA_Numeric_Min, nhi->nhi_LogicalMin,
                                                    MUIA_Numeric_Max, nhi->nhi_LogicalMax,
                                                    MUIA_Numeric_Value, nhi->nhi_OldValue,
                                                    End),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SLIDERIMM;
                                    }
                                } else {
                                    if((nhgi = bAllocGHCItem(nhb, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* Vertical slider */
                                        obj = HGroup,
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)VGroup,
                                                Child, (IPTR)(nhgi->nhgi_GUIObj = SliderObject, SliderFrame,
                                                    MUIA_Slider_Horiz, FALSE,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_InputMode, MUIV_InputMode_Immediate,
                                                    MUIA_Numeric_Min, nhi->nhi_LogicalMin,
                                                    MUIA_Numeric_Max, nhi->nhi_LogicalMax,
                                                    MUIA_Numeric_Value, nhi->nhi_OldValue,
                                                    End),
                                                Child, (IPTR)Label((IPTR)nhgi->nhgi_Name),
                                                End,
                                            Child, (IPTR)HSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SLIDERIMM;
                                    }
                                }
                            } else {
                                if((nhi->nhi_LogicalMin == 0) && (nhi->nhi_LogicalMax == 1))
                                {
                                    if((nhgi = bAllocGHCItem(nhb, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* toggle button */
                                        obj = VGroup,
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)(nhgi->nhgi_GUIObj = TextObject, ButtonFrame,
                                                MUIA_Background, MUII_ButtonBack,
                                                MUIA_CycleChain, 1,
                                                MUIA_InputMode, MUIV_InputMode_Toggle,
                                                MUIA_Text_PreParse, (IPTR)"\33c",
                                                MUIA_Text_Contents, (IPTR)nhgi->nhgi_Name,
                                                End),
                                            Child, (IPTR)VSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_TOGGLEBUTTON;
                                    }
                                }
                                else if(nhi->nhi_LogicalMin < 0)
                                {
                                    if((nhgi = bAllocGHCItem(nhb, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* Horizontal slider */
                                        obj = VGroup,
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR)nhgi->nhgi_Name),
                                                Child, (IPTR)(nhgi->nhgi_GUIObj = SliderObject, SliderFrame,
                                                    MUIA_Slider_Horiz, TRUE,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Numeric_Min, nhi->nhi_LogicalMin,
                                                    MUIA_Numeric_Max, nhi->nhi_LogicalMax,
                                                    MUIA_Numeric_Value, nhi->nhi_OldValue,
                                                    End),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SLIDER;
                                    }
                                } else {
                                    if((nhgi = bAllocGHCItem(nhb, nhi, NULL, nhi->nhi_Usage)))
                                    {
                                        /* Vertical slider */
                                        obj = HGroup,
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)VGroup,
                                                Child, (IPTR)(nhgi->nhgi_GUIObj = SliderObject, SliderFrame,
                                                    MUIA_Slider_Horiz, FALSE,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Numeric_Min, nhi->nhi_LogicalMin,
                                                    MUIA_Numeric_Max, nhi->nhi_LogicalMax,
                                                    MUIA_Numeric_Value, nhi->nhi_OldValue,
                                                    End),
                                                Child, (IPTR)Label(nhgi->nhgi_Name),
                                                End,
                                            Child, (IPTR)HSpace(0),
                                            End;
                                        nhgi->nhgi_ObjType = NHGIOT_SLIDER;
                                    }
                                }
                            }
                        }
                        if(obj)
                        {
                            DoMethod(nhb->nhb_HCGroupObj, OM_ADDMEMBER, obj);
                            switch(nhgi->nhgi_ObjType)
                            {
                                case NHGIOT_SHOTBUTTON:
                                    DoMethod(nhgi->nhgi_GUIObj, MUIM_Notify, MUIA_Pressed, FALSE,
                                             nhb->nhb_HCActionObj, 2, MUIM_Action_UpdateHIDCtrl, nhgi);
                                    break;

                                case NHGIOT_TOGGLEBUTTON:
                                    DoMethod(nhgi->nhgi_GUIObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
                                             nhb->nhb_HCActionObj, 2, MUIM_Action_UpdateHIDCtrl, nhgi);
                                    break;

                                case NHGIOT_SLIDERIMM:
                                    DoMethod(nhgi->nhgi_GUIObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
                                             nhb->nhb_HCActionObj, 2, MUIM_Action_UpdateHIDCtrl, nhgi);
                                    break;

                                case NHGIOT_SLIDER:
                                    DoMethod(nhgi->nhgi_GUIObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
                                             nhb->nhb_HCActionObj, 2, MUIM_Action_UpdateHIDCtrl, nhgi);
                                    break;
                            }
                            numobj++;
                        }
                    } while(--count);
                }
            }
            nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
        }
        if(!numobj)
        {
            DoMethod(nhb->nhb_HCGroupObj, OM_ADDMEMBER, Label("No output items in this interface!"));
        }
    }

    DoMethod(nhb->nhb_HCMainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             nhb->nhb_HCActionObj, 1, MUIM_Action_HideHIDControl);
    DoMethod(nhb->nhb_HCCloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_HCActionObj, 1, MUIM_Action_HideHIDControl);

    DoMethod(nhb->nhb_HCAboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_HCActionObj, 1, MUIM_Action_About);
    DoMethod(nhb->nhb_HCCloseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_HCActionObj, 1, MUIM_Action_HideHIDControl);
    DoMethod(nhb->nhb_HCMUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_HCApp, 2, MUIM_Application_OpenConfigWindow, 0);

    {
        IPTR  isopen = 0;
        IPTR  iconify = 0;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        if(nhb->nhb_CDC->cdc_HIDCtrlOpen)
        {
            get(nhb->nhb_HCApp, MUIA_Application_Iconified, &iconify);
            set(nhb->nhb_HCMainWindow, MUIA_Window_Open, TRUE);
            get(nhb->nhb_HCMainWindow, MUIA_Window_Open, &isopen);
            if(!(isopen || iconify))
            {
                GM_UNIQUENAME(bHIDCtrlGUITaskCleanup)(nhb);
                return;
            }
        }
        sigmask = 0;
        do
        {
            retid = DoMethod(nhb->nhb_HCApp, MUIM_Application_NewInput, &sigs);
            if(sigs)
            {
                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C)
                {
                    break;
                }
            }
        } while(retid != MUIV_Application_ReturnID_Quit);
        set(nhb->nhb_HCMainWindow, MUIA_Window_Open, FALSE);
    }
    GM_UNIQUENAME(bHIDCtrlGUITaskCleanup)(nhb);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "bHIDCtrlGUITaskCleanup()" */
void GM_UNIQUENAME(bHIDCtrlGUITaskCleanup)(struct BTHidBinding *nhb)
{
    struct BtHidGItem *nhgi;
    if(nhb->nhb_HCApp)
    {
        MUI_DisposeObject(nhb->nhb_HCApp);
        nhb->nhb_HCApp = NULL;
        nhb->nhb_HCActionObj = NULL;
    }

    nhgi = (struct BtHidGItem *) nhb->nhb_HCGUIItems.lh_Head;
    while(nhgi->nhgi_Node.ln_Succ)
    {
        Remove(&nhgi->nhgi_Node);
        btFreeVec(nhgi->nhgi_Name);
        btFreeVec(nhgi);
        nhgi = (struct BtHidGItem *) nhb->nhb_HCGUIItems.lh_Head;
    }
    if(nhb->nhb_HCActionClass)
    {
        MUI_DeleteCustomClass(nhb->nhb_HCActionClass);
        nhb->nhb_HCActionClass = NULL;
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
    if(BluetoothBase)
    {
        CloseLibrary(BluetoothBase);
        BluetoothBase = NULL;
    }
    Forbid();
    nhb->nhb_HCGUITask = NULL;
    if(nhb->nhb_ReadySigTask)
    {
        Signal(nhb->nhb_ReadySigTask, 1L<<nhb->nhb_ReadySignal);
    }
    --nhb->nhb_ClsBase->nh_Library.lib_OpenCnt;
}
/* \\\ */

/* /// "bGetGHCUsageName()" */
STRPTR bGetGHCUsageName(struct BTHidBinding *nhb, ULONG uid)
{
    STRPTR uname;

    uname = bNumToStr(nhb, NTS_USAGEID, uid, NULL);
    if(uname)
    {
        return(btCopyStr(uname));
    }
    uname = bNumToStr(nhb, NTS_USAGEPAGE, uid>>16, "unknown");
    return(btCopyStrFmt("%s (0x%lx)", uname, uid));
}
/* \\\ */

/* /// "bAllocGHCItem()" */
struct BtHidGItem * bAllocGHCItem(struct BTHidBinding *nhb, struct BtHidItem *nhi, struct List *actionlist, ULONG usageid)
{
    struct BtHidGItem *nhgi;

    if(!(nhgi = btAllocVec(sizeof(struct BtHidGItem))))
    {
        return(NULL);
    }
    nhgi->nhgi_Item = nhi;
    nhgi->nhgi_ActionList = actionlist;
    if(usageid)
    {
        nhgi->nhgi_Name = bGetGHCUsageName(nhb, usageid);
    }
    AddTail(&nhb->nhb_HCGUIItems, &nhgi->nhgi_Node);
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
    struct BTHidBinding *nhb = NULL;
    if(msg->MethodID != OM_NEW)
    {
        ad = INST_DATA(cl, obj);
        nhb = ad->ad_NCH;
    }
    switch(msg->MethodID)
    {
        case OM_NEW:
            if(!(obj = (Object *) DoSuperMethodA(cl,obj,msg)))
            {
                return(0);
            }
            return (IPTR)obj;

        case MUIM_Action_UpdateHIDCtrl:
        {
            struct BtHidGItem *nhgi = (struct BtHidGItem *) ((struct opSet *) msg)->ops_AttrList;
            switch(nhgi->nhgi_ObjType)
            {
                case NHGIOT_SHOTBUTTON:
                    nhgi->nhgi_Item->nhi_OldValue = 1;
                    nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                    Signal(nhb->nhb_Task, (1L<<nhb->nhb_TaskMsgPort->mp_SigBit));
                    while(nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched)
                    {
                        btDelayMS(10);
                    }
                    nhgi->nhgi_Item->nhi_OldValue = 0;
                    nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                    nhb->nhb_OutFeatTouched = TRUE;
                    Signal(nhb->nhb_Task, (1L<<nhb->nhb_TaskMsgPort->mp_SigBit));
                    break;

                case NHGIOT_TOGGLEBUTTON:
                    get(nhgi->nhgi_GUIObj, MUIA_Selected, &nhgi->nhgi_Item->nhi_OldValue);
                    nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                    nhb->nhb_OutFeatTouched = TRUE;
                    Signal(nhb->nhb_Task, (1L<<nhb->nhb_TaskMsgPort->mp_SigBit));
                    break;

                case NHGIOT_SLIDER:
                case NHGIOT_SLIDERIMM:
                    get(nhgi->nhgi_GUIObj, MUIA_Numeric_Value, &nhgi->nhgi_Item->nhi_OldValue);
                    nhgi->nhgi_Item->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                    nhb->nhb_OutFeatTouched = TRUE;
                    Signal(nhb->nhb_Task, (1L<<nhb->nhb_TaskMsgPort->mp_SigBit));
                    break;
            }
            return(TRUE);
        }

        case MUIM_Action_About:
            MUI_RequestA(nhb->nhb_HCApp, nhb->nhb_HCMainWindow, 0, NULL, "Blimey!", "HID Output Control Window", NULL);
            return(TRUE);

        case MUIM_Action_ShowHIDControl:
            set(nhb->nhb_HCMainWindow, MUIA_Window_Open, TRUE);
            return(TRUE);

        case MUIM_Action_HideHIDControl:
            set(nhb->nhb_HCMainWindow, MUIA_Window_Open, FALSE);
            return(TRUE);
    }
    return(DoSuperMethodA(cl,obj,msg));
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

