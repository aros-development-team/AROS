/*
**  OpenURL - MUI preferences for openurl.library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**
**  Pens adjust window
*/


#include "OpenURL.h"
#define CATCOMP_NUMBERS
#include "loc.h"

/**************************************************************************/
/*
**  A Poppen that Impot/Export its spec
*/

static struct MUI_CustomClass *penClass = NULL;
#define penObject NewObject(penClass->mcc_Class,NULL

struct penData
{
    struct MUI_PenSpec spec; /* Needed to check changes */
};

/**************************************************************************/
/*
** Import pen spec
*/

static ULONG
mPenImport(struct IClass *cl,Object *obj,struct MUIP_Import *msg)
{
    register ULONG id;

    if (id = (muiNotifyData(obj)->mnd_ObjectID))
    {
        struct penData     *data = INST_DATA(cl,obj);
        struct MUI_PenSpec *spec;

        if (spec = (struct MUI_PenSpec *)DoMethod(msg->dataspace,MUIM_Dataspace_Find,id))
        {
            strcpy((STRPTR)&data->spec,(STRPTR)spec);
            set(obj,MUIA_Pendisplay_Spec,&data->spec);
        }
    }

    return 0;
}

/**************************************************************************/
/*
** Export pen spec
*/

static ULONG
mPenExport(struct IClass *cl,Object *obj,struct MUIP_Export *msg)
{
    register ULONG id;

    if (id = (muiNotifyData(obj)->mnd_ObjectID))
    {
        struct MUI_PenSpec *spec;

        get(obj,MUIA_Pendisplay_Spec,&spec);
        DoMethod(msg->dataspace,MUIM_Dataspace_Add,(ULONG)spec,sizeof(struct MUI_PenSpec),id);
    }

    return 0;
}

/**************************************************************************/
/*
** Check if pen spec changed
*/

static ULONG
mPenCheckSave(struct IClass *cl,Object *obj,Msg msg)
{
    struct penData     *data = INST_DATA(cl,obj);
    struct MUI_PenSpec *spec;

    get(obj,MUIA_Pendisplay_Spec,&spec);

    return (ULONG)strcmp((STRPTR)spec,(STRPTR)&data->spec);
}

/**************************************************************************/

M_DISP(penDispatcher)
{
    M_DISPSTART

    switch(msg->MethodID)
    {
        case MUIM_Import:        return mPenImport(cl,obj,(APTR)msg);
        case MUIM_Export:        return mPenExport(cl,obj,(APTR)msg);

        case MUIM_App_CheckSave: return mPenCheckSave(cl,obj,(APTR)msg);

        default:                 return DoSuperMethodA(cl,obj,msg);
    }
}

M_DISPEND(penDispatcher)

/***********************************************************************/

static ULONG
initPenClass(void)
{
    return (ULONG)(penClass = MUI_CreateCustomClass(NULL,MUIC_Poppen,NULL,sizeof(struct penData),DISP(penDispatcher)));
}

/**************************************************************************/

static void
disposePenClass(void)
{
    if (penClass) MUI_DeleteCustomClass(penClass);
}

/**************************************************************************/

struct data
{

    Object             *enabled;
    Object             *disabled;
    Object             *detail;

    struct MUI_PenSpec *specs[3];

    ULONG              flags;
};

static ULONG
mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    Object *enabled, *disabled, *detail;

    if (obj = (Object *)DoSuperNew(cl,obj,
        MUIA_HelpNode,           "PWIN",
        MUIA_Window_ID,          MAKE_ID('P', 'W', 'I', 'N'),
        MUIA_Window_Title,       getString(MSG_Pens_WinTitle),
        MUIA_Window_ScreenTitle, getString(MSG_App_ScreenTitle),

        WindowContents, VGroup,
            Child, ColGroup(6),

                Child, oflabel(MSG_Pens_EnabledPen),
                Child, enabled = penObject,
                    MUIA_ObjectID,        MAKE_ID('E','P', 'E', 'N'),
                    MUIA_ShortHelp,       getString(MSG_Pens_EnabledPen_Help),
                    MUIA_ControlChar,     getKeyChar(NULL,MSG_Pens_EnabledPen),
                    MUIA_Window_Title,    (ULONG)getString(MSG_Pens_EnabledPen_WinTitle),
                    MUIA_Draggable,       TRUE,
                    MUIA_CycleChain,      TRUE,
                    MUIA_Pendisplay_Spec, DEF_EnabledPen,
                End,

                Child, oflabel(MSG_Pens_DisabledPen),
                Child, disabled = penObject,
                    MUIA_ObjectID,        MAKE_ID('D','P', 'E', 'N'),
                    MUIA_ShortHelp,       getString(MSG_Pens_DisabledPen_Help),
                    MUIA_ControlChar,     getKeyChar(NULL,MSG_Pens_DisabledPen),
                    MUIA_Window_Title,    (ULONG)getString(MSG_Pens_DisabledPen_WinTitle),
                    MUIA_Draggable,       TRUE,
                    MUIA_CycleChain,      TRUE,
                    MUIA_Pendisplay_Spec, DEF_DisabledPen,
                End,

                Child, oflabel(MSG_Pens_DetailPen),
                Child, detail = penObject,
                    MUIA_ObjectID,        MAKE_ID('I','P', 'E', 'N'),
                    MUIA_ShortHelp,       getString(MSG_Pens_DetailPen_Help),
                    MUIA_ControlChar,     getKeyChar(NULL,MSG_Pens_DetailPen),
                    MUIA_Window_Title,    (ULONG)getString(MSG_Pens_DetailPen_WinTitle),
                    MUIA_Draggable,       TRUE,
                    MUIA_CycleChain,      TRUE,
                    MUIA_Pendisplay_Spec, DEF_DetailPen,
                End,
            End,
        End,
        TAG_MORE, msg->ops_AttrList))
    {
        struct data *data = INST_DATA(cl,obj);

        data->enabled  = enabled;
        data->disabled = disabled;
        data->detail   = detail;

        DoMethod(enabled,MUIM_Notify,MUIA_Pendisplay_Spec,MUIV_EveryTime,(ULONG)obj,1,MUIM_Pens_Change);
        DoMethod(disabled,MUIM_Notify,MUIA_Pendisplay_Spec,MUIV_EveryTime,(ULONG)obj,1,MUIM_Pens_Change);
        DoMethod(detail,MUIM_Notify,MUIA_Pendisplay_Spec,MUIV_EveryTime,(ULONG)obj,1,MUIM_Pens_Change);
    }

    return (ULONG)obj;
}

/**************************************************************************/
/*
** Called when a pen spec is changed by the user
*/

static ULONG
mChange(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);
    Object      *app;

    get(data->enabled,MUIA_Pendisplay_Spec,data->specs);
    get(data->disabled,MUIA_Pendisplay_Spec,data->specs+1);
    get(data->detail,MUIA_Pendisplay_Spec,data->specs+2);

    /* To App -> Main win -> Applist -> list -> lamp */
    get(obj,MUIA_ApplicationObject,&app);
    set(app,MUIA_App_Pens,data->specs);

    return 0;
}

/***********************************************************************/

static ULONG
mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
    struct data *data = INST_DATA(cl,obj);

    switch (msg->opg_AttrID)
    {
        case MUIA_App_Pens:
        {
            get(data->enabled, MUIA_Pendisplay_Spec,data->specs);
            get(data->disabled,MUIA_Pendisplay_Spec,data->specs+1);
            get(data->detail,  MUIA_Pendisplay_Spec,data->specs+2);

            *msg->opg_Storage = (ULONG)&data->specs; return TRUE;
        }

        default: return DoSuperMethodA(cl,obj,(Msg)msg);
    }
}

/**************************************************************************/
/*
** Check if pen spec changed
*/

static ULONG
mCheckSave(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

    return (ULONG)(DoMethodA(data->enabled,msg) ||
    			   DoMethodA(data->disabled,msg) ||
                   DoMethodA(data->detail,msg));
}

/**************************************************************************/

M_DISP(dispatcher)
{
    M_DISPSTART

    switch (msg->MethodID)
    {
        case OM_NEW:              return mNew(cl,obj,(APTR)msg);
        case OM_GET:              return mGet(cl,obj,(APTR)msg);

        case MUIM_Pens_Change:    return mChange(cl,obj,(APTR)msg);
        case MUIM_App_CheckSave:  return mCheckSave(cl,obj,(APTR)msg);

        default:                  return DoSuperMethodA(cl,obj,msg);
    }
}

M_DISPEND(dispatcher)

/**************************************************************************/

ULONG
initPensClass(void)
{
    if (initPenClass())
    {
        if (g_pensClass = MUI_CreateCustomClass(NULL,MUIC_Window,NULL,sizeof(struct data),DISP(dispatcher)))
        {
            return TRUE;
        }

        disposePenClass();
    }

    return FALSE;
}

/**************************************************************************/

void
disposePensClass(void)
{
    disposePenClass();
    if (g_pensClass) MUI_DeleteCustomClass(g_pensClass);
}

/**************************************************************************/

