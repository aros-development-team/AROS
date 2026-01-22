#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: find_dev_params.c,v 3.6 95/12/16 18:36:44 Martin_Apel Exp $";

BOOL FindDevParams (char *logical_dev, struct DOSDevParams *ddp)

{
struct DosList *dl;
struct DosEnvec *de;
struct FileSysStartupMsg *fssm;

dl = LockDosList (LDF_DEVICES | LDF_WRITE);

dl = FindDosEntry (dl, logical_dev, LDF_DEVICES);
if ((dl == NULL) ||
    (TypeOfMem (BADDR (dl->dol_misc.dol_handler.dol_Startup)) == 0))
  {
  UnLockDosList (LDF_DEVICES | LDF_WRITE);
  return (FALSE);
  }

fssm = (struct FileSysStartupMsg*) 
                     BADDR (dl->dol_misc.dol_handler.dol_Startup);
de = (struct DosEnvec*) BADDR (fssm->fssm_Environ);
ddp->heads = de->de_Surfaces;
ddp->secs_per_track = de->de_BlocksPerTrack;
ddp->low_cyl = de->de_LowCyl;
ddp->high_cyl = de->de_HighCyl;
ddp->res_start = de->de_Reserved;
ddp->res_end = de->de_PreAlloc;
ddp->block_size = de->de_SizeBlock * sizeof (ULONG);
ddp->secs_per_block = de->de_SectorPerBlock;
ddp->logical_dev_name = (char*)(BADDR (dl->dol_Name)) + 1;
ddp->unit = fssm->fssm_Unit;
ddp->flags = fssm->fssm_Flags;
ddp->MaxTransfer = de->de_MaxTransfer;
Forbid ();
ddp->device = (struct Device*) FindName (&(SysBase->DeviceList),
                          (UBYTE*)((char*)(BADDR (fssm->fssm_Device)) + 1));
Permit ();
if (dl->dol_Task == NULL)
  ddp->SysTask = NULL;
else
  ddp->SysTask = (struct Task*)dl->dol_Task - 1;
UnLockDosList (LDF_DEVICES | LDF_WRITE);
return (TRUE);
}

/***********************************************************************/

BOOL GetPartName (char *part, char *filename)

/* Finds out the partition, on which "filename" resides and puts
 * the partition name into part
 */
{
struct DevProc *PartInfo;
struct Task *DevTask;

if ((PartInfo = GetDeviceProc (filename, NULL)) == NULL)
  return (FALSE);

DevTask = (struct Task*)PartInfo->dvp_Port->mp_SigTask;
strcpy (part, DevTask->tc_Node.ln_Name);

PRINT_DEB ("Converting filename:", 0L);
PRINT_DEB (filename, 0L);
PRINT_DEB (part, 0L);

FreeDeviceProc (PartInfo);
return (TRUE);
}
