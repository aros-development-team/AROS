#include "defs.h"

#define isspace(c) (((c)==' ') || ((c) == '\t') || ((c) == '\n'))

static char rcsid [] = "$Id: config.c,v 3.6 95/12/16 19:02:56 Martin_Apel Rel $";

#define VERSION 3
#define REVISION 3

#define AT_LEAST_3_1 ((CurrentConfig.Version > 3) || \
                     (CurrentConfig.Version == 3 && CurrentConfig.Revision >= 1))
#define AT_LEAST_3_3 ((CurrentConfig.Version > 3) || \
                     (CurrentConfig.Version == 3 && CurrentConfig.Revision >= 3))

/************************************************************************/

static void AllowVM (BOOL Allowed)

{
struct MsgPort *VMPort;
struct VMMsg *AbleMsg;
LONG AckSignal;

if ((VMPort = FindPort (VMPORTNAME)) == NULL)
  return;

if ((AbleMsg = AllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  return;

if ((AckSignal = AllocSignal (-1L)) == -1)
  {
  FreeMem (AbleMsg, sizeof (struct VMMsg));
  return;
  }

AbleMsg->VMSender = FindTask (NULL);
AbleMsg->ReplySignal = AckSignal;
AbleMsg->VMCommand = Allowed ? VMCMD_EnableVM : VMCMD_DisableVM;

PutMsg (VMPort, (struct Message*) AbleMsg);
Wait (1L << AckSignal);

FreeMem (AbleMsg, sizeof (struct VMMsg));
FreeSignal (AckSignal);
}

/************************************************************************/

void ReadConfigFromVMM (void)

{
/* Tries to contact VMM to get the current configuration of VMM.
 * The task list is not requested, it is taken from the config file.
 */

struct MsgPort *VMPort;
struct VMMsg *CfgQuestion;
ULONG CfgSignal;
struct VMMConfig *CurrentConfig;
struct TaskEntry *DefaultEntry;
ULONG MemType;

  bug("[VMM:GUI] %s()\n", __func__);

if ((CurrentConfig = AllocMem (sizeof (struct VMMConfig), MEMF_PUBLIC)) != NULL)
  {
  if ((CfgQuestion = AllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) != NULL)
    {
    if ((CfgSignal = AllocSignal (-1L)) != -1)
      {
      CfgQuestion->VMCommand = VMCMD_AskConfig;
      CfgQuestion->VMSender  = FindTask (NULL);
      CfgQuestion->ReplySignal = CfgSignal;
      CfgQuestion->Config = CurrentConfig;

      Forbid ();
      if ((VMPort = FindPort (VMPORTNAME)) != NULL)
        {
        PutMsg (VMPort, (struct Message*)CfgQuestion);  
        Permit ();
        Wait (1L << CfgSignal);

        DoMethod (LV_TaskList, MUIM_List_GetEntry, 0L, &DefaultEntry);
        DefaultEntry->te_teif.MinPublic = CurrentConfig->DefaultMinPublic;
        DefaultEntry->te_teif.MinNonPublic = CurrentConfig->DefaultMinNonPublic;
        DefaultEntry->te_teif.DataPaging = CurrentConfig->DefaultDataPaging;
        DefaultEntry->te_teif.CodePaging = CurrentConfig->DefaultCodePaging;
        DefaultEntry->te_teif.IsDefault = TRUE;

        DoMethod (LV_TaskList, MUIM_List_Redraw, 0L);

        if (CurrentConfig->MinMem == CurrentConfig->MaxMem)
          MemType = MT_FIXED;
        else if (CurrentConfig->MaxMem == DYN_MAX)
          {
          MemType = MT_DYNAMIC;
          CurrentConfig->MinMem = CurrentConfig->MaxMem = 100 * 1024L;
          }
        else
          MemType = MT_RESTRICTED;

        set (CY_Policy, MUIA_Cycle_Active, MemType);

        set (CY_MemFlags, MUIA_Cycle_Active, CurrentConfig->MemFlags >> 1);
        set (SL_MinMem, MUIA_Slider_Max, MaxAvailMem (CurrentConfig->MemFlags) / (100 * 1024));
        set (SL_MaxMem, MUIA_Slider_Max, MaxAvailMem (CurrentConfig->MemFlags) / (100 * 1024));
        set (SL_MinMem, MUIA_Slider_Level, CurrentConfig->MinMem / (100 * 1024));
        set (SL_MaxMem, MUIA_Slider_Level, CurrentConfig->MaxMem / (100 * 1024));

        set (CM_StatEnabled, MUIA_Selected, CurrentConfig->StatEnabled);
        set (CM_Zoomed, MUIA_Selected, CurrentConfig->StatZoomed);
        set (ST_ZLeft, MUIA_String_Integer, CurrentConfig->ZLeftEdge);
        set (ST_ZTop, MUIA_String_Integer, CurrentConfig->ZTopEdge);
        set (ST_UnZLeft, MUIA_String_Integer, CurrentConfig->UnZLeftEdge);
        set (ST_UnZTop, MUIA_String_Integer, CurrentConfig->UnZTopEdge);

        if (CurrentConfig->PageDev == PD_PART)
          {
          char *tmp = CurrentConfig->PartOrFileName;
        
          *(tmp + strlen (tmp) - 1) = 0;
          set (ST_PartName, MUIA_String_Contents, CurrentConfig->PartOrFileName);
          set (ST_FileName, MUIA_String_Contents, NULL);
          set (SL_FileSize, MUIA_Slider_Level, 1);
          }
        else
          {
          set (ST_FileName, MUIA_String_Contents, CurrentConfig->PartOrFileName);
          set (SL_FileSize, MUIA_Slider_Level, CurrentConfig->FileSize);
          set (ST_PartName, MUIA_String_Contents, NULL);
          }

        set (CY_Device, MUIA_Cycle_Active, CurrentConfig->PageDev);
        set (SL_Prio, MUIA_Slider_Level, CurrentConfig->VMPriority);
#if defined(PLATFORM_HASZ2)
        set (CM_CacheZ2RAM, MUIA_Selected, CurrentConfig->CacheZ2RAM);
#endif
        set (CM_WBPatch, MUIA_Selected, CurrentConfig->PatchWB);
        set (ST_MinVMAlloc, MUIA_String_Integer, CurrentConfig->MinVMAlloc);
        set (ST_EnableHotkey, MUIA_String_Contents, CurrentConfig->EnableHotkey);
        set (ST_DisableHotkey, MUIA_String_Contents, CurrentConfig->DisableHotkey);
        set (SL_Buffer, MUIA_Slider_Level, CurrentConfig->WriteBuffer / (10 * 1024));
        set (CM_MemTracking, MUIA_Selected, CurrentConfig->MemTracking);
        }
      else
        Permit ();

      FreeSignal (CfgSignal);
      }
    FreeMem (CfgQuestion, sizeof (struct VMMsg));
    }
  FreeMem (CurrentConfig, sizeof (struct VMMConfig));
  }
  bug("[VMM:GUI] %s: done\n", __func__);
}

/************************************************************************/

static BOOL ReadOldTaskEntry (BPTR cfg_file, char *filename)

{
struct OldTaskEntryInFile old_te;
struct TaskEntry te;

  bug("[VMM:GUI] %s()\n", __func__);

if (Read (cfg_file, &old_te, sizeof (struct OldTaskEntryInFile)) != 
    sizeof (struct OldTaskEntryInFile))
  {
  MUI_Request (Application, 0L, 0L, NULL, "*_OK", _(msgCorruptCfgFile), filename);
  return (FALSE);
  }

te.te_TaskName = old_te.TaskName;
te.te_teif.MinPublic = old_te.MinPublic;
te.te_teif.MinNonPublic = old_te.MinNonPublic;
te.te_teif.DataPaging = old_te.DataPaging;
te.te_teif.CodePaging = old_te.CodePaging;
te.te_teif.IsDefault = old_te.IsDefault;
te.te_teif.NameLen = strlen (old_te.TaskName) + 1;

DoMethod (LV_TaskList, MUIM_List_InsertSingle, &te, MUIV_List_Insert_Bottom);
return (TRUE);
}

/************************************************************************/

static BOOL ReadNewTaskEntry (BPTR cfg_file, char *filename)

{
struct TaskEntry te;

  bug("[VMM:GUI] %s()\n", __func__);

if (Read (cfg_file, &(te.te_teif), sizeof (struct TaskEntryInFile)) != 
    sizeof (struct TaskEntryInFile))
  {
  MUI_Request (Application, 0L, 0L, NULL, "*_OK", _(msgCorruptCfgFile), filename);
  return (FALSE);
  }

if ((te.te_TaskName = AllocMem ((ULONG)te.te_teif.NameLen, MEMF_PUBLIC)) == NULL)
  {
  MUI_Request (Application, 0L, 0L, NULL, "*_OK", _(msgOutOfMem));
  return (FALSE);
  }

if (Read (cfg_file, te.te_TaskName, (ULONG)te.te_teif.NameLen) != te.te_teif.NameLen)
  {
  MUI_Request (Application, 0L, 0L, NULL, "*_OK", _(msgCorruptCfgFile), filename);
  FreeMem (te.te_TaskName, (ULONG)te.te_teif.NameLen);
  return (FALSE);
  }

DoMethod (LV_TaskList, MUIM_List_InsertSingle, &te, MUIV_List_Insert_Bottom);

FreeMem (te.te_TaskName, (ULONG)te.te_teif.NameLen);
return (TRUE);
}

/************************************************************************/

BOOL ReadConfigFileMUI (char *filename)

{
/* This function reads the config file and sets the MUI gadgets 
 * accordingly.
 */

BPTR cfg_file;
struct TaskEntry te;
static struct VMMConfig CurrentConfig;
ULONG MemType;
int i;
BOOL success;

AllowVM (FALSE);         /* So RAM will not allocate any VM */

if ((cfg_file = Open (filename, MODE_OLDFILE)) == NULL)
  {
  MUI_Request (Application, 0L, 0L, NULL, "*_OK", _(msgNoCfgFile), filename);
  AllowVM (TRUE);
  return (FALSE);
  }

if ((Read (cfg_file, &CurrentConfig, sizeof (struct VMMConfig)) != 
     sizeof (struct VMMConfig)) || (CurrentConfig.CfgMagic != CFG_MAGIC))
  {
  MUI_Request (Application, 0L, 0L, NULL, "*_OK", _(msgCorruptCfgFile), filename);
  Close (cfg_file);
  AllowVM (TRUE);
  return (FALSE);
  }

set (LV_TaskList, MUIA_List_Quiet, TRUE);
DoMethod (LV_TaskList, MUIM_List_Clear);

te.te_TaskName = (char *)_(msgDefault);
te.te_teif.MinPublic = CurrentConfig.DefaultMinPublic;
te.te_teif.MinNonPublic = CurrentConfig.DefaultMinNonPublic;
te.te_teif.DataPaging = CurrentConfig.DefaultDataPaging;
te.te_teif.CodePaging = CurrentConfig.DefaultCodePaging;
te.te_teif.IsDefault = TRUE;
te.te_teif.NameLen = strlen (te.te_TaskName) + 1;

DoMethod (LV_TaskList, MUIM_List_InsertSingle, &te, 0);
  bug("[VMM:GUI] %s: default te @ 0x%p added\n", __func__, &te);

if (CurrentConfig.MinMem == CurrentConfig.MaxMem)
  MemType = MT_FIXED;
else if (CurrentConfig.MaxMem == DYN_MAX)
  {
  MemType = MT_DYNAMIC;
  CurrentConfig.MinMem = CurrentConfig.MaxMem = 100 * 1024L;
  }
else
  MemType = MT_RESTRICTED;

set (CY_Policy, MUIA_Cycle_Active, MemType);
set (CY_MemFlags, MUIA_Cycle_Active, CurrentConfig.MemFlags >> 1);
set (SL_MinMem, MUIA_Slider_Max, MaxAvailMem (CurrentConfig.MemFlags) / (100 * 1024));
set (SL_MaxMem, MUIA_Slider_Max, MaxAvailMem (CurrentConfig.MemFlags) / (100 * 1024));
set (SL_MinMem, MUIA_Slider_Level, CurrentConfig.MinMem / (100 * 1024));
set (SL_MaxMem, MUIA_Slider_Level, CurrentConfig.MaxMem / (100 * 1024));

set (CM_StatEnabled, MUIA_Selected, CurrentConfig.StatEnabled);
set (CM_Zoomed, MUIA_Selected, CurrentConfig.StatZoomed);
set (ST_ZLeft, MUIA_String_Integer, CurrentConfig.ZLeftEdge);
set (ST_ZTop, MUIA_String_Integer, CurrentConfig.ZTopEdge);
set (ST_UnZLeft, MUIA_String_Integer, CurrentConfig.UnZLeftEdge);
set (ST_UnZTop, MUIA_String_Integer, CurrentConfig.UnZTopEdge);

if (CurrentConfig.PageDev == PD_PART)
  {
  char *tmp = CurrentConfig.PartOrFileName;

  *(tmp + strlen (tmp) - 1) = 0;
  set (ST_PartName, MUIA_String_Contents, CurrentConfig.PartOrFileName);
  set (ST_FileName, MUIA_String_Contents, NULL);
  set (SL_FileSize, MUIA_Slider_Level, 1);
  }
else
  {
  set (ST_FileName, MUIA_String_Contents, CurrentConfig.PartOrFileName);
  set (SL_FileSize, MUIA_Slider_Level, CurrentConfig.FileSize);
  set (ST_PartName, MUIA_String_Contents, NULL);
  }

set (CY_Device, MUIA_Cycle_Active, CurrentConfig.PageDev);
set (SL_Prio, MUIA_Slider_Level, CurrentConfig.VMPriority);
#if defined(PLATFORM_HASZ2)
set (CM_CacheZ2RAM, MUIA_Selected, CurrentConfig.CacheZ2RAM);
#endif
set (CM_WBPatch, MUIA_Selected, CurrentConfig.PatchWB);
set (ST_MinVMAlloc, MUIA_String_Integer, CurrentConfig.MinVMAlloc);
set (ST_EnableHotkey, MUIA_String_Contents, CurrentConfig.EnableHotkey);
set (ST_DisableHotkey, MUIA_String_Contents, CurrentConfig.DisableHotkey);
set (SL_Buffer, MUIA_Slider_Level, CurrentConfig.WriteBuffer / (10 * 1024));
set (CM_MemTracking, MUIA_Selected, CurrentConfig.MemTracking);
#if defined(PLATFORM_HASFASTROM)
if (AT_LEAST_3_1)
  set (CM_FastROM, MUIA_Selected, CurrentConfig.FastROM);
#endif
for (i = 0; i < CurrentConfig.NumTaskEntries; i++)
  {
  if (AT_LEAST_3_3)
    {
    if (!(success = ReadNewTaskEntry (cfg_file, filename)))
      break;
    }
  else
    {
    if (!(success = ReadOldTaskEntry (cfg_file, filename)))
      break;
    }
  }

set (LV_TaskList, MUIA_List_Active, 0);
set (LV_TaskList, MUIA_List_Quiet, FALSE);

Close (cfg_file);
AllowVM (TRUE);
  bug("[VMM:GUI] %s: Config Read\n", __func__);

return (success);
}

/***********************************************************************/

void ReadHotkeysFromConfig (char *filename)

{
/* This function reads only the hotkey lines from the config file.
 * This is needed in case of a direct start without opening the 
 * user interface.
 */

BPTR cfg_file;
struct VMMConfig CurrentConfig;

strcpy (EnableVMHotkey, DEFAULT_ENABLE_KEY);
strcpy (DisableVMHotkey, DEFAULT_DISABLE_KEY);

AllowVM (FALSE);         /* So RAM will not allocate any VM */

if ((cfg_file = Open (filename, MODE_OLDFILE)) != NULL)
  {
  if ((Read (cfg_file, &CurrentConfig, sizeof (struct VMMConfig)) ==
       sizeof (struct VMMConfig)) && (CurrentConfig.CfgMagic == CFG_MAGIC))
    {
    strcpy (EnableVMHotkey, CurrentConfig.EnableHotkey);
    strcpy (DisableVMHotkey, CurrentConfig.DisableHotkey);
    }

  }

Close (cfg_file);
AllowVM (TRUE);
}

/***********************************************************************/

BOOL WriteConfigFile (char *filename)

{
BPTR cfg_file;
struct TaskEntry *cur_task = NULL;
struct VMMConfig CurrentConfig;
IPTR Policy = 0;
char *tmp;
int i;
IPTR val;

AllowVM (FALSE);

if ((cfg_file = Open (filename, MODE_NEWFILE)) == NULL)
  {
  AllowVM (TRUE);
  return (FALSE);
  }

CurrentConfig.CfgMagic = CFG_MAGIC;
CurrentConfig.Version = VERSION;
CurrentConfig.Revision = REVISION;
get (LV_TaskList, MUIA_List_Entries, &(CurrentConfig.NumTaskEntries));

/* Default is stored separately */
CurrentConfig.NumTaskEntries -= 1; 

/* Task params are read when needed */

/* Memory page */
get (CY_Policy, MUIA_Cycle_Active, &Policy);
get (SL_MinMem, MUIA_Slider_Level, &val);
CurrentConfig.MinMem = val * 100 * 1024;
get (SL_MaxMem, MUIA_Slider_Level, &val);
CurrentConfig.MaxMem = val * 100 * 1024;

switch (Policy)
  {
  case MT_FIXED: CurrentConfig.MaxMem = CurrentConfig.MinMem;
                 break;
  case MT_DYNAMIC: CurrentConfig.MinMem = DYN_MIN;
                   CurrentConfig.MaxMem = DYN_MAX;
                   break;
  }

get (CY_MemFlags, MUIA_Cycle_Active, &val);
CurrentConfig.MemFlags = (val << 1) | MEMF_PUBLIC;
get (SL_Buffer, MUIA_Slider_Level, &val);
CurrentConfig.WriteBuffer = val * 10 * 1024;
get (SL_Prio, MUIA_Slider_Level, &val);
 CurrentConfig.VMPriority = val;

get (CY_Device, MUIA_Cycle_Active, &val);
CurrentConfig.PageDev = (UWORD)val;

if (CurrentConfig.PageDev == PD_PART)
  {
  get (ST_PartName, MUIA_String_Contents, &tmp);
  strcpy (CurrentConfig.PartOrFileName, tmp);
  strcat (CurrentConfig.PartOrFileName, ":");
  CurrentConfig.FileSize = 1;
  }
else
  {
  get (ST_FileName, MUIA_String_Contents, &tmp);
  strcpy (CurrentConfig.PartOrFileName, tmp);
  get (SL_FileSize, MUIA_Slider_Level, &val);
    CurrentConfig.FileSize = val;
  }

/* Statistics */
get (CM_StatEnabled, MUIA_Selected, &val);
CurrentConfig.StatEnabled = (BOOL) val;
get (CM_Zoomed, MUIA_Selected, &val);
CurrentConfig.StatZoomed = (BOOL) val;
get (ST_ZLeft,   MUIA_String_Integer, &val);
  CurrentConfig.ZLeftEdge = val;
get (ST_ZTop,    MUIA_String_Integer, &val);
  CurrentConfig.ZTopEdge = val;
get (ST_UnZLeft, MUIA_String_Integer, &val);
  CurrentConfig.UnZLeftEdge = val;
get (ST_UnZTop,  MUIA_String_Integer, &val);
  CurrentConfig.UnZTopEdge = val;

/* Misc */
#if defined(PLATFORM_HASZ2)
get (CM_CacheZ2RAM, MUIA_Selected, &val);
#else
  val = FALSE;
#endif
CurrentConfig.CacheZ2RAM = (BOOL) val;
get (CM_WBPatch,    MUIA_Selected, &val);
CurrentConfig.PatchWB = (BOOL) val;
get (CM_MemTracking,MUIA_Selected, &val);
CurrentConfig.MemTracking = (BOOL) val;
#if defined(PLATFORM_HASFASTROM)
get (CM_FastROM,MUIA_Selected, &val);
#else
 val = FALSE;
#endif
CurrentConfig.FastROM = (BOOL) val;

get (ST_MinVMAlloc, MUIA_String_Integer, &val);
CurrentConfig.MinVMAlloc = val;
get (ST_EnableHotkey, MUIA_String_Contents, &tmp);
strcpy (CurrentConfig.EnableHotkey, tmp);
get (ST_DisableHotkey, MUIA_String_Contents, &tmp);
strcpy (CurrentConfig.DisableHotkey, tmp);

DoMethod (LV_TaskList, MUIM_List_GetEntry, 0, &cur_task);

switch (cur_task->te_teif.DataPaging)
  {
  case DP_TRUE:
       CurrentConfig.DefaultMinPublic = USE_NEVER;
       CurrentConfig.DefaultMinNonPublic = USE_ALWAYS;
       break;
  case DP_FALSE:
       CurrentConfig.DefaultMinPublic = USE_NEVER;
       CurrentConfig.DefaultMinNonPublic = USE_NEVER;
       break;
  case DP_ADVANCED:
       CurrentConfig.DefaultMinPublic = cur_task->te_teif.MinPublic;
       CurrentConfig.DefaultMinNonPublic = cur_task->te_teif.MinNonPublic;
       break;
  }

CurrentConfig.DefaultCodePaging = cur_task->te_teif.CodePaging;
CurrentConfig.DefaultDataPaging = cur_task->te_teif.DataPaging;

Write (cfg_file, &CurrentConfig, sizeof (struct VMMConfig));

/* First task is default */
i = 1;
DoMethod (LV_TaskList, MUIM_List_GetEntry, 1, &cur_task);

while (cur_task != NULL)
  {
  i++;

  switch (cur_task->te_teif.DataPaging)
    {
    case DP_TRUE:
         cur_task->te_teif.MinPublic = USE_NEVER;
         cur_task->te_teif.MinNonPublic = USE_ALWAYS;
         break;
    case DP_FALSE:
         cur_task->te_teif.MinPublic = USE_NEVER;
         cur_task->te_teif.MinNonPublic = USE_NEVER;
         break;
    case DP_ADVANCED:
         break;
    }

  Write (cfg_file, &(cur_task->te_teif), sizeof (struct TaskEntryInFile));
  Write (cfg_file, cur_task->te_TaskName, (LONG)cur_task->te_teif.NameLen);

  DoMethod (LV_TaskList, MUIM_List_GetEntry, i, &cur_task);
  }

Close (cfg_file);
AllowVM (TRUE);
return (TRUE);
}
