#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#ifdef _DCC
#define __inline
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <clib/utility_protos.h>
#include "VMMUsage.h"
#ifdef __GNUC__
#include "../include/VMM_stat.h"
#include <inline/muimaster.h>
#else
#include "/include/VMM_stat.h"
#endif

extern void *InitTimer (int, UWORD *);
extern void CloseTimer (void *);
extern BOOL AddTimedFunction (void *, ULONG secs, ULONG micros, void (*function) ());
extern void HandleTimerReturn (void *);

struct Hook DisplayHook;
struct Hook DestructHook;
struct ObjApp *ObjApp;
void *TimerData;
struct VMUsageMsg *UsageMsg;
LONG VMSignal;
BOOL ListEmpty = TRUE;

struct Library *MUIMasterBase;
struct Library *UtilityBase;
UWORD TimerSignal;

#ifdef __GNUC__
extern void CallHook_C (void);
#endif

/******************************************************************************/

#ifdef __GNUC__
static LONG DisplayElem (struct Hook *hook, struct TaskVMUsage *te, char **array )
#else
static __saveds __asm LONG DisplayElem (register __a1 struct TaskVMUsage *te,
                                        register __a2 char **array)
#endif

{
static char buf [50];

*array++ = te->vu_Node.ln_Name;

sprintf (buf, "%ld", te->VMInUse);
*array++ = buf;

return (0);
}

/******************************************************************************/

#ifdef __GNUC__
static void DestructElem (struct Hook *hook, struct TaskVMUsage *te, int dummy)
#else
static __saveds __asm void DestructElem (register __a1 struct TaskVMUsage *te)
#endif

{
FreeMem (te, te->FreeSize);
}

/******************************************************************************/

struct ObjApp * CreateApp(void)
{
	struct ObjApp * Object;

	APTR	GROUP_ROOT_0, TX_UpdateTime, TX_Title;

	if (!(Object = AllocVec(sizeof(struct ObjApp), MEMF_PUBLIC|MEMF_CLEAR)))
		return(NULL);

#ifdef __GNUC__
     DisplayHook.h_Entry = (HOOKFUNC)CallHook_C;
     DisplayHook.h_SubEntry = (HOOKFUNC)DisplayElem;
     DestructHook.h_Entry = (HOOKFUNC)CallHook_C;
     DestructHook.h_SubEntry = (HOOKFUNC)DestructElem;
#else
     DisplayHook.h_Entry = (HOOKFUNC)DisplayElem;
     DestructHook.h_Entry = (HOOKFUNC)DestructElem;
#endif

	Object->STR_TX_UpdateTime = "\033cUpdate frequency";
	Object->STR_TX_Frequency = NULL;

     TX_Title = TextObject,
		MUIA_Text_Contents, "Tasks using VM",
		MUIA_Text_PreParse, "\033c",
     End;

	Object->LV_TaskList = ListObject,
		MUIA_Frame, MUIV_Frame_ReadList,
		MUIA_List_Format, "MINWIDTH=20 MAXWIDTH=100,PREPARSE=\33r",
		MUIA_List_DisplayHook, &DisplayHook,
		MUIA_List_DestructHook, &DestructHook,
	End;

	Object->LV_TaskList = ListviewObject,
		MUIA_Listview_List, Object->LV_TaskList,
		MUIA_Listview_Input, FALSE,
		MUIA_Weight, 1000,
	End;

	TX_UpdateTime = TextObject,
		MUIA_Background, MUII_WindowBack,
		MUIA_Text_Contents, Object->STR_TX_UpdateTime,
	End;

	Object->SL_UpdateTime = SliderObject,
		MUIA_Weight, 0,
		MUIA_Slider_Min, 1,
		MUIA_Slider_Max, 100,
		MUIA_Slider_Quiet, TRUE,
		MUIA_Slider_Level, 10,
	End;

	Object->TX_Frequency = TextObject,
		MUIA_Background, MUII_WindowBack,
		MUIA_Text_Contents, Object->STR_TX_Frequency,
		MUIA_Text_PreParse, "\033c",
		MUIA_Text_Contents, "10/10 s",
		MUIA_Text_SetMin, TRUE,
	End;

	GROUP_ROOT_0 = GroupObject,
	     Child, TX_Title,
		Child, Object->LV_TaskList,
		Child, TX_UpdateTime,
		Child, Object->SL_UpdateTime,
		Child, Object->TX_Frequency,
	End;

	Object->WI_Usage = WindowObject,
		MUIA_Window_Title, "VM Usage",
		MUIA_Window_ID, MAKE_ID('0', 'W', 'I', 'N'),
		WindowContents, GROUP_ROOT_0,
	End;

	Object->App = ApplicationObject,
		MUIA_Application_Author, "Martin Apel",
		MUIA_Application_Base, "VMU",
		MUIA_Application_Title, "VMUsage",
		MUIA_Application_Version, "$VER: VMUsage 1.1 (15.12.95)",
		MUIA_Application_Copyright, "Martin Apel",
		MUIA_Application_Description, "Vm usage display",
		SubWindow, Object->WI_Usage,
	End;


	if (!Object->App)
	{
		FreeVec(Object);
		return(NULL);
	}

	DoMethod(Object->SL_UpdateTime,
		MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime,
		Object->TX_Frequency,
		4,
		MUIM_SetAsString, MUIA_Text_Contents, "%2ld/10 s", MUIV_TriggerValue
		);

     DoMethod (Object->WI_Usage,
          MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		Object->App,
		2,
		MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
		);
          
	DoMethod(Object->WI_Usage,
		MUIM_Window_SetCycleChain, Object->LV_TaskList,
		Object->SL_UpdateTime,
		0
		);

	set(Object->WI_Usage,
		MUIA_Window_Open, TRUE
		);


	return(Object);
}

/******************************************************************************/

void DisposeApp(struct ObjApp * Object)
{
	MUI_DisposeObject(Object->App);
	FreeVec(Object);
}

/******************************************************************************/

void SortList (struct List *list)

{
/* Sort a list by ln_Name. */

struct Node *tmp1,
            *tmp2;
struct List TmpList;

NewList (&TmpList);

while (list->lh_Head->ln_Succ != NULL)    /* Not empty */
  {
  tmp1 = list->lh_Head;
  for (tmp2 = tmp1->ln_Succ; tmp2->ln_Succ != NULL; tmp2 = tmp2->ln_Succ)
    {
    if (Stricmp (tmp1->ln_Name, tmp2->ln_Name) < 0)
      tmp1 = tmp2;
    }
  Remove (tmp1);
  AddHead (&TmpList, tmp1);
  }

while (TmpList.lh_Head->ln_Succ != NULL)
  {
  tmp1 = RemHead (&TmpList);
  AddTail (list, tmp1);
  }
}

/******************************************************************************/

struct List *GetListFromVMM (void)

{
BOOL success = FALSE;
struct MsgPort *VMPort;

UsageMsg->VMMessage.mn_Length = sizeof (struct VMUsageMsg);
UsageMsg->Sender      = FindTask (NULL);
UsageMsg->Command     = VMCMD_AskVMUsage;
UsageMsg->ReplySignal = VMSignal;

Forbid ();
if ((VMPort = FindPort ("VMM_Port")) != NULL)
  {
  PutMsg (VMPort, (struct Message*)UsageMsg);
  success = TRUE;
  }
Permit ();

if (!success)
  return (NULL);

Wait (1L << VMSignal);

return (UsageMsg->TaskList);
}

/******************************************************************************/

void UpdateList (void)

{
/* Updates the currently visible list. To minimize the amount
 * of flickering on the screen only new, deleted  or changed entries are
 * updated. To do this the list received from VMM is sorted first
 * and then stepwise compared with the list currently displayed.
 * As changed entries are encountered the display is changed accordingly.
 */

ULONG level;
struct List *UpdatedList;
struct TaskVMUsage *upd_entry,
                   *disp_entry;
int pos;

get (ObjApp->SL_UpdateTime, MUIA_Slider_Level, &level);

AddTimedFunction (TimerData, level / 10, (level % 10) * 100000L, UpdateList);

if ((UpdatedList = GetListFromVMM ()) != NULL)
  {
  SortList (UpdatedList);

  pos = 0;
  DoMethod (ObjApp->LV_TaskList, MUIM_List_GetEntry, pos, &disp_entry);
  upd_entry = (struct TaskVMUsage*) RemHead (UpdatedList);

  while (upd_entry != NULL || disp_entry != NULL)
    {
    int relation;

    if (disp_entry == NULL)
      relation = 1;               /* As if the entry was larger */
    else if (upd_entry == NULL)
      relation = -1;
    else
      relation = Stricmp (disp_entry->vu_Node.ln_Name, upd_entry->vu_Node.ln_Name);

    if (relation < 0)
      {
      /* There is an entry displayed which is not in the updated
       * list anymore. Remove it.
       */
      DoMethod (ObjApp->LV_TaskList, MUIM_List_Remove, pos);
      DoMethod (ObjApp->LV_TaskList, MUIM_List_GetEntry, pos, &disp_entry);
      }
    else if (relation > 0)
      {
      /* There is a new entry in the updated list.
       * Add it to the display list.
       */
      DoMethod (ObjApp->LV_TaskList, MUIM_List_InsertSingle, upd_entry, pos);
      pos++;
      /* disp_entry remains the same */
      upd_entry = (struct TaskVMUsage*)RemHead (UpdatedList);
      ListEmpty = FALSE;
      }
    else
      {
      /* The entries are identical. Check if the values differ
       * and do a refresh only when necessary.
       */
      if (disp_entry->VMInUse != upd_entry->VMInUse)
        {
        disp_entry->VMInUse = upd_entry->VMInUse;
        DoMethod (ObjApp->LV_TaskList, MUIM_List_Redraw, pos);
        }
      FreeMem (upd_entry, upd_entry->FreeSize);
      upd_entry = (struct TaskVMUsage*)RemHead (UpdatedList);
      pos++;
      DoMethod (ObjApp->LV_TaskList, MUIM_List_GetEntry, pos, &disp_entry);
      }
    } 

  FreeMem (UpdatedList, sizeof (struct List));
  }
else if (!ListEmpty)
  {
  DoMethod (ObjApp->LV_TaskList, MUIM_List_Clear);
  ListEmpty = TRUE;
  }
}

/******************************************************************************/

BOOL Init (void)

{
VMSignal = -1L;

if ((MUIMasterBase = OpenLibrary ("muimaster.library", 10L)) == NULL)
  {
  printf ("Couldn't open muimaster.library\n");
  return (FALSE);
  }

if ((UtilityBase = OpenLibrary ("utility.library", 10L)) == NULL)
  {
  printf ("Couldn't open utility.library\n");
  return (FALSE);
  }

if ((TimerData = InitTimer (1, &TimerSignal)) == NULL)
  {
  printf ("Couldn't initialize timer\n");
  return (FALSE);
  }

if ((UsageMsg = AllocMem (sizeof (struct VMUsageMsg), MEMF_PUBLIC)) == NULL)
  {
  printf ("Not enough memory\n");
  return (FALSE);
  }

if ((VMSignal = AllocSignal (-1L)) == -1L)
  {
  printf ("Ran out of signal bits\n");
  return (FALSE);
  }

if ((ObjApp = CreateApp ()) == NULL)
  {
  printf ("Couldn't create MUI application\n");
  return (FALSE);
  }
}

/******************************************************************************/

void Cleanup (void)

{
if (ObjApp)
  DisposeApp (ObjApp);

if (VMSignal != -1L)
  FreeSignal (VMSignal);

if (UsageMsg != NULL)
  FreeMem (UsageMsg, sizeof (struct VMUsageMsg));

if (TimerData != NULL)
  CloseTimer (TimerData);

if (UtilityBase != NULL)
  CloseLibrary (UtilityBase);

if (MUIMasterBase != NULL)
  CloseLibrary (MUIMasterBase);
}

/******************************************************************************/

void main (void)

{
ULONG MUISignals;
BOOL running = TRUE;
ULONG ReceivedSignals;

if (!Init ())
  {
  Cleanup ();
  exit (10);
  }

ReceivedSignals = 0L;
AddTimedFunction (TimerData, 1L, 0L, UpdateList);
UpdateList ();

while (running)
  {
  switch (DoMethod(ObjApp->App, MUIM_Application_Input, &MUISignals))
    {
    case MUIV_Application_ReturnID_Quit:
         running = FALSE;
         break;
    }

  if (running && (MUISignals != 0L))
    ReceivedSignals = Wait (MUISignals | (1L << TimerSignal));

  if (ReceivedSignals & (1L << TimerSignal))
    HandleTimerReturn (TimerData);
  }

Cleanup ();
}
