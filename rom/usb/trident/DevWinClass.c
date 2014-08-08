
/*****************************************************************************
** This is the DevWinList custom class, a sub class of Window.mui.
******************************************************************************/

#include "debug.h"

#define USE_INLINE_STDARG
#define __NOLIBBASE__
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/poseidon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/usbclass.h>
#include <proto/icon.h>
#include <proto/utility.h>

#include "Trident.h"
#include "ActionClass.h"
#include "IconListClass.h"
#include "DevWinClass.h"
#include "CfgListClass.h"

extern struct ExecBase *SysBase;
extern struct Library *ps;
extern struct IntuitionBase *IntuitionBase;
extern struct Library *UtilityBase;
/* extern struct IntuitionBase *IntuitionBase; */
extern struct DosLibrary *DOSBase;

#define NewList(list) NEWLIST(list)

static char *overridepowerstrings[] =
{
    "Trust device",
    "Override to bus-powered",
    "Override to self-powered",
    NULL
};


/* /// "AllocIfEntry()" */
struct IfListEntry * AllocIfEntry(struct DevWinData *data, struct Node *pif, BOOL intend)
{
    struct IfListEntry *iflnode;
    iflnode = psdAllocVec(sizeof(struct IfListEntry));
    if(iflnode)
    {
        iflnode->pif = pif;
        AddTail(&data->iflist, &iflnode->node);
    }
    return(iflnode);
}
/* \\\ */

/* /// "FreeIfEntry()" */
void FreeIfEntry(struct DevWinData *data, struct IfListEntry *iflnode)
{
    Remove(&iflnode->node);
    psdFreeVec(iflnode);
}
/* \\\ */

/* /// "InterfaceListDisplayHook()" */
AROS_UFH3(LONG, InterfaceListDisplayHook,
                   AROS_UFHA(struct Hook *, hook, A0),
                   AROS_UFHA(char **, strarr, A2),
                   AROS_UFHA(struct IfListEntry *, iflnode, A1))
{
    AROS_USERFUNC_INIT

    static char buf1[12];
    static char buf3[48];
    static char buf4[8];
    static char buf5[8];
    static char buf6[8];
    struct IconListData *data;
    struct DevWinData *devdata = (struct DevWinData *) hook->h_Data;

    if(iflnode)
    {
        IPTR clsimg;
        IPTR ifnum;
        IPTR ifaltnum;
        IPTR ifclass;
        IPTR ifsubclass;
        IPTR ifproto;
        STRPTR ifname;
        IPTR ifnumeps;
        APTR binding;
        struct Library *bindingcls;
        data = (struct IconListData *) INST_DATA(IconListClass->mcc_Class, devdata->iflvobj);

        psdGetAttrs(PGA_INTERFACE, iflnode->pif,
                    IFA_InterfaceNum, &ifnum,
                    IFA_AlternateNum, &ifaltnum,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &ifsubclass,
                    IFA_Protocol, &ifproto,
                    IFA_InterfaceName, &ifname,
                    IFA_NumEndpoints, &ifnumeps,
                    IFA_Binding, &binding,
                    IFA_BindingClass, &bindingcls,
                    TAG_END);
        psdSafeRawDoFmt(buf1, 12, "%ld/%ld", ifnum, ifaltnum);
        clsimg = 5;
        switch(ifclass)
        {
            case STILLIMG_CLASSCODE:
                clsimg = 22;
                break;
            case BLUETOOTH_CLASSCODE:
                clsimg = 21;
                break;
            case FWUPGRADE_CLASSCODE:
                clsimg = 1;
                break;
            case VENDOR_CLASSCODE:
                clsimg++;
            case SECURITY_CLASSCODE:
                clsimg++;
            case SMARTCARD_CLASSCODE:
                clsimg++;
            case CDCDATA_CLASSCODE:
                clsimg++;
            case HUB_CLASSCODE:
                clsimg++;
            case MASSSTORE_CLASSCODE:
                clsimg++;
            case PRINTER_CLASSCODE:
                clsimg++;
            case PHYSICAL_CLASSCODE:
                clsimg++;
            case HID_CLASSCODE:
                clsimg++;
            case CDCCTRL_CLASSCODE:
                clsimg += 2;
            case AUDIO_CLASSCODE:
                clsimg++;
        }

        psdSafeRawDoFmt(buf3, 48, "\33O[%08lx] %ld (%s)",
                        data->mimainlist[clsimg],
                        ifclass,
                        psdNumToStr(NTS_COMBOCLASS, (ifclass<<NTSCCS_CLASS)|(ifsubclass<<NTSCCS_SUBCLASS)|(ifproto<<NTSCCS_PROTO)|
                                                    NTSCCF_CLASS|NTSCCF_SUBCLASS|NTSCCF_PROTO, "None"));
        psdSafeRawDoFmt(buf4, 8, "%ld", ifsubclass);
        psdSafeRawDoFmt(buf5, 8, "%ld", ifproto);
        psdSafeRawDoFmt(buf6, 8, "%ld", ifnumeps);
        *strarr++ = buf1;
        *strarr++ = ifname;
        *strarr++ = buf3;
        *strarr++ = buf4;
        *strarr++ = buf5;
        *strarr++ = buf6;
        if(binding)
        {
            *strarr = bindingcls->lib_Node.ln_Name;
        } else {
            *strarr = "None";
        }
    } else {
        *strarr++ = "\33l\33uNum";
        *strarr++ = "\33l\33uName";
        *strarr++ = "\33l\33uClass";
        *strarr++ = "\33l\33uSub";
        *strarr++ = "\33l\33uProto";
        *strarr++ = "\33l\33uEPs";
        *strarr   = "\33l\33uBinding";
    }
    return(0);
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "DevWinDispatcher()" */
AROS_UFH3(IPTR, DevWinDispatcher,
          AROS_UFHA(struct IClass *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
    // There should never be an uninitialized pointer, but just in case, try to get an mungwall hit if so.
    struct DevWinData *data = (struct DevWinData *) 0xABADCAFE;

    // on OM_NEW the obj pointer will be void, so don't try to get the data base in this case.
    if(msg->MethodID != OM_NEW) data = INST_DATA(cl,obj);

    switch(msg->MethodID)
    {
        case OM_NEW:
        {
            struct DevListEntry *dlnode;
            struct IfListEntry  *iflnode;
            IPTR devadr;
            IPTR devusbvers;
            IPTR devclass;
            IPTR devsubclass;
            IPTR devproto;
            IPTR devvendorid;
            IPTR devprodid;
            IPTR devversion;
            STRPTR devmanufact;
            STRPTR devprodname = NULL;
            STRPTR devserial;
            STRPTR devidstr;
            STRPTR customname;
            IPTR devcurrlang;
            UWORD *devlangarray;
            IPTR devislowspeed;
            IPTR devishighspeed;
            #ifdef AROS_USB30_CODE
            IPTR devissuperspeed;
            #endif
            IPTR devisconnected;
            IPTR devhasaddress;
            IPTR devhasdevdesc;
            IPTR devisconfigured;
            IPTR devlowpower = 0;
            IPTR devnumcfgs;
            IPTR devdontpopup = 0;
            IPTR noclassbind = 0;
            IPTR overridepower = 0;
            IPTR devpowerdrain = 0;
            IPTR devpowersupply = 0;
            IPTR devhubport = 0;
            struct Node *devhub = NULL;
            STRPTR devhubname = "";

            struct Node *phw = NULL;

            struct List *cfgs;
            struct Node *pc;
            IPTR cfgselfpow;
            IPTR cfgremwake;
            IPTR cfgnum;
            IPTR cfgmaxpower;
            STRPTR cfgname;
            IPTR cfgnumifs;

            struct List *ifs;
            struct Node *pif;
            struct Node *altpif;
            struct List *altiflist;

            APTR binding;
            struct Library *bindingcls;

            STRPTR textbuf1, textbuf2, textbuf3;
            STRPTR devstate;

            Object *oldroot = NULL;

            if(!(obj = (Object *) DoSuperMethodA(cl, obj, msg)))
            {
                return(0);
            }
            data = INST_DATA(cl, obj);
            NewList(&data->iflist);
            data->InterfaceDisplayHook.h_Data = data;
            data->InterfaceDisplayHook.h_Entry = (APTR) InterfaceListDisplayHook;

            dlnode = (struct DevListEntry *) GetTagData(MUIA_DevWin_DevEntry, (IPTR) NULL, ((struct opSet *) msg)->ops_AttrList);

            if(dlnode)
            {
                dlnode->devdata = data;
                data->dlnode = dlnode;
                data->pd = dlnode->pd;
                psdGetAttrs(PGA_DEVICE, dlnode->pd,
                            DA_Address, &devadr,
                            DA_UsbVersion, &devusbvers,
                            DA_Class, &devclass,
                            DA_SubClass, &devsubclass,
                            DA_Protocol, &devproto,
                            DA_VendorID, &devvendorid,
                            DA_ProductID, &devprodid,
                            DA_Version, &devversion,
                            DA_Manufacturer, &devmanufact,
                            DA_OrigProductName, &devprodname,
                            DA_ProductName, &customname,
                            DA_SerialNumber, &devserial,
                            DA_IDString, &devidstr,
                            DA_CurrLangID, &devcurrlang,
                            DA_LangIDArray, &devlangarray,
                            DA_IsLowspeed, &devislowspeed,
                            DA_IsHighspeed, &devishighspeed,
                            #ifdef AROS_USB30_CODE
                            DA_IsSuperspeed, &devissuperspeed,
                            #endif
                            DA_IsConnected, &devisconnected,
                            DA_HasAddress, &devhasaddress,
                            DA_HasDevDesc, &devhasdevdesc,
                            DA_IsConfigured, &devisconfigured,
                            DA_LowPower, &devlowpower,
                            DA_NumConfigs, &devnumcfgs,
                            DA_ConfigList, &cfgs,
                            DA_Binding, &binding,
                            DA_BindingClass, &bindingcls,
                            DA_InhibitPopup, &devdontpopup,
                            DA_InhibitClassBind, &noclassbind,
                            DA_OverridePowerInfo, &overridepower,
                            DA_PowerSupply, &devpowersupply,
                            DA_PowerDrained, &devpowerdrain,
                            DA_AtHubPortNumber, &devhubport,
                            DA_HubDevice, &devhub,
                            DA_Hardware, &phw,
                            TAG_END);
                if(devhub)
                {
                    psdGetAttrs(PGA_DEVICE, devhub,
                                DA_ProductName, &devhubname,
                                TAG_END);
                }

                if(!devprodname) /* backward compatibility */
                {
                    devprodname = customname;
                }
                textbuf1 = psdAllocVec(3*1024);
                textbuf2 = &textbuf1[1*1024];
                textbuf3 = &textbuf1[2*1024];
                if(!textbuf1)
                {
                    return(FALSE);
                }
                psdSafeRawDoFmt(textbuf1, 1024, "%s\n%ld (=0x%04lx)\n%04lx\n%s\n%ld (=0x%04lx)\n%s\n%s\n%ld mA supply / %ldmA drained",
                                devprodname, devprodid, devprodid, devversion, devmanufact, devvendorid, devvendorid,
                                psdNumToStr(NTS_VENDORID, (LONG) devvendorid, "unknown"),
                                devserial,
                                devpowersupply, devpowerdrain);
                if(devisconfigured)
                {
                    devstate = "Configured";
                }
                else if(devhasdevdesc)
                {
                    devstate = "DevDescriptor";
                }
                else if(devhasaddress)
                {
                    devstate = "Valid addr";
                }
                else if(devisconnected)
                {
                    devstate = "Connected";
                } else {
                    devstate = "Dead";
                }

                if( (devclass == HUB_CLASSCODE) && (devhubport == NULL)) {

                    STRPTR cmpdevicename = "";
                    STRPTR devicename = "";
                    IPTR deviceunit = 0;

                    if(phw != NULL) {
                        psdGetAttrs(PGA_HARDWARE, phw, HA_DeviceName, &cmpdevicename, HA_DeviceUnit, &deviceunit, TAG_END);
                        devicename = FilePart(cmpdevicename);
                    }

                    psdSafeRawDoFmt(textbuf2, 1024, "%s%s\n%s\n%ld\nRoot hub of %s unit %ld\n%ld (%s)\n%ld\n%ld\n%04lx",
                                    devstate, (devlowpower ? " (Lowpower)" : ""),
                                    #ifdef AROS_USB30_CODE
                                    (devislowspeed ? "Lowspeed" : (devissuperspeed ? "Superspeed" : (devishighspeed ? "Highspeed" : "Fullspeed"))),
                                    #else
                                    (devislowspeed ? "Lowspeed" : (devishighspeed ? "Highspeed" : "Fullspeed")),
                                    #endif
                                    devadr,
                                    devicename, deviceunit,
                                    devclass,
                                    psdNumToStr(NTS_COMBOCLASS,
                                                (devclass<<NTSCCS_CLASS)|(devsubclass<<NTSCCS_SUBCLASS)|(devproto<<NTSCCS_PROTO)|
                                                NTSCCF_CLASS|NTSCCF_SUBCLASS|NTSCCF_PROTO, "None"),
                                                devsubclass, devproto, devusbvers);
                } else {
                    psdSafeRawDoFmt(textbuf2, 1024, "%s%s\n%s\n%ld\nPort %ld at %s\n%ld (%s)\n%ld\n%ld\n%04lx",
                                    devstate, (devlowpower ? " (Lowpower)" : ""),
                                    #ifdef AROS_USB30_CODE
                                    (devislowspeed ? "Lowspeed" : (devissuperspeed ? "Superspeed" : (devishighspeed ? "Highspeed" : "Fullspeed"))),
                                    #else
                                    (devislowspeed ? "Lowspeed" : (devishighspeed ? "Highspeed" : "Fullspeed")),
                                    #endif
                                    devadr,
                                    devhubport, devhubname,
                                    devclass,
                                    psdNumToStr(NTS_COMBOCLASS,
                                                (devclass<<NTSCCS_CLASS)|(devsubclass<<NTSCCS_SUBCLASS)|(devproto<<NTSCCS_PROTO)|
                                                NTSCCF_CLASS|NTSCCF_SUBCLASS|NTSCCF_PROTO, "None"),
                                                devsubclass, devproto, devusbvers);
                }

                pc = cfgs->lh_Head;
                textbuf3[0] = 0;
                while(pc->ln_Succ)
                {
                    psdGetAttrs(PGA_CONFIG, pc,
                                CA_SelfPowered, &cfgselfpow,
                                CA_RemoteWakeup, &cfgremwake,
                                CA_ConfigNum, &cfgnum,
                                CA_MaxPower, &cfgmaxpower,
                                CA_ConfigName, &cfgname,
                                CA_NumInterfaces, &cfgnumifs,
                                CA_InterfaceList, &ifs,
                                TAG_END);

                    psdSafeRawDoFmt(&textbuf3[strlen(textbuf3)], (ULONG) 1024-strlen(textbuf3),
                                    "Config %ld (%s)\n  Attributes: %s%s\n  MaxPower: %ld mA\n  %ld interface%s.\n",
                                    cfgnum, cfgname,
                                    cfgselfpow ? "self-powered " : "bus-powered ",
                                    cfgremwake ? "remote-wakeup" : "",
                                    cfgmaxpower, cfgnumifs,
                                    (cfgnumifs != 1) ? "s" : "");
                    pc = pc->ln_Succ;
                }

                data->classpopup = CreateClassPopup();
                data->contents = VGroup,
                    Child, HGroup,
                        MUIA_ShortHelp, "These fields show some general information\n"
                                        "on the USB device. Note that strings are not\n"
                                        "mandatory and might be artificially created\n"
                                        "from the product and vendor IDs.",
                        MUIA_FrameTitle, "General device information",
                        Child, LabelB("Product name:\nProduct ID:\nProduct version:\nManufacturer:\nVendor:\n\nSerial #:\nPower State:"),
                        Child, TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_Text_Contents, textbuf1,
                            End,
                        //Child, HSpace(0),
                        Child, LabelB("State:\nSpeed:\nAddress:\nConnected:\nDevClass:\nSubclass:\nProtocol:\nUSB vers:"),
                        Child, TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_Text_Contents, textbuf2,
                            End,
                        End,
                    Child, HGroup,
                        Child, Label("Custom Device Name:"),
                        Child, data->cwnameobj = StringObject,
                            MUIA_ShortHelp, "You can enter a more readable name\n"
                                            "here, if the device does not provide\n"
                                            "a sensible name.",
                            StringFrame,
                            //MUIA_HorizWeight, 10,
                            MUIA_CycleChain, 1,
                            MUIA_String_Contents, customname,
                            MUIA_String_AdvanceOnCR, TRUE,
                            End,
                        Child, HGroup,
                            MUIA_Group_SameWidth, TRUE,
                            Child, data->changenameobj = TextObject,
                                MUIA_ShortHelp, "Click on this button to change the name of the\n"
                                                "name of the device to the new one.",
                                ButtonFrame,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_CycleChain, 1,
                                MUIA_InputMode, MUIV_InputMode_RelVerify,
                                MUIA_Text_Contents, "\33c Change Name ",
                                End,
                            Child, data->resetnameobj = TextObject,
                                MUIA_ShortHelp, "Removes the custom name and restores\n"
                                                "the original one that Poseidon generated.",
                                ButtonFrame,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_CycleChain, 1,
                                MUIA_InputMode, MUIV_InputMode_RelVerify,
                                MUIA_Text_Contents, "\33c Restore Name ",
                                End,
                            End,
                        End,
                    Child, HGroup,
                        Child, Label("Disable class bindings:"),
                        Child, data->noclassbindobj = ImageObject, ImageButtonFrame,
                            MUIA_ShortHelp, "Skips this device during class scan,\n"
                                            "making it available for application bindings.",
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, noclassbind,
                            MUIA_ShowSelState, FALSE,
                            End,
                        Child, HSpace(0),
                        Child, Label("Inhibit popup:"),
                        Child, data->dontpopupobj = ImageObject, ImageButtonFrame,
                            MUIA_ShortHelp, "Inhibits a popup window appearing\n"
                                            "for this particular device.",
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, devdontpopup,
                            MUIA_ShowSelState, FALSE,
                            End,
                        Child, HSpace(0),
                        Child, Label("Power info:"),
                        Child, data->overridepowerobj = CycleObject,
                            MUIA_CycleChain, 1,
                            MUIA_ShortHelp, "Some devices and hubs give wrong information\n"
                                            "about being self-powered, when they're actually\n"
                                            "bus-powered, making me lose my hair.\n"
                                            "Hence, you can override the information the\n"
                                            "device gives about itself, allowing the power\n"
                                            "management to work nicely.",
                            MUIA_Cycle_Entries, overridepowerstrings,
                            MUIA_Cycle_Active, overridepower,
                            End,
                        End,
                    Child, HGroup,
                        Child, VGroup,
                            MUIA_ShortHelp, "This is a list of supported languages\n"
                                            "for the USB device. It's not manadatory\n"
                                            "and in fact cheap devices won't even use\n"
                                            "any string descriptors in their firmware.",
                            Child, data->langlvobj = ListviewObject,
                                MUIA_FrameTitle, "Supported languages",
                                MUIA_Listview_List, ListObject,
                                    ReadListFrame,
                                    End,
                                End,
                            Child, HGroup,
                                Child, LabelB("Current language:"),
                                Child, TextObject,
                                    TextFrame,
                                    MUIA_Background, MUII_TextBack,
                                    MUIA_Text_Contents, psdNumToStr(NTS_LANGID, devcurrlang, "<unknown>"),
                                    End,
                                End,
                            End,
                        Child, HGroup,
                            MUIA_ShortHelp, "USB devices are able to support\n"
                                            "different configurations. However,\n"
                                            "this is really rare.",
                            MUIA_FrameTitle, "Configurations",
                            Child, data->cfglvobj = FloattextObject,
                                ReadListFrame,
                                MUIA_Floattext_Text, textbuf3,
                                End,
                            End,
                        End,
                    Child, HGroup,
                        Child, ListviewObject,
                            MUIA_ShortHelp, "This is a list of so called interfaces\n"
                                            "the device has. USB devices can be built\n"
                                            "as compound, so that each interface has\n"
                                            "different functions. Each interface can be\n"
                                            "bound to a different class.",
                            MUIA_FrameTitle, "Interfaces",
                            MUIA_Listview_List, data->iflvobj =
    NewObject(IconListClass->mcc_Class, 0, MUIA_ContextMenu, data->classpopup, InputListFrame, MUIA_List_MinLineHeight, 16, MUIA_List_Format, "BAR,BAR,BAR,BAR,BAR,BAR,", MUIA_List_Title, TRUE,MUIA_List_DisplayHook, &data->InterfaceDisplayHook, TAG_END),
                            End,
                        End,
                    Child, HGroup,
                        MUIA_Group_SameWidth, TRUE,
                        Child, data->clsscanobj = TextObject,
                            MUIA_ShortHelp, "Clicking on this button will start a class scan. This means that\n"
                                            "each device will be examined if it matches some of the standard\n"
                                            "classes in the system. In this case, a binding will be established\n"
                                            "and the functionality will be added to the system automatically.",
                            ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, "\33c Class Scan ",
                            End,
                        Child, data->unbindobj = TextObject,
                            MUIA_ShortHelp, "Manually removes an interface binding. This can be\n"
                                            "useful to temporarily deactivate a device.\n"
                                            "Use 'Class Scan' to reactivate the binding.",
                            MUIA_Disabled, TRUE,
                            ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, "\33c Release Binding ",
                            End,
                        Child, data->cfgobj = TextObject,
                            MUIA_ShortHelp, "If there is an interface binding and the class\n"
                                            "supports a configuration GUI. Clicking on this\n"
                                            "button will open the interface preferences window.\n\n"
                                            "Note well, that the corrsponding button for the\n"
                                            "device binding settings can be found in the device\n"
                                            "list window.",
                            MUIA_Disabled, TRUE,
                            ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, "\33c Settings ",
                            End,
                        End,
                    End;
                psdFreeVec(textbuf1);
                if(data->contents)
                {
                    set(obj, MUIA_Window_IsSubWindow, FALSE);
                    set(obj, MUIA_Window_Title, devidstr);
                    get(obj, MUIA_Window_RootObject, &oldroot);
                    set(obj, MUIA_Window_RootObject, data->contents);
                    DoMethod(oldroot, OM_DISPOSE);
                    set(obj, MUIA_Window_ID, MAKE_ID('D','I','N','F'));
                    if(devlangarray)
                    {
                        UWORD *wptr = devlangarray;
                        while(*wptr)
                        {
                            DoMethod(data->langlvobj, MUIM_List_InsertSingle,
                                     psdNumToStr(NTS_LANGID, (ULONG) *wptr++, "<unknown>"), MUIV_List_Insert_Bottom);
                        }
                    } else {
                        DoMethod(data->langlvobj, MUIM_List_InsertSingle, "<none>", MUIV_List_Insert_Bottom);
                    }
                    pc = cfgs->lh_Head;
                    while(pc->ln_Succ)
                    {
                        psdGetAttrs(PGA_CONFIG, pc,
                                    CA_InterfaceList, &ifs,
                                    TAG_END);

                        pif = ifs->lh_Head;
                        while(pif->ln_Succ)
                        {
                            iflnode = AllocIfEntry(data, pif, FALSE);
                            if(iflnode)
                            {
                                DoMethod(data->iflvobj, MUIM_List_InsertSingle, iflnode, MUIV_List_Insert_Bottom);
                            }
                            psdGetAttrs(PGA_INTERFACE, pif,
                                        IFA_AlternateIfList, &altiflist,
                                        TAG_END);
                            altpif = altiflist->lh_Head;
                            while(altpif->ln_Succ)
                            {
                                iflnode = AllocIfEntry(data, altpif, TRUE);
                                if(iflnode)
                                {
                                    DoMethod(data->iflvobj, MUIM_List_InsertSingle, iflnode, MUIV_List_Insert_Bottom);
                                }

                                altpif = altpif->ln_Succ;
                            }
                            pif = pif->ln_Succ;
                        }
                        pc = pc->ln_Succ;
                    }

                    DoMethod(data->clsscanobj, MUIM_Notify, MUIA_Pressed, FALSE,
                             obj, 1, MUIM_DevWin_Dev_Bind);
                    DoMethod(data->unbindobj, MUIM_Notify, MUIA_Pressed, FALSE,
                             obj, 1, MUIM_DevWin_If_Unbind);
                    DoMethod(data->cfgobj, MUIM_Notify, MUIA_Pressed, FALSE,
                             obj, 2, MUIM_DevWin_If_Config);
                    DoMethod(data->iflvobj, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
                             obj, 1, MUIM_DevWin_If_Config);
                    DoMethod(data->iflvobj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
                             obj, 1, MUIM_DevWin_If_Activate);
                    DoMethod(data->iflvobj, MUIM_Notify, MUIA_ContextMenuTrigger, MUIV_EveryTime,
                             obj, 2, MUIM_DevWin_If_FBind, MUIV_TriggerValue);
                    DoMethod(data->cwnameobj, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
                             obj, 1, MUIM_DevWin_SetCustomName);
                    DoMethod(data->changenameobj, MUIM_Notify, MUIA_Pressed, FALSE,
                             obj, 1, MUIM_DevWin_SetCustomName);
                    DoMethod(data->resetnameobj, MUIM_Notify, MUIA_Pressed, FALSE,
                             obj, 1, MUIM_DevWin_ResetCustomName);
                    DoMethod(data->dontpopupobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
                             obj, 1, MUIM_DevWin_PopupInhibitChg);
                    DoMethod(data->noclassbindobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
                             obj, 1, MUIM_DevWin_NoClassBindChg);
                    DoMethod(data->overridepowerobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
                             obj, 1, MUIM_DevWin_PowerInfoChg);
                } else {
                    CoerceMethod(cl, obj, OM_DISPOSE);
                    return((IPTR) NULL);
                }
            }
            return((IPTR) obj);
        }

        case OM_DISPOSE:
        {
            struct IfListEntry *iflnode;
            if(data->dlnode)
            {
                if(data->dlnode->infowindow)
                {
                    data->dlnode->infowindow = NULL;
                }
                data->dlnode->devdata = NULL;
            }
            if(data->classpopup)
            {
                DoMethod(data->classpopup, OM_DISPOSE);
                data->classpopup = NULL;
            }
            iflnode = (struct IfListEntry *) data->iflist.lh_Head;
            while(iflnode->node.ln_Succ)
            {
                FreeIfEntry(data, iflnode);
                iflnode = (struct IfListEntry *) data->iflist.lh_Head;
            }
            data->dlnode = NULL;
            break;
        }

        case MUIM_DevWin_Dev_Bind:
            psdClassScan();
            DoMethod(obj, MUIM_DevWin_If_Activate);
            DoMethod(data->iflvobj, MUIM_List_Redraw, MUIV_List_Redraw_All);
            return(TRUE);

        case MUIM_DevWin_If_Activate:
        {
            APTR binding;
            struct Node *puc;
            IPTR hascfggui = FALSE;
            struct Library *UsbClsBase;
            struct IfListEntry *iflnode;

            DoMethod(data->iflvobj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &iflnode);
            if(iflnode)
            {
                psdGetAttrs(PGA_INTERFACE, iflnode->pif,
                            IFA_Binding, &binding,
                            IFA_BindingClass, &puc,
                            TAG_END);
                set(data->unbindobj, MUIA_Disabled, !binding);
                if(binding && puc)
                {
                    psdGetAttrs(PGA_USBCLASS, puc,
                        UCA_ClassBase, &UsbClsBase,
                        TAG_END);
                    usbGetAttrs(UGA_CLASS, NULL,
                        UCCA_HasBindingCfgGUI, &hascfggui,
                        TAG_END);
                    set(data->cfgobj, MUIA_Disabled, !hascfggui);
                } else {
                    set(data->cfgobj, MUIA_Disabled, TRUE);
                }
                set(data->iflvobj, MUIA_ContextMenu, data->classpopup);
            } else {
                set(data->unbindobj, MUIA_Disabled, TRUE);
                set(data->cfgobj, MUIA_Disabled, TRUE);
                set(data->iflvobj, MUIA_ContextMenu, NULL);
            }
            return(TRUE);
        }

        case MUIM_DevWin_If_Unbind:
        {
            APTR binding;
            struct IfListEntry *iflnode;

            DoMethod(data->iflvobj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &iflnode);
            if(iflnode)
            {
                psdGetAttrs(PGA_INTERFACE, iflnode->pif,
                            IFA_Binding, &binding,
                            TAG_END);
                if(binding)
                {
                    psdReleaseIfBinding(iflnode->pif);
                    set(data->unbindobj, MUIA_Disabled, TRUE);
                    set(data->cfgobj, MUIA_Disabled, TRUE);
                    DoMethod(data->iflvobj, MUIM_List_Redraw, MUIV_List_Redraw_All);
                }
            }
            return(TRUE);
        }

        case MUIM_DevWin_If_Config:
        {
            APTR binding;
            struct Node *puc;
            struct Library *UsbClsBase;
            struct IfListEntry *iflnode;

            DoMethod(data->iflvobj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &iflnode);
            if(iflnode)
            {
                psdGetAttrs(PGA_INTERFACE, iflnode->pif,
                            IFA_Binding, &binding,
                            IFA_BindingClass, &puc,
                            TAG_END);
                if(binding && puc)
                {
                    psdGetAttrs(PGA_USBCLASS, puc,
                                UCA_ClassBase, &UsbClsBase,
                                TAG_END);
                    usbDoMethod(UCM_OpenBindingCfgWindow, binding);
                }
            }
            return(TRUE);
        }

        case MUIM_DevWin_If_FBind:
        {
            Object *mi = (Object *) (((IPTR *) msg)[1]);
            STRPTR name = NULL;
            STRPTR devid = NULL;
            STRPTR ifid = NULL;
            STRPTR devname = NULL;
            BOOL clever;
            struct IfListEntry *iflnode;

            DoMethod(data->iflvobj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &iflnode);
            if(iflnode)
            {
                get(mi, MUIA_Menuitem_Title, &name);
                if(!strcmp(name, "None"))
                {
                    name = NULL;
                }
                psdGetAttrs(PGA_DEVICE, data->pd,
                            DA_ProductName, &devname,
                            DA_IDString, &devid,
                            TAG_END);
                psdGetAttrs(PGA_INTERFACE, iflnode->pif,
                            IFA_IDString, &ifid,
                            TAG_END);
                if(name)
                {
                    clever = MUI_RequestA(_app(obj), obj, 0, NULL, "I'm not dumb!|I'll reconsider",
                                         "You are about to establish a forced \33binterface\33n\n"
                                         "binding. As most people are not capable of reading the\n"
                                         "manual and they cause more harm than good,\n"
                                         "please make sure you know, what you're doing\n"
                                         "and not breaking things (and then bugger me with\n"
                                         "silly emails).", NULL);
                    if(!clever)
                    {
                        return(FALSE);
                    }
                }
                if(psdSetForcedBinding(name, devid, ifid))
                {
                    if(name)
                    {
                        psdAddErrorMsg(RETURN_OK, "Trident", "Forcing interface binding of %s to %s.", devname, name);
                    } else {
                        psdAddErrorMsg(RETURN_OK, "Trident", "Removed forced interface binding of %s.", devname);
                    }
                }
            }

            return(TRUE);
        }

        case MUIM_DevWin_SetCustomName:
        {
            APTR pic;
            STRPTR devidstr = NULL;
            STRPTR oldname = NULL;
            CONST_STRPTR newname = "";
            STRPTR newnewname;
            psdGetAttrs(PGA_DEVICE, data->pd,
                        DA_IDString, &devidstr,
                        DA_ProductName, &oldname,
                        TAG_END);
            if(!devidstr)
            {
                return(FALSE);
            }
            get(data->cwnameobj, MUIA_String_Contents, &newname);
            if(!(*newname))
            {
                newname = "Empty";
            }
            if(oldname && !strcmp(newname, oldname))
            {
                return(FALSE);
            }
            pic = psdGetUsbDevCfg("Trident", devidstr, NULL);
            if(!pic)
            {
                psdSetUsbDevCfg("Trident", devidstr, NULL, NULL);
                pic = psdGetUsbDevCfg("Trident", devidstr, NULL);
            }
            if(pic)
            {
                if(psdAddStringChunk(pic, IFFCHNK_NAME, newname))
                {
                    psdAddErrorMsg(RETURN_OK, "Trident", "Set new custom name '%s'.", newname);
                    newnewname = psdCopyStr(newname);
                    if(newnewname)
                    {
                        psdSetAttrs(PGA_DEVICE, data->pd,
                            DA_ProductName, newnewname,
                            TAG_END);
                        if (oldname)
                            psdFreeVec(oldname);
                    }
                }
            }
            set(data->cwnameobj, MUIA_String_Contents, newname);
            DoMethod(data->dlnode->adata->devlistobj, MUIM_List_Redraw, MUIV_List_Redraw_All);
            return(TRUE);
        }

        case MUIM_DevWin_ResetCustomName:
        {
            APTR pic;
            STRPTR devidstr = NULL;
            STRPTR oldname = NULL;
            STRPTR origname = NULL;
            STRPTR newname;
            psdGetAttrs(PGA_DEVICE, data->pd,
                        DA_IDString, &devidstr,
                        DA_ProductName, &oldname,
                        DA_OrigProductName, &origname,
                        TAG_END);
            if(!devidstr)
            {
                return(FALSE);
            }
            pic = psdGetUsbDevCfg("Trident", devidstr, NULL);
            if(pic)
            {
                if(psdRemCfgChunk(pic, IFFCHNK_NAME))
                {
                    psdAddErrorMsg(RETURN_OK, "Trident", "Custom name '%s' removed.", oldname);
                    if(origname)
                    {
                        newname = psdCopyStr(origname);
                        if(newname)
                        {
                            psdSetAttrs(PGA_DEVICE, data->pd,
                                DA_ProductName, newname,
                                TAG_END);
                            psdFreeVec(oldname);
                        }
                    }
                }
            }
            psdGetAttrs(PGA_DEVICE, data->pd,
                        DA_ProductName, &oldname,
                        TAG_END);
            set(data->cwnameobj, MUIA_String_Contents, oldname);
            DoMethod(data->dlnode->adata->devlistobj, MUIM_List_Redraw, MUIV_List_Redraw_All);
            return(TRUE);
        }

        case MUIM_DevWin_PopupInhibitChg:
        case MUIM_DevWin_NoClassBindChg:
        case MUIM_DevWin_PowerInfoChg:
        {
            IPTR dontpopup = 0;
            IPTR noclassbind = 0;
            IPTR overridepower = 0;
            get(data->dontpopupobj, MUIA_Selected, &dontpopup);
            get(data->noclassbindobj, MUIA_Selected, &noclassbind);
            get(data->overridepowerobj, MUIA_Cycle_Active, &overridepower);

            psdSetAttrs(PGA_DEVICE, data->pd,
                        DA_InhibitPopup, dontpopup,
                        DA_InhibitClassBind, noclassbind,
                        DA_OverridePowerInfo, overridepower,
                        TAG_END);
            return(TRUE);
        }

    }
    return(DoSuperMethodA(cl,obj,msg));
    AROS_USERFUNC_EXIT
}
/* \\\ */
