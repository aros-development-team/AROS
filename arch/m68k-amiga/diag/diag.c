/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/expansion.h>
#include <aros/asmcall.h>
#include <libraries/expansionbase.h>
#include <libraries/configvars.h>
#include <libraries/configregs.h>

/* This is first RTF_COLDSTART resident.
 * Update ExpansionBase->ExecBase, call DAC_CONFIGTIME.
 */

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
   NAME " " STR(VERSION) "." STR(REVISION) " " ADATE "\n";

extern void diag_end(void);

const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)&diag_end,
   RTF_COLDSTART,
   VERSION,
   NT_UNKNOWN,
   105,
   (STRPTR)name_string,
   (STRPTR)version_string,
   (APTR)Init
};

D(
static void debugRAM(void)
{
	struct MemHeader *mh;
	ForeachNode(&SysBase->MemList, mh) {
		bug("%08x: %08x - %08x %08x %d '%s'\n",
			mh, mh->mh_Lower, mh->mh_Upper, mh->mh_Attributes,
			mh->mh_Node.ln_Pri, mh->mh_Node.ln_Name ? (const char *)mh->mh_Node.ln_Name : "<null>");
	}
}
)

static BOOL calldiagrom(struct ExpansionBase *ExpansionBase, struct ConfigDev *configDev)
{
	struct DiagArea *diag = configDev->cd_Rom.er_DiagArea;
	UWORD offset = diag->da_DiagPoint;
	APTR code = (APTR)(((UBYTE*)diag) + offset);
	ULONG ret;

	// call autoconfig ROM da_DiagPoint
	D(bug("Call boot rom @%p board %p diag %p configdev %p\n",
		code, configDev->cd_BoardAddr, diag, configDev));
	ret = AROS_UFC5(ULONG, code,
		AROS_UFCA(APTR, configDev->cd_BoardAddr, A0),
		AROS_UFCA(struct DiagArea*, diag, A2),
		AROS_UFCA(struct ConfigDev*, configDev, A3),
		AROS_UFCA(struct ExpansionBase*, ExpansionBase, A5),
		AROS_UFCA(struct ExecBase*, SysBase, A6));
	D(bug(ret ? "->success\n" : "->failed\n"));
	return ret != 0;
}


// read one byte from expansion autoconfig ROM
static void copyromdata(struct ConfigDev *configDev, UBYTE buswidth, UWORD size, UBYTE *out)
{
	volatile UBYTE *rom = (UBYTE*)(configDev->cd_BoardAddr + configDev->cd_Rom.er_InitDiagVec);
	UWORD offset = 0;
	
	switch (buswidth)
	{
		case DAC_NIBBLEWIDE:
			while (size-- > 0) {
				*out++ = (rom[offset * 4 + 0] & 0xf0) | ((rom[offset * 4 + 2] & 0xf0) >> 4);
				offset++;
			}
			break;
		case DAC_BYTEWIDE:
			while (size-- > 0) {
				*out++ = rom[offset * 2];
				offset++;
			}
			break;
		case DAC_WORDWIDE:
		default:
			/* AOS does it this way */
			CopyMem((void*)rom, out, size);
			break;
	}
}

static BOOL diagrom(struct ExpansionBase *ExpansionBase, struct ConfigDev *configDev)
{
	struct DiagArea *da, datmp;
	UBYTE da_config, buswidth;

	D(bug("Read boot ROM base=%p cd=%p type=%02x\n", configDev->cd_BoardAddr, configDev, configDev->cd_Rom.er_Type));

	if (!(configDev->cd_Rom.er_Type & ERTF_DIAGVALID) || !configDev->cd_Rom.er_InitDiagVec) {
		D(bug("Board without boot ROM\n"));
		return FALSE;
	}

	copyromdata(configDev, DAC_BYTEWIDE, 1, &da_config);
	/* NOTE: lower nibble may not be valid if actual bus type is not BYTEWIDE */
	D(bug("da_Config=%02x\n", da_config & 0xf0));
	buswidth = da_config & DAC_BUSWIDTH;
	if (buswidth == DAC_BUSWIDTH) // illegal
		return FALSE;
	if ((da_config & DAC_BOOTTIME) != DAC_CONFIGTIME)
		return FALSE;

	// read DiagArea only
	copyromdata(configDev, buswidth, sizeof(struct DiagArea), (UBYTE*)&datmp);

	D(bug("Size=%04x DiagPoint=%04x BootPoint=%04x Name=%04x\n",
		datmp.da_Size, datmp.da_DiagPoint, datmp.da_BootPoint, datmp.da_Name));
	if (datmp.da_Size < sizeof (struct DiagArea))
		return FALSE;

	da = AllocMem(datmp.da_Size, MEMF_PUBLIC);
	if (!da)
		return FALSE;

	configDev->cd_Rom.er_DiagArea = da;
	// read rom data, DiagArea is also copied again. AOS compatibility!
	copyromdata(configDev, buswidth, datmp.da_Size, (UBYTE*)da);
	
	D(if (da->da_Name != 0 && da->da_Name != 0xffff && da->da_Name < da->da_Size)
		bug("Name='%s'\n", (UBYTE*)da + da->da_Name);)

	return TRUE;
}

static void callroms(struct ExpansionBase *ExpansionBase)
{
	struct Node *node;
	D(bug("callroms\n"));
	ForeachNode(&ExpansionBase->BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		if (diagrom(ExpansionBase, configDev)) {
			if (!calldiagrom(ExpansionBase, configDev)) {
				FreeMem(configDev->cd_Rom.er_DiagArea, configDev->cd_Rom.er_DiagArea->da_Size);
				configDev->cd_Rom.er_DiagArea = NULL;
			}	
		}
	}
	D(bug("callroms done\n"));
}


void InitKickMemDiag(struct ExecBase *SysBase);

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

   eb->eb_Private02 = (IPTR)SysBase;

   callroms(eb);

   D(debugRAM());

   /* ArosBootStrap mode? Check for kick modules again if some of our kick modules
    * are located in diag initialized ram (Blizzard A1200 accelerator boards)
    */
   InitKickMemDiag(SysBase);

   AROS_USERFUNC_EXIT

   return NULL;
}
