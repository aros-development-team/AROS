#ifndef EFSEDITOR_H
#define EFSEDITOR_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <libraries/mui.h>

/* own range: PrefsEditor uses TAG_USER | 0x1000000x for private methods */
#define MUIB_EfsEditor                  (TAG_USER | 0x2EF50000)
#define MUIM_EfsEditor_Select           (MUIB_EfsEditor | 1)
#define MUIM_EfsEditor_Store            (MUIB_EfsEditor | 2)
#define MUIM_EfsEditor_Add              (MUIB_EfsEditor | 3)
#define MUIM_EfsEditor_Remove           (MUIB_EfsEditor | 4)
#define MUIM_EfsEditor_AddAccess        (MUIB_EfsEditor | 5)
#define MUIM_EfsEditor_RemoveAccess     (MUIB_EfsEditor | 6)

extern struct MUI_CustomClass *EfsEditor_CLASS;
#define EfsEditorObject BOOPSIOBJMACRO_START(EfsEditor_CLASS->mcc_Class)

extern struct Library *AccountsBase;
extern struct Library *EnvoyBase;

#endif
