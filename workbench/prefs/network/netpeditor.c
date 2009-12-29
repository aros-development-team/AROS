/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
 */

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/prefhdr.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <string.h>
#include <stdio.h>

#include "misc.h"
#include "locale.h"
#include "netpeditor.h"
#include "prefsdata.h"

#include <proto/alib.h>
#include <utility/hooks.h>

static CONST_STRPTR NetworkTabs[] = { NULL, NULL, NULL };
static CONST_STRPTR DHCPCycle[] = { NULL, NULL, NULL };

static struct Hook  netpeditor_displayHook,
                    netpeditor_constructHook,
                    netpeditor_destructHook;

/*** Instance Data **********************************************************/
struct NetPEditor_DATA
{
    // Main window
    Object  *netped_interfaceList,
            *netped_DHCPState,
            *netped_gateString,
            *netped_DNSString[2],
            *netped_hostString,
            *netped_domainString,
            *netped_Autostart,
            *netped_addButton,
            *netped_editButton,
            *netped_removeButton,
            *netped_inputGroup;

    // Interface window
    Object  *netped_ifWindow,
            *netped_upState,
            *netped_ifDHCPState,
            *netped_nameString,
            *netped_deviceString,
            *netped_unitString,
            *netped_IPString,
            *netped_maskString,
            *netped_applyButton,
            *netped_closeButton;
};

AROS_UFH3S(APTR, constructFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(struct Interface *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct Interface *new;

    if ((new = AllocPooled(pool, sizeof(*new))))
    {
        *new = *entry;
    }
    return new;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, destructFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(struct Interface *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct Interface));

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(LONG, displayFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(char **, array, A2),
AROS_UFHA(struct Interface *, entry, A1))
{
    AROS_USERFUNC_INIT
    if (entry)
    {
        static char unitbuffer[20];
        sprintf(unitbuffer, "%d", entry->unit);
        *array++ = entry->name;
        *array++ = entry->up ? "*" : "";
        *array++ = entry->ifDHCP ? (STRPTR)"DHCP" : entry->IP;
        *array++ = entry->ifDHCP ? (STRPTR)"DHCP" : entry->mask;
        *array++ = entry->device;
        *array   = unitbuffer;
    }
    else
    {
        *array++ = (STRPTR)_(MSG_IFNAME);
        *array++ = (STRPTR)_(MSG_UP);
        *array++ = (STRPTR)_(MSG_IP);
        *array++ = (STRPTR)_(MSG_MASK);
        *array++ = (STRPTR)_(MSG_DEVICE);
        *array   = (STRPTR)_(MSG_UNIT);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

BOOL Gadgets2NetworkPrefs(struct NetPEditor_DATA *data)
{
    STRPTR str = NULL;
    IPTR lng = 0;
    LONG i;

    LONG entries = XGET(data->netped_interfaceList, MUIA_List_Entries);
    for(i = 0; i < entries; i++)
    {
        struct Interface *iface = GetInterface(i);
        struct Interface *ifaceentry;
        DoMethod
        (
            data->netped_interfaceList,
            MUIM_List_GetEntry, i, &ifaceentry
        );
        SetName(iface, ifaceentry->name);
        SetUp(iface, ifaceentry->up);
        SetIfDHCP(iface, ifaceentry->ifDHCP);
        SetDevice(iface, ifaceentry->device);
        SetUnit(iface, ifaceentry->unit);
        SetIP(iface, ifaceentry->IP);
        SetMask(iface, ifaceentry->mask);
    }
    SetInterfaceCount(entries);

    GET(data->netped_gateString, MUIA_String_Contents, &str);
    SetGate(str);
    GET(data->netped_DNSString[0], MUIA_String_Contents, &str);
    SetDNS(0, str);
    GET(data->netped_DNSString[1], MUIA_String_Contents, &str);
    SetDNS(1, str);
    GET(data->netped_hostString, MUIA_String_Contents, &str);
    SetHost(str);
    GET(data->netped_domainString, MUIA_String_Contents, &str);
    SetDomain(str);
    GET(data->netped_Autostart, MUIA_Selected, &lng);
    SetAutostart(lng);
    GET(data->netped_DHCPState, MUIA_Cycle_Active, &lng);
    SetDHCP(lng);

    return TRUE;
}

BOOL NetworkPrefs2Gadgets
(
    struct NetPEditor_DATA *data
)
{
    LONG i;
    LONG entries = GetInterfaceCount();
    SET(data->netped_interfaceList, MUIA_List_Quiet, TRUE);
    DoMethod(data->netped_interfaceList, MUIM_List_Clear);
    for(i = 0; i < entries; i++)
    {
        struct Interface *iface = GetInterface(i);
        struct Interface ifaceentry;

        SetInterface
        (
            &ifaceentry,
            GetName(iface),
            GetIfDHCP(iface),
            GetIP(iface),
            GetMask(iface),
            GetDevice(iface),
            GetUnit(iface),
            GetUp(iface)
        );

        DoMethod
        (
            data->netped_interfaceList,
            MUIM_List_InsertSingle, &ifaceentry, MUIV_List_Insert_Bottom
        );
    }

    SET(data->netped_interfaceList, MUIA_List_Quiet, FALSE);

    NNSET(data->netped_gateString, MUIA_String_Contents, (IPTR)GetGate());
    NNSET(data->netped_DNSString[0], MUIA_String_Contents, (IPTR)GetDNS(0));
    NNSET(data->netped_DNSString[1], MUIA_String_Contents, (IPTR)GetDNS(1));
    NNSET(data->netped_hostString, MUIA_String_Contents, (IPTR)GetHost());
    NNSET(data->netped_domainString, MUIA_String_Contents, (IPTR)GetDomain());
    NNSET(data->netped_Autostart, MUIA_Selected, (IPTR)GetAutostart());
    NNSET(data->netped_DHCPState, MUIA_Cycle_Active, (IPTR)GetDHCP() ? 1 : 0);

    return TRUE;
}

void DisplayErrorMessage(Object * obj, enum ErrorCode errorcode)
{
    CONST_STRPTR errormessage = NULL;
    CONST_STRPTR additionaldata = NULL;
    Object * app = NULL;
    Object * wnd = NULL;

    GET(obj, MUIA_ApplicationObject, &app);
    GET(obj, MUIA_Window_Window, &wnd);

    switch(errorcode)
    {
        case UNKNOWN_ERROR:
            errormessage = _(MSG_ERR_UNKNOWN_ERROR);
            break;
        case NOT_RESTARTED_STACK:
            errormessage = _(MSG_ERR_NOT_RESTARTED_STACK);
            break;
        case NOT_SAVED_PREFS_ENV:
            errormessage = _(MSG_ERR_NOT_SAVED_PREFS);
            additionaldata = PREFS_PATH_ENV;
            break;
        case NOT_SAVED_PREFS_ENVARC:
            errormessage = _(MSG_ERR_NOT_SAVED_PREFS);
            additionaldata = PREFS_PATH_ENVARC;
            break;
        case NOT_COPIED_FILES_ENV:
            errormessage = _(MSG_ERR_NOT_COPIED_FILES);
            additionaldata = PREFS_PATH_ENV;
            break;
        case NOT_COPIED_FILES_ENVARC:
            errormessage = _(MSG_ERR_NOT_COPIED_FILES);
            additionaldata = PREFS_PATH_ENVARC;
            break;
        case ALL_OK:
            return;
    }

    MUI_Request(app, wnd, 0, _(MSG_ERROR_TITLE), _(MSG_BUTTON_OK), errormessage, additionaldata);
}

/*** Methods ****************************************************************/
Object * NetPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    // main window
    Object  *gateString, *DNSString[2], *hostString, *domainString,
            *autostart, *interfaceList, *DHCPState,
            *addButton, *editButton, *removeButton, *inputGroup;

    // inferface window
    Object  *deviceString, *IPString, *maskString,
            *ifDHCPState, *unitString, *nameString, *upState,
            *ifWindow, *applyButton, *closeButton;

    DHCPCycle[0] = _(MSG_IP_MODE_MANUAL);
    DHCPCycle[1] = _(MSG_IP_MODE_DHCP);

    NetworkTabs[0] = _(MSG_TAB_IP_CONFIGURATION);
    NetworkTabs[1] = _(MSG_TAB_COMPUTER_NAME);

    netpeditor_constructHook.h_Entry = (HOOKFUNC)constructFunc;
    netpeditor_destructHook.h_Entry = (HOOKFUNC)destructFunc;
    netpeditor_displayHook.h_Entry = (HOOKFUNC)displayFunc;

    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_PrefsEditor_Name, __(MSG_NAME),
        MUIA_PrefsEditor_Path, (IPTR)"AROSTCP/arostcp.prefs",

        Child, RegisterGroup((IPTR)NetworkTabs),
            Child, VGroup,
                Child, (IPTR)(HGroup,
                    GroupFrame,
                    Child, ListviewObject,
                        MUIA_Listview_List, (IPTR)(interfaceList = (Object *)ListObject,
                            ReadListFrame,
                            MUIA_List_Title, TRUE,
                            MUIA_List_Format, (IPTR)"BAR,P=\33c BAR,BAR,BAR,BAR,",
                            MUIA_List_ConstructHook, (IPTR)&netpeditor_constructHook,
                            MUIA_List_DestructHook, (IPTR)&netpeditor_destructHook,
                            MUIA_List_DisplayHook, (IPTR)&netpeditor_displayHook,
                        End),
                    End,
                    Child, (IPTR)(VGroup,
                        MUIA_HorizWeight, 0,
                        Child, (IPTR)(addButton = SimpleButton(_(MSG_BUTTON_ADD))),
                        Child, (IPTR)(editButton = SimpleButton(_(MSG_BUTTON_EDIT))),
                        Child, (IPTR)(removeButton = SimpleButton(_(MSG_BUTTON_REMOVE))),
                        Child, (IPTR)HVSpace,
                    End),
                End),
                Child, (IPTR)(inputGroup = (Object *)ColGroup(2),
                    GroupFrame,
                    Child, (IPTR)Label2(__(MSG_IP_MODE)),
                    Child, (IPTR)(DHCPState = (Object *)CycleObject,
                        MUIA_Cycle_Entries, (IPTR)DHCPCycle,
                    End),
                    Child, (IPTR)Label2(__(MSG_GATE)),
                    Child, (IPTR)(gateString = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)IPCHARS,
                        MUIA_CycleChain, 1,
                    End),
                    Child, (IPTR)Label2(__(MSG_DNS1)),
                    Child, (IPTR)(DNSString[0] = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)IPCHARS,
                        MUIA_CycleChain, 1,
                    End),
                    Child, (IPTR)Label2(__(MSG_DNS2)),
                    Child, (IPTR)(DNSString[1] = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)IPCHARS,
                        MUIA_CycleChain, 1,
                    End),
                End),
                Child, (IPTR)ColGroup(2),
                    Child, (IPTR)Label2(__(MSG_AUTOSTART_STACK)),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(autostart = MUI_MakeObject(MUIO_Checkmark, NULL)),
                        Child, (IPTR)HVSpace,
                    End,
                End,
            End,

            Child, (IPTR)ColGroup(2),
                Child, (IPTR)Label2(__(MSG_HOST_NAME)),
                Child, (IPTR)(hostString = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)NAMECHARS,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(__(MSG_DOMAIN_NAME)),
                Child, (IPTR)(domainString = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)NAMECHARS,
                    MUIA_CycleChain, 1,
                End),
            End,

        End, // register

        TAG_DONE
    );

    ifWindow = (Object *)WindowObject,
        MUIA_Window_Title, __(MSG_IFWINDOW_TITLE),
        MUIA_Window_ID, MAKE_ID('I', 'P', 'W', 'I'),
        MUIA_Window_CloseGadget, FALSE,
        WindowContents, VGroup,
            GroupFrame,
            Child, ImageObject,
                MUIA_Image_Spec, (IPTR)"3:Images:interface",
                MUIA_FixWidth, 52,
                MUIA_FixHeight, 48,
            End,
            Child, (IPTR)ColGroup(2),
                GroupFrame,
                Child, (IPTR)Label2(_(MSG_IFNAME)),
                Child, (IPTR)(nameString = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)NAMECHARS,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(_(MSG_UP)),
                Child, (IPTR)HGroup,
                    Child, (IPTR)(upState = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)HVSpace,
                End,
                Child, (IPTR)Label2(__(MSG_DEVICE)),
                Child, (IPTR)PopaslObject,
                    MUIA_Popasl_Type,              ASL_FileRequest,
                    ASLFO_MaxHeight,               100,
                    MUIA_Popstring_String, (IPTR)(deviceString = (Object *)StringObject,
                        StringFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                    End),
                    MUIA_Popstring_Button,  (IPTR)PopButton(MUII_PopUp),
                End,
                Child, (IPTR)Label2(_(MSG_UNIT)),
                Child, (IPTR)(unitString = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)"0123456789",
                    MUIA_CycleChain, 1,
                End),

                Child, (IPTR)Label2(__(MSG_IP_MODE)),
                Child, (IPTR)(ifDHCPState = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)DHCPCycle,
                End),
                Child, (IPTR)Label2(__(MSG_IP)),
                Child, (IPTR)(IPString = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)IPCHARS,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(__(MSG_MASK)),
                Child, (IPTR)(maskString = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)IPCHARS,
                    MUIA_CycleChain, 1,
                End),
            End,
            Child, HGroup,
                Child, (IPTR)(applyButton = ImageButton(_(MSG_BUTTON_APPLY), "THEME:Images/Gadgets/Prefs/Save")),
                Child, (IPTR)(closeButton = ImageButton(_(MSG_BUTTON_CLOSE), "THEME:Images/Gadgets/Prefs/Cancel")),
            End,
        End,
    End;


    if (self != NULL && ifWindow != NULL)
    {
        struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

        // main window
        data->netped_DHCPState = DHCPState;
        data->netped_gateString = gateString;
        data->netped_DNSString[0] = DNSString[0];
        data->netped_DNSString[1] = DNSString[1];
        data->netped_hostString = hostString;
        data->netped_domainString = domainString;
        data->netped_Autostart = autostart;
        data->netped_interfaceList = interfaceList;
        data->netped_inputGroup = inputGroup;
        data->netped_addButton = addButton;
        data->netped_editButton = editButton;
        data->netped_removeButton = removeButton;

        // interface window
        data->netped_ifWindow = ifWindow;
        data->netped_upState = upState;
        data->netped_ifDHCPState = ifDHCPState;
        data->netped_nameString = nameString;
        data->netped_deviceString  = deviceString;
        data->netped_unitString = unitString;
        data->netped_IPString = IPString;
        data->netped_maskString = maskString;
        data->netped_applyButton = applyButton;
        data->netped_closeButton = closeButton;

        SET(removeButton, MUIA_Disabled, TRUE);
        SET(editButton, MUIA_Disabled, TRUE);

        /*-- Setup notifications -------------------------------------------*/

        // main window
        DoMethod
        (
            interfaceList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            (IPTR)self, 1, MUIM_NetPEditor_ShowEntry
        );
        DoMethod
        (
            interfaceList, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_NetPEditor_EditEntry, FALSE
        );

        DoMethod
        (
            gateString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            DNSString[0], MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            DNSString[1], MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            hostString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            domainString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            autostart, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            DHCPState, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 2, MUIM_NetPEditor_IPModeChanged, FALSE
        );

        DoMethod
        (
            addButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 2, MUIM_NetPEditor_EditEntry, TRUE
        );
        DoMethod
        (
            editButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 1, MUIM_NetPEditor_EditEntry, FALSE
        );
        DoMethod
        (
            removeButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)interfaceList, 2, MUIM_List_Remove, MUIV_List_Remove_Active
        );

        // interface window
        DoMethod
        (
            ifDHCPState, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 2, MUIM_NetPEditor_IPModeChanged, TRUE
        );
        DoMethod
        (
            upState, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            applyButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 1, MUIM_NetPEditor_ApplyEntry
        );
        DoMethod
        (
            closeButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)ifWindow, 3, MUIM_Set, MUIA_Window_Open, FALSE
        );
    }

    return self;
}

IPTR NetPEditor__MUIM_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;

    DoMethod(_app(self), OM_ADDMEMBER, data->netped_ifWindow);

    return TRUE;
}

IPTR NetPEditor__MUIM_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    DoMethod(_app(self), OM_REMMEMBER, data->netped_ifWindow);

    return DoSuperMethodA(CLASS, self, message);
}

IPTR NetPEditor__MUIM_PrefsEditor_Save
(
    Class *CLASS, Object *self, Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);
    enum ErrorCode errorcode = UNKNOWN_ERROR;

    Gadgets2NetworkPrefs(data);

    if ((errorcode = SaveNetworkPrefs()) == ALL_OK)
    {
        SET(self, MUIA_PrefsEditor_Changed, FALSE);
        SET(self, MUIA_PrefsEditor_Testing, FALSE);
        return TRUE;
    }

    DisplayErrorMessage(self, errorcode);

    /* Prefs saved to disk, but stack not restarted. Inform about restart will 'apply' changes */
    if (errorcode == NOT_RESTARTED_STACK)
    {
        Object * app = NULL;
        Object * wnd = NULL;

        GET(self, MUIA_ApplicationObject, &app);
        GET(self, MUIA_Window_Window, &wnd);

        MUI_Request(app, wnd, 0, _(MSG_INFO_TITLE), _(MSG_BUTTON_OK),
            _(MSG_PREFS_SAVED_RESTART), PREFS_PATH_ENVARC);

        return TRUE;
    }

    return FALSE;
}

IPTR NetPEditor__MUIM_PrefsEditor_Use
(
    Class *CLASS, Object *self, Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);
    enum ErrorCode errorcode = UNKNOWN_ERROR;

    Gadgets2NetworkPrefs(data);

    if ((errorcode = UseNetworkPrefs()) == ALL_OK)
    {
        SET(self, MUIA_PrefsEditor_Changed, FALSE);
        SET(self, MUIA_PrefsEditor_Testing, FALSE);
        return TRUE;
    }

    DisplayErrorMessage(self, errorcode);

    return FALSE;
}

IPTR NetPEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);
    BOOL success = TRUE;

    NetworkPrefs2Gadgets(data);

    DoMethod(self, MUIM_NetPEditor_IPModeChanged, TRUE);
    DoMethod(self, MUIM_NetPEditor_IPModeChanged, FALSE);

    return success;
}

IPTR NetPEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);
    BOOL success = TRUE;

    NetworkPrefs2Gadgets(data);

    DoMethod(self, MUIM_NetPEditor_IPModeChanged, TRUE);
    DoMethod(self, MUIM_NetPEditor_IPModeChanged, FALSE);

    return success;
}

IPTR NetPEditor__MUIM_NetPEditor_IPModeChanged
(
    Class *CLASS, Object *self,
    struct MUIP_NetPEditor_IPModeChanged *message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    IPTR lng = 0;

    if (message->interface)
    {
        GetAttr(MUIA_Cycle_Active, data->netped_ifDHCPState, &lng);

        if (lng == 1)
        {
            SET(data->netped_IPString, MUIA_Disabled, TRUE);
            SET(data->netped_IPString, MUIA_String_Contents, "");
            SET(data->netped_maskString, MUIA_Disabled, TRUE);
            SET(data->netped_maskString, MUIA_String_Contents, "");
        }
        else
        {
            SET(data->netped_IPString, MUIA_Disabled, FALSE);
            SET(data->netped_IPString, MUIA_String_Contents, DEFAULTIP);
            SET(data->netped_maskString, MUIA_Disabled, FALSE);
            SET(data->netped_maskString, MUIA_String_Contents, DEFAULTMASK);
        }
    }
    else
    {
        GetAttr(MUIA_Cycle_Active, data->netped_DHCPState, &lng);

        if (lng == 1)
        {
            SET(data->netped_gateString, MUIA_Disabled, TRUE);
            SET(data->netped_gateString, MUIA_String_Contents, "");
            SET(data->netped_DNSString[0], MUIA_Disabled, TRUE);
            SET(data->netped_DNSString[0], MUIA_String_Contents, "");
            SET(data->netped_DNSString[1], MUIA_Disabled, TRUE);
            SET(data->netped_DNSString[1], MUIA_String_Contents, "");
        }
        else
        {
            SET(data->netped_gateString, MUIA_Disabled, FALSE);
            SET(data->netped_gateString, MUIA_String_Contents, DEFAULTGATE);
            SET(data->netped_DNSString[0], MUIA_Disabled, FALSE);
            SET(data->netped_DNSString[0], MUIA_String_Contents, DEFAULTDNS);
            SET(data->netped_DNSString[1], MUIA_Disabled, FALSE);
            SET(data->netped_DNSString[1], MUIA_String_Contents, DEFAULTDNS);
        }
    }

    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    return TRUE;
}

/*
    Shows content of current list entry in the interface window.
*/
IPTR NetPEditor__MUIM_NetPEditor_ShowEntry
(
    Class *CLASS, Object *self,
    Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    struct Interface *iface;

    DoMethod
    (
        data->netped_interfaceList,
        MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &iface
    );
    if (iface)
    {
        SET(data->netped_removeButton, MUIA_Disabled, FALSE);
        SET(data->netped_editButton, MUIA_Disabled, FALSE);

        if (GetIfDHCP(iface))
        {
            SET(data->netped_IPString, MUIA_String_Contents, "");
            SET(data->netped_maskString, MUIA_String_Contents, "");
        }
        else
        {
            SET(data->netped_IPString, MUIA_String_Contents, GetIP(iface));
            SET(data->netped_maskString, MUIA_String_Contents, GetMask(iface));
        }
        SET(data->netped_nameString, MUIA_String_Contents, GetName(iface));
        SET(data->netped_upState, MUIA_Selected, GetUp(iface) ? 1 : 0);
        SET(data->netped_ifDHCPState, MUIA_Cycle_Active, GetIfDHCP(iface) ? 1 : 0);
        SET(data->netped_deviceString, MUIA_String_Contents, GetDevice(iface));
        SET(data->netped_unitString, MUIA_String_Integer, GetUnit(iface));
    }
    else
    {
        SET(data->netped_removeButton, MUIA_Disabled, TRUE);
        SET(data->netped_editButton, MUIA_Disabled, TRUE);
        SET(data->netped_ifWindow, MUIA_Window_Open, FALSE);
    }
    return 0;
}

IPTR NetPEditor__MUIM_NetPEditor_EditEntry
(
    Class *CLASS, Object *self,
    struct MUIP_NetPEditor_EditEntry *message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    if (message->addEntry)
    {
        /*
            Create a new entry and make it the current one
        */
        LONG entries = XGET(data->netped_interfaceList, MUIA_List_Entries);
        if (entries < MAXINTERFACES)
        {
            struct Interface iface;
            InitInterface(&iface);
            SetUp(&iface, TRUE);    // new entries are UP
            DoMethod
            (
                data->netped_interfaceList,
                MUIM_List_InsertSingle, &iface, MUIV_List_Insert_Bottom
            );
        }
        SET(data->netped_interfaceList, MUIA_List_Active, entries + 1);
    }

    LONG active = XGET(data->netped_interfaceList, MUIA_List_Active);
    if (active != MUIV_List_Active_Off)
    {
        SET(data->netped_ifWindow, MUIA_Window_Open, TRUE);
    }

    return 0;
}

/*
    Store data from interface window back in current list entry
*/
IPTR NetPEditor__MUIM_NetPEditor_ApplyEntry
(
    Class *CLASS, Object *self,
    Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    LONG active = XGET(data->netped_interfaceList, MUIA_List_Active);
    if (active != MUIV_List_Active_Off)
    {
        struct Interface iface;
        SetInterface
        (
            &iface,
            (STRPTR)XGET(data->netped_nameString, MUIA_String_Contents),
            XGET(data->netped_ifDHCPState, MUIA_Cycle_Active),
            (STRPTR)XGET(data->netped_IPString, MUIA_String_Contents),
            (STRPTR)XGET(data->netped_maskString, MUIA_String_Contents),
            (STRPTR)XGET(data->netped_deviceString, MUIA_String_Contents),
            XGET(data->netped_unitString, MUIA_String_Integer),
            XGET(data->netped_upState, MUIA_Selected)
        );
        DoMethod(data->netped_interfaceList, MUIM_List_Remove, active);
        DoMethod(data->netped_interfaceList, MUIM_List_InsertSingle, &iface, active);
        SET(data->netped_interfaceList, MUIA_List_Active, active);
        SET(self, MUIA_PrefsEditor_Changed, TRUE);
    }

    return 0;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_11
(
    NetPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                         struct opSet *,
    MUIM_Setup,                     Msg,
    MUIM_Cleanup,                   Msg,
    MUIM_PrefsEditor_ImportFH,      struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,      struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Save,          Msg,
    MUIM_PrefsEditor_Use,           Msg,
    MUIM_NetPEditor_IPModeChanged,  struct MUIP_NetPEditor_IPModeChanged *,
    MUIM_NetPEditor_ShowEntry,      Msg,
    MUIM_NetPEditor_EditEntry,      struct MUIP_NetPEditor_EditEntry *,
    MUIM_NetPEditor_ApplyEntry,     Msg
);
