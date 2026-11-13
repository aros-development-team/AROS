#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: replacement.c,v 3.1 95/03/08 19:52:13 Martin_Apel Rel $";

/*******************************************************************/

int InitReplacementAlgorithm (void)

{
/* Initialize the data structure used by the page replacement algorithm */
}

/*******************************************************************/

PRIVATE BOOL TablePagingAllowed (struct FrameDescr *table_frame)

{
ULONG *cur_descr;
int i;

cur_descr = (ULONG*) table_frame->PhysAddr;
for (i = 0; i < PAGESIZE / sizeof (ULONG); i++, cur_descr++)
  {
  if (PAGE_RESIDENT_P (*cur_descr) || (STATE_FROM_DESCR (*cur_descr) == COMING_IN))
    return (FALSE);
  }
return (TRUE);
}

/*******************************************************************/

struct FrameDescr *FindRemoveablePage (void)

{
/* Finds a page to be evicted specified by the current page replacement
 * algorithm. This page is already removed from the list.
 */
}

/*******************************************************************/

void ReuseSoon (struct FrameDescr *fd)

{
/* Tells the page replacement algorithm to make this frame available soon. */
}

/*******************************************************************/

void ReuseLate (struct FrameDescr *fd)

{
/* Tells the page replacement algorithm to make this frame available
 * as late as possible.
 */
}

/*******************************************************************/

void RemoveFromAvail (struct FrameDescr *fd)

{
/* Removes the page from the list of available frames. */
}
