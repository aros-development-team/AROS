/*
    Copyright  2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

// #define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/mui.h>
#include <prefs/input.h>

#include <zune/systemprefswindow.h>

#include "locale.h"
#include "args.h"
#include "ipeditor.h"
#include "prefs.h"

#include <aros/debug.h>

#define VERSION "$VER: Input 0.1 ("ADATE") AROS Dev Team"

extern  struct List             keymap_list;
extern  struct InputPrefs       inputprefs;
extern  struct InputPrefs       restore_prefs;
extern  IPTR                    mempool;
extern  struct MsgPort         *InputMP;
extern  struct timerequest     *InputIO;
        struct MUI_CustomClass *StringifyClass;

/*********************************************************************************************/

static BOOL OpenInputDev(void)
{
    if ((InputMP = CreateMsgPort()))
    {
        if ((InputIO = (struct timerequest *) CreateIORequest(InputMP, sizeof(struct IOStdReq))))
        {
            OpenDevice("input.device", 0, (struct IORequest *)InputIO, 0);
            return TRUE;
        }
    }
    return FALSE;
}

/*********************************************************************************************/

static void CloseInputDev(void)
{
    if (InputIO)
    {
        CloseDevice((struct IORequest *)InputIO);
        DeleteIORequest((struct IORequest *)InputIO);
    }
    
    if (InputMP)
    {
        DeleteMsgPort(InputMP);
    }
}

/*********************************************************************************************/

struct StringifyData
{
    UWORD   Type;
    char    buf[16];
};

AROS_UFH3S(IPTR, StringifyDispatcher,
           AROS_UFHA(Class  *, cl,  A0),
           AROS_UFHA(Object *, obj, A2),
           AROS_UFHA(Msg     , msg, A1))
{
    AROS_USERFUNC_INIT

    if (msg->MethodID==OM_NEW)
    {
        obj = (Object*) DoSuperMethodA(cl,obj,msg);
        if (obj != NULL)
        {
            struct StringifyData *data = INST_DATA(cl,obj);
            data->Type = (UWORD) GetTagData(MUIA_MyStringifyType, 0, ((struct opSet *)msg)->ops_AttrList);
        }
        return (IPTR) obj;
    }
    else if (msg->MethodID==MUIM_Numeric_Stringify)
    {
        struct StringifyData *data = INST_DATA(cl,obj);

        struct MUIP_Numeric_Stringify *m = (APTR)msg;
        
        if (data->Type == STRINGIFY_RepeatRate)
        {
            sprintf((char *)data->buf,"%3.2fs", 0.02*(12-m->value));
        }
        else if (data->Type == STRINGIFY_RepeatDelay)
        {
            sprintf((char *)data->buf,"%ldms", 20+20*m->value);
        }
        else if (data->Type == STRINGIFY_DoubleClickDelay)
        {
            sprintf((char *)data->buf,"%3.2fs", 0.02 + 0.02 * m->value);
        }
        return((IPTR) data->buf);
    }

    return (IPTR) DoSuperMethodA(cl,obj,msg);
       
    AROS_USERFUNC_EXIT
}

int main(void)
{
    Object *application,  *window;

    Locale_Initialize();
    
    if (ReadArguments())
    {
        /* FIXME: handle arguments... */
        
        // FROM - import prefs from this file at start
        // USE  - 'use' the loaded prefs immediately, don't open window.
        // SAVE - 'save' the lodaed prefs immediately, don't open window.
        
        FreeArguments();
    }

    if (!OpenInputDev()) return 0;

    DefaultPrefs();
    CopyPrefs(&inputprefs, &restore_prefs);

    NewList(&keymap_list);

    mempool = (IPTR) CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 2048, 2048);

    if (mempool != 0)
    {
        ScanDirectory("DEVS:Keymaps/#?_~(#?.info)", &keymap_list, sizeof(struct KeymapEntry));

        StringifyClass = (struct MUI_CustomClass *) MUI_CreateCustomClass(NULL, MUIC_Slider, NULL, sizeof(struct StringifyData), StringifyDispatcher);
        if (StringifyClass != NULL)
        {
            application = ApplicationObject,
                MUIA_Application_Title,  __(MSG_NAME),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description,  __(MSG_DESCRIPTION),
                MUIA_Application_Base, (IPTR) "INPUTPREF",
                SubWindow, (IPTR) (window = SystemPrefsWindowObject,
                    MUIA_Window_ID, MAKE_ID('I','W','I','N'),
                    WindowContents, (IPTR) IPEditorObject,
                    TAG_DONE),
                End),
            End;
    
            if (application != NULL)
            {
                SET(window, MUIA_Window_Open, TRUE);
                DoMethod(application, MUIM_Application_Execute);
                SET(window, MUIA_Window_Open, FALSE);

                MUI_DisposeObject(application);
            }
            MUI_DeleteCustomClass(StringifyClass);
        }

        DeletePool((APTR)mempool);
    }

    kbd_cleanup();

    CloseInputDev();

    Locale_Deinitialize();

    return 0;
}
