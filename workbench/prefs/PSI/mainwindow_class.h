#ifndef MAINWINDOW_CLASS_H
#define MAINWINDOW_CLASS_H

/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include "psi.h"

#define MUIM_MainWindow_Finish  (TAGBASE_STUNTZI | 0x1070)
#define MUIM_MainWindow_About   (TAGBASE_STUNTZI | 0x1071)
#define MUIM_MainWindow_Restore (TAGBASE_STUNTZI | 0x1072)
#define MUIM_MainWindow_Open    (TAGBASE_STUNTZI | 0x1073)
#define MUIM_MainWindow_SaveAs  (TAGBASE_STUNTZI | 0x1074)

struct MUIP_MainWindow_Finish  { STACKED ULONG MethodID; STACKED LONG level; };
struct MUIP_MainWindow_Restore { STACKED ULONG MethodID; STACKED LONG envarc; };
struct MUIP_MainWindow_Open    { STACKED ULONG MethodID; STACKED LONG append; };

struct MUI_CustomClass *CL_MainWindow;

VOID MainWindow_Init(VOID);
VOID MainWindow_Exit(VOID);

extern struct NewMenu MainMenu[];

#endif
