#ifndef _OBJECTS_H
#define _OBJECTS_H

#include <exec/types.h>
#include <libraries/iffparse.h>
#include <utility/tagitem.h>
#include "blockstructure.h"
#include "nodes.h"

#define OBJECTCONTAINER_ID         AROS_LONG2BE(MAKE_ID('O','B','J','C'))
#define HASHTABLE_ID               AROS_LONG2BE(MAKE_ID('H','T','A','B'))
#define SOFTLINK_ID                AROS_LONG2BE(MAKE_ID('S','L','N','K'))

/* Below is the structure describing an Object.  Objects can be files or
   directories.  Multiple Objects can be stored in an ObjectContainer
   block.

   owneruid & ownergid:  These are not used at the moment.  They have
   been reserved for future use.  They must be set to zero for now.

   objectnode:  This field contains a number uniquely identifying this
   object.  This number can be used to find out the ObjectContainer
   where the Object is stored in.  It is used to refer to an Object
   without having to use BLCK pointers.

   protection:  Contains the Object's protection bits.  By default this
   field is set to 0x0000000F, which means bits R, W, E and D are set
   (note that this is opposite to what is used by AmigaDOS).

   data (files only):  Contains the BLCK number of the first data block of
   a file.  To find out where the rest of the data is located this BLCK
   number can be looked up in a special B+-Tree structure (see below).

   size (files only):  Contains the size of a file in bytes.

   hashtable (directories only):  A BLCK pointer.  It points to a
   HashTable block.

   firstdirblock (directories only):  This BLCK pointer points to the
   first ObjectContainer block which belongs to this directory object.
   For empty directories this field is zero.

   datemodified:  The date of the last modification of this Object.  The
   date is stored in seconds from 1-1-1978 (enough for storing 136 years)

   bits:  See the defines below.  At the moment this field can be checked
   to see if the object is a file or directory.  A bit is reserved for
   links, but isn't currently used (and may or may not be used depending
   on how and if links are implemented).

   name:  Directly following the main structure is the name of the object.
   It is zero terminated.

   comment:  Directly following the name of the object is the comment
   field.  This field is zero terminated as well (even if there is no
   comment). */

struct fsObject {
  UWORD be_owneruid;
  UWORD be_ownergid;
  NODE  be_objectnode;
  ULONG be_protection;

  union {
    struct {
      BLCK  be_data;
      ULONG be_size;
    } file;

    struct {
      BLCK  be_hashtable;   /* for directories & root, 0 means no hashblock */
      BLCK  be_firstdirblock;
    } dir;
  } object;

  ULONG be_datemodified;
  UBYTE bits;

  UBYTE name[0];
  UBYTE comment[0];
};

#define OTYPE_HIDDEN      (1)
#define OTYPE_UNDELETABLE (2)
#define OTYPE_QUICKDIR    (4)
#define OTYPE_RINGLIST    (8)     /* Already partially implemented, but
                                     doesn't seem to be very useful. */
#define OTYPE_HARDLINK    (32)
#define OTYPE_LINK        (64)
#define OTYPE_DIR         (128)

/* hashtable can be zero as well.  If it is zero then the directory
   does not have a hashblock.  This means looking up a file by name
   will be slower since the system will fall-back to scanning the
   complete directory.  However, file creation speed will be higher
   (which includes moving a file to this directory, which happens
   often with the Recycled directory for example). */

/* for all types of objects:
   -------------------------

   HIDDEN: The Object won't be returned by EXAMINE_NEXT or
           EXAMINE_ALL.

   UNDELETABLE: ACTION_DELETE_OBJECT will return an error for Objects
                protected by this bit.

   for directories only:
   ---------------------

   QUICKDIR: Entries are added at the start of the directory without
             checking if there is room anywhere else in the dir. */

/* SFS supports Soft links.  When OTYPE_LINK is set and OTYPE_HARDLINK is
   clear then the entry is a soft link.  The path of the soft link isn't
   stored with the directory entry, but instead is stored as the file
   data. */



/* The fsObjectContainer structure is used to hold various Objects which
   have the same parent directory.  Objects always start at 2-byte
   boundaries, which means sometimes a padding byte is inserted between
   2 fsObject structures.  The next & previous fields link all
   ObjectContainers in a doubly linked list.

   parent:  The node-number of the parent Object.  The node number can be
   used to lookup the BLCK number of the block where the parent Object is
   located.

   next:  The next ObjectContainer belonging to this directory or zero if
   it is the last in the chain.

   previous:  The previous ObjectContainer belonging to this directory or
   zero if it is the first ObjectContainer.

   object:  A variable number of fsObject structures, depending on their
   sizes and on the size of the block.  The next object structure can be
   found by creating a pointer pointing to the name field of the current
   object, then skip 2 strings (name & comment) and if the address is odd,
   adding 1. */

struct fsObjectContainer {
  struct fsBlockHeader bheader;

  NODE be_parent;
  BLCK be_next;
  BLCK be_previous;   /* 0 for the first block in the directory chain */

  struct fsObject object[0];
};



/* fsHashTable is the structure of a HashTable block.  It functions much
   like the HashTable found in FFS, except that it is stored in a seperate
   block.  This block contains a number of hash-chains (about 120 for a
   512 byte block).  Each hash-chain is a chain of Nodes.  Each Node
   contains a BLCK pointer to an Object and the node number of the next
   entry in the hash-chain.

   parent:  The node-number of the parent object.

   hashentry:  The node-number of the first entry of a specific
   hash-chain. */

struct fsHashTable {
  struct fsBlockHeader bheader;

  NODE be_parent;

  NODE be_hashentry[0];
};



struct fsSoftLink {
  struct fsBlockHeader bheader;

  NODE be_parent;
  BLCK be_next;
  BLCK be_previous;

  UBYTE string[0];
};



struct fsObjectNode {
  struct fsNode node;
  NODE  be_next;
  UWORD be_hash16;
} __attribute__((packed));



/* Tags used by createobjecttags: */

#define COBASE            (TAG_USER)

#define CO_OBJECTNODE     (COBASE+1)   /* The ObjectNode of the object.  Defaults to creating a
                                          new ObjectNode.  If an ObjectNode is passed then its
                                          data field will be set to point to the block which
                                          contains the new Object. */

#define CO_PROTECTION     (COBASE+2)   /* The protection bits of the object.  Defaults to
                                          FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE. */

#define CO_DATEMODIFIED   (COBASE+3)   /* The modification data of the object.  Defaults to
                                          current date. */

#define CO_OWNER          (COBASE+4)   /* The owneruid and ownergid of the object.  Default to
                                          zero. */

#define CO_COMMENT        (COBASE+5)   /* Comment of the new object (79 chars max).  Defaults to
                                          no comment. */

#define CO_BITS           (COBASE+6)   /* See bits in fsObject. */

#define CO_HASHOBJECT     (COBASE+7)   /* If not FALSE, then the new Object will be hashed
                                          automatically.  Defaults to FALSE. */

#define CO_UPDATEPARENT   (COBASE+8)   /* If not FALSE, then the new Object's Parent's archive
                                          bit will be cleared and its modification date will be
                                          updated.  Defaults to FALSE. */

#define CO_ALLOWDUPLICATES (COBASE+9)  /* If not FALSE, then createobjecttags() won't check to
                                          see if an object already exists.  Defaults to FALSE. */

  /* File specific tags: */

#define CO_SIZE           (COBASE+20)   /* The size of the file.  Defaults to zero.  Ignored if
                                          CO_BITS indicates its a directory. */

#define CO_DATA           (COBASE+21)   /* The first block of the file.  Defaults to zero.
                                          Ignored if CO_BITS indicates its a directory. */

  /* Directory specific tags: */

#define CO_FIRSTDIRBLOCK  (COBASE+30)  /* The first ObjectContainer of the directory.  Defaults
                                          to zero.  Ignored if CO_BITS indicates its a file. */

#define CO_HASHBLOCK      (COBASE+31)  /* The Hashblock of the directory.  Defaults to creating
                                          a new Hashblock.  Ignored if CO_BITS indicates its a
                                          file. */

  /* Softlink specific tags: */

#define CO_SOFTLINK       (COBASE+40)  /* The Softlink string.  No default!  Ignored if CO_BITS
                                          indicates its not a softlink. */

#endif // _OBJECTS_H
