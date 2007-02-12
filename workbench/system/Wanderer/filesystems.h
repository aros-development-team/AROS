#ifndef WANDERER_FILESYSTEMS_H
#define	WANDERER_FILESYSTEMS_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuitionbase.h>
#include <intuition/classusr.h>
#include <clib/alib_protos.h>
#include <utility/utility.h>
#include <dos/dosextens.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>

/* FILEINFO CONSTANTS */

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
#define  ACTION_GETINFO          16


#define	 PATH_NOINFO			 0
#define	 PATH_RECURSIVE			 1
#define	 PATH_NONRECURSIVE		 2


#define	 PATHBUFFERSIZE		2048
#define	 COPYLEN            131072
#define	 POOLSIZE           COPYLEN * 2


struct	 dCopyStruct 
{
		    char  *spath;
		    char  *dpath;
		    char  *file;
            APTR  userdata;
            BPTR  slock;
            ULONG flags;
            unsigned long long filelen;
			UWORD type;
};

struct  MUIDisplayObjects 
{
    Object              *sourceObject;
    Object              *destObject;
    Object              *fileObject;
    Object              *stopObject;
    Object              *copyApp;
    Object              *performanceObject;
    Object              *win;
    ULONG               stopflag;
    ULONG               numfiles;
    UWORD               action;

    unsigned long long  bytes;
    unsigned int        starttime;
    char                Buffer[120];
};

struct  FileInfo 
{
    ULONG   len;
    ULONG   protection;
    char    *comment;
};

struct FileEntry
{
    struct  FileEntry   *next;
    char    name[1];
};

char  *CombineString(char *format, ...);
void freeString(APTR pool, char *str);

WORD AskChoiceNew(char *title, char *strg, char *gadgets, UWORD sel, BOOL centered);
WORD AskChoice(char *title, char *strg, char *gadgets, UWORD sel);
WORD AskChoiceCentered(char *title, char *strg, char *gadgets, UWORD sel);

BOOL CopyContent(APTR p, char *s, char *d, BOOL makeparentdir, ULONG flags, struct Hook *displayHook, struct Hook *delHook, APTR userdata);

#endif /* WANDERER_FILESYSTEMS_H */
