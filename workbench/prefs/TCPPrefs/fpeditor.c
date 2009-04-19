/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id: fpeditor.c 21816 2004-06-25 12:35:29Z chodorowski $
 */

#define MUIMASTER_YES_INLINE_STDARG

//#define NO_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
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
#include "fpeditor.h"
#include "AROSTCPPrefs.h"

#include <proto/alib.h>
#include <utility/hooks.h>

/*** Moje *******************************************************************/
static const char *titles[] = { "IP Config", "Computer Name", NULL };
static const char *DHCPCycle[] = { "Manual", "Get address from DHCP", NULL };
static struct Hook DHCPHook;
// wróciæ z tym do lokalnego obszaru, albo na dowrót - zlikwidow¹æ GET i NNSET...
Object *interfString, *IPString, *maskString, *gateString, *DNSString[2], *hostString, *domainString, *DHCPState;
/*** Instance Data **********************************************************/
#define FP_COUNT (7)  /* Number of entries in fped_FontPrefs array */

struct FPEditor_DATA {
	struct FontPrefs fped_FontPrefs[FP_COUNT];
	Object           *fped_interfString,
	*fped_IPString,
	*fped_maskString,
	*fped_gateString,
	*fped_DNSString[2],
	*fped_hostString,
	*fped_domainString,
	*fped_DHCPState,
	*fped_Self;

};

/*** Macros *****************************************************************/
//#define FP(i) (&(data->fped_FontPrefs[(i)]))


void FlipDHCP()
{
	//printf("DEBUG: Jestem w DHCFlip\n");

	LONG lng = 0;

	GetAttr(MUIA_Cycle_Active, DHCPState, &lng);

	if (lng==1) {
		set(IPString, MUIA_Disabled, TRUE);
		set(gateString, MUIA_Disabled, TRUE);
		set(DNSString[0], MUIA_Disabled, TRUE);
		set(DNSString[1], MUIA_Disabled, TRUE);
		set(maskString, MUIA_Disabled, TRUE);
	} else {
		set(IPString, MUIA_Disabled, FALSE);
		set(gateString, MUIA_Disabled, FALSE);
		set(DNSString[0], MUIA_Disabled, FALSE);
		set(DNSString[1], MUIA_Disabled, FALSE);
		set(maskString, MUIA_Disabled, FALSE);
	}	
}

void BumpTCP()
{
	/*
	struct Task *tcp_task;
	if ((tcp_task = FindTask("bsdsocket.library")) != NULL)
	{
		//printf("DEBUG: Zabijam AROSTCP\n");
		Signal(tcp_task, SIGBREAKF_CTRL_C);
		// uruchomiæ jeszcze raz
	} 
	*/
	// execute s:arostcpd stop
	// execute s:arostcpd start
}

static ULONG DHCPNotify(struct Hook *hook, Object *object, IPTR *params)
{
	Object *self;
	Class *CLASS;

	self = (Object *)params[0];
	CLASS = (Class *)params[1];

	struct FPEditor_DATA *data = INST_DATA(CLASS, *self);
		
	SET(self, MUIA_PrefsEditor_Changed, TRUE);

	FlipDHCP();

    return 1;
}

BOOL Gadgets2FontPrefs(struct FPEditor_DATA *data)

{
	//printf("DEBUG: W gadgets2fontprefs przed setup...\n");

	STRPTR str = NULL;
	LONG lng = 0;

	//printf("DEBUG: interfejs\n",str);
	GET(data->fped_interfString, MUIA_String_Contents, &str);
	SetInterf(str);

	//printf("DEBUG: IP\n",str);
	GET(data->fped_IPString, MUIA_String_Contents, &str);
	SetIP(str);

	//printf("DEBUG: Mask\n",str);
	GET(data->fped_maskString, MUIA_String_Contents, &str);
	SetMask(str);

	//printf("DEBUG: Gate\n",str);
	GET(data->fped_gateString, MUIA_String_Contents, &str);
	SetGate(str);

	//printf("DEBUG: DNS0\n",str);
	GET(data->fped_DNSString[0], MUIA_String_Contents, &str);
	SetDNS(0, str);

	//printf("DEBUG: DNS1\n",str);
	GET(data->fped_DNSString[1], MUIA_String_Contents, &str);
	SetDNS(1, str);

	//printf("DEBUG: host get\n",str);
	GET(data->fped_hostString, MUIA_String_Contents, &str);
	//printf("DEBUG: host set\n",str);
	SetHost(str);

	//printf("DEBUG: get domain\n",str);
	GET(data->fped_domainString, MUIA_String_Contents, &str);
	//printf("DEBUG: set domain\n",str);
	SetDomain(str);

	//printf("DEBUG: DHCP\n",str);
	GET(data->fped_DHCPState, MUIA_Cycle_Active, &lng);
	//printf("DEBUG: w gadget2fontprefs: Muia cycle: %ld\n",lng);
	SetDHCP(lng);

	return TRUE;
}

BOOL FontPrefs2Gadgets
(
	struct FPEditor_DATA *data
)
{
	TEXT buffer[1000];

	// FIXME: error checking
	NNSET(data->fped_interfString, MUIA_String_Contents, (IPTR)GetInterf());
	NNSET(data->fped_IPString, MUIA_String_Contents, (IPTR)GetIP());
	NNSET(data->fped_maskString, MUIA_String_Contents, (IPTR)GetMask());
	NNSET(data->fped_gateString, MUIA_String_Contents, (IPTR)GetGate());

	NNSET(data->fped_DNSString[0], MUIA_String_Contents, (IPTR)GetDNS(0));
	NNSET(data->fped_DNSString[1], MUIA_String_Contents, (IPTR)GetDNS(1));
	NNSET(data->fped_hostString, MUIA_String_Contents, (IPTR)GetHost());
	NNSET(data->fped_domainString, MUIA_String_Contents, (IPTR)GetDomain());

	//printf("DEBUG: ustawiam fped_DHCPState na %d\n",GetDHCP());
	NNSET(data->fped_DHCPState, MUIA_Cycle_Active, (IPTR)GetDHCP());
	NNSET(data->fped_Self,MUIA_PrefsEditor_Changed,TRUE);
	FlipDHCP();
	
	return TRUE;
}

/*** Methods ****************************************************************/
Object *FPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
	//Object *interfString, *IPString, *maskString, *gateString, *DNSString[2], *hostString, *domainString, *DHCPState;

    DHCPHook.h_Entry = HookEntry;
    DHCPHook.h_SubEntry = (HOOKFUNC)DHCPNotify;
	
	long xxx=666;
	
	self = (Object *)DoSuperNewTags
	(
		CLASS, self, NULL,

		MUIA_PrefsEditor_Name,        "Proba",
		MUIA_PrefsEditor_Path, (IPTR)"AROSTCP/arostcp.prefs",


		Child, RegisterGroup(titles),

			Child, (IPTR)ColGroup(2),
				Child, (IPTR)Label2("Interface"),Child, (IPTR)PopaslObject,
					MUIA_Popasl_Type,              ASL_FileRequest,
					ASLFO_MaxHeight,               100,
					MUIA_Popstring_String,  (IPTR)(interfString = StringObject, TextFrame, MUIA_Background, MUII_TextBack, End),
					MUIA_Popstring_Button,  (IPTR)PopButton(MUII_PopUp),
				End,

				Child, (IPTR)Label2("IP Configuration"), Child, (IPTR)(DHCPState = CycleObject, MUIA_Cycle_Entries, DHCPCycle, End),
				Child, (IPTR)Label2("IP"), Child, (IPTR)(IPString = StringObject, TextFrame, MUIA_String_Accept, "0123456789.", End),
				Child, (IPTR)Label2("Mask"),Child, (IPTR)(maskString = StringObject, TextFrame, MUIA_String_Accept, "0123456789.", End),
				Child, (IPTR)Label2("Gate"),Child, (IPTR)(gateString = StringObject, TextFrame, MUIA_String_Accept, "0123456789.", End),
				Child, (IPTR)Label2("DNS 1"),Child, (IPTR)(DNSString[0] = StringObject, TextFrame, MUIA_String_Accept, "0123456789.", End),
				Child, (IPTR)Label2("DNS 2"),Child, (IPTR)(DNSString[1] = StringObject, TextFrame, MUIA_String_Accept, "0123456789.", End),
			End,

			Child, (IPTR)ColGroup(2),
				Child, (IPTR)Label2("Name"), Child, (IPTR)(hostString = StringObject, TextFrame, MUIA_String_Accept, "0123456789abcdefghijklmnopqrstuvwxyz-", End),
				Child, (IPTR)Label2("Domain"), Child, (IPTR)(domainString = StringObject, TextFrame, MUIA_String_Accept, "0123456789abcdefghijklmnopqrstuvwxyz-.", End),
			End,

		End, // register

		TAG_DONE
	);

	
	if (self != NULL) {
		struct FPEditor_DATA *data = INST_DATA(CLASS, self);
		data->fped_interfString  = interfString;
		data->fped_IPString = IPString;
		data->fped_maskString = maskString;
		data->fped_gateString = gateString;

		data->fped_DNSString[0] = DNSString[0];
		data->fped_DNSString[1] = DNSString[1];
		data->fped_hostString = hostString;
		data->fped_domainString = domainString;
		data->fped_DHCPState = DHCPState;
		data->fped_Self = self;
		
		/*-- Setup notifications -------------------------------------------*/
		DoMethod
		(
			interfString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
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
			DHCPState, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            //(IPTR)self, 3, MUIM_CallHook, (IPTR)&DHCPHook, (IPTR)&self
            (IPTR)self, 4, MUIM_CallHook, (IPTR)&DHCPHook, (IPTR)self, (IPTR)CLASS

		);
	}

	return self;
}


IPTR FPEditor__MUIM_PrefsEditor_Save
(
	Class *CLASS, Object *self, Msg message
)
{
	//printf("DEBUG: Jestem w Save\n");
	if (DoMethod(self, MUIM_PrefsEditor_Use)) return WriteTCPPrefs("ENVARC:AROSTCP/db");

	return FALSE;
}

IPTR FPEditor__MUIM_PrefsEditor_Use
(
	Class *CLASS, Object *self, Msg message
)
{
	struct FPEditor_DATA *data = INST_DATA(CLASS, self);
	Gadgets2FontPrefs(data);

	if (WriteTCPPrefs("ENV:AROSTCP/db")) {

		SET(self, MUIA_PrefsEditor_Changed, FALSE);
		SET(self, MUIA_PrefsEditor_Testing, FALSE);

		BumpTCP();
		return TRUE;
	}
	
	return FALSE;
}

IPTR FPEditor__MUIM_PrefsEditor_ImportFH
(
	Class *CLASS, Object *self,
	struct MUIP_PrefsEditor_ImportFH *message
)
{
	struct FPEditor_DATA *data = INST_DATA(CLASS, self);
	BOOL success = TRUE;

	//printf("DEBUG: Jestem w ImportFH\n");
	FontPrefs2Gadgets(data);

	return success;
}

IPTR FPEditor__MUIM_PrefsEditor_ExportFH
(
	Class *CLASS, Object *self,
	struct MUIP_PrefsEditor_ExportFH *message
)
{
	struct FPEditor_DATA *data = INST_DATA(CLASS, self);
	struct PrefHeader header;
	struct IFFHandle *handle;
	BOOL success = TRUE;
	LONG error   = 0;

	//printf("DEBUG: Jestem w ExportFH\n");
	FontPrefs2Gadgets(data);
	//Gadgets2FontPrefs(data);

	return success;
}



/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_5
(
	FPEditor, NULL, MUIC_PrefsEditor, NULL,
	OM_NEW,                    struct opSet *,
	MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
	MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *,
	MUIM_PrefsEditor_Save,     struct MUIP_PrefsEditor_Save *,
	MUIM_PrefsEditor_Use,      struct MUIP_PrefsEditor_Use *
);
