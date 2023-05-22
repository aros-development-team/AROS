#ifndef IA_STAGE_H
#define IA_STAGE_H

#include "ia_install.h"

/* ************************************************
        Installer Stage Class Methods/Attribs
 * ************************************************/

#define MUIM_IC_NextStep                        (MUIM_IS_BASE + 0x1)
#define MUIM_IC_PrevStep                        (MUIM_IS_BASE + 0x2)
#define MUIM_IC_UndoSteps                       (MUIM_IS_BASE + 0x3)

#define MUIM_IC_Install                         (MUIM_IS_BASE + 0x4)
#define MUIM_IC_SetLocalePrefs                  (MUIM_IS_BASE + 0x5)

#define MUIM_PartitionFree                      (MUIM_IS_BASE + 0x7)
#define MUIM_Partition                          (MUIM_IS_BASE + 0x8)
#define MUIM_Format                             (MUIM_IS_BASE + 0x9)

#define MUIM_IC_CopyFiles                       (MUIM_IS_BASE + 0xa)
#define MUIM_IC_CopyFile                        (MUIM_IS_BASE + 0xb)

#define MUIM_IC_CancelInstall                   (MUIM_IS_BASE + 0x1a)
#define MUIM_IC_ContinueInstall                 (MUIM_IS_BASE + 0x1b)
#define MUIM_IC_QuitInstall                     (MUIM_IS_BASE + 0x1c)

#define MUIM_Reboot                             (MUIM_IS_BASE + 0x1d)
#define MUIM_RefreshWindow                      (MUIM_IS_BASE + 0x20)

/* to be made obsolete */

#define MUIA_Page                               (MUIA_IS_BASE + 0x0)

#define MUIA_PartitionButton                    (MUIA_IS_BASE + 0x1)
#define MUIA_Gauge1                             (MUIA_IS_BASE + 0x2)
#define MUIA_Gauge2                             (MUIA_IS_BASE + 0x3)
#define MUIA_Install                            (MUIA_IS_BASE + 0x4)
/**/
#define MUIA_WelcomeMsg                         (MUIA_IS_BASE + 0xa)
#define MUIA_FinishedMsg                        (MUIA_IS_BASE + 0xb)

/* new - some/most will "vanish(tm)" */

#define MUIA_OBJ_Installer                      (MUIA_IS_BASE + 0xd)
#define MUIA_Grub_Options                       (MUIA_IS_BASE + 0xe)
#define MUIA_List_Options                       (MUIA_IS_BASE + 0xf)

#define MUIA_OBJ_Window                         (MUIA_IS_BASE + 0x10)
#define MUIA_OBJ_WindowContent                  (MUIA_IS_BASE + 0x11)

#define MUIA_OBJ_PageTitle                      (MUIA_IS_BASE + 0x12)
#define MUIA_OBJ_PageHeader                     (MUIA_IS_BASE + 0x13)
#define MUIA_OBJ_CActionStrng                   (MUIA_IS_BASE + 0x14)

#define MUIA_OBJ_Back                           (MUIA_IS_BASE + 0x15)
#define MUIA_OBJ_Proceed                        (MUIA_IS_BASE + 0x16)
#define MUIA_OBJ_Cancel                         (MUIA_IS_BASE + 0x17)

#define MUIA_IC_License_File                    (MUIA_IS_BASE + 0x20)
#define MUIA_IC_License_Mandatory	            (MUIA_IS_BASE + 0x21)

#define MUIA_IC_EnableUndo                      (MUIA_IS_BASE + 0x30)

/* Install Results */

#define MUIA_InstallComplete                    (MUIA_IS_BASE + 0xff)
#define MUIA_InstallFailed                      (MUIA_IS_BASE + 0xfe)

#define MUIV_Inst_Completed                     (0xff)
#define MUIV_Inst_InProgress                    (0x00)
#define MUIV_Inst_Cancelled                     (0x01)
#define MUIV_Inst_Failed                        (0x10)


#endif /* IA_STAGE_H */
