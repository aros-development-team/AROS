/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/muiscreen.h>
#include <libraries/iffparse.h>
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>
#include <proto/exec.h>
#include <proto/dos.h>
#define DEBUG 1
#include <aros/debug.h>

#include "fileformat.h"

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH1(struct MUI_PubScreenDesc *, MUIS_ReadPubFile,

/*  SYNOPSIS */
	AROS_LHA(APTR, pf,  A0),

/*  LOCATION */
	struct Library *, MUIScreenBase, 0x36, MUIScreen)

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

    D(bug("MUIS_ReadPubFile(%p)\n", pf));

    struct IFFHandle *iff = (struct IFFHandle *) pf;
    struct MUI_PubScreenDesc *retval = NULL;

    if (!StopChunk(iff, ID_PREF, ID_MPUB))
    {
	if (!ParseIFF(iff, IFFPARSE_SCAN))
	{
	    struct ContextNode *cn;
	    cn = CurrentChunk(iff);
	    if (cn->cn_Size == sizeof(struct MUI_PubScreenDescArray))
	    {
		struct MUI_PubScreenDesc desc;
		struct MUI_PubScreenDescArray desc_tmp;
		if (ReadChunkBytes(iff, &desc_tmp, sizeof(struct MUI_PubScreenDescArray)) == sizeof(struct MUI_PubScreenDescArray))
		{
		    desc.Version = ARRAY_TO_LONG(desc_tmp.Version);
		    CopyMem(desc_tmp.Name, desc.Name, sizeof(desc_tmp.Name));
		    CopyMem(desc_tmp.Title, desc.Title, sizeof(desc_tmp.Title));
		    CopyMem(desc_tmp.Font, desc.Font, sizeof(desc_tmp.Font));
		    CopyMem(desc_tmp.Background, desc.Background, sizeof(desc_tmp.Background));
		    desc.DisplayID = ARRAY_TO_LONG(desc_tmp.DisplayID);
		    desc.DisplayWidth = ARRAY_TO_WORD(desc_tmp.DisplayWidth);
		    desc.DisplayHeight = ARRAY_TO_WORD(desc_tmp.DisplayHeight);
		    desc.DisplayDepth = desc_tmp.DisplayDepth;
		    desc.OverscanType = desc_tmp.OverscanType;
		    desc.AutoScroll = desc_tmp.AutoScroll;
		    desc.NoDrag = desc_tmp.NoDrag;
		    desc.Exclusive = desc_tmp.Exclusive;
		    desc.Interleaved = desc_tmp.Interleaved;
		    desc.SysDefault = desc_tmp.SysDefault;
		    desc.Behind = desc_tmp.Behind;
		    desc.AutoClose = desc_tmp.AutoClose;
		    desc.CloseGadget = desc_tmp.CloseGadget;
		    desc.DummyWasForeign = desc_tmp.DummyWasForeign;
		    CopyMem(desc_tmp.SystemPens, desc.SystemPens, sizeof(desc_tmp.SystemPens));
		    CopyMem(desc_tmp.Reserved, desc.Reserved, sizeof(desc_tmp.Reserved));
		    ARRAY_TO_COLS(desc_tmp.Palette, desc.Palette);
		    ARRAY_TO_COLS(desc_tmp.rsvd, desc.rsvd);
		    CopyMem(desc_tmp.rsvd2, desc.rsvd2, sizeof(desc_tmp.rsvd2));
		    desc.Changed = ARRAY_TO_LONG(desc_tmp.Changed);
		    
		    retval = MUIS_AllocPubScreenDesc(&desc);
		}
	    }
	}
    }

    return retval;

    AROS_LIBFUNC_EXIT
}
