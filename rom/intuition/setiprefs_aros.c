/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <intuition/iprefs.h>
#include <intuition/pointerclass.h>

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

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
	case IPREFS_TYPE_ICONTROL:
            DEBUG_SETIPREFS(bug("SetIPrefs: IPREFS_TYPE_ICONTROL\n"));
            if (length > sizeof(struct IIControlPrefs))
        	length = sizeof(struct IIControlPrefs);
            CopyMem(data, &GetPrivIBase(IntuitionBase)->IControlPrefs, length);
            break;
        
	case IPREFS_TYPE_SCREENMODE:
	{
	    struct IScreenModePrefs old_prefs;
	    
            DEBUG_SETIPREFS(bug("SetIPrefs: IP_SCREENMODE\n"));
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
		    #warning FIXME: handle the error condition!
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

	case IPREFS_TYPE_POINTER:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_POINTER\n"));
        {
            struct IPointerPrefs *fp = data;
            struct TagItem pointertags[] =
                {
                    {
                        POINTERA_BitMap    , (ULONG) fp->BitMap
                    },
                    {POINTERA_XOffset   , fp->XOffset   },
                    {POINTERA_YOffset   , fp->YOffset   },
                    {TAG_DONE               }
                };

            Object *pointer = NewObjectA(
                          GetPrivIBase(IntuitionBase)->pointerclass,
                          NULL,
                          pointertags);

            Object **oldptr = fp->Which ?
                      &GetPrivIBase(IntuitionBase)->BusyPointer :
                      &GetPrivIBase(IntuitionBase)->DefaultPointer;

            InstallPointer(IntuitionBase, oldptr, pointer);
        }
        break;

        case IPREFS_TYPE_OLD_PALETTE:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_OLDPALETTE\n"));
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
	
	case IPREFS_TYPE_POINTER_ALPHA:
	    GetPrivIBase(IntuitionBase)->PointerAlpha = *(UWORD *)data;
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
