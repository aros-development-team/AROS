#ifndef _WANDERER_FILESYSTEMS_H
#define	_WANDERER_FILESYSTEMS_H

#include    <exec/types.h>

/* FILEINFO CONSTANTS */

#define	 COPYLEN 65536
#define	 DELMODE_ASK		  0
#define	 DELMODE_DELETE		  1
#define	 DELMODE_ALL		  2
#define	 DELMODE_NO			  3
#define	 DELMODE_NONE		  4

#define	 ACCESS_SKIP		  DELMODE_DELETE
#define	 ACCESS_BREAK   	  DELMODE_NONE

#define	 FILEINFO_DIR		  1
#define	 FILEINFO_PROTECTED	  2
#define	 FILEINFO_WRITE		  4

#define	 ACTION_COPY		  	 1
#define	 ACTION_DELETE		  	 2
#define	 ACTION_DIRTOABS		 4
#define	 ACTION_MAKEDIRS		 8

#define	 PATH_NOINFO			 0
#define	 PATH_RECURSIVE			 1
#define	 PATH_NONRECURSIVE		 2


#define	 PATHBUFFERSIZE			 8192

         struct dCopyStruct {
             char  *spath;
             char  *dpath;
             char  *file;
             APTR  userdata;
             BPTR  slock;
             ULONG flags;
             unsigned long long filelen;
             UWORD type;
         };

         struct  FileInfo {
             ULONG   len;
             ULONG   protection;
             char    *comment;
         };

            BOOL  actionDir(ULONG flags, char *source, char *dest, BOOL quit, UWORD delmode, UWORD protectmode, UWORD overwritemode, struct Hook *dHook, struct Hook *delHook, APTR userdata);
            BOOL CopyContent(char *s, char *d, BOOL makeparentdir, ULONG flags, struct Hook *displayHook, struct Hook *delHook, APTR userdata);

#endif /* _WANDERER_FILESYSTEMS_H */
