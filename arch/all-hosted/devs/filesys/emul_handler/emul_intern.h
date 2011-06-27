#ifndef __EMUL_INTERN_H
#define __EMUL_INTERN_H
/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

#include <resources/emul.h>
#include <resources/emul_host.h>

#include <sys/types.h>

/* FIXME: Remove these #define xxxBase hacks
   Do not use this in new code !
*/
#define HostLibBase emulbase->HostLibBase
#define KernelBase  emulbase->KernelBase

/* File name manipulation functions (filenames.c) */
BOOL shrink(char *filename);
ULONG validate(const char *filename);
char *append(char *c, const char *filename);
long startpos(char *name, long i);
void copyname(char *result, char *name, long i);
char *nextpart(char *sp);

extern const ULONG sizes[];

/* Host OS file manipulation functions */
LONG DoOpen(struct emulbase *emulbase, struct filehandle *fh, LONG mode, LONG protect, BOOL AllowDir);
void DoClose(struct emulbase *emulbase, struct filehandle *fh);
LONG DoRewindDir(struct emulbase *emulbase, struct filehandle *fh);
size_t DoRead(struct emulbase *emulbase, struct filehandle *fh, APTR buff, size_t len, BOOL *async, SIPTR *err);
size_t DoWrite(struct emulbase *emulbase, struct filehandle *fh, CONST_APTR buff, size_t len, BOOL *async, SIPTR *err);
off_t DoSeek(struct emulbase *emulbase, struct filehandle *fh, off_t Offset, ULONG Mode, SIPTR *err);
LONG DoMkDir(struct emulbase *emulbase, struct filehandle *fh, ULONG protect);
LONG DoDelete(struct emulbase *emulbase, char *name);
LONG DoChMod(struct emulbase *emulbase, char *filename, ULONG prot);
LONG DoHardLink(struct emulbase *emulbase, char *fn, char *oldfile);
LONG DoSymLink(struct emulbase *emulbase, char *dest, char *src);
LONG DoRename(struct emulbase *emulbase, char *filename, char *newfilename);
int DoReadLink(struct emulbase *emulbase, char *filename, char *buffer, ULONG size, LONG *err);
LONG DoSetDate(struct emulbase *emulbase, char *fullname, struct DateStamp *date);
off_t DoSetSize(struct emulbase *emulbase, struct filehandle *fh, off_t offset, ULONG mode, SIPTR *err);
LONG DoStatFS(struct emulbase *emulbase, char *path, struct InfoData *id);

LONG DoExamineEntry(struct emulbase *emulbase, struct filehandle *fh, char *EntryName,
		   struct ExAllData *ead, ULONG size, ULONG type);
LONG DoExamineNext(struct emulbase *emulbase,  struct filehandle *fh, struct FileInfoBlock *FIB);
LONG DoExamineAll(struct emulbase *emulbase, struct filehandle *fh, struct ExAllData *ead,
                  struct ExAllControl *eac, ULONG size, ULONG  type);

char *GetHomeDir(struct emulbase *emulbase, char *user);
ULONG GetCurrentDir(struct emulbase *emulbase, char *path, ULONG len);
BOOL CheckDir(struct emulbase *emulbase, char *name);

#endif /* __EMUL_INTERN_H */
