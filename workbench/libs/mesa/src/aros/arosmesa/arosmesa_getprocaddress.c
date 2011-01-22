/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#define GL_GLEXT_PROTOTYPES
#define AROSMESA_NO_MANGLING
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

AROSMesaProc AROSMesaGetProcAddress(const GLubyte * procname)
{
    return get_static_proc_address(procname);
}

/* Implementation of no-op functions to remove linker problems */
void GLAPIENTRY gl_dispatch_stub_343(GLenum target, GLenum format, GLenum type, GLvoid * table) {};
void GLAPIENTRY gl_dispatch_stub_344(GLenum target, GLenum pname, GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_345(GLenum target, GLenum pname, GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_356(GLenum target, GLenum format, GLenum type, GLvoid * image) {};
void GLAPIENTRY gl_dispatch_stub_357(GLenum target, GLenum pname, GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_358(GLenum target, GLenum pname, GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_359(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span) {};
void GLAPIENTRY gl_dispatch_stub_361(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values) {};
void GLAPIENTRY gl_dispatch_stub_362(GLenum target, GLenum pname, GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_363(GLenum target, GLenum pname, GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_364(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values) {};
void GLAPIENTRY gl_dispatch_stub_365(GLenum target, GLenum pname, GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_366(GLenum target, GLenum pname, GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_601(GLenum pname, GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_602(GLenum pname, GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_603(GLenum pname, GLfloat param) {};
void GLAPIENTRY gl_dispatch_stub_604(GLenum pname, const GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_605(GLenum pname, GLint param) {};
void GLAPIENTRY gl_dispatch_stub_606(GLenum pname, const GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_607(GLclampf value, GLboolean invert) {};
void GLAPIENTRY gl_dispatch_stub_608(GLenum pattern) {};
void GLAPIENTRY gl_dispatch_stub_643(GLenum mode) {};
void GLAPIENTRY gl_dispatch_stub_685(const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride) {};
void GLAPIENTRY gl_dispatch_stub_686(const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride) {};
void GLAPIENTRY gl_dispatch_stub_687(GLsizei n, const GLuint * fences) {};
void GLAPIENTRY gl_dispatch_stub_688(GLuint fence) {};
void GLAPIENTRY gl_dispatch_stub_689(GLsizei n, GLuint * fences) {};
void GLAPIENTRY gl_dispatch_stub_690(GLuint fence, GLenum pname, GLint * params) {};
GLboolean GLAPIENTRY gl_dispatch_stub_691(GLuint fence) { return GL_TRUE; };
void GLAPIENTRY gl_dispatch_stub_692(GLuint fence, GLenum condition) {};
GLboolean GLAPIENTRY gl_dispatch_stub_693(GLuint fence) { return GL_TRUE; };
void GLAPIENTRY gl_dispatch_stub_774(GLenum face) {};
void GLAPIENTRY gl_dispatch_stub_775(GLuint array) {};
void GLAPIENTRY gl_dispatch_stub_776(GLsizei n, const GLuint * arrays) {};
void GLAPIENTRY gl_dispatch_stub_777(GLsizei n, GLuint * arrays) {};
GLboolean GLAPIENTRY gl_dispatch_stub_778(GLuint array) { return GL_TRUE; };
void GLAPIENTRY gl_dispatch_stub_787(GLclampd zmin, GLclampd zmax) {};
void GLAPIENTRY gl_dispatch_stub_788(GLenum modeRGB, GLenum modeA) {};
void GLAPIENTRY gl_dispatch_stub_806(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {};
void GLAPIENTRY gl_dispatch_stub_807(GLenum target, GLenum pname, GLint param) {};
void GLAPIENTRY gl_dispatch_stub_808(GLenum target, GLintptr offset, GLsizeiptr size) {};
void GLAPIENTRY gl_dispatch_stub_866(GLenum target, GLenum pname, GLvoid ** params) {};
void GLAPIENTRY gl_dispatch_stub_867(GLenum target, GLsizei length, GLvoid * pointer) {};
void GLAPIENTRY gl_dispatch_stub_874(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask) {};
void GLAPIENTRY gl_dispatch_stub_875(GLenum target, GLuint index, GLsizei count, const GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_876(GLenum target, GLuint index, GLsizei count, const GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_877(GLuint id, GLenum pname, GLint64EXT * params) {};
void GLAPIENTRY gl_dispatch_stub_878(GLuint id, GLenum pname, GLuint64EXT * params) {};

