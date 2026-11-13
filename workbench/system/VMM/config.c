#define DEBUG 1
#include <exec/types.h>
#include <string.h>
#include "defs.h"

#define AT_LEAST_3_3 ((CurrentConfig.Version > 3) || \
                     (CurrentConfig.Version == 3 && CurrentConfig.Revision >= 3))

static char rcsid [] = "$Id: config.c,v 3.6 95/12/16 18:36:55 Martin_Apel Exp $";

/************************************************************************/

PRIVATE int ReadOldTaskEntry (BPTR cfg_file)

{
struct OldTaskEntryInFile old_te;

if (Read (cfg_file, &old_te, sizeof (struct OldTaskEntryInFile)) != 
    sizeof (struct OldTaskEntryInFile))
  return (ERR_CORRUPT_CFG_FILE);

return (EnterTask (old_te.TaskName, old_te.MinPublic, old_te.MinNonPublic, 
        old_te.CodePaging, INSERT_BACK));
}

/************************************************************************/

PRIVATE int ReadNewTaskEntry (BPTR cfg_file)

{
struct TaskEntryInFile teif;
char *name_buf;
int rc;

bug("[VMM-Handler] %s()\n", __func__);

if (Read (cfg_file, &teif, sizeof (struct TaskEntryInFile)) != 
    sizeof (struct TaskEntryInFile))
  return (ERR_CORRUPT_CFG_FILE);

bug("[VMM-Handler] %s: %u bytes\n", __func__, teif.NameLen);

if ((name_buf = AllocMem ((ULONG)teif.NameLen + 1, MEMF_PUBLIC)) == NULL)
  return (ERR_NOT_ENOUGH_MEM);

if (Read (cfg_file, name_buf, (ULONG)teif.NameLen) != teif.NameLen)
  {
  FreeMem (name_buf, (ULONG)teif.NameLen);
  return (ERR_CORRUPT_CFG_FILE);
  }

bug("[VMM-Handler] %s: '%s'\n", __func__, name_buf);
bug("[VMM-Handler] %s: %d min pub, %d non\n", __func__, teif.MinPublic, teif.MinNonPublic);

rc = EnterTask (name_buf, teif.MinPublic, teif.MinNonPublic, teif.CodePaging,
                INSERT_BACK);

FreeMem (name_buf, (ULONG)teif.NameLen + 1);
return (rc);
}

/************************************************************************/

int ReadConfigFile (char *name)

{
int i;
PRIVATE BOOL FirstTime = TRUE;
UWORD OldPageDev;
ULONG OldFileSize;
char OldPartOrFileName [80];
int rc;
BPTR cfg_file;

bug("[VMM-Handler] %s(%s)\n", __func__, name);

DISABLE_VM;

if ((cfg_file = Open (name, MODE_OLDFILE)) == NULL)
  {
  PRINT_DEB ("Couldn't find config file", 0L);
  ENABLE_VM;
  return (ERR_NO_CONFIG_FILE);
  }

if (!FirstTime)
  {
  OldPageDev = CurrentConfig.PageDev;  
  OldFileSize = CurrentConfig.FileSize;
  strcpy (OldPartOrFileName, CurrentConfig.PartOrFileName);
  }

if ((Read (cfg_file, &CurrentConfig, sizeof (struct VMMConfig)) !=
     sizeof (struct VMMConfig)) || (CurrentConfig.CfgMagic != CFG_MAGIC))
  {
  Close (cfg_file);
  ENABLE_VM;
  return (ERR_CORRUPT_CFG_FILE);
  }

if (!FirstTime && 
    ((OldPageDev != CurrentConfig.PageDev) ||
     (OldFileSize != CurrentConfig.FileSize) ||
     (strcmp (OldPartOrFileName, CurrentConfig.PartOrFileName) != 0)))
  {
  ReportError (_(msgPageDevChanged), ERR_NOERROR);
  CurrentConfig.PageDev = OldPageDev;
  CurrentConfig.FileSize = OldFileSize;
  strcpy (CurrentConfig.PartOrFileName, OldPartOrFileName);
  }

bug("[VMM-Handler] %s: adjusting values ...\n", __func__);
  
/* Adjust some of the values to legal ones */
if (CurrentConfig.MinMem < MAX_FAULTS * PAGESIZE)
  CurrentConfig.MinMem = MAX_FAULTS * PAGESIZE;
CurrentConfig.MinMem = ALIGN_DOWN (CurrentConfig.MinMem, PAGESIZE);

if (CurrentConfig.MaxMem < MAX_FAULTS * PAGESIZE)
  CurrentConfig.MaxMem = MAX_FAULTS * PAGESIZE;
CurrentConfig.MaxMem = ALIGN_DOWN (CurrentConfig.MaxMem, PAGESIZE);

/* The following two assignments are done for performance only */
MinVMAlloc = CurrentConfig.MinVMAlloc;
MemTracking = CurrentConfig.MemTracking;

if (CurrentConfig.PageDev == PD_PART)
  {
  strcpy (PartWithColon, CurrentConfig.PartOrFileName);
  strcpy (PartitionName, PartWithColon);
  *(PartitionName + strlen (PartitionName) - 1) = 0;
  }
else
  {
  if (!GetPartName (PartitionName, CurrentConfig.PartOrFileName))
    {
    Close (cfg_file);
    ENABLE_VM;
    return (ERR_VOLUME_NOT_FOUND);
    }
  strcpy (PartWithColon, PartitionName);
  strcat (PartWithColon, ":");
  }

bug("[VMM-Handler] %s: reading task entries ..\n", __func__);
if (AT_LEAST_3_3)
  {
  PRINT_DEB ("Reading config file for V3.3 or later", 0L);

  for (i = 0; i < CurrentConfig.NumTaskEntries; i++)
    {
    if ((rc = ReadNewTaskEntry (cfg_file)) != SUCCESS)    
      {
      Close (cfg_file);
      ENABLE_VM;
      return (rc);
      }
    }
  }
else
  {
  PRINT_DEB ("Reading pre-V3.3 config file", 0L);

  for (i = 0; i < CurrentConfig.NumTaskEntries; i++)
    {
    if ((rc = ReadOldTaskEntry (cfg_file)) != SUCCESS)    
      {
      Close (cfg_file);
      ENABLE_VM;
      return (rc);
      }
    }
  }

bug("[VMM-Handler] %s: config read\n", __func__);

Close (cfg_file);
ENABLE_VM;
FirstTime = FALSE;
return (SUCCESS);
}
