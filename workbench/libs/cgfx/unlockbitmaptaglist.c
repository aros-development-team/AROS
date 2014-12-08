/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include <cybergraphx/cybergraphics.h>
#include <hidd/graphics.h>

#include "cybergraphics_intern.h"


struct RectList
{
    ULONG rl_num;             // no. of rects in this list
    struct RectList *rl_next; // pointer to next list
    struct Rectangle rect1;   // First Rectangle in the list
};

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH2(void, UnLockBitMapTagList,

/*  SYNOPSIS */
	AROS_LHA(APTR            , Handle, A0),
	AROS_LHA(struct TagItem *, Tags, A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 30, Cybergraphics)

/*  FUNCTION
        Releases exclusive access to a bitmap. Options for the unlocking
        process are given in a taglist. The possible tags are as follows:
            UBMI_UPDATERECTS (struct RectList *) - pointer to a series of
                rectangle lists that need to be refreshed.
            UBMI_REALLYUNLOCK (BOOL) - if FALSE, the bitmap will not be
                unlocked; only rectangle updates will be done.

    INPUTS
        Handle - handle to the bitmap to unlock.
        Tags - a taglist as described above.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        UnLockBitMap(), LockBitMapTagList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag;
    BOOL reallyunlock = TRUE;
    struct RectList *rl = NULL;
    struct BitMap *bm = (struct BitMap *)Handle;

    D(bug("[Cybergfx] %s(0x%p, 0x%p)\n", __PRETTY_FUNCTION__, bm, Tags));

    if ((bm) && (IS_HIDD_BM(bm)))
    {
        while ((tag = NextTagItem(&Tags)))
        {
            switch (tag->ti_Tag)
            {
                case UBMI_REALLYUNLOCK:
                    reallyunlock = (BOOL)tag->ti_Data;
                    break;

                case UBMI_UPDATERECTS:
                {
                    rl = (struct RectList *)tag->ti_Data;
                    break;
                }

                default:
                    D(bug("[Cybergfx] %s: !!! UNKNOWN TAG %08x !!!\n", __PRETTY_FUNCTION__, tag->ti_Tag));
                    break;
            }
        }

        if (reallyunlock)
        {
            D(bug("[Cybergfx] %s: Calling HIDD_BM_ReleaseDirectAccess...\n", __PRETTY_FUNCTION__));
            HIDD_BM_ReleaseDirectAccess(HIDD_BM_OBJ(bm));
        }

        if (rl)
        {
            while (rl)
            {
                struct Rectangle *rectCurrent;
                int count;

                D(bug("[Cybergfx] %s: RectList @ 0x%p\n", __PRETTY_FUNCTION__, rl));

                if ((count = rl->rl_num) > 0)
                {
                    D(bug("[Cybergfx] %s: %d entries\n", __PRETTY_FUNCTION__, rl->rl_num));

                    for (rectCurrent = &rl->rect1; count > 0; count--)
                    {
                        D(bug("[Cybergfx] %s: Updating BitMap Rect [%d, %d -> %d, %d]\n", __PRETTY_FUNCTION__, rectCurrent->MinX, rectCurrent->MinY, rectCurrent->MaxX, rectCurrent->MaxY));
                        UpdateBitMap(bm, rectCurrent->MinX, rectCurrent->MinY, rectCurrent->MaxX - rectCurrent->MinX + 1, rectCurrent->MaxY - rectCurrent->MinY + 1);
                        rectCurrent = &rectCurrent[1];
                    }
                }
                rl = rl->rl_next;
            }
        }
        else
        {
            D(bug("[Cybergfx] %s: Updating full bitmap\n", __PRETTY_FUNCTION__));
            UpdateBitMap(bm, 0, 0, GetCyberMapAttr(bm, CYBRMATTR_WIDTH), GetCyberMapAttr(bm, CYBRMATTR_HEIGHT));
        }
    }
    else
    {
        D(bug("[Cybergfx] %s: Called on Illegal BitMap @ 0x%p\n", __PRETTY_FUNCTION__, bm));
    }

    AROS_LIBFUNC_EXIT
} /* UnLockBitMapTagList */
