/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>
#include <stdlib.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

#define MYDEBUG 1
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "penspec.h"
#include "penadjust_private.h"
#include "locale.h"

/****** Penadjust.mui/MUIC_Penadjust ************
*
*   NAME
*       MUIC_Penadjust
*
*   SUPERCLASS
*       Register.mui
*
*   DESCRIPTION
*       Edit a pen using three register pages: eight MUI pen names, a color-map
*       slider from -128 to 127, and Coloradjust. The default specification is
*       m5. Child controls are owned through the register/group hierarchy;
*       internal specifications are embedded copies. Construction returns NULL
*       if superclass construction fails.
*       The private Spec interface copies a complete MUI_PenSpec and maps m, p
*       or r forms to the corresponding page. Public PSIMode is declared but has
*       no implementation.
*
*   METHODS
*       Setting the private specification updates controls through ordinary
*       setters. Reading it samples the active page and formats a new internal
*       specification; Get therefore changes stored bytes and returns a borrowed
*       pointer. There is no local notification wiring that automatically
*       publishes a new Spec after every user edit.
*       MUI entries select indices zero through seven. The color-map page uses
*       Numeric's -128..127 limits. Coloradjust supplies full component values
*       subject to its interaction quantization. There is no system-pen page:
*       parsing s forms leaves the page unchanged, and the next Get replaces
*       that specification from the currently active page.
*
*   NOTES
*       Only valid terminated specifications in full 32-byte structures may be
*       passed to the private interface; NULL is dereferenced and
*       parser/converter results are ignored. Invalid forms have no dependable
*       editor behavior. System pens are not preserved through a normal Get. The
*       display hook uses a shared static text buffer; its icon text must be
*       consumed synchronously. Preserve the built-in child structure while
*       using this class.
*
*/
/****** Penadjust.mui/MUIA_Penadjust_PSIMode ************
*
*   NAME
*       MUIA_Penadjust_PSIMode
*
*   FUNCTION
*       Declared constructor option without local implementation.
*
*   INPUTS
*       The header declares initialize-only BOOL. No local default, state,
*       setter or getter is implemented.
*
*   NOTES
*       The constructor forwards the tag to Register but does not inspect it. It
*       does not alter the available pages or enable a system-pen editor. Do not
*       infer behavior from the attribute name.
*
*/

extern struct Library *MUIMasterBase;

/*
 * Parse embedded specification and select/synchronize MUI, color-map or RGB
 * page. Requires valid terminated Spec and live controls. Parser failure is
 * unchecked; system form does not select a page. Child setters can notify
 * synchronously; no ownership transfer or result.
 */
static void UpdateState(Object *obj, struct Penadjust_DATA *data)
{
    zune_pen_spec_to_intern(&data->penspec, &data->intpenspec);

    switch (data->intpenspec.p_type)
    {
    case PST_MUI:
        set(data->listobj, MUIA_List_Active, data->intpenspec.p_mui);
        set(obj, MUIA_Group_ActivePage, 0);
        break;

    case PST_CMAP:
        set(data->sliderobj, MUIA_Numeric_Value, data->intpenspec.p_cmap);
        set(obj, MUIA_Group_ActivePage, 1);
        break;

    case PST_RGB:
        set(data->coloradjobj, MUIA_Coloradjust_RGB,
            (IPTR) & data->intpenspec.p_rgb);
        set(obj, MUIA_Group_ActivePage, 2);
        break;

    case PST_SYS:
        break;
    }
}

/*
 * Sample active page and format embedded specification. Requires live
 * controls and valid instance. Getter failures are ignored with values
 * initialized to zero. Replaces system forms from active page; converter
 * status ignored. Returned data stays embedded; no result.
 */
static void UpdatePenspec(Object *obj, struct Penadjust_DATA *data)
{
    IPTR val;

    val = 0;
    get(obj, MUIA_Group_ActivePage, &val);

    switch (val)
    {
    case 0:
        data->intpenspec.p_type = PST_MUI;

        val = 0;
        get(data->listobj, MUIA_List_Active, &val);
        data->intpenspec.p_mui = (LONG) val;
        break;

    case 1:
        data->intpenspec.p_type = PST_CMAP;

        val = 0;
        get(data->sliderobj, MUIA_Numeric_Value, &val);
        data->intpenspec.p_cmap = (LONG) val;
        break;

    case 2:
        data->intpenspec.p_type = PST_RGB;

        val = 0;
        get(data->coloradjobj, MUIA_Coloradjust_Red, &val);
        data->intpenspec.p_rgb.red = (ULONG) val;
        val = 0;
        get(data->coloradjobj, MUIA_Coloradjust_Green, &val);
        data->intpenspec.p_rgb.green = (ULONG) val;
        val = 0;
        get(data->coloradjobj, MUIA_Coloradjust_Blue, &val);
        data->intpenspec.p_rgb.blue = (ULONG) val;
    }

    zune_pen_intern_to_spec(&data->intpenspec, &data->penspec);

    D(bug(" ## penspec now '%s'\n", (char *)&data->penspec));
}

/*
 * Fill list display columns with a static MUI-pen image escape, empty
 * column and borrowed entry label. Requires valid display-array convention
 * including array[-1]; indices outside 0..7 map to zero. Return zero;
 * static buffer is overwritten by later calls and must be consumed
 * synchronously.
 */
static IPTR MuipenDisplayFunc(struct Hook *hook, char **array, char *entry)
{
    LONG line;
    static char buf[16];

    line = (SIPTR) array[-1];
    if (line < 0 || line > 7)
        line = 0;
    snprintf(buf, sizeof(buf), "\033I[2:m%d]", (int)line);

    *array++ = buf;
    *array++ = "";
    *array = entry;

    return 0;
}

/*
 * Construct Register with three owned editor pages and localized static
 * labels, initialize m5 and copy private MUIA_Penadjust_Spec tags. Return
 * object or zero on superclass failure. Spec requires a non-NULL readable
 * full structure; update controls after parsing. PSIMode is not handled.
 */
IPTR Penadjust__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    static const char *register_labels[4];
    register_labels[0] = _(MSG_PENADJUST_MUI);
    register_labels[1] = _(MSG_PENADJUST_COLORMAP);
    register_labels[2] = _(MSG_PENADJUST_RGB);
    register_labels[3] = NULL;
    
    static const char *lv_labels[9];
    lv_labels[0] = _(MSG_PENADJUST_SHINE);
    lv_labels[1] = _(MSG_PENADJUST_HALFSHINE);
    lv_labels[2] = _(MSG_PENADJUST_BACKGROUND);
    lv_labels[3] = _(MSG_PENADJUST_HALFSHADOW);
    lv_labels[4] = _(MSG_PENADJUST_SHADOW);
    lv_labels[5] = _(MSG_PENADJUST_TEXT);
    lv_labels[6] = _(MSG_PENADJUST_FILL);
    lv_labels[7] = _(MSG_PENADJUST_MARK);
    lv_labels[8] = NULL;

    static const struct Hook muipen_display_hook =
        { {NULL, NULL}, HookEntry, MuipenDisplayFunc, NULL };

    struct Penadjust_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    Object *listobj, *sliderobj, *coloradjobj;

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
            
        MUIA_Register_Titles, (IPTR) register_labels,
        Child, (IPTR) ListviewObject,
            MUIA_Listview_List, (IPTR) (listobj = (Object *)ListObject,
                InputListFrame,
                MUIA_List_SourceArray, (IPTR) lv_labels,
                MUIA_List_Format,      (IPTR) ",,",
                MUIA_List_DisplayHook, (IPTR) &muipen_display_hook,
            End),
        End,
        Child, (IPTR) (sliderobj = (Object *)SliderObject,
            MUIA_Slider_Horiz, TRUE,
            MUIA_Numeric_Min,  -128,
            MUIA_Numeric_Max,  127,
        End),
        Child, (IPTR) (coloradjobj = (Object *)ColoradjustObject, End),
        
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->listobj = listobj;
    data->sliderobj = sliderobj;
    data->coloradjobj = coloradjobj;

    strcpy(data->penspec.buf, "m5");

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Penadjust_Spec:
            data->penspec = *(struct MUI_PenSpec *)tag->ti_Data;
            break;
        }
    }

    UpdateState(obj, data);

    return (IPTR) obj;
}

/*
 * Copy each private MUIA_Penadjust_Spec structure, update controls once if
 * supplied, then return Register setter result. Input must be non-NULL,
 * readable and terminated; parser status is unchecked. No ownership
 * transfer and no PSIMode handling.
 */
IPTR Penadjust__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags;
    struct TagItem *tag;
    struct Penadjust_DATA *data = INST_DATA(cl, obj);
    BOOL update = FALSE;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Penadjust_Spec:
            data->penspec = *(struct MUI_PenSpec *)tag->ti_Data;
            update = TRUE;
            break;
        }
    }

    if (update)
        UpdateState(obj, data);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * For private MUIA_Penadjust_Spec, sample active page, overwrite embedded
 * specification and return its borrowed pointer with TRUE. It expires at
 * disposal and changes on later Get/Set. Requires valid storage and
 * children; delegate other queries to Register.
 */
IPTR Penadjust__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Penadjust_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    case MUIA_Penadjust_Spec:
        UpdatePenspec(obj, data);
        *store = (IPTR) & data->penspec;
        break;

    default:
        return DoSuperMethodA(cl, obj, (Msg) msg);

    }

    return TRUE;
}

#if ZUNE_BUILTIN_PENADJUST
/*
 * Dispatch local methods and return their results, delegating other
 * messages to the superclass. Requires ABI-correct messages and a live
 * object except construction. Owned and borrowed resources follow the class
 * lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Penadjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Penadjust__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_SET:
        return Penadjust__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Penadjust__OM_GET(cl, obj, (APTR) msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Penadjust_desc =
{
    MUIC_Penadjust,
    MUIC_Register,
    sizeof(struct Penadjust_DATA),
    (void *) Penadjust_Dispatcher
};
#endif /* ZUNE_BUILTIN_PENADJUST */
