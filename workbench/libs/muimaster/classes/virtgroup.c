/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"

/****** Virtgroup.mui/MUIC_Virtgroup ************
*
*   NAME
*       MUIC_Virtgroup
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Create a Group with virtual-content layout enabled. The only local
*       override is construction: prepend Group_Virtual TRUE and then process
*       caller tags. A later Group_Virtual FALSE can override that default.
*       Virtgroup has no additional instance storage, local disposal or local
*       scrolling implementation.
*       All child ownership, setup/cleanup, clipping, layout hooks, offset
*       storage and mouse-driven edge scrolling belong to Group. Scrollbars are
*       not created automatically. Group saves content minima while exposing a
*       small viewport minimum; width/height getters report computed virtual
*       content extents, not the viewport size.
*
*   METHODS
*       Construction returns the Group constructor result, including NULL on
*       failure. All other methods delegate unchanged to Group. Use a valid
*       connected/setup object and completed min/max calculation before changing
*       offsets: Group's setter can call Layout even when the object is not
*       drawable.
*       In virtual mode Group tracks mouse buttons and ticks for edge
*       autoscroll. Virtgroup_Input does not control that behavior in this
*       implementation.
*
*   NOTES
*       Left/Top are implemented by Group's setter and getter but not its
*       initialization-tag parser, despite the header marking them
*       initialize/set/get. Initial offsets remain the zero-initialized values.
*       Width/Height become meaningful after layout. Private MinWidth/MinHeight
*       expose saved content minima; they are internal interfaces, not public
*       scrolling controls.
*
*/
/****** Virtgroup.mui/MUIA_Virtgroup_Left ************
*
*   NAME
*       MUIA_Virtgroup_Left
*
*   FUNCTION
*       Set or query the horizontal offset into virtual content. Zero selects
*       the leading edge.
*
*   INPUTS
*       LONG; implemented set/get, with int setter temporaries and WORD
*       layout-clamping temporaries. Initialization tags are not consumed by
*       Group.
*
*   NOTES
*       A changed setter stores the converted value and relayouts children. When
*       drawable, it hides/shows children and requests a scroll redraw. Layout
*       clamps offsets to zero through content extent minus viewport extent
*       (with zero as the minimum), then uses Group_Forward FALSE for
*       corrections. This does not suppress notifications. The getter returns
*       the stored value. Use setup and valid child min/max state before
*       setting; avoid setting virtual offsets on nonvirtual groups.
*
*   BUGS
*       Layout copies the stored LONG offset into a WORD before clamping;
*       out-of-range input can wrap instead of saturating. Layout can reenter
*       the setter while correcting offsets. A setter return is not a
*       child-layout success report. Initialization support advertised by the
*       header is absent in this implementation.
*
*/
/****** Virtgroup.mui/MUIA_Virtgroup_Top ************
*
*   NAME
*       MUIA_Virtgroup_Top
*
*   FUNCTION
*       Set or query the vertical offset into virtual content. Zero selects the
*       leading edge.
*
*   INPUTS
*       LONG; implemented set/get, with int setter temporaries and WORD
*       layout-clamping temporaries. Initialization tags are not consumed by
*       Group.
*
*   NOTES
*       A changed setter stores the converted value and relayouts children. When
*       drawable, it hides/shows children and requests a scroll redraw. Layout
*       clamps offsets to zero through content extent minus viewport extent
*       (with zero as the minimum), then uses Group_Forward FALSE for
*       corrections. This does not suppress notifications. The getter returns
*       the stored value. Use setup and valid child min/max state before
*       setting; avoid setting virtual offsets on nonvirtual groups.
*
*   BUGS
*       Layout copies the stored LONG offset into a WORD before clamping;
*       out-of-range input can wrap instead of saturating. Layout can reenter
*       the setter while correcting offsets. A setter return is not a
*       child-layout success report. Initialization support advertised by the
*       header is absent in this implementation.
*
*/
/****** Virtgroup.mui/MUIA_Virtgroup_Width ************
*
*   NAME
*       MUIA_Virtgroup_Width
*
*   FUNCTION
*       Return the computed virtual content width, excluding this Group's outer
*       Area contribution.
*
*   INPUTS
*       LONG; get only. Supply writable IPTR getter storage.
*
*   NOTES
*       Group stores this extent during layout. Read after valid setup, min/max
*       calculation and layout; the initial zero is not a measured content size.
*       The viewport is the inner visible box, not this value. Built-in virtual
*       layout and custom LayoutHook paths calculate extents differently; a
*       custom hook writes Width/Height back in its layout message. No local
*       setter is implemented.
*
*/
/****** Virtgroup.mui/MUIA_Virtgroup_Height ************
*
*   NAME
*       MUIA_Virtgroup_Height
*
*   FUNCTION
*       Return the computed virtual content height, excluding this Group's outer
*       Area contribution.
*
*   INPUTS
*       LONG; get only. Supply writable IPTR getter storage.
*
*   NOTES
*       Group stores this extent during layout. Read after valid setup, min/max
*       calculation and layout; the initial zero is not a measured content size.
*       The viewport is the inner visible box, not this value. Built-in virtual
*       layout and custom LayoutHook paths calculate extents differently; a
*       custom hook writes Width/Height back in its layout message. No local
*       setter is implemented.
*
*/
/****** Virtgroup.mui/MUIA_Virtgroup_Input ************
*
*   NAME
*       MUIA_Virtgroup_Input
*
*   FUNCTION
*       Declared initialization flag without an implementation in the AROS
*       Virtgroup/Group classes.
*
*   INPUTS
*       BOOL initialization declaration in the header; no local consumer, setter
*       or getter.
*
*   NOTES
*       Setting FALSE does not disable Group virtual mouse/tick autoscroll.
*       Setting TRUE does not enable an additional input mode. Unrecognized
*       getters may fall through to children through Group; that does not
*       establish local support. No default behavior should be inferred from
*       this unused declaration.
*
*/

/* Private MUIA_Virtgroup_MinWidth/MinHeight are handled by Group OM_GET.
 * They return saved content minima from AskMinMax, not Boolean flags
 * despite the header annotation. Read only after that calculation;
 * no new storage or ownership is introduced by Virtgroup.
 */
extern struct Library *MUIMasterBase;

/*
 * Construct Group with Group_Virtual TRUE before caller tags, which can
 * override it. Return superclass object or NULL. Child ownership and
 * partial-construction failure cleanup belong to Group; no local allocation
 * or instance state.
 */
IPTR Virtgroup__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    return DoSuperNewTags
        (cl, obj, NULL,
        MUIA_Group_Virtual, TRUE, TAG_MORE, (IPTR) msg->ops_AttrList);
}

#if ZUNE_BUILTIN_VIRTGROUP
/*
 * When built in, route OM_NEW to the virtual-default constructor and every
 * other message unchanged to Group. Return its result. Requires ABI-correct
 * message and live object except construction; no local resources.
 */
BOOPSI_DISPATCHER(IPTR, Virtgroup_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Virtgroup__OM_NEW(cl, obj, (struct opSet *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Virtgroup_desc =
{
    MUIC_Virtgroup,
    MUIC_Group,
    0,
    (void *) Virtgroup_Dispatcher
};
#endif /* ZUNE_BUILTIN_VIRTGROUP */
