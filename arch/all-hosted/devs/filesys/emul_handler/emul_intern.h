#ifndef __EMUL_HANDLER_INTERN_H
#define __EMUL_HANDLER_INTERN_H
/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: emul_handler_intern.h 34306 2010-08-31 07:47:37Z sonic $

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

#include <emul_host.h>

struct filehandle
{
    char * hostname;	/* full host's pathname (includes volume root prefix 	  */
    char * name;	/* full name including pathname				  */
    int    type;	/* type flags, see below		       		  */
    char * pathname;	/* if type == FHD_FILE then you'll find the pathname here */
    char * volumename;	/* volume name						  */
    void * fd;		/* Object itself					  */
    ULONG  dirpos;	/* directory position during searching			  */
    struct DosList *dl; /* Volume node						  */
};

/* type flags */
#define FHD_FILE      0x01
#define FHD_DIRECTORY 0x02
#define FHD_STDIO     0x80

struct emulbase
{
    struct Device	      device;
    			      /* nlorentz: Cal it eb_std* because std* is reserved */
    struct filehandle  	     *eb_stdin;
    struct filehandle 	     *eb_stdout;
    struct filehandle 	     *eb_stderr;
    APTR		      mempool;
    APTR		      HostLibBase;
    struct DosLibrary	     *DOSBase;
    struct Emul_PlatformData  pdata;	/* Platform-specific portion */
};

#define HostLibBase emulbase->HostLibBase
#define DOSBase     emulbase->DOSBase

/* File name manipulation functions (filenames.c) */
BOOL shrink(char *filename);
ULONG validate(const char *filename);
char *append(char *c, const char *filename);
long startpos(char *name, long i);
void copyname(char *result, char *name, long i);
char *nextpart(char *sp);

/* Host OS file manipulation functions */
LONG DoOpen(struct emulbase *emulbase, struct filehandle *fh, LONG mode, LONG protect, BOOL AllowDir);
void DoClose(struct emulbase *emulbase, void *file);
void DoCloseDir(struct emulbase *emulbase, void *dir);
LONG DoRead(struct emulbase *emulbase, void *File, void *Buffer, ULONG *Length);
LONG DoAsyncRead(struct emulbase *emulbase, void *File, void *Buffer, ULONG *Length);
LONG DoWrite(struct emulbase *emulbase, void *File, void *Buffer, ULONG *Length);
LONG DoSeek(struct emulbase *emulbase, void *file, UQUAD *Offset, ULONG Mode);
LONG DoMkDir(struct emulbase *emulbase, struct filehandle *fh, ULONG protect);
LONG DoDelete(struct emulbase *emulbase, char *name);
LONG DoChMod(struct emulbase *emulbase, char *filename, ULONG prot);
LONG DoHardLink(struct emulbase *emulbase, char *fn, char *oldfile);
LONG DoSymLink(struct emulbase *emulbase, char *dest, char *src);
LONG DoRename(struct emulbase *emulbase, char *filename, char *newfilename);
int DoReadLink(struct emulbase *emulbase, char *filename, char *buffer, ULONG size, LONG *err);
LONG DoSetDate(struct emulbase *emulbase, char *fullname, struct DateStamp *date);
LONG DoSetSize(struct emulbase *emulbase, struct filehandle *fh, struct IFS_SEEK *io_SEEK);
BOOL DoGetType(struct emulbase *emulbase, void *fd);
LONG DoStatFS(struct emulbase *emulbase, char *path, struct InfoData *id);

extern const ULONG sizes[];

LONG examine(struct emulbase *emulbase, struct filehandle *fh,
             struct ExAllData *ead, ULONG size, ULONG type,
             LONG *dirpos);
LONG examine_next(struct emulbase *emulbase,  struct filehandle *fh, struct FileInfoBlock *FIB);
LONG examine_all(struct emulbase *emulbase, struct filehandle *fh, struct ExAllData *ead,
                  struct ExAllControl *eac, ULONG size, ULONG  type);
LONG examine_all_end(struct emulbase *emulbase, struct filehandle *fh);

char *GetHomeDir(struct emulbase *emulbase, char *user);
ULONG GetCurrentDir(struct emulbase *emulbase, char *path, ULONG len);

#endif /* __EMUL_HANDLER_INTERN_H */
