#ifndef _EXEC_PRIVATE_H
#define _EXEC_PRIVATE_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

#if defined(_AMIGA) && defined(__GNUC__)
#   ifndef NO_INLINE_STDARG
#	define NO_INLINE_STDARG
#   endif
#   include "exec_pinline.h"
#else
#   include "exec_pdefs.h"
#endif

/*
    Prototypes
*/
AROS_LP0(void, RawIOInit,
    struct ExecBase *, SysBase, 84, Exec)

AROS_LP0(LONG, RawMayGetChar,
    struct ExecBase *, SysBase, 85, Exec)

AROS_LP1(void, RawPutChar,
    AROS_LPA(UBYTE, chr, D0),
    struct ExecBase *, SysBase, 86, Exec)

AROS_LP1(APTR, TaggedOpenLibrary,
    AROS_LPA(LONG, tag, D0),
    struct ExecBase *, SysBase, 135, Exec)


#endif /* _EXEC_PRIVATE_H */
