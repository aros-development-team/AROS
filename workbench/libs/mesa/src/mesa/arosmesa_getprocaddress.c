/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#define GL_GLEXT_PROTOTYPES
#undef USE_MGL_NAMESPACE
#include <GL/arosmesa.h>
#include "glapi.h"
#include "glapioffsets.h"
#define NEED_FUNCTION_POINTER
#include "glprocs.h"
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

#if defined(AROS_MESA_SHARED)
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
void GLAPIENTRY gl_dispatch_stub_578(GLenum pname, GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_579(GLenum pname, GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_580(GLenum pname, GLfloat param) {};
void GLAPIENTRY gl_dispatch_stub_581(GLenum pname, const GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_582(GLenum pname, GLint param) {};
void GLAPIENTRY gl_dispatch_stub_583(GLenum pname, const GLint * params) {};
void GLAPIENTRY gl_dispatch_stub_584(GLclampf value, GLboolean invert) {};
void GLAPIENTRY gl_dispatch_stub_585(GLenum pattern) {};
void GLAPIENTRY gl_dispatch_stub_596(GLenum pname, GLdouble * params) {};
void GLAPIENTRY gl_dispatch_stub_597(GLenum pname, GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_622(GLenum mode) {};
void GLAPIENTRY gl_dispatch_stub_664(const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride) {};
void GLAPIENTRY gl_dispatch_stub_665(const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride) {};
void GLAPIENTRY gl_dispatch_stub_666(GLsizei n, const GLuint * fences) {};
void GLAPIENTRY gl_dispatch_stub_667(GLuint fence) {};
void GLAPIENTRY gl_dispatch_stub_668(GLsizei n, GLuint * fences) {};
void GLAPIENTRY gl_dispatch_stub_669(GLuint fence, GLenum pname, GLint * params) {};
GLboolean GLAPIENTRY gl_dispatch_stub_670(GLuint fence) { return GL_TRUE; };
void GLAPIENTRY gl_dispatch_stub_671(GLuint fence, GLenum condition) {};
GLboolean GLAPIENTRY gl_dispatch_stub_672(GLuint fence) { return GL_TRUE; };
void GLAPIENTRY gl_dispatch_stub_753(GLenum face) {};
void GLAPIENTRY gl_dispatch_stub_754(GLuint array) {};
void GLAPIENTRY gl_dispatch_stub_755(GLsizei n, const GLuint * arrays) {};
void GLAPIENTRY gl_dispatch_stub_756(GLsizei n, GLuint * arrays) {};
GLboolean GLAPIENTRY gl_dispatch_stub_757(GLuint array) { return GL_TRUE; } ;
void GLAPIENTRY gl_dispatch_stub_764(GLclampd zmin, GLclampd zmax) {};
void GLAPIENTRY gl_dispatch_stub_765(GLenum modeRGB, GLenum modeA) {};
void GLAPIENTRY gl_dispatch_stub_783(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {};
void GLAPIENTRY gl_dispatch_stub_784(GLenum target, GLenum pname, GLint param) {};
void GLAPIENTRY gl_dispatch_stub_785(GLenum target, GLintptr offset, GLsizeiptr size) {};
void GLAPIENTRY gl_dispatch_stub_796(GLenum target, GLenum pname, GLvoid ** params) {};
void GLAPIENTRY gl_dispatch_stub_797(GLenum target, GLsizei length, GLvoid * pointer) {};
void GLAPIENTRY gl_dispatch_stub_801(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask) {};
void GLAPIENTRY gl_dispatch_stub_802(GLenum target, GLuint index, GLsizei count, const GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_803(GLenum target, GLuint index, GLsizei count, const GLfloat * params) {};
void GLAPIENTRY gl_dispatch_stub_804(GLuint id, GLenum pname, GLint64EXT * params) {};
void GLAPIENTRY gl_dispatch_stub_805(GLuint id, GLenum pname, GLuint64EXT * params) {};
void GLAPIENTRY glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image) {};
void GLAPIENTRY glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image) {};
#endif
