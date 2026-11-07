/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Dragging entries out of DragType_Immediate lists: single-select,
    multi-select and a sortable list. A double-click is shown in the
    text line and must not start a drag.
*/

#include <exec/types.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#include <stdio.h>

static CONST_STRPTR entries[] =
{
    "Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot",
    "Golf", "Hotel", "India", "Juliett", "Kilo", "Lima", NULL
};

static Object *MakeList(ULONG multiselect, BOOL sortable)
{
    return ListviewObject,
        MUIA_Listview_DragType, MUIV_Listview_DragType_Immediate,
        MUIA_Listview_MultiSelect, multiselect,
        MUIA_Listview_List, ListObject,
            InputListFrame,
            MUIA_List_SourceArray, entries,
            MUIA_List_DragSortable, sortable,
        End,
    End;
}

int main(void)
{
    Object *app, *win, *lists[3], *status;
    static CONST_STRPTR titles[] = { "Single", "Multi", "Shifted, sortable" };
    char buf[64];
    ULONG sigs = 0;
    LONG id, i;

    app = ApplicationObject,
        MUIA_Application_Title, "ListDrag",
        SubWindow, win = WindowObject,
            MUIA_Window_Title, "List drag test",
            WindowContents, VGroup,
                Child, HGroup,
                    Child, VGroup,
                        Child, CLabel(titles[0]),
                        Child, lists[0] = MakeList(MUIV_Listview_MultiSelect_None, FALSE),
                    End,
                    Child, VGroup,
                        Child, CLabel(titles[1]),
                        Child, lists[1] = MakeList(MUIV_Listview_MultiSelect_Default, FALSE),
                    End,
                    Child, VGroup,
                        Child, CLabel(titles[2]),
                        Child, lists[2] = MakeList(MUIV_Listview_MultiSelect_Shifted, TRUE),
                    End,
                End,
                Child, status = TextObject, TextFrame, MUIA_Text_Contents, "", End,
            End,
        End,
    End;

    if (app == NULL)
    {
        printf("Could not create the application\n");
        return 20;
    }

    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2,
        MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    for (i = 0; i < 3; i++)
        DoMethod(lists[i], MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, app, 2,
            MUIM_Application_ReturnID, i + 1);

    set(win, MUIA_Window_Open, TRUE);

    while ((id = DoMethod(app, MUIM_Application_NewInput, &sigs))
        != MUIV_Application_ReturnID_Quit)
    {
        if (id >= 1 && id <= 3)
        {
            snprintf(buf, sizeof(buf), "Double-click in %s, entry %ld",
                titles[id - 1], (long)XGET(lists[id - 1], MUIA_List_Active));
            set(status, MUIA_Text_Contents, buf);
        }

        if (sigs)
        {
            sigs = Wait(sigs | SIGBREAKF_CTRL_C);
            if (sigs & SIGBREAKF_CTRL_C)
                break;
        }
    }

    MUI_DisposeObject(app);

    return 0;
}
