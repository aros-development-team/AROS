#ifndef MUIMASTER_INTERN_H
#define MUIMASTER_INTERN_H

/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: Internal definitions for muimaster.library
    Lang: English
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifdef _AROS
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#else
#define AROS_LIBFUNC_INIT
#define AROS_LIBBASE_EXT_DECL(a,b)
#define AROS_LIBFUNC_EXIT
#endif

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef UTILITY_UTILITY_H
#   include <utility/utility.h>
#endif

/****************************************************************************************/

struct MUIMasterBase_intern
{
    struct Library		library;
    struct ExecBase		*sysbase;
    BPTR			seglist;

    struct UtilityBase		*utilitybase;
};

/****************************************************************************************/

#undef MUIMB
#define MUIMB(b)		((struct MUIMasterBase_intern *)b)

#undef SysBase
#define SysBase		(MUIMB(MUIMasterBase)->sysbase)

#undef UtilityBase
#define UtilityBase	(MUIMB(MUIMasterBase)->utilitybase)

/****************************************************************************************/

#endif /* MUIMASTER_INTERN_H */
