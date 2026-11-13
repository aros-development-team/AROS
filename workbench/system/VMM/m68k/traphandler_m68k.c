#include <exec/types.h>
#include "defs.h"

BOOL VMMInstallTrapHandler(void)
{
  ULONG *VectorTable;

  Forbid ();
  VectorTable = (ULONG*)ReadVBR ();
  OrigTrapHandler = (void (*) ()) *(VectorTable + 2);

  PRINT_DEB("VBR=%08lx",(ULONG)VectorTable);

  switch (ProcessorType)
    {
    case PROC_68060: *(VectorTable + 2) = (ULONG)&TrapHandler60;
                     break;
    case PROC_68040: *(VectorTable + 2) = (ULONG)&TrapHandler40;
                     break;
    case PROC_68851:
    case PROC_68030: *(VectorTable + 2) = (ULONG)&TrapHandler30;
                     break;
#ifdef DEBUG
    default:
      PRINT_DEB ("InitPageHandler: Unknown processor type", 0L);
      ColdReboot ();
#endif
    }

  CacheClearU ();
  Permit ();
  PRINT_DEB ("Trap-Vector set", 0L);
  return TRUE;
}

void VMMRemoveTrapHandler(void)
{
  ULONG *VectorTable;

  if (OrigTrapHandler != NULL)
    {
    Forbid ();
    VectorTable = (ULONG*)ReadVBR ();
    *(VectorTable + 2) = (ULONG) OrigTrapHandler;
    CacheClearU ();
    Permit ();
    }
}
