#include "defs.h"

#define DEBUG 1
#include <aros/debug.h>

char *CodeEntries [3];
char *DataEntries [4];

static struct Hook ConstructHook,
                   DestructHook,
                   DisplayHook,
                   CompareHook,
                   SelectHook,
                   EntrySelectedHook;

APTR TaskResBase;

/***********************************************************************/

#if defined(__AROS__)
AROS_UFH3(struct TaskEntry *, ConstructElem,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR, unused, A2),
    AROS_UFHA(struct TaskEntry *, te, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static struct TaskEntry *ConstructElem (struct Hook *hook, struct TaskEntry *te)
{
#else
static __saveds __asm struct TaskEntry *ConstructElem (register __a1 struct TaskEntry *te)
{
#endif
#endif
  
struct TaskEntry *newEntry;

  if (te->te_TaskName )
    bug("[VMM:GUI] %s: te @ 0x%p, name @ 0x%p '%s'\n", __func__, te, te->te_TaskName, te->te_TaskName);

if ((newEntry = AllocMem (sizeof (struct TaskEntry), MEMF_PUBLIC)) == NULL)
  return (NULL);

  bug("[VMM:GUI] %s: new entry @ 0x%p\n", __func__, newEntry);
*newEntry = *te;
  bug("[VMM:GUI] %s: namelen %d\n", __func__, newEntry->te_teif.NameLen);
if ((newEntry->te_TaskName = AllocMem ((ULONG)te->te_teif.NameLen + 1, MEMF_PUBLIC)) == NULL)
  {
  FreeMem (newEntry, sizeof (struct TaskEntry));
  return (NULL);
  }

strcpy (newEntry->te_TaskName, te->te_TaskName);
return (newEntry);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/***********************************************************************/

#if defined(__AROS__)
AROS_UFH3(VOID, DestructElem,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR, unused, A2),
    AROS_UFHA(struct TaskEntry *, te, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static void DestructElem (struct Hook *hook, struct TaskEntry *te)
{
#else
static __saveds __asm void DestructElem (register __a1 struct TaskEntry *te)
{
#endif
#endif
  bug("[VMM:GUI] %s: te @ 0x%p\n", __func__, te);
  bug("[VMM:GUI] %s: TaskName @ 0x%p (len %d)\n", __func__, te->te_TaskName, te->te_teif.NameLen);
  if (te->te_teif.NameLen > 0)
  FreeMem (te->te_TaskName, (ULONG)te->te_teif.NameLen + 1);
FreeMem (te, sizeof (struct TaskEntry));

#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/***********************************************************************/

#if defined(__AROS__)
AROS_UFH3(APTR, DisplayElem,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(char **, array, A2),
    AROS_UFHA(struct TaskEntry *, te, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static LONG DisplayElem (struct Hook *hook, struct TaskEntry *te, char **array)
{
#else
static __saveds __asm LONG DisplayElem (register __a1 struct TaskEntry *te,
                                        register __a2 char **array)
{
#endif
#endif
static char title_buf [50];

if (te == NULL)
  {
  *array++ = (char *)_(msgCode);
  *array++ = (char *)_(msgData);
  strcpy (title_buf, "\33c");
  strcat (title_buf, _(msgTasksProgs));
  *array++ = title_buf;
  }
else
  {
  *array++ = te->te_teif.CodePaging ? (char *)_(msgUseCodeShort) : (char *)_(msgNoUseCodeShort);

  switch (te->te_teif.DataPaging)
    {
    case DP_FALSE: *array++ = (char *)_(msgNoUseDataShort); break;
    case DP_TRUE:  *array++ = (char *)_(msgUseDataShort); break;
    case DP_ADVANCED: *array++ = (char *)_(msgAdvancedDataShort); break;
    }

  *array++ = te->te_TaskName;
  }

return (0);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/***********************************************************************/

#if defined(__AROS__)
AROS_UFH3(LONG, CompareElems,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(struct TaskEntry *, te2, A2),
    AROS_UFHA(struct TaskEntry *, te1, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static LONG CompareElems (struct Hook *hook, struct TaskEntry *te1, 
                          struct TaskEntry *te2)
{
#else
static __saveds __asm LONG CompareElems (register __a1 struct TaskEntry *te1,
                                         register __a2 struct TaskEntry *te2)
{
#endif
#endif
if (te1->te_teif.IsDefault)
  return (-1);
else if (te2->te_teif.IsDefault)
  return (1);
else
  return (Stricmp (te1->te_TaskName, te2->te_TaskName));
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/***********************************************************************/

#if defined(__AROS__)
AROS_UFH3(void, SelectElem,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR, unused2, A2),
    AROS_UFHA(APTR, unused1, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static void SelectElem (void)
{
#else
static __saveds __asm void SelectElem (void)
{
#endif
#endif
struct TaskEntry *te = NULL;
SIPTR pos = 0,
     num_entries = 0;

DoMethod (LV_TaskList, MUIM_List_GetEntry, MUIV_List_GetEntry_Active,
          &te);

if (te == NULL)
  /* This can only happen by clearing the list */
  return;

get (LV_TaskList, MUIA_List_Active, &pos);
get (LV_TaskList, MUIA_List_Entries, &num_entries);

set (BT_Top,    MUIA_Disabled, pos <= 1);
set (BT_Up,     MUIA_Disabled, pos <= 1);
set (BT_Down,   MUIA_Disabled, pos == 0 || pos == num_entries - 1);
set (BT_Bottom, MUIA_Disabled, pos == 0 || pos == num_entries - 1);
set (BT_Delete, MUIA_Disabled, pos == 0);

if (te->te_teif.IsDefault)
  {
  set (ST_CurrentTask, MUIA_Disabled, TRUE);
  set (ST_CurrentTask, MUIA_String_Contents, NULL);
  }
else
  {
  set (ST_CurrentTask, MUIA_Disabled, FALSE);
  set (ST_CurrentTask, MUIA_String_Contents, te->te_TaskName);
  }

set (CY_CodeOptions, MUIA_Cycle_Active, te->te_teif.CodePaging);
set (CY_DataOptions, MUIA_Cycle_Active, te->te_teif.DataPaging);
set (ST_MinPublic,    MUIA_String_Integer, te->te_teif.MinPublic);
set (ST_MinNonPublic, MUIA_String_Integer, te->te_teif.MinNonPublic);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/***********************************************************************/

Object *MyButton (IPTR label, IPTR shortcut)

{
/* The KeyButton macro needs a character as its second argument,
 * but _returns a string. This will convert it and
 * call the macro.
 */
const char *tmp;

tmp = _(shortcut);
return (KeyButton (_(label), *tmp));
}

/***********************************************************************/

Object *CreateTaskPage (void)

{
InitHook (&ConstructHook, (HOOKFUNC) ConstructElem);
InitHook (&DestructHook, (HOOKFUNC) DestructElem);
InitHook (&DisplayHook, (HOOKFUNC) DisplayElem);
InitHook (&CompareHook, (HOOKFUNC) CompareElems);
InitHook (&SelectHook, (HOOKFUNC) SelectElem);

DataEntries [0] = CodeEntries [0] = (char *)_(msgDontUseVM);
DataEntries [1] = CodeEntries [1] = (char *)_(msgUseVM);
CodeEntries [2] = NULL;
DataEntries [2] = (char *)_(msgAdvancedOptions);
DataEntries [3] = NULL;

#if defined(__AROS__)  
TaskResBase = OpenResource("task.resource");
#endif

LV_TaskList = ListviewObject,
                MUIA_Listview_Input, TRUE,
                MUIA_Listview_List,
                  ListObject,
                    MUIA_Frame, MUIV_Frame_InputList,
                    MUIA_List_Title, TRUE,
                    MUIA_List_Format, "P=\33c,P=\33c,",
                    MUIA_List_DisplayHook, &DisplayHook,
                    MUIA_List_CompareHook, &CompareHook,
                    MUIA_List_ConstructHook, &ConstructHook,
                    MUIA_List_DestructHook, &DestructHook,
                  End,
              End;

ST_CurrentTask = StringObject,
                   MUIA_Disabled, TRUE,
                   StringFrame,
                   MUIA_String_AttachedList, LV_TaskList,
                   MUIA_String_MaxLen, 10000,
                 End;

GR_TaskList = VGroup,
                Child, LV_TaskList,
                Child, ST_CurrentTask,
              End;
                
GR_OrderButtons = ColGroup (2),
                    Child, BT_AddProg = MyButton (msgAddProg, msgAddProgShort),
                    Child, BT_Top =     MyButton (msgTop, msgTopShort),
                    Child, BT_AddDir =  MyButton (msgAddDir, msgAddDirShort),
                    Child, BT_Up =      MyButton (msgUp, msgUpShort),
                    Child, BT_AddTask = MyButton (msgAddTask, msgAddTaskShort),
                    Child, BT_Down =    MyButton (msgDown, msgDownShort),
                    Child, BT_Add =     MyButton (msgAdd, msgAddShort),
                    Child, BT_Bottom =  MyButton (msgBottom, msgBottomShort),
                    Child, BT_Delete =  MyButton (msgDelete, msgDeleteShort),
                    Child, BT_Sort = MyButton (msgSort, msgSortShort),
                  End;

GR_TaskButtons = VGroup,
                   MUIA_HorizWeight, 10,
                   Child, GR_OrderButtons,
                   Child, VSpace (0),
                 End;

GR_TaskSub = HGroup,
               Child, GR_TaskList,
               Child, GR_TaskButtons,
             End;

GR_VMPermissions = ColGroup (5),
                     Child, Label (_(msgCode)),
                     Child, CY_CodeOptions = 
                       CycleObject,
                         MUIA_HorizWeight, 40,
                         MUIA_Cycle_Entries, CodeEntries,
                       End,
                     Child, HSpace (0), 
                     Child, Label (_(msgMinPublic)),
                     Child, ST_MinPublic = 
                       StringObject,
                         MUIA_Frame, MUIV_Frame_String,
                         MUIA_Disabled, TRUE,
                         MUIA_String_Accept, "-0123456789",
                         MUIA_String_Format, MUIV_String_Format_Right,
                         MUIA_String_Integer, 0,
                       End,
                     Child, Label (_(msgData)),
                     Child, CY_DataOptions =
                       CycleObject,
                         MUIA_HorizWeight, 40,
                         MUIA_Cycle_Entries, DataEntries,
                       End,
                     Child, HSpace (0),
                     Child, Label (_(msgMinNonPublic)),
                     Child, ST_MinNonPublic =
                       StringObject, 
                         MUIA_Frame, MUIV_Frame_String,
                         MUIA_Disabled, TRUE,
                         MUIA_String_Accept, "-0123456789",
                         MUIA_String_Format, MUIV_String_Format_Right,
                         MUIA_String_Integer, 0,
                       End,
                   End;

GR_Tasks = VGroup,
             MUIA_Frame, MUIV_Frame_Group,
             MUIA_HelpNode, "Tasks_Gadget",
             Child, GR_TaskSub,
             Child,
               RectangleObject,
                 MUIA_Rectangle_HBar, TRUE,
                 MUIA_Weight, 0,
               End,
             Child, GR_VMPermissions,
           End;

DoMethod (CY_DataOptions, MUIM_Notify, MUIA_Cycle_Active, DP_FALSE, 
          ST_MinNonPublic, 3, MUIM_Set, MUIA_Disabled, TRUE);
DoMethod (CY_DataOptions, MUIM_Notify, MUIA_Cycle_Active, DP_TRUE, 
          ST_MinNonPublic, 3, MUIM_Set, MUIA_Disabled, TRUE);
DoMethod (CY_DataOptions, MUIM_Notify, MUIA_Cycle_Active, DP_ADVANCED, 
          ST_MinNonPublic, 3, MUIM_Set, MUIA_Disabled, FALSE);
DoMethod (CY_DataOptions, MUIM_Notify, MUIA_Cycle_Active, DP_FALSE, 
          ST_MinPublic, 3, MUIM_Set, MUIA_Disabled, TRUE);
DoMethod (CY_DataOptions, MUIM_Notify, MUIA_Cycle_Active, DP_TRUE, 
          ST_MinPublic, 3, MUIM_Set, MUIA_Disabled, TRUE);
DoMethod (CY_DataOptions, MUIM_Notify, MUIA_Cycle_Active, DP_ADVANCED, 
          ST_MinPublic, 3, MUIM_Set, MUIA_Disabled, FALSE);

DoMethod (LV_TaskList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
          LV_TaskList, 2, MUIM_CallHook, &SelectHook);
return (GR_Tasks);
}

/***********************************************************************/

static void AddTaskToSelectList (struct Task *task)

{
struct Process *cur_proc;
struct CommandLineInterface *CLI;
char *name;
char my_buffer [200];

cur_proc = (struct Process*) task;
CLI = BADDR (cur_proc->pr_CLI);

if ((task->tc_Node.ln_Type == NT_PROCESS) && (CLI != NULL) &&
    (CLI->cli_CommandName != NULL))
  {
  unsigned int len;
  name = AROS_BSTR_ADDR(CLI->cli_CommandName);
  if (name && ((len = AROS_BSTR_strlen(CLI->cli_CommandName)) != 0))
    {
    strncpy (my_buffer, name, len);
    my_buffer [len] = 0;
    name = FilePart (my_buffer);
    }
  else
    name = task->tc_Node.ln_Name;
  }
else
  name = task->tc_Node.ln_Name;

if (strncmp (PROGNAME, name, strlen (PROGNAME)) != 0)
  DoMethod (LV_TaskSelect, MUIM_List_InsertSingle, name,
            MUIV_List_Insert_Bottom);

}

/***********************************************************************/

void BuildTaskList (void)

{
set (WI_Window, MUIA_Window_Sleep, TRUE);
DoMethod (LV_TaskSelect, MUIM_List_Clear);

#if !defined(__AROS__)
struct Node *current;
Forbid ();

for (current = SysBase->TaskReady.lh_Head; current->ln_Succ != NULL;
     current = current->ln_Succ)
  AddTaskToSelectList ((struct Task*) current);

for (current = SysBase->TaskWait.lh_Head; current->ln_Succ != NULL;
     current = current->ln_Succ)
  AddTaskToSelectList (current);

Permit ();
#else
    struct Task*current;
    struct TaskList *systasklist = LockTaskList(LTF_ALL);
    while ((current = NextTaskEntry(systasklist, LTF_ALL)) != NULL)
    {
      AddTaskToSelectList ((struct Task*) current);
    }
    UnLockTaskList(systasklist, LTF_ALL);
#endif

DoMethod (LV_TaskSelect, MUIM_List_Sort);

set (WI_TaskSelect, MUIA_Window_Open, TRUE);
}

/***********************************************************************/

#if defined(__AROS__)
AROS_UFH3(void, EntrySelected,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR, unused2, A2),
    AROS_UFHA(APTR, unused1, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static void EntrySelected (void)
{
#else
static __saveds __asm void EntrySelected (void)
{
#endif
#endif
char *name = NULL;

DoMethod (LV_TaskSelect, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &name);
AddNamed (name);
set (WI_TaskSelect, MUIA_Window_Open, FALSE);
set (WI_Window, MUIA_Window_Sleep, FALSE);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/***********************************************************************/

Object *CreateTaskSelectWindow (void)

{
Object *win;

win = WindowObject,
        MUIA_Window_Title, _(msgSelectTask),
        MUIA_Window_CloseGadget, TRUE,
        MUIA_Window_ID, 2,
        MUIA_HelpNode, "Tasks_Gadget",
        WindowContents, LV_TaskSelect =
          ListviewObject,
            MUIA_Listview_Input, TRUE,
            MUIA_Listview_List,
              ListObject,
                MUIA_Frame, MUIV_Frame_InputList,
                MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                MUIA_List_DestructHook, MUIV_List_DestructHook_String,
              End,
          End,
      End;

DoMethod (win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
          win, 
          3, MUIM_Set, MUIA_Window_Open, FALSE);

InitHook (&EntrySelectedHook, (HOOKFUNC) EntrySelected);

DoMethod (LV_TaskSelect, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
          win,
          2, MUIM_CallHook, &EntrySelectedHook);

return (win);
}

/***********************************************************************/

void AddNamed (char *name)

{
IPTR pos = 0;
struct TaskEntry te,
                *cur_active = NULL;

get (LV_TaskList, MUIA_List_Active, &pos);
DoMethod (LV_TaskList, MUIM_List_GetEntry, pos, &cur_active);

te = *cur_active;
te.te_teif.IsDefault = FALSE;
te.te_TaskName = name;
te.te_teif.NameLen = strlen(name);

set (LV_TaskList, MUIA_List_Quiet, TRUE);
DoMethod (LV_TaskList, MUIM_List_InsertSingle, &te, pos + 1);
set (LV_TaskList, MUIA_List_Active, pos + 1);
set (LV_TaskList, MUIA_List_Quiet, FALSE);
}

