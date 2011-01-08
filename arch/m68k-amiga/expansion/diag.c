
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>

#include "expansion_intern.h"

/* This is first RTF_COLDSTART resident.
 * Update eb_SysBase, call DAC_CONFIGTIME.
 */

#undef SysBase

#define _STR(A) #A
#define STR(A) _STR(A)

#define NAME "diag init"
#define VERSION 41
#define REVISION 1

static AROS_UFP3 (APTR, Init,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT name_string[] = NAME;
static const TEXT version_string[] =
   NAME " " STR(VERSION) "." STR(REVISION) " (" __DATE__ ")\n";

const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_COLDSTART,
   VERSION,
   NT_UNKNOWN,
   105,
   (STRPTR)name_string,
   (STRPTR)version_string,
   (APTR)Init
};

static BOOL calldiagrom(struct ExpansionBase *ExpansionBase, struct ExecBase *sb, struct ConfigDev *configDev, UBYTE whenflag)
{
	struct DiagArea *diag = configDev->cd_Rom.er_DiagArea;
	UWORD offset = DAC_CONFIGTIME ? diag->da_DiagPoint : diag->da_BootPoint;
	APTR code = (APTR)(((UBYTE*)diag) + offset);
	BOOL ret;
	
	// call autoconfig ROM da_DiagPoint or bootpoint da_BootPoint
	D(bug("%x %x %x %x\n", code, configDev->cd_BoardAddr, diag, configDev));
	ret = AROS_UFC5(BOOL, code,
   		AROS_UFCA(APTR, configDev->cd_BoardAddr, A0),
		AROS_UFCA(struct DiagArea*, diag, A2),
		AROS_UFCA(struct ConfigDev*, configDev, A3),
		AROS_UFCA(struct ExpansionBase*, ExpansionBase, A5),
		AROS_UFCA(struct ExecBase*, sb, A6));
	return ret;
}

static void callroms(struct ExpansionBase *ExpansionBase, UBYTE whenflag)
{
	struct Node *node;
	D(bug("callroms %x\n", whenflag));
	ForeachNode(&IntExpBase(ExpansionBase)->eb_BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		if (configDev->cd_Rom.er_DiagArea && (configDev->cd_Rom.er_DiagArea->da_Config & DAC_BOOTTIME) == whenflag) {
			if (!calldiagrom(ExpansionBase, IntExpBase(ExpansionBase)->eb_SysBase, configDev, whenflag)) {
				FreeMem(configDev->cd_Rom.er_DiagArea, configDev->cd_Rom.er_DiagArea->da_Size);
				configDev->cd_Rom.er_DiagArea = NULL;
			}	
		}
	}
}

static AROS_UFH3 (APTR, Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
   AROS_USERFUNC_INIT

   struct ExpansionBase *eb = (struct ExpansionBase*)FindName(&SysBase->LibList, "expansion.library");
   if (!eb)
	Alert(AT_DeadEnd | AO_ExpansionLib);
   ((struct IntExpansionBase*)eb)->eb_SysBase = SysBase;
   callroms(eb, DAC_CONFIGTIME);

   AROS_USERFUNC_EXIT

   return NULL;
}