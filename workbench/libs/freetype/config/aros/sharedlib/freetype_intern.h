#ifndef FREETYPE_INTERN_H
#define FREETYPE_INTERN_H

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

struct IntFreeTypeBase
{
    struct Library 	libnode;
    struct ExecBase 	*sysbase;
    struct Library	*dosbase;
    BPTR 		seglist;
};

#define GetFreeTypeBase(base) ((struct IntFreeTypeBase *)base)

#define DOSBase GetFreeTypeBase(FreeTypeBase)->dosbase

#endif /* FREETYPE_INTERN_H */

