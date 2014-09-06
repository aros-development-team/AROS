/*
    Copyright � 2009-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/locale.h>
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

static CONST_STRPTR NetworkTabs[] = { NULL, NULL, NULL, NULL, NULL, NULL};
static CONST_STRPTR DHCPCycle[] = { NULL, NULL, NULL };
static CONST_STRPTR EncCycle[] = { NULL, NULL, NULL, NULL };
static CONST_STRPTR KeyCycle[] = { NULL, NULL, NULL };
static CONST_STRPTR ServiceTypeCycle[] = { NULL, NULL };
static const TEXT max_ip_str[] = "255.255.255.255 ";

static struct Hook  netpeditor_displayHook,
                    netpeditor_constructHook,
                    netpeditor_destructHook;
static struct Hook  hosts_displayHook,
                    hosts_constructHook,
                    hosts_destructHook;
static struct Hook  wireless_displayHook,
                    wireless_constructHook,
                    wireless_destructHook;
static struct Hook  server_displayHook,
                    server_constructHook,
                    server_destructHook;

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
            *netped_inputGroup,
            *netped_hostList,
            *netped_hostAddButton,
            *netped_hostEditButton,
            *netped_hostRemoveButton,
            *netped_networkList,
            *netped_netAddButton,
            *netped_netEditButton,
            *netped_netRemoveButton,
            *netped_MBBInitString[MAXATCOMMANDS],
            *netped_MBBDeviceString,
            *netped_MBBUnit,
            *netped_MBBUsername,
            *netped_MBBPassword,
            *netped_serverList,
            *netped_serverAddButton,
            *netped_serverEditButton,
            *netped_serverRemoveButton;

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

    // Host window
    Object  *netped_hostWindow,
            *netped_hostNamesString,
            *netped_hostAddressString,
            *netped_hostApplyButton,
            *netped_hostCloseButton;

    // Wireless network window
    Object  *netped_netWindow,
            *netped_sSIDString,
            *netped_encType,
            *netped_keyType,
            *netped_keyString,
            *netped_hiddenState,
            *netped_adHocState,
            *netped_netApplyButton,
            *netped_netCloseButton;

    // File-server window
    Object  *netped_serverWindow,
            *netped_serverServiceType,
            *netped_serverDevice,
            *netped_serverActive,
            *netped_serverHost,
            *netped_serverService,
            *netped_serverUser,
            *netped_serverPass,
            *netped_serverGroup,
            *netped_serverApplyButton,
            *netped_serverCloseButton;
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
        sprintf(unitbuffer, "%d", (int)entry->unit);
        *array++ = entry->name;
        *array++ = entry->up ? "*" : "";
        *array++ = FilePart(entry->device);
        *array++ = unitbuffer;
        *array = entry->ifDHCP ? (STRPTR)_(MSG_IP_MODE_DHCP) : entry->IP;
    }
    else
    {
        *array++ = (STRPTR)_(MSG_IFNAME);
        *array++ = (STRPTR)_(MSG_UP);
        *array++ = (STRPTR)_(MSG_DEVICE);
        *array++ = (STRPTR)_(MSG_UNIT);
        *array = (STRPTR)_(MSG_IP);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(APTR, hostsConstructFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(struct Host *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct Host *new;

    if ((new = AllocPooled(pool, sizeof(*new))))
    {
        *new = *entry;
    }
    return new;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, hostsDestructFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(struct Host *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct Host));

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(LONG, hostsDisplayFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(char **, array, A2),
AROS_UFHA(struct Host *, entry, A1))
{
    AROS_USERFUNC_INIT
    if (entry)
    {
        *array++ = entry->address;
        *array++ = entry->names;
    }
    else
    {
        *array++ = (STRPTR)_(MSG_IP);
        *array++ = (STRPTR)_(MSG_HOST_NAMES);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(APTR, netConstructFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(struct Network *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct Network *new;

    if ((new = AllocPooled(pool, sizeof(*new))))
    {
        *new = *entry;
    }
    return new;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, netDestructFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(struct Network *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct Network));

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(LONG, netDisplayFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(char **, array, A2),
AROS_UFHA(struct Network *, entry, A1))
{
    AROS_USERFUNC_INIT
    if (entry)
    {
        *array++ = entry->name;
        switch(entry->encType)
        {
            case 2:
                *array++ = (STRPTR)_(MSG_ENC_WPA);
                break;
            case 1:
                *array++ = (STRPTR)_(MSG_ENC_WEP);
                break;
            default:
                *array++ = (STRPTR)_(MSG_ENC_NONE);
        }
        *array++ = (STRPTR)L_(entry->hidden ? YESSTR : NOSTR);
        *array++ = (STRPTR)L_(entry->adHoc ? YESSTR : NOSTR);
    }
    else
    {
        *array++ = (STRPTR)_(MSG_SSID);
        *array++ = (STRPTR)_(MSG_ENCTYPE);
        *array++   = (STRPTR)_(MSG_HIDDEN);
        *array++   = (STRPTR)_(MSG_ADHOC);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(APTR, serverConstructFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(struct Server *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct Server *new;

    if ((new = AllocPooled(pool, sizeof(*new))))
    {
        *new = *entry;
    }
    return new;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, serverDestructFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(struct Server *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct Server));

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(LONG, serverDisplayFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(char **, array, A2),
AROS_UFHA(struct Server *, entry, A1))
{
    AROS_USERFUNC_INIT
    if (entry)
    {
        *array++ = entry->device;
        *array++ = entry->active ? "*" : "";
        *array++ = entry->host;
        *array++ = entry->group;
        *array++ = entry->service;
        *array++ = entry->user;
    }
    else
    {
        *array++ = (STRPTR)_(MSG_DEVICE);
        *array++ = (STRPTR)_(MSG_UP);
        *array++ = (STRPTR)_(MSG_HOST_NAME);
        *array++ = (STRPTR)_(MSG_WORKGROUP);
        *array++ = (STRPTR)_(MSG_SERVICE);
        *array++ = (STRPTR)_(MSG_USERNAME);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

BOOL Gadgets2NetworkPrefs(struct NetPEditor_DATA *data)
{
    STRPTR str = NULL;
    IPTR lng = 0;
    LONG i,a;

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
    SetHostname(str);
    GET(data->netped_domainString, MUIA_String_Contents, &str);
    SetDomain(str);
    GET(data->netped_Autostart, MUIA_Selected, &lng);
    SetAutostart(lng);
    GET(data->netped_DHCPState, MUIA_Cycle_Active, &lng);
    SetDHCP(lng);

    entries = XGET(data->netped_hostList, MUIA_List_Entries);
    for(i = 0; i < entries; i++)
    {
        struct Host *host = GetHost(i);
        struct Host *hostentry;
        DoMethod
        (
            data->netped_hostList,
            MUIM_List_GetEntry, i, &hostentry
        );
        SetHostAddress(host, hostentry->address);
        SetHostNames(host, hostentry->names);
    }
    SetHostCount(entries);

    entries = XGET(data->netped_networkList, MUIA_List_Entries);
    for(i = 0; i < entries; i++)
    {
        struct Network *net = GetNetwork(i);
        struct Network *netentry;
        DoMethod
        (
            data->netped_networkList,
            MUIM_List_GetEntry, i, &netentry
        );
        SetNetworkName(net, netentry->name);
        SetEncType(net, netentry->encType);
        SetKey(net, netentry->key, netentry->keyIsHex);
        SetHidden(net, netentry->hidden);
        SetAdHoc(net, netentry->adHoc);
    }
    SetNetworkCount(entries);

    for(i = 0 ; i < MAXATCOMMANDS; i++) SetMobile_atcommand(i,"");

    a = 0;
    for(i = 0 ; i < MAXATCOMMANDS; i++)
    {
        GET(data->netped_MBBInitString[i], MUIA_String_Contents, &str);
        if( strlen(str) > 0 ) SetMobile_atcommand( a++ , str );
    }
    GET(data->netped_MBBDeviceString, MUIA_String_Contents, &str);
    SetMobile_devicename(str);
    GET(data->netped_MBBUnit, MUIA_Numeric_Value , &lng);
    SetMobile_unit(lng);
    GET(data->netped_MBBUsername, MUIA_String_Contents, &str);
    SetMobile_username(str);
    GET(data->netped_MBBPassword, MUIA_String_Contents, &str);
    SetMobile_password(str);

    entries = XGET(data->netped_serverList, MUIA_List_Entries);
    for(i = 0; i < entries; i++)
    {
        struct Server *server = GetServer(i);
        struct Server *serverentry;
        DoMethod
        (
            data->netped_serverList,
            MUIM_List_GetEntry, i, &serverentry
        );
        SetServerDevice(server, serverentry->device);
        SetServerHost(server, serverentry->host);
        SetServerService(server, serverentry->service);
        SetServerUser(server, serverentry->user);
        SetServerGroup(server, serverentry->group);
        SetServerPass(server, serverentry->pass);
        SetServerActive(server, serverentry->active);
    }
    SetServerCount(entries);

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
    NNSET(data->netped_hostString, MUIA_String_Contents, (IPTR)GetHostname());
    NNSET(data->netped_domainString, MUIA_String_Contents, (IPTR)GetDomain());
    NNSET(data->netped_Autostart, MUIA_Selected, (IPTR)GetAutostart());
    NNSET(data->netped_DHCPState, MUIA_Cycle_Active, (IPTR)GetDHCP() ? 1 : 0);

    entries = GetHostCount();

    SET(data->netped_hostList, MUIA_List_Quiet, TRUE);
    DoMethod(data->netped_hostList, MUIM_List_Clear);
    for(i = 0; i < entries; i++)
    {
        struct Host *host = GetHost(i);
        struct Host hostentry;

        SetHost
        (
            &hostentry,
            GetHostNames(host),
            GetHostAddress(host)
        );

        DoMethod
        (
            data->netped_hostList,
            MUIM_List_InsertSingle, &hostentry, MUIV_List_Insert_Bottom
        );
    }
    SET(data->netped_hostList, MUIA_List_Quiet, FALSE);

    SET(data->netped_networkList, MUIA_List_Quiet, TRUE);
    DoMethod(data->netped_networkList, MUIM_List_Clear);
    entries = GetNetworkCount();
    for(i = 0; i < entries; i++)
    {
        struct Network *net = GetNetwork(i);
        struct Network netentry;

        SetNetwork
        (
            &netentry,
            GetNetworkName(net),
            GetEncType(net),
            GetKey(net),
            net->keyIsHex,
            GetHidden(net),
            GetAdHoc(net)
        );

        DoMethod
        (
            data->netped_networkList,
            MUIM_List_InsertSingle, &netentry, MUIV_List_Insert_Bottom
        );
    }

    SET(data->netped_networkList, MUIA_List_Quiet, FALSE);

    for(i = 0 ; i < MAXATCOMMANDS; i++)
        NNSET((data->netped_MBBInitString[i]), MUIA_String_Contents, "");

    entries = GetMobile_atcommandcount();
    for(i = 0 ; i < entries; i++)
    {
        NNSET((data->netped_MBBInitString[i+MAXATCOMMANDS-entries]), MUIA_String_Contents, GetMobile_atcommand(i));
    }

    NNSET((data->netped_MBBDeviceString), MUIA_String_Contents, GetMobile_devicename());
    NNSET((data->netped_MBBUnit), MUIA_Numeric_Value, GetMobile_unit());
    NNSET((data->netped_MBBUsername), MUIA_String_Contents, GetMobile_username());
    NNSET((data->netped_MBBPassword), MUIA_String_Contents, GetMobile_password());

    SET(data->netped_serverList, MUIA_List_Quiet, TRUE);
    DoMethod(data->netped_serverList, MUIM_List_Clear);
    entries = GetServerCount();
    for(i = 0; i < entries; i++)
    {
        struct Server *server = GetServer(i);
        struct Server serverentry;

        SetServer
        (
            &serverentry,
            GetServerDevice(server),
            GetServerHost(server),
            GetServerService(server),
            GetServerUser(server),
            GetServerGroup(server),
            GetServerPass(server),
            GetServerActive(server)
        );

        DoMethod
        (
            data->netped_serverList,
            MUIM_List_InsertSingle, &serverentry, MUIV_List_Insert_Bottom
        );
    }

    SET(data->netped_serverList, MUIA_List_Quiet, FALSE);

    return TRUE;
}

void DisplayErrorMessage(Object * obj, enum ErrorCode errorcode)
{
    CONST_STRPTR title = _(MSG_ERROR_TITLE);
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
        case NOT_RESTARTED_WIRELESS:
            errormessage = _(MSG_ERR_NOT_RESTARTED_WIRELESS);
            break;
        case NOT_RESTARTED_MOBILE:
            errormessage = _(MSG_ERR_NOT_RESTARTED_MOBILE);
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
        case MULTIPLE_IFACES:
            errormessage = _(MSG_WARN_MULTIPLE_IFACES);
            title = _(MSG_WARNING_TITLE);
            break;
        case ALL_OK:
            return;
    }

    MUI_Request(app, wnd, 0, title, _(MSG_BUTTON_OK), errormessage,
        additionaldata);
}

/*** Methods ****************************************************************/
Object * NetPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    // main window
    Object  *gateString, *DNSString[2], *hostString, *domainString,
            *autostart, *interfaceList, *DHCPState,
            *addButton, *editButton, *removeButton, *inputGroup,
            *hostList, *hostAddButton, *hostEditButton, *hostRemoveButton,
            *networkList, *netAddButton, *netEditButton, *netRemoveButton,
            *serverList, *serverAddButton, *serverEditButton, *serverRemoveButton,
            *MBBInitString[MAXATCOMMANDS], *MBBDeviceString, *MBBUnit,
            *MBBUsername, *MBBPassword;

    // inferface window
    Object  *deviceString, *IPString, *maskString,
            *ifDHCPState, *unitString, *nameString, *upState,
            *ifWindow, *applyButton, *closeButton;

    // host window
    Object  *hostNamesString, *hostAddressString, *hostWindow,
            *hostApplyButton, *hostCloseButton;

    // network window
    Object  *sSIDString, *keyString, *encType, *keyType, *hiddenState,
            *adHocState, *netWindow, *netApplyButton, *netCloseButton;

    // file-server window
    Object  *serverWindow, *serverServiceType, *serverDevice, *serverActive, *serverHost,
            *serverService, *serverUser, *serverGroup, *serverPass,
            *serverApplyButton, *serverCloseButton;

    DHCPCycle[0] = _(MSG_IP_MODE_MANUAL);
    DHCPCycle[1] = _(MSG_IP_MODE_DHCP);

    EncCycle[0] = _(MSG_ENC_NONE);
    EncCycle[1] = _(MSG_ENC_WEP);
    EncCycle[2] = _(MSG_ENC_WPA);

    KeyCycle[0] = _(MSG_KEY_TEXT);
    KeyCycle[1] = _(MSG_KEY_HEX);

    ServiceTypeCycle[0] = _(MSG_SERVICETYPE_CIFS);

    NetworkTabs[0] = _(MSG_TAB_IP_CONFIGURATION);
    NetworkTabs[1] = _(MSG_TAB_COMPUTER_NAMES);
    NetworkTabs[2] = _(MSG_TAB_WIRELESS);
    NetworkTabs[3] = _(MSG_TAB_MOBILE);
    NetworkTabs[4] = _(MSG_TAB_SERVERS);

    netpeditor_constructHook.h_Entry = (HOOKFUNC)constructFunc;
    netpeditor_destructHook.h_Entry = (HOOKFUNC)destructFunc;
    netpeditor_displayHook.h_Entry = (HOOKFUNC)displayFunc;

    hosts_constructHook.h_Entry = (HOOKFUNC)hostsConstructFunc;
    hosts_destructHook.h_Entry = (HOOKFUNC)hostsDestructFunc;
    hosts_displayHook.h_Entry = (HOOKFUNC)hostsDisplayFunc;

    wireless_constructHook.h_Entry = (HOOKFUNC)netConstructFunc;
    wireless_destructHook.h_Entry = (HOOKFUNC)netDestructFunc;
    wireless_displayHook.h_Entry = (HOOKFUNC)netDisplayFunc;

    server_constructHook.h_Entry = (HOOKFUNC)serverConstructFunc;
    server_destructHook.h_Entry = (HOOKFUNC)serverDestructFunc;
    server_displayHook.h_Entry = (HOOKFUNC)serverDisplayFunc;

    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_PrefsEditor_Name, __(MSG_NAME),
        MUIA_PrefsEditor_Path, (IPTR)"AROSTCP/arostcp.prefs",

        Child, RegisterGroup((IPTR)NetworkTabs),

            Child, (IPTR)VGroup,
                Child, (IPTR)(HGroup,
                    GroupFrame,
                    Child, (IPTR)ListviewObject,
                        MUIA_Listview_List, (IPTR)(interfaceList = (Object *)ListObject,
                            ReadListFrame,
                            MUIA_List_Title, TRUE,
                            MUIA_List_Format, (IPTR)"BAR,P=\33c BAR,BAR,BAR,",
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
                Child, (IPTR)(inputGroup = (Object *)ColGroup(4),
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
                        MUIA_FixWidthTxt, (IPTR)max_ip_str,
                    End),
                    Child, (IPTR)Label2(__(MSG_HOST_NAME)),
                    Child, (IPTR)(hostString = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)NAMECHARS,
                        MUIA_CycleChain, 1,
                    End),
                    Child, (IPTR)Label2(__(MSG_DNS1)),
                    Child, (IPTR)(DNSString[0] = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)IPCHARS,
                        MUIA_CycleChain, 1,
                        MUIA_FixWidthTxt, (IPTR)max_ip_str,
                    End),
                    Child, (IPTR)Label2(__(MSG_DOMAIN_NAME)),
                    Child, (IPTR)(domainString = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)NAMECHARS,
                        MUIA_CycleChain, 1,
                    End),
                    Child, (IPTR)Label2(__(MSG_DNS2)),
                    Child, (IPTR)(DNSString[1] = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)IPCHARS,
                        MUIA_CycleChain, 1,
                        MUIA_FixWidthTxt, (IPTR)max_ip_str,
                    End),
                End),
                Child, (IPTR)ColGroup(2),
                    Child, (IPTR)(autostart = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)Label2(__(MSG_AUTOSTART_STACK)),
                        Child, (IPTR)HVSpace,
                    End,
                End,
            End,

            Child, (IPTR)VGroup,
                Child, (IPTR)(HGroup,
                    GroupFrame,
                    Child, (IPTR)ListviewObject,
                        MUIA_Listview_List, (IPTR)(hostList = (Object *)ListObject,
                            ReadListFrame,
                            MUIA_List_Title, TRUE,
                            MUIA_List_Format, (IPTR)"BAR,",
                            MUIA_List_ConstructHook, (IPTR)&hosts_constructHook,
                            MUIA_List_DestructHook, (IPTR)&hosts_destructHook,
                            MUIA_List_DisplayHook, (IPTR)&hosts_displayHook,
                        End),
                    End,
                    Child, (IPTR)(VGroup,
                        MUIA_HorizWeight, 0,
                        Child, (IPTR)(hostAddButton = SimpleButton(_(MSG_BUTTON_ADD))),
                        Child, (IPTR)(hostEditButton = SimpleButton(_(MSG_BUTTON_EDIT))),
                        Child, (IPTR)(hostRemoveButton = SimpleButton(_(MSG_BUTTON_REMOVE))),
                        Child, (IPTR)HVSpace,
                    End),
                End),
            End,

            Child, (IPTR)VGroup,
                Child, (IPTR)(HGroup,
                    GroupFrame,
                    Child, (IPTR)ListviewObject,
                        MUIA_Listview_List, (IPTR)(networkList = (Object *)ListObject,
                            ReadListFrame,
                            MUIA_List_Title, TRUE,
                            MUIA_List_Format, (IPTR)"BAR,BAR,BAR,",
                            MUIA_List_ConstructHook, (IPTR)&wireless_constructHook,
                            MUIA_List_DestructHook, (IPTR)&wireless_destructHook,
                            MUIA_List_DisplayHook, (IPTR)&wireless_displayHook,
                        End),
                    End,
                    Child, (IPTR)(VGroup,
                        MUIA_HorizWeight, 0,
                        Child, (IPTR)(netAddButton = SimpleButton(_(MSG_BUTTON_ADD))),
                        Child, (IPTR)(netEditButton = SimpleButton(_(MSG_BUTTON_EDIT))),
                        Child, (IPTR)(netRemoveButton = SimpleButton(_(MSG_BUTTON_REMOVE))),
                        Child, (IPTR)HVSpace,
                    End),
                End),
            End,

            Child, (IPTR)VGroup,

                Child, (IPTR)ColGroup(2),
                GroupFrame,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)Label2(_(MSG_SERIAL_DEVICE)),
                        Child, (IPTR)(MBBDeviceString = (Object *)StringObject,
                            StringFrame,
                            MUIA_CycleChain, 1,
                        End),

                        Child, (IPTR)Label2(_(MSG_UNIT_NUMBER)),
                        Child, (IPTR)(MBBUnit = (Object *)NumericbuttonObject,
                            MUIA_Numeric_Min, 0,
                            MUIA_Numeric_Max, 100,
                        End),

                    End,
                End,

                Child, (IPTR)ColGroup(2),
                GroupFrame,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)Label2(_(MSG_USERNAME)),
                        Child, (IPTR)(MBBUsername = (Object *)StringObject,
                            StringFrame,
                            MUIA_CycleChain, 1,
                        End),
                        Child, (IPTR)Label2(_(MSG_PASSWORD)),
                        Child, (IPTR)(MBBPassword = (Object *)StringObject,
                            StringFrame,
                            MUIA_String_Secret, TRUE,
                            MUIA_CycleChain, 1,
                        End),
                    End,
                End,

                Child, (IPTR)ColGroup(2),
                    GroupFrame,
                    Child, (IPTR)Label2("Init1"),
                    Child, (IPTR)(MBBInitString[0] = (Object *)StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                    End),

                    Child, (IPTR)Label2("Init2"),
                    Child, (IPTR)(MBBInitString[1] = (Object *)StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                    End),

                    Child, (IPTR)Label2("Init3"),
                    Child, (IPTR)(MBBInitString[2] = (Object *)StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                    End),

                    Child, (IPTR)Label2("Init4"),
                    Child, (IPTR)(MBBInitString[3] = (Object *)StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                    End),

                    Child, (IPTR)Label2("Init5"),
                    Child, (IPTR)(MBBInitString[4] = (Object *)StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                    End),
                End,

            End,

            Child, (IPTR)VGroup,
                Child, (IPTR)(HGroup,
                    GroupFrame,
                    Child, (IPTR)ListviewObject,
                        MUIA_Listview_List, (IPTR)(serverList = (Object *)ListObject,
                            ReadListFrame,
                            MUIA_List_Title, TRUE,
                            MUIA_List_Format, (IPTR)"BAR,P=\33c BAR,BAR,BAR,BAR,",
                            MUIA_List_ConstructHook, (IPTR)&server_constructHook,
                            MUIA_List_DestructHook, (IPTR)&server_destructHook,
                            MUIA_List_DisplayHook, (IPTR)&server_displayHook,
                        End),
                    End,
                    Child, (IPTR)(VGroup,
                        MUIA_HorizWeight, 0,
                        Child, (IPTR)(serverAddButton = SimpleButton(_(MSG_BUTTON_ADD))),
                        Child, (IPTR)(serverEditButton = SimpleButton(_(MSG_BUTTON_EDIT))),
                        Child, (IPTR)(serverRemoveButton = SimpleButton(_(MSG_BUTTON_REMOVE))),
                        Child, (IPTR)HVSpace,
                    End),
                End),
            End,

        End, // register

        TAG_DONE
    );

    ifWindow = (Object *)WindowObject,
        MUIA_Window_Title, __(MSG_IFWINDOW_TITLE),
        MUIA_Window_ID, MAKE_ID('I', 'P', 'W', 'I'),
        MUIA_Window_CloseGadget, FALSE,
        WindowContents, (IPTR)VGroup,
            GroupFrame,
            Child, (IPTR)HGroup,
                Child, (IPTR)HVSpace,
                Child, (IPTR)ImageObject,
                    MUIA_Image_Spec, (IPTR)"3:Images:interface",
                    MUIA_FixWidth, 52,
                    MUIA_FixHeight, 48,
                End,
                Child, (IPTR)HVSpace,
            End,
            Child, (IPTR)ColGroup(2),
                GroupFrame,
                Child, (IPTR)Label2(_(MSG_IFNAME)),
                Child, (IPTR)HGroup,
                    Child, (IPTR)(nameString = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)NAMECHARS,
                        MUIA_CycleChain, 1,
                    End),
                    Child, (IPTR)(upState = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)Label2(_(MSG_UP)),
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
                Child, (IPTR)HGroup,
                    Child, (IPTR)(unitString = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)"0123456789",
                        MUIA_CycleChain, 1,
                        MUIA_FixWidthTxt, (IPTR)"0",
                    End),
                    Child, (IPTR)HVSpace,
                End,
            End,
            Child, (IPTR)ColGroup(4),
                GroupFrame,
                Child, (IPTR)Label2(__(MSG_IP_MODE)),
                Child, (IPTR)(ifDHCPState = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)DHCPCycle,
                End),
                Child, (IPTR)Label2(__(MSG_IP)),
                Child, (IPTR)(IPString = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)IPCHARS,
                    MUIA_CycleChain, 1,
                    MUIA_FixWidthTxt, (IPTR)max_ip_str,
                End),
                Child, (IPTR)HVSpace,
                Child, (IPTR)HVSpace,
                Child, (IPTR)Label2(__(MSG_MASK)),
                Child, (IPTR)(maskString = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)IPCHARS,
                    MUIA_CycleChain, 1,
                    MUIA_FixWidthTxt, (IPTR)max_ip_str,
                End),
            End,
            Child, (IPTR)HGroup,
                Child, (IPTR)(applyButton = ImageButton(_(MSG_BUTTON_APPLY), "THEME:Images/Gadgets/Prefs/Save")),
                Child, (IPTR)(closeButton = ImageButton(_(MSG_BUTTON_CLOSE), "THEME:Images/Gadgets/Prefs/Cancel")),
            End,
        End,
    End;

    hostWindow = (Object *)WindowObject,
        MUIA_Window_Title, __(MSG_HOST_NAMES),
        MUIA_Window_ID, MAKE_ID('H', 'O', 'S', 'T'),
        MUIA_Window_CloseGadget, FALSE,
        MUIA_Window_SizeGadget, TRUE,
        WindowContents, (IPTR)VGroup,
            GroupFrame,
            Child, (IPTR)HGroup,
                Child, (IPTR)HVSpace,
                Child, (IPTR)ImageObject,
                    MUIA_Image_Spec, (IPTR)"3:Images:host",
                    MUIA_FixWidth, 26,
                    MUIA_FixHeight, 50,
                End,
                Child, (IPTR)HVSpace,
            End,
            Child, (IPTR)ColGroup(2),
                GroupFrame,
                Child, (IPTR)Label2(__(MSG_IP)),
                Child, (IPTR)(hostAddressString = (Object *)StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(_(MSG_HOSTWINDOW_TITLE)),
                Child, (IPTR)(hostNamesString = (Object *)StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),
            End,
            Child, (IPTR)HGroup,
                Child, (IPTR)(hostApplyButton = ImageButton(_(MSG_BUTTON_APPLY), "THEME:Images/Gadgets/Save")),
                Child, (IPTR)(hostCloseButton = ImageButton(_(MSG_BUTTON_CLOSE), "THEME:Images/Gadgets/Cancel")),
            End,
        End,
    End;

    netWindow = (Object *)WindowObject,
        MUIA_Window_Title, __(MSG_NETWINDOW_TITLE),
        MUIA_Window_ID, MAKE_ID('W', 'I', 'F', 'I'),
        MUIA_Window_CloseGadget, FALSE,
        MUIA_Window_SizeGadget, TRUE,
        WindowContents, (IPTR)VGroup,
            GroupFrame,
            Child, (IPTR)HGroup,
                Child, (IPTR)HVSpace,
                Child, (IPTR)ImageObject,
                    MUIA_Image_Spec, (IPTR)"3:Images:wireless",
                    MUIA_FixWidth, 50,
                    MUIA_FixHeight, 50,
                End,
                Child, (IPTR)HVSpace,
            End,
            Child, (IPTR)ColGroup(2),
                GroupFrame,
                Child, (IPTR)Label2(_(MSG_SSID)),
                Child, (IPTR)(sSIDString = (Object *)StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),

                Child, (IPTR)Label2(__(MSG_ENCTYPE)),
                Child, (IPTR)(encType = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)EncCycle,
                End),
                Child, (IPTR)Label2(__(MSG_KEYTYPE)),
                Child, (IPTR)(keyType = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)KeyCycle,
                End),
                Child, (IPTR)Label2(__(MSG_KEY)),
                Child, (IPTR)(keyString = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Secret, TRUE,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)HGroup,
                    Child, (IPTR)(hiddenState = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)Label2(_(MSG_HIDDEN)),
                    Child, (IPTR)HVSpace,
                End,
                Child, (IPTR)HGroup,
                    Child, (IPTR)(adHocState = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)Label2(_(MSG_ADHOC)),
                    Child, (IPTR)HVSpace,
                End,
            End,
            Child, (IPTR)HGroup,
                Child, (IPTR)(netApplyButton = ImageButton(_(MSG_BUTTON_APPLY), "THEME:Images/Gadgets/Save")),
                Child, (IPTR)(netCloseButton = ImageButton(_(MSG_BUTTON_CLOSE), "THEME:Images/Gadgets/Cancel")),
            End,
        End,
    End;

    serverWindow = (Object *)WindowObject,
        MUIA_Window_Title, __(MSG_SERVERWINDOW_TITLE),
        MUIA_Window_ID, MAKE_ID('S', 'H', 'R', 'E'),
        MUIA_Window_CloseGadget, FALSE,
        MUIA_Window_SizeGadget, TRUE,
        WindowContents, (IPTR)VGroup,
            GroupFrame,
            Child, (IPTR)HGroup,
                Child, (IPTR)HVSpace,
                Child, (IPTR)ImageObject,
                    MUIA_Image_Spec, (IPTR)"3:Images:host",
                    MUIA_FixWidth, 26,
                    MUIA_FixHeight, 50,
                End,
                Child, (IPTR)HVSpace,
            End,
            Child, (IPTR)ColGroup(2),
                GroupFrame,
                Child, (IPTR)Label2(_(MSG_SERVICE_TYPES)),
                Child, (IPTR)(serverServiceType = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)ServiceTypeCycle,
                End),
                Child, (IPTR)Label2(_(MSG_DEVICE)),
                Child, (IPTR)(serverDevice = (Object *)StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)HVSpace,
                Child, (IPTR)HGroup,
                    Child, (IPTR)(serverActive = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)Label2(_(MSG_UP)),
                    Child, (IPTR)HVSpace,
                End,
                Child, (IPTR)Label2(_(MSG_HOST_NAME)),
                Child, (IPTR)(serverHost = (Object *)StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(__(MSG_WORKGROUP)),
                Child, (IPTR)(serverGroup = (Object *)StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(__(MSG_SERVICE)),
                Child, (IPTR)(serverService = (Object *)StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(__(MSG_USERNAME)),
                Child, (IPTR)(serverUser = (Object *)StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(__(MSG_PASSWORD)),
                Child, (IPTR)(serverPass = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Secret, TRUE,
                    MUIA_CycleChain, 1,
                End),
            End,
            Child, (IPTR)HGroup,
                Child, (IPTR)(serverApplyButton = ImageButton(_(MSG_BUTTON_APPLY), "THEME:Images/Gadgets/Save")),
                Child, (IPTR)(serverCloseButton = ImageButton(_(MSG_BUTTON_CLOSE), "THEME:Images/Gadgets/Cancel")),
            End,
        End,
    End;

    if (self != NULL && ifWindow != NULL && hostWindow != NULL
        && netWindow != NULL && serverWindow != NULL)
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
        data->netped_hostList = hostList;
        data->netped_hostAddButton = hostAddButton;
        data->netped_hostEditButton = hostEditButton;
        data->netped_hostRemoveButton = hostRemoveButton;
        data->netped_networkList = networkList;
        data->netped_netAddButton = netAddButton;
        data->netped_netEditButton = netEditButton;
        data->netped_netRemoveButton = netRemoveButton;
        data->netped_serverList = serverList;
        data->netped_serverAddButton = serverAddButton;
        data->netped_serverEditButton = serverEditButton;
        data->netped_serverRemoveButton = serverRemoveButton;

        data->netped_MBBInitString[0] = MBBInitString[0];
        data->netped_MBBInitString[1] = MBBInitString[1];
        data->netped_MBBInitString[2] = MBBInitString[2];
        data->netped_MBBInitString[3] = MBBInitString[3];
        data->netped_MBBInitString[4] = MBBInitString[4];
        data->netped_MBBDeviceString = MBBDeviceString;
        data->netped_MBBUnit = MBBUnit;
        data->netped_MBBUsername = MBBUsername;
        data->netped_MBBPassword = MBBPassword;

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

        // host window
        data->netped_hostWindow = hostWindow;
        data->netped_hostAddressString = hostAddressString;
        data->netped_hostNamesString = hostNamesString;
        data->netped_hostApplyButton = hostApplyButton;
        data->netped_hostCloseButton = hostCloseButton;

        // wireless window
        data->netped_netWindow = netWindow;
        data->netped_hiddenState = hiddenState;
        data->netped_adHocState = adHocState;
        data->netped_sSIDString = sSIDString;
        data->netped_encType = encType;
        data->netped_keyType = keyType;
        data->netped_keyString = keyString;
        data->netped_netApplyButton = netApplyButton;
        data->netped_netCloseButton = netCloseButton;

        // file-server window
        data->netped_serverWindow = serverWindow;
        data->netped_serverServiceType = serverServiceType;
        data->netped_serverDevice = serverDevice;
        data->netped_serverActive = serverActive;
        data->netped_serverHost = serverHost;
        data->netped_serverService = serverService;
        data->netped_serverUser = serverUser;
        data->netped_serverGroup = serverGroup;
        data->netped_serverPass = serverPass;
        data->netped_serverApplyButton = serverApplyButton;
        data->netped_serverCloseButton = serverCloseButton;

        SET(removeButton, MUIA_Disabled, TRUE);
        SET(editButton, MUIA_Disabled, TRUE);

        SET(hostRemoveButton, MUIA_Disabled, TRUE);
        SET(hostEditButton, MUIA_Disabled, TRUE);

        SET(netRemoveButton, MUIA_Disabled, TRUE);
        SET(netEditButton, MUIA_Disabled, TRUE);

        SET(serverRemoveButton, MUIA_Disabled, TRUE);
        SET(serverEditButton, MUIA_Disabled, TRUE);

        /*-- Set up notifications ------------------------------------------*/

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
            (IPTR)self, 2, MUIM_NetPEditor_EditEntry, FALSE
        );
        DoMethod
        (
            removeButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)interfaceList, 2, MUIM_List_Remove, MUIV_List_Remove_Active
        );

        DoMethod
        (
            hostList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            (IPTR)self, 1, MUIM_NetPEditor_ShowHostEntry
        );
        DoMethod
        (
            hostList, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_NetPEditor_EditHostEntry, FALSE
        );

        DoMethod
        (
            hostAddButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 2, MUIM_NetPEditor_EditHostEntry, TRUE
        );
        DoMethod
        (
            hostEditButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 1, MUIM_NetPEditor_EditHostEntry, FALSE
        );
        DoMethod
        (
            hostRemoveButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)hostList, 2, MUIM_List_Remove, MUIV_List_Remove_Active
        );

        DoMethod
        (
            networkList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            (IPTR)self, 1, MUIM_NetPEditor_ShowNetEntry
        );
        DoMethod
        (
            networkList, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_NetPEditor_EditNetEntry, FALSE
        );

        DoMethod
        (
            netAddButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 2, MUIM_NetPEditor_EditNetEntry, TRUE
        );
        DoMethod
        (
            netEditButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 1, MUIM_NetPEditor_EditNetEntry, FALSE
        );
        DoMethod
        (
            netRemoveButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)networkList, 2, MUIM_List_Remove, MUIV_List_Remove_Active
        );

        DoMethod
        (
            MBBDeviceString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            MBBUnit, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
         DoMethod
        (
            MBBUsername, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
         DoMethod
        (
            MBBPassword, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        LONG i;
        for(i=0;i<MAXATCOMMANDS;i++)
            DoMethod
            (
                MBBInitString[i], MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
                (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
            );

        DoMethod
        (
            serverList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            (IPTR)self, 1, MUIM_NetPEditor_ShowServerEntry
        );
        DoMethod
        (
            serverList, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_NetPEditor_EditServerEntry, FALSE
        );

        DoMethod
        (
            serverAddButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 2, MUIM_NetPEditor_EditServerEntry, TRUE
        );
        DoMethod
        (
            serverEditButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 1, MUIM_NetPEditor_EditServerEntry, FALSE
        );
        DoMethod
        (
            serverRemoveButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)serverList, 2, MUIM_List_Remove, MUIV_List_Remove_Active
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

        // host window
        DoMethod
        (
            hostApplyButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 1, MUIM_NetPEditor_ApplyHostEntry
        );
        DoMethod
        (
            hostCloseButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)hostWindow, 3, MUIM_Set, MUIA_Window_Open, FALSE
        );

        // network window
        DoMethod
        (
            netApplyButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 1, MUIM_NetPEditor_ApplyNetEntry
        );
        DoMethod
        (
            netCloseButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)netWindow, 3, MUIM_Set, MUIA_Window_Open, FALSE
        );

        // server window
        DoMethod
        (
            serverApplyButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 1, MUIM_NetPEditor_ApplyServerEntry
        );
        DoMethod
        (
            serverCloseButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)serverWindow, 3, MUIM_Set, MUIA_Window_Open, FALSE
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
    DoMethod(_app(self), OM_ADDMEMBER, data->netped_hostWindow);
    DoMethod(_app(self), OM_ADDMEMBER, data->netped_netWindow);
    DoMethod(_app(self), OM_ADDMEMBER, data->netped_serverWindow);

    return TRUE;
}

IPTR NetPEditor__MUIM_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    DoMethod(_app(self), OM_REMMEMBER, data->netped_ifWindow);
    DoMethod(_app(self), OM_REMMEMBER, data->netped_hostWindow);
    DoMethod(_app(self), OM_REMMEMBER, data->netped_netWindow);
    DoMethod(_app(self), OM_REMMEMBER, data->netped_serverWindow);

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
    STRPTR str = NULL;
    IPTR lng = 0;
    struct Interface *iface;

    if (message->interface)
    {
        DoMethod
        (
            data->netped_interfaceList,
            MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &iface
        );
        GetAttr(MUIA_Cycle_Active, data->netped_ifDHCPState, &lng);

        if (iface != NULL)
        {
            if (lng == 1)
            {
                /* Clear and disable text boxes, but keep their values for
                 * later */

                SET(data->netped_IPString, MUIA_Disabled, TRUE);
                GET(data->netped_IPString, MUIA_String_Contents, &str);
                SetIP(iface, str);
                SET(data->netped_IPString, MUIA_String_Contents, "");

                SET(data->netped_maskString, MUIA_Disabled, TRUE);
                GET(data->netped_maskString, MUIA_String_Contents, &str);
                SetMask(iface, str);
                SET(data->netped_maskString, MUIA_String_Contents, "");
            }
            else
            {
                /* Enable text boxes, and reset their previous values */

                SET(data->netped_IPString, MUIA_Disabled, FALSE);
                SET(data->netped_IPString, MUIA_String_Contents,
                    GetIP(iface));

                SET(data->netped_maskString, MUIA_Disabled, FALSE);
                SET(data->netped_maskString, MUIA_String_Contents,
                    GetMask(iface));
            }
        }
    }
    else
    {
        GetAttr(MUIA_Cycle_Active, data->netped_DHCPState, &lng);

        if (lng == 1)
        {
            /* Clear and disable text boxes, but keep their values for later */

            SET(data->netped_gateString, MUIA_Disabled, TRUE);
            GET(data->netped_gateString, MUIA_String_Contents, &str);
            SetGate(str);
            SET(data->netped_gateString, MUIA_String_Contents, "");

            SET(data->netped_DNSString[0], MUIA_Disabled, TRUE);
            GET(data->netped_DNSString[0], MUIA_String_Contents, &str);
            SetDNS(0, str);
            SET(data->netped_DNSString[0], MUIA_String_Contents, "");

            SET(data->netped_DNSString[1], MUIA_Disabled, TRUE);
            GET(data->netped_DNSString[1], MUIA_String_Contents, &str);
            SetDNS(1, str);
            SET(data->netped_DNSString[1], MUIA_String_Contents, "");
        }
        else
        {
            /* Enable text boxes, and reset their previous values */

            SET(data->netped_gateString, MUIA_Disabled, FALSE);
            SET(data->netped_gateString, MUIA_String_Contents, GetGate());

            SET(data->netped_DNSString[0], MUIA_Disabled, FALSE);
            SET(data->netped_DNSString[0], MUIA_String_Contents, GetDNS(0));

            SET(data->netped_DNSString[1], MUIA_Disabled, FALSE);
            SET(data->netped_DNSString[1], MUIA_String_Contents, GetDNS(1));
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
            iface.name[strlen(iface.name) - 1] += entries;
            SetUp(&iface, TRUE);    // new entries are UP
            DoMethod
            (
                data->netped_interfaceList,
                MUIM_List_InsertSingle, &iface, MUIV_List_Insert_Bottom
            );
        }

        /* Warn about DHCP limitations with more than one interface */
        if (entries == 1)
            DisplayErrorMessage(self, MULTIPLE_IFACES);

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

/*
    Shows content of current list entry in the host window.
*/
IPTR NetPEditor__MUIM_NetPEditor_ShowHostEntry
(
    Class *CLASS, Object *self,
    Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    struct Host *host;

    DoMethod
    (
        data->netped_hostList,
        MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &host
    );
    if (host)
    {
        SET(data->netped_hostRemoveButton, MUIA_Disabled, FALSE);
        SET(data->netped_hostEditButton, MUIA_Disabled, FALSE);

        SET(data->netped_hostAddressString, MUIA_String_Contents, GetHostAddress(host));
        SET(data->netped_hostNamesString, MUIA_String_Contents, GetHostNames(host));
    }
    else
    {
        SET(data->netped_hostRemoveButton, MUIA_Disabled, TRUE);
        SET(data->netped_hostEditButton, MUIA_Disabled, TRUE);
        SET(data->netped_hostWindow, MUIA_Window_Open, FALSE);
    }
    return 0;
}

IPTR NetPEditor__MUIM_NetPEditor_EditHostEntry
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
        LONG entries = XGET(data->netped_hostList, MUIA_List_Entries);
        if (entries < MAXHOSTS)
        {
            struct Host host;
            InitHost(&host);
            DoMethod
            (
                data->netped_hostList,
                MUIM_List_InsertSingle, &host, MUIV_List_Insert_Bottom
            );
        }
        SET(data->netped_hostList, MUIA_List_Active, entries + 1);
    }

    LONG active = XGET(data->netped_hostList, MUIA_List_Active);
    if (active != MUIV_List_Active_Off)
    {
        SET(data->netped_hostWindow, MUIA_Window_Open, TRUE);
    }

    return 0;
}

/*
    Store data from host window back in current list entry
*/
IPTR NetPEditor__MUIM_NetPEditor_ApplyHostEntry
(
    Class *CLASS, Object *self,
    Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    LONG active = XGET(data->netped_hostList, MUIA_List_Active);
    if (active != MUIV_List_Active_Off)
    {
        struct Host host;
        SetHost
        (
            &host,
            (STRPTR)XGET(data->netped_hostNamesString, MUIA_String_Contents),
            (STRPTR)XGET(data->netped_hostAddressString, MUIA_String_Contents)
        );
        DoMethod(data->netped_hostList, MUIM_List_Remove, active);
        DoMethod(data->netped_hostList, MUIM_List_InsertSingle, &host, active);
        SET(data->netped_hostList, MUIA_List_Active, active);
        SET(self, MUIA_PrefsEditor_Changed, TRUE);
    }

    return 0;
}

/*
    Shows content of current list entry in the network window.
*/
IPTR NetPEditor__MUIM_NetPEditor_ShowNetEntry
(
    Class *CLASS, Object *self,
    Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    struct Network *net;

    DoMethod
    (
        data->netped_networkList,
        MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &net
    );
    if (net)
    {
        SET(data->netped_netRemoveButton, MUIA_Disabled, FALSE);
        SET(data->netped_netEditButton, MUIA_Disabled, FALSE);

        SET(data->netped_sSIDString, MUIA_String_Contents, GetNetworkName(net));
        SET(data->netped_encType, MUIA_Cycle_Active, GetEncType(net));
        SET(data->netped_keyType, MUIA_Cycle_Active, net->keyIsHex ? 1 : 0);
        SET(data->netped_keyString, MUIA_String_Contents, GetKey(net));
        SET(data->netped_adHocState, MUIA_Selected, GetAdHoc(net) ? 1 : 0);
        SET(data->netped_hiddenState, MUIA_Selected, GetHidden(net) ? 1 : 0);
    }
    else
    {
        SET(data->netped_netRemoveButton, MUIA_Disabled, TRUE);
        SET(data->netped_netEditButton, MUIA_Disabled, TRUE);
        SET(data->netped_netWindow, MUIA_Window_Open, FALSE);
    }
    return 0;
}

IPTR NetPEditor__MUIM_NetPEditor_EditNetEntry
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
        LONG entries = XGET(data->netped_networkList, MUIA_List_Entries);
        if (entries < MAXNETWORKS)
        {
            struct Network net;
            InitNetwork(&net);
            DoMethod
            (
                data->netped_networkList,
                MUIM_List_InsertSingle, &net, MUIV_List_Insert_Bottom
            );
        }
        SET(data->netped_networkList, MUIA_List_Active, entries + 1);
    }

    LONG active = XGET(data->netped_networkList, MUIA_List_Active);
    if (active != MUIV_List_Active_Off)
    {
        SET(data->netped_netWindow, MUIA_Window_Open, TRUE);
    }

    return 0;
}

/*
    Store data from network window back in current list entry
*/
IPTR NetPEditor__MUIM_NetPEditor_ApplyNetEntry
(
    Class *CLASS, Object *self,
    Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    LONG active = XGET(data->netped_networkList, MUIA_List_Active);
    if (active != MUIV_List_Active_Off)
    {
        struct Network net;
        SetNetwork
        (
            &net,
            (STRPTR)XGET(data->netped_sSIDString, MUIA_String_Contents),
            XGET(data->netped_encType, MUIA_Cycle_Active),
            (STRPTR)XGET(data->netped_keyString, MUIA_String_Contents),
            XGET(data->netped_keyType, MUIA_Cycle_Active),
            XGET(data->netped_hiddenState, MUIA_Selected),
            XGET(data->netped_adHocState, MUIA_Selected)
        );
        DoMethod(data->netped_networkList, MUIM_List_Remove, active);
        DoMethod(data->netped_networkList, MUIM_List_InsertSingle, &net, active);
        SET(data->netped_networkList, MUIA_List_Active, active);
        SET(self, MUIA_PrefsEditor_Changed, TRUE);
    }

    return 0;
}

/*
    Shows content of current list entry in the file-server window.
*/
IPTR NetPEditor__MUIM_NetPEditor_ShowServerEntry
(
    Class *CLASS, Object *self,
    Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    struct Server *server;

    DoMethod
    (
        data->netped_serverList,
        MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &server
    );
    if (server)
    {
        SET(data->netped_serverRemoveButton, MUIA_Disabled, FALSE);
        SET(data->netped_serverEditButton, MUIA_Disabled, FALSE);

        SET(data->netped_serverDevice, MUIA_String_Contents, GetServerDevice(server));
        SET(data->netped_serverActive, MUIA_Selected, GetServerActive(server) ? 1 : 0);
        SET(data->netped_serverHost, MUIA_String_Contents, GetServerHost(server));
        SET(data->netped_serverService, MUIA_String_Contents, GetServerService(server));
        SET(data->netped_serverUser, MUIA_String_Contents, GetServerUser(server));
        SET(data->netped_serverGroup, MUIA_String_Contents, GetServerGroup(server));
        SET(data->netped_serverPass, MUIA_String_Contents, GetServerPass(server));
    }
    else
    {
        SET(data->netped_serverRemoveButton, MUIA_Disabled, TRUE);
        SET(data->netped_serverEditButton, MUIA_Disabled, TRUE);
        SET(data->netped_serverWindow, MUIA_Window_Open, FALSE);
    }
    return 0;
}

IPTR NetPEditor__MUIM_NetPEditor_EditServerEntry
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
        LONG entries = XGET(data->netped_serverList, MUIA_List_Entries);
        if (entries < MAXSERVERS)
        {
            struct Server server;
            InitServer(&server,
                (STRPTR)XGET(data->netped_domainString, MUIA_String_Contents));
            server.device[strlen(server.device) - 1] += entries;
            DoMethod
            (
                data->netped_serverList,
                MUIM_List_InsertSingle, &server, MUIV_List_Insert_Bottom
            );
        }
        SET(data->netped_serverList, MUIA_List_Active, entries + 1);
    }

    LONG active = XGET(data->netped_serverList, MUIA_List_Active);
    if (active != MUIV_List_Active_Off)
    {
        SET(data->netped_serverWindow, MUIA_Window_Open, TRUE);
    }

    return 0;
}

/*
    Store data from file-server window back in current list entry
*/
IPTR NetPEditor__MUIM_NetPEditor_ApplyServerEntry
(
    Class *CLASS, Object *self,
    Msg message
)
{
    struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    LONG active = XGET(data->netped_serverList, MUIA_List_Active);
    if (active != MUIV_List_Active_Off)
    {
        struct Server server;
        SetServer
        (
            &server,
            (STRPTR)XGET(data->netped_serverDevice, MUIA_String_Contents),
            (STRPTR)XGET(data->netped_serverHost, MUIA_String_Contents),
            (STRPTR)XGET(data->netped_serverService, MUIA_String_Contents),
            (STRPTR)XGET(data->netped_serverUser, MUIA_String_Contents),
            (STRPTR)XGET(data->netped_serverGroup, MUIA_String_Contents),
            (STRPTR)XGET(data->netped_serverPass, MUIA_String_Contents),
            XGET(data->netped_serverActive, MUIA_Selected)
        );
        DoMethod(data->netped_serverList, MUIM_List_Remove, active);
        DoMethod(data->netped_serverList, MUIM_List_InsertSingle, &server, active);
        SET(data->netped_serverList, MUIA_List_Active, active);
        SET(self, MUIA_PrefsEditor_Changed, TRUE);
    }

    return 0;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_20
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
    MUIM_NetPEditor_ApplyEntry,     Msg,
    MUIM_NetPEditor_ShowHostEntry,  Msg,
    MUIM_NetPEditor_EditHostEntry,  struct MUIP_NetPEditor_EditEntry *,
    MUIM_NetPEditor_ApplyHostEntry, Msg,
    MUIM_NetPEditor_ShowNetEntry,   Msg,
    MUIM_NetPEditor_EditNetEntry,   struct MUIP_NetPEditor_EditEntry *,
    MUIM_NetPEditor_ApplyNetEntry,  Msg,
    MUIM_NetPEditor_ShowServerEntry,   Msg,
    MUIM_NetPEditor_EditServerEntry,   struct MUIP_NetPEditor_EditEntry *,
    MUIM_NetPEditor_ApplyServerEntry,  Msg
);
