#ifndef BSDSOCKET_INTERN_H
#define BSDSOCKET_INTERN_H

/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Internal definitions for bsdsocket.library
    Lang: English
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
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
#ifndef UTILITY_UTILITY_H
#   include <utility/utility.h>
#endif

/****************************************************************************************/

struct SocketBase {
    struct Library       library;
    BPTR                 sb_SegList;

    struct ExecBase     *sb_SysBase;
    struct UtilityBase  *sb_UtilityBase;
};

/****************************************************************************************/

#undef BSDSB
#define BSDSB(b)    ((struct SocketBase *)b)

#undef SysBase
#define SysBase     (BSDSB(SocketBase)->sb_SysBase)

#undef UtilityBase
#define UtilityBase (BSDSB(SocketBase)->sb_UtilityBase)

/****************************************************************************************/

#endif /* BSDSOCKET_INTERN_H */
