/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: layers.library Resident and initialization.
    Lang: english
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include "libdefs.h"
#include "layers_extfuncs.h"

#include <graphics/clip.h>
#include <graphics/regions.h>
#include <graphics/layers.h>
#include <aros/asmcall.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

extern const UBYTE Layers_name[];
extern const UBYTE Layers_version[];
extern UBYTE Layers_dearray[];
int Layers_start(void);
void patchlist(void);
extern const char END;

int Layers_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

struct SpecialResident
{
    struct Resident res;
    ULONG magiccookie;
    UBYTE *statusarray;
    UWORD maxslot;
};

#define SR_COOKIE 0x4afa4afb

const struct SpecialResident Layers_resident =
{
    {
    RTC_MATCHWORD,
    (struct Resident *)&Layers_resident,
    (APTR)&END,
    RTF_COLDSTART,
    LIBVERSION,
    NT_LIBRARY,
    63,		/* priority; just after layers.library */
    (STRPTR)Layers_name,
    (STRPTR)&Layers_version[6],
    &Layers_start
    },
    SR_COOKIE,		/* magic cookie to recognize a patchable library */
    Layers_dearray,	/* pointer to array of function status bytes */
    36			/* highest vector slot in this library */
};

UBYTE Layers_dearray[] =
{
    /* 36 functions in layers.library V40 (plus one for offset 0) */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*   0-  9 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  10- 19 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  20- 29 */
    1, 1, 1, 1, 1, 1, 1           /*  30- 39 */
};

const UBYTE Layers_name[]="layers.strap";

const UBYTE Layers_version[]="$VER: layers.strap 41.2 (02.11.1997)";

#define SetFunc(offset,name) \
{ \
    if(Layers_dearray[offset]) \
	SetFunction((struct Library *)LayersBase, (offset * -6), (APTR)&AROS_SLIB_ENTRY(name,Layers,offset)); \
}

/* use this to disable a setfunc that doesn't work yet */
#define SetFunc0(offset,name) /* eps */

/* use this to disable a setfunc that works (to keep the 2 types apart), so
 * the malfunctioning function can be isolated
 * If this doesn't make sense, don't worry. :-)
 */
#define SetFunc1(offset,name) /* eps */

int Layers_start(void)
{
    struct Library *LayersBase;
    struct Library *SysBase = *(void **)4;

    D(bug("\nlayers.strap installing...\n"));
    D(patchlist());

    if( (LayersBase = OpenLibrary("layers.library", 37)))
    {
	/*
	 * fc = functions correctly (according to observations or test program)
	 * ni = not implemented
	 */
	SetFunc1( 5, InitLayers);		// fc
	SetFunc1( 6, CreateUpfrontLayer);	// fc
	SetFunc1( 7, CreateBehindLayer);	// fc
	SetFunc0( 8, UpfrontLayer);		// ni
	SetFunc0( 9, BehindLayer);		// ni
	SetFunc1(10, MoveLayer);		// fc
	SetFunc1(11, SizeLayer);		// fc
	SetFunc0(12, ScrollLayer);		// ni
	SetFunc0(13, BeginUpdate);		// ni
	SetFunc0(14, EndUpdate);		// ni
	SetFunc0(15, DeleteLayer);		// ni
	SetFunc1(16, LockLayer);		// fc
	SetFunc1(17, UnlockLayer);		// fc
	SetFunc0(18, LockLayers);		// ni
	SetFunc0(19, UnlockLayers);		// ni
	SetFunc1(20, LockLayerInfo);		// fc
	SetFunc1(21, SwapBitsRastPortClipRect);	// fc
	SetFunc1(22, WhichLayer);		// fc
	SetFunc1(23, UnlockLayerInfo);		// fc
	SetFunc1(24, NewLayerInfo);		// fc
	SetFunc1(25, DisposeLayerInfo);		// fc
	SetFunc1(26, FattenLayerInfo);		// fc
	SetFunc1(27, ThinLayerInfo);		// fc
	SetFunc0(28, MoveLayerInFrontOf);	// ni
	SetFunc0(29, InstallClipRegion);	// ni
	SetFunc0(30, MoveSizeLayer);		// ni
	SetFunc1(31, CreateUpfrontHookLayer);	// fc
	SetFunc0(32, CreateBehindHookLayer);	// ni
	SetFunc1(33, InstallLayerHook);		// fc
	SetFunc1(34, InstallLayerInfoHook);	// fc
	SetFunc1(35, SortLayerCR);		// fc
	SetFunc1(36, DoHookClipRects);		// fc

	/*
	 * General note about these functions: superbitmap handling is untested!
         */

	CloseLibrary(LayersBase);
    }

    return 0;
}

D(void patchlist(void)
{
    int i;

    for(i = 1; i < Layers_resident.maxslot; i++)
	kprintf(Layers_dearray[i] ? "+" : "-");

    kprintf("\n\n");
});
