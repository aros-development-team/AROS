/*
 *      $Id$
 *
 *      fibex.h - extensions to FIB (File Information Block)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <dos/dosextens.h>

/*
 * This is already in 3.0 includes
 */
#ifndef FIBB_OTR_READ
#define FileInfoBlock myFileInfoBlock

/* Returned by Examine() and ExNext(), must be on a LONG boundary */
struct FileInfoBlock {
   LONG	  fib_DiskKey;
   LONG	  fib_DirEntryType;
   char	  fib_FileName[108]; 
   LONG	  fib_Protection;    
   LONG	  fib_EntryType;
   LONG	  fib_Size;	     
   LONG	  fib_NumBlocks;     
   struct DateStamp fib_Date;
   char	  fib_Comment[80];   
   /* Note: the following fields are not supported by all filesystems.	*/
   /* They should be initialized to 0 sending an ACTION_EXAMINE packet.	*/
   /* When Examine() is called, these are set to 0 for you.		*/
   /* AllocDosObject() also initializes them to 0.			*/
   UWORD  fib_OwnerUID;		/* owner's UID */
   UWORD  fib_OwnerGID;		/* owner's GID */
   char	  fib_Reserved[32];
}; /* FileInfoBlock */

/* FIB stands for FileInfoBlock */

/* FIBB are bit definitions, FIBF are field definitions */
/* Regular RWED bits are 0 == allowed. */
/* NOTE: GRP and OTR RWED permissions are 0 == not allowed! */
/* Group and Other permissions are not directly handled by the filesystem */
#define FIBB_OTR_READ	   15	/* Other: file is readable */
#define FIBB_OTR_WRITE	   14	/* Other: file is writable */
#define FIBB_OTR_EXECUTE   13	/* Other: file is executable */
#define FIBB_OTR_DELETE    12	/* Other: prevent file from being deleted */
#define FIBB_GRP_READ	   11	/* Group: file is readable */
#define FIBB_GRP_WRITE	   10	/* Group: file is writable */
#define FIBB_GRP_EXECUTE   9	/* Group: file is executable */
#define FIBB_GRP_DELETE    8	/* Group: prevent file from being deleted */

#define FIBF_OTR_READ	   (1<<FIBB_OTR_READ)
#define FIBF_OTR_WRITE	   (1<<FIBB_OTR_WRITE)
#define FIBF_OTR_EXECUTE   (1<<FIBB_OTR_EXECUTE)
#define FIBF_OTR_DELETE    (1<<FIBB_OTR_DELETE)
#define FIBF_GRP_READ	   (1<<FIBB_GRP_READ)
#define FIBF_GRP_WRITE	   (1<<FIBB_GRP_WRITE)
#define FIBF_GRP_EXECUTE   (1<<FIBB_GRP_EXECUTE)
#define FIBF_GRP_DELETE    (1<<FIBB_GRP_DELETE)

#endif /* defined(FIBB_OTR_READ) */

/*
 * These are probably defined only for MuFS
 */
#define FIBB_SUID 31
#define FIBB_SGID 30		/* in MuFS 2, perhaps? */

#define FIBF_SUID (1<<FIBB_SUID)
#define FIBF_SGID (1<<FIBB_SGID)

void __dostat(struct FileInfoBlock *fib, struct stat *st);
extern struct FileInfoBlock __dostat_fib[];

