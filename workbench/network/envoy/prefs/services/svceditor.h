#ifndef SVCPREFS_SVCEDITOR_H
#define SVCPREFS_SVCEDITOR_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <libraries/mui.h>

/* not TAG_USER | 0x1000000x: the PrefsEditor class uses that range for its own private
   methods (one of them loads the settings when the window is set up) */
#define MUIB_SvcEditor                  (TAG_USER | 0x2EF60000)
#define MUIM_SvcEditor_Select           (MUIB_SvcEditor | 1)   /* list selection changed     */
#define MUIM_SvcEditor_Update           (MUIB_SvcEditor | 2)   /* an entry field was edited  */
#define MUIM_SvcEditor_Add              (MUIB_SvcEditor | 3)
#define MUIM_SvcEditor_Remove           (MUIB_SvcEditor | 4)
#define MUIM_SvcEditor_AutoRun          (MUIB_SvcEditor | 5)

extern struct MUI_CustomClass *SvcEditor_CLASS;

#define SvcEditorObject BOOPSIOBJMACRO_START(SvcEditor_CLASS->mcc_Class)

#endif
