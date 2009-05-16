/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
 */

#define MUIMASTER_YES_INLINE_STDARG

//#define NO_INLINE_STDARG

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

/*** Instance Data **********************************************************/
struct NetPEditor_DATA {
    Object  *netped_deviceString,
            *netped_IPString,
            *netped_maskString,
            *netped_gateString,
            *netped_DNSString[2],
            *netped_hostString,
            *netped_domainString,
            *netped_DHCPState,
            *netped_Autostart;
};

BOOL Gadgets2NetworkPrefs(struct NetPEditor_DATA *data)

{
	STRPTR str = NULL;
	IPTR lng = 0;

	GET(data->netped_deviceString, MUIA_String_Contents, &str);
	SetDevice(str);
	GET(data->netped_IPString, MUIA_String_Contents, &str);
	SetIP(str);
	GET(data->netped_maskString, MUIA_String_Contents, &str);
	SetMask(str);
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
	GET(data->netped_DHCPState, MUIA_Cycle_Active, &lng);
	SetDHCP(lng);
    GET(data->netped_Autostart, MUIA_Selected, &lng);
    SetAutostart(lng);

	return TRUE;
}

BOOL NetworkPrefs2Gadgets
(
	struct NetPEditor_DATA *data
)
{
	NNSET(data->netped_deviceString, MUIA_String_Contents, (IPTR)GetDevice());
	NNSET(data->netped_IPString, MUIA_String_Contents, (IPTR)GetIP());
	NNSET(data->netped_maskString, MUIA_String_Contents, (IPTR)GetMask());
	NNSET(data->netped_gateString, MUIA_String_Contents, (IPTR)GetGate());
	NNSET(data->netped_DNSString[0], MUIA_String_Contents, (IPTR)GetDNS(0));
	NNSET(data->netped_DNSString[1], MUIA_String_Contents, (IPTR)GetDNS(1));
	NNSET(data->netped_hostString, MUIA_String_Contents, (IPTR)GetHost());
	NNSET(data->netped_domainString, MUIA_String_Contents, (IPTR)GetDomain());
	NNSET(data->netped_DHCPState, MUIA_Cycle_Active, (IPTR)GetDHCP());
    NNSET(data->netped_Autostart, MUIA_Selected, (IPTR)GetAutostart());
	
	return TRUE;
}

/*** Methods ****************************************************************/
Object * NetPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object  *deviceString, *IPString, *maskString, *gateString, 
            *DNSString[2], *hostString, *domainString, *DHCPState, *autostart;

    DHCPCycle[0] = _(MSG_IP_MODE_MANUAL);
    DHCPCycle[1] = _(MSG_IP_MODE_DHCP);

    NetworkTabs[0] = _(MSG_TAB_IP_CONFIGURATION);
    NetworkTabs[1] = _(MSG_TAB_COMPUTER_NAME);
	
	self = (Object *)DoSuperNewTags
	(
		CLASS, self, NULL,

		MUIA_PrefsEditor_Name, __(MSG_NAME),
		MUIA_PrefsEditor_Path, (IPTR)"AROSTCP/arostcp.prefs",


		Child, RegisterGroup((IPTR)NetworkTabs),
			Child, (IPTR)ColGroup(2),
				Child, (IPTR)Label2(__(MSG_DEVICE)),Child, (IPTR)PopaslObject,
					MUIA_Popasl_Type,              ASL_FileRequest,
					ASLFO_MaxHeight,               100,
					MUIA_Popstring_String,  (IPTR)(deviceString = (Object *)StringObject, TextFrame, MUIA_Background, MUII_TextBack, End),
					MUIA_Popstring_Button,  (IPTR)PopButton(MUII_PopUp),
				End,

				Child, (IPTR)Label2(__(MSG_IP_MODE)), Child, (IPTR)(DHCPState = (Object *)CycleObject, MUIA_Cycle_Entries, (IPTR)DHCPCycle, End),
				Child, (IPTR)Label2(__(MSG_IP)), Child, (IPTR)(IPString = (Object *)StringObject, TextFrame, MUIA_String_Accept, (IPTR)"0123456789.", End),
				Child, (IPTR)Label2(__(MSG_MASK)),Child, (IPTR)(maskString = (Object *)StringObject, TextFrame, MUIA_String_Accept, (IPTR)"0123456789.", End),
				Child, (IPTR)Label2(__(MSG_GATE)),Child, (IPTR)(gateString = (Object *)StringObject, TextFrame, MUIA_String_Accept, (IPTR)"0123456789.", End),
				Child, (IPTR)Label2(__(MSG_DNS1)),Child, (IPTR)(DNSString[0] = (Object *)StringObject, TextFrame, MUIA_String_Accept, (IPTR)"0123456789.", End),
				Child, (IPTR)Label2(__(MSG_DNS2)),Child, (IPTR)(DNSString[1] = (Object *)StringObject, TextFrame, MUIA_String_Accept, (IPTR)"0123456789.", End),
                Child, (IPTR)Label2(__(MSG_AUTOSTART_STACK)),
                Child, (IPTR)HGroup,
                    Child, (IPTR)(autostart = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)HVSpace,
                End,
			End,

			Child, (IPTR)ColGroup(2),
				Child, (IPTR)Label2(__(MSG_HOST_NAME)), Child, (IPTR)(hostString = (Object *)StringObject, TextFrame, MUIA_String_Accept, (IPTR)"0123456789abcdefghijklmnopqrstuvwxyz-", End),
				Child, (IPTR)Label2(__(MSG_DOMAIN_NAME)), Child, (IPTR)(domainString = (Object *)StringObject, TextFrame, MUIA_String_Accept, (IPTR)"0123456789abcdefghijklmnopqrstuvwxyz-.", End),
			End,

		End, // register

		TAG_DONE
	);

	
	if (self != NULL) {
		struct NetPEditor_DATA *data = INST_DATA(CLASS, self);
		data->netped_deviceString  = deviceString;
		data->netped_IPString = IPString;
		data->netped_maskString = maskString;
		data->netped_gateString = gateString;
		data->netped_DNSString[0] = DNSString[0];
		data->netped_DNSString[1] = DNSString[1];
		data->netped_hostString = hostString;
		data->netped_domainString = domainString;
		data->netped_DHCPState = DHCPState;
        data->netped_Autostart = autostart;
		
		/*-- Setup notifications -------------------------------------------*/
		DoMethod
		(
			deviceString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
			(IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
		);
		DoMethod
		(
			IPString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
			(IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
		);
		DoMethod
		(
			maskString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
			(IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
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
			autostart, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
			(IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
		);
		DoMethod
		(
			DHCPState, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 1, MUIM_NetPEditor_IPModeChanged 

		);
	}

	return self;
}

IPTR NetPEditor__MUIM_PrefsEditor_Save
(
	Class *CLASS, Object *self, Msg message
)
{
	struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

	Gadgets2NetworkPrefs(data);

    if (SaveNetworkPrefs())
    {
		SET(self, MUIA_PrefsEditor_Changed, FALSE);
		SET(self, MUIA_PrefsEditor_Testing, FALSE);
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

	Gadgets2NetworkPrefs(data);

    if (UseNetworkPrefs())
    {
		SET(self, MUIA_PrefsEditor_Changed, FALSE);
		SET(self, MUIA_PrefsEditor_Testing, FALSE);
        return TRUE;
    }

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

    DoMethod(self, MUIM_NetPEditor_IPModeChanged);

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

    DoMethod(self, MUIM_NetPEditor_IPModeChanged);

	return success;
}

IPTR NetPEditor__MUIM_NetPEditor_IPModeChanged
(
	Class *CLASS, Object *self,
	Msg message
)
{
	struct NetPEditor_DATA *data = INST_DATA(CLASS, self);

    IPTR lng = 0;

    GetAttr(MUIA_Cycle_Active, data->netped_DHCPState, &lng);

    if (lng==1) 
    {
        set(data->netped_IPString, MUIA_Disabled, TRUE);
        set(data->netped_gateString, MUIA_Disabled, TRUE);
        set(data->netped_DNSString[0], MUIA_Disabled, TRUE);
        set(data->netped_DNSString[1], MUIA_Disabled, TRUE);
        set(data->netped_maskString, MUIA_Disabled, TRUE);
    } 
    else 
    {
        set(data->netped_IPString, MUIA_Disabled, FALSE);
        set(data->netped_gateString, MUIA_Disabled, FALSE);
        set(data->netped_DNSString[0], MUIA_Disabled, FALSE);
        set(data->netped_DNSString[1], MUIA_Disabled, FALSE);
        set(data->netped_maskString, MUIA_Disabled, FALSE);
    }	

	SET(self, MUIA_PrefsEditor_Changed, TRUE);

    return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_6
(
	NetPEditor, NULL, MUIC_PrefsEditor, NULL,
	OM_NEW,                    struct opSet *,
	MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
	MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *,
	MUIM_PrefsEditor_Save,     Msg,
	MUIM_PrefsEditor_Use,      Msg,
    MUIM_NetPEditor_IPModeChanged, Msg
);
