#ifndef DOS_INTERN_H
#define DOS_INTERN_H

#include <dos/dosextens.h>

#ifdef SysBase
#undef SysBase
#endif
#define SysBase (DOSBase->dl_SysBase)
#ifdef UtilityBase
#undef UtilityBase
#endif
#define UtilityBase (DOSBase->dl_UtilityBase)

/* Needed for close() */
#define expunge() \
__AROS_LC0(BPTR, expunge, struct DosLibrary *, DOSBase, 3, Dos)

struct DAList
{
    STRPTR *ArgBuf;
    UBYTE *StrBuf;
    STRPTR *MultVec;
};

struct EString
{
    LONG Number;
    STRPTR String;
};

extern struct EString EString[];

#ifndef EOF
#define EOF -1
#endif
#ifndef IOBUFSIZE
#define IOBUFSIZE 4096
#endif

struct vfp
{
    BPTR file;
    LONG count;
};

#define FPUTC(f,c) \
(((struct FileHandle *)BADDR(f))->fh_Flags&FHF_WRITE&& \
 ((struct FileHandle *)BADDR(f))->fh_Pos<((struct FileHandle *)BADDR(f))->fh_End? \
*((struct FileHandle *)BADDR(f))->fh_Pos++=c,0:FPutC(f,c))

#endif
