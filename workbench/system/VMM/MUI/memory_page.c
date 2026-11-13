#include "defs.h"

static const char *Policies [4];
static const char *PagingDevice [4];
static const char *MemoryOptions [4];

static struct Hook MinMemHook,
                   MaxMemHook,
                   ConstructHook,
                   DestructHook,
                   DisplayHook,
                   List2StrHook;

struct DriveEntry 
  {
  char name [40];
  ULONG size;            /* in KB */
  };

/********************************************************************/

  #if defined(__AROS__)
AROS_UFH3(void, RaiseHook,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR, unused2, A2),
    AROS_UFHA(APTR, unused1, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static void RaiseHook (void)
{
#else
static __saveds __asm void RaiseHook (void)
{
#endif
#endif

IPTR CurMinMem,
      CurMaxMem;

get (SL_MinMem, MUIA_Slider_Level, &CurMinMem);
get (SL_MaxMem, MUIA_Slider_Level, &CurMaxMem);
if (CurMinMem > CurMaxMem)
  set (SL_MaxMem, MUIA_Slider_Level, CurMinMem);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/********************************************************************/

  #if defined(__AROS__)
AROS_UFH3(void, LowerHook,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR, unused2, A2),
    AROS_UFHA(APTR, unused1, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static void LowerHook (void)
{
#else
static __saveds __asm void LowerHook (void)
{
#endif
#endif

IPTR CurMinMem = 0,
      CurMaxMem = 0;

get (SL_MinMem, MUIA_Slider_Level, &CurMinMem);
get (SL_MaxMem, MUIA_Slider_Level, &CurMaxMem);
if (CurMinMem > CurMaxMem)
  set (SL_MinMem, MUIA_Slider_Level, CurMaxMem);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/********************************************************************/

#if defined(__AROS__)
AROS_UFH3(APTR, ConstructFunc,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR, unused, A2),
    AROS_UFHA(struct DriveEntry *, de, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static APTR ConstructFunc (struct Hook *hook, struct DriveEntry *de)
{
#else
static __saveds __asm APTR ConstructFunc (register __a1 struct DriveEntry *de)
{
#endif
#endif
struct DriveEntry *newEntry;

if ((newEntry = AllocMem (sizeof (struct DriveEntry), MEMF_PUBLIC)) == NULL)
  return (NULL);

strcpy (newEntry->name, de->name);
newEntry->size = de->size;

return (newEntry);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/********************************************************************/

#if defined(__AROS__)
AROS_UFH3(VOID, DestructFunc,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR, unused, A2),
    AROS_UFHA(struct DriveEntry *, de, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static void DestructFunc (struct Hook *hook, struct DriveEntry *de)
{
#else
static __saveds __asm void DestructFunc (register __a1 struct DriveEntry *de)
{
#endif
#endif
FreeMem (de, sizeof (struct DriveEntry));
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/********************************************************************/

#if defined(__AROS__)
AROS_UFH3(APTR, DisplayFunc,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(char **, array, A2),
    AROS_UFHA(struct DriveEntry *, de, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static APTR DisplayFunc (struct Hook *hook, struct DriveEntry *de, char **array)
{
#else
static __saveds __asm LONG DisplayFunc (register __a1 struct DriveEntry *de,
                                        register __a2 char **array)
{
#endif
#endif
static char buf [80];

if (de->size < 2048)
  sprintf (buf, "\033I[6:%ld] %-6s %4ld kB", MUII_HardDisk, 
           de->name, de->size);
else
  sprintf (buf, "\033I[6:%ld] %-6s %4ld MB", MUII_HardDisk, 
           de->name, de->size / 1024);

*array++ = buf;
return (0);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/********************************************************************/

#if defined(__AROS__)
AROS_UFH3(void, List2StrFunc,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(Object *, list, A2),
    AROS_UFHA(Object *, str, A1))
{
    AROS_USERFUNC_INIT
#else
#ifdef __GNUC__
static void List2StrFunc (struct Hook *hook, Object *str, Object *list)
{
#else
static __saveds __asm void List2StrFunc (register __a1 Object *str,
                                         register __a2 Object *list)
{
#endif
#endif
struct DriveEntry *de = NULL;

DoMethod (list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &de);
if (de != NULL)
  set (str, MUIA_String_Contents, de->name);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

/********************************************************************/

Object *CreateMemoryPage (void)

{
InitHook (&ConstructHook, (HOOKFUNC)ConstructFunc);
InitHook (&DestructHook, (HOOKFUNC)DestructFunc);
InitHook (&DisplayHook, (HOOKFUNC)DisplayFunc);
InitHook (&List2StrHook, (HOOKFUNC)List2StrFunc);

Policies [0] = _(msgFixedSize);
Policies [1] = _(msgDynamic);
Policies [2] = _(msgRestrictedDynamic);
Policies [3] = NULL;

PagingDevice [0] = _(msgFile);
PagingDevice [1] = _(msgDevice);
PagingDevice [2] = _(msgPseudoPart);
PagingDevice [3] = NULL;

MemoryOptions [0] = _(msgAny);
MemoryOptions [1] = _(msgChip);
MemoryOptions [2] = _(msgFast);
MemoryOptions [3] = NULL;

GR_File = PopaslObject,
            MUIA_Popstring_String, ST_FileName = KeyString (0, 256, 'f'),
            MUIA_Popstring_Button, PopButton (MUII_PopFile),
            MUIA_Popasl_Type, ASL_FileRequest,
            ASLFR_TitleText, _(msgSelectFile),
          End;

DiskList = ListObject, 
             MUIA_List_ConstructHook, &ConstructHook,
             MUIA_List_DestructHook, &DestructHook,
             MUIA_List_DisplayHook, &DisplayHook,
           End;

PO_Disks = PopobjectObject,
             MUIA_Popobject_Object, LV_Disks =
               ListviewObject,
                 MUIA_Listview_List, DiskList,
                 MUIA_Frame, MUIV_Frame_InputList,
                 MUIA_HelpNode, "SwapMedium_Gadget",
               End,
             MUIA_Popstring_String, ST_PartName = KeyString (0, 60, 'n'),
             MUIA_Popstring_Button, PopButton (MUII_PopUp),
             MUIA_Popobject_ObjStrHook, &List2StrHook,
           End;


GR_Memory = ColGroup (3),
              MUIA_Group_SameWidth, TRUE,
              MUIA_HelpNode, "Memory_Settings",

              Child, Label (_(msgPagingMemory)),
              Child, Label (_(msgMemType)),
              Child, HVSpace,

              Child, CY_Policy   = 
                CycleObject, 
                  MUIA_Cycle_Entries, Policies,
                  MUIA_HelpNode, "MemType_Gadget",
                End,
              Child, CY_MemFlags = 
                CycleObject,
                  MUIA_Cycle_Entries, MemoryOptions,
                  MUIA_HelpNode, "MemFlags_Gadget",
                End,
              Child, CY_Device   = 
                CycleObject,
                  MUIA_Cycle_Entries, PagingDevice,
                  MUIA_HelpNode, "SwapMedium_Gadget",
                End,

              Child, VSpace (0),
              Child, VSpace (0),
              Child, VSpace (0),

              Child, Label (_(msgMinMem)),
              Child, Label (_(msgWriteBuffer)),
              Child, VSpace (0),

              Child, SL_MinMem =
                SliderObject, 
                  MUIA_Slider_Min, 1,
                  MUIA_Slider_Max, 40,
                  MUIA_Slider_Level, 1,
                  MUIA_Slider_Quiet, TRUE,
                  MUIA_FixHeight, TRUE,
                  MUIA_HelpNode, "MemType_Gadget",
                End,
              Child, SL_Buffer =
                SliderObject,
                  MUIA_Slider_Min, 0,
                  MUIA_Slider_Max, 100,
                  MUIA_Slider_Level, 10,
                  MUIA_Slider_Quiet, TRUE,
                  MUIA_FixHeight, TRUE,
                  MUIA_HelpNode, "WriteBuffer_Gadget",
                End,
              Child, GR_DevName =
                PageGroup, 
                  Child, PO_Disks,
                  Child, GR_File,
                  MUIA_Group_ActivePage, 1,
                  MUIA_HelpNode, "SwapMedium_Gadget",
                End,

              Child, TX_MinMem =
                TextObject,
                  MUIA_Text_PreParse, "\033c",
                  MUIA_Text_Contents, "100 KByte",
                End,
              Child, TX_Buffer =
                TextObject,
                  MUIA_Text_PreParse, "\033c",
                  MUIA_Text_Contents, "100 KByte",
                End,
              Child, HVSpace,

              Child, VSpace (0),
              Child, VSpace (0),
              Child, VSpace (0),

              Child, Label (_(msgMaxMem)),
              Child, Label (_(msgPrio)),
              Child, Label (_(msgFileSize)),

              Child, SL_MaxMem =
                SliderObject, 
                  MUIA_Slider_Min, 1,
                  MUIA_Slider_Max, 40,
                  MUIA_Slider_Level, 1,
                  MUIA_Slider_Quiet, TRUE,
                  MUIA_Disabled, TRUE,
                  MUIA_FixHeight, TRUE,
                  MUIA_HelpNode, "MemType_Gadget",
                End,
              Child, SL_Prio =
                SliderObject,
                  MUIA_Slider_Min, -128,
                  MUIA_Slider_Max, 127,
                  MUIA_Slider_Level, 40,
                  MUIA_Slider_Quiet, TRUE,
                  MUIA_FixHeight, TRUE,
                  MUIA_HelpNode, "MemPri_Gadget",
                End,
              Child, SL_FileSize =
                SliderObject,
                  MUIA_Slider_Min, 1,
                  MUIA_Slider_Max, NUM_PTR_TABLES * POINTERS_PER_TABLE * PAGES_PER_TABLE * PAGESIZE
                                   / (1024 * 1024),
                  MUIA_Slider_Level, 1,
                  MUIA_Slider_Quiet, TRUE,
                  MUIA_FixHeight, TRUE,
                  MUIA_Disabled, FALSE,
                  MUIA_HelpNode, "FileSize_Gadget",
                End,
              
              Child, TX_MaxMem =
                TextObject,
                  MUIA_Text_PreParse, "\033c",
                  MUIA_Text_Contents, "100 KByte",
                End,
              Child, TX_Prio =
                TextObject,
                  MUIA_Text_PreParse, "\033c",
                  MUIA_Text_Contents, "40",
                End,
              Child, TX_FileSize =
                TextObject,
                  MUIA_Text_PreParse, "\033c",
                  MUIA_Text_Contents, "1 MByte",
                End,
            End;
              
DoMethod (SL_MinMem, MUIM_Notify, 
          MUIA_Slider_Level, MUIV_EveryTime,
          TX_MinMem,
          4,
          MUIM_SetAsString, MUIA_Text_Contents, "%ld00 KByte", MUIV_TriggerValue);

DoMethod (SL_MaxMem, MUIM_Notify, 
          MUIA_Slider_Level, MUIV_EveryTime,
          TX_MaxMem,
          4,
          MUIM_SetAsString, MUIA_Text_Contents, "%ld00 KByte", MUIV_TriggerValue);

DoMethod (CY_Policy, MUIM_Notify,
          MUIA_Cycle_Active, MT_FIXED, 
          SL_MinMem,
          3,
          MUIM_Set, MUIA_Disabled, FALSE);
DoMethod (CY_Policy, MUIM_Notify,
          MUIA_Cycle_Active, MT_FIXED, 
          SL_MaxMem,
          3,
          MUIM_Set, MUIA_Disabled, TRUE);
DoMethod (CY_Policy, MUIM_Notify,
          MUIA_Cycle_Active, MT_DYNAMIC, 
          SL_MinMem,
          3,
          MUIM_Set, MUIA_Disabled, TRUE);
DoMethod (CY_Policy, MUIM_Notify,
          MUIA_Cycle_Active, MT_DYNAMIC, 
          SL_MaxMem,
          3,
          MUIM_Set, MUIA_Disabled, TRUE);
DoMethod (CY_Policy, MUIM_Notify,
          MUIA_Cycle_Active, MT_RESTRICTED, 
          SL_MinMem,
          3,
          MUIM_Set, MUIA_Disabled, FALSE);
DoMethod (CY_Policy, MUIM_Notify,
          MUIA_Cycle_Active, MT_RESTRICTED, 
          SL_MaxMem,
          3,
          MUIM_Set, MUIA_Disabled, FALSE);

DoMethod (SL_Buffer, MUIM_Notify, 
          MUIA_Slider_Level, MUIV_EveryTime,
          TX_Buffer,
          4,
          MUIM_SetAsString, MUIA_Text_Contents, "%ld0 KByte", MUIV_TriggerValue);

DoMethod (SL_Prio, MUIM_Notify, 
          MUIA_Slider_Level, MUIV_EveryTime,
          TX_Prio,
          4,
          MUIM_SetAsString, MUIA_Text_Contents, "%ld", MUIV_TriggerValue);

DoMethod (LV_Disks, MUIM_Notify, 
          MUIA_Listview_DoubleClick, TRUE,
          PO_Disks, 2, MUIM_Popstring_Close, TRUE);

DoMethod (SL_FileSize, MUIM_Notify, 
          MUIA_Slider_Level, MUIV_EveryTime,
          TX_FileSize,
          4,
          MUIM_SetAsString, MUIA_Text_Contents, "%ld MByte", MUIV_TriggerValue);

DoMethod (CY_Device, MUIM_Notify,
          MUIA_Cycle_Active, PD_PART,
          GR_DevName,
          3,
          MUIM_Set, MUIA_Group_ActivePage, 0);

DoMethod (CY_Device, MUIM_Notify,
          MUIA_Cycle_Active, PD_FILE,
          GR_DevName,
          3,
          MUIM_Set, MUIA_Group_ActivePage, 1);

DoMethod (CY_Device, MUIM_Notify,
          MUIA_Cycle_Active, PD_PSEUDOPART,
          GR_DevName,
          3,
          MUIM_Set, MUIA_Group_ActivePage, 1);

DoMethod (CY_Device, MUIM_Notify,
          MUIA_Cycle_Active, PD_PART,
          SL_FileSize,
          3,
          MUIM_Set, MUIA_Disabled, TRUE);

DoMethod (CY_Device, MUIM_Notify,
          MUIA_Cycle_Active, PD_FILE,
          SL_FileSize,
          3,
          MUIM_Set, MUIA_Disabled, FALSE);

DoMethod (CY_Device, MUIM_Notify,
          MUIA_Cycle_Active, PD_PSEUDOPART,
          SL_FileSize,
          3,
          MUIM_Set, MUIA_Disabled, FALSE);

InitHook (&MinMemHook, (HOOKFUNC)RaiseHook);
InitHook (&MaxMemHook, (HOOKFUNC)LowerHook);

DoMethod (SL_MinMem, MUIM_Notify,
          MUIA_Slider_Level, MUIV_EveryTime,
          SL_MinMem,
          2,
          MUIM_CallHook, &MinMemHook);

DoMethod (SL_MaxMem, MUIM_Notify,
          MUIA_Slider_Level, MUIV_EveryTime,
          SL_MaxMem,
          2,
          MUIM_CallHook, &MaxMemHook);

return (GR_Memory);
}

/***********************************************************************/

void HandleDiskPopup (void)

{
ULONG num_entries;
char *DriveName;
int length;
struct DosList *dl;
struct FileSysStartupMsg *fssm;
struct DosEnvec *de;
static struct DriveEntry ThisDrive;

set (DiskList, MUIA_List_Quiet, TRUE);
get (DiskList, MUIA_List_Entries, &num_entries);

if (num_entries == 0)
  {
  dl = LockDosList (LDF_DEVICES | LDF_WRITE);

  while ((dl = NextDosEntry (dl, LDF_DEVICES | LDF_WRITE)) != NULL)
    {
    DriveName = (char*)(BADDR (dl->dol_Name));
    length = *DriveName++;
    fssm = BADDR (dl->dol_misc.dol_handler.dol_Startup);
    if (TypeOfMem (fssm) != 0)
      {
      de = (struct DosEnvec*) BADDR (fssm->fssm_Environ);
      if ((TypeOfMem (de) != 0) && de->de_SizeBlock == 128L)
        {
        strncpy (ThisDrive.name, DriveName, length);
        ThisDrive.name [length] = 0;
        ThisDrive.size = de->de_Surfaces * de->de_BlocksPerTrack * 
                         (de->de_HighCyl - de->de_LowCyl + 1) / 2;
        DoMethod (DiskList, MUIM_List_InsertSingle, &ThisDrive,
                  MUIV_List_Insert_Bottom);
        }
      }
    }

  UnLockDosList (LDF_DEVICES | LDF_WRITE);
  }

set (DiskList, MUIA_List_Quiet, FALSE);
}
