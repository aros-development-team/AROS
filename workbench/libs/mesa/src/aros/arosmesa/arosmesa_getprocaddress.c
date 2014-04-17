/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#define GL_GLEXT_PROTOTYPES
#include <GL/arosmesa.h>
#include "glapi/glapi.h"
#define NEED_FUNCTION_POINTER
#include "glapi/glprocs.h"
#include <string.h>

/**
 * Search the table of static entrypoint functions for the named function
 * and return the corresponding glprocs_table_t entry.
 */
static const glprocs_table_t *
find_entry( const char * n )
{
   GLuint i;
   for (i = 0; static_functions[i].Name_offset >= 0; i++) {
      const char *testName = gl_string_table + static_functions[i].Name_offset;
#ifdef MANGLE
      /* skip the "m" prefix on the name */
      if (strcmp(testName, n + 1) == 0)
#else
      if (strcmp(testName, n) == 0)
#endif
      {
         return &static_functions[i];
      }
   }
   return NULL;
}

#ifdef USE_X86_ASM

#if defined( GLX_USE_TLS )
extern       GLubyte gl_dispatch_functions_start[];
extern       GLubyte gl_dispatch_functions_end[];
#else
extern const GLubyte gl_dispatch_functions_start[];
#endif

#endif /* USE_X86_ASM */

/**
 * Return dispatch function address for the named static (built-in) function.
 * Return NULL if function not found.
 */
static _glapi_proc
get_static_proc_address(const char *funcName)
{
   const glprocs_table_t * const f = find_entry( funcName );
   if (f) {
#if defined(DISPATCH_FUNCTION_SIZE) && defined(GLX_INDIRECT_RENDERING)
      return (f->Address == NULL)
     ? (_glapi_proc) (gl_dispatch_functions_start
              + (DISPATCH_FUNCTION_SIZE * f->Offset))
         : f->Address;
#elif defined(DISPATCH_FUNCTION_SIZE)
      return (_glapi_proc) (gl_dispatch_functions_start 
                            + (DISPATCH_FUNCTION_SIZE * f->Offset));
#else
      return f->Address;
#endif
   }
   else {
      return NULL;
   }
}

/* Put declaration here since it should not be present in GL/arosmesa.h */

APTR AROSMesaGetProcAddressInternal(const GLubyte * procname);

AROSMesaProc AROSMesaGetProcAddress(const GLubyte * procname)
{
    if (AROSMesaGetProcAddressInternal(procname) == NULL)
        return NULL;
    else
        return get_static_proc_address(procname);
}

