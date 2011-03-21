/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <intuition/iprefs.h>
#include <intuition/pointerclass.h>
#include <prefs/palette.h>

#include <proto/intuition.h>
#include <proto/graphics.h>

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
    AROS_LH3(ULONG, SetIPrefs,

/*  SYNOPSIS */
         AROS_LHA(APTR , data, A0),
         AROS_LHA(ULONG, length, D0),
         AROS_LHA(ULONG, type, D1),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 96, Intuition)

/*  FUNCTION

    INPUTS

    RESULT
    Depending on the operation

    NOTES
        This function is currently considered private

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG Result = TRUE;
    ULONG lock = LockIBase(0);

    DEBUG_SETIPREFS(bug("SetIPrefs: data %p length %lu type %lu\n", data, length, type));

    switch (type)
    {
	case IPREFS_TYPE_ICONTROL_V37:
            DEBUG_SETIPREFS(bug("SetIPrefs: IP_ICONTROL_V37\n"));
            if (length > sizeof(struct IIControlPrefs))
        	length = sizeof(struct IIControlPrefs);
            CopyMem(data, &GetPrivIBase(IntuitionBase)->IControlPrefs, length);

	    DEBUG_SETIPREFS(bug("SetIPrefs: Drag modes: 0x%04lX\n", GetPrivIBase(IntuitionBase)->IControlPrefs.ic_VDragModes[0]));

            break;
        
	case IPREFS_TYPE_SCREENMODE_V37:
	{
	    struct IScreenModePrefs old_prefs;
	    
            DEBUG_SETIPREFS(bug("SetIPrefs: IP_SCREENMODE_V37\n"));
            if (length > sizeof(struct IScreenModePrefs))
                length = sizeof(struct IScreenModePrefs);
	    
	    if (memcmp(&GetPrivIBase(IntuitionBase)->ScreenModePrefs, data,
	               sizeof(struct IScreenModePrefs)) == 0)
	        break;
	    
	    old_prefs = GetPrivIBase(IntuitionBase)->ScreenModePrefs;
	    GetPrivIBase(IntuitionBase)->ScreenModePrefs = *(struct IScreenModePrefs *)data;

	    if (GetPrivIBase(IntuitionBase)->WorkBench)
	    {
	        BOOL try = TRUE, closed;
		
	        UnlockIBase(lock);
		
		while (try && !(closed = CloseWorkBench()))
		{
                    struct EasyStruct es =
                    {
                        sizeof(struct EasyStruct),
                        0,
                        "System Request",
                        "Intuition is attempting to reset the screen,\n"
			"please close all windows except Wanderer's ones.",
                        "Retry|Cancel"
                    };

                    try = EasyRequestArgs(NULL, &es, NULL, NULL) == 1;
		}
		
		if (closed)
		    /* FIXME: handle the error condition if OpenWorkBench() fails */
		    /* What to do if OpenWorkBench() fails? Try until it succeeds?
		       Try for a finite amount of times? Don't try and do nothing 
		       at all? */
		    OpenWorkBench();
		else
		{
		    lock = LockIBase(0);
                    GetPrivIBase(IntuitionBase)->ScreenModePrefs = old_prefs;
		    UnlockIBase(lock);
		    Result = FALSE;
		}
		
		return Result;
		
	    }
	    
            break;
	}

	case IPREFS_TYPE_POINTER_V39:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_POINTER_V39\n"));
        {
            struct IPointerPrefs *fp = data;
            struct TagItem pointertags[] = {
                {POINTERA_BitMap , (IPTR)fp->BitMap},
                {POINTERA_XOffset, fp->XOffset     },
                {POINTERA_YOffset, fp->YOffset     },
                {TAG_DONE        , 0               }
            };

            Object *pointer = NewObjectA(
                          GetPrivIBase(IntuitionBase)->pointerclass,
                          NULL,
                          pointertags);

            Object **oldptr = fp->Which ?
                      &GetPrivIBase(IntuitionBase)->BusyPointer :
                      &GetPrivIBase(IntuitionBase)->DefaultPointer;

            InstallPointer(IntuitionBase, fp->Which, oldptr, pointer);
	    /* return -1 so that WB3.x C:IPrefs is happy */
            Result = -1;
        }
        break;

	case IPREFS_TYPE_POINTER_V37:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_POINTER_V37\n"));
        {
 	    /* return -1 so that WB2.x C:IPrefs is happy */
           Result = -1;
        }
        break;

        case IPREFS_TYPE_PALETTE_V39:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_PALETTE_V39 %p %d\n", data, length));
        {
            struct ColorSpec *pp = data;
            struct Color32 *p = GetPrivIBase(IntuitionBase)->Colors;
	    BOOL update_pointer = FALSE;
	    struct Preferences *ActivePrefs = GetPrivIBase(IntuitionBase)->ActivePreferences;

            DEBUG_SETIPREFS(bug("SetIPrefs: Intuition Color32 Table 0x%p\n", p));

            while (pp->ColorIndex != -1)
            {
                DEBUG_SETIPREFS(bug("SetIPrefs: Index %ld Red 0x%04lX Green 0x%04lX Blue 0x%04lX\n",
                                    pp->ColorIndex, pp->Red, pp->Green, pp->Blue));
                if (pp->ColorIndex < COLORTABLEENTRIES)
                {
                    p[pp->ColorIndex].red   = (pp->Red<<16)|pp->Red;
                    p[pp->ColorIndex].green = (pp->Green<<16)|pp->Green;
                    p[pp->ColorIndex].blue  = (pp->Blue<<16)|pp->Blue;

                    /* Update oldstyle preferences */
                    if (ActivePrefs)
                    {
		        UWORD *cols = NULL;
			UWORD baseindex;
			
			if (pp->ColorIndex < 4) {
			    baseindex = 0;
			    cols = &ActivePrefs->color0;
                        } else if (pp->ColorIndex >= 8 && pp->ColorIndex <= 10) {
			    baseindex = 8;
                            cols=&ActivePrefs->color17;
			    update_pointer = TRUE;
                        }
			
			if (cols)
			    cols[pp->ColorIndex - baseindex] = ((pp->Red >> 4) & 0xf00) | ((pp->Green >> 8) & 0x0f0) | (pp->Blue >> 12);
                    }
                    DEBUG_SETIPREFS(bug("SetIPrefs: Set Color32 %ld Red 0x%lx Green 0x%lx Blue 0x%lx\n",
                                (LONG) pp->ColorIndex,
                                p[pp->ColorIndex].red,
                                p[pp->ColorIndex].green,
                                p[pp->ColorIndex].blue));
                }
                pp++;
            }
	    
	    if (update_pointer) {
	        DEBUG_SETIPREFS(bug("[SetIPrefs] Updating pointer colors\n"));
	        SetPointerColors(IntuitionBase);
	    }
        }
        break;

	case IPREFS_TYPE_PALETTE_V37:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_PALETTE_V37\n"));
        {
        }
        break;
	
	case IPREFS_TYPE_PENS_V39:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_PENS_V39\n"));
        {
            struct IOldPenPrefs *fp = data;
            UWORD *dataptr;
            int i;
            DEBUG_SETIPREFS(bug("SetIPrefs: Count %ld Type %ld\n",
                        (LONG) fp->Count,
                        (LONG) fp->Type));

            if (fp->Type==0)
            {
                dataptr = &GetPrivIBase(IntuitionBase)->DriPens4[0];
                DEBUG_SETIPREFS(bug("SetIPrefs: Pens4[]\n"));
            }
            else
            {
                dataptr = &GetPrivIBase(IntuitionBase)->DriPens8[0];
                DEBUG_SETIPREFS(bug("SetIPrefs: Pens8[]\n"));
            }
            for (i=0;i<NUMDRIPENS;i++)
            {
                if (fp->PenTable[i]==(UWORD)~0UL)
                {
                    /*
                     * end of the array
                     */
                    DEBUG_SETIPREFS(bug("SetIPrefs: PenTable end at entry %ld\n", (LONG) i));
                    break;
                }
                else
                {
                    DEBUG_SETIPREFS(bug("SetIPrefs: Pens[%ld] %ld\n",
                                (LONG) i,
                                (LONG) fp->PenTable[i]));
                    dataptr[i] = fp->PenTable[i];
                }
            }
        }
        break;


	case IPREFS_TYPE_POINTER_ALPHA:
	    DEBUG_SETIPREFS(bug("[SetIPrefs]: IP_POINTER_ALPHA\n"));
	    GetPrivIBase(IntuitionBase)->PointerAlpha = *(UWORD *)data;
	break;

	case IPREFS_TYPE_OVERSCAN_V37:
	    DEBUG_SETIPREFS(bug("[SetIPrefs]: IP_OVERSCAN_V37\n"));
	break;

	case IPREFS_TYPE_FONT_V37:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_FONT_V37\n"));
        {
            struct IFontPrefs *fp = data;
            struct TextFont *font = OpenFont(&fp->fp_TextAttr);
            struct TextFont **fontptr;

            DEBUG_SETIPREFS(bug("SetIPrefs: Type %d Name <%s> Size %d Font %p\n", fp->fp_ScrFont, fp->fp_Name, fp->fp_TextAttr.ta_YSize, font));

            if (font)
            {
                if (fp->fp_ScrFont==0)
                {
                    /*
                     * We can't free graphics defaultfont..it`s shared
                     */
                    fontptr = &GfxBase->DefaultFont;
                }
                else
                {
                    fontptr = &GetPrivIBase(IntuitionBase)->ScreenFont;
                    CloseFont(*fontptr);
                }
                *fontptr = font;
            }
        }
        break;

	default:
            DEBUG_SETIPREFS(bug("SetIPrefs: Unknown Prefs Type\n"));
            Result = FALSE;
            break;
    }

    UnlockIBase(lock);

    DEBUG_SETIPREFS(bug("SetIPrefs: Result 0x%lx\n",Result));
    
    return(Result);
    
    AROS_LIBFUNC_EXIT
} /* private1 */
