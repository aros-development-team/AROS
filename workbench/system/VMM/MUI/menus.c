#include "defs.h"

/**********************************************************************/
/*                            Menu functions                          */
/**********************************************************************/

BOOL MenuOpen (void)

{
struct FileRequester *OpenReq;
char path [80];

if ((OpenReq = MUI_AllocAslRequest (ASL_FileRequest, NULL)) == NULL)
  {
  printf ("Out of memory\n");
  return (TRUE);
  }

if (MUI_AslRequestTags (OpenReq, ASLFR_Window, VMMWindow,
                    ASLFR_SleepWindow, TRUE,
                    ASLFR_TitleText, _(msgLoadPrefs),
                    ASLFR_RejectIcons, TRUE,
                    TAG_DONE, 0L))
  {
  strcpy (path, OpenReq->fr_Drawer);
  AddPart (path, OpenReq->fr_File, 80);
  ReadConfigFileMUI (path);
  }

MUI_FreeAslRequest (OpenReq);
return (TRUE);
}

/**********************************************************************/

BOOL MenuSaveAs (void)

{
struct FileRequester *SaveReq;
char path [80];
if (!ValidSettings ())
  return (TRUE);

if ((SaveReq = MUI_AllocAslRequest (ASL_FileRequest, NULL)) == NULL)
  {
  printf ("Out of memory\n");
  return (TRUE);
  }

if (MUI_AslRequestTags (SaveReq, ASLFR_Window, VMMWindow,
                    ASLFR_SleepWindow, TRUE,
                    ASLFR_TitleText, _(msgSavePrefs),
                    ASLFR_RejectIcons, TRUE,
                    ASLFR_DoSaveMode, TRUE,
                    TAG_DONE, 0L))
  {
  strcpy (path, SaveReq->fr_Drawer);
  AddPart (path, SaveReq->fr_File, 80);
  WriteConfigFile (path);
  }

MUI_FreeAslRequest (SaveReq);
return (TRUE);
}

/**********************************************************************/

BOOL MenuSaveWin (void)

{
if (SystemTags ("copy >nil: ENV:mui/" PROGNAME "#? ENVARC:mui quiet", 
                TAG_DONE) != 0)
  MUI_Request (Application, 0L, 0L, NULL, "*_OK", _(msgSaveWinError));

return (TRUE);
}

/**********************************************************************/

BOOL MenuAbout (void)

{
MUI_Request (Application, 0L, 0L, NULL, "*_OK", _(msgAboutString));
return (TRUE);
}

/**********************************************************************/

BOOL MenuHide (void)

{
if (!VMM_RUNNING)
  StartVMM ();
return (FALSE);
}

/**********************************************************************/

BOOL MenuDefault (void)

{
struct TaskEntry te;

te.te_TaskName = (char *)_(msgDefault);
te.te_teif.MinPublic = USE_NEVER;
te.te_teif.MinNonPublic = USE_ALWAYS;
te.te_teif.DataPaging = DP_TRUE;
te.te_teif.CodePaging = FALSE;
te.te_teif.IsDefault = TRUE;
te.te_teif.NameLen = strlen (te.te_TaskName) + 1;

DoMethod (LV_TaskList, MUIM_List_Clear);
DoMethod (LV_TaskList, MUIM_List_InsertSingle, &te, 0);
set (LV_TaskList, MUIA_List_Active, 0);

set (CM_StatEnabled, MUIA_Selected, TRUE);
set (CM_Zoomed, MUIA_Selected, FALSE);
set (ST_ZTop, MUIA_String_Integer, 0);
set (ST_ZLeft, MUIA_String_Integer, 0);
set (ST_UnZTop, MUIA_String_Integer, 0);
set (ST_UnZLeft, MUIA_String_Integer, 0);

set (CY_Policy, MUIA_Cycle_Active, MT_DYNAMIC);
set (CY_Device, MUIA_Cycle_Active, PD_FILE);
set (SL_MinMem, MUIA_Slider_Level, 1);       /* 100 K */
set (SL_MaxMem, MUIA_Slider_Level, 1);
set (CY_MemFlags, MUIA_Cycle_Active, MF_FAST);
set (ST_PartName, MUIA_String_Contents, NULL);
set (ST_FileName, MUIA_String_Contents, "SYS:page_file");
set (SL_FileSize, MUIA_Slider_Level, 1);

set (SL_Prio, MUIA_Slider_Level, 40);
set (SL_Buffer, MUIA_Slider_Level, 10);

set (ST_EnableHotkey, MUIA_String_Contents, DEFAULT_ENABLE_KEY);
set (ST_DisableHotkey, MUIA_String_Contents, DEFAULT_DISABLE_KEY);

set (ST_MinVMAlloc, MUIA_String_Integer, DEFAULT_MINVMSIZE);
#if defined(PLATFORM_HASZ2)
set (CM_CacheZ2RAM, MUIA_Selected, TRUE);
#endif
return (TRUE);
}

/**********************************************************************/

BOOL HandleMenuChoice (ULONG id)

{
switch (id)
  {
  case OPEN_ID:          return (MenuOpen ());
  case SAVE_AS_ID:       return (MenuSaveAs ());
  case SAVE_WIN_ID:      return (MenuSaveWin ());
  case ABOUT_ID:         return (MenuAbout ());
  case HIDE_ID:          return (MenuHide ());
  case RESET_ID:         return (MenuDefault ());
  case LAST_SAVED_ID:    ReadConfigFileMUI (CFG_NAME_SAVE);
                         return (TRUE);
  case RESTORE_ID:       ReadConfigFileMUI (CfgName);
                         return (TRUE);
  }
return (FALSE);
}
