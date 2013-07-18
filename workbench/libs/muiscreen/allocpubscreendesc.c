/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/muiscreen.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH1(struct MUI_PubScreenDesc *, MUIS_AllocPubScreenDesc,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_PubScreenDesc *, src,  A0),

/*  LOCATION */
	struct Library *, MUIScreenBase, 5, MUIScreen)

/*  FUNCTION

    INPUTS

    RESULT
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT
    
    struct MUI_PubScreenDesc *psd;
    
    D(bug("MUIS_AllocPubScreenDesc(%p)\n", src));
    
    psd = AllocMem (sizeof (struct MUI_PubScreenDesc), MEMF_ANY|MEMF_CLEAR);

    if(src)
        CopyMem(src, psd, sizeof(struct MUI_PubScreenDesc));
    else
    {
	/* Copy default values from Workbench screen */
	struct Screen *wbscreen = LockPubScreen(NULL);
	CopyMem(PSD_INITIAL_NAME, psd->Name, sizeof(PSD_INITIAL_NAME));
	CopyMem(PSD_INITIAL_TITLE, psd->Title, sizeof(PSD_INITIAL_TITLE));
	psd->DisplayID = GetVPModeID(&wbscreen->ViewPort);
	psd->DisplayWidth = wbscreen->Width;
	psd->DisplayHeight = wbscreen->Height;
	psd->DisplayDepth = GetBitMapAttr(wbscreen->RastPort.BitMap, BMA_DEPTH);
	psd->OverscanType = OSCAN_TEXT;
	psd->AutoScroll = (wbscreen->Flags & AUTOSCROLL) ? TRUE : FALSE;
	psd->NoDrag = FALSE;
	psd->Exclusive = FALSE;
	psd->Interleaved = (GetBitMapAttr(wbscreen->RastPort.BitMap, BMA_FLAGS) & BMF_INTERLEAVED);
	psd->SysDefault = FALSE;
	psd->Behind = (wbscreen->Flags & SCREENBEHIND) ? TRUE : FALSE;
	psd->AutoClose = FALSE;
	psd->CloseGadget = FALSE;

	int def_pens[] = { 0, 1, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1 };
	int i;
	for(i = 0; i < sizeof(def_pens)/sizeof(def_pens[0]); i++)
	{
	    psd->SystemPens[i] = def_pens[i];
	}

	struct MUI_RGBcolor col[8] =
	{
	    { 0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA },
	    { 0x00000000,0x00000000,0x00000000 },
	    { 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
	    { 0x66666666,0x88888888,0xBBBBBBBB },
	    { 0xEEEEEEEE,0x44444444,0x44444444 },
	    { 0x55555555,0xDDDDDDDD,0x55555555 },
	    { 0x00000000,0x44444444,0xDDDDDDDD },
	    { 0xEEEEEEEE,0x99999999,0x00000000 }
	};
	
	for(i = 0; i < 8; i++)
	{
	    psd->Palette[i] = col[i];
	}
    }

    D(bug("Allocated struct MUI_PubScreenDesc %p\n", psd));

    return psd;
    
    AROS_LIBFUNC_EXIT
}
