/*
 * $Id$
 *
 * Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2010 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <proto/dos.h>
#include "flexcat.h"

#ifndef ZERO
# define ZERO (BPTR)NULL
#endif

/// getft

/* Returns the time of change.
   Used for compatibility. */
int32 getft(char *filename)
{
  int32 timestamp = 0;
  #if defined(__amigaos4__)
  struct ExamineData *ed;
  #else // __amigaos4__
  BPTR p_flock;
  struct FileInfoBlock *p_fib;
  #endif // __amigaos4__

  #if defined(__amigaos4__)
  if((ed = ExamineObjectTags(EX_StringNameInput, filename, TAG_DONE)) != NULL)
  {
    timestamp = ed->Date.ds_Days * 86400;                /* days    */
    timestamp += ed->Date.ds_Minute * 60;                /* minutes */
    timestamp += ed->Date.ds_Tick / TICKS_PER_SECOND;    /* seconds */

    FreeDosObject(DOS_EXAMINEDATA, ed);
  }
  #else // __amigaos4__
  if((p_fib = AllocDosObject(DOS_FIB, NULL)) != NULL)
  {
    if((p_flock = Lock(filename, ACCESS_READ)) != ZERO)
    {
      Examine(p_flock, p_fib);

      timestamp = p_fib->fib_Date.ds_Days * 86400;                /* days    */
      timestamp += p_fib->fib_Date.ds_Minute * 60;                /* minutes */
      timestamp += p_fib->fib_Date.ds_Tick / TICKS_PER_SECOND;    /* seconds */

      UnLock(p_flock);
    }

    FreeDosObject(DOS_FIB, p_fib);
  }
  #endif // __amigaos4__

  return timestamp;
}

///
