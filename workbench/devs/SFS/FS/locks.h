#ifndef _LOCKS_H
#define _LOCKS_H

#include <dos/dos.h>
#include <exec/types.h>
#include "nodes.h"

struct ExtFileLock {
  BPTR  link;
  NODE  objectnode;
  LONG  access;
  struct MsgPort *task;
  BPTR  volume;
  /* structure above is the same as the FileLock structure */

  struct ExtFileLock *next;
  struct ExtFileLock *prev;
  ULONG id;

  struct GlobalHandle *gh;

  /* EXAMINE_OBJECT and EXAMINE_NEXT use these: */

  NODE  currentnode;
  NODE  nextnode;  /* ObjectNode number, 0 means get first entry */
  BLCK  nextnodeblock;

  /* Used to keep track of the filepointer. */

  BLCK  curextent;     /* This can be zero to indicate the start of the file, even if there ARE extents! */
  ULONG extentoffset;  /* Byte offset in current extent.  0 = start of extent */
  ULONG offset;        /* Byte offset in entire file.  0 = start of file */

  /* If the file-ptr is located at the first byte AFTER the end of the file
     (so the file can be extended) then offset==size.  curextent should in that
     case be the last link the file has (or zero if it has no links) */

  /* Used for setting the Roving Block ptr when a file is closed which was
     extended.  A value of zero indicates the file was not extended.  Otherwise
     the value is the last block of the file + 1. */

  ULONG lastextendedblock;

  UBYTE bits;          /* EFL_MODIFIED : if file was newly created (FINDOUTPUT or FINDUPDATE) or
                                         modified by using ACTION_WRITE or ACTION_SET_FILE_SIZE
                                         then this bit is set.

                          EFL_FILE     : When set, indicates that the lock refers to a file. */

};

#define EFL_MODIFIED  (1)
#define EFL_FILE      (2)



struct GlobalHandle {
  /* Structure which stores information on a specific file.  This structure
     is pointed to by all locks which currently have this file open. */

  struct MinNode node;

  ULONG count;       /* Number of locks referencing this globalhandle */

  NODE  objectnode;  /* The file's ObjectNode */
  ULONG size;        /* Size of the file in bytes */
  ULONG protection;  /* The protection bits of the file */

  ULONG data;        /* The first data block of this file (ExtentBNode) */

  ULONG pad1;        /* Extra bytes to prevent weird CVision3D monitor crash... */
  ULONG pad2;

  // BLCK  startblock; /* First block of this link */
  // UWORD blocks;     /* Blocks in this link */

};

#endif // _LOCKS_H
