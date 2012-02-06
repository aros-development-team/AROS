/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "title_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

/* Note: Code taken from OWB's tabs handling */

#define MUIB_Tabs                   (TAG_USER | 0x10000000)
#define MUIV_Tabs_Top               (MUIB_Tabs | 0x00000000)
#define MUIV_Tabs_Left              (MUIB_Tabs | 0x00000001)

#define _isinobj(x,y,obj) (_between(_left(obj),(x),_right (obj)) \
                          && _between(_top(obj) ,(y),_bottom(obj)))


ULONG Tabs_Layout_Function(struct Hook *hook, Object *obj, struct MUI_LayoutMsg *lm)
{
    struct Title_DATA *data = (struct Title_DATA *) hook->h_Data;
    switch (lm->lm_Type)
    {
        case MUILM_MINMAX:
        {
            Object *cstate = (Object *)lm->lm_Children->mlh_Head;
            Object *child;

            WORD maxminwidth  = 0;
            WORD maxminheight = 0;
            WORD mintotalwidth = 0;
            WORD mintotalheight = 0;
            LONG number_of_children = 0;

            /* find out biggest widths & heights of our children */

            while((child = NextObject(&cstate)))
            {
                if(maxminwidth < MUI_MAXMAX && _minwidth(child) > maxminwidth)
                    maxminwidth  = _minwidth(child);

                if(maxminheight < MUI_MAXMAX && _minheight(child) > maxminheight)
                    maxminheight = _minheight(child);

                mintotalheight += _minheight(child);

                number_of_children++;
            }

            if(data->location == MUIV_Tabs_Top)
            {
                mintotalwidth = number_of_children * maxminwidth + (number_of_children - 1) * XGET(obj, MUIA_Group_HorizSpacing);
                lm->lm_MinMax.MinWidth = lm->lm_MinMax.DefWidth = mintotalwidth;
                lm->lm_MinMax.MinHeight = lm->lm_MinMax.DefHeight = maxminheight + data->protrusion;
                lm->lm_MinMax.MaxWidth  = MUI_MAXMAX;
                lm->lm_MinMax.MaxHeight = lm->lm_MinMax.DefHeight = maxminheight + data->protrusion;
            }
            else if(data->location == MUIV_Tabs_Left)
            {
                mintotalheight += (number_of_children - 1) * XGET(obj, MUIA_Group_VertSpacing);
                lm->lm_MinMax.MinWidth = lm->lm_MinMax.DefWidth = maxminwidth + data->protrusion;
                lm->lm_MinMax.MinHeight = lm->lm_MinMax.DefHeight = mintotalheight + data->protrusion;
                lm->lm_MinMax.MaxWidth  = lm->lm_MinMax.DefWidth = maxminwidth + data->protrusion;
                lm->lm_MinMax.MaxHeight = MUI_MAXMAX;
            }

            return 0;
        }
        case MUILM_LAYOUT:
        {
            APTR cstate;
            Object *child;
            LONG number_of_children = 0;

            cstate = lm->lm_Children->mlh_Head;
            while((child = NextObject(&cstate)))
            {
                number_of_children++;
            }

            if(data->location == MUIV_Tabs_Top)
            {
                WORD horiz_spacing = XGET(obj, MUIA_Group_HorizSpacing);
                WORD childwidth = (lm->lm_Layout.Width - (number_of_children - 1) * horiz_spacing) / number_of_children;
                WORD leftovers = lm->lm_Layout.Width - (number_of_children - 1) * horiz_spacing - number_of_children * childwidth;
                WORD left = 0;
                LONG tab = 0;
                cstate = lm->lm_Children->mlh_Head;
                while((child = NextObject(&cstate)))
                {
                    WORD cwidth = childwidth;
                    WORD cheight = _height(obj);
                    if(leftovers-- > 0)
                        cwidth++;
                    if(tab != data->activetab)
                        cheight -= data->protrusion;
                    if(!MUI_Layout(child, left, lm->lm_Layout.Height - cheight, cwidth, cheight, 0))
                        return(FALSE);

                    left += cwidth + horiz_spacing;
                    tab++;
                }
            }
            else if(data->location == MUIV_Tabs_Left)
            {
                WORD vert_spacing = XGET(obj, MUIA_Group_VertSpacing);
                WORD top = 0;
                LONG tab = 0;
                cstate = lm->lm_Children->mlh_Head;
                while((child = NextObject(&cstate)))
                {
                    WORD cheight = _minheight(child);
                    if(tab != data->activetab)
                        cheight += data->protrusion;

                    if(!MUI_Layout(child, 0, top, lm->lm_Layout.Width, cheight, 0))
                        return(FALSE);

                    top += cheight + vert_spacing;
                }
            }
            else
                return FALSE;

            return TRUE;
        }
    }

    return TRUE;
}

IPTR Title__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Title_DATA *data = NULL;
    struct Hook *layout_hook;

    /* Lay tabs horizontally by default */
    LONG location = MUIV_Tabs_Top;

    layout_hook = AllocVec(sizeof(struct Hook), MEMF_ANY | MEMF_CLEAR);
    if (!layout_hook) return (IPTR) NULL;

    layout_hook->h_Entry = HookEntry;
    layout_hook->h_SubEntry = (HOOKFUNC)Tabs_Layout_Function;

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_LayoutHook, (IPTR) layout_hook,
        MUIA_ShowSelState, FALSE,
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    layout_hook->h_Data = data;
    data->layout_hook = layout_hook;
    data->location = location;
    data->protrusion = 4;
    data->activetab = -1;

    /* We need tab events to be processed after all objects contained in tabs
       like for example close button, hence the low priority value */
    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = -7;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    D(bug("muimaster.library/title.c: Title Object created at 0x%lx\n",obj));

    return (IPTR)obj;
}

IPTR Title__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);
    FreeVec(data->layout_hook);

    return DoSuperMethodA(cl, obj, msg);
}

/* MUIM_Draw helpers */

/* Similar like in Register class */
static void DrawTopTab(Object *obj, struct Title_DATA *data, BOOL active, WORD x1, WORD y1, WORD x2, WORD y2)
{
    if (!active)
    {
        /* Clear the rounded edges of an unactive tab with default background */
        nnset(obj, MUIA_Background, data->background);

        DoMethod(obj,MUIM_DrawBackground, (IPTR) x1, (IPTR) y1,
                 (IPTR) 1, (IPTR) 3,
                 (IPTR) x1, (IPTR) y1, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x1 + 1, (IPTR) y1,
                 (IPTR) 1, (IPTR) 2,
                 (IPTR) x1 + 1, (IPTR) y1, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x1 + 2, (IPTR) y1,
                 (IPTR) 1, (IPTR) 1,
                 (IPTR) x1 + 2, (IPTR) y1, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x2 - 3, (IPTR) y1,
                 (IPTR) 3, (IPTR) 1,
                 (IPTR) x2 - 3, (IPTR) y1, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x2 - 1, (IPTR) y1,
                 (IPTR) 1, (IPTR) 2,
                 (IPTR) x2 - 1, (IPTR) y1, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x2, (IPTR) y1,
                 (IPTR) 1, (IPTR) 3,
                 (IPTR) x2, (IPTR) y1, (IPTR) 0);

        nnset(obj, MUIA_Background, MUII_BACKGROUND);
    }

    /* top horiz bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    RectFill(_rp(obj), x1 + 4, y1, x2 - 4, y1);
    /* left vert bar */
    RectFill(_rp(obj), x1, y1 + 4, x1, y2 - (active ? 2 : 1));
    WritePixel(_rp(obj), x1 + 1, y1 + 3);
    WritePixel(_rp(obj), x1 + 1, y1 + 2);
    WritePixel(_rp(obj), x1 + 2, y1 + 1);
    WritePixel(_rp(obj), x1 + 3, y1 + 1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHINE]);
    WritePixel(_rp(obj), x1 + 3, y1);
    WritePixel(_rp(obj), x1 + 4, y1 + 1);
    WritePixel(_rp(obj), x1 + 2, y1 + 2);
    WritePixel(_rp(obj), x1 + 3, y1 + 2);
    WritePixel(_rp(obj), x1 + 2, y1 + 3);
    WritePixel(_rp(obj), x1, y1 + 3);
    WritePixel(_rp(obj), x1 + 1, y1 + 4);

    if (active)
    {
        /* bottom horiz bar */
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        WritePixel(_rp(obj), x1, y2 - 1);
        WritePixel(_rp(obj), x1, y2);

        SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
        WritePixel(_rp(obj), x2, y2 - 1);
        WritePixel(_rp(obj), x2, y2);
    }
    else
    {
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        RectFill(_rp(obj), x1, y2, x2, y2);
    }

    /* right vert bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    WritePixel(_rp(obj), x2 - 1, y1 + 2);
    RectFill(_rp(obj), x2, y1 + 4, x2, y2 - (active ? 2 : 1));
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHADOW]);
    WritePixel(_rp(obj), x2 - 2, y1 + 1);
    WritePixel(_rp(obj), x2 - 1, y1 + 3);
    WritePixel(_rp(obj), x2, y1 + 3);
    SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
    WritePixel(_rp(obj), x2 - 3, y1 + 1);
}

/* Like above, just symetrically flipped */
static void DrawLeftTab(Object *obj, struct Title_DATA *data, BOOL active, WORD x1, WORD y1, WORD x2, WORD y2)
{
    if (!active)
    {
        /* Clear the rounded edges of an unactive tab with default background */
        nnset(obj, MUIA_Background, data->background);

        DoMethod(obj,MUIM_DrawBackground, (IPTR) x1, (IPTR) y1,
                 (IPTR) 3, (IPTR) 1,
                 (IPTR) x1, (IPTR) y1, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x1, (IPTR) y1 + 1,
                 (IPTR) 2, (IPTR) 1,
                 (IPTR) x1, (IPTR) y1 + 1, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x1, (IPTR) y1 + 2,
                 (IPTR) 1, (IPTR) 1,
                 (IPTR) x1, (IPTR) y1 + 2, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x1, (IPTR) y2 - 2,
                 (IPTR) 1, (IPTR) 1,
                 (IPTR) x1, (IPTR) y2 - 2, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x1, (IPTR) y2 - 1,
                 (IPTR) 2, (IPTR) 1,
                 (IPTR) x1, (IPTR) y2 - 1, (IPTR) 0);
        DoMethod(obj,MUIM_DrawBackground, (IPTR) x1, (IPTR) y2,
                 (IPTR) 3, (IPTR) 1,
                 (IPTR) x1, (IPTR) y2, (IPTR) 0);

        nnset(obj, MUIA_Background, MUII_BACKGROUND);
    }

    /* left vert bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    RectFill(_rp(obj), x1, y1 + 4, x1, y2 - 4);
    /* top horiz bar */
    RectFill(_rp(obj), x1 + 4, y1, x2 - (active ? 2 : 1), y1);
    WritePixel(_rp(obj), x1 + 1, y1 + 3);
    WritePixel(_rp(obj), x1 + 1, y1 + 2);
    WritePixel(_rp(obj), x1 + 2, y1 + 1);
    WritePixel(_rp(obj), x1 + 3, y1 + 1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHINE]);
    WritePixel(_rp(obj), x1 + 3, y1);
    WritePixel(_rp(obj), x1 + 4, y1 + 1);
    WritePixel(_rp(obj), x1 + 2, y1 + 2);
    WritePixel(_rp(obj), x1 + 3, y1 + 2);
    WritePixel(_rp(obj), x1 + 2, y1 + 3);
    WritePixel(_rp(obj), x1, y1 + 3);
    WritePixel(_rp(obj), x1 + 1, y1 + 4);

    if (active)
    {
        /* bottom horiz bar */
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        WritePixel(_rp(obj), x2 - 1, y1);
        WritePixel(_rp(obj), x2, y1);

        SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
        WritePixel(_rp(obj), x2 - 1, y2);
        WritePixel(_rp(obj), x2, y2);
    }
    else
    {
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        RectFill(_rp(obj), x2, y1, x2, y2);
    }

    /* bottom horiz bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    WritePixel(_rp(obj), x1 + 2, y2 - 1);
    RectFill(_rp(obj), x1 + 4, y2, x2 - (active ? 2 : 1), y2);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHADOW]);
    WritePixel(_rp(obj), x1 + 1, y2 - 2);
    WritePixel(_rp(obj), x1 + 3, y2 - 1);
    WritePixel(_rp(obj), x1 + 3, y2);
    SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
    WritePixel(_rp(obj), x1 + 1, y2 - 3);
}

/* Drawing code */
IPTR Tab__MUIM_Draw(Object * child, struct Title_DATA * data, BOOL active)
{
    WORD x1 = _left(child) ;
    WORD y1 = _top(child);
    WORD x2 = _right(child);
    WORD y2 = _bottom(child);

//    if(data->nodraw)
//        return 1;
//
//    /* Zune does MUI_Redraw(obj, MADF_DRAWOBJECT) when setting new background,
//       we don't need it */
//    data->nodraw = 1;
//
    if (active)
    {
        nnset(child, MUIA_Background, data->background);
    }
    else
    {
        nnset(child, MUIA_Background, MUII_BACKGROUND);
    }

    /* Draw all child objects */
    MUI_Redraw(child, MADF_DRAWOBJECT);

//    DoSuperMethodA(cl, obj, (Msg) msg);

    /* Draw tab frame */
    if(data->location == MUIV_Tabs_Top)
        DrawTopTab(child, data, active, x1, y1, x2, y2);
    else if(data->location == MUIV_Tabs_Left)
        DrawLeftTab(child, data, active, x1, y1, x2, y2);

//    data->nodraw = 0;

    return TRUE;
}
/* MUIM_Draw helpers */

IPTR Title__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);
    struct List *children;
    APTR cstate;
    Object *child;
    WORD horiz_spacing = XGET(obj, MUIA_Group_HorizSpacing);
    WORD vert_spacing = XGET(obj, MUIA_Group_VertSpacing);
    LONG tab = 0;

    /* Draw all the children */
    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return(0);

    /* Now draw missing TabbedGroup border between the spaces */
    get(obj, MUIA_Group_ChildList, &children);

    cstate = children->lh_Head;

    while ((child = NextObject(&cstate)))
    {
        if (tab == data->activetab)
            Tab__MUIM_Draw(child, data, TRUE);
        else
            Tab__MUIM_Draw(child, data, FALSE);
        tab++;
    }

    cstate = children->lh_Head;
    child = NextObject(&cstate);

    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    if(data->location == MUIV_Tabs_Top)
    {
        while(child && (child = NextObject(&cstate)))
        {
            RectFill(_rp(obj), _left(child) - horiz_spacing, _bottom(child), _left(child) - 1, _bottom(child));
        }
    }
    else if(data->location == MUIV_Tabs_Left)
    {
        WORD lasty;
        while(child && (child = NextObject(&cstate)))
        {
            RectFill(_rp(obj), _right(child), _top(child) - vert_spacing, _right(child), _top(child) - 1);
            lasty = _bottom(child);
        }
        RectFill(_rp(obj), _right(obj), lasty + 1, _right(obj), _bottom(obj));
    }
    else
        return FALSE;

    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    WritePixel(_rp(obj), _right(obj), _bottom(obj));

    return TRUE;
}

IPTR Title__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);

    if(!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    get(_parent(obj), MUIA_Background, &data->background);

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    return TRUE;
}

IPTR Title__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Title__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);

    if (!msg->imsg)
        return 0;

    switch (msg->imsg->Class)
    {
        case IDCMP_MOUSEBUTTONS:
            if (msg->imsg->Code == SELECTDOWN && _isinobj(msg->imsg->MouseX, msg->imsg->MouseY,obj))
            {
                struct List *children = (struct List*) XGET(obj, MUIA_Group_ChildList);
                APTR cstate = children->lh_Head;
                Object *child;
                int i;

                /* Find previous and next tab */
                for(i = 0; (child = NextObject(&cstate)); i++)
                {
                    if(_isinobj(msg->imsg->MouseX, msg->imsg->MouseY,child))
                    {
                        /* Activate this tab */
                        data->activetab = i;
                        MUI_Redraw(obj, MADF_DRAWUPDATE);
                        break;
                    }
                }
            }
            break;
    }
    return 0;
}

#if ZUNE_BUILTIN_TITLE
BOOPSI_DISPATCHER(IPTR, Title_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:        return Title__OM_NEW(cl, obj, (struct opSet *)msg);
        case OM_DISPOSE:    return Title_OM_DISPOSE(cl, obj, msg);
        case MUIM_Draw:     return Title__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
        case MUIM_Setup:    return Title__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
        case MUIM_Cleanup:  return Title__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
        case MUIM_HandleEvent: return Title__MUIM_HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
        default: return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Title_desc =
{
    MUIC_Title,
    MUIC_Group,
    sizeof(struct Title_DATA),
    (void*)Title_Dispatcher
};
#endif /* ZUNE_BUILTIN_TITLE */
