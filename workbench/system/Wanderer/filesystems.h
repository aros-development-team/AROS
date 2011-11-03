#ifndef WANDERER_FILESYSTEMS_H
#define WANDERER_FILESYSTEMS_H

#ifndef __AROS__
#include "portable_macros.h"
#endif

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

#ifdef __AROS__
#include <clib/alib_protos.h>
#endif

#include <utility/utility.h>
#include <dos/dosextens.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/dos.h>
#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#include <proto/muimaster.h>

#ifndef _PROTO_INTUITION_H
#include <proto/intuition.h>
#endif

/* FILEINFO CONSTANTS */

#define OPMODE_ASK          0
#define OPMODE_DELETE       1
#define OPMODE_ALL          2
#define OPMODE_NO           3
#define OPMODE_NONE         4

#define ACCESS_SKIP         OPMODE_DELETE
#define ACCESS_BREAK        OPMODE_NONE

#define FILEINFO_DIR        1
#define FILEINFO_PROTECTED  2
#define FILEINFO_WRITE      4

#define ACTION_COPY         1
#define ACTION_DELETE       2
#define ACTION_DIRTOABS     4
#define ACTION_MAKEDIRS     8
#define ACTION_GETINFO      16
#define ACTION_UPDATE       (1 << 31)

#define PATH_NOINFO         0
#define PATH_RECURSIVE      1
#define PATH_NONRECURSIVE   2


#define PATHBUFFERSIZE      2048
#define COPYLEN             131072
#define POOLSIZE            COPYLEN * 2


struct dCopyStruct
{
    char  *spath;
    char  *dpath;
    char  *file;
    APTR            userdata;
    ULONG           flags;
    ULONG           filelen;
    ULONG           actlen;
    ULONG           totallen;
    UWORD           type;
    unsigned int    difftime;
};

struct MUIDisplayObjects
{
    Object              *sourceObject;
    Object              *destObject;
    Object              *fileObject;
    Object              *stopObject;
    Object              *copyApp;
    Object              *performanceObject;
    Object              *win;
    Object              *gauge;
    ULONG               stopflag;
    ULONG               numfiles;
    ULONG               smallobjects;
    UWORD               action;
    BOOL                updateme;

    unsigned long long  bytes;
    char                Buffer[128];
    char                SpeedBuffer[32];
};

struct FileInfo
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

struct OpModes
{
    struct Hook *askhook;
    WORD        deletemode;
};

char  *CombineString(char *format, ...);
void freeString(APTR pool, char *str);

WORD AskChoiceNew(const char *title, const char *strg, const char *gadgets, UWORD sel, BOOL centered);
WORD AskChoice(const char *title, const char *strg, const char *gadgets, UWORD sel);
WORD AskChoiceCentered(const char *title, const char *strg, const char *gadgets, UWORD sel);

BOOL CopyContent(APTR p, char *s, char *d, BOOL makeparentdir, ULONG flags, struct Hook *displayHook, struct OpModes *opModes, APTR userdata);

#endif /* WANDERER_FILESYSTEMS_H */
