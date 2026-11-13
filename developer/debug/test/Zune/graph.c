/*
    Copyright (C) 2022, The AROS Development Team.
*/

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <dos/dos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#include <libraries/mui.h>
#include <zune/graph.h>

#define DEBUG 0
#include <aros/debug.h>

Object *app;

struct graphPrivateData
{
    ULONG   xceiling;
    ULONG   xinterv;
    ULONG   yceiling;
};

AROS_UFH3(IPTR, GraphUpdateFunc,
        AROS_UFHA(struct Hook *, procHook, A0),
        AROS_UFHA(IPTR *, storage, A2),
        AROS_UFHA(struct graphPrivateData *, data, A1))
{
    AROS_USERFUNC_INIT

    static ULONG count = 0;
    ULONG phase = (((count++) * data->xinterv) % data->xceiling);
    double result;

#define M_TWOPI 6.283185307179586476925286766559
    result = sin( (double)phase / (double)data->xceiling * M_TWOPI );
    *storage = (data->yceiling / 2) + (int)floor(result * ((double)data->yceiling / 2));

    return TRUE;

    AROS_USERFUNC_EXIT
}

int main(void)
{
    struct graphPrivateData data;
    struct Hook graphReadHook;
    Object *wnd, *graph;

    data.xceiling = 100000;
    data.xinterv    = 1000;
    data.yceiling = 1000;

    app = ApplicationObject,
        SubWindow, wnd = WindowObject,
            MUIA_Window_Activate, TRUE,
            MUIA_Window_Title,    (IPTR)"Zune Graph",

            WindowContents, VGroup,
                MUIA_Background, (IPTR)"7:h,8d8d8d8d,b5b5b5b5,babababa-80808080,96969696,99999999",
                Child, (graph = GraphObject,
                    MUIA_Graph_InfoText,        (IPTR)"Test Graph",
                    MUIA_Graph_ValueCeiling,    data.yceiling,
                    MUIA_Graph_ValueStep,       100,
                    MUIA_Graph_PeriodCeiling,   data.xceiling,
                    MUIA_Graph_PeriodStep,      data.xinterv * 10,
                    MUIA_Graph_PeriodInterval,  data.xinterv,
                End),
            End,
        End,
    End;

    if (app)
    {
        ULONG sigs = 0;
        APTR graphDataSource = (APTR)DoMethod(graph, MUIM_Graph_GetSourceHandle, 0);
        graphReadHook.h_Entry = (APTR)GraphUpdateFunc;
        graphReadHook.h_Data = (APTR)&data;
        DoMethod(graph, MUIM_Graph_SetSourceAttrib, graphDataSource, MUIV_Graph_Source_ReadHook, &graphReadHook);

        DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2,
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        set(wnd, MUIA_Window_Open, TRUE);

        while((LONG) DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)
              != MUIV_Application_ReturnID_Quit)
        {
            if (sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
                if (sigs & SIGBREAKF_CTRL_C) break;
                if (sigs & SIGBREAKF_CTRL_D) break;
            }
        }
        set(wnd, MUIA_Window_Open, FALSE);
        MUI_DisposeObject(app);
    }
    
    return 0;
}

