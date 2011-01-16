/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#define GL_GLEXT_PROTOTYPES
#define AROSMESA_NO_MANGLING
#include <GL/arosmesa.h>
#include "glapi/glapi.h"
#include "glapi/glapioffsets.h"
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
void GLAPIENTRY gl_dispatch_stub_590(GLenum pname, GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_591(GLenum pname, GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_592(GLenum pname, GLfloat param) {};
void GLAPIENTRY gl_dispatch_stub_593(GLenum pname, const GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_594(GLenum pname, GLint param) {};
void GLAPIENTRY gl_dispatch_stub_595(GLenum pname, const GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_596(GLclampf value, GLboolean invert) {};
void GLAPIENTRY gl_dispatch_stub_597(GLenum pattern) {};
void GLAPIENTRY gl_dispatch_stub_608(GLenum pname, GLdouble * params) {};
void GLAPIENTRY gl_dispatch_stub_609(GLenum pname, GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_634(GLenum mode) {};
void GLAPIENTRY gl_dispatch_stub_676(const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride) {};
void GLAPIENTRY gl_dispatch_stub_677(const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride) {};
void GLAPIENTRY gl_dispatch_stub_678(GLsizei n, const GLuint * fences) {};
void GLAPIENTRY gl_dispatch_stub_679(GLuint fence) {};
void GLAPIENTRY gl_dispatch_stub_680(GLsizei n, GLuint * fences) {};
void GLAPIENTRY gl_dispatch_stub_681(GLuint fence, GLenum pname, GLint * params) {};
GLboolean GLAPIENTRY gl_dispatch_stub_682(GLuint fence) { return GL_TRUE; };
void GLAPIENTRY gl_dispatch_stub_683(GLuint fence, GLenum condition) {};
GLboolean GLAPIENTRY gl_dispatch_stub_684(GLuint fence) { return GL_TRUE; };
void GLAPIENTRY gl_dispatch_stub_765(GLenum face) {};
void GLAPIENTRY gl_dispatch_stub_766(GLuint array) {};
void GLAPIENTRY gl_dispatch_stub_767(GLsizei n, const GLuint * arrays) {};
void GLAPIENTRY gl_dispatch_stub_768(GLsizei n, GLuint * arrays) {};
GLboolean GLAPIENTRY gl_dispatch_stub_769(GLuint array) { return GL_TRUE; };
void GLAPIENTRY gl_dispatch_stub_776(GLclampd zmin, GLclampd zmax) {};
void GLAPIENTRY gl_dispatch_stub_777(GLenum modeRGB, GLenum modeA) {};
void GLAPIENTRY gl_dispatch_stub_795(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {};
void GLAPIENTRY gl_dispatch_stub_796(GLenum target, GLenum pname, GLint param) {};
void GLAPIENTRY gl_dispatch_stub_797(GLenum target, GLintptr offset, GLsizeiptr size) {};
void GLAPIENTRY gl_dispatch_stub_815(GLenum target, GLenum pname, GLvoid ** params) {};
void GLAPIENTRY gl_dispatch_stub_816(GLenum target, GLsizei length, GLvoid * pointer) {};
void GLAPIENTRY gl_dispatch_stub_820(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask) {};
void GLAPIENTRY gl_dispatch_stub_821(GLenum target, GLuint index, GLsizei count, const GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_822(GLenum target, GLuint index, GLsizei count, const GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_823(GLuint id, GLenum pname, GLint64EXT * params) {};
void GLAPIENTRY gl_dispatch_stub_824(GLuint id, GLenum pname, GLuint64EXT * params) {};

