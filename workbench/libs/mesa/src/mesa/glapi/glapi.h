/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \mainpage Mesa GL API Module
 *
 * \section GLAPIIntroduction Introduction
 *
 * The Mesa GL API module is responsible for dispatching all the
 * gl*() functions.  All GL functions are dispatched by jumping through
 * the current dispatch table (basically a struct full of function
 * pointers.)
 *
 * A per-thread current dispatch table and per-thread current context
 * pointer are managed by this module too.
 *
 * This module is intended to be non-Mesa-specific so it can be used
 * with the X/DRI libGL also.
 */


#ifndef _GLAPI_H
#define _GLAPI_H

#include "glthread.h"


struct _glapi_table;

#if defined(__AROS__) 
#if defined(AROS_MESA_SHARED)

#if defined(__i386__)
register struct MesaBase * REGMesaBase __asm__("ebx");
#elif defined(__x86_64__)
register struct MesaBase * REGMesaBase __asm__("rbx");
#elif defined(PPC) || defined (__powerpc__)
/*
 * r11 or r12 emit call clobbered register warnings
 * r13 works as well, but is small data area pointer
 * in SysV ABI, which we might use in the future
 */
register struct MesaBase * REGMesaBase __asm__("r14");
#else
#error Select register for your architecture
#endif

#define SAVE_REG            struct MesaBase * reg = REGMesaBase;
#define PUT_MESABASE_IN_REG REGMesaBase = (struct MesaBase *)MesaBase;
#define RESTORE_REG         REGMesaBase = reg;
extern void                     **GETMESABASECTX(void);
extern struct _glapi_table      **GETMESABASEDDISPATCH(void);
#else

#define SAVE_REG
#define PUT_MESABASE_IN_REG
#define RESTORE_REG

#endif
#endif

typedef void (*_glapi_proc)(void); /* generic function pointer */


#if defined(USE_MGL_NAMESPACE)
#define _glapi_set_dispatch _mglapi_set_dispatch
#define _glapi_get_dispatch _mglapi_get_dispatch
#define _glapi_set_context _mglapi_set_context
#define _glapi_get_context _mglapi_get_context
#define _glapi_Dispatch _mglapi_Dispatch
#define _glapi_Context _mglapi_Context
#endif


#if defined(__GNUC__)
#  define likely(x)   __builtin_expect(!!(x), 1)
#  define unlikely(x) __builtin_expect(!!(x), 0)
#else
#  define likely(x)   (x)
#  define unlikely(x) (x)
#endif


/**
 ** Define the GET_DISPATCH() and GET_CURRENT_CONTEXT() macros.
 **
 ** \param C local variable which will hold the current context.
 **/
#if defined (GLX_USE_TLS)

extern const struct _glapi_table *_glapi_Dispatch;

extern const void *_glapi_Context;

extern __thread struct _glapi_table * _glapi_tls_Dispatch
    __attribute__((tls_model("initial-exec")));

extern __thread void * _glapi_tls_Context
    __attribute__((tls_model("initial-exec")));

# define GET_DISPATCH() _glapi_tls_Dispatch

# define GET_CURRENT_CONTEXT(C)  GLcontext *C = (GLcontext *) _glapi_tls_Context

#else

#if defined(__AROS__) && defined(AROS_MESA_SHARED)
# if defined(USE_MGL_NAMESPACE)
#  define _mglapi_Context *(GETMESABASECTX())
# define _mglapi_Dispatch *(GETMESABASEDDISPATCH())
# else
#  define _glapi_Context *(GETMESABASECTX())
#  define _glapi_Dispatch *(GETMESABASEDDISPATCH())
# endif
#else
extern struct _glapi_table *_glapi_Dispatch;

extern void *_glapi_Context;
#endif

# ifdef THREADS

#  define GET_DISPATCH() \
     (likely(_glapi_Dispatch) ? _glapi_Dispatch : _glapi_get_dispatch())

#  define GET_CURRENT_CONTEXT(C)  GLcontext *C = (GLcontext *) \
     (likely(_glapi_Context) ? _glapi_Context : _glapi_get_context())

# else

#  define GET_DISPATCH() _glapi_Dispatch

#  define GET_CURRENT_CONTEXT(C)  GLcontext *C = (GLcontext *) _glapi_Context

# endif

#endif /* defined (GLX_USE_TLS) */


/**
 ** GL API public functions
 **/

extern void
_glapi_init_multithread(void);


extern void
_glapi_destroy_multithread(void);


extern void
_glapi_check_multithread(void);


extern void
_glapi_set_context(void *context);


extern void *
_glapi_get_context(void);


extern void
_glapi_set_dispatch(struct _glapi_table *dispatch);


extern struct _glapi_table *
_glapi_get_dispatch(void);


extern unsigned int
_glapi_get_dispatch_table_size(void);


extern int
_glapi_add_dispatch( const char * const * function_names,
		     const char * parameter_signature );

extern int
_glapi_get_proc_offset(const char *funcName);


extern _glapi_proc
_glapi_get_proc_address(const char *funcName);


/**
 * GL API local functions and defines
 */

extern void
init_glapi_relocs_once(void);

extern void
_glapi_check_table_not_null(const struct _glapi_table *table);


extern void
_glapi_check_table(const struct _glapi_table *table);


extern const char *
_glapi_get_proc_name(unsigned int offset);


/*
 * Number of extension functions which we can dynamically add at runtime.
 */
#define MAX_EXTENSION_FUNCS 300


#endif
