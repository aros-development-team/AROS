#include <exec/types.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#ifdef __GNUC__
#include "../shared_defs.h"
#include "../protos.h"
#include "../mmu_bits30.h"
#else
#include "/shared_defs.h"
#include "/protos.h"
#include "/mmu_bits30.h"
#endif
void printf (char*, ...);

/* ShowPageSize determines the possible pagesize on a system.
 * It prints an information text to this fact and sets the 
 * return code appropriately.
 * Return code == 1 means 4K is possible
 * Return code == 2 means 8K is possible
 * Return code == 3 means moth sizes are possible
 */

extern struct ExecBase *SysBase;

#define TC40_ENABLE 0x8000
#define TC40_PAGE8K 0x4000


int main (void)

{
struct MMUState30 MMUState30;
struct MMUState40 MMUState40;
BOOL Page4K,
     Page8K;
int ret_val = 0;

/* 68040 and 68060 are the same here */
if (SysBase->AttnFlags & AFF_68040)
  {
  ReadMMUState40 (&MMUState40);
  if (MMUState40.TC & TC40_ENABLE)
    {
    if (MMUState40.TC & TC40_PAGE8K)
      {
      Page8K = TRUE;
      Page4K = FALSE;
      }
    else
      {
      Page8K = FALSE;
      Page4K = TRUE;
      }
    }
  else
    {
    Page8K = TRUE;
    Page4K = TRUE;
    }
  }

else if (SysBase->AttnFlags & AFF_68030)

  {
  ReadMMUState30 (&MMUState30);
  if (MMUState30.TC & (MAKE_MASK(TC_E, TC_E, 1L ) |
                       MAKE_MASK( TC_SRE, TC_SRE, 0L ) |
                       MAKE_MASK( TC_FCL, TC_FCL, 0L ) |
                       MAKE_MASK( TC_PS_START, TC_PS_END, 12) |  /* 4K pages */
                       MAKE_MASK( TC_IS_START, TC_IS_END, 0L ) |
                       MAKE_MASK( TC_TIA_START, TC_TIA_END, LEVEL_A_BITS ) |
                       MAKE_MASK( TC_TIB_START, TC_TIB_END, LEVEL_B_BITS ) |
                       MAKE_MASK( TC_TIC_START, TC_TIC_END, 6) |
                       MAKE_MASK( TC_TID_START, TC_TID_END, 0L )))
    {
    Page8K = FALSE;
    Page4K = TRUE;
    }
  else if (MMUState30.TC & (MAKE_MASK(TC_E, TC_E, 1L ) |
                       MAKE_MASK( TC_SRE, TC_SRE, 0L ) |
                       MAKE_MASK( TC_FCL, TC_FCL, 0L ) |
                       MAKE_MASK( TC_PS_START, TC_PS_END, 13) |  /* 8K pages */
                       MAKE_MASK( TC_IS_START, TC_IS_END, 0L ) |
                       MAKE_MASK( TC_TIA_START, TC_TIA_END, LEVEL_A_BITS ) |
                       MAKE_MASK( TC_TIB_START, TC_TIB_END, LEVEL_B_BITS ) |
                       MAKE_MASK( TC_TIC_START, TC_TIC_END, 5) |
                       MAKE_MASK( TC_TID_START, TC_TID_END, 0L )))
    {
    Page8K = TRUE;
    Page4K = FALSE;
    }
  else
    {
    Page8K = TRUE;
    Page4K = TRUE;
    }
  }

else if ((SysBase->AttnFlags & AFF_68020) && MMU68851)

  {
  ReadMMUState851 (&MMUState30);
  if (MMUState30.TC & (MAKE_MASK(TC_E, TC_E, 1L ) |
                       MAKE_MASK( TC_SRE, TC_SRE, 0L ) |
                       MAKE_MASK( TC_FCL, TC_FCL, 0L ) |
                       MAKE_MASK( TC_PS_START, TC_PS_END, 12) |  /* 4K pages */
                       MAKE_MASK( TC_IS_START, TC_IS_END, 0L ) |
                       MAKE_MASK( TC_TIA_START, TC_TIA_END, LEVEL_A_BITS ) |
                       MAKE_MASK( TC_TIB_START, TC_TIB_END, LEVEL_B_BITS ) |
                       MAKE_MASK( TC_TIC_START, TC_TIC_END, 6) |
                       MAKE_MASK( TC_TID_START, TC_TID_END, 0L )))
    {
    Page8K = FALSE;
    Page4K = TRUE;
    }
  else if (MMUState30.TC & (MAKE_MASK(TC_E, TC_E, 1L ) |
                       MAKE_MASK( TC_SRE, TC_SRE, 0L ) |
                       MAKE_MASK( TC_FCL, TC_FCL, 0L ) |
                       MAKE_MASK( TC_PS_START, TC_PS_END, 13) |  /* 8K pages */
                       MAKE_MASK( TC_IS_START, TC_IS_END, 0L ) |
                       MAKE_MASK( TC_TIA_START, TC_TIA_END, LEVEL_A_BITS ) |
                       MAKE_MASK( TC_TIB_START, TC_TIB_END, LEVEL_B_BITS ) |
                       MAKE_MASK( TC_TIC_START, TC_TIC_END, 5) |
                       MAKE_MASK( TC_TID_START, TC_TID_END, 0L )))
    {
    Page8K = TRUE;
    Page4K = FALSE;
    }
  else
    {
    Page8K = TRUE;
    Page4K = TRUE;
    }
  }  
else
  {
  printf ("You need a MMU to run this program\n");
  return (10);
  }

if (Page4K)
  {
  ret_val = 1;
  printf ("A pagesize of 4K is usable on your system\n");
  }

if (Page8K)
  {
  ret_val += 2;
  printf ("A pagesize of 8K is usable on your system\n");
  }

return (ret_val);
}

void OrigColdReboot (void)         /* just a dummy for sv_regs30.asm */

{
}

void PrintDebugMsg (char *string, unsigned long val)    /* just a dummy for sv_regs30.asm */

{
}
