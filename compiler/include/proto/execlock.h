#ifndef PROTO_EXECLOCK_H
#define PROTO_EXECLOCK_H

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <aros/system.h>

#include <clib/execlock_protos.h>

#ifndef __EXECLOCK_RELLIBBASE__
 #if !defined(__NOLIBBASE__) && !defined(__EXECLOCK_NOLIBBASE__)
  #if !defined(ExecLockBase)
   #ifdef __EXECLOCK_STDLIBBASE__
    extern struct Library *ExecLockBase;
   #else
    extern struct Library *ExecLockBase;
   #endif
  #endif
 #endif
 #ifndef __aros_getbase_ExecLockBase
  #define __aros_getbase_ExecLockBase() (ExecLockBase)
 #endif
#else /* __EXECLOCK_RELLIBASE__ */
 extern const IPTR __aros_rellib_offset_ExecLockBase;
 #define AROS_RELLIB_OFFSET_EXECLOCK __aros_rellib_offset_ExecLockBase
 #define AROS_RELLIB_BASE_EXECLOCK __aros_rellib_base_ExecLockBase
 #ifndef __aros_getbase_ExecLockBase
  #ifndef __aros_getoffsettable
   char *__aros_getoffsettable(void);
  #endif
  #define __aros_getbase_ExecLockBase() (*(struct Library **)(__aros_getoffsettable()+__aros_rellib_offset_ExecLockBase))
 #endif
#endif

#if !defined(NOLIBINLINE) && !defined(EXECLOCK_NOLIBINLINE) && !defined(__EXECLOCK_RELLIBBASE__)
#   include <inline/execlock.h>
#elif !defined(NOLIBDEFINES) && !defined(EXECLOCK_NOLIBDEFINES)
#   include <defines/execlock.h>
#endif

#endif /* PROTO_EXECLOCK_H */
