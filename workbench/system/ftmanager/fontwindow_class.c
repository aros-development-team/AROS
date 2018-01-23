#include <proto/alib.h>

#define NO_INLINE_STDARG

#include <aros/debug.h>
#include <aros/asmcall.h>
#include <libraries/mui.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "fontwindow_class.h"
#include "fontinfo_class.h"
#include "globals.h"
#include "locale.h"

struct FontWindowData
{
    FT_Face Face;
};
typedef struct FontWindowData FontWindowData;

AROS_UFH3(void, CloseWinFunc,
        AROS_UFHA(struct Hook *, hook, A0),
        AROS_UFHA(Object *, app, A2),
        AROS_UFHA(Object **, winp, A1))
{
    AROS_USERFUNC_INIT

    set(*winp, MUIA_Window_Open, FALSE);
    DoMethod(app, OM_REMMEMBER, *winp);
    MUI_DisposeObject(*winp);

    AROS_USERFUNC_EXIT
}

struct Hook CloseWinHook = {{NULL, NULL}, UFHN(CloseWinFunc) };

IPTR fwNew(Class *cl, Object *o, struct opSet *msg)
{
    struct opSet method;
    struct TagItem tags[4];
    STRPTR filename = (STRPTR)GetTagData(MUIA_FontWindow_Filename, (IPTR) NULL, msg->ops_AttrList);
    FT_Face face;
    FT_Error error;
    Object *install, *close, *info, *app;

    if (filename == NULL)
    {
        DEBUG_FONTWINDOW(dprintf("FontWindow: no filename.\n"));
        return 0;
    }

    error = FT_New_Face(ftlibrary, filename, 0, &face);
    if (error)
    {
        DEBUG_FONTWINDOW(dprintf("FontWindow: New_Face error %d.\n", error));
        return 0;
    }

    app = (Object *)GetTagData(MUIA_UserData, 0,
                   msg->ops_AttrList);
    if (NULL == app)
    {
        DEBUG_FONTWINDOW(dprintf("FontWindow: no app ptr.\n"));
        return 0;
    }

    tags[0].ti_Tag = MUIA_Window_ID;
    tags[0].ti_Data = MAKE_ID('F','O','N','T');
    tags[1].ti_Tag = MUIA_Window_Title;
    tags[1].ti_Data = (IPTR)filename;
    tags[2].ti_Tag = MUIA_Window_RootObject;
    tags[2].ti_Data = (IPTR)VGroup,
        Child, info = FontInfoObject,
            MUIA_FontInfo_Filename, filename,
            MUIA_FontInfo_Face, face,
            End,
        Child, HGroup,
            Child, install = SimpleButton(_(MSG_BUTTON_INSTALL)),
            Child, RectangleObject,
                End,
            Child, close = SimpleButton(_(MSG_BUTTON_CLOSE)),
            End,
        End;
    tags[3].ti_Tag = TAG_MORE;
    tags[3].ti_Data = (IPTR)msg->ops_AttrList;

    method.MethodID = OM_NEW;
    method.ops_AttrList = tags;
    method.ops_GInfo = NULL;

    o = (Object *)DoSuperMethodA(cl, o, (Msg)&method);
    if (o)
    {
        FontWindowData *dat = INST_DATA(cl, o);
        dat->Face = face;

        DoMethod(install, MUIM_Notify, MUIA_Pressed, FALSE,
                info, 1, MUIM_FontInfo_WriteFiles);
        DoMethod(close, MUIM_Notify, MUIA_Pressed, FALSE,
             app, 6, MUIM_Application_PushMethod, app, 3,
             MUIM_CallHook, &CloseWinHook, o);

    }
    else
    {
        FT_Done_Face(face);
    }

    DEBUG_FONTWINDOW(dprintf("FontWindow: created object 0x%lx.\n", o));

    return (IPTR)o;
}

IPTR fwDispose(Class *cl, Object *o)
{
    FontWindowData *dat = INST_DATA(cl, o);

    DEBUG_FONTWINDOW(dprintf("FontWindow: destroy object 0x%lx\n", o));

    FT_Done_Face(dat->Face);

    return DoSuperMethod(cl, o, OM_DISPOSE);
}

AROS_UFH3(ULONG, FontWindowDispatch,
        AROS_UFHA(Class *, cl, A0),
        AROS_UFHA(Object *, o, A2),
        AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    ULONG ret;

    switch (msg->MethodID)
    {
        case OM_NEW:
            ret = fwNew(cl, o, (struct opSet *)msg);
            break;

        case OM_DISPOSE:
            ret = fwDispose(cl, o);
            break;

        default:
            ret = DoSuperMethodA(cl, o, msg);
            break;
    }

    return ret;

    AROS_USERFUNC_EXIT
}


void CleanupFontWindowClass(void)
{
    if (FontWindowClass)
    {
        MUI_DeleteCustomClass(FontWindowClass);
        FontWindowClass = NULL;
    }
}

int InitFontWindowClass(void)
{
    FontWindowClass = MUI_CreateCustomClass(NULL, MUIC_Window, NULL,
            sizeof(FontWindowData), UFHN(FontWindowDispatch));
    return FontWindowClass != NULL;
}
