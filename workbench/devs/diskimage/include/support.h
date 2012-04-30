/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SUPPORT_H
#define SUPPORT_H

#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif
#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif
#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif
#ifndef WORKBENCH_WORKBENCH_H
#include <workbench/workbench.h>
#endif
#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif
#ifndef PROTO_LOCALE_H
#include <proto/locale.h>
#endif
#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif
#ifndef __AROS__
#ifndef CLASSES_WINDOW_H
#include <classes/window.h>
#endif
#ifndef GADGETS_LISTBROWSER_H
#include <gadgets/listbrowser.h>
#endif
#endif
#ifndef TYPES_H
#include "types.h"
#endif
#include <SDI_compiler.h>
#include <string.h>

#ifndef __AROS__
#define GetHead(list) (((list) && (list)->lh_Head && (list)->lh_Head->ln_Succ) \
	? (list)->lh_Head : (struct Node *)NULL)
#define GetSucc(node) (((node) && (node)->ln_Succ && (node)->ln_Succ->ln_Succ) \
	? (node)->ln_Succ : (struct Node *)NULL)
#endif
#define ClearMem(ptr,siz) memset(ptr, 0, siz)

#ifndef __amigaos4__
#define IOERR_SUCCESS 0
#endif

/* allocvec_os3.s */
#if !defined(__MORPHOS__) && !defined(__AROS__)
APTR AllocVecPooled(APTR pool, ULONG size);
void FreeVecPooled(APTR pool, APTR mem);
#endif

/* asprintf_os3.s / asprintf_aros.c */
VARARGS68K void SNPrintf (STRPTR buf, LONG len, CONST_STRPTR fmt, ...);
VARARGS68K STRPTR ASPrintf (CONST_STRPTR fmt, ...);
void VSNPrintf (STRPTR buf, LONG len, CONST_STRPTR fmt, CONST_APTR args);
STRPTR VASPrintf (CONST_STRPTR fmt, CONST_APTR args);

/* asprintfpooled_aros.c */
VARARGS68K STRPTR ASPrintfPooled (APTR pool, CONST_STRPTR fmt, ...);
STRPTR VASPrintfPooled (APTR pool, CONST_STRPTR fmt, CONST_APTR args);

/* checklib.c */
BOOL CheckLib (struct Library *lib, ULONG ver, ULONG rev);

/* checkbptr.c */
APTR CheckBPTR (BPTR bptr);

/* copystringbstrtoc.c */
LONG CopyStringBSTRToC (BSTR src, STRPTR dst, ULONG dst_size);

/* deletelibrary.c */
void DeleteLibrary (struct Library *lb);

/* dos64.c */
QUAD GetFileSize (BPTR file);
QUAD GetFilePosition (BPTR file);
int32 ChangeFilePosition (BPTR file, QUAD position, LONG mode);

/* envvar.c */
STRPTR GetEnvVar (CONST_STRPTR name);
BOOL SetEnvVar (CONST_STRPTR name, CONST_STRPTR contents, BOOL envarc);

/* getattrs.c */
VARARGS68K ULONG GetAttrs (Object *obj, IPTR tag1, ...);
ULONG GetAttrsA (Object *obj, const struct TagItem *tags);

/* getcurrentdir.c */
BPTR GetCurrentDir (void);

/* hooks.c */
#define CreateHook(func,data) (struct Hook *)CreateExtHook(sizeof(struct Hook), func, data)
void FreeHook (struct Hook *hook);
APTR CreateExtHook (ULONG size, HOOKFUNC func, APTR data);
#define FreeExtHook(hook) FreeHook((struct Hook *)hook)

/* lists.c */
struct List *CreateList (BOOL min);
void DeleteList (struct List *list);

/* messages.c */
struct Message *CreateMsg (LONG size);
void DeleteMsg (struct Message *msg);

/* mutexes.c */
APTR CreateMutex (void);
void DeleteMutex (APTR mutex);
void MutexObtain (APTR mutex);
void MutexRelease (APTR mutex);
BOOL MutexAttempt (APTR mutex);

/* semaphores.c */
struct SignalSemaphore *CreateSemaphore (void);
void DeleteSemaphore (struct SignalSemaphore *semaphore);

/* setprocwindow.c */
APTR SetProcWindow (APTR window);

/* paths.c */
STRPTR CombinePaths(CONST_STRPTR p1, CONST_STRPTR p2);

/* ports.c */
struct MsgPort *CreatePortNoSignal (void);
void DeletePortNoSignal (struct MsgPort *port);

/* strlcpy.c */
void Strlcpy (STRPTR dst, CONST_STRPTR src, int size);
void Strlcat (STRPTR dst, CONST_STRPTR src, int size);

/* swab2.c */
void swab2 (CONST_APTR source, APTR dest, ULONG bytes);

/* istext.c */
BOOL IsText (const UBYTE *data, LONG len);

/* trimstr.c */
STRPTR TrimStr (STRPTR str);

/* tooltypes.c */
BOOL TTBoolean (struct DiskObject *icon, CONST_STRPTR name);
LONG TTInteger (struct DiskObject *icon, CONST_STRPTR name, LONG def_value);
STRPTR TTString (struct DiskObject *icon, CONST_STRPTR name, STRPTR def_value);

/* diskimagedevice.c */
BOOL OpenDiskImageDevice (ULONG unit_num);
void CloseDiskImageDevice (void);

struct LocaleInfo {
    struct Library     *li_LocaleBase;
    struct Catalog     *li_Catalog;
};

/* localeinfo.c */
void InitLocaleInfo (APTR SysBase, struct LocaleInfo *li, CONST_STRPTR catalog);
void FreeLocaleInfo (APTR SysBase, struct LocaleInfo *li);
CONST_STRPTR GetString (struct LocaleInfo *li, LONG stringNum);

#ifndef STR_ID
#define STR_ID(x) ((STRPTR)(x))
#endif

/* translatefuncs.c */
void TranslateMenus (struct LocaleInfo *li, struct NewMenu *nm);
void TranslateArray (struct LocaleInfo *li, CONST_STRPTR *array);
#ifndef __AROS__
void TranslateHints (struct LocaleInfo *li, struct HintInfo *hi);
void TranslateColumnTitles (struct LocaleInfo *li, struct ColumnInfo *ci);
#endif

/* reallocbuf.c */
APTR ReAllocBuf (APTR buf, ULONG *size, ULONG new_size);

#endif
