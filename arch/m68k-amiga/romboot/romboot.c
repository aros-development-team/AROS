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

#define _STR(A) #A
#define STR(A) _STR(A)

#define NAME "romboot"
#define VERSION 41
#define REVISION 1

static AROS_UFP3 (APTR, Init,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT name_string[] = NAME;
static const TEXT version_string[] =
   NAME " " STR(VERSION) "." STR(REVISION) " " ADATE "\n";

extern void romboot_end(void);

const struct Resident rb_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rb_tag,
   (APTR)&romboot_end,
   RTF_COLDSTART,
   VERSION,
   NT_UNKNOWN,
   -9, /* this MUST be run before uaegfx! */
   (STRPTR)name_string,
   (STRPTR)version_string,
   (APTR)Init
};

// ROMTAG INIT time
static void romtaginit(struct ExpansionBase *ExpansionBase)
{
	struct Node *node;
	// look for possible romtags in expansion ROM image and InitResident() them if found
	D(bug("romtaginit\n"));
	ObtainConfigBinding();
	ForeachNode(&ExpansionBase->BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		if ((configDev->cd_Flags & CDF_CONFIGME) && (configDev->cd_Rom.er_Type & ERTF_DIAGVALID) &&
		    configDev->cd_Rom.er_DiagArea && (configDev->cd_Rom.er_DiagArea->da_Config & DAC_BOOTTIME) == DAC_CONFIGTIME) {
			struct Resident *res;
			UWORD *romptr = (UWORD*)configDev->cd_Rom.er_DiagArea;
			UWORD *romend = (UWORD*)(((UBYTE*)configDev->cd_Rom.er_DiagArea) + configDev->cd_Rom.er_DiagArea->da_Size - 26); // 26 = real sizeof(struct Resident)!
			struct CurrentBinding cb = {
			    .cb_ConfigDev = configDev
			};
			SetCurrentBinding(&cb, sizeof(cb));
			while (romptr <= romend) {
				res = (struct Resident*)romptr;
				if (res->rt_MatchWord == RTC_MATCHWORD && res->rt_MatchTag == res) {
					D(bug("Diag board %p InitResident %p (V=%d P=%d F=%02x '%s' '%s')\n",
						configDev->cd_BoardAddr, res, res->rt_Version, res->rt_Pri, res->rt_Flags,
						res->rt_Name != NULL ? (char*)res->rt_Name : "<null>",
						res->rt_IdString != NULL ? (char*)res->rt_IdString : "<null>"));
					InitResident(res, BNULL);
					break; /* must not keep looking */
				}
				romptr++;
			}
		}
	}
	ReleaseConfigBinding();
	D(bug("romtaginit done\n"));
}

/* Stupid hack.
 * romtaginit() would initialize WinUAE built-in uaegfx.card which unfortunately also
 * disables direct RTG uaelib calls that uaegfx needs if uaelib is not called at least once.
 * We need to do this here because it was wrong to call romtaginit() after uaegfx, there
 * are RTG boards that are only active after rormtaginit, for example PicassoIV.
 */

static void uaegfxhack(APTR uaeres, UBYTE *name)
{
    asm volatile (
	"move.l %0,%%a6\n"
	"move.l %1,%%a0\n"
	"jsr -6(%%a6)\n"
	"tst.l %%d0\n"
	"beq.s 0f\n"
	"move.l %%d0,%%a0\n"
	/* 35 = return if RTG enabled, safe function to call */
	"moveq #35,%%d0\n"
	"move.l %%d0,-(%%sp)\n"
	"jsr (%%a0)\n"
	"addq.l #4,%%sp\n"
	"0:\n"
	: : "m" (uaeres), "m" (name) : "d0", "d1", "a0", "a1", "a6"
   );
}

static AROS_UFH3 (APTR, Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
   AROS_USERFUNC_INIT

   struct ExpansionBase *eb = (struct ExpansionBase*)TaggedOpenLibrary(TAGGEDOPEN_EXPANSION);
   APTR res;

   res = OpenResource("uae.resource");
   if (res)
	uaegfxhack(res, "uaelib_demux");

   romtaginit(eb);

   CloseLibrary((struct Library*)eb);

   AROS_USERFUNC_EXIT

   return NULL;
}
