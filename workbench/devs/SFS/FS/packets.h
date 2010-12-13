#ifndef _PACKETS_H
#define _PACKETS_H

#ifdef __AROS__
#define SFS_SPECIFIC_MESSAGE    0xff00
#endif

#define SFS_PACKET_BASE         (0xf00000)

#define SFS_INQUIRY             (SFS_PACKET_BASE + 0x11000)

#define ACTION_GET_TOTAL_BLOCKS (SFS_INQUIRY + 0)
#define ACTION_GET_BLOCKSIZE    (SFS_INQUIRY + 1)
#define ACTION_GET_BLOCKDATA    (SFS_INQUIRY + 2)

#define ACTION_SET_DEBUG        (SFS_PACKET_BASE + 0xBDC0)
#define ACTION_SET_CACHE        (SFS_PACKET_BASE + 0xBDC0 + 1)
#define ACTION_FORMAT_ARGS      (SFS_PACKET_BASE + 0xBDC0 + 2)

/* Above are 'old' packet types.  Below are the new types. */

#define ACTION_SFS_QUERY             (SFS_PACKET_BASE + 1)
#define ACTION_SFS_SET               (SFS_PACKET_BASE + 2)

#define ACTION_SFS_SET_OBJECTBITS    (SFS_PACKET_BASE + 100 + 1)
#define ACTION_SFS_LOCATE_OBJECT     (SFS_PACKET_BASE + 100 + 2)
#define ACTION_SFS_FORMAT            (SFS_PACKET_BASE + 100 + 3)

#define ACTION_SFS_READ_BITMAP       (SFS_PACKET_BASE + 200 + 1)

#define ACTION_SFS_DEFRAGMENT_INIT     (SFS_PACKET_BASE + 300 + 1)
#define ACTION_SFS_DEFRAGMENT_STEP     (SFS_PACKET_BASE + 300 + 2)

/* Tags used by ACTION_SFS_FORMAT: */

#define ASFBASE            (TAG_USER)

#define ASF_NAME           (ASFBASE + 1)  /* The name of the disk.  If this tag is not specified
                                             format will fail with ERROR_INVALID_COMPONENT_NAME. */

#define ASF_NORECYCLED     (ASFBASE + 2)  /* If TRUE, then this tag will prevent the Recycled
                                             directory from being created.  It defaults to creating
                                             the Recycled directory. */

#define ASF_CASESENSITIVE  (ASFBASE + 3)  /* If TRUE, then this tag will use case sensitive file
                                             and directory names.  It defaults to case insensitive
                                             names. */

#define ASF_SHOWRECYCLED   (ASFBASE + 4)  /* If TRUE, then the Recycled directory will be visible
                                             in directory listings.  It defaults to an invisible
                                             Recycled directory. */

#define ASF_RECYCLEDNAME   (ASFBASE + 5)  /* The name of the Recycled directory.  Defaults to
                                             '.recycled'. */

/*

ACTION_SFS_QUERY

arg1: (struct TagItem *); Pointer to a TagList.

res1: DOSTRUE if no error occured.
res2: If res1 is DOSFALSE, then this contains the errorcode.

This packet fills the tags you provided in the taglist with
the desired information.  See query.h for the tags (ASQ_#?).



ACTION_SFS_SET

arg1: (struct TagItem *); Pointer to a TagList

res1: DOSTRUE if no error occured.
res2: If res1 is DOSFALSE, then this contains the errorcode.

This packet allows you to set a number of parameters of the
filesystem.  See query.h for the tags (ASS_#?).



ACTION_SFS_SET_OBJECTBITS

arg1: <unused>
arg2: BPTR to a struct FileLock
arg3: BSTR with the path and objectname
arg4: New value

res1: DOSTRUE if no error occured.
res2: If res1 is DOSFALSE, then this contains the errorcode.

This packet allows you to set SFS specific bits for objects.
At the moment it allows you to set or clear the OTYPE_HIDE
bit which causes objects to be excluded from directory
listings.



ACTION_SFS_LOCATE_OBJECT

arg1: ObjectNode number
arg2: Accessmode

res1: A (normal) pointer to a Lock, or 0 if an error occured.  Don't
      forget that you should use TOBADDR() if you want to pass this
      lock to any DOS functions.
res2: If res1 is 0, then this contains the errorcode.

This packet can be used to obtain a FileLock by ObjectNode
number.  The Recycled directory has a fixed ObjectNode number
RECYCLEDNODE (2) and you can use this packet to obtain a lock
on that directory.

Accessmode is similair to ACTION_LOCATE_OBJECT.  It can be
SHARED_LOCK or EXCLUSIVE_LOCK.



ACTION_SFS_FORMAT

arg1: (struct TagItem *); Pointer to a TagList.



ACTION_SFS_READ_BITMAP

arg1: Pointer to area to store the requested part of the
      bitmap.  Make sure it is large enough to hold the
      requested amount.
arg2: Block number.  The start block of the requested area;
      this must be a multiple of 8.
arg3: Amount.  The amount of blocks your requesting.

res1: DOSTRUE if no error occured.
res2: If res1 is DOSFALSE, then this contains the errorcode.

This packet can be used to read (part of) the bitmap.  Each
set bit indicates a free block, and each cleared bit
indicates a block which is in use.  This function will
return ERROR_BAD_NUMBER if you specified a bad block number
(not a multiple of eight, or a block which is too high).
This error will also be returned if the amount you specified
would run past the end of the partition.



ACTION_SFS_DEFRAGMENT_INIT

res1: DOSTRUE if no error occured.
res2: If res1 is DOSFALSE, then this contains the errorcode.

Resets the defragmenter to the beginning of the disk.



ACTION_SFS_DEFRAGMENT_STEP

arg1: Pointer to area to store the steps the defragmenter
      makes.
arg2: Size of the buffer in longwords(!).  This is not used
      at the moment.  Make sure the buffer consists of atleast
      20 longwords.

res1: DOSTRUE if no error occured.
res2: If res1 is DOSFALSE, then this contains the errorcode.

This packet causes SFS to do one step of the defragmenting
process.  It is possible that 'nothing' seems to happen during
a step.  The buffer pointed to by arg1 is filled with a special
structure which you can use to determine what SFS has done.

A single step looks like this:

 struct DefragmentStep {
   ULONG id;       // id of the step ("MOVE", "DONE" or 0)
   ULONG length;   // length in longwords (can be 0)
   ULONG data[0];  // size of this array is determined by length.
 };

Multiple of these steps can be present in the buffer you gave
in arg1.  The last step will always have an id of 0.

There are 2 types of steps:

 "MOVE", length (3), blocks, source, destination

The MOVE step tells you that a number of blocks have moved from
a source to a new destination.

 "DONE", length (0)

The DONE step tells you that SFS can't defragment any further --
in other words, it is done defragmenting.

In the future new types of steps might be defined.  If you don't
recognize a step, then use the length field to skip it and just
continue processing the other steps.

Example 1:

  "MOVE", 3, 10, 20, 30,
  "MOVE", 3, 1, 55, 56,
  0;

-> There have been 10 blocks moved from block 20 to block 30 and
   there has been 1 block moved from block 55 to block 56.

Example 2:

  0;

-> Nothing has happened, but SFS is not yet done defragmenting
   either.

*/

#endif // _PACKETS_H
