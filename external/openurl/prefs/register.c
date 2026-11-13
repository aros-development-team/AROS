/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include <proto/openurl.h>
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "openurl.h"

#define CATCOMP_NUMBERS
#include "locale.h"

#include <libraries/openurl.h>

#include "SDI_hook.h"
#include "macros.h"

#include "debug.h"

#include <aros/debug.h>

#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <stdio.h>
#include <string.h>

/*** Instance Data **********************************************************/

struct OpenURLRegister_DATA
{
    ULONG _unused;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct OpenURLRegister_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/

Object *OpenURLRegister__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *regObject;

    if (!message->ops_AttrList)
        return NULL;

    regObject = MUI_NewObjectA(MUIC_Register, message->ops_AttrList);
    if (!regObject)
        return NULL;

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
//        MUIA_PrefsEditor_Name, _(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/locale.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Locale",
        Child, regObject,
        TAG_DONE
    );
    return self;
}

IPTR OpenURLRegister__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[OpenURLRegister] %s()\n", __func__));

    return success;
}

IPTR OpenURLRegister__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[OpenURLRegister] %s()\n", __func__));

    return success;
}

IPTR OpenURLRegister__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[OpenURLRegister] %s()\n", __func__));

    return success;
}

IPTR OpenURLRegister__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    D(bug("[OpenURLRegister] %s()\n", __func__));

    return DoSuperMethodA(CLASS, self, message);
}

IPTR OpenURLRegister__MUIM_PrefsEditor_Save(Class *cl, Object *obj, Msg msg)
{
    IPTR res;

    D(bug("[OpenURLRegister] %s()\n", __func__));

    DoMethod(_win(obj), MUIM_Win_StorePrefs, MUIV_Win_StorePrefs_Save);

    res = DoSuperMethodA(cl, obj, msg);

    return res;
}

IPTR OpenURLRegister__MUIM_PrefsEditor_Use(Class *cl, Object *obj, Msg msg)
{
    D(bug("[OpenURLRegister] %s()\n", __func__));

    DoMethod(_win(obj), MUIM_Win_StorePrefs, MUIV_Win_StorePrefs_Use);

    if (!DoSuperMethodA(cl, obj, msg))
        return FALSE;

    return TRUE;
}

#define ZUNE_CUSTOMCLASS_OPENURLREGISTER(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                        \
                           m2, m2_msg_type,                        \
                           m3, m3_msg_type,                        \
                           m4, m4_msg_type,                        \
                           m5, m5_msg_type,                        \
                           m6, m6_msg_type,                        \
                           m7, m7_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);


/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_OPENURLREGISTER
(
    OpenURLRegister, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    OM_DISPOSE,                   Msg,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg,
    MUIM_PrefsEditor_Save,        Msg,
    MUIM_PrefsEditor_Use,         Msg
)
        default:
            return DoSuperMethodA(__class, __self, __msg);
    }

    return (IPTR) NULL;
}
BOOPSI_DISPATCHER_END

struct MUI_CustomClass * OpenURLRegister_CLASS;

BOOL initRegClass(void)
{
    OpenURLRegister_CLASS = MUI_CreateCustomClass
    (
        NULL, MUIC_PrefsEditor, NULL,
        sizeof(struct OpenURLRegister_DATA), (APTR) OpenURLRegister_Dispatcher
    );

    if (!OpenURLRegister_CLASS)
    {
        __showerror
        ( (char *)
            "Failed to create Zune custom class `OpenURLRegister '.", NULL
        );

        return FALSE;
    }

    return TRUE;
}

void disposeRegClass(void)
{
    MUI_DeleteCustomClass(OpenURLRegister_CLASS);
}
