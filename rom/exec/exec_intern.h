#ifndef __EXEC_INTERN_H__
#define __EXEC_INTERN_H__

/* This is a short file that contains a few things every Exec function
    needs */

#ifndef AROS_OPTIONS_H
#   include <aros/options.h>
#endif
#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef CLIB_EXEC_PROTOS_H
#   include <clib/exec_protos.h>
#endif

#if UseLVOs
extern void __AROS_InitExecBase (void);
#endif

#endif /* __EXEC_INTERN_H__ */
