#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: bitmap.c,v 3.5 95/12/16 18:36:59 Martin_Apel Exp $";

PRIVATE ULONG *BitMap;
PRIVATE ULONG *BitMapEnd;
PRIVATE ULONG BitMapSize;

/* The bitmap looks like this:
 * An allocated bit is symbolized by a set bit.
 * There is one extra long-word after the bitmap used as a sentinel
 * The last but one long-word is filled up with ones and the sentinel
 * is zero. BitMapEnd points to the sentinel.
 */

/************************************************************************/

int InitMap (ULONG num_slots)

{
ULONG num_longwords;
int i;

/* Excluding sentinel */
PRINT_DEB ("InitMap (%ld)", num_slots);
num_longwords = (num_slots + 31) / 32;

BitMap = DoOrigAllocMem ((num_longwords + 1) * sizeof (ULONG), MEMF_PUBLIC | MEMF_CLEAR);
if (BitMap == NULL)
  return (ERR_NOT_ENOUGH_MEM);

BitMapEnd = BitMap + num_longwords;

/* Mark last bits as allocated */
if ((num_slots % 32) != 0)
  {
  for (i = num_slots % 32; i < 32; i++)
    *(BitMap + num_longwords - 1) |= (1 << i);
  }

BitMapSize = num_slots;
return (SUCCESS);
}

/************************************************************************/

void KillMap (void)

{
ULONG num_longwords;

num_longwords = (BitMapSize + 31) / 32;

if (BitMap != NULL)
  FreeMem (BitMap, (num_longwords + 1) * sizeof (ULONG));
}

/************************************************************************/

ULONG AllocSlotNextTo (ULONG neighbour)

/* Returns the number of the bit found */
{
ULONG *tmp;
ULONG i;
ULONG mask;

#ifdef DEBUG
if (neighbour >= BitMapSize)
  {
  PRINT_DEB ("AllocBitNextTo with invalid neighbour called: %lx", neighbour);
  ColdReboot();
  }
#endif

tmp = BitMap + neighbour / 32;

while (*tmp == 0xffffffff)
  tmp++;

if (tmp == BitMapEnd)
  {
  tmp = BitMap;
  while (*tmp == 0xffffffff)
    tmp++;
  if (tmp == BitMapEnd)
    {
#ifdef DEBUG
    PRINT_DEB ("AllocBitNextTo: Ran out of bits", 0L);
    ColdReboot ();
#endif
    return (0L);
    }
  }

/* Find first unset bit in this long-word */
mask = 0x1;
for (i = 0; i < 32; i++)
  {
  if ((*tmp & mask) == 0)
    {
    /* Found it */
    *tmp |= mask;
    return ((tmp - BitMap) * 32 + i);
    }
  mask += mask;
  }
#ifdef DEBUG
  PRINT_DEB ("Internal error: AllocBitNextTo", 0L);
  ColdReboot ();
#endif
return (0L);
}

/************************************************************************/

#ifdef __GNUC__
PRIVATE __inline__ ULONG NumContiguousSlots (ULONG slot_num, ULONG needed)
#else
PRIVATE ULONG NumContiguousSlots (ULONG slot_num, ULONG needed)
#endif

{
/* Determines the number of contiguous slots starting at slot 'num'.
 * Uses doubling a mask instead of shifting each time for efficiency.
 */

ULONG *tmp;
ULONG count = 0;
ULONG mask;

tmp = BitMap + slot_num / 32;

/* PRINT_DEB ("Checking contiguous slots starting at %ld", slot_num); */

if ((slot_num % 32) != 0)
  {
  for (mask = (1L << (slot_num % 32)); mask != 0; mask += mask)
    {
    if (*tmp & mask)
      return (count);
    count++;
    }

  tmp++;
  }

if (count >= needed)
  return (count);

while (tmp < BitMapEnd && *tmp == 0L)
  {
  count += 32;
  if (count >= needed)
    return (count);
  tmp++;
  }

if (*tmp == 0xffffffff || tmp == BitMapEnd)
  return (count);

for (mask = 1; mask != 0; mask += mask)
  {
  if (*tmp & mask)
    return (count);
  count++;
  }

/* Should never come here */

PRINT_DEB ("NumContiguousSlots: Internal error", 0L);
#ifdef DEBUG
ColdReboot ();
#endif

return (0L);        /* To please the compiler */
}

/************************************************************************/

PRIVATE void AllocateContiguousSlots (ULONG start_slot, ULONG num_slots)

{
/* Allocates 'num_slots' starting at 'start_slot'. These slots should
 * have been checked to be free previously.
 */

ULONG *tmp;
ULONG mask;

/* PRINT_DEB ("Allocating contiguous slots starting at %ld", start_slot); */
/* PRINT_DEB ("Num_slots = %ld", num_slots); */

tmp = BitMap + start_slot / 32;

if ((start_slot % 32) != 0)
  {
  for (mask = (1L << (start_slot % 32)); mask != 0 && num_slots > 0; mask += mask)
    {
#ifdef DEBUG
    if (*tmp & mask)
      {
      PRINT_DEB ("AllocateContiguousSlots: Trying to allocate already used slot (1)", 0L);
      ColdReboot ();
      }
#endif

    *tmp |= mask;
    num_slots--;
    }

  tmp++;
  }

while (num_slots >= 32)
  {
#ifdef DEBUG
  if (*tmp != 0)
    {
    PRINT_DEB ("AllocateContiguousSlots: Trying to allocate already used slot (2)", 0L);
    ColdReboot ();
    }
#endif

  *tmp++ = 0xffffffff;
  num_slots -= 32;
  }

for (mask = 1; num_slots > 0; mask += mask)
  {
#ifdef DEBUG
  if (*tmp & mask)
    {
    PRINT_DEB ("AllocateContiguousSlots: Trying to allocate already used slot (3)", 0L);
    ColdReboot ();
    }
#endif

  *tmp |= mask;
  num_slots--;
  }
}

/************************************************************************/

ULONG AllocMultipleSlots (ULONG *num_slots)

{
/* Allocates multiple contiguous slots and returns the number of the 
 * first one allocated. If there are not enough contiguous slots 
 * available the largest contiguous chunk is used and its start returned.
 */

ULONG *tmp;
ULONG start_of_largest = 0,
      size_of_largest = 0,
      size;
ULONG i;

for (tmp = BitMap; tmp < BitMapEnd; tmp++)
  {
  if (*tmp == 0xffffffff)
    continue;

  for (i = 0; i < 32; i++)
    {
    if ((*tmp & (1L << i)) == 0)
      {
      size = NumContiguousSlots ((tmp - BitMap) * 32 + i, *num_slots);
      if (size >= *num_slots)
        {
        AllocateContiguousSlots ((tmp - BitMap) * 32 + i, *num_slots);
        return ((tmp - BitMap) * 32 + i);
        }
      else if (size > size_of_largest)
        {
        start_of_largest = (tmp - BitMap) * 32 + i;
        size_of_largest = size;
        break;
        }
      i += size;
      }
    }
  }

/* If we have arrived here it means there is no chunk which is large
 * enough available. Use the largest we have found so far.
 */

#ifdef DEBUG
if (size_of_largest == 0)
  {
  PRINT_DEB ("AllocMultipleSlots: No slot free", 0L);
  ColdReboot ();
  }
#endif

PRINT_DEB ("AllocMultipleSlots: Only %ld contiguous slots available", 
           size_of_largest);
AllocateContiguousSlots (start_of_largest, size_of_largest);
*num_slots = size_of_largest;
return (start_of_largest);
}

/************************************************************************/

void FreeSlot (ULONG num)

{
ULONG *tmp;

/* PRINT_DEB ("Freeing slot %ld", num); */

#ifdef DEBUG
if (num >= BitMapSize)
  {
  PRINT_DEB ("FreeBit with invalid num called: %lx", num);
  ColdReboot();
  }
#endif

tmp = BitMap + num / 32;

#ifdef DEBUG
if (!(*tmp & (1 << (num % 32))))
  {
  PRINT_DEB ("Internal error: Free bit which is already free. Slot %ld", num);
  ColdReboot ();
  }
#endif

*tmp &= ~(1 << (num % 32));
}
