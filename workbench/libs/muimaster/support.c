/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <string.h>

#include <intuition/classes.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/utility.h>

#include "mui.h"
#include "support.h"
#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

/**************************************************************************
 check if region is entirely within given bounds
**************************************************************************/
int isRegionWithinBounds(struct Region *r, int left, int top, int width, int height)
{
	if ((left <= r->bounds.MinX) && (left + width  - 1 >= r->bounds.MaxX)
	 && (top  <= r->bounds.MinY) && (top  + height - 1 >= r->bounds.MaxY))
		return 1;

	return 0;
}


/**************************************************************************
 Converts a Rawkey to a vanillakey
**************************************************************************/
ULONG ConvertKey(struct IntuiMessage *imsg)
{
   struct InputEvent event;
   UBYTE code = 0;
   event.ie_NextEvent    = NULL;
   event.ie_Class        = IECLASS_RAWKEY;
   event.ie_SubClass     = 0;
   event.ie_Code         = imsg->Code;
   event.ie_Qualifier    = imsg->Qualifier;
   event.ie_EventAddress = (APTR *) *((ULONG *)imsg->IAddress);
   MapRawKey(&event, &code, 1, NULL);
   return code;
}

/**************************************************************************
 Convient way to get an attribute of an object easily. If the object
 doesn't support the attribute this call returns an undefined value. So use
 this call only if the attribute is known to be known by the object. 
 Implemented as a macro when compiling with GCC.
**************************************************************************/
#ifndef __GNUC__
IPTR XGET(Object *obj, Tag attr)
{
  IPTR storage = 0;
  GetAttr(attr, obj, &storage);
  return storage;
}
#endif /* __GNUC__ */

/**************************************************************************
 Call the Setup Method of an given object, but before set the renderinfo
**************************************************************************/
IPTR DoSetupMethod(Object *obj, struct MUI_RenderInfo *info)
{
    /* MUI set the correct render info *before* it calls MUIM_Setup so please only use this function instead of DoMethodA() */
    muiRenderInfo(obj) = info;
    return DoMethod(obj, MUIM_Setup, (IPTR)info);
}

IPTR DoShowMethod(Object *obj)
{
    IPTR ret;

    ret = DoMethod(obj, MUIM_Show);
    if (ret)
	_flags(obj) |= MADF_CANDRAW;
    return ret;
}

IPTR DoHideMethod(Object *obj)
{
    _flags(obj) &= ~MADF_CANDRAW;
    return DoMethod(obj, MUIM_Hide);
}


void *Node_Next(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ->mln_Succ == NULL)
		return NULL;
    return ((struct MinNode*)node)->mln_Succ;
}

void *List_First(APTR list)
{
    if( !((struct MinList*)list)->mlh_Head) return NULL;
    if(((struct MinList*)list)->mlh_Head->mln_Succ == NULL) return NULL;
    return ((struct MinList*)list)->mlh_Head;
}

/* subtract rectangle b from rectangle b. resulting rectangles will be put into
   destrectarray which must have place for at least 4 rectangles. Returns number
   of resulting rectangles */
   
WORD SubtractRectFromRect(struct Rectangle *a, struct Rectangle *b, struct Rectangle *destrectarray)
{
    struct Rectangle    intersect;
    BOOL            	intersecting = FALSE;
    WORD            	numrects = 0;

    /* calc. intersection between a and b */

    if (a->MinX <= b->MaxX)
    {
        if (a->MinY <= b->MaxY)
        {
            if (a->MaxX >= b->MinX)
            {
                if (a->MaxY >= b->MinY)
                {
                    intersect.MinX = MAX(a->MinX, b->MinX);
                    intersect.MinY = MAX(a->MinY, b->MinY);
                    intersect.MaxX = MIN(a->MaxX, b->MaxX);
                    intersect.MaxY = MIN(a->MaxY, b->MaxY);

                    intersecting = TRUE;
                }
            }
        }
    }

    if (!intersecting)
    {
        destrectarray[numrects++] = *a;

    } /* not intersecting */
    else
    {
        if (intersect.MinY > a->MinY) /* upper */
        {
            destrectarray->MinX = a->MinX;
            destrectarray->MinY = a->MinY;
            destrectarray->MaxX = a->MaxX;
            destrectarray->MaxY = intersect.MinY - 1;

            numrects++;
            destrectarray++;
        }

        if (intersect.MaxY < a->MaxY) /* lower */
        {
            destrectarray->MinX = a->MinX;
            destrectarray->MinY = intersect.MaxY + 1;
            destrectarray->MaxX = a->MaxX;
            destrectarray->MaxY = a->MaxY;

            numrects++;
            destrectarray++;
        }

        if (intersect.MinX > a->MinX) /* left */
        {
            destrectarray->MinX = a->MinX;
            destrectarray->MinY = intersect.MinY;
            destrectarray->MaxX = intersect.MinX - 1;
            destrectarray->MaxY = intersect.MaxY;

            numrects++;
            destrectarray++;
        }

        if (intersect.MaxX < a->MaxX) /* right */
        {
            destrectarray->MinX = intersect.MaxX + 1;
            destrectarray->MinY = intersect.MinY;
            destrectarray->MaxX = a->MaxX;
            destrectarray->MaxY = intersect.MaxY;

            numrects++;
            destrectarray++;
        }

    } /* intersecting */

    return numrects;

}
