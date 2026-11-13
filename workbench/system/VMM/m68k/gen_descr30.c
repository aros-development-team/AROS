#include <exec/types.h>
#include "defs.h"

ULONG GenDescr30 (ULONG address)

{
/* This function generates an MMU descriptor for the given address with
 * same cache mode and write protection status as the current MMU mapping.
 */

ULONG tc, 
      tt0, 
      tt1, 
      last_descr;
UWORD read_stat0,        /* Contents of MMUSR after a ptestr #0 */
      write_stat0,       /* Contents of MMUSR after a ptestw #0 */
      rw_stat7;          /* Contents of MMUSR after a ptestr #7 */
ULONG descr;
BOOL read_done = FALSE;
     write_done = FALSE;

if (ProcessorType == PROC_68030)
  {
  GetAddrStatus30 (&tc, &tt0, &tt1, &read_stat0, &write_stat0, &rw_stat7, &last_descr);

  /* Check if the current address is mapped by one of TT registers */
  
  }
else
  GetAddrStatus30 (&tc, &rw_stat7, &last_descr);

