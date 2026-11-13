#ifndef TEXTPARSE_INTERN_H
#define TEXTPARSE_INTERN_H

/* Include files */

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef CLIB_ALIB_PROTOS_H
#   include <clib/alib_protos.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif
#ifndef PROTO_DOS_H
#   include <proto/dos.h>
#endif
#ifndef PROTO_UTILITY_H
#   include <proto/utility.h>
#endif
#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#include <stdlib.h>

struct TextBase_intern
{
    struct Library    library;
    struct ExecBase * sysbase;
    BPTR	      seglist;
    struct IClass    *class;
};

#define IPB(ipb)        ((struct TextBase_intern *)ipb)

#define expunge() \
AROS_LC0(BPTR, expunge, struct TextBase_intern *, TextBase, 3, Text)


#endif /* TEXTPARSE_INTERN_H */
