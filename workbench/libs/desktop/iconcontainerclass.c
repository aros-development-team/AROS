/*
   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "desktop_intern.h"
#include "presentation.h"
#include "iconcontainerclass.h"
#include "iconclass.h"

#include "desktop_intern_protos.h"

#include <stdlib.h>


static ULONG DoSetupMethod(Object * obj, struct MUI_RenderInfo *info)
{
/*
   MUI set the correct render info *before* it calls MUIM_Setup so please
   only use this function instead of DoMethodA()
 */
    muiRenderInfo(obj) = info;

    return DoMethod(obj, MUIM_Setup, (IPTR) info);
}

IPTR iconConNew(Class * cl, Object * obj, struct opSet *ops)
{
    IPTR            retval = 0;
    struct IconContainerClassData *data;
    struct TagItem *tag,
                   *tstate = ops->ops_AttrList;
    Object         *vert = NULL,
        *horiz = NULL;
    BYTE            viewMode = ICAVM_LARGE;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case ICA_VertScroller:
                vert = (Object *) tag->ti_Data;
                break;

            case ICA_HorizScroller:
                horiz = (Object *) tag->ti_Data;
                break;

            case ICA_ViewMode:
                viewMode = tag->ti_Data;
                break;

            default:
                continue;       /* Don't supress non-processed tags */
        }

        tag->ti_Tag = TAG_IGNORE;
    }

    retval = DoSuperMethodA(cl, obj, (Msg) ops);

    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);

        data->perfectLayout = TRUE;
        data->thisColumnWidth = 0;
        data->thisColumnHeight = 0;
        data->virtualWidth = 0;
        data->virtualHeight = 0;
        data->xView = 0;
        data->yView = 0;
        data->visibleWidth = 0;
        data->visibleHeight = 0;
        data->heightAdjusted = 0;
        data->widthAdjusted = 0;
        data->horizScroll = FALSE;
        data->vertScroll = FALSE;
        data->horizProp = horiz;
        data->vertProp = vert;
        data->open = TRUE;
        data->ehn.ehn_Priority = 0;
        data->ehn.ehn_Flags = (1 << 1);
        data->ehn.ehn_Object = obj;
        data->ehn.ehn_Class = NULL;
        data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
        data->viewMode = viewMode;

        data->columns = AllocVec(sizeof(struct DetailColumn) * 4, MEMF_ANY);
        data->columns[0].dc_Content = IA_Label;
        data->columns[0].dc_X = 0;
        data->columns[0].dc_Width = 120;
        data->columns[1].dc_Content = IA_Size;
        data->columns[1].dc_X = 121;
        data->columns[1].dc_Width = 60;
        data->columns[2].dc_Content = IA_Type;
        data->columns[2].dc_X = 182;
        data->columns[2].dc_Width = 80;
        data->columns[3].dc_Content = IA_LastModified;
        data->columns[3].dc_X = 263;
        data->columns[3].dc_Width = 160;

        data->numColumns = 4;
    }

    return retval;
}

IPTR iconConSetup(Class * cl, Object * obj, struct MUIP_Setup * msg)
{
    IPTR            retval;
    //struct MemberNode *mn;
    struct IconContainerClassData *data;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    // the superclass will send the method to the memberlist
    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->horizProp)
        DoSetupMethod(data->horizProp, msg->RenderInfo);

    if (data->vertProp)
        DoSetupMethod(data->vertProp, msg->RenderInfo);

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) &data->ehn);

    return retval;
}

IPTR iconConCleanup(Class * cl, Object * obj, struct MUIP_Cleanup * msg)
{
    IPTR            retval = 0;
    struct IconContainerClassData *data;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    DoMethod(obj, MUIM_Window_RemEventHandler, (IPTR) &data->ehn);

    // the superclass will send this method to the memberlist
    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR iconConShow(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;
    //struct MemberNode *mn;
    struct IconContainerClassData *data;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    // the superclass will send this method to the memberlist
    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->horizProp)
        DoMethod(data->horizProp, MUIM_Show);

    if (data->vertProp)
        DoMethod(data->vertProp, MUIM_Show);

    return retval;
}

IPTR iconConAskMinMax(Class * cl, Object * obj, struct MUIP_AskMinMax * msg)
{
    IPTR            retval;
    struct MUI_MinMax minMax;
    struct IconContainerClassData *data;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    // the superclass will send this method to the memberlist
    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth += 20;
    msg->MinMaxInfo->DefWidth += 300;
    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MinHeight += 20;
    msg->MinMaxInfo->DefHeight += 300;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    if (data->vertProp)
    {
        minMax.MinWidth = 0;
        minMax.DefWidth = 0;
        minMax.MaxWidth = 0;
        minMax.MinHeight = 0;
        minMax.DefHeight = 0;
        minMax.MaxHeight = 0;
        DoMethod(data->vertProp, MUIM_AskMinMax, (IPTR) &minMax);
    }

    if (data->horizProp)
    {
        minMax.MinWidth = 0;
        minMax.DefWidth = 0;
        minMax.MaxWidth = 0;
        minMax.MinHeight = 0;
        minMax.DefHeight = 0;
        minMax.MaxHeight = 0;
        DoMethod(data->horizProp, MUIM_AskMinMax, (IPTR) &minMax);
    }

    return retval;
}

BOOL canLay(Object * parent, Object * newObject, ULONG newX, ULONG newY,
            struct MinList * memberList, struct IconContainerClassData * data)
{
    struct MemberNode *mn;

    if (_memberCount(parent) != 1)
    {
        mn = (struct MemberNode *) _memberList(parent).mlh_Head;
        while (mn->m_Node.mln_Succ)
        {
            if (!
                ((newX < _left(mn->m_Object) || newX > _right(mn->m_Object))
                 && (newY < _top(mn->m_Object)
                     || newY > _bottom(mn->m_Object))
                 && (newX + _defwidth(mn->m_Object) < _left(mn->m_Object)
                     || newX + _defwidth(mn->m_Object) > _right(mn->m_Object))
                 && (newY + _defheight(mn->m_Object) < _top(mn->m_Object)
                     || newY + _defheight(mn->m_Object) >
                     _bottom(mn->m_Object))))
            {
                return FALSE;
            }

            mn = (struct MemberNode *) mn->m_Node.mln_Succ;
        }

        return TRUE;
    }
    else
        return TRUE;

    return FALSE;
}

// lay out a newly added icon
ULONG layoutObject(struct IconContainerClassData * data, Object * obj,
                   Object * newObject)
{
    ULONG           retval = 0;
    ULONG           newX,
                    newY;
    BOOL            laid = TRUE;

    if (data->viewMode == ICAVM_DETAIL)
    {
        newX = ICONSPACINGX;
        if (_memberCount(obj) == 1)
            newY = ICONSPACINGY;
        else
            newY = _bottom(data->last) + ICONSPACINGY - _mtop(obj);
        if (newX + _defwidth(newObject) + ICONSPACINGX > data->virtualWidth)
            data->virtualWidth = newX + _defwidth(newObject) + ICONSPACINGX;

        if (newY + _defheight(newObject) + ICONSPACINGY > data->virtualHeight)
            data->virtualHeight = newY + _defheight(newObject) + ICONSPACINGX;

        MUI_Layout(newObject, newX, newY, _defwidth(newObject),
                   _defheight(newObject), 0);
    }
    else
    {
        if (data->perfectLayout)
        {
            if (_memberCount(obj) == 1)
            {
                newX = ICONSPACINGX;
                newY = ICONSPACINGY;

            // data->virtualWidth=_defwidth(newObject)+ICONSPACINGX;
                data->thisColumnWidth = _defwidth(newObject);
            }
            else
            {
                newX = _left(data->last) - _mleft(obj);
                newY = _bottom(data->last) + ICONSPACINGY - _mtop(obj);
                if (newY + _defheight(newObject) > _mheight(obj))
                {
                // new column
                    newX += data->thisColumnWidth + ICONSPACINGX;
                    newY = ICONSPACINGY;
                // data->virtualWidth+=(_defwidth(newObject)+ICONSPACINGX);
                    data->thisColumnHeight = 0;
                    data->thisColumnWidth = 0;
                }

                if (_defwidth(newObject) > data->thisColumnWidth)
                {
                // data->virtualWidth+=(_defwidth(newObject)-data->thisColumnWidth);
                    data->thisColumnWidth = _defwidth(newObject);
                }
            }

            data->thisColumnHeight += (ICONSPACINGY + _defheight(newObject));
        // if(data->thisColumnHeight+_defheight(newObject)+ICONSPACINGY >
        // data->virtualHeight)
        // data->virtualHeight+=data->thisColumnHeight;
        }
        else
        {
            struct MemberNode *mn;
            BOOL            laid = FALSE;

            newX = ICONSPACINGX;
            newY = ICONSPACINGY;

            laid =
                canLay(obj, newObject, newX, newY, &(_memberList(obj)), data);

            mn = (struct MemberNode *) _memberList(obj).mlh_Head;
            while (mn->m_Node.mln_Succ && !laid)
            {
                newX = _left(mn->m_Object) - _mleft(obj);
                newY = _bottom(mn->m_Object) + ICONSPACINGY - _mtop(obj);

            // will the icon go off the bottom of the screen?
                if ((newY + _defwidth(newObject) > _mheight(obj))
                    && _mheight(obj) > _defheight(newObject))
                {
                    newX += _width(mn->m_Object) + ICONSPACINGX;
                    newY = ICONSPACINGY;
                }

                laid =
                    canLay(obj, newObject, newX, newY, &(_memberList(obj)),
                           data);

                mn = (struct MemberNode *) mn->m_Node.mln_Succ;
            }
        }

        if (laid)
        {
            if (newX + _defwidth(newObject) + ICONSPACINGX >
                data->virtualWidth)
                data->virtualWidth =
                    newX + _defwidth(newObject + ICONSPACINGX);

            if (newY + _defheight(newObject) + ICONSPACINGY >
                data->virtualHeight)
                data->virtualHeight =
                    newY + _defheight(newObject) + ICONSPACINGX;

            MUI_Layout(newObject, newX, newY, _defwidth(newObject),
                       _defheight(newObject), 0);
        }
    }

    data->last = newObject;

    return retval;
}


ULONG iconConLayout(Class * cl, Object * obj, Msg msg)
{
    ULONG           retval;
    struct IconContainerClassData *data;

    data = INST_DATA(cl, obj);
    retval = DoSuperMethodA(cl, obj, msg);

    if (data->visibleHeight == 0)
    {
    // first layout
        SetAttrs(data->horizProp, MUIA_Prop_Visible, _mwidth(obj), TAG_END);
        SetAttrs(data->vertProp, MUIA_Prop_Visible, _mheight(obj), TAG_END);
    }
    else
    {
        if (data->visibleHeight != _mheight(obj))
        {
        // height adjusted
            data->heightAdjusted = _mheight(obj) - data->visibleHeight;
        }

        if (data->visibleWidth != _mwidth(obj))
        {
        // width adjusted
            data->widthAdjusted = _mwidth(obj) - data->visibleWidth;
        }

    }

    return retval;
}

IPTR iconConAdd(Class * cl, Object * obj, struct opMember * msg)
{
    struct IconContainerClassData *data;
    //struct MemberNode *mn;
    ULONG           retval = 1;
    struct MUI_MinMax minMax;
    APTR            clip;

    minMax.MinWidth = 0;
    minMax.DefWidth = 0;
    minMax.MaxWidth = 0;
    minMax.MinHeight = 0;
    minMax.DefHeight = 0;
    minMax.MaxHeight = 0;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    DoSetupMethod(msg->opam_Object, muiRenderInfo(obj));

    DoMethod(msg->opam_Object, MUIM_AskMinMax, (IPTR) &minMax);

    _minwidth(msg->opam_Object) = minMax.MinWidth;
    _minheight(msg->opam_Object) = minMax.MinHeight;
    _maxwidth(msg->opam_Object) = minMax.MaxWidth;
    _maxheight(msg->opam_Object) = minMax.MaxHeight;
    _defwidth(msg->opam_Object) = minMax.DefWidth;
    _defheight(msg->opam_Object) = minMax.DefHeight;

    layoutObject(data, obj, msg->opam_Object);

    DoMethod(msg->opam_Object, MUIM_Show);

    clip =
        MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
                        _mwidth(obj), _mheight(obj));
    MUI_Redraw(msg->opam_Object, MADF_DRAWOBJECT);
    MUI_RemoveClipping(muiRenderInfo(obj), clip);
//    _memberCount(obj)++;
//    AddTail((struct List *) &(_memberList(obj)), (struct Node *) mn);
    SetAttrs(data->horizProp, MUIA_Prop_Entries, data->virtualWidth, TAG_END);
    SetAttrs(data->vertProp, MUIA_Prop_Entries, data->virtualHeight, TAG_END);

    return retval;
}

void redrawRectangle(LONG x1, LONG y1, LONG x2, LONG y2, Object * obj,
                     struct IconContainerClassData *data)
{
    struct MemberNode *mn;

    mn = (struct MemberNode *) _memberList(obj).mlh_Head;
    while (mn->m_Node.mln_Succ)
    {
    // check to see if the left or right edge is in the damaged
    // area - also, check to see whether an object is in the area where
    // both edges are to the left and right of the damaged area
        if ((_mleft(mn->m_Object) >= x1 && _mleft(mn->m_Object) <= x2) ||
            (_mright(mn->m_Object) >= x1 && _mright(mn->m_Object) <= x2) ||
            (_mleft(mn->m_Object) <= x1 && _mright(mn->m_Object) >= x2))
        {
        // as above, except with the top and bottom edges of the member
        // objects
            if ((_mtop(mn->m_Object) >= y1 && _mtop(mn->m_Object) <= y2) ||
                (_mbottom(mn->m_Object) >= y1 && _mbottom(mn->m_Object) <= y2)
                || (_mtop(mn->m_Object) <= y1
                    && _mbottom(mn->m_Object) >= y2))
            {
                MUI_Redraw(mn->m_Object, MADF_DRAWOBJECT);
            }
        }

        mn = (struct MemberNode *) mn->m_Node.mln_Succ;
    }
}

IPTR drawAll(Class * cl, Object * obj, struct MUIP_Draw *msg,
             struct IconContainerClassData *data)
{
    IPTR            retval = 1;
    struct MemberNode *mn;
    APTR            clip;

    clip =
        MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
                        _mwidth(obj), _mheight(obj));

    if (data->widthAdjusted != 0)
    {
        if (data->widthAdjusted > 0)
            redrawRectangle(_mright(obj) - data->widthAdjusted, _mtop(obj),
                            _mright(obj), _mbottom(obj), obj, data);
        SetAttrs(data->horizProp, MUIA_Prop_Visible, _mwidth(obj), TAG_END);
    }

    if (data->heightAdjusted != 0)
    {
        if (data->heightAdjusted > 0)
            redrawRectangle(_mleft(obj), _mbottom(obj) - data->heightAdjusted,
                            _mright(obj), _mbottom(obj), obj, data);
        SetAttrs(data->vertProp, MUIA_Prop_Visible, _mheight(obj), TAG_END);
    }

    if (data->heightAdjusted == 0 && data->widthAdjusted == 0)
    {
    // if(!(muiRenderInfo(obj)->mri_Flags & MUIMRI_REFRESHMODE))
    // {
    // SetAttrs(data->horizProp, MUIA_Prop_Visible, _mwidth(obj), TAG_END);
    // SetAttrs(data->vertProp, MUIA_Prop_Visible, _mheight(obj), TAG_END);
    // }
        EraseRect(_rp(obj), _mleft(obj), _mtop(obj), _mright(obj),
                  _mbottom(obj));

        mn = (struct MemberNode *) _memberList(obj).mlh_Head;
        while (mn->m_Node.mln_Succ)
        {
            MUI_Redraw(mn->m_Object, MADF_DRAWOBJECT);
            mn = (struct MemberNode *) mn->m_Node.mln_Succ;
        }
    }

    data->visibleWidth = _mwidth(obj);
    data->visibleHeight = _mheight(obj);
    data->widthAdjusted = 0;
    data->heightAdjusted = 0;

    MUI_RemoveClipping(muiRenderInfo(obj), clip);

    return retval;
}

IPTR iconConDraw(Class * cl, Object * obj, struct MUIP_Draw * msg)
{
    struct IconContainerClassData *data;
    struct MemberNode *mn;
    IPTR            retval = 1;
    APTR            clip = NULL;
    BOOL            layerrefresh,
                    scroll_caused_damage;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    SetAttrs(obj, AICA_ApplyMethodsToMembers, FALSE, TAG_END);
    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    SetAttrs(obj, AICA_ApplyMethodsToMembers, TRUE, TAG_END);

    if ((msg->flags & MADF_DRAWOBJECT) || (msg->flags & MADF_DRAWALL))
    {
        retval = drawAll(cl, obj, msg, data);
    }
    else if (msg->flags & MADF_DRAWUPDATE)
    {
        LONG            scrollAmountX = 0,
            scrollAmountY = 0;
        LONG            redrawX1 = 0,
                        redrawY1 = 0,
                        redrawX2 = 0,
                        redrawY2 = 0;

        if (data->vertScroll)
        {
        // vertical scroll
            scrollAmountY = data->yView - data->lastYView;

            if (scrollAmountY > 0)
            {
            // scroll bottom, displays shifts up, redraw gap at bottom
                redrawY1 = _mbottom(obj) - abs(scrollAmountY);
                redrawY2 = _mbottom(obj);
            }
            else if (scrollAmountY < 0)
            {
            // scroll top, display shifts bottom, redraw gap at top
                redrawY1 = _mtop(obj);
                redrawY2 = _mtop(obj) + abs(scrollAmountY);
            }

            redrawX1 = _mleft(obj);
            redrawX2 = _mright(obj);

        // shift the positions of the member objects
            mn = (struct MemberNode *) _memberList(obj).mlh_Head;
            while (mn->m_Node.mln_Succ)
            {
                MUI_Layout(mn->m_Object,
                           _left(mn->m_Object) - scrollAmountX - _mleft(obj),
                           _top(mn->m_Object) - scrollAmountY - _mtop(obj),
                           _width(mn->m_Object), _height(mn->m_Object), 0);
                mn = (struct MemberNode *) mn->m_Node.mln_Succ;
            }

            clip =
                MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
                                _mwidth(obj), _mheight(obj));

            layerrefresh =
                (_rp(obj)->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;

            ScrollRaster(_rp(obj), 0, scrollAmountY, _mleft(obj), _mtop(obj),
                         _mright(obj), _mbottom(obj));

        // redraw in gap
            redrawRectangle(redrawX1, redrawY1, redrawX2, redrawY2, obj,
                            data);

            if ((_rp(obj)->Layer->Flags & LAYERREFRESH) && !layerrefresh)
                scroll_caused_damage = TRUE;
            else
                scroll_caused_damage = FALSE;

            MUI_RemoveClipping(muiRenderInfo(obj), clip);

            if (scroll_caused_damage)
            {
                struct Region  *damageList;
                WORD            x1,
                                y1,
                                x2,
                                y2;

            // get the damage area bounds in x1,x2,y1,y2
                LockLayer(0, _window(obj)->WLayer);
                damageList = _window(obj)->WLayer->DamageList;
                x1 = damageList->bounds.MinX;
                y1 = damageList->bounds.MinY;
                x2 = damageList->bounds.MaxX;
                y2 = damageList->bounds.MaxY;
                UnlockLayer(_window(obj)->WLayer);

                if (MUI_BeginRefresh(muiRenderInfo(obj), 0))
                {
                    EraseRect(_rp(obj), x1, y1, x2, y2);
                    redrawRectangle(x1, y1, x2, y2, obj, data);

                    MUI_EndRefresh(muiRenderInfo(obj), 0);
                }
            }

            data->vertScroll = FALSE;
        }
        else if (data->horizScroll)
        {
        // horizontal scroll
            scrollAmountX = data->xView - data->lastXView;

        // redraw gap area
            if (scrollAmountX > 0)
            {
            // scroll right, displays shifts left, redraw gap at right
                redrawX1 = _mright(obj) - abs(scrollAmountX);
                redrawX2 = _mright(obj);

            }
            else if (scrollAmountX < 0)
            {
            // scroll left, display shifts right, redraw gap at left
                redrawX1 = _mleft(obj);
                redrawX2 = _mleft(obj) + abs(scrollAmountX);
            }

            redrawY1 = _mtop(obj);
            redrawY2 = _mbottom(obj);

        // shift the positions of the member objects
            mn = (struct MemberNode *) _memberList(obj).mlh_Head;
            while (mn->m_Node.mln_Succ)
            {
                MUI_Layout(mn->m_Object,
                           _left(mn->m_Object) - scrollAmountX - _mleft(obj),
                           _top(mn->m_Object) - scrollAmountY - _mtop(obj),
                           _width(mn->m_Object), _height(mn->m_Object), 0);

                mn = (struct MemberNode *) mn->m_Node.mln_Succ;
            }

            clip =
                MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
                                _mwidth(obj), _mheight(obj));

            layerrefresh =
                (_rp(obj)->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;

            ScrollRaster(_rp(obj), scrollAmountX, 0, _mleft(obj), _mtop(obj),
                         _mright(obj), _mbottom(obj));

            redrawRectangle(redrawX1, redrawY1, redrawX2, redrawY2, obj,
                            data);

            if ((_rp(obj)->Layer->Flags & LAYERREFRESH) && !layerrefresh)
                scroll_caused_damage = TRUE;
            else
                scroll_caused_damage = FALSE;

            MUI_RemoveClipping(muiRenderInfo(obj), clip);

            if (scroll_caused_damage)
            {
                struct Region  *damageList;
                WORD            x1,
                                y1,
                                x2,
                                y2;

            // get the damage area bounds in x1,x2,y1,y2
                LockLayer(0, _window(obj)->WLayer);
                damageList = _window(obj)->WLayer->DamageList;
                x1 = damageList->bounds.MinX;
                y1 = damageList->bounds.MinY;
                x2 = damageList->bounds.MaxX;
                y2 = damageList->bounds.MaxY;
                UnlockLayer(_window(obj)->WLayer);

                if (MUI_BeginRefresh(muiRenderInfo(obj), 0))
                {
                    EraseRect(_rp(obj), x1, y1, x2, y2);
                    redrawRectangle(x1, y1, x2, y2, obj, data);

                    MUI_EndRefresh(muiRenderInfo(obj), 0);
                }
            }
        }
        data->horizScroll = FALSE;
    }

    return retval;
}

IPTR iconConSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct IconContainerClassData *data;
    IPTR            retval;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case ICA_ScrollToHoriz:
                {
                    data->lastXView = data->xView;
                    data->xView = tag->ti_Data;
                    data->horizScroll = TRUE;
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                    break;
                }
            case ICA_ScrollToVert:

                data->lastYView = data->yView;
                data->yView = tag->ti_Data;
                data->vertScroll = TRUE;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                break;
            case ICA_Open:

                data->open = tag->ti_Data;
                SetAttrs(_win(obj), MUIA_Window_Open, FALSE, TAG_END);
                break;
            case ICA_ViewMode:
{
                    ULONG           iconViewMode = IAVM_LARGEICON;
                    struct MemberNode *mn;
                    struct MUI_MinMax minMax;

                    data->viewMode = tag->ti_Data;
                    switch (data->viewMode)
                    {
                        case ICAVM_LARGE:
                            iconViewMode = IAVM_LARGEICON;
                            break;
                        case ICAVM_SMALL:
                            iconViewMode = IAVM_SMALLICON;
                            break;
                        case ICAVM_DETAIL:
                            iconViewMode = IAVM_DETAIL;
                            break;
                    }

                    _memberCount(obj) = 0;
                    data->thisColumnWidth = 0;
                    data->thisColumnHeight = 0;
                    data->virtualWidth = 0;
                    data->virtualHeight = 0;

//                    mn = (struct MemberNode *) data->memberList.mlh_Head;
                    mn = (struct MemberNode *) _memberList(obj).mlh_Head;
                    while (mn->m_Node.mln_Succ)
                    {
                        DoMethod(mn->m_Object, MUIM_Hide);
                        SetAttrs(mn->m_Object, IA_ViewMode, iconViewMode,
                                 TAG_END);
                        muiNotifyData(mn->m_Object)->mnd_ParentObject = obj;
                        DoMethod(mn->m_Object, MUIM_ConnectParent, (IPTR) obj);
                        DoMethod(mn->m_Object, MUIM_AskMinMax, (IPTR) &minMax);

                        _minwidth(mn->m_Object) = minMax.MinWidth;
                        _minheight(mn->m_Object) = minMax.MinHeight;
                        _maxwidth(mn->m_Object) = minMax.MaxWidth;
                        _maxheight(mn->m_Object) = minMax.MaxHeight;
                        _defwidth(mn->m_Object) = minMax.DefWidth;
                        _defheight(mn->m_Object) = minMax.DefHeight;

                        _memberCount(obj)++;

                        layoutObject(data, obj, mn->m_Object);
                        DoMethod(mn->m_Object, MUIM_Show);

                        SetAttrs(data->horizProp, MUIA_Prop_Entries,
                                 data->virtualWidth, TAG_END);
                        SetAttrs(data->vertProp, MUIA_Prop_Entries,
                                 data->virtualHeight, TAG_END);

                        mn = (struct MemberNode *) mn->m_Node.mln_Succ;
                    }

                    MUI_Redraw(obj, MADF_DRAWALL);
                    break;
                }
            default:
                break;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return 0;
}

IPTR iconConGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct IconContainerClassData *data;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        case ICA_Open:
            *msg->opg_Storage = (ULONG) data->open;
            break;
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR iconConConnectParent(Class * cl, Object * obj,
                          struct MUIP_ConnectParent * msg)
{
    struct IconContainerClassData *data;
    IPTR            retval;

    retval = DoSuperMethodA(cl, obj, msg);

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    if (data->horizProp)
    {
        muiNotifyData(data->horizProp)->mnd_ParentObject = obj;
        DoMethod(data->horizProp, MUIM_ConnectParent, (IPTR) obj);
    }
    if (data->vertProp)
    {
        muiNotifyData(data->vertProp)->mnd_ParentObject = obj;
        DoMethod(data->vertProp, MUIM_ConnectParent, (IPTR) obj);
    }

    return retval;
}

IPTR iconConHandleInput(Class * cl, Object * obj,
                        struct MUIP_HandleInput * msg)
{
    IPTR            retval = 0;

    struct IconContainerClassData *data;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    if (msg->imsg)
    {
        switch (msg->imsg->Class)
        {
            case IDCMP_MOUSEBUTTONS:
                {
                    if (msg->imsg->Code == SELECTDOWN)
                    {
                    // kprintf("ic handleinput\n");

                        if (msg->imsg->MouseX >= _mleft(obj)
                            && msg->imsg->MouseX <= _mright(obj)
                            && msg->imsg->MouseY >= _mtop(obj)
                            && msg->imsg->MouseY <= _mbottom(obj))
                        {
                        // kprintf("ic1\n");
                        // if(!data->justSelected)
                        // {
                        // kprintf("ic2\n");
                        // DoMethod(obj, ICM_UnselectAll);
                        // }
                        // else
                        // {
                        // kprintf("ic3\n");
                        // data->justSelected=FALSE;
                        // }
                        // kprintf("ic4\n");
                        }
                    }
                    break;
                }
        }
    }

    return retval;
}

IPTR iconConDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;

    SetAttrs(obj, ICA_DeleteMe, TRUE, TAG_END);

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR iconConGetColumn(Class * cl, Object * obj, struct opGetColumn * msg)
{
    struct IconContainerClassData *data;
    BOOL            found = FALSE;
    ULONG           i = 0;
    struct DetailColumn *dc = NULL;

    data = (struct IconContainerClassData *) INST_DATA(cl, obj);

    while (!found && i < data->numColumns)
    {
        if (data->columns[i].dc_Content == msg->colType)
        {
            found = TRUE;
            dc = &data->columns[i];
        }
        i++;
    }

    return (IPTR) dc;
}

BOOPSI_DISPATCHER(IPTR, iconContainerDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = iconConNew(cl, obj, (struct opSet *) msg);
            break;
        case MUIM_Setup:
            retval = iconConSetup(cl, obj, (struct MUIP_Setup *) msg);
            break;
        case MUIM_Cleanup:
            retval = iconConCleanup(cl, obj, (struct MUIP_Cleanup *) msg);
            break;
        case MUIM_Show:
            retval = iconConShow(cl, obj, msg);
            break;
        case MUIM_Draw:
            retval = iconConDraw(cl, obj, (struct MUIP_Draw *) msg);
            break;
        case MUIM_AskMinMax:
            retval = iconConAskMinMax(cl, obj, (struct MUIP_AskMinMax *) msg);
            break;
        case MUIM_Layout:
            retval = iconConLayout(cl, obj, msg);
            break;
        case MUIM_DrawBackground:
            break;
        case OM_ADDMEMBER:
            retval = iconConAdd(cl, obj, (struct opMember *) msg);
            break;
        case OM_SET:
            retval = iconConSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = iconConGet(cl, obj, (struct opGet *) msg);
            break;
        case MUIM_ConnectParent:
            retval =
                iconConConnectParent(cl, obj,
                                     (struct MUIP_ConnectParent *) msg);
            break;
        case MUIM_HandleInput:
            retval =
                iconConHandleInput(cl, obj, (struct MUIP_HandleInput *) msg);
            break;
        case OM_DISPOSE:
            retval = iconConDispose(cl, obj, msg);
            break;
        case ICM_GetColumn:
            retval = iconConGetColumn(cl, obj, (struct opGetColumn *) msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
