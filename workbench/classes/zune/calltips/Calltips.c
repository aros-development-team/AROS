/*
    Copyright (C) 2014, Thore Böckelmann. All rights reserved.
    Copyright (C) 2014-2026, The AROS Development Team. All rights reserved.
*/

/****** Calltips.mcc/MUIC_Calltips ************
*
*   NAME
*       MUIC_Calltips
*
*   SUPERCLASS
*       Window.mui
*
*   DESCRIPTION
*       Create a window positioned relative to a source object's native window.
*       Supply a live Source and valid Window content/root object through the
*       superclass interface. The source must already provide usable screen and
*       window information: construction immediately reads it and calculates
*       geometry.
*       The class supplies a borderless, nonactivating window without size,
*       drag, depth, or close gadgets and without menus, then passes the caller
*       tags to Window. It does not create tip text or a content object, open
*       automatically, or establish an input/event observer for the source. The
*       caller must invoke the Parent* methods at the corresponding lifecycle
*       points.
*       Without Rectangle, use the source inner left and right edges, starting
*       one pixel below its inner bottom. The stored bottom is source inner
*       bottom plus 101, giving an inclusive height of 101 pixels. Source-window
*       screen position is added when applying the rectangle. The class reports
*       Version 20 and Revision 12; its configuration also declares 20.12.
*
*   NOTES
*       Source and the Window root object are borrowed references used during
*       geometry updates. Keep them valid and keep the source's native window
*       open whenever geometry is applied. Close the calltips window before its
*       source window becomes unavailable. Rectangle input is copied, not
*       retained.
*       For an open window, changed geometry calls ChangeWindowBox and then
*       MoveWindowInFrontOf relative to the source window. For a closed window,
*       the class sets superclass position and dimensions. It does not report
*       whether the requested geometry was accepted or clamp it to screen
*       bounds.
*       Construction uses the last supplied value of each local attribute. Only
*       Rectangle and Layout have local setters. No Calltips attribute has a
*       local getter; OM_GET handles Version and Revision and otherwise
*       delegates.
*
*   BUGS
*       Layout is stored but never consulted, so Exact is the only declared mode
*       and other values add no behavior. ParentSetup and ParentCleanup are
*       no-ops.
*       The vertical margin calculation tests MarginLeft, not MarginTop.
*       MarginTop is stored at construction but has no effect. MarginLeft
*       controls subtraction of both the root object's inner-left and inner-top
*       offsets.
*       ParentShow and ParentWindowArranged do not provide continuous tracking
*       or an always-on-top policy. An already-open window with unchanged
*       geometry is not explicitly brought forward. ParentShow returns zero even
*       when opening fails; query Window_Open to determine its state.
*
*/
/****** Calltips.mcc/MUIA_Calltips_Source ************
*
*   NAME
*       MUIA_Calltips_Source
*
*   FUNCTION
*       Select the object whose native window anchors the calltips window.
*
*   INPUTS
*       Initialize only; non-NULL Object *. No local setter or getter.
*
*   NOTES
*       A missing or NULL final Source causes construction to return NULL. The
*       pointer is borrowed, not copied or disposed by this class. Its screen is
*       supplied to Window and its native window position is read during every
*       geometry update. Supply an object with valid screen/window state and
*       keep it alive for the calltips lifetime. No automatic parent
*       notifications or observers are installed.
*
*/
/****** Calltips.mcc/MUIA_Calltips_Rectangle ************
*
*   NAME
*       MUIA_Calltips_Rectangle
*
*   FUNCTION
*       Copy the desired rectangle relative to the source native window.
*
*   INPUTS
*       Initialize, set; const struct Rect32 * to readable MinX, MinY, MaxX,
*       MaxY LONG fields. No local getter.
*
*   NOTES
*       The fields are copied immediately. Width and height are Max-Min+1, so
*       coordinates are inclusive. Use ordered coordinates and representable
*       dimensions; they are not validated here. The coordinate system is the
*       source native window, not the source object's local origin.
*       At construction, NULL chooses the default rectangle below the source.
*       During Set, NULL is ignored and does not reset geometry. Each non-NULL
*       setter occurrence is applied before forwarding the original tags.
*       Source-window origin and the MarginLeft-controlled adjustments are then
*       applied. Input storage may be released after the call, but forwarded
*       notifications still carry the caller pointer.
*
*/
/****** Calltips.mcc/MUIA_Calltips_Layout ************
*
*   NAME
*       MUIA_Calltips_Layout
*
*   FUNCTION
*       Store the requested layout selector.
*
*   INPUTS
*       Initialize, set; ULONG. MUIV_Calltips_Layout_Exact (0, default). No
*       local getter.
*
*   NOTES
*       The current implementation never reads this field when sizing or
*       positioning. Setting it alone does not update geometry. Other numeric
*       values are stored without validation and have no defined additional
*       behavior. This is a stored selector, not an implemented layout-mode
*       switch.
*
*/
/****** Calltips.mcc/MUIA_Calltips_MarginLeft ************
*
*   NAME
*       MUIA_Calltips_MarginLeft
*
*   FUNCTION
*       Control subtraction of root-object offsets during positioning.
*
*   INPUTS
*       Initialize only; BOOL, default TRUE. Nonzero input is normalized to
*       TRUE. No local setter or getter.
*
*   NOTES
*       TRUE subtracts the root object's inner-left offset from the requested
*       screen left. The implementation also uses this same flag to subtract its
*       inner-top offset from the requested screen top. FALSE disables both
*       subtractions. It does not change width or height. MarginTop does not
*       independently control the vertical subtraction in this source.
*
*/
/****** Calltips.mcc/MUIA_Calltips_MarginTop ************
*
*   NAME
*       MUIA_Calltips_MarginTop
*
*   FUNCTION
*       Store the requested vertical margin flag.
*
*   INPUTS
*       Initialize only; BOOL, default TRUE. Nonzero input is normalized to
*       TRUE. No local setter or getter.
*
*   NOTES
*       The value is saved but never consulted by the geometry helper. Actual
*       vertical offset subtraction follows MarginLeft. Changing this
*       constructor flag alone has no positioning effect; no independent
*       top-margin behavior is implemented.
*
*/
/****** Calltips.mcc/MUIM_Calltips_SetRectangle ************
*
*   NAME
*       MUIM_Calltips_SetRectangle
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Calltips_SetRectangle, MinX, MinY,
*       MaxX, MaxY);
*
*   FUNCTION
*       Replace the stored rectangle and immediately apply geometry.
*
*   INPUTS
*       obj - Live Calltips object.
*       MinX, MinY, MaxX, MaxY - LONG coordinates, inclusive, relative to the
*       source native window.
*
*   RESULT
*       Always zero; no success or failure status is returned.
*
*   NOTES
*       Copies the four values directly, without range checking, then runs the
*       same geometry helper as Rectangle. The source window and root object
*       must be valid. This method does not send a Rectangle attribute tag, so
*       no Rectangle notification is synthesized.
*
*/
/****** Calltips.mcc/MUIM_Calltips_ParentSetup ************
*
*   NAME
*       MUIM_Calltips_ParentSetup
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Calltips_ParentSetup);
*
*   FUNCTION
*       Accept the parent setup lifecycle call without performing work.
*
*   INPUTS
*       obj - Live Calltips object.
*       No payload arguments.
*
*   RESULT
*       Always zero; no success or failure status is returned.
*
*   NOTES
*       The implementation only returns zero. It does not set up the calltips
*       window, register tracking, or validate the source. The caller remains
*       responsible for the normal Window/Application lifecycle.
*
*/
/****** Calltips.mcc/MUIM_Calltips_ParentCleanup ************
*
*   NAME
*       MUIM_Calltips_ParentCleanup
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Calltips_ParentCleanup);
*
*   FUNCTION
*       Accept the parent cleanup lifecycle call without performing work.
*
*   INPUTS
*       obj - Live Calltips object.
*       No payload arguments.
*
*   RESULT
*       Always zero; no success or failure status is returned.
*
*   NOTES
*       The implementation only returns zero. It does not close the window,
*       clear borrowed pointers, or release parent-related state. Use ParentHide
*       to close the calltips window before the source native window becomes
*       unavailable.
*
*/
/****** Calltips.mcc/MUIM_Calltips_ParentShow ************
*
*   NAME
*       MUIM_Calltips_ParentShow
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Calltips_ParentShow);
*
*   FUNCTION
*       Apply current geometry and open the calltips window if closed.
*
*   INPUTS
*       obj - Live Calltips object.
*       No payload arguments.
*
*   RESULT
*       Always zero; no success or failure status is returned.
*
*   NOTES
*       Requires valid source and root objects and the normal prerequisites for
*       opening a Window. Apply geometry first. If initially closed, set
*       Window_Open TRUE and bring the new native window in front of the source
*       only if the getter confirms it opened. Query Window_Open afterward; the
*       return value does not report success. An already-open window is brought
*       forward only when the geometry helper detects a change.
*
*/
/****** Calltips.mcc/MUIM_Calltips_ParentHide ************
*
*   NAME
*       MUIM_Calltips_ParentHide
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Calltips_ParentHide);
*
*   FUNCTION
*       Close the calltips window.
*
*   INPUTS
*       obj - Live Calltips object.
*       No payload arguments.
*
*   RESULT
*       Always zero; no success or failure status is returned.
*
*   NOTES
*       Set Window_Open FALSE. The method does not dispose the object, clear
*       Source, or change the stored rectangle. It returns no close-status
*       information. Call before the parent window or source object is torn
*       down.
*
*/
/****** Calltips.mcc/MUIM_Calltips_ParentWindowArranged ************
*
*   NAME
*       MUIM_Calltips_ParentWindowArranged
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Calltips_ParentWindowArranged);
*
*   FUNCTION
*       Reapply geometry using the source window's current position.
*
*   INPUTS
*       obj - Live Calltips object.
*       No payload arguments.
*
*   RESULT
*       Always zero; no success or failure status is returned.
*
*   NOTES
*       The caller must deliver this method; the class installs no arrangement
*       observer. It recomputes screen position from the stored rectangle and
*       source window, but does not regenerate the default rectangle from a
*       resized source object. For open windows, only changed geometry triggers
*       ChangeWindowBox and MoveWindowInFrontOf. It does not open a closed
*       window.
*
*/

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <libraries/mui.h>

#include "Calltips_mcc.h"
#include "Calltips_private.h"

#define VERSION  20
#define REVISION 12

/* Read the live source native window and compute inclusive rectangle
 * dimensions. MarginLeft controls both root-offset subtractions. If open
 * and geometry differs, request ChangeWindowBox and bring the window in
 * front of its source; otherwise set closed-window superclass geometry.
 * Require valid source/root/window state. No validation or success result
 * is supplied.
 */
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

/* Collect the last local tag values, require Source, and create Window with
 * popup-like defaults followed by caller tags. Borrow Source and root
 * references, copy Rectangle or derive the 101-pixel-high default, then
 * immediately apply geometry. Return the created object or zero on missing
 * Source or superclass failure.
 */
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


/* Return the local version constants 20 and 12 for Version and Revision,
 * with TRUE. Delegate all other queries to Window. In particular, none of
 * the Calltips-specific attributes is exposed by a local getter.
 */
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


/* Copy and apply every non-NULL Rectangle in tag order; store Layout
 * without using it. Ignore other local attributes here, then forward the
 * original tags and return the superclass result. NULL Rectangle does not
 * clear or reset geometry and no geometry-success status is returned.
 */
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


/* Copy all four message coordinates into the instance rectangle and apply
 * geometry immediately. Require valid borrowed source/root state and
 * sensible inclusive dimensions. Return zero and do not synthesize a
 * Rectangle attribute notification.
 */
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


/* Accept the lifecycle message and return zero without changing state. No
 * setup, observer registration, window opening, source validation, or
 * resource acquisition occurs in this implementation.
 */
IPTR Calltips__MUIM_Calltips_ParentSetup( struct IClass *cl, Object *obj, Msg msg)
{
    return 0;
}


/* Accept the lifecycle message and return zero without changing state. No
 * close, resource release, pointer clearing, or superclass cleanup is
 * performed; callers must close the window through its normal lifecycle.
 */
IPTR Calltips__MUIM_Calltips_ParentCleanup(struct IClass *cl, Object *obj, Msg msg)
{
    return 0;
}


/* Apply geometry using live source/root state, then request opening only if
 * closed. Confirm Window_Open before bringing a newly opened window ahead
 * of the source. Return zero even on opening failure. Already-open
 * unchanged geometry does not force front placement.
 */
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


/* Set Window_Open FALSE and return zero, ignoring the setter result. Keep
 * the object, borrowed source/root references and stored rectangle for
 * later reuse; no disposal or parent cleanup is performed.
 */
IPTR Calltips__MUIM_Calltips_ParentHide(struct IClass *cl, Object *obj, Msg msg)
{
    set(obj, MUIA_Window_Open, FALSE);

    return 0;
}


/* Apply the saved rectangle relative to the source native window current
 * origin. Do not recompute it from source object bounds and do not open a
 * closed calltips window. The caller must deliver this message while
 * required objects/windows are valid. Return zero.
 */
IPTR Calltips__MUIM_Calltips_ParentWindowArranged(struct IClass *cl, Object *obj, Msg msg)
{
    setRectangle(cl, obj);

    return 0;
}
