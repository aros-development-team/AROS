#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/commodities.h>
#include <libraries/locale.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/commodities.h>
#include <proto/asl.h>
#include <proto/icon.h>

#include <clib/alib_protos.h>

#include <libraries/mui.h>

#if defined(__AROS__)
#include <aros/debug.h>
#include <proto/task.h>
#include <resources/task.h>
#endif

#include <stdio.h>
#include <string.h>
#ifdef __GNUC__
#include "../shared_defs.h"
#else
#include "/shared_defs.h"
#endif
#include "protos.h"
#include "locale.h"

struct TaskEntry
  {
  struct MinNode te_Node;
  char *te_TaskName;
  struct TaskEntryInFile te_teif;
  };

GLOBAL Object *Application,
              *DiskList,
              *WI_Window,
              *WI_TaskSelect,
              *LV_TaskSelect,
              *BT_Save,
              *BT_Use,
              *BT_Cancel;


/* Objects in the memory page */
GLOBAL Object *GR_Memory,
              *GR_File,
              *GR_DevName,
              *ST_PartName,
              *ST_FileName,
              *PO_Disks,
              *LV_Disks,
              *CY_Policy,
              *CY_Device,
              *CY_MemFlags,
              *TX_MinMem,
              *SL_MinMem,
              *TX_MaxMem,
              *SL_MaxMem,
              *TX_FileSize,
              *SL_FileSize,
              *TX_Buffer,
              *SL_Buffer,
              *TX_Prio,
              *SL_Prio;

/* Objects in the task page */
GLOBAL Object *LV_TaskList,
              *ST_CurrentTask,
              *GR_OrderButtons,
              *GR_TaskButtons,
              *GR_TaskList,
              *GR_TaskSub,
              *GR_Tasks,
              *GR_VMPermissions,
              *CY_DataOptions,
              *CY_CodeOptions,
              *ST_MinPublic,
              *ST_MinNonPublic,
              *BT_AddProg,
              *BT_Top,
              *BT_AddTask,
              *BT_AddDir,
              *BT_Up,
              *BT_Add,
              *BT_Down,
              *BT_Delete,
              *BT_Bottom,
              *BT_Sort;

/* Objects in the misc page */
GLOBAL Object *GR_Misc;
GLOBAL Object *GR_StatParams,
              *GR_Statistics,
              *CM_StatEnabled,
              *CM_Zoomed,
              *ST_UnZTop,
              *ST_UnZLeft,
              *ST_ZTop,
              *ST_ZLeft,
              *CM_CacheZ2RAM,
              *CM_WBPatch,
              *CM_MemTracking,
              *CM_FastROM,
              *ST_MinVMAlloc,
              *ST_EnableHotkey,
              *ST_DisableHotkey;

GLOBAL struct Window *VMMWindow;

#ifdef DO_INIT
GLOBAL struct Library *MUIMasterBase = NULL;
GLOBAL struct UtilityBase *UtilityBase = NULL;
GLOBAL struct Library *IconBase = NULL;
GLOBAL struct Library *CxBase = NULL;
GLOBAL struct Library *AslBase = NULL;
GLOBAL struct Library *LocaleBase = NULL;
GLOBAL struct IntuitionBase *IntuitionBase = NULL;
#else
GLOBAL struct Library *MUIMasterBase;
GLOBAL struct UtilityBase *UtilityBase;
GLOBAL struct Library *IconBase;
GLOBAL struct Library *CxBase;
GLOBAL struct Library *AslBase;
GLOBAL struct Library *LocaleBase;
GLOBAL struct IntuitionBase *IntuitionBase;
#endif

extern struct ExecBase *SysBase;

enum
  {
  /* Termination buttons */
  SAVE_ID = 1,
  USE_ID,
  CANCEL_ID,

  /* Menu choices */
  OPEN_ID,
  SAVE_AS_ID,
  SAVE_WIN_ID,
  ABOUT_ID,
  HIDE_ID,
  RESET_ID,
  LAST_SAVED_ID,
  RESTORE_ID,

  /* Other buttons */
  POP_ID,
  ADDPROG_ID,
  ADDTASK_ID,
  ADD_ID,
  ADDDIR_ID,
  DELETE_ID,
  TOP_ID,
  UP_ID,
  DOWN_ID,
  BOTTOM_ID,
  SORT_ID,

  /* Cycle and string gadgets for task params */
  CODE_PG_ID,
  DATA_PG_ID,
  MINPUBLIC_ID,
  MINNONPUBLIC_ID,
  TASKNAME_ID,

  MEMFLAGS_ID
  };

#define FIRST_MENU_ID OPEN_ID
#define LAST_MENU_ID  RESTORE_ID

/* MemType defines */
#define MT_FIXED      0
#define MT_DYNAMIC    1
#define MT_RESTRICTED 2

/* MemFlags defines */
#define MF_ANY  0
#define MF_CHIP 1
#define MF_FAST 2

/* Quit codes */
#define Q_DONTQUIT 0
#define Q_QUITGUI  1
#define Q_QUITBOTH 2

#define CFG_NAME_SAVE ("ENVARC:" CFG_FILEBASE)
#define CFG_NAME_USE  ("ENV:" CFG_FILEBASE)

GLOBAL BOOL ShowGUI;
GLOBAL BOOL ForceOverwrite;
GLOBAL UWORD CXPri;
GLOBAL char *CXPopKey;
GLOBAL char *CfgName;
GLOBAL char  EnableVMHotkey [200];
GLOBAL char  DisableVMHotkey [200];

GLOBAL struct CxParams *CxParams;
GLOBAL struct ExtPort *ExtCxPort;

#define DEFAULT_POPKEY     "ralt rshift v"

#define VMM_RUNNING (FindPort (VMPORTNAME) != NULL)
