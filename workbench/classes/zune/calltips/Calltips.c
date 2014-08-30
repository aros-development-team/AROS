/*
    Copyright © 2014, Thore Böckelmann. All rights reserved.
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <libraries/mui.h>

#include "Calltips_mcc.h"
#include "Calltips_private.h"

#define VERSION  20
#define REVISION 12

static void setRectangle(struct IClass *cl, Object *obj)
{
    struct Data *data = INST_DATA(cl, obj);
    struct Window *parentWin = _window(data->source);
    LONG left = parentWin->LeftEdge + data->rectangle.MinX - ((data->marginLeft == TRUE) ? _mleft(data->rootGroup) : 0);
    LONG top = parentWin->TopEdge + data->rectangle.MinY - ((data->marginLeft == TRUE) ? _mtop(data->rootGroup) : 0);
    LONG width = data->rectangle.MaxX - data->rectangle.MinX + 1;
    LONG height = data->rectangle.MaxY - data->rectangle.MinY + 1;

    // setting the position/dimension depends on the window's open state
    if(XGET(obj, MUIA_Window_Open) == TRUE)
    {
        struct Window *thisWin = _window(data->rootGroup);

        if(left != thisWin->LeftEdge || top != thisWin->TopEdge || width != thisWin->Width || height != thisWin->Height)
        {
            ChangeWindowBox(thisWin, left, top, width, height);
            MoveWindowInFrontOf(thisWin, parentWin);
        }
    }
    else
    {
        SetSuperAttrs(cl, obj,
            MUIA_Window_LeftEdge, left,
            MUIA_Window_TopEdge, top,
            MUIA_Window_Width, width,
            MUIA_Window_Height, height,
            TAG_DONE);
    }
}

/* ------------------------------------------------------------------------- */

IPTR Calltips__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    Object *source = NULL;
    struct Rect32 *rectangle = NULL;
    ULONG layout = MUIV_Calltips_Layout_Exact;
    BOOL marginLeft = TRUE;
    BOOL marginTop = TRUE;
    struct TagItem  *tstate = msg->ops_AttrList;
    struct TagItem  *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch(tag->ti_Tag)
        {
            case MUIA_Calltips_Source:
            {
                source = (Object *)tag->ti_Data;
            }
            break;

            case MUIA_Calltips_Rectangle:
            {
                rectangle = (struct Rect32 *)tag->ti_Data;
            }
            break;

            case MUIA_Calltips_Layout:
            {
                layout = tag->ti_Data;
            }
            break;

            case MUIA_Calltips_MarginLeft:
            {
                marginLeft = tag->ti_Data ? TRUE : FALSE;
            }
            break;

            case MUIA_Calltips_MarginTop:
            {
                marginTop = tag->ti_Data ? TRUE : FALSE;
            }
            break;
        }
    }

    if(source != NULL)
    {
        if((obj = (Object *)DoSuperNewTags(cl, obj, NULL,
            MUIA_Window_Screen, _screen(source),
            MUIA_Window_Borderless, TRUE,
            MUIA_Window_SizeGadget, FALSE,
            MUIA_Window_DragBar, FALSE,
            MUIA_Window_DepthGadget, FALSE,
            MUIA_Window_CloseGadget, FALSE,
            MUIA_Window_NoMenus, TRUE,
//            MUIA_Window_IsPopup, TRUE,
            MUIA_Window_Activate, FALSE,
//            MUIA_Window_StayTop, TRUE,
            TAG_MORE, msg->ops_AttrList)) != NULL)
        {
            struct Data *data = INST_DATA(cl, obj);

            data->source = source;
            data->rootGroup = (Object *)XGET(obj, MUIA_Window_RootObject);
            data->layout = layout;
            data->marginLeft = marginLeft;
            data->marginTop = marginTop;
            if(rectangle != NULL)
            {
                data->rectangle = *rectangle;
            }
            else
            {
                data->rectangle.MinX = _mleft(source);
                data->rectangle.MinY = _mbottom(source)+1;
                data->rectangle.MaxX = _mright(source);
                data->rectangle.MaxY = _mbottom(source)+1+100;
            }

            setRectangle(cl, obj);
        }
    }
    else
    {
        obj = NULL;
    }

    return (IPTR)obj;
}


IPTR Calltips__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    IPTR *store = msg->opg_Storage;

    switch(msg->opg_AttrID)
    {
        case MUIA_Version:        *store = VERSION; return TRUE;
        case MUIA_Revision:       *store = REVISION; return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


IPTR Calltips__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Data     *data = INST_DATA(cl, obj);
    struct TagItem  *tstate = msg->ops_AttrList;
    struct TagItem  *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch(tag->ti_Tag)
        {
            case MUIA_Calltips_Rectangle:
            {
                struct Rect32 *rectangle = (struct Rect32 *)tag->ti_Data;

                if(rectangle != NULL)
                {
                    data->rectangle = *rectangle;
                    setRectangle(cl, obj);
                }
            }
            break;

            case MUIA_Calltips_Layout:
            {
                data->layout = tag->ti_Data;
            }
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


IPTR Calltips__MUIM_Calltips_SetRectangle(struct IClass *cl, Object *obj, struct MUIP_Calltips_SetRectangle *msg)
{
    struct Data *data = INST_DATA(cl, obj);

    data->rectangle.MinX = msg->MinX;
    data->rectangle.MinY = msg->MinY;
    data->rectangle.MaxX = msg->MaxX;
    data->rectangle.MaxY = msg->MaxY;
    setRectangle(cl, obj);

    return 0;
}


IPTR Calltips__MUIM_Calltips_ParentSetup( struct IClass *cl, Object *obj, Msg msg)
{
    return 0;
}


IPTR Calltips__MUIM_Calltips_ParentCleanup(struct IClass *cl, Object *obj, Msg msg)
{
    return 0;
}


IPTR Calltips__MUIM_Calltips_ParentShow(struct IClass *cl, Object *obj, Msg msg)
{
    struct Data *data = INST_DATA(cl, obj);

    setRectangle(cl, obj);

    // open our window if it was not opened before
    if(XGET(obj, MUIA_Window_Open) == FALSE)
    {
        set(obj, MUIA_Window_Open, TRUE);
        // check again if we opened successfully
        if(XGET(obj, MUIA_Window_Open) == TRUE)
        {
            // make sure we are in front of our parent window
            MoveWindowInFrontOf(_window(data->rootGroup), _window(data->source));
        }
    }

    return 0;
}


IPTR Calltips__MUIM_Calltips_ParentHide(struct IClass *cl, Object *obj, Msg msg)
{
    set(obj, MUIA_Window_Open, FALSE);

    return 0;
}


IPTR Calltips__MUIM_Calltips_ParentWindowArranged(struct IClass *cl, Object *obj, Msg msg)
{
    setRectangle(cl, obj);

    return 0;
}
