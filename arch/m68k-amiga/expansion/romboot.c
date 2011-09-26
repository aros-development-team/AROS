
#define DEBUG 1

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/expansion.h>
#include <aros/asmcall.h>

#include "expansion_intern.h"

#undef SysBase

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
   NAME " " STR(VERSION) "." STR(REVISION) " (" ADATE ")\n";

const struct Resident rb_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rb_tag,
   (APTR)(&rb_tag + 1),
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
	ForeachNode(&IntExpBase(ExpansionBase)->eb_BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		if (configDev->cd_Rom.er_DiagArea && (configDev->cd_Rom.er_DiagArea->da_Config & DAC_BOOTTIME) == DAC_CONFIGTIME && (configDev->cd_Flags & CDF_CONFIGME)) {
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
					D(bug("Diag board %08x InitResident %08x (%d %02x '%s')\n",
						configDev->cd_BoardAddr, res, res->rt_Pri, res->rt_Flags, res->rt_Name));
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

static void Enable68060SuperScalar(void)
{
    asm volatile (
	".text\n"
	/* enable supercalar */
	"dc.l	0x4e7a0808\n"	// movec %pcr,%d0
	"bset	#0,%d0\n"
	"dc.l	0x4e7b0808\n"	// movec %d0,%pcr
	/* enable code&data caches, store buffer and branch cache */
	"dc.l	0x4e7a0002\n"	// movec %cacr,%d0
	"or.l	#0xa0808000,%d0\n"
	"dc.l	0x4e7b0002\n"	// movec %d0,%cacr
	"rte\n"
    );
}
static ULONG Check68030MMU(void)
{
    register UWORD ret asm("%d0");
    asm volatile (
    	".chip 68030\n"
    	"subq.l	#4,%%sp\n"
    	"pmove	%%tc,(%%sp)\n"
    	"move.l	(%%sp),%%d0\n"
    	"addq.l	#4,%%sp\n"
    	"rte\n"
    	: "=r" (ret)
    );
    return ret;
};

static AROS_UFH3 (APTR, Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
   AROS_USERFUNC_INIT

   struct ExpansionBase *eb = (struct ExpansionBase*)TaggedOpenLibrary(TAGGEDOPEN_EXPANSION);
   romtaginit(eb);

   // enable 68040+ data caches and 68060 superscalar mode
   // this is not the right place but we can't enable them
   // any earlier (memory detection, boot roms that breaks if
   // full 68060 caching enabled)
   if (SysBase->AttnFlags & AFF_68060) {
	Supervisor((ULONG_FUNC)Enable68060SuperScalar);
   } else if (SysBase->AttnFlags & AFF_68040) {
	CacheControl(CACRF_EnableD, CACRF_EnableD);
   } else if (SysBase->AttnFlags & AFF_68030) {
	ULONG tc = Supervisor((ULONG_FUNC)Check68030MMU);
	if (tc & (1 << 31)) { /* Only if MMU enabled */
	    CacheControl(CACRF_EnableD, CACRF_EnableD);
	}
   }

   CloseLibrary((struct Library*)eb);

   AROS_USERFUNC_EXIT

   return NULL;
}