#include "defs.h"

const char *Pages [4];

static struct NewMenu Menu [] = {
  { NM_TITLE, NULL,        0, 0, 0, (APTR) 0          },
  { NM_ITEM,  NULL,        0, 0, 0, (APTR) OPEN_ID    },
  { NM_ITEM,  NULL,        0, 0, 0, (APTR) SAVE_AS_ID },
  { NM_ITEM,  NULL,        0, 0, 0, (APTR) SAVE_WIN_ID },
  { NM_ITEM,  NM_BARLABEL, 0, 0, 0, (APTR) 0          },
  { NM_ITEM,  NULL,        0, 0, 0, (APTR) ABOUT_ID   },
  { NM_ITEM,  NM_BARLABEL, 0, 0, 0, (APTR) 0          },
  { NM_ITEM,  NULL,        0, 0, 0, (APTR) HIDE_ID    },
  { NM_ITEM,  NULL,        0, 0, 0, (APTR) MUIV_Application_ReturnID_Quit },

  { NM_TITLE, NULL,        0, 0, 0, (APTR) 0             },
  { NM_ITEM,  NULL,        0, 0, 0, (APTR) RESET_ID      },
  { NM_ITEM,  NULL,        0, 0, 0, (APTR) LAST_SAVED_ID },
  { NM_ITEM,  NULL,        0, 0, 0, (APTR) RESTORE_ID    },
  { NM_END,   0,           0, 0, 0, (APTR) 0             }
};

/***********************************************************************/

static void AttachID_BT (Object *obj, ULONG id)

{
DoMethod (obj, MUIM_Notify,
          MUIA_Pressed, FALSE,
          Application,
          2,
          MUIM_Application_ReturnID, id);
}

/***********************************************************************/

static void AttachID_CY (Object *obj, ULONG id)

{
DoMethod (obj, MUIM_Notify,
          MUIA_Cycle_Active, MUIV_EveryTime,
          Application,
          2,
          MUIM_Application_ReturnID, id);
}

/***********************************************************************/

static void AttachID_ST (Object *obj, ULONG id)

{
DoMethod (obj, MUIM_Notify,
          MUIA_String_Acknowledge, MUIV_EveryTime,
          Application,
          2,
          MUIM_Application_ReturnID, id);
}

/***********************************************************************/

BOOL CreateGUI (void)

{
/* creates the GUI, opens the window and returns an application object */
Pages [0] = _(msgTasksProgs);
Pages [1] = _(msgMemory);
Pages [2] = _(msgMisc);
Pages [3] = NULL;

Menu [0].nm_Label   = _(msgProject);
Menu [1].nm_Label   = _(msgOpen);
Menu [1].nm_CommKey = _(msgOpenShort);
Menu [2].nm_Label   = _(msgSaveAs);
Menu [2].nm_CommKey = _(msgSaveAsShort);
Menu [3].nm_Label   = _(msgSaveWindow);
Menu [5].nm_Label   = _(msgAbout);
Menu [7].nm_Label   = _(msgHide);
Menu [7].nm_CommKey = _(msgHideShort);
Menu [8].nm_Label   = _(msgQuit);
Menu [8].nm_CommKey = _(msgQuitShort);
Menu [9].nm_Label   = _(msgEdit);
Menu [10].nm_Label   = _(msgDefaults);
Menu [10].nm_CommKey = _(msgDefaultsShort);
Menu [11].nm_Label   = _(msgLastSaved);
Menu [11].nm_CommKey = _(msgLastSavedShort);
Menu [12].nm_Label   = _(msgRestore);
Menu [12].nm_CommKey = _(msgRestoreShort);

Application = ApplicationObject, 
  MUIA_Application_Author, AUTHOR,
  MUIA_Application_Base, PROGNAME,
  MUIA_Application_Title, PROGNAME,
  MUIA_Application_Version, "$VER: 3.3",
  MUIA_Application_Copyright, _(msgCopyright),
  MUIA_Application_Description, _(msgVMMDescr),
  MUIA_Application_UseCommodities, FALSE,
  MUIA_Application_UseRexx, FALSE,
  MUIA_Application_HelpFile, "HELP:english/VMM.guide",
  SubWindow, WI_Window =
    WindowObject,
      MUIA_Window_Title, PROGNAME " " VER_STRING,
      MUIA_Window_CloseGadget, FALSE,
      MUIA_Window_Menustrip, MUI_MakeObject (MUIO_MenustripNM, Menu, 0),
      MUIA_Window_ID, 1,
      WindowContents, 
        VGroup,
          MUIA_HelpNode, "VMMPREFS",
          Child, 
            RegisterGroup (Pages),
              Child, CreateTaskPage (),
              Child, CreateMemoryPage (),
              Child, CreateMiscPage (),
            End,
          Child,
            HGroup,
              MUIA_Group_SameSize, TRUE,
              Child, BT_Save = MyButton (msgSave, msgSaveShort),
              Child, HSpace (0),
              Child, BT_Use = MyButton (msgUse, msgUseShort),
              Child, HSpace (0),
              Child, BT_Cancel = MyButton (msgCancel, msgCancelShort),
            End,
        End,
    End,
  SubWindow, WI_TaskSelect = CreateTaskSelectWindow (),
End;

AttachID_BT (BT_Save, SAVE_ID);
AttachID_BT (BT_Use, USE_ID);
AttachID_BT (BT_Cancel, CANCEL_ID);
AttachID_BT (PO_Disks, POP_ID);
AttachID_BT (BT_AddProg, ADDPROG_ID);
AttachID_BT (BT_AddDir, ADDDIR_ID);
AttachID_BT (BT_Top, TOP_ID);
AttachID_BT (BT_AddTask, ADDTASK_ID);
AttachID_BT (BT_Up, UP_ID);
AttachID_BT (BT_Add, ADD_ID);
AttachID_BT (BT_Down, DOWN_ID);
AttachID_BT (BT_Delete, DELETE_ID);
AttachID_BT (BT_Bottom, BOTTOM_ID);
AttachID_BT (BT_Sort, SORT_ID);
AttachID_CY (CY_CodeOptions, CODE_PG_ID);
AttachID_CY (CY_DataOptions, DATA_PG_ID);
AttachID_ST (ST_MinPublic, MINPUBLIC_ID);
AttachID_ST (ST_MinNonPublic, MINNONPUBLIC_ID);
AttachID_ST (ST_CurrentTask, TASKNAME_ID);

AttachID_CY (CY_MemFlags, MEMFLAGS_ID);

DoMethod (WI_TaskSelect, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
          WI_Window, 
          3, MUIM_Set, MUIA_Window_Sleep, FALSE);


DoMethod (WI_Window, MUIM_Window_SetCycleChain, 
          LV_TaskList, /* ST_CurrentTask, */
          CY_CodeOptions, CY_DataOptions,
          ST_MinPublic, ST_MinNonPublic, 

          CY_Policy, SL_MinMem, SL_MaxMem,
          CY_MemFlags, SL_Buffer, SL_Prio,
          CY_Device, ST_PartName, ST_FileName,
          SL_FileSize,
          CM_StatEnabled, CM_Zoomed, 
          ST_UnZLeft, ST_UnZTop,
          ST_ZLeft, ST_ZTop,
#if defined(PLATFORM_HASZ2)
          CM_CacheZ2RAM,
#endif
          CM_WBPatch, CM_MemTracking,
          ST_MinVMAlloc,
          ST_EnableHotkey, ST_DisableHotkey,

          BT_Save, BT_Use, BT_Cancel,
          NULL);

set (WI_Window, MUIA_Window_ActiveObject, LV_TaskList);

/*
DoMethod (LV_TaskList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
          WI_Window, 3, MUIM_Set, MUIA_Window_ActiveObject, ST_CurrentTask);
DoMethod (LV_TaskList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
          ST_CurrentTask, 3, MUIM_Set, MUIA_String_BufferPos, 0);
*/

return ((BOOL)(Application != NULL));
}

/***********************************************************************/

void DisposeGUI (void)

{
MUI_DisposeObject (Application);
}

/***********************************************************************/

BOOL ValidSettings (void)

{
ULONG PageDev;
char *PartName,
     *FileName;

get (CY_Device, MUIA_Cycle_Active, &PageDev);
get (ST_PartName, MUIA_String_Contents, &PartName);
get (ST_FileName, MUIA_String_Contents, &FileName);

if (((PageDev == PD_PART) && 
    ((PartName == NULL) || (*PartName == 0)))
                     ||
    ((PageDev != PD_PART) && 
    ((FileName == NULL) || (*FileName == 0))))
  {
  MUI_Request (Application, NULL, 0L, NULL, "*_OK", _(msgNameMissing));
  return (FALSE);
  }

return (TRUE);
}

/***********************************************************************/

int HandleMUIId (ULONG id)

{
struct TaskEntry *te;
LONG pos, val;
char *name;

if (id >= FIRST_MENU_ID && id <= LAST_MENU_ID)
  {
  if (HandleMenuChoice (id))
    return (Q_DONTQUIT);
  else
    return (Q_QUITGUI);
  }

switch (id)
  {
  case MUIV_Application_ReturnID_Quit:
       return (Q_QUITBOTH);

  case SAVE_ID:
       if (!ValidSettings ())
         return (Q_DONTQUIT);
       if (!WriteConfigFile (CFG_NAME_SAVE))
         printf ("Couldn't write envarc file\n");
       if (!WriteConfigFile (CFG_NAME_USE))
         printf ("Couldn't write env file\n");

       if (!VMM_RUNNING)
         StartVMM ();
       return (Q_QUITGUI);

  case USE_ID:
       if (!ValidSettings ())
         return (Q_DONTQUIT);
       if (!WriteConfigFile (CFG_NAME_USE))
         printf ("Couldn't write env file\n");

       if (!VMM_RUNNING)
         StartVMM ();
       return (Q_QUITGUI);

  case CANCEL_ID:
       if (!VMM_RUNNING)
         StartVMM ();
       return (Q_QUITGUI);

  case POP_ID:
       HandleDiskPopup ();
       break;

  case ADD_ID:
       AddNamed ((char *)_(msgNew));
       break;

  case ADDPROG_ID:
       {
       struct FileRequester *AddProgReq;

       if ((AddProgReq = MUI_AllocAslRequest (ASL_FileRequest, NULL)) == NULL)
         {
         DisplayBeep (NULL);
         break;
         }

       if (MUI_AslRequestTags (AddProgReq, ASLFR_Window, VMMWindow,
                               ASLFR_SleepWindow, TRUE,
                               ASLFR_TitleText, _(msgSelectProg),
                               ASLFR_RejectIcons, TRUE,
                               ASLFR_DoSaveMode, FALSE,
                               TAG_DONE, 0L))
         {
         AddNamed (AddProgReq->fr_File);
         }

       MUI_FreeAslRequest (AddProgReq);
       }
       break;

  case ADDDIR_ID:
       {
       struct FileRequester *AddDirReq;
       char directory [300];
       BPTR DirLock;

       if ((AddDirReq = MUI_AllocAslRequest (ASL_FileRequest, NULL)) == NULL)
         {
         DisplayBeep (NULL);
         break;
         }

       if (MUI_AslRequestTags (AddDirReq, ASLFR_Window, VMMWindow,
                               ASLFR_SleepWindow, TRUE,
                               ASLFR_TitleText, _(msgSelectDir),
                               ASLFR_DrawersOnly, TRUE,
                               ASLFR_DoSaveMode, FALSE,
                               TAG_DONE, 0L))
         {
         strcpy (directory, AddDirReq->fr_Drawer);
         if (index (directory, ':') == 0)
           {
           /* Obtain a fully qualified path */
           DirLock = Lock (directory, ACCESS_READ);
           if (!NameFromLock (DirLock, directory, 300))
             strcpy (directory, AddDirReq->fr_Drawer);
           UnLock (DirLock);
           }
         if (*(directory + strlen (directory) - 1) != ':')
           strcat (directory, "/");
         AddNamed (directory);
         }

       MUI_FreeAslRequest (AddDirReq);
       }
       break;

  case ADDTASK_ID:
       BuildTaskList ();
       break;

  case DELETE_ID:
       DoMethod (LV_TaskList, MUIM_List_GetEntry, 
                 MUIV_List_GetEntry_Active, &te);
       DoMethod (LV_TaskList, MUIM_List_Remove, MUIV_List_Remove_Active);
       break;

  case TOP_ID:
       DoMethod (LV_TaskList, MUIM_List_Move, MUIV_List_Move_Active, 1);
       set (LV_TaskList, MUIA_List_Active, 1);
       break;

  case UP_ID:
       DoMethod (LV_TaskList, MUIM_List_Move, MUIV_List_Move_Active,
                 MUIV_List_Move_Previous);
       set (LV_TaskList, MUIA_List_Active, MUIV_List_Active_Up);
       break;

  case DOWN_ID:
       DoMethod (LV_TaskList, MUIM_List_Move, MUIV_List_Move_Active,
                 MUIV_List_Move_Next);
       set (LV_TaskList, MUIA_List_Active, MUIV_List_Active_Down);
       break;

  case BOTTOM_ID:
       DoMethod (LV_TaskList, MUIM_List_Move, MUIV_List_Move_Active, 
                 MUIV_List_Move_Bottom);
       set (LV_TaskList, MUIA_List_Active, MUIV_List_Active_Bottom);
       break;

  case SORT_ID:
       DoMethod (LV_TaskList, MUIM_List_Sort);
       break;

  case CODE_PG_ID:
       DoMethod (LV_TaskList, MUIM_List_GetEntry, 
                 MUIV_List_GetEntry_Active, &te);
       get (CY_CodeOptions, MUIA_Cycle_Active, &pos);
       te->te_teif.CodePaging = pos;
       DoMethod (LV_TaskList, MUIM_List_Redraw, MUIV_List_Redraw_Active);
       break;

  case DATA_PG_ID:
       DoMethod (LV_TaskList, MUIM_List_GetEntry, 
                 MUIV_List_GetEntry_Active, &te);
       get (CY_DataOptions, MUIA_Cycle_Active, &pos);
       te->te_teif.DataPaging = pos;
       DoMethod (LV_TaskList, MUIM_List_Redraw, MUIV_List_Redraw_Active);
       break;

  case MINPUBLIC_ID:
       DoMethod (LV_TaskList, MUIM_List_GetEntry, 
                 MUIV_List_GetEntry_Active, &te);
       get (ST_MinPublic, MUIA_String_Integer, &val);
       te->te_teif.MinPublic = val;
       break;

  case MINNONPUBLIC_ID:
       DoMethod (LV_TaskList, MUIM_List_GetEntry, 
                 MUIV_List_GetEntry_Active, &te);
       get (ST_MinNonPublic, MUIA_String_Integer, &val);
       te->te_teif.MinNonPublic = val;
       break;

  case TASKNAME_ID:
       DoMethod (LV_TaskList, MUIM_List_GetEntry, 
                 MUIV_List_GetEntry_Active, &te);
       get (ST_CurrentTask, MUIA_String_Contents, &name);
       FreeMem (te->te_TaskName, (ULONG)te->te_teif.NameLen);
       te->te_teif.NameLen = strlen (name) + 1;
       if ((te->te_TaskName = AllocMem ((ULONG)te->te_teif.NameLen, MEMF_PUBLIC)) == NULL)
         break;
       strcpy (te->te_TaskName, name);
       DoMethod (LV_TaskList, MUIM_List_Redraw, MUIV_List_Redraw_Active);
       break;

  case MEMFLAGS_ID:
       {
       ULONG flags;

       get (CY_MemFlags, MUIA_Cycle_Active, &val);
       switch (val)
         {
         case MF_FAST: flags = MEMF_FAST | MEMF_PUBLIC; break;
         case MF_CHIP: flags = MEMF_CHIP | MEMF_PUBLIC; break;
         case MF_ANY:  flags = MEMF_ANY | MEMF_PUBLIC;  break;
         }

       val = MaxAvailMem (flags) / (100 * 1024);
       set (SL_MinMem, MUIA_Slider_Max, val);
       set (SL_MaxMem, MUIA_Slider_Max, val);

       /* Because of a bug in MUI, notifications for MUIA_Slider_Level
        * are forgotten when, when Slider_Max is reduced below Slider_Level.
        * They are done by hand here instead.
        */

       get (SL_MinMem, MUIA_Slider_Level, &val);
       DoMethod (TX_MinMem, MUIM_SetAsString, MUIA_Text_Contents, 
                 "%ld00 KByte", val);

       get (SL_MaxMem, MUIA_Slider_Level, &val);
       DoMethod (TX_MaxMem, MUIM_SetAsString, MUIA_Text_Contents, 
                 "%ld00 KByte", val);

       }
       break;
  }

return (Q_DONTQUIT);
}

/***********************************************************************/

int HandleGUI (void)

{
int running_state = Q_DONTQUIT;
ULONG MUISignals,
      WaitMask,
      ReceivedSignals;
ULONG ReturnedId;

if (!CreateGUI ())
  {
  printf ("Could not create MUI application\n");
  CloseLibrary (MUIMasterBase);
  return (Q_QUITGUI);
  }

if (!ReadConfigFileMUI (CfgName))
  MenuDefault ();

ReadConfigFromVMM ();

  bug("[VMM:GUI] %s: opening display\n", __func__);

set(WI_Window, MUIA_Window_Open, TRUE);
get (WI_Window, MUIA_Window_Window, &VMMWindow);

WaitMask = SIGBREAKF_CTRL_C | (1L << ExtCxPort->ShowSignal);
if (CxParams != NULL)
  WaitMask |= (1L << ExtCxPort->CxPort.mp_SigBit);


ReceivedSignals = MUISignals = 0; /* to enter the input method once */
  
while (running_state == Q_DONTQUIT)
  {
  if ((MUISignals & ReceivedSignals) || (MUISignals == 0L))
    {
    ReturnedId = DoMethod(Application, MUIM_Application_Input, &MUISignals);
    running_state = HandleMUIId (ReturnedId);
    }

  if (running_state == Q_DONTQUIT && MUISignals != 0L) 
    {
    ReceivedSignals = Wait(MUISignals | WaitMask);

    if ((CxParams != NULL) && 
        (ReceivedSignals & (1L << ExtCxPort->CxPort.mp_SigBit)))
      {
      int TermVMM;

#if !defined(MAX)
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

  bug("[VMM:GUI] %s: calling HandleCxMsg\n", __func__);

      TermVMM = HandleCxMsg ();
      /* store the quit status which is 'stronger' */
      running_state = MAX (TermVMM, running_state);      
      }

    if (ReceivedSignals & (1L << ExtCxPort->ShowSignal))
      set (Application, MUIA_Application_Iconified, FALSE);

    if (ReceivedSignals & SIGBREAKF_CTRL_C)
      running_state = Q_QUITGUI;
    }
  }

  bug("[VMM:GUI] %s: disposing UI ..\n", __func__);

DisposeGUI ();
  
  bug("[VMM:GUI] %s: exiting\n", __func__);

return (running_state);
}
