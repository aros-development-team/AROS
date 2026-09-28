/*
    Copyright (C) 2026, The AROS Development Team.
    All rights reserved.
*/

#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/muimaster.h>

#include <zune/prefswindow.h>

#include "zunestuff.h"

struct MUI_CustomClass *ClassPrefsWindow_CLASS;

BOOPSI_DISPATCHER(IPTR, ClassPrefsWindow_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case MUIM_PrefsWindow_Test:
        main_test_pressed();
        return 0;

    case MUIM_PrefsWindow_Revert:
        main_revert_pressed();
        return 0;

    case MUIM_PrefsWindow_Save:
        main_save_pressed();
        return 0;

    case MUIM_PrefsWindow_Use:
        main_use_pressed();
        return 0;

    case MUIM_PrefsWindow_Cancel:
        main_cancel_pressed();
        return 0;

    default:
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END


struct MUI_CustomClass *create_prefswindow_class(void)
{
    return MUI_CreateCustomClass(NULL, MUIC_PrefsWindow, NULL, 0, ClassPrefsWindow_Dispatcher);
}

void delete_prefswindow_class(void)
{
    if (ClassPrefsWindow_CLASS)
        MUI_DeleteCustomClass(ClassPrefsWindow_CLASS);
}
