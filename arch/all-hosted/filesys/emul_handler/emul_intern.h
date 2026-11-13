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
#include <dos/exall.h>

#include <emul_host.h>

#include <sys/types.h>

/* Internal form of our handle */
struct filehandle
{
    char * hostname;		/* full host pathname (includes volume root prefix) */
    char * name;		/* full AROS name including pathname		    */
    int    type;		/* type flags, see below		       	    */
    char * volumename;		/* volume name					    */
    void * fd;			/* Object itself				    */
    struct DosList *dl;		/* Volume node					    */
    unsigned int locks;         /* Number of open locks				    */
    struct PlatformHandle ph;	/* Platform-specific data			    */
};

/* type flags */
#define FHD_FILE      0x01
#define FHD_DIRECTORY 0x02
#define FHD_STDIO     0x80

struct emulbase
{
    struct Node		      eb_Node;
    APTR		      eb_stdin;
    APTR		      eb_stdout;
    APTR		      eb_stderr;
    APTR		      mempool;
    APTR		      ReadIRQ;
    APTR		      HostLibBase;
    APTR		      KernelBase;
    struct Emul_PlatformData  pdata;	/* Platform-specific portion */
};

#define HostLibBase emulbase->HostLibBase
#define KernelBase  emulbase->KernelBase

AROS_PROCP(EmulHandlerMain);

/* File name manipulation functions (filenames.c) */
BOOL shrink(char *filename);
ULONG validate(const char *filename);
char *append(char *c, const char *filename);
long startpos(char *name, long i);
void copyname(char *result, char *name, long i);
char *nextpart(char *sp);

extern const ULONG sizes[];

/*
 * Host OS file manipulation functions.
 * Please do not use size_t, off_t, etc, here. They can be different between AROS
 * and host OS, which will cause weird problems, which are hard to detect if you
 * don't know about them. A nice example is Darwin, where off_t is 64-bit wide even
 * on 32-bit machines. On AROS size off_t is 32-bit wide.
 */
LONG DoOpen(struct emulbase *emulbase, struct filehandle *fh, LONG access, LONG mode, LONG protect, BOOL AllowDir);
void DoClose(struct emulbase *emulbase, struct filehandle *fh);
LONG DoRewindDir(struct emulbase *emulbase, struct filehandle *fh);
LONG DoRead(struct emulbase *emulbase, struct filehandle *fh, APTR buff, ULONG len, SIPTR *err);
LONG DoWrite(struct emulbase *emulbase, struct filehandle *fh, CONST_APTR buff, ULONG len, SIPTR *err);
SIPTR DoSeek(struct emulbase *emulbase, struct filehandle *fh, SIPTR Offset, ULONG Mode, SIPTR *err);
LONG DoMkDir(struct emulbase *emulbase, struct filehandle *fh, ULONG protect);
LONG DoDelete(struct emulbase *emulbase, char *name);
LONG DoChMod(struct emulbase *emulbase, char *filename, ULONG prot);
LONG DoHardLink(struct emulbase *emulbase, char *fn, char *oldfile);
LONG DoSymLink(struct emulbase *emulbase, char *dest, char *src);
LONG DoRename(struct emulbase *emulbase, char *filename, char *newfilename);
int DoReadLink(struct emulbase *emulbase, char *filename, char *buffer, ULONG size, LONG *err);
LONG DoSetDate(struct emulbase *emulbase, char *fullname, struct DateStamp *date);
SIPTR DoSetSize(struct emulbase *emulbase, struct filehandle *fh, SIPTR offset, ULONG mode, SIPTR *err);
LONG DoStatFS(struct emulbase *emulbase, char *path, struct InfoData *id);

LONG DoExamineEntry(struct emulbase *emulbase, struct filehandle *fh, char *EntryName,
		   struct ExAllData *ead, ULONG size, ULONG type);
LONG DoExamineNext(struct emulbase *emulbase,  struct filehandle *fh, struct FileInfoBlock *FIB);
LONG DoExamineAll(struct emulbase *emulbase, struct filehandle *fh, struct ExAllData *ead,
                  struct ExAllControl *eac, ULONG size, ULONG type, struct DosLibrary *DOSBase);

char *GetHomeDir(struct emulbase *emulbase, char *user);
ULONG GetCurrentDir(struct emulbase *emulbase, char *path, ULONG len);
BOOL CheckDir(struct emulbase *emulbase, char *name);

#endif /* __EMUL_INTERN_H */
