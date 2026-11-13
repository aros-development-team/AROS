/*
    Copyright 2011-2025, The AROS Development Team. All rights reserved.
*/

#include "hostgl_ctx_manager.h"
#include "glx_hostlib.h"
#include <aros/debug.h>

#define HOSTGL_PRE                                          \
    HostGL_Lock();                                          \
    HostGL_UpdateGlobalGLXContext();

#define HOSTGL_POST                                         \
    HostGL_UnLock();

void glClearIndex (GLfloat c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearIndex", __func__, FindTask(NULL)));
    GLCALL(glClearIndex, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearColor", __func__, FindTask(NULL)));
    GLCALL(glClearColor, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClear (GLbitfield mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClear", __func__, FindTask(NULL)));
    GLCALL(glClear, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexMask (GLuint mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexMask", __func__, FindTask(NULL)));
    GLCALL(glIndexMask, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorMask", __func__, FindTask(NULL)));
    GLCALL(glColorMask, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glAlphaFunc (GLenum func, GLclampf ref)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAlphaFunc", __func__, FindTask(NULL)));
    GLCALL(glAlphaFunc, func, ref);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendFunc (GLenum sfactor, GLenum dfactor)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendFunc", __func__, FindTask(NULL)));
    GLCALL(glBlendFunc, sfactor, dfactor);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLogicOp (GLenum opcode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLogicOp", __func__, FindTask(NULL)));
    GLCALL(glLogicOp, opcode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCullFace (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCullFace", __func__, FindTask(NULL)));
    GLCALL(glCullFace, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFrontFace (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFrontFace", __func__, FindTask(NULL)));
    GLCALL(glFrontFace, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointSize (GLfloat size)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointSize", __func__, FindTask(NULL)));
    GLCALL(glPointSize, size);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLineWidth (GLfloat width)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLineWidth", __func__, FindTask(NULL)));
    GLCALL(glLineWidth, width);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLineStipple (GLint factor, GLushort pattern)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLineStipple", __func__, FindTask(NULL)));
    GLCALL(glLineStipple, factor, pattern);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPolygonMode (GLenum face, GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPolygonMode", __func__, FindTask(NULL)));
    GLCALL(glPolygonMode, face, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPolygonOffset (GLfloat factor, GLfloat units)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPolygonOffset", __func__, FindTask(NULL)));
    GLCALL(glPolygonOffset, factor, units);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPolygonStipple (const GLubyte * mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPolygonStipple", __func__, FindTask(NULL)));
    GLCALL(glPolygonStipple, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetPolygonStipple (GLubyte * mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetPolygonStipple", __func__, FindTask(NULL)));
    GLCALL(glGetPolygonStipple, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEdgeFlag (GLboolean flag)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEdgeFlag", __func__, FindTask(NULL)));
    GLCALL(glEdgeFlag, flag);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEdgeFlagv (const GLboolean * flag)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEdgeFlagv", __func__, FindTask(NULL)));
    GLCALL(glEdgeFlagv, flag);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glScissor", __func__, FindTask(NULL)));
    GLCALL(glScissor, x, y, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClipPlane (GLenum plane, const GLdouble * equation)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClipPlane", __func__, FindTask(NULL)));
    GLCALL(glClipPlane, plane, equation);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetClipPlane (GLenum plane, GLdouble * equation)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetClipPlane", __func__, FindTask(NULL)));
    GLCALL(glGetClipPlane, plane, equation);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawBuffer (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawBuffer", __func__, FindTask(NULL)));
    GLCALL(glDrawBuffer, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glReadBuffer (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glReadBuffer", __func__, FindTask(NULL)));
    GLCALL(glReadBuffer, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEnable (GLenum cap)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEnable", __func__, FindTask(NULL)));
    GLCALL(glEnable, cap);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDisable (GLenum cap)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDisable", __func__, FindTask(NULL)));
    GLCALL(glDisable, cap);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsEnabled (GLenum cap)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsEnabled", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsEnabled, cap);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glEnableClientState (GLenum cap)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEnableClientState", __func__, FindTask(NULL)));
    GLCALL(glEnableClientState, cap);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDisableClientState (GLenum cap)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDisableClientState", __func__, FindTask(NULL)));
    GLCALL(glDisableClientState, cap);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetBooleanv (GLenum pname, GLboolean * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBooleanv", __func__, FindTask(NULL)));
    GLCALL(glGetBooleanv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetDoublev (GLenum pname, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetDoublev", __func__, FindTask(NULL)));
    GLCALL(glGetDoublev, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetFloatv (GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetFloatv", __func__, FindTask(NULL)));
    GLCALL(glGetFloatv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetIntegerv (GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetIntegerv", __func__, FindTask(NULL)));
    GLCALL(glGetIntegerv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPushAttrib (GLbitfield mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPushAttrib", __func__, FindTask(NULL)));
    GLCALL(glPushAttrib, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPopAttrib ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPopAttrib", __func__, FindTask(NULL)));
    GLCALL(glPopAttrib);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPushClientAttrib (GLbitfield mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPushClientAttrib", __func__, FindTask(NULL)));
    GLCALL(glPushClientAttrib, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPopClientAttrib ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPopClientAttrib", __func__, FindTask(NULL)));
    GLCALL(glPopClientAttrib);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLint glRenderMode (GLenum mode)
{
    GLint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRenderMode", __func__, FindTask(NULL)));
    _ret = GLCALL(glRenderMode, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

GLenum glGetError ()
{
    GLenum _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetError", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetError);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

const GLubyte * glGetString (GLenum name)
{
    const GLubyte * _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetString", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetString, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glFinish ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFinish", __func__, FindTask(NULL)));
    GLCALL(glFinish);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFlush ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFlush", __func__, FindTask(NULL)));
    GLCALL(glFlush);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glHint (GLenum target, GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glHint", __func__, FindTask(NULL)));
    GLCALL(glHint, target, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearDepth (GLclampd depth)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearDepth", __func__, FindTask(NULL)));
    GLCALL(glClearDepth, depth);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDepthFunc (GLenum func)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDepthFunc", __func__, FindTask(NULL)));
    GLCALL(glDepthFunc, func);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDepthMask (GLboolean flag)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDepthMask", __func__, FindTask(NULL)));
    GLCALL(glDepthMask, flag);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDepthRange (GLclampd near_val, GLclampd far_val)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDepthRange", __func__, FindTask(NULL)));
    GLCALL(glDepthRange, near_val, far_val);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearAccum", __func__, FindTask(NULL)));
    GLCALL(glClearAccum, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glAccum (GLenum op, GLfloat value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAccum", __func__, FindTask(NULL)));
    GLCALL(glAccum, op, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMatrixMode (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMatrixMode", __func__, FindTask(NULL)));
    GLCALL(glMatrixMode, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glOrtho", __func__, FindTask(NULL)));
    GLCALL(glOrtho, left, right, bottom, top, near_val, far_val);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFrustum", __func__, FindTask(NULL)));
    GLCALL(glFrustum, left, right, bottom, top, near_val, far_val);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glViewport", __func__, FindTask(NULL)));
    GLCALL(glViewport, x, y, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPushMatrix ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPushMatrix", __func__, FindTask(NULL)));
    GLCALL(glPushMatrix);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPopMatrix ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPopMatrix", __func__, FindTask(NULL)));
    GLCALL(glPopMatrix);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLoadIdentity ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLoadIdentity", __func__, FindTask(NULL)));
    GLCALL(glLoadIdentity);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLoadMatrixd (const GLdouble * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLoadMatrixd", __func__, FindTask(NULL)));
    GLCALL(glLoadMatrixd, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLoadMatrixf (const GLfloat * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLoadMatrixf", __func__, FindTask(NULL)));
    GLCALL(glLoadMatrixf, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultMatrixd (const GLdouble * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultMatrixd", __func__, FindTask(NULL)));
    GLCALL(glMultMatrixd, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultMatrixf (const GLfloat * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultMatrixf", __func__, FindTask(NULL)));
    GLCALL(glMultMatrixf, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRotated", __func__, FindTask(NULL)));
    GLCALL(glRotated, angle, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRotatef", __func__, FindTask(NULL)));
    GLCALL(glRotatef, angle, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glScaled (GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glScaled", __func__, FindTask(NULL)));
    GLCALL(glScaled, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glScalef (GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glScalef", __func__, FindTask(NULL)));
    GLCALL(glScalef, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTranslated (GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTranslated", __func__, FindTask(NULL)));
    GLCALL(glTranslated, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTranslatef", __func__, FindTask(NULL)));
    GLCALL(glTranslatef, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsList (GLuint list)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsList", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsList, list);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glDeleteLists (GLuint list, GLsizei range)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteLists", __func__, FindTask(NULL)));
    GLCALL(glDeleteLists, list, range);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLuint glGenLists (GLsizei range)
{
    GLuint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenLists", __func__, FindTask(NULL)));
    _ret = GLCALL(glGenLists, range);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glNewList (GLuint list, GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNewList", __func__, FindTask(NULL)));
    GLCALL(glNewList, list, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEndList ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEndList", __func__, FindTask(NULL)));
    GLCALL(glEndList);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCallList (GLuint list)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCallList", __func__, FindTask(NULL)));
    GLCALL(glCallList, list);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCallLists (GLsizei n, GLenum type, const GLvoid * lists)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCallLists", __func__, FindTask(NULL)));
    GLCALL(glCallLists, n, type, lists);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glListBase (GLuint base)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glListBase", __func__, FindTask(NULL)));
    GLCALL(glListBase, base);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBegin (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBegin", __func__, FindTask(NULL)));
    GLCALL(glBegin, mode);
    D(bug(" ->returned\n"));
    /* glBegin/glEnd must be atomic */
}

void glEnd ()
{
    /* glBegin/glEnd must be atomic */
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEnd", __func__, FindTask(NULL)));
    GLCALL(glEnd);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex2d (GLdouble x, GLdouble y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex2d", __func__, FindTask(NULL)));
    GLCALL(glVertex2d, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex2f (GLfloat x, GLfloat y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex2f", __func__, FindTask(NULL)));
    GLCALL(glVertex2f, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex2i (GLint x, GLint y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex2i", __func__, FindTask(NULL)));
    GLCALL(glVertex2i, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex2s (GLshort x, GLshort y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex2s", __func__, FindTask(NULL)));
    GLCALL(glVertex2s, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex3d (GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex3d", __func__, FindTask(NULL)));
    GLCALL(glVertex3d, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex3f", __func__, FindTask(NULL)));
    GLCALL(glVertex3f, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex3i (GLint x, GLint y, GLint z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex3i", __func__, FindTask(NULL)));
    GLCALL(glVertex3i, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex3s (GLshort x, GLshort y, GLshort z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex3s", __func__, FindTask(NULL)));
    GLCALL(glVertex3s, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex4d", __func__, FindTask(NULL)));
    GLCALL(glVertex4d, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex4f", __func__, FindTask(NULL)));
    GLCALL(glVertex4f, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex4i (GLint x, GLint y, GLint z, GLint w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex4i", __func__, FindTask(NULL)));
    GLCALL(glVertex4i, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex4s", __func__, FindTask(NULL)));
    GLCALL(glVertex4s, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex2dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex2dv", __func__, FindTask(NULL)));
    GLCALL(glVertex2dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex2fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex2fv", __func__, FindTask(NULL)));
    GLCALL(glVertex2fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex2iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex2iv", __func__, FindTask(NULL)));
    GLCALL(glVertex2iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex2sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex2sv", __func__, FindTask(NULL)));
    GLCALL(glVertex2sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex3dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex3dv", __func__, FindTask(NULL)));
    GLCALL(glVertex3dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex3fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex3fv", __func__, FindTask(NULL)));
    GLCALL(glVertex3fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex3iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex3iv", __func__, FindTask(NULL)));
    GLCALL(glVertex3iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex3sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex3sv", __func__, FindTask(NULL)));
    GLCALL(glVertex3sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex4dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex4dv", __func__, FindTask(NULL)));
    GLCALL(glVertex4dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex4fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex4fv", __func__, FindTask(NULL)));
    GLCALL(glVertex4fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex4iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex4iv", __func__, FindTask(NULL)));
    GLCALL(glVertex4iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertex4sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertex4sv", __func__, FindTask(NULL)));
    GLCALL(glVertex4sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3b", __func__, FindTask(NULL)));
    GLCALL(glNormal3b, nx, ny, nz);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3d", __func__, FindTask(NULL)));
    GLCALL(glNormal3d, nx, ny, nz);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3f", __func__, FindTask(NULL)));
    GLCALL(glNormal3f, nx, ny, nz);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3i (GLint nx, GLint ny, GLint nz)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3i", __func__, FindTask(NULL)));
    GLCALL(glNormal3i, nx, ny, nz);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3s (GLshort nx, GLshort ny, GLshort nz)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3s", __func__, FindTask(NULL)));
    GLCALL(glNormal3s, nx, ny, nz);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3bv (const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3bv", __func__, FindTask(NULL)));
    GLCALL(glNormal3bv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3dv", __func__, FindTask(NULL)));
    GLCALL(glNormal3dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3fv", __func__, FindTask(NULL)));
    GLCALL(glNormal3fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3iv", __func__, FindTask(NULL)));
    GLCALL(glNormal3iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormal3sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormal3sv", __func__, FindTask(NULL)));
    GLCALL(glNormal3sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexd (GLdouble c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexd", __func__, FindTask(NULL)));
    GLCALL(glIndexd, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexf (GLfloat c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexf", __func__, FindTask(NULL)));
    GLCALL(glIndexf, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexi (GLint c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexi", __func__, FindTask(NULL)));
    GLCALL(glIndexi, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexs (GLshort c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexs", __func__, FindTask(NULL)));
    GLCALL(glIndexs, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexub (GLubyte c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexub", __func__, FindTask(NULL)));
    GLCALL(glIndexub, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexdv (const GLdouble * c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexdv", __func__, FindTask(NULL)));
    GLCALL(glIndexdv, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexfv (const GLfloat * c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexfv", __func__, FindTask(NULL)));
    GLCALL(glIndexfv, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexiv (const GLint * c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexiv", __func__, FindTask(NULL)));
    GLCALL(glIndexiv, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexsv (const GLshort * c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexsv", __func__, FindTask(NULL)));
    GLCALL(glIndexsv, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexubv (const GLubyte * c)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexubv", __func__, FindTask(NULL)));
    GLCALL(glIndexubv, c);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3b (GLbyte red, GLbyte green, GLbyte blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3b", __func__, FindTask(NULL)));
    GLCALL(glColor3b, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3d (GLdouble red, GLdouble green, GLdouble blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3d", __func__, FindTask(NULL)));
    GLCALL(glColor3d, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3f", __func__, FindTask(NULL)));
    GLCALL(glColor3f, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3i (GLint red, GLint green, GLint blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3i", __func__, FindTask(NULL)));
    GLCALL(glColor3i, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3s (GLshort red, GLshort green, GLshort blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3s", __func__, FindTask(NULL)));
    GLCALL(glColor3s, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3ub (GLubyte red, GLubyte green, GLubyte blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3ub", __func__, FindTask(NULL)));
    GLCALL(glColor3ub, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3ui (GLuint red, GLuint green, GLuint blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3ui", __func__, FindTask(NULL)));
    GLCALL(glColor3ui, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3us (GLushort red, GLushort green, GLushort blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3us", __func__, FindTask(NULL)));
    GLCALL(glColor3us, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4b", __func__, FindTask(NULL)));
    GLCALL(glColor4b, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4d", __func__, FindTask(NULL)));
    GLCALL(glColor4d, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4f", __func__, FindTask(NULL)));
    GLCALL(glColor4f, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4i (GLint red, GLint green, GLint blue, GLint alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4i", __func__, FindTask(NULL)));
    GLCALL(glColor4i, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4s", __func__, FindTask(NULL)));
    GLCALL(glColor4s, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4ub", __func__, FindTask(NULL)));
    GLCALL(glColor4ub, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4ui", __func__, FindTask(NULL)));
    GLCALL(glColor4ui, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4us", __func__, FindTask(NULL)));
    GLCALL(glColor4us, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3bv (const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3bv", __func__, FindTask(NULL)));
    GLCALL(glColor3bv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3dv", __func__, FindTask(NULL)));
    GLCALL(glColor3dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3fv", __func__, FindTask(NULL)));
    GLCALL(glColor3fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3iv", __func__, FindTask(NULL)));
    GLCALL(glColor3iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3sv", __func__, FindTask(NULL)));
    GLCALL(glColor3sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3ubv (const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3ubv", __func__, FindTask(NULL)));
    GLCALL(glColor3ubv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3uiv (const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3uiv", __func__, FindTask(NULL)));
    GLCALL(glColor3uiv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor3usv (const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor3usv", __func__, FindTask(NULL)));
    GLCALL(glColor3usv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4bv (const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4bv", __func__, FindTask(NULL)));
    GLCALL(glColor4bv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4dv", __func__, FindTask(NULL)));
    GLCALL(glColor4dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4fv", __func__, FindTask(NULL)));
    GLCALL(glColor4fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4iv", __func__, FindTask(NULL)));
    GLCALL(glColor4iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4sv", __func__, FindTask(NULL)));
    GLCALL(glColor4sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4ubv (const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4ubv", __func__, FindTask(NULL)));
    GLCALL(glColor4ubv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4uiv (const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4uiv", __func__, FindTask(NULL)));
    GLCALL(glColor4uiv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColor4usv (const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColor4usv", __func__, FindTask(NULL)));
    GLCALL(glColor4usv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord1d (GLdouble s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord1d", __func__, FindTask(NULL)));
    GLCALL(glTexCoord1d, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord1f (GLfloat s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord1f", __func__, FindTask(NULL)));
    GLCALL(glTexCoord1f, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord1i (GLint s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord1i", __func__, FindTask(NULL)));
    GLCALL(glTexCoord1i, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord1s (GLshort s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord1s", __func__, FindTask(NULL)));
    GLCALL(glTexCoord1s, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord2d (GLdouble s, GLdouble t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord2d", __func__, FindTask(NULL)));
    GLCALL(glTexCoord2d, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord2f (GLfloat s, GLfloat t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord2f", __func__, FindTask(NULL)));
    GLCALL(glTexCoord2f, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord2i (GLint s, GLint t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord2i", __func__, FindTask(NULL)));
    GLCALL(glTexCoord2i, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord2s (GLshort s, GLshort t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord2s", __func__, FindTask(NULL)));
    GLCALL(glTexCoord2s, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord3d (GLdouble s, GLdouble t, GLdouble r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord3d", __func__, FindTask(NULL)));
    GLCALL(glTexCoord3d, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord3f (GLfloat s, GLfloat t, GLfloat r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord3f", __func__, FindTask(NULL)));
    GLCALL(glTexCoord3f, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord3i (GLint s, GLint t, GLint r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord3i", __func__, FindTask(NULL)));
    GLCALL(glTexCoord3i, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord3s (GLshort s, GLshort t, GLshort r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord3s", __func__, FindTask(NULL)));
    GLCALL(glTexCoord3s, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord4d", __func__, FindTask(NULL)));
    GLCALL(glTexCoord4d, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord4f", __func__, FindTask(NULL)));
    GLCALL(glTexCoord4f, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord4i (GLint s, GLint t, GLint r, GLint q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord4i", __func__, FindTask(NULL)));
    GLCALL(glTexCoord4i, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord4s", __func__, FindTask(NULL)));
    GLCALL(glTexCoord4s, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord1dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord1dv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord1dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord1fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord1fv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord1fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord1iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord1iv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord1iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord1sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord1sv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord1sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord2dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord2dv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord2dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord2fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord2fv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord2fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord2iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord2iv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord2iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord2sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord2sv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord2sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord3dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord3dv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord3dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord3fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord3fv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord3fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord3iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord3iv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord3iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord3sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord3sv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord3sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord4dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord4dv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord4dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord4fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord4fv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord4fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord4iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord4iv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord4iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoord4sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoord4sv", __func__, FindTask(NULL)));
    GLCALL(glTexCoord4sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos2d (GLdouble x, GLdouble y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos2d", __func__, FindTask(NULL)));
    GLCALL(glRasterPos2d, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos2f (GLfloat x, GLfloat y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos2f", __func__, FindTask(NULL)));
    GLCALL(glRasterPos2f, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos2i (GLint x, GLint y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos2i", __func__, FindTask(NULL)));
    GLCALL(glRasterPos2i, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos2s (GLshort x, GLshort y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos2s", __func__, FindTask(NULL)));
    GLCALL(glRasterPos2s, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos3d (GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos3d", __func__, FindTask(NULL)));
    GLCALL(glRasterPos3d, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos3f (GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos3f", __func__, FindTask(NULL)));
    GLCALL(glRasterPos3f, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos3i (GLint x, GLint y, GLint z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos3i", __func__, FindTask(NULL)));
    GLCALL(glRasterPos3i, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos3s (GLshort x, GLshort y, GLshort z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos3s", __func__, FindTask(NULL)));
    GLCALL(glRasterPos3s, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos4d", __func__, FindTask(NULL)));
    GLCALL(glRasterPos4d, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos4f", __func__, FindTask(NULL)));
    GLCALL(glRasterPos4f, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos4i (GLint x, GLint y, GLint z, GLint w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos4i", __func__, FindTask(NULL)));
    GLCALL(glRasterPos4i, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos4s", __func__, FindTask(NULL)));
    GLCALL(glRasterPos4s, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos2dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos2dv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos2dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos2fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos2fv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos2fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos2iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos2iv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos2iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos2sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos2sv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos2sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos3dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos3dv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos3dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos3fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos3fv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos3fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos3iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos3iv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos3iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos3sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos3sv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos3sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos4dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos4dv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos4dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos4fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos4fv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos4fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos4iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos4iv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos4iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRasterPos4sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRasterPos4sv", __func__, FindTask(NULL)));
    GLCALL(glRasterPos4sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRectd", __func__, FindTask(NULL)));
    GLCALL(glRectd, x1, y1, x2, y2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRectf", __func__, FindTask(NULL)));
    GLCALL(glRectf, x1, y1, x2, y2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRecti (GLint x1, GLint y1, GLint x2, GLint y2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRecti", __func__, FindTask(NULL)));
    GLCALL(glRecti, x1, y1, x2, y2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRects", __func__, FindTask(NULL)));
    GLCALL(glRects, x1, y1, x2, y2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRectdv (const GLdouble * v1, const GLdouble * v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRectdv", __func__, FindTask(NULL)));
    GLCALL(glRectdv, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRectfv (const GLfloat * v1, const GLfloat * v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRectfv", __func__, FindTask(NULL)));
    GLCALL(glRectfv, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRectiv (const GLint * v1, const GLint * v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRectiv", __func__, FindTask(NULL)));
    GLCALL(glRectiv, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRectsv (const GLshort * v1, const GLshort * v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRectsv", __func__, FindTask(NULL)));
    GLCALL(glRectsv, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid * ptr)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexPointer", __func__, FindTask(NULL)));
    GLCALL(glVertexPointer, size, type, stride, ptr);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormalPointer (GLenum type, GLsizei stride, const GLvoid * ptr)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormalPointer", __func__, FindTask(NULL)));
    GLCALL(glNormalPointer, type, stride, ptr);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid * ptr)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorPointer", __func__, FindTask(NULL)));
    GLCALL(glColorPointer, size, type, stride, ptr);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexPointer (GLenum type, GLsizei stride, const GLvoid * ptr)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexPointer", __func__, FindTask(NULL)));
    GLCALL(glIndexPointer, type, stride, ptr);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid * ptr)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoordPointer", __func__, FindTask(NULL)));
    GLCALL(glTexCoordPointer, size, type, stride, ptr);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEdgeFlagPointer (GLsizei stride, const GLvoid * ptr)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEdgeFlagPointer", __func__, FindTask(NULL)));
    GLCALL(glEdgeFlagPointer, stride, ptr);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetPointerv (GLenum pname, GLvoid * * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetPointerv", __func__, FindTask(NULL)));
    GLCALL(glGetPointerv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glArrayElement (GLint i)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glArrayElement", __func__, FindTask(NULL)));
    GLCALL(glArrayElement, i);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawArrays", __func__, FindTask(NULL)));
    GLCALL(glDrawArrays, mode, first, count);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid * indices)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawElements", __func__, FindTask(NULL)));
    GLCALL(glDrawElements, mode, count, type, indices);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glInterleavedArrays", __func__, FindTask(NULL)));
    GLCALL(glInterleavedArrays, format, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glShadeModel (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glShadeModel", __func__, FindTask(NULL)));
    GLCALL(glShadeModel, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLightf (GLenum light, GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLightf", __func__, FindTask(NULL)));
    GLCALL(glLightf, light, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLighti (GLenum light, GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLighti", __func__, FindTask(NULL)));
    GLCALL(glLighti, light, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLightfv (GLenum light, GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLightfv", __func__, FindTask(NULL)));
    GLCALL(glLightfv, light, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLightiv (GLenum light, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLightiv", __func__, FindTask(NULL)));
    GLCALL(glLightiv, light, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetLightfv (GLenum light, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetLightfv", __func__, FindTask(NULL)));
    GLCALL(glGetLightfv, light, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetLightiv (GLenum light, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetLightiv", __func__, FindTask(NULL)));
    GLCALL(glGetLightiv, light, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLightModelf (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLightModelf", __func__, FindTask(NULL)));
    GLCALL(glLightModelf, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLightModeli (GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLightModeli", __func__, FindTask(NULL)));
    GLCALL(glLightModeli, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLightModelfv (GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLightModelfv", __func__, FindTask(NULL)));
    GLCALL(glLightModelfv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLightModeliv (GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLightModeliv", __func__, FindTask(NULL)));
    GLCALL(glLightModeliv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMaterialf (GLenum face, GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMaterialf", __func__, FindTask(NULL)));
    GLCALL(glMaterialf, face, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMateriali (GLenum face, GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMateriali", __func__, FindTask(NULL)));
    GLCALL(glMateriali, face, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMaterialfv (GLenum face, GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMaterialfv", __func__, FindTask(NULL)));
    GLCALL(glMaterialfv, face, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMaterialiv (GLenum face, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMaterialiv", __func__, FindTask(NULL)));
    GLCALL(glMaterialiv, face, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMaterialfv (GLenum face, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMaterialfv", __func__, FindTask(NULL)));
    GLCALL(glGetMaterialfv, face, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMaterialiv (GLenum face, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMaterialiv", __func__, FindTask(NULL)));
    GLCALL(glGetMaterialiv, face, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorMaterial (GLenum face, GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorMaterial", __func__, FindTask(NULL)));
    GLCALL(glColorMaterial, face, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelZoom (GLfloat xfactor, GLfloat yfactor)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelZoom", __func__, FindTask(NULL)));
    GLCALL(glPixelZoom, xfactor, yfactor);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelStoref (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelStoref", __func__, FindTask(NULL)));
    GLCALL(glPixelStoref, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelStorei (GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelStorei", __func__, FindTask(NULL)));
    GLCALL(glPixelStorei, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelTransferf (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelTransferf", __func__, FindTask(NULL)));
    GLCALL(glPixelTransferf, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelTransferi (GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelTransferi", __func__, FindTask(NULL)));
    GLCALL(glPixelTransferi, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelMapfv", __func__, FindTask(NULL)));
    GLCALL(glPixelMapfv, map, mapsize, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelMapuiv", __func__, FindTask(NULL)));
    GLCALL(glPixelMapuiv, map, mapsize, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelMapusv (GLenum map, GLsizei mapsize, const GLushort * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelMapusv", __func__, FindTask(NULL)));
    GLCALL(glPixelMapusv, map, mapsize, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetPixelMapfv (GLenum map, GLfloat * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetPixelMapfv", __func__, FindTask(NULL)));
    GLCALL(glGetPixelMapfv, map, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetPixelMapuiv (GLenum map, GLuint * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetPixelMapuiv", __func__, FindTask(NULL)));
    GLCALL(glGetPixelMapuiv, map, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetPixelMapusv (GLenum map, GLushort * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetPixelMapusv", __func__, FindTask(NULL)));
    GLCALL(glGetPixelMapusv, map, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBitmap", __func__, FindTask(NULL)));
    GLCALL(glBitmap, width, height, xorig, yorig, xmove, ymove, bitmap);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glReadPixels", __func__, FindTask(NULL)));
    GLCALL(glReadPixels, x, y, width, height, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawPixels", __func__, FindTask(NULL)));
    GLCALL(glDrawPixels, width, height, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyPixels", __func__, FindTask(NULL)));
    GLCALL(glCopyPixels, x, y, width, height, type);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glStencilFunc", __func__, FindTask(NULL)));
    GLCALL(glStencilFunc, func, ref, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glStencilMask (GLuint mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glStencilMask", __func__, FindTask(NULL)));
    GLCALL(glStencilMask, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glStencilOp", __func__, FindTask(NULL)));
    GLCALL(glStencilOp, fail, zfail, zpass);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearStencil (GLint s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearStencil", __func__, FindTask(NULL)));
    GLCALL(glClearStencil, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexGend (GLenum coord, GLenum pname, GLdouble param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexGend", __func__, FindTask(NULL)));
    GLCALL(glTexGend, coord, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexGenf (GLenum coord, GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexGenf", __func__, FindTask(NULL)));
    GLCALL(glTexGenf, coord, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexGeni (GLenum coord, GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexGeni", __func__, FindTask(NULL)));
    GLCALL(glTexGeni, coord, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexGendv (GLenum coord, GLenum pname, const GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexGendv", __func__, FindTask(NULL)));
    GLCALL(glTexGendv, coord, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexGenfv (GLenum coord, GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexGenfv", __func__, FindTask(NULL)));
    GLCALL(glTexGenfv, coord, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexGeniv (GLenum coord, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexGeniv", __func__, FindTask(NULL)));
    GLCALL(glTexGeniv, coord, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexGendv (GLenum coord, GLenum pname, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexGendv", __func__, FindTask(NULL)));
    GLCALL(glGetTexGendv, coord, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexGenfv (GLenum coord, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexGenfv", __func__, FindTask(NULL)));
    GLCALL(glGetTexGenfv, coord, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexGeniv (GLenum coord, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexGeniv", __func__, FindTask(NULL)));
    GLCALL(glGetTexGeniv, coord, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexEnvf", __func__, FindTask(NULL)));
    GLCALL(glTexEnvf, target, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexEnvi (GLenum target, GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexEnvi", __func__, FindTask(NULL)));
    GLCALL(glTexEnvi, target, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexEnvfv (GLenum target, GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexEnvfv", __func__, FindTask(NULL)));
    GLCALL(glTexEnvfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexEnviv (GLenum target, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexEnviv", __func__, FindTask(NULL)));
    GLCALL(glTexEnviv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexEnvfv (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexEnvfv", __func__, FindTask(NULL)));
    GLCALL(glGetTexEnvfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexEnviv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexEnviv", __func__, FindTask(NULL)));
    GLCALL(glGetTexEnviv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexParameterf", __func__, FindTask(NULL)));
    GLCALL(glTexParameterf, target, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexParameteri (GLenum target, GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexParameteri", __func__, FindTask(NULL)));
    GLCALL(glTexParameteri, target, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexParameterfv (GLenum target, GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexParameterfv", __func__, FindTask(NULL)));
    GLCALL(glTexParameterfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexParameteriv (GLenum target, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexParameteriv", __func__, FindTask(NULL)));
    GLCALL(glTexParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexParameterfv (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexParameterfv", __func__, FindTask(NULL)));
    GLCALL(glGetTexParameterfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexParameteriv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetTexParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexLevelParameterfv", __func__, FindTask(NULL)));
    GLCALL(glGetTexLevelParameterfv, target, level, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexLevelParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetTexLevelParameteriv, target, level, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexImage1D (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexImage1D", __func__, FindTask(NULL)));
    GLCALL(glTexImage1D, target, level, internalFormat, width, border, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexImage2D (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexImage2D", __func__, FindTask(NULL)));
    GLCALL(glTexImage2D, target, level, internalFormat, width, height, border, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexImage", __func__, FindTask(NULL)));
    GLCALL(glGetTexImage, target, level, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenTextures (GLsizei n, GLuint * textures)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenTextures", __func__, FindTask(NULL)));
    GLCALL(glGenTextures, n, textures);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteTextures (GLsizei n, const GLuint * textures)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteTextures", __func__, FindTask(NULL)));
    GLCALL(glDeleteTextures, n, textures);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindTexture (GLenum target, GLuint texture)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindTexture", __func__, FindTask(NULL)));
    GLCALL(glBindTexture, target, texture);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPrioritizeTextures (GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPrioritizeTextures", __func__, FindTask(NULL)));
    GLCALL(glPrioritizeTextures, n, textures, priorities);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glAreTexturesResident (GLsizei n, const GLuint * textures, GLboolean * residences)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAreTexturesResident", __func__, FindTask(NULL)));
    _ret = GLCALL(glAreTexturesResident, n, textures, residences);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

GLboolean glIsTexture (GLuint texture)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsTexture", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsTexture, texture);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexSubImage1D", __func__, FindTask(NULL)));
    GLCALL(glTexSubImage1D, target, level, xoffset, width, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexSubImage2D", __func__, FindTask(NULL)));
    GLCALL(glTexSubImage2D, target, level, xoffset, yoffset, width, height, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexImage1D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexImage1D", __func__, FindTask(NULL)));
    GLCALL(glCopyTexImage1D, target, level, internalformat, x, y, width, border);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexImage2D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexImage2D", __func__, FindTask(NULL)));
    GLCALL(glCopyTexImage2D, target, level, internalformat, x, y, width, height, border);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexSubImage1D", __func__, FindTask(NULL)));
    GLCALL(glCopyTexSubImage1D, target, level, xoffset, x, y, width);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexSubImage2D", __func__, FindTask(NULL)));
    GLCALL(glCopyTexSubImage2D, target, level, xoffset, yoffset, x, y, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble * points)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMap1d", __func__, FindTask(NULL)));
    GLCALL(glMap1d, target, u1, u2, stride, order, points);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat * points)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMap1f", __func__, FindTask(NULL)));
    GLCALL(glMap1f, target, u1, u2, stride, order, points);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble * points)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMap2d", __func__, FindTask(NULL)));
    GLCALL(glMap2d, target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat * points)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMap2f", __func__, FindTask(NULL)));
    GLCALL(glMap2f, target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMapdv (GLenum target, GLenum query, GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMapdv", __func__, FindTask(NULL)));
    GLCALL(glGetMapdv, target, query, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMapfv (GLenum target, GLenum query, GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMapfv", __func__, FindTask(NULL)));
    GLCALL(glGetMapfv, target, query, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMapiv (GLenum target, GLenum query, GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMapiv", __func__, FindTask(NULL)));
    GLCALL(glGetMapiv, target, query, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalCoord1d (GLdouble u)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalCoord1d", __func__, FindTask(NULL)));
    GLCALL(glEvalCoord1d, u);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalCoord1f (GLfloat u)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalCoord1f", __func__, FindTask(NULL)));
    GLCALL(glEvalCoord1f, u);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalCoord1dv (const GLdouble * u)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalCoord1dv", __func__, FindTask(NULL)));
    GLCALL(glEvalCoord1dv, u);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalCoord1fv (const GLfloat * u)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalCoord1fv", __func__, FindTask(NULL)));
    GLCALL(glEvalCoord1fv, u);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalCoord2d (GLdouble u, GLdouble v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalCoord2d", __func__, FindTask(NULL)));
    GLCALL(glEvalCoord2d, u, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalCoord2f (GLfloat u, GLfloat v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalCoord2f", __func__, FindTask(NULL)));
    GLCALL(glEvalCoord2f, u, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalCoord2dv (const GLdouble * u)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalCoord2dv", __func__, FindTask(NULL)));
    GLCALL(glEvalCoord2dv, u);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalCoord2fv (const GLfloat * u)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalCoord2fv", __func__, FindTask(NULL)));
    GLCALL(glEvalCoord2fv, u);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMapGrid1d (GLint un, GLdouble u1, GLdouble u2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMapGrid1d", __func__, FindTask(NULL)));
    GLCALL(glMapGrid1d, un, u1, u2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMapGrid1f (GLint un, GLfloat u1, GLfloat u2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMapGrid1f", __func__, FindTask(NULL)));
    GLCALL(glMapGrid1f, un, u1, u2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMapGrid2d", __func__, FindTask(NULL)));
    GLCALL(glMapGrid2d, un, u1, u2, vn, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMapGrid2f", __func__, FindTask(NULL)));
    GLCALL(glMapGrid2f, un, u1, u2, vn, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalPoint1 (GLint i)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalPoint1", __func__, FindTask(NULL)));
    GLCALL(glEvalPoint1, i);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalPoint2 (GLint i, GLint j)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalPoint2", __func__, FindTask(NULL)));
    GLCALL(glEvalPoint2, i, j);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalMesh1 (GLenum mode, GLint i1, GLint i2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalMesh1", __func__, FindTask(NULL)));
    GLCALL(glEvalMesh1, mode, i1, i2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEvalMesh2", __func__, FindTask(NULL)));
    GLCALL(glEvalMesh2, mode, i1, i2, j1, j2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogf (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogf", __func__, FindTask(NULL)));
    GLCALL(glFogf, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogi (GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogi", __func__, FindTask(NULL)));
    GLCALL(glFogi, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogfv (GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogfv", __func__, FindTask(NULL)));
    GLCALL(glFogfv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogiv (GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogiv", __func__, FindTask(NULL)));
    GLCALL(glFogiv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFeedbackBuffer (GLsizei size, GLenum type, GLfloat * buffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFeedbackBuffer", __func__, FindTask(NULL)));
    GLCALL(glFeedbackBuffer, size, type, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPassThrough (GLfloat token)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPassThrough", __func__, FindTask(NULL)));
    GLCALL(glPassThrough, token);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSelectBuffer (GLsizei size, GLuint * buffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSelectBuffer", __func__, FindTask(NULL)));
    GLCALL(glSelectBuffer, size, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glInitNames ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glInitNames", __func__, FindTask(NULL)));
    GLCALL(glInitNames);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLoadName (GLuint name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLoadName", __func__, FindTask(NULL)));
    GLCALL(glLoadName, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPushName (GLuint name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPushName", __func__, FindTask(NULL)));
    GLCALL(glPushName, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPopName ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPopName", __func__, FindTask(NULL)));
    GLCALL(glPopName);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawRangeElements (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawRangeElements", __func__, FindTask(NULL)));
    GLCALL(glDrawRangeElements, mode, start, end, count, type, indices);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexImage3D (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexImage3D", __func__, FindTask(NULL)));
    GLCALL(glTexImage3D, target, level, internalFormat, width, height, depth, border, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexSubImage3D", __func__, FindTask(NULL)));
    GLCALL(glTexSubImage3D, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexSubImage3D", __func__, FindTask(NULL)));
    GLCALL(glCopyTexSubImage3D, target, level, xoffset, yoffset, zoffset, x, y, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorTable (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorTable", __func__, FindTask(NULL)));
    GLCALL(glColorTable, target, internalformat, width, format, type, table);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorSubTable (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorSubTable", __func__, FindTask(NULL)));
    GLCALL(glColorSubTable, target, start, count, format, type, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorTableParameteriv (GLenum target, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorTableParameteriv", __func__, FindTask(NULL)));
    GLCALL(glColorTableParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorTableParameterfv (GLenum target, GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorTableParameterfv", __func__, FindTask(NULL)));
    GLCALL(glColorTableParameterfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyColorSubTable (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyColorSubTable", __func__, FindTask(NULL)));
    GLCALL(glCopyColorSubTable, target, start, x, y, width);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyColorTable (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyColorTable", __func__, FindTask(NULL)));
    GLCALL(glCopyColorTable, target, internalformat, x, y, width);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetColorTable (GLenum target, GLenum format, GLenum type, GLvoid * table)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetColorTable", __func__, FindTask(NULL)));
    GLCALL(glGetColorTable, target, format, type, table);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetColorTableParameterfv (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetColorTableParameterfv", __func__, FindTask(NULL)));
    GLCALL(glGetColorTableParameterfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetColorTableParameteriv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetColorTableParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetColorTableParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendEquation (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendEquation", __func__, FindTask(NULL)));
    GLCALL(glBlendEquation, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendColor", __func__, FindTask(NULL)));
    GLCALL(glBlendColor, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glHistogram (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glHistogram", __func__, FindTask(NULL)));
    GLCALL(glHistogram, target, width, internalformat, sink);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glResetHistogram (GLenum target)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glResetHistogram", __func__, FindTask(NULL)));
    GLCALL(glResetHistogram, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetHistogram (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetHistogram", __func__, FindTask(NULL)));
    GLCALL(glGetHistogram, target, reset, format, type, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetHistogramParameterfv (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetHistogramParameterfv", __func__, FindTask(NULL)));
    GLCALL(glGetHistogramParameterfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetHistogramParameteriv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetHistogramParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetHistogramParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMinmax (GLenum target, GLenum internalformat, GLboolean sink)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMinmax", __func__, FindTask(NULL)));
    GLCALL(glMinmax, target, internalformat, sink);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glResetMinmax (GLenum target)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glResetMinmax", __func__, FindTask(NULL)));
    GLCALL(glResetMinmax, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMinmax (GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMinmax", __func__, FindTask(NULL)));
    GLCALL(glGetMinmax, target, reset, format, types, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMinmaxParameterfv (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMinmaxParameterfv", __func__, FindTask(NULL)));
    GLCALL(glGetMinmaxParameterfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMinmaxParameteriv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMinmaxParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetMinmaxParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionFilter1D (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionFilter1D", __func__, FindTask(NULL)));
    GLCALL(glConvolutionFilter1D, target, internalformat, width, format, type, image);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionFilter2D (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionFilter2D", __func__, FindTask(NULL)));
    GLCALL(glConvolutionFilter2D, target, internalformat, width, height, format, type, image);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionParameterf (GLenum target, GLenum pname, GLfloat params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionParameterf", __func__, FindTask(NULL)));
    GLCALL(glConvolutionParameterf, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionParameterfv (GLenum target, GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionParameterfv", __func__, FindTask(NULL)));
    GLCALL(glConvolutionParameterfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionParameteri (GLenum target, GLenum pname, GLint params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionParameteri", __func__, FindTask(NULL)));
    GLCALL(glConvolutionParameteri, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionParameteriv (GLenum target, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionParameteriv", __func__, FindTask(NULL)));
    GLCALL(glConvolutionParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyConvolutionFilter1D (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyConvolutionFilter1D", __func__, FindTask(NULL)));
    GLCALL(glCopyConvolutionFilter1D, target, internalformat, x, y, width);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyConvolutionFilter2D (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyConvolutionFilter2D", __func__, FindTask(NULL)));
    GLCALL(glCopyConvolutionFilter2D, target, internalformat, x, y, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetConvolutionFilter (GLenum target, GLenum format, GLenum type, GLvoid * image)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetConvolutionFilter", __func__, FindTask(NULL)));
    GLCALL(glGetConvolutionFilter, target, format, type, image);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetConvolutionParameterfv (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetConvolutionParameterfv", __func__, FindTask(NULL)));
    GLCALL(glGetConvolutionParameterfv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetConvolutionParameteriv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetConvolutionParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetConvolutionParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSeparableFilter2D (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSeparableFilter2D", __func__, FindTask(NULL)));
    GLCALL(glSeparableFilter2D, target, internalformat, width, height, format, type, row, column);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetSeparableFilter (GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetSeparableFilter", __func__, FindTask(NULL)));
    GLCALL(glGetSeparableFilter, target, format, type, row, column, span);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glActiveTexture (GLenum texture)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glActiveTexture", __func__, FindTask(NULL)));
    GLCALL(glActiveTexture, texture);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClientActiveTexture (GLenum texture)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClientActiveTexture", __func__, FindTask(NULL)));
    GLCALL(glClientActiveTexture, texture);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexImage1D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexImage1D", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexImage1D, target, level, internalformat, width, border, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexImage2D", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexImage2D, target, level, internalformat, width, height, border, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexImage3D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexImage3D", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexImage3D, target, level, internalformat, width, height, depth, border, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexSubImage1D", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexSubImage1D, target, level, xoffset, width, format, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexSubImage2D", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexSubImage2D, target, level, xoffset, yoffset, width, height, format, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexSubImage3D", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexSubImage3D, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetCompressedTexImage (GLenum target, GLint lod, GLvoid * img)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetCompressedTexImage", __func__, FindTask(NULL)));
    GLCALL(glGetCompressedTexImage, target, lod, img);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1d (GLenum target, GLdouble s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1d", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1d, target, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1dv (GLenum target, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1dv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1dv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1f (GLenum target, GLfloat s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1f", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1f, target, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1fv (GLenum target, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1fv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1fv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1i (GLenum target, GLint s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1i", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1i, target, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1iv (GLenum target, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1iv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1iv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1s (GLenum target, GLshort s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1s", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1s, target, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1sv (GLenum target, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1sv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1sv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2d (GLenum target, GLdouble s, GLdouble t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2d", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2d, target, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2dv (GLenum target, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2dv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2dv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2f (GLenum target, GLfloat s, GLfloat t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2f", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2f, target, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2fv (GLenum target, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2fv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2fv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2i (GLenum target, GLint s, GLint t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2i", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2i, target, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2iv (GLenum target, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2iv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2iv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2s (GLenum target, GLshort s, GLshort t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2s", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2s, target, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2sv (GLenum target, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2sv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2sv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3d (GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3d", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3d, target, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3dv (GLenum target, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3dv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3dv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3f (GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3f", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3f, target, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3fv (GLenum target, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3fv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3fv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3i (GLenum target, GLint s, GLint t, GLint r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3i", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3i, target, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3iv (GLenum target, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3iv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3iv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3s (GLenum target, GLshort s, GLshort t, GLshort r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3s", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3s, target, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3sv (GLenum target, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3sv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3sv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4d (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4d", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4d, target, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4dv (GLenum target, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4dv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4dv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4f (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4f", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4f, target, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4fv (GLenum target, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4fv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4fv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4i (GLenum target, GLint s, GLint t, GLint r, GLint q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4i", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4i, target, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4iv (GLenum target, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4iv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4iv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4s (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4s", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4s, target, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4sv (GLenum target, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4sv", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4sv, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLoadTransposeMatrixd (const GLdouble * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLoadTransposeMatrixd", __func__, FindTask(NULL)));
    GLCALL(glLoadTransposeMatrixd, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLoadTransposeMatrixf (const GLfloat * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLoadTransposeMatrixf", __func__, FindTask(NULL)));
    GLCALL(glLoadTransposeMatrixf, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultTransposeMatrixd (const GLdouble * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultTransposeMatrixd", __func__, FindTask(NULL)));
    GLCALL(glMultTransposeMatrixd, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultTransposeMatrixf (const GLfloat * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultTransposeMatrixf", __func__, FindTask(NULL)));
    GLCALL(glMultTransposeMatrixf, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSampleCoverage (GLclampf value, GLboolean invert)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSampleCoverage", __func__, FindTask(NULL)));
    GLCALL(glSampleCoverage, value, invert);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glActiveTextureARB (GLenum texture)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glActiveTextureARB", __func__, FindTask(NULL)));
    GLCALL(glActiveTextureARB, texture);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClientActiveTextureARB (GLenum texture)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClientActiveTextureARB", __func__, FindTask(NULL)));
    GLCALL(glClientActiveTextureARB, texture);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1dARB (GLenum target, GLdouble s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1dARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1dARB, target, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1dvARB (GLenum target, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1dvARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1dvARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1fARB (GLenum target, GLfloat s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1fARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1fARB, target, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1fvARB (GLenum target, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1fvARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1fvARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1iARB (GLenum target, GLint s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1iARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1iARB, target, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1ivARB (GLenum target, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1ivARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1ivARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1sARB (GLenum target, GLshort s)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1sARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1sARB, target, s);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord1svARB (GLenum target, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord1svARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord1svARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2dARB (GLenum target, GLdouble s, GLdouble t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2dARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2dARB, target, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2dvARB (GLenum target, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2dvARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2dvARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2fARB (GLenum target, GLfloat s, GLfloat t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2fARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2fARB, target, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2fvARB (GLenum target, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2fvARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2fvARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2iARB (GLenum target, GLint s, GLint t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2iARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2iARB, target, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2ivARB (GLenum target, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2ivARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2ivARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2sARB (GLenum target, GLshort s, GLshort t)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2sARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2sARB, target, s, t);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord2svARB (GLenum target, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord2svARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord2svARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3dARB (GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3dARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3dARB, target, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3dvARB (GLenum target, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3dvARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3dvARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3fARB (GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3fARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3fARB, target, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3fvARB (GLenum target, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3fvARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3fvARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3iARB (GLenum target, GLint s, GLint t, GLint r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3iARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3iARB, target, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3ivARB (GLenum target, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3ivARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3ivARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3sARB (GLenum target, GLshort s, GLshort t, GLshort r)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3sARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3sARB, target, s, t, r);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord3svARB (GLenum target, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord3svARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord3svARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4dARB (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4dARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4dARB, target, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4dvARB (GLenum target, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4dvARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4dvARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4fARB (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4fARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4fARB, target, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4fvARB (GLenum target, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4fvARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4fvARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4iARB (GLenum target, GLint s, GLint t, GLint r, GLint q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4iARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4iARB, target, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4ivARB (GLenum target, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4ivARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4ivARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4sARB (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4sARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4sARB, target, s, t, r, q);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiTexCoord4svARB (GLenum target, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiTexCoord4svARB", __func__, FindTask(NULL)));
    GLCALL(glMultiTexCoord4svARB, target, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendFuncSeparate", __func__, FindTask(NULL)));
    GLCALL(glBlendFuncSeparate, sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoordf (GLfloat coord)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoordf", __func__, FindTask(NULL)));
    GLCALL(glFogCoordf, coord);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoordfv (const GLfloat * coord)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoordfv", __func__, FindTask(NULL)));
    GLCALL(glFogCoordfv, coord);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoordd (GLdouble coord)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoordd", __func__, FindTask(NULL)));
    GLCALL(glFogCoordd, coord);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoorddv (const GLdouble * coord)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoorddv", __func__, FindTask(NULL)));
    GLCALL(glFogCoorddv, coord);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoordPointer (GLenum type, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoordPointer", __func__, FindTask(NULL)));
    GLCALL(glFogCoordPointer, type, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiDrawArrays (GLenum mode, const GLint * first, const GLsizei * count, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiDrawArrays", __func__, FindTask(NULL)));
    GLCALL(glMultiDrawArrays, mode, first, count, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiDrawElements (GLenum mode, const GLsizei * count, GLenum type, const GLvoid *  * indices, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiDrawElements", __func__, FindTask(NULL)));
    GLCALL(glMultiDrawElements, mode, count, type, indices, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameterf (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameterf", __func__, FindTask(NULL)));
    GLCALL(glPointParameterf, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameterfv (GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameterfv", __func__, FindTask(NULL)));
    GLCALL(glPointParameterfv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameteri (GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameteri", __func__, FindTask(NULL)));
    GLCALL(glPointParameteri, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameteriv (GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameteriv", __func__, FindTask(NULL)));
    GLCALL(glPointParameteriv, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3b (GLbyte red, GLbyte green, GLbyte blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3b", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3b, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3bv (const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3bv", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3bv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3d (GLdouble red, GLdouble green, GLdouble blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3d", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3d, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3dv", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3f", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3f, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3fv", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3i (GLint red, GLint green, GLint blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3i", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3i, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3iv", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3s (GLshort red, GLshort green, GLshort blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3s", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3s, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3sv", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3ub (GLubyte red, GLubyte green, GLubyte blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3ub", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3ub, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3ubv (const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3ubv", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3ubv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3ui (GLuint red, GLuint green, GLuint blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3ui", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3ui, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3uiv (const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3uiv", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3uiv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3us (GLushort red, GLushort green, GLushort blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3us", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3us, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3usv (const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3usv", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3usv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColorPointer", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColorPointer, size, type, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2d (GLdouble x, GLdouble y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2d", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2d, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2dv", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2f (GLfloat x, GLfloat y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2f", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2f, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2fv", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2i (GLint x, GLint y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2i", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2i, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2iv", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2s (GLshort x, GLshort y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2s", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2s, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2sv", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3d (GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3d", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3d, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3dv (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3dv", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3dv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3f (GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3f", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3f, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3fv (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3fv", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3fv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3i (GLint x, GLint y, GLint z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3i", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3i, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3iv (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3iv", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3iv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3s (GLshort x, GLshort y, GLshort z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3s", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3s, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3sv (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3sv", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3sv, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenQueries (GLsizei n, GLuint * ids)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenQueries", __func__, FindTask(NULL)));
    GLCALL(glGenQueries, n, ids);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteQueries (GLsizei n, const GLuint * ids)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteQueries", __func__, FindTask(NULL)));
    GLCALL(glDeleteQueries, n, ids);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsQuery (GLuint id)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsQuery", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsQuery, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBeginQuery (GLenum target, GLuint id)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBeginQuery", __func__, FindTask(NULL)));
    GLCALL(glBeginQuery, target, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEndQuery (GLenum target)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEndQuery", __func__, FindTask(NULL)));
    GLCALL(glEndQuery, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetQueryiv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetQueryiv", __func__, FindTask(NULL)));
    GLCALL(glGetQueryiv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetQueryObjectiv (GLuint id, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetQueryObjectiv", __func__, FindTask(NULL)));
    GLCALL(glGetQueryObjectiv, id, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetQueryObjectuiv (GLuint id, GLenum pname, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetQueryObjectuiv", __func__, FindTask(NULL)));
    GLCALL(glGetQueryObjectuiv, id, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindBuffer (GLenum target, GLuint buffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindBuffer", __func__, FindTask(NULL)));
    GLCALL(glBindBuffer, target, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteBuffers (GLsizei n, const GLuint * buffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteBuffers", __func__, FindTask(NULL)));
    GLCALL(glDeleteBuffers, n, buffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenBuffers (GLsizei n, GLuint * buffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenBuffers", __func__, FindTask(NULL)));
    GLCALL(glGenBuffers, n, buffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsBuffer (GLuint buffer)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsBuffer", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsBuffer, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBufferData (GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBufferData", __func__, FindTask(NULL)));
    GLCALL(glBufferData, target, size, data, usage);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBufferSubData", __func__, FindTask(NULL)));
    GLCALL(glBufferSubData, target, offset, size, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBufferSubData", __func__, FindTask(NULL)));
    GLCALL(glGetBufferSubData, target, offset, size, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLvoid* glMapBuffer (GLenum target, GLenum access)
{
    GLvoid* _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMapBuffer", __func__, FindTask(NULL)));
    _ret = GLCALL(glMapBuffer, target, access);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

GLboolean glUnmapBuffer (GLenum target)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUnmapBuffer", __func__, FindTask(NULL)));
    _ret = GLCALL(glUnmapBuffer, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glGetBufferParameteriv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBufferParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetBufferParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetBufferPointerv (GLenum target, GLenum pname, GLvoid *  * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBufferPointerv", __func__, FindTask(NULL)));
    GLCALL(glGetBufferPointerv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendEquationSeparate", __func__, FindTask(NULL)));
    GLCALL(glBlendEquationSeparate, modeRGB, modeAlpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawBuffers (GLsizei n, const GLenum * bufs)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawBuffers", __func__, FindTask(NULL)));
    GLCALL(glDrawBuffers, n, bufs);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glStencilOpSeparate (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glStencilOpSeparate", __func__, FindTask(NULL)));
    GLCALL(glStencilOpSeparate, face, sfail, dpfail, dppass);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glStencilFuncSeparate", __func__, FindTask(NULL)));
    GLCALL(glStencilFuncSeparate, face, func, ref, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glStencilMaskSeparate (GLenum face, GLuint mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glStencilMaskSeparate", __func__, FindTask(NULL)));
    GLCALL(glStencilMaskSeparate, face, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glAttachShader (GLuint program, GLuint shader)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAttachShader", __func__, FindTask(NULL)));
    GLCALL(glAttachShader, program, shader);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindAttribLocation (GLuint program, GLuint index, const GLchar * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindAttribLocation", __func__, FindTask(NULL)));
    GLCALL(glBindAttribLocation, program, index, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompileShader (GLuint shader)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompileShader", __func__, FindTask(NULL)));
    GLCALL(glCompileShader, shader);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLuint glCreateProgram ()
{
    GLuint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCreateProgram", __func__, FindTask(NULL)));
    _ret = GLCALL(glCreateProgram);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

GLuint glCreateShader (GLenum type)
{
    GLuint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCreateShader", __func__, FindTask(NULL)));
    _ret = GLCALL(glCreateShader, type);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glDeleteProgram (GLuint program)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteProgram", __func__, FindTask(NULL)));
    GLCALL(glDeleteProgram, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteShader (GLuint shader)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteShader", __func__, FindTask(NULL)));
    GLCALL(glDeleteShader, shader);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDetachShader (GLuint program, GLuint shader)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDetachShader", __func__, FindTask(NULL)));
    GLCALL(glDetachShader, program, shader);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDisableVertexAttribArray (GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDisableVertexAttribArray", __func__, FindTask(NULL)));
    GLCALL(glDisableVertexAttribArray, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEnableVertexAttribArray (GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEnableVertexAttribArray", __func__, FindTask(NULL)));
    GLCALL(glEnableVertexAttribArray, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetActiveAttrib (GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetActiveAttrib", __func__, FindTask(NULL)));
    GLCALL(glGetActiveAttrib, program, index, bufSize, length, size, type, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetActiveUniform", __func__, FindTask(NULL)));
    GLCALL(glGetActiveUniform, program, index, bufSize, length, size, type, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetAttachedShaders (GLuint program, GLsizei maxCount, GLsizei * count, GLuint * obj)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetAttachedShaders", __func__, FindTask(NULL)));
    GLCALL(glGetAttachedShaders, program, maxCount, count, obj);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLint glGetAttribLocation (GLuint program, const GLchar * name)
{
    GLint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetAttribLocation", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetAttribLocation, program, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glGetProgramiv (GLuint program, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramiv", __func__, FindTask(NULL)));
    GLCALL(glGetProgramiv, program, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramInfoLog", __func__, FindTask(NULL)));
    GLCALL(glGetProgramInfoLog, program, bufSize, length, infoLog);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetShaderiv (GLuint shader, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetShaderiv", __func__, FindTask(NULL)));
    GLCALL(glGetShaderiv, shader, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetShaderInfoLog", __func__, FindTask(NULL)));
    GLCALL(glGetShaderInfoLog, shader, bufSize, length, infoLog);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetShaderSource", __func__, FindTask(NULL)));
    GLCALL(glGetShaderSource, shader, bufSize, length, source);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLint glGetUniformLocation (GLuint program, const GLchar * name)
{
    GLint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetUniformLocation", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetUniformLocation, program, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glGetUniformfv (GLuint program, GLint location, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetUniformfv", __func__, FindTask(NULL)));
    GLCALL(glGetUniformfv, program, location, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetUniformiv (GLuint program, GLint location, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetUniformiv", __func__, FindTask(NULL)));
    GLCALL(glGetUniformiv, program, location, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribdv (GLuint index, GLenum pname, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribdv", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribdv, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribfv", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribfv, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribiv (GLuint index, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribiv", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribiv, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribPointerv (GLuint index, GLenum pname, GLvoid *  * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribPointerv", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribPointerv, index, pname, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsProgram (GLuint program)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsProgram", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsProgram, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

GLboolean glIsShader (GLuint shader)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsShader", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsShader, shader);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glLinkProgram (GLuint program)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLinkProgram", __func__, FindTask(NULL)));
    GLCALL(glLinkProgram, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glShaderSource (GLuint shader, GLsizei count, const GLchar *  * string, const GLint * length)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glShaderSource", __func__, FindTask(NULL)));
    GLCALL(glShaderSource, shader, count, string, length);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUseProgram (GLuint program)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUseProgram", __func__, FindTask(NULL)));
    GLCALL(glUseProgram, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1f (GLint location, GLfloat v0)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1f", __func__, FindTask(NULL)));
    GLCALL(glUniform1f, location, v0);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2f (GLint location, GLfloat v0, GLfloat v1)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2f", __func__, FindTask(NULL)));
    GLCALL(glUniform2f, location, v0, v1);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3f", __func__, FindTask(NULL)));
    GLCALL(glUniform3f, location, v0, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4f", __func__, FindTask(NULL)));
    GLCALL(glUniform4f, location, v0, v1, v2, v3);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1i (GLint location, GLint v0)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1i", __func__, FindTask(NULL)));
    GLCALL(glUniform1i, location, v0);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2i (GLint location, GLint v0, GLint v1)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2i", __func__, FindTask(NULL)));
    GLCALL(glUniform2i, location, v0, v1);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3i (GLint location, GLint v0, GLint v1, GLint v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3i", __func__, FindTask(NULL)));
    GLCALL(glUniform3i, location, v0, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4i (GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4i", __func__, FindTask(NULL)));
    GLCALL(glUniform4i, location, v0, v1, v2, v3);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1fv (GLint location, GLsizei count, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1fv", __func__, FindTask(NULL)));
    GLCALL(glUniform1fv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2fv (GLint location, GLsizei count, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2fv", __func__, FindTask(NULL)));
    GLCALL(glUniform2fv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3fv (GLint location, GLsizei count, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3fv", __func__, FindTask(NULL)));
    GLCALL(glUniform3fv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4fv (GLint location, GLsizei count, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4fv", __func__, FindTask(NULL)));
    GLCALL(glUniform4fv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1iv (GLint location, GLsizei count, const GLint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1iv", __func__, FindTask(NULL)));
    GLCALL(glUniform1iv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2iv (GLint location, GLsizei count, const GLint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2iv", __func__, FindTask(NULL)));
    GLCALL(glUniform2iv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3iv (GLint location, GLsizei count, const GLint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3iv", __func__, FindTask(NULL)));
    GLCALL(glUniform3iv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4iv (GLint location, GLsizei count, const GLint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4iv", __func__, FindTask(NULL)));
    GLCALL(glUniform4iv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix2fv", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix2fv, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix3fv", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix3fv, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix4fv", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix4fv, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glValidateProgram (GLuint program)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glValidateProgram", __func__, FindTask(NULL)));
    GLCALL(glValidateProgram, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1d (GLuint index, GLdouble x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1d", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1d, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1dv (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1dv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1dv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1f (GLuint index, GLfloat x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1f", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1f, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1fv (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1fv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1fv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1s (GLuint index, GLshort x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1s", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1s, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1sv (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1sv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1sv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2d (GLuint index, GLdouble x, GLdouble y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2d", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2d, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2dv (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2dv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2dv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2f (GLuint index, GLfloat x, GLfloat y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2f", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2f, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2fv (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2fv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2fv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2s (GLuint index, GLshort x, GLshort y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2s", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2s, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2sv (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2sv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2sv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3d (GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3d", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3d, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3dv (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3dv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3dv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3f (GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3f", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3f, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3fv (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3fv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3fv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3s (GLuint index, GLshort x, GLshort y, GLshort z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3s", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3s, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3sv (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3sv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3sv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4Nbv (GLuint index, const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4Nbv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4Nbv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4Niv (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4Niv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4Niv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4Nsv (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4Nsv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4Nsv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4Nub (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4Nub", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4Nub, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4Nubv (GLuint index, const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4Nubv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4Nubv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4Nuiv (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4Nuiv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4Nuiv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4Nusv (GLuint index, const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4Nusv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4Nusv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4bv (GLuint index, const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4bv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4bv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4d (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4d", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4d, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4dv (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4dv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4dv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4f (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4f", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4f, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4fv (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4fv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4fv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4iv (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4iv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4iv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4s (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4s", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4s, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4sv (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4sv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4sv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4ubv (GLuint index, const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4ubv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4ubv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4uiv (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4uiv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4uiv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4usv (GLuint index, const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4usv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4usv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribPointer", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribPointer, index, size, type, normalized, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix2x3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix2x3fv", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix2x3fv, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix3x2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix3x2fv", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix3x2fv, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix2x4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix2x4fv", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix2x4fv, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix4x2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix4x2fv", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix4x2fv, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix3x4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix3x4fv", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix3x4fv, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix4x3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix4x3fv", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix4x3fv, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLoadTransposeMatrixfARB (const GLfloat * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLoadTransposeMatrixfARB", __func__, FindTask(NULL)));
    GLCALL(glLoadTransposeMatrixfARB, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLoadTransposeMatrixdARB (const GLdouble * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLoadTransposeMatrixdARB", __func__, FindTask(NULL)));
    GLCALL(glLoadTransposeMatrixdARB, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultTransposeMatrixfARB (const GLfloat * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultTransposeMatrixfARB", __func__, FindTask(NULL)));
    GLCALL(glMultTransposeMatrixfARB, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultTransposeMatrixdARB (const GLdouble * m)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultTransposeMatrixdARB", __func__, FindTask(NULL)));
    GLCALL(glMultTransposeMatrixdARB, m);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSampleCoverageARB (GLclampf value, GLboolean invert)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSampleCoverageARB", __func__, FindTask(NULL)));
    GLCALL(glSampleCoverageARB, value, invert);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexImage3DARB (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexImage3DARB", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexImage3DARB, target, level, internalformat, width, height, depth, border, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexImage2DARB (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexImage2DARB", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexImage2DARB, target, level, internalformat, width, height, border, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexImage1DARB (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexImage1DARB", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexImage1DARB, target, level, internalformat, width, border, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexSubImage3DARB (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexSubImage3DARB", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexSubImage3DARB, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexSubImage2DARB (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexSubImage2DARB", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexSubImage2DARB, target, level, xoffset, yoffset, width, height, format, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompressedTexSubImage1DARB (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompressedTexSubImage1DARB", __func__, FindTask(NULL)));
    GLCALL(glCompressedTexSubImage1DARB, target, level, xoffset, width, format, imageSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetCompressedTexImageARB (GLenum target, GLint level, GLvoid * img)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetCompressedTexImageARB", __func__, FindTask(NULL)));
    GLCALL(glGetCompressedTexImageARB, target, level, img);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameterfARB (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameterfARB", __func__, FindTask(NULL)));
    GLCALL(glPointParameterfARB, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameterfvARB (GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameterfvARB", __func__, FindTask(NULL)));
    GLCALL(glPointParameterfvARB, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2dARB (GLdouble x, GLdouble y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2dARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2dARB, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2dvARB (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2dvARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2dvARB, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2fARB (GLfloat x, GLfloat y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2fARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2fARB, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2fvARB (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2fvARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2fvARB, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2iARB (GLint x, GLint y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2iARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2iARB, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2ivARB (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2ivARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2ivARB, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2sARB (GLshort x, GLshort y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2sARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2sARB, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2svARB (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2svARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2svARB, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3dARB (GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3dARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3dARB, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3dvARB (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3dvARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3dvARB, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3fARB (GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3fARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3fARB, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3fvARB (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3fvARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3fvARB, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3iARB (GLint x, GLint y, GLint z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3iARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3iARB, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3ivARB (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3ivARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3ivARB, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3sARB (GLshort x, GLshort y, GLshort z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3sARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3sARB, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3svARB (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3svARB", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3svARB, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1dARB (GLuint index, GLdouble x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1dARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1dARB, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1dvARB (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1dvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1dvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1fARB (GLuint index, GLfloat x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1fARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1fARB, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1fvARB (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1fvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1fvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1sARB (GLuint index, GLshort x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1sARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1sARB, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1svARB (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1svARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1svARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2dARB (GLuint index, GLdouble x, GLdouble y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2dARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2dARB, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2dvARB (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2dvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2dvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2fARB (GLuint index, GLfloat x, GLfloat y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2fARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2fARB, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2fvARB (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2fvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2fvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2sARB (GLuint index, GLshort x, GLshort y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2sARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2sARB, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2svARB (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2svARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2svARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3dARB (GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3dARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3dARB, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3dvARB (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3dvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3dvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3fARB (GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3fARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3fARB, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3fvARB (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3fvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3fvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3sARB (GLuint index, GLshort x, GLshort y, GLshort z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3sARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3sARB, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3svARB (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3svARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3svARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4NbvARB (GLuint index, const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4NbvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4NbvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4NivARB (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4NivARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4NivARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4NsvARB (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4NsvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4NsvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4NubARB (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4NubARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4NubARB, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4NubvARB (GLuint index, const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4NubvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4NubvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4NuivARB (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4NuivARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4NuivARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4NusvARB (GLuint index, const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4NusvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4NusvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4bvARB (GLuint index, const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4bvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4bvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4dARB (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4dARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4dARB, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4dvARB (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4dvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4dvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4fARB (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4fARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4fARB, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4fvARB (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4fvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4fvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4ivARB (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4ivARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4ivARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4sARB (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4sARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4sARB, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4svARB (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4svARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4svARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4ubvARB (GLuint index, const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4ubvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4ubvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4uivARB (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4uivARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4uivARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4usvARB (GLuint index, const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4usvARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4usvARB, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribPointerARB (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribPointerARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribPointerARB, index, size, type, normalized, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEnableVertexAttribArrayARB (GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEnableVertexAttribArrayARB", __func__, FindTask(NULL)));
    GLCALL(glEnableVertexAttribArrayARB, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDisableVertexAttribArrayARB (GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDisableVertexAttribArrayARB", __func__, FindTask(NULL)));
    GLCALL(glDisableVertexAttribArrayARB, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramStringARB (GLenum target, GLenum format, GLsizei len, const GLvoid * string)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramStringARB", __func__, FindTask(NULL)));
    GLCALL(glProgramStringARB, target, format, len, string);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindProgramARB (GLenum target, GLuint program)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindProgramARB", __func__, FindTask(NULL)));
    GLCALL(glBindProgramARB, target, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteProgramsARB (GLsizei n, const GLuint * programs)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteProgramsARB", __func__, FindTask(NULL)));
    GLCALL(glDeleteProgramsARB, n, programs);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenProgramsARB (GLsizei n, GLuint * programs)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenProgramsARB", __func__, FindTask(NULL)));
    GLCALL(glGenProgramsARB, n, programs);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramEnvParameter4dARB (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramEnvParameter4dARB", __func__, FindTask(NULL)));
    GLCALL(glProgramEnvParameter4dARB, target, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramEnvParameter4dvARB (GLenum target, GLuint index, const GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramEnvParameter4dvARB", __func__, FindTask(NULL)));
    GLCALL(glProgramEnvParameter4dvARB, target, index, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramEnvParameter4fARB (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramEnvParameter4fARB", __func__, FindTask(NULL)));
    GLCALL(glProgramEnvParameter4fARB, target, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramEnvParameter4fvARB (GLenum target, GLuint index, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramEnvParameter4fvARB", __func__, FindTask(NULL)));
    GLCALL(glProgramEnvParameter4fvARB, target, index, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramLocalParameter4dARB (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramLocalParameter4dARB", __func__, FindTask(NULL)));
    GLCALL(glProgramLocalParameter4dARB, target, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramLocalParameter4dvARB (GLenum target, GLuint index, const GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramLocalParameter4dvARB", __func__, FindTask(NULL)));
    GLCALL(glProgramLocalParameter4dvARB, target, index, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramLocalParameter4fARB (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramLocalParameter4fARB", __func__, FindTask(NULL)));
    GLCALL(glProgramLocalParameter4fARB, target, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramLocalParameter4fvARB (GLenum target, GLuint index, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramLocalParameter4fvARB", __func__, FindTask(NULL)));
    GLCALL(glProgramLocalParameter4fvARB, target, index, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramEnvParameterdvARB (GLenum target, GLuint index, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramEnvParameterdvARB", __func__, FindTask(NULL)));
    GLCALL(glGetProgramEnvParameterdvARB, target, index, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramEnvParameterfvARB (GLenum target, GLuint index, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramEnvParameterfvARB", __func__, FindTask(NULL)));
    GLCALL(glGetProgramEnvParameterfvARB, target, index, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramLocalParameterdvARB (GLenum target, GLuint index, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramLocalParameterdvARB", __func__, FindTask(NULL)));
    GLCALL(glGetProgramLocalParameterdvARB, target, index, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramLocalParameterfvARB (GLenum target, GLuint index, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramLocalParameterfvARB", __func__, FindTask(NULL)));
    GLCALL(glGetProgramLocalParameterfvARB, target, index, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramivARB (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramivARB", __func__, FindTask(NULL)));
    GLCALL(glGetProgramivARB, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramStringARB (GLenum target, GLenum pname, GLvoid * string)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramStringARB", __func__, FindTask(NULL)));
    GLCALL(glGetProgramStringARB, target, pname, string);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribdvARB (GLuint index, GLenum pname, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribdvARB", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribdvARB, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribfvARB (GLuint index, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribfvARB", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribfvARB, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribivARB (GLuint index, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribivARB", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribivARB, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribPointervARB (GLuint index, GLenum pname, GLvoid *  * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribPointervARB", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribPointervARB, index, pname, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsProgramARB (GLuint program)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsProgramARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsProgramARB, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBindBufferARB (GLenum target, GLuint buffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindBufferARB", __func__, FindTask(NULL)));
    GLCALL(glBindBufferARB, target, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteBuffersARB (GLsizei n, const GLuint * buffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteBuffersARB", __func__, FindTask(NULL)));
    GLCALL(glDeleteBuffersARB, n, buffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenBuffersARB (GLsizei n, GLuint * buffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenBuffersARB", __func__, FindTask(NULL)));
    GLCALL(glGenBuffersARB, n, buffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsBufferARB (GLuint buffer)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsBufferARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsBufferARB, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBufferDataARB (GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBufferDataARB", __func__, FindTask(NULL)));
    GLCALL(glBufferDataARB, target, size, data, usage);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBufferSubDataARB (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBufferSubDataARB", __func__, FindTask(NULL)));
    GLCALL(glBufferSubDataARB, target, offset, size, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetBufferSubDataARB (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBufferSubDataARB", __func__, FindTask(NULL)));
    GLCALL(glGetBufferSubDataARB, target, offset, size, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLvoid* glMapBufferARB (GLenum target, GLenum access)
{
    GLvoid* _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMapBufferARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glMapBufferARB, target, access);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

GLboolean glUnmapBufferARB (GLenum target)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUnmapBufferARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glUnmapBufferARB, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glGetBufferParameterivARB (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBufferParameterivARB", __func__, FindTask(NULL)));
    GLCALL(glGetBufferParameterivARB, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetBufferPointervARB (GLenum target, GLenum pname, GLvoid *  * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBufferPointervARB", __func__, FindTask(NULL)));
    GLCALL(glGetBufferPointervARB, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenQueriesARB (GLsizei n, GLuint * ids)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenQueriesARB", __func__, FindTask(NULL)));
    GLCALL(glGenQueriesARB, n, ids);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteQueriesARB (GLsizei n, const GLuint * ids)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteQueriesARB", __func__, FindTask(NULL)));
    GLCALL(glDeleteQueriesARB, n, ids);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsQueryARB (GLuint id)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsQueryARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsQueryARB, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBeginQueryARB (GLenum target, GLuint id)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBeginQueryARB", __func__, FindTask(NULL)));
    GLCALL(glBeginQueryARB, target, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEndQueryARB (GLenum target)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEndQueryARB", __func__, FindTask(NULL)));
    GLCALL(glEndQueryARB, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetQueryivARB (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetQueryivARB", __func__, FindTask(NULL)));
    GLCALL(glGetQueryivARB, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetQueryObjectivARB (GLuint id, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetQueryObjectivARB", __func__, FindTask(NULL)));
    GLCALL(glGetQueryObjectivARB, id, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetQueryObjectuivARB (GLuint id, GLenum pname, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetQueryObjectuivARB", __func__, FindTask(NULL)));
    GLCALL(glGetQueryObjectuivARB, id, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteObjectARB (GLhandleARB obj)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteObjectARB", __func__, FindTask(NULL)));
    GLCALL(glDeleteObjectARB, obj);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLhandleARB glGetHandleARB (GLenum pname)
{
    GLhandleARB _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetHandleARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetHandleARB, pname);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glDetachObjectARB (GLhandleARB containerObj, GLhandleARB attachedObj)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDetachObjectARB", __func__, FindTask(NULL)));
    GLCALL(glDetachObjectARB, containerObj, attachedObj);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLhandleARB glCreateShaderObjectARB (GLenum shaderType)
{
    GLhandleARB _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCreateShaderObjectARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glCreateShaderObjectARB, shaderType);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glShaderSourceARB (GLhandleARB shaderObj, GLsizei count, const GLcharARB *  * string, const GLint * length)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glShaderSourceARB", __func__, FindTask(NULL)));
    GLCALL(glShaderSourceARB, shaderObj, count, string, length);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCompileShaderARB (GLhandleARB shaderObj)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCompileShaderARB", __func__, FindTask(NULL)));
    GLCALL(glCompileShaderARB, shaderObj);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLhandleARB glCreateProgramObjectARB ()
{
    GLhandleARB _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCreateProgramObjectARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glCreateProgramObjectARB);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glAttachObjectARB (GLhandleARB containerObj, GLhandleARB obj)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAttachObjectARB", __func__, FindTask(NULL)));
    GLCALL(glAttachObjectARB, containerObj, obj);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLinkProgramARB (GLhandleARB programObj)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLinkProgramARB", __func__, FindTask(NULL)));
    GLCALL(glLinkProgramARB, programObj);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUseProgramObjectARB (GLhandleARB programObj)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUseProgramObjectARB", __func__, FindTask(NULL)));
    GLCALL(glUseProgramObjectARB, programObj);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glValidateProgramARB (GLhandleARB programObj)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glValidateProgramARB", __func__, FindTask(NULL)));
    GLCALL(glValidateProgramARB, programObj);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1fARB (GLint location, GLfloat v0)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1fARB", __func__, FindTask(NULL)));
    GLCALL(glUniform1fARB, location, v0);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2fARB (GLint location, GLfloat v0, GLfloat v1)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2fARB", __func__, FindTask(NULL)));
    GLCALL(glUniform2fARB, location, v0, v1);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3fARB", __func__, FindTask(NULL)));
    GLCALL(glUniform3fARB, location, v0, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4fARB", __func__, FindTask(NULL)));
    GLCALL(glUniform4fARB, location, v0, v1, v2, v3);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1iARB (GLint location, GLint v0)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1iARB", __func__, FindTask(NULL)));
    GLCALL(glUniform1iARB, location, v0);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2iARB (GLint location, GLint v0, GLint v1)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2iARB", __func__, FindTask(NULL)));
    GLCALL(glUniform2iARB, location, v0, v1);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3iARB (GLint location, GLint v0, GLint v1, GLint v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3iARB", __func__, FindTask(NULL)));
    GLCALL(glUniform3iARB, location, v0, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4iARB (GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4iARB", __func__, FindTask(NULL)));
    GLCALL(glUniform4iARB, location, v0, v1, v2, v3);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1fvARB (GLint location, GLsizei count, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1fvARB", __func__, FindTask(NULL)));
    GLCALL(glUniform1fvARB, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2fvARB (GLint location, GLsizei count, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2fvARB", __func__, FindTask(NULL)));
    GLCALL(glUniform2fvARB, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3fvARB (GLint location, GLsizei count, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3fvARB", __func__, FindTask(NULL)));
    GLCALL(glUniform3fvARB, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4fvARB (GLint location, GLsizei count, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4fvARB", __func__, FindTask(NULL)));
    GLCALL(glUniform4fvARB, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1ivARB (GLint location, GLsizei count, const GLint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1ivARB", __func__, FindTask(NULL)));
    GLCALL(glUniform1ivARB, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2ivARB (GLint location, GLsizei count, const GLint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2ivARB", __func__, FindTask(NULL)));
    GLCALL(glUniform2ivARB, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3ivARB (GLint location, GLsizei count, const GLint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3ivARB", __func__, FindTask(NULL)));
    GLCALL(glUniform3ivARB, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4ivARB (GLint location, GLsizei count, const GLint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4ivARB", __func__, FindTask(NULL)));
    GLCALL(glUniform4ivARB, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix2fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix2fvARB", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix2fvARB, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix3fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix3fvARB", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix3fvARB, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniformMatrix4fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniformMatrix4fvARB", __func__, FindTask(NULL)));
    GLCALL(glUniformMatrix4fvARB, location, count, transpose, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetObjectParameterfvARB (GLhandleARB obj, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetObjectParameterfvARB", __func__, FindTask(NULL)));
    GLCALL(glGetObjectParameterfvARB, obj, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetObjectParameterivARB (GLhandleARB obj, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetObjectParameterivARB", __func__, FindTask(NULL)));
    GLCALL(glGetObjectParameterivARB, obj, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetInfoLogARB (GLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * infoLog)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetInfoLogARB", __func__, FindTask(NULL)));
    GLCALL(glGetInfoLogARB, obj, maxLength, length, infoLog);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetAttachedObjectsARB (GLhandleARB containerObj, GLsizei maxCount, GLsizei * count, GLhandleARB * obj)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetAttachedObjectsARB", __func__, FindTask(NULL)));
    GLCALL(glGetAttachedObjectsARB, containerObj, maxCount, count, obj);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLint glGetUniformLocationARB (GLhandleARB programObj, const GLcharARB * name)
{
    GLint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetUniformLocationARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetUniformLocationARB, programObj, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glGetActiveUniformARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetActiveUniformARB", __func__, FindTask(NULL)));
    GLCALL(glGetActiveUniformARB, programObj, index, maxLength, length, size, type, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetUniformfvARB (GLhandleARB programObj, GLint location, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetUniformfvARB", __func__, FindTask(NULL)));
    GLCALL(glGetUniformfvARB, programObj, location, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetUniformivARB (GLhandleARB programObj, GLint location, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetUniformivARB", __func__, FindTask(NULL)));
    GLCALL(glGetUniformivARB, programObj, location, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetShaderSourceARB (GLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * source)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetShaderSourceARB", __func__, FindTask(NULL)));
    GLCALL(glGetShaderSourceARB, obj, maxLength, length, source);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindAttribLocationARB (GLhandleARB programObj, GLuint index, const GLcharARB * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindAttribLocationARB", __func__, FindTask(NULL)));
    GLCALL(glBindAttribLocationARB, programObj, index, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetActiveAttribARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetActiveAttribARB", __func__, FindTask(NULL)));
    GLCALL(glGetActiveAttribARB, programObj, index, maxLength, length, size, type, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLint glGetAttribLocationARB (GLhandleARB programObj, const GLcharARB * name)
{
    GLint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetAttribLocationARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetAttribLocationARB, programObj, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glDrawBuffersARB (GLsizei n, const GLenum * bufs)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawBuffersARB", __func__, FindTask(NULL)));
    GLCALL(glDrawBuffersARB, n, bufs);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsRenderbuffer (GLuint renderbuffer)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsRenderbuffer", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsRenderbuffer, renderbuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindRenderbuffer", __func__, FindTask(NULL)));
    GLCALL(glBindRenderbuffer, target, renderbuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteRenderbuffers (GLsizei n, const GLuint * renderbuffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteRenderbuffers", __func__, FindTask(NULL)));
    GLCALL(glDeleteRenderbuffers, n, renderbuffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenRenderbuffers (GLsizei n, GLuint * renderbuffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenRenderbuffers", __func__, FindTask(NULL)));
    GLCALL(glGenRenderbuffers, n, renderbuffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRenderbufferStorage", __func__, FindTask(NULL)));
    GLCALL(glRenderbufferStorage, target, internalformat, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetRenderbufferParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetRenderbufferParameteriv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsFramebuffer (GLuint framebuffer)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsFramebuffer", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsFramebuffer, framebuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBindFramebuffer (GLenum target, GLuint framebuffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindFramebuffer", __func__, FindTask(NULL)));
    GLCALL(glBindFramebuffer, target, framebuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteFramebuffers (GLsizei n, const GLuint * framebuffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteFramebuffers", __func__, FindTask(NULL)));
    GLCALL(glDeleteFramebuffers, n, framebuffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenFramebuffers (GLsizei n, GLuint * framebuffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenFramebuffers", __func__, FindTask(NULL)));
    GLCALL(glGenFramebuffers, n, framebuffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLenum glCheckFramebufferStatus (GLenum target)
{
    GLenum _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCheckFramebufferStatus", __func__, FindTask(NULL)));
    _ret = GLCALL(glCheckFramebufferStatus, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glFramebufferTexture1D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTexture1D", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTexture1D, target, attachment, textarget, texture, level);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTexture2D", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTexture2D, target, attachment, textarget, texture, level);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTexture3D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTexture3D", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTexture3D, target, attachment, textarget, texture, level, zoffset);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferRenderbuffer", __func__, FindTask(NULL)));
    GLCALL(glFramebufferRenderbuffer, target, attachment, renderbuffertarget, renderbuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetFramebufferAttachmentParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetFramebufferAttachmentParameteriv, target, attachment, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenerateMipmap (GLenum target)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenerateMipmap", __func__, FindTask(NULL)));
    GLCALL(glGenerateMipmap, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlitFramebuffer", __func__, FindTask(NULL)));
    GLCALL(glBlitFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRenderbufferStorageMultisample (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRenderbufferStorageMultisample", __func__, FindTask(NULL)));
    GLCALL(glRenderbufferStorageMultisample, target, samples, internalformat, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTextureLayer (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTextureLayer", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTextureLayer, target, attachment, texture, level, layer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendColorEXT (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendColorEXT", __func__, FindTask(NULL)));
    GLCALL(glBlendColorEXT, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPolygonOffsetEXT (GLfloat factor, GLfloat bias)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPolygonOffsetEXT", __func__, FindTask(NULL)));
    GLCALL(glPolygonOffsetEXT, factor, bias);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexImage3DEXT (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexImage3DEXT", __func__, FindTask(NULL)));
    GLCALL(glTexImage3DEXT, target, level, internalformat, width, height, depth, border, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexSubImage3DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexSubImage3DEXT", __func__, FindTask(NULL)));
    GLCALL(glTexSubImage3DEXT, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexSubImage1DEXT (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexSubImage1DEXT", __func__, FindTask(NULL)));
    GLCALL(glTexSubImage1DEXT, target, level, xoffset, width, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexSubImage2DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexSubImage2DEXT", __func__, FindTask(NULL)));
    GLCALL(glTexSubImage2DEXT, target, level, xoffset, yoffset, width, height, format, type, pixels);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexImage1DEXT (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexImage1DEXT", __func__, FindTask(NULL)));
    GLCALL(glCopyTexImage1DEXT, target, level, internalformat, x, y, width, border);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexImage2DEXT (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexImage2DEXT", __func__, FindTask(NULL)));
    GLCALL(glCopyTexImage2DEXT, target, level, internalformat, x, y, width, height, border);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexSubImage1DEXT (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexSubImage1DEXT", __func__, FindTask(NULL)));
    GLCALL(glCopyTexSubImage1DEXT, target, level, xoffset, x, y, width);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexSubImage2DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexSubImage2DEXT", __func__, FindTask(NULL)));
    GLCALL(glCopyTexSubImage2DEXT, target, level, xoffset, yoffset, x, y, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyTexSubImage3DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyTexSubImage3DEXT", __func__, FindTask(NULL)));
    GLCALL(glCopyTexSubImage3DEXT, target, level, xoffset, yoffset, zoffset, x, y, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glAreTexturesResidentEXT (GLsizei n, const GLuint * textures, GLboolean * residences)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAreTexturesResidentEXT", __func__, FindTask(NULL)));
    _ret = GLCALL(glAreTexturesResidentEXT, n, textures, residences);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBindTextureEXT (GLenum target, GLuint texture)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindTextureEXT", __func__, FindTask(NULL)));
    GLCALL(glBindTextureEXT, target, texture);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteTexturesEXT (GLsizei n, const GLuint * textures)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteTexturesEXT", __func__, FindTask(NULL)));
    GLCALL(glDeleteTexturesEXT, n, textures);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenTexturesEXT (GLsizei n, GLuint * textures)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenTexturesEXT", __func__, FindTask(NULL)));
    GLCALL(glGenTexturesEXT, n, textures);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsTextureEXT (GLuint texture)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsTextureEXT", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsTextureEXT, texture);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glPrioritizeTexturesEXT (GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPrioritizeTexturesEXT", __func__, FindTask(NULL)));
    GLCALL(glPrioritizeTexturesEXT, n, textures, priorities);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glArrayElementEXT (GLint i)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glArrayElementEXT", __func__, FindTask(NULL)));
    GLCALL(glArrayElementEXT, i);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorPointerEXT (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorPointerEXT", __func__, FindTask(NULL)));
    GLCALL(glColorPointerEXT, size, type, stride, count, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawArraysEXT (GLenum mode, GLint first, GLsizei count)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawArraysEXT", __func__, FindTask(NULL)));
    GLCALL(glDrawArraysEXT, mode, first, count);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEdgeFlagPointerEXT (GLsizei stride, GLsizei count, const GLboolean * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEdgeFlagPointerEXT", __func__, FindTask(NULL)));
    GLCALL(glEdgeFlagPointerEXT, stride, count, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetPointervEXT (GLenum pname, GLvoid *  * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetPointervEXT", __func__, FindTask(NULL)));
    GLCALL(glGetPointervEXT, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glIndexPointerEXT (GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIndexPointerEXT", __func__, FindTask(NULL)));
    GLCALL(glIndexPointerEXT, type, stride, count, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glNormalPointerEXT (GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glNormalPointerEXT", __func__, FindTask(NULL)));
    GLCALL(glNormalPointerEXT, type, stride, count, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexCoordPointerEXT (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexCoordPointerEXT", __func__, FindTask(NULL)));
    GLCALL(glTexCoordPointerEXT, size, type, stride, count, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexPointerEXT (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexPointerEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexPointerEXT, size, type, stride, count, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendEquationEXT (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendEquationEXT", __func__, FindTask(NULL)));
    GLCALL(glBlendEquationEXT, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameterfEXT (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameterfEXT", __func__, FindTask(NULL)));
    GLCALL(glPointParameterfEXT, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameterfvEXT (GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameterfvEXT", __func__, FindTask(NULL)));
    GLCALL(glPointParameterfvEXT, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorTableEXT (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorTableEXT", __func__, FindTask(NULL)));
    GLCALL(glColorTableEXT, target, internalFormat, width, format, type, table);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetColorTableEXT (GLenum target, GLenum format, GLenum type, GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetColorTableEXT", __func__, FindTask(NULL)));
    GLCALL(glGetColorTableEXT, target, format, type, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetColorTableParameterivEXT (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetColorTableParameterivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetColorTableParameterivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetColorTableParameterfvEXT (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetColorTableParameterfvEXT", __func__, FindTask(NULL)));
    GLCALL(glGetColorTableParameterfvEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glLockArraysEXT (GLint first, GLsizei count)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLockArraysEXT", __func__, FindTask(NULL)));
    GLCALL(glLockArraysEXT, first, count);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUnlockArraysEXT ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUnlockArraysEXT", __func__, FindTask(NULL)));
    GLCALL(glUnlockArraysEXT);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawRangeElementsEXT (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawRangeElementsEXT", __func__, FindTask(NULL)));
    GLCALL(glDrawRangeElementsEXT, mode, start, end, count, type, indices);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3bEXT (GLbyte red, GLbyte green, GLbyte blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3bEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3bEXT, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3bvEXT (const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3bvEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3bvEXT, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3dEXT (GLdouble red, GLdouble green, GLdouble blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3dEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3dEXT, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3dvEXT (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3dvEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3dvEXT, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3fEXT (GLfloat red, GLfloat green, GLfloat blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3fEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3fEXT, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3fvEXT (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3fvEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3fvEXT, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3iEXT (GLint red, GLint green, GLint blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3iEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3iEXT, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3ivEXT (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3ivEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3ivEXT, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3sEXT (GLshort red, GLshort green, GLshort blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3sEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3sEXT, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3svEXT (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3svEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3svEXT, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3ubEXT (GLubyte red, GLubyte green, GLubyte blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3ubEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3ubEXT, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3ubvEXT (const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3ubvEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3ubvEXT, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3uiEXT (GLuint red, GLuint green, GLuint blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3uiEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3uiEXT, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3uivEXT (const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3uivEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3uivEXT, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3usEXT (GLushort red, GLushort green, GLushort blue)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3usEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3usEXT, red, green, blue);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColor3usvEXT (const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColor3usvEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColor3usvEXT, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSecondaryColorPointerEXT (GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSecondaryColorPointerEXT", __func__, FindTask(NULL)));
    GLCALL(glSecondaryColorPointerEXT, size, type, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiDrawArraysEXT (GLenum mode, const GLint * first, const GLsizei * count, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiDrawArraysEXT", __func__, FindTask(NULL)));
    GLCALL(glMultiDrawArraysEXT, mode, first, count, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiDrawElementsEXT (GLenum mode, const GLsizei * count, GLenum type, const GLvoid *  * indices, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiDrawElementsEXT", __func__, FindTask(NULL)));
    GLCALL(glMultiDrawElementsEXT, mode, count, type, indices, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoordfEXT (GLfloat coord)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoordfEXT", __func__, FindTask(NULL)));
    GLCALL(glFogCoordfEXT, coord);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoordfvEXT (const GLfloat * coord)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoordfvEXT", __func__, FindTask(NULL)));
    GLCALL(glFogCoordfvEXT, coord);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoorddEXT (GLdouble coord)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoorddEXT", __func__, FindTask(NULL)));
    GLCALL(glFogCoorddEXT, coord);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoorddvEXT (const GLdouble * coord)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoorddvEXT", __func__, FindTask(NULL)));
    GLCALL(glFogCoorddvEXT, coord);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFogCoordPointerEXT (GLenum type, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFogCoordPointerEXT", __func__, FindTask(NULL)));
    GLCALL(glFogCoordPointerEXT, type, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendFuncSeparateEXT (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendFuncSeparateEXT", __func__, FindTask(NULL)));
    GLCALL(glBlendFuncSeparateEXT, sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFlushVertexArrayRangeNV ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFlushVertexArrayRangeNV", __func__, FindTask(NULL)));
    GLCALL(glFlushVertexArrayRangeNV);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexArrayRangeNV (GLsizei length, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexArrayRangeNV", __func__, FindTask(NULL)));
    GLCALL(glVertexArrayRangeNV, length, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCombinerParameterfvNV (GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCombinerParameterfvNV", __func__, FindTask(NULL)));
    GLCALL(glCombinerParameterfvNV, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCombinerParameterfNV (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCombinerParameterfNV", __func__, FindTask(NULL)));
    GLCALL(glCombinerParameterfNV, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCombinerParameterivNV (GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCombinerParameterivNV", __func__, FindTask(NULL)));
    GLCALL(glCombinerParameterivNV, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCombinerParameteriNV (GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCombinerParameteriNV", __func__, FindTask(NULL)));
    GLCALL(glCombinerParameteriNV, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCombinerInputNV (GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCombinerInputNV", __func__, FindTask(NULL)));
    GLCALL(glCombinerInputNV, stage, portion, variable, input, mapping, componentUsage);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCombinerOutputNV (GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCombinerOutputNV", __func__, FindTask(NULL)));
    GLCALL(glCombinerOutputNV, stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFinalCombinerInputNV (GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFinalCombinerInputNV", __func__, FindTask(NULL)));
    GLCALL(glFinalCombinerInputNV, variable, input, mapping, componentUsage);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetCombinerInputParameterfvNV (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetCombinerInputParameterfvNV", __func__, FindTask(NULL)));
    GLCALL(glGetCombinerInputParameterfvNV, stage, portion, variable, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetCombinerInputParameterivNV (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetCombinerInputParameterivNV", __func__, FindTask(NULL)));
    GLCALL(glGetCombinerInputParameterivNV, stage, portion, variable, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetCombinerOutputParameterfvNV (GLenum stage, GLenum portion, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetCombinerOutputParameterfvNV", __func__, FindTask(NULL)));
    GLCALL(glGetCombinerOutputParameterfvNV, stage, portion, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetCombinerOutputParameterivNV (GLenum stage, GLenum portion, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetCombinerOutputParameterivNV", __func__, FindTask(NULL)));
    GLCALL(glGetCombinerOutputParameterivNV, stage, portion, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetFinalCombinerInputParameterfvNV (GLenum variable, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetFinalCombinerInputParameterfvNV", __func__, FindTask(NULL)));
    GLCALL(glGetFinalCombinerInputParameterfvNV, variable, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetFinalCombinerInputParameterivNV (GLenum variable, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetFinalCombinerInputParameterivNV", __func__, FindTask(NULL)));
    GLCALL(glGetFinalCombinerInputParameterivNV, variable, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glResizeBuffersMESA ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glResizeBuffersMESA", __func__, FindTask(NULL)));
    GLCALL(glResizeBuffersMESA);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2dMESA (GLdouble x, GLdouble y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2dMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2dMESA, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2dvMESA (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2dvMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2dvMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2fMESA (GLfloat x, GLfloat y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2fMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2fMESA, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2fvMESA (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2fvMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2fvMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2iMESA (GLint x, GLint y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2iMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2iMESA, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2ivMESA (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2ivMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2ivMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2sMESA (GLshort x, GLshort y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2sMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2sMESA, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos2svMESA (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos2svMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos2svMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3dMESA (GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3dMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3dMESA, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3dvMESA (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3dvMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3dvMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3fMESA (GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3fMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3fMESA, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3fvMESA (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3fvMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3fvMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3iMESA (GLint x, GLint y, GLint z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3iMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3iMESA, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3ivMESA (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3ivMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3ivMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3sMESA (GLshort x, GLshort y, GLshort z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3sMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3sMESA, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos3svMESA (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos3svMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos3svMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos4dMESA (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos4dMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos4dMESA, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos4dvMESA (const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos4dvMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos4dvMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos4fMESA (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos4fMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos4fMESA, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos4fvMESA (const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos4fvMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos4fvMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos4iMESA (GLint x, GLint y, GLint z, GLint w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos4iMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos4iMESA, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos4ivMESA (const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos4ivMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos4ivMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos4sMESA (GLshort x, GLshort y, GLshort z, GLshort w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos4sMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos4sMESA, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glWindowPos4svMESA (const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWindowPos4svMESA", __func__, FindTask(NULL)));
    GLCALL(glWindowPos4svMESA, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glAreProgramsResidentNV (GLsizei n, const GLuint * programs, GLboolean * residences)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAreProgramsResidentNV", __func__, FindTask(NULL)));
    _ret = GLCALL(glAreProgramsResidentNV, n, programs, residences);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBindProgramNV (GLenum target, GLuint id)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindProgramNV", __func__, FindTask(NULL)));
    GLCALL(glBindProgramNV, target, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteProgramsNV (GLsizei n, const GLuint * programs)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteProgramsNV", __func__, FindTask(NULL)));
    GLCALL(glDeleteProgramsNV, n, programs);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glExecuteProgramNV (GLenum target, GLuint id, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glExecuteProgramNV", __func__, FindTask(NULL)));
    GLCALL(glExecuteProgramNV, target, id, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenProgramsNV (GLsizei n, GLuint * programs)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenProgramsNV", __func__, FindTask(NULL)));
    GLCALL(glGenProgramsNV, n, programs);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramParameterdvNV (GLenum target, GLuint index, GLenum pname, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramParameterdvNV", __func__, FindTask(NULL)));
    GLCALL(glGetProgramParameterdvNV, target, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramParameterfvNV (GLenum target, GLuint index, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramParameterfvNV", __func__, FindTask(NULL)));
    GLCALL(glGetProgramParameterfvNV, target, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramivNV (GLuint id, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramivNV", __func__, FindTask(NULL)));
    GLCALL(glGetProgramivNV, id, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramStringNV (GLuint id, GLenum pname, GLubyte * program)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramStringNV", __func__, FindTask(NULL)));
    GLCALL(glGetProgramStringNV, id, pname, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTrackMatrixivNV (GLenum target, GLuint address, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTrackMatrixivNV", __func__, FindTask(NULL)));
    GLCALL(glGetTrackMatrixivNV, target, address, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribdvNV (GLuint index, GLenum pname, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribdvNV", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribdvNV, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribfvNV (GLuint index, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribfvNV", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribfvNV, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribivNV (GLuint index, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribivNV", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribivNV, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribPointervNV (GLuint index, GLenum pname, GLvoid *  * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribPointervNV", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribPointervNV, index, pname, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsProgramNV (GLuint id)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsProgramNV", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsProgramNV, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glLoadProgramNV (GLenum target, GLuint id, GLsizei len, const GLubyte * program)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glLoadProgramNV", __func__, FindTask(NULL)));
    GLCALL(glLoadProgramNV, target, id, len, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramParameter4dNV (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramParameter4dNV", __func__, FindTask(NULL)));
    GLCALL(glProgramParameter4dNV, target, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramParameter4dvNV (GLenum target, GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramParameter4dvNV", __func__, FindTask(NULL)));
    GLCALL(glProgramParameter4dvNV, target, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramParameter4fNV (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramParameter4fNV", __func__, FindTask(NULL)));
    GLCALL(glProgramParameter4fNV, target, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramParameter4fvNV (GLenum target, GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramParameter4fvNV", __func__, FindTask(NULL)));
    GLCALL(glProgramParameter4fvNV, target, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramParameters4dvNV (GLenum target, GLuint index, GLsizei count, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramParameters4dvNV", __func__, FindTask(NULL)));
    GLCALL(glProgramParameters4dvNV, target, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramParameters4fvNV (GLenum target, GLuint index, GLsizei count, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramParameters4fvNV", __func__, FindTask(NULL)));
    GLCALL(glProgramParameters4fvNV, target, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRequestResidentProgramsNV (GLsizei n, const GLuint * programs)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRequestResidentProgramsNV", __func__, FindTask(NULL)));
    GLCALL(glRequestResidentProgramsNV, n, programs);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTrackMatrixNV (GLenum target, GLuint address, GLenum matrix, GLenum transform)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTrackMatrixNV", __func__, FindTask(NULL)));
    GLCALL(glTrackMatrixNV, target, address, matrix, transform);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribPointerNV (GLuint index, GLint fsize, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribPointerNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribPointerNV, index, fsize, type, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1dNV (GLuint index, GLdouble x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1dNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1dNV, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1dvNV (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1dvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1dvNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1fNV (GLuint index, GLfloat x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1fNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1fNV, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1fvNV (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1fvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1fvNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1sNV (GLuint index, GLshort x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1sNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1sNV, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib1svNV (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib1svNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib1svNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2dNV (GLuint index, GLdouble x, GLdouble y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2dNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2dNV, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2dvNV (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2dvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2dvNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2fNV (GLuint index, GLfloat x, GLfloat y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2fNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2fNV, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2fvNV (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2fvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2fvNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2sNV (GLuint index, GLshort x, GLshort y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2sNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2sNV, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib2svNV (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib2svNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib2svNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3dNV (GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3dNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3dNV, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3dvNV (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3dvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3dvNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3fNV (GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3fNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3fNV, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3fvNV (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3fvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3fvNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3sNV (GLuint index, GLshort x, GLshort y, GLshort z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3sNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3sNV, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib3svNV (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib3svNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib3svNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4dNV (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4dNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4dNV, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4dvNV (GLuint index, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4dvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4dvNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4fNV (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4fNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4fNV, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4fvNV (GLuint index, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4fvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4fvNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4sNV (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4sNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4sNV, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4svNV (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4svNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4svNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4ubNV (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4ubNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4ubNV, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttrib4ubvNV (GLuint index, const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttrib4ubvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttrib4ubvNV, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs1dvNV (GLuint index, GLsizei count, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs1dvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs1dvNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs1fvNV (GLuint index, GLsizei count, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs1fvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs1fvNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs1svNV (GLuint index, GLsizei count, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs1svNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs1svNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs2dvNV (GLuint index, GLsizei count, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs2dvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs2dvNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs2fvNV (GLuint index, GLsizei count, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs2fvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs2fvNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs2svNV (GLuint index, GLsizei count, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs2svNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs2svNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs3dvNV (GLuint index, GLsizei count, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs3dvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs3dvNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs3fvNV (GLuint index, GLsizei count, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs3fvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs3fvNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs3svNV (GLuint index, GLsizei count, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs3svNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs3svNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs4dvNV (GLuint index, GLsizei count, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs4dvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs4dvNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs4fvNV (GLuint index, GLsizei count, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs4fvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs4fvNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs4svNV (GLuint index, GLsizei count, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs4svNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs4svNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribs4ubvNV (GLuint index, GLsizei count, const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribs4ubvNV", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribs4ubvNV, index, count, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexBumpParameterivATI (GLenum pname, const GLint * param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexBumpParameterivATI", __func__, FindTask(NULL)));
    GLCALL(glTexBumpParameterivATI, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexBumpParameterfvATI (GLenum pname, const GLfloat * param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexBumpParameterfvATI", __func__, FindTask(NULL)));
    GLCALL(glTexBumpParameterfvATI, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexBumpParameterivATI (GLenum pname, GLint * param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexBumpParameterivATI", __func__, FindTask(NULL)));
    GLCALL(glGetTexBumpParameterivATI, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexBumpParameterfvATI (GLenum pname, GLfloat * param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexBumpParameterfvATI", __func__, FindTask(NULL)));
    GLCALL(glGetTexBumpParameterfvATI, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLuint glGenFragmentShadersATI (GLuint range)
{
    GLuint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenFragmentShadersATI", __func__, FindTask(NULL)));
    _ret = GLCALL(glGenFragmentShadersATI, range);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBindFragmentShaderATI (GLuint id)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindFragmentShaderATI", __func__, FindTask(NULL)));
    GLCALL(glBindFragmentShaderATI, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteFragmentShaderATI (GLuint id)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteFragmentShaderATI", __func__, FindTask(NULL)));
    GLCALL(glDeleteFragmentShaderATI, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBeginFragmentShaderATI ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBeginFragmentShaderATI", __func__, FindTask(NULL)));
    GLCALL(glBeginFragmentShaderATI);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEndFragmentShaderATI ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEndFragmentShaderATI", __func__, FindTask(NULL)));
    GLCALL(glEndFragmentShaderATI);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPassTexCoordATI (GLuint dst, GLuint coord, GLenum swizzle)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPassTexCoordATI", __func__, FindTask(NULL)));
    GLCALL(glPassTexCoordATI, dst, coord, swizzle);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSampleMapATI (GLuint dst, GLuint interp, GLenum swizzle)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSampleMapATI", __func__, FindTask(NULL)));
    GLCALL(glSampleMapATI, dst, interp, swizzle);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorFragmentOp1ATI (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorFragmentOp1ATI", __func__, FindTask(NULL)));
    GLCALL(glColorFragmentOp1ATI, op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorFragmentOp2ATI (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorFragmentOp2ATI", __func__, FindTask(NULL)));
    GLCALL(glColorFragmentOp2ATI, op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorFragmentOp3ATI (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorFragmentOp3ATI", __func__, FindTask(NULL)));
    GLCALL(glColorFragmentOp3ATI, op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glAlphaFragmentOp1ATI (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAlphaFragmentOp1ATI", __func__, FindTask(NULL)));
    GLCALL(glAlphaFragmentOp1ATI, op, dst, dstMod, arg1, arg1Rep, arg1Mod);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glAlphaFragmentOp2ATI (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAlphaFragmentOp2ATI", __func__, FindTask(NULL)));
    GLCALL(glAlphaFragmentOp2ATI, op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glAlphaFragmentOp3ATI (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glAlphaFragmentOp3ATI", __func__, FindTask(NULL)));
    GLCALL(glAlphaFragmentOp3ATI, op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSetFragmentShaderConstantATI (GLuint dst, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSetFragmentShaderConstantATI", __func__, FindTask(NULL)));
    GLCALL(glSetFragmentShaderConstantATI, dst, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameteriNV (GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameteriNV", __func__, FindTask(NULL)));
    GLCALL(glPointParameteriNV, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameterivNV (GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameterivNV", __func__, FindTask(NULL)));
    GLCALL(glPointParameterivNV, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawBuffersATI (GLsizei n, const GLenum * bufs)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawBuffersATI", __func__, FindTask(NULL)));
    GLCALL(glDrawBuffersATI, n, bufs);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramNamedParameter4fNV (GLuint id, GLsizei len, const GLubyte * name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramNamedParameter4fNV", __func__, FindTask(NULL)));
    GLCALL(glProgramNamedParameter4fNV, id, len, name, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramNamedParameter4dNV (GLuint id, GLsizei len, const GLubyte * name, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramNamedParameter4dNV", __func__, FindTask(NULL)));
    GLCALL(glProgramNamedParameter4dNV, id, len, name, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramNamedParameter4fvNV (GLuint id, GLsizei len, const GLubyte * name, const GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramNamedParameter4fvNV", __func__, FindTask(NULL)));
    GLCALL(glProgramNamedParameter4fvNV, id, len, name, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramNamedParameter4dvNV (GLuint id, GLsizei len, const GLubyte * name, const GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramNamedParameter4dvNV", __func__, FindTask(NULL)));
    GLCALL(glProgramNamedParameter4dvNV, id, len, name, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramNamedParameterfvNV (GLuint id, GLsizei len, const GLubyte * name, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramNamedParameterfvNV", __func__, FindTask(NULL)));
    GLCALL(glGetProgramNamedParameterfvNV, id, len, name, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetProgramNamedParameterdvNV (GLuint id, GLsizei len, const GLubyte * name, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetProgramNamedParameterdvNV", __func__, FindTask(NULL)));
    GLCALL(glGetProgramNamedParameterdvNV, id, len, name, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsRenderbufferEXT (GLuint renderbuffer)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsRenderbufferEXT", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsRenderbufferEXT, renderbuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBindRenderbufferEXT (GLenum target, GLuint renderbuffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindRenderbufferEXT", __func__, FindTask(NULL)));
    GLCALL(glBindRenderbufferEXT, target, renderbuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteRenderbuffersEXT (GLsizei n, const GLuint * renderbuffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteRenderbuffersEXT", __func__, FindTask(NULL)));
    GLCALL(glDeleteRenderbuffersEXT, n, renderbuffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenRenderbuffersEXT (GLsizei n, GLuint * renderbuffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenRenderbuffersEXT", __func__, FindTask(NULL)));
    GLCALL(glGenRenderbuffersEXT, n, renderbuffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRenderbufferStorageEXT (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRenderbufferStorageEXT", __func__, FindTask(NULL)));
    GLCALL(glRenderbufferStorageEXT, target, internalformat, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetRenderbufferParameterivEXT (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetRenderbufferParameterivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetRenderbufferParameterivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsFramebufferEXT (GLuint framebuffer)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsFramebufferEXT", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsFramebufferEXT, framebuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBindFramebufferEXT (GLenum target, GLuint framebuffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindFramebufferEXT", __func__, FindTask(NULL)));
    GLCALL(glBindFramebufferEXT, target, framebuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteFramebuffersEXT (GLsizei n, const GLuint * framebuffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteFramebuffersEXT", __func__, FindTask(NULL)));
    GLCALL(glDeleteFramebuffersEXT, n, framebuffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenFramebuffersEXT (GLsizei n, GLuint * framebuffers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenFramebuffersEXT", __func__, FindTask(NULL)));
    GLCALL(glGenFramebuffersEXT, n, framebuffers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLenum glCheckFramebufferStatusEXT (GLenum target)
{
    GLenum _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCheckFramebufferStatusEXT", __func__, FindTask(NULL)));
    _ret = GLCALL(glCheckFramebufferStatusEXT, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glFramebufferTexture1DEXT (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTexture1DEXT", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTexture1DEXT, target, attachment, textarget, texture, level);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTexture2DEXT (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTexture2DEXT", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTexture2DEXT, target, attachment, textarget, texture, level);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTexture3DEXT (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTexture3DEXT", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTexture3DEXT, target, attachment, textarget, texture, level, zoffset);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferRenderbufferEXT (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferRenderbufferEXT", __func__, FindTask(NULL)));
    GLCALL(glFramebufferRenderbufferEXT, target, attachment, renderbuffertarget, renderbuffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetFramebufferAttachmentParameterivEXT (GLenum target, GLenum attachment, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetFramebufferAttachmentParameterivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetFramebufferAttachmentParameterivEXT, target, attachment, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenerateMipmapEXT (GLenum target)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenerateMipmapEXT", __func__, FindTask(NULL)));
    GLCALL(glGenerateMipmapEXT, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTextureLayerEXT (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTextureLayerEXT", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTextureLayerEXT, target, attachment, texture, level, layer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLvoid* glMapBufferRange (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    GLvoid* _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMapBufferRange", __func__, FindTask(NULL)));
    _ret = GLCALL(glMapBufferRange, target, offset, length, access);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glFlushMappedBufferRange (GLenum target, GLintptr offset, GLsizeiptr length)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFlushMappedBufferRange", __func__, FindTask(NULL)));
    GLCALL(glFlushMappedBufferRange, target, offset, length);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindVertexArray (GLuint array)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindVertexArray", __func__, FindTask(NULL)));
    GLCALL(glBindVertexArray, array);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteVertexArrays (GLsizei n, const GLuint * arrays)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteVertexArrays", __func__, FindTask(NULL)));
    GLCALL(glDeleteVertexArrays, n, arrays);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenVertexArrays (GLsizei n, GLuint * arrays)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenVertexArrays", __func__, FindTask(NULL)));
    GLCALL(glGenVertexArrays, n, arrays);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsVertexArray (GLuint array)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsVertexArray", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsVertexArray, array);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glCopyBufferSubData (GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyBufferSubData", __func__, FindTask(NULL)));
    GLCALL(glCopyBufferSubData, readTarget, writeTarget, readOffset, writeOffset, size);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLsync glFenceSync (GLenum condition, GLbitfield flags)
{
    GLsync _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFenceSync", __func__, FindTask(NULL)));
    _ret = GLCALL(glFenceSync, condition, flags);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

GLboolean glIsSync (GLsync sync)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsSync", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsSync, sync);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glDeleteSync (GLsync sync)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteSync", __func__, FindTask(NULL)));
    GLCALL(glDeleteSync, sync);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLenum glClientWaitSync (GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    GLenum _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClientWaitSync", __func__, FindTask(NULL)));
    _ret = GLCALL(glClientWaitSync, sync, flags, timeout);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glWaitSync (GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glWaitSync", __func__, FindTask(NULL)));
    GLCALL(glWaitSync, sync, flags, timeout);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetInteger64v (GLenum pname, GLint64 * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetInteger64v", __func__, FindTask(NULL)));
    GLCALL(glGetInteger64v, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetSynciv (GLsync sync, GLenum pname, GLsizei bufSize, GLsizei * length, GLint * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetSynciv", __func__, FindTask(NULL)));
    GLCALL(glGetSynciv, sync, pname, bufSize, length, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProvokingVertexEXT (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProvokingVertexEXT", __func__, FindTask(NULL)));
    GLCALL(glProvokingVertexEXT, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawElementsBaseVertex (GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLint basevertex)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawElementsBaseVertex", __func__, FindTask(NULL)));
    GLCALL(glDrawElementsBaseVertex, mode, count, type, indices, basevertex);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawRangeElementsBaseVertex (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices, GLint basevertex)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawRangeElementsBaseVertex", __func__, FindTask(NULL)));
    GLCALL(glDrawRangeElementsBaseVertex, mode, start, end, count, type, indices, basevertex);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiDrawElementsBaseVertex (GLenum mode, const GLsizei * count, GLenum type, const GLvoid *  * indices, GLsizei primcount, const GLint * basevertex)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiDrawElementsBaseVertex", __func__, FindTask(NULL)));
    GLCALL(glMultiDrawElementsBaseVertex, mode, count, type, indices, primcount, basevertex);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProvokingVertex (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProvokingVertex", __func__, FindTask(NULL)));
    GLCALL(glProvokingVertex, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glRenderbufferStorageMultisampleEXT (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glRenderbufferStorageMultisampleEXT", __func__, FindTask(NULL)));
    GLCALL(glRenderbufferStorageMultisampleEXT, target, samples, internalformat, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorMaskIndexedEXT (GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorMaskIndexedEXT", __func__, FindTask(NULL)));
    GLCALL(glColorMaskIndexedEXT, index, r, g, b, a);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetBooleanIndexedvEXT (GLenum target, GLuint index, GLboolean * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBooleanIndexedvEXT", __func__, FindTask(NULL)));
    GLCALL(glGetBooleanIndexedvEXT, target, index, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetIntegerIndexedvEXT (GLenum target, GLuint index, GLint * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetIntegerIndexedvEXT", __func__, FindTask(NULL)));
    GLCALL(glGetIntegerIndexedvEXT, target, index, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEnableIndexedEXT (GLenum target, GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEnableIndexedEXT", __func__, FindTask(NULL)));
    GLCALL(glEnableIndexedEXT, target, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDisableIndexedEXT (GLenum target, GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDisableIndexedEXT", __func__, FindTask(NULL)));
    GLCALL(glDisableIndexedEXT, target, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsEnabledIndexedEXT (GLenum target, GLuint index)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsEnabledIndexedEXT", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsEnabledIndexedEXT, target, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBeginConditionalRenderNV (GLuint id, GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBeginConditionalRenderNV", __func__, FindTask(NULL)));
    GLCALL(glBeginConditionalRenderNV, id, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEndConditionalRenderNV ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEndConditionalRenderNV", __func__, FindTask(NULL)));
    GLCALL(glEndConditionalRenderNV);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLenum glObjectPurgeableAPPLE (GLenum objectType, GLuint name, GLenum option)
{
    GLenum _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glObjectPurgeableAPPLE", __func__, FindTask(NULL)));
    _ret = GLCALL(glObjectPurgeableAPPLE, objectType, name, option);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

GLenum glObjectUnpurgeableAPPLE (GLenum objectType, GLuint name, GLenum option)
{
    GLenum _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glObjectUnpurgeableAPPLE", __func__, FindTask(NULL)));
    _ret = GLCALL(glObjectUnpurgeableAPPLE, objectType, name, option);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glGetObjectParameterivAPPLE (GLenum objectType, GLuint name, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetObjectParameterivAPPLE", __func__, FindTask(NULL)));
    GLCALL(glGetObjectParameterivAPPLE, objectType, name, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBeginTransformFeedback (GLenum primitiveMode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBeginTransformFeedback", __func__, FindTask(NULL)));
    GLCALL(glBeginTransformFeedback, primitiveMode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEndTransformFeedback ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEndTransformFeedback", __func__, FindTask(NULL)));
    GLCALL(glEndTransformFeedback);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindBufferRange (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindBufferRange", __func__, FindTask(NULL)));
    GLCALL(glBindBufferRange, target, index, buffer, offset, size);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindBufferBase (GLenum target, GLuint index, GLuint buffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindBufferBase", __func__, FindTask(NULL)));
    GLCALL(glBindBufferBase, target, index, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTransformFeedbackVaryings (GLuint program, GLsizei count, const GLchar *  * varyings, GLenum bufferMode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTransformFeedbackVaryings", __func__, FindTask(NULL)));
    GLCALL(glTransformFeedbackVaryings, program, count, varyings, bufferMode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTransformFeedbackVarying (GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTransformFeedbackVarying", __func__, FindTask(NULL)));
    GLCALL(glGetTransformFeedbackVarying, program, index, bufSize, length, size, type, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawArraysInstanced (GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawArraysInstanced", __func__, FindTask(NULL)));
    GLCALL(glDrawArraysInstanced, mode, first, count, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawElementsInstanced (GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawElementsInstanced", __func__, FindTask(NULL)));
    GLCALL(glDrawElementsInstanced, mode, count, type, indices, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawArraysInstancedARB (GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawArraysInstancedARB", __func__, FindTask(NULL)));
    GLCALL(glDrawArraysInstancedARB, mode, first, count, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawElementsInstancedARB (GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawElementsInstancedARB", __func__, FindTask(NULL)));
    GLCALL(glDrawElementsInstancedARB, mode, count, type, indices, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramParameteriARB (GLuint program, GLenum pname, GLint value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramParameteriARB", __func__, FindTask(NULL)));
    GLCALL(glProgramParameteriARB, program, pname, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTextureARB (GLenum target, GLenum attachment, GLuint texture, GLint level)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTextureARB", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTextureARB, target, attachment, texture, level);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTextureFaceARB (GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTextureFaceARB", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTextureFaceARB, target, attachment, texture, level, face);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindTransformFeedback (GLenum target, GLuint id)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindTransformFeedback", __func__, FindTask(NULL)));
    GLCALL(glBindTransformFeedback, target, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteTransformFeedbacks (GLsizei n, const GLuint * ids)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteTransformFeedbacks", __func__, FindTask(NULL)));
    GLCALL(glDeleteTransformFeedbacks, n, ids);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenTransformFeedbacks (GLsizei n, GLuint * ids)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenTransformFeedbacks", __func__, FindTask(NULL)));
    GLCALL(glGenTransformFeedbacks, n, ids);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsTransformFeedback (GLuint id)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsTransformFeedback", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsTransformFeedback, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glPauseTransformFeedback ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPauseTransformFeedback", __func__, FindTask(NULL)));
    GLCALL(glPauseTransformFeedback);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glResumeTransformFeedback ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glResumeTransformFeedback", __func__, FindTask(NULL)));
    GLCALL(glResumeTransformFeedback);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawTransformFeedback (GLenum mode, GLuint id)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawTransformFeedback", __func__, FindTask(NULL)));
    GLCALL(glDrawTransformFeedback, mode, id);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawArraysInstancedEXT (GLenum mode, GLint start, GLsizei count, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawArraysInstancedEXT", __func__, FindTask(NULL)));
    GLCALL(glDrawArraysInstancedEXT, mode, start, count, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawElementsInstancedEXT (GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei primcount)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawElementsInstancedEXT", __func__, FindTask(NULL)));
    GLCALL(glDrawElementsInstancedEXT, mode, count, type, indices, primcount);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBeginTransformFeedbackEXT (GLenum primitiveMode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBeginTransformFeedbackEXT", __func__, FindTask(NULL)));
    GLCALL(glBeginTransformFeedbackEXT, primitiveMode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEndTransformFeedbackEXT ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEndTransformFeedbackEXT", __func__, FindTask(NULL)));
    GLCALL(glEndTransformFeedbackEXT);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindBufferRangeEXT (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindBufferRangeEXT", __func__, FindTask(NULL)));
    GLCALL(glBindBufferRangeEXT, target, index, buffer, offset, size);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindBufferOffsetEXT (GLenum target, GLuint index, GLuint buffer, GLintptr offset)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindBufferOffsetEXT", __func__, FindTask(NULL)));
    GLCALL(glBindBufferOffsetEXT, target, index, buffer, offset);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindBufferBaseEXT (GLenum target, GLuint index, GLuint buffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindBufferBaseEXT", __func__, FindTask(NULL)));
    GLCALL(glBindBufferBaseEXT, target, index, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTransformFeedbackVaryingsEXT (GLuint program, GLsizei count, const GLchar *  * varyings, GLenum bufferMode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTransformFeedbackVaryingsEXT", __func__, FindTask(NULL)));
    GLCALL(glTransformFeedbackVaryingsEXT, program, count, varyings, bufferMode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTransformFeedbackVaryingEXT (GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTransformFeedbackVaryingEXT", __func__, FindTask(NULL)));
    GLCALL(glGetTransformFeedbackVaryingEXT, program, index, bufSize, length, size, type, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEGLImageTargetTexture2DOES", __func__, FindTask(NULL)));
    GLCALL(glEGLImageTargetTexture2DOES, target, image);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEGLImageTargetRenderbufferStorageOES", __func__, FindTask(NULL)));
    GLCALL(glEGLImageTargetRenderbufferStorageOES, target, image);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorMaski (GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorMaski", __func__, FindTask(NULL)));
    GLCALL(glColorMaski, index, r, g, b, a);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetBooleani_v (GLenum target, GLuint index, GLboolean * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBooleani_v", __func__, FindTask(NULL)));
    GLCALL(glGetBooleani_v, target, index, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetIntegeri_v (GLenum target, GLuint index, GLint * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetIntegeri_v", __func__, FindTask(NULL)));
    GLCALL(glGetIntegeri_v, target, index, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEnablei (GLenum target, GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEnablei", __func__, FindTask(NULL)));
    GLCALL(glEnablei, target, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDisablei (GLenum target, GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDisablei", __func__, FindTask(NULL)));
    GLCALL(glDisablei, target, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsEnabledi (GLenum target, GLuint index)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsEnabledi", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsEnabledi, target, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glClampColor (GLenum target, GLenum clamp)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClampColor", __func__, FindTask(NULL)));
    GLCALL(glClampColor, target, clamp);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBeginConditionalRender (GLuint id, GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBeginConditionalRender", __func__, FindTask(NULL)));
    GLCALL(glBeginConditionalRender, id, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glEndConditionalRender ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glEndConditionalRender", __func__, FindTask(NULL)));
    GLCALL(glEndConditionalRender);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribIPointer (GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribIPointer", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribIPointer, index, size, type, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribIiv (GLuint index, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribIiv", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribIiv, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribIuiv (GLuint index, GLenum pname, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribIuiv", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribIuiv, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI1i (GLuint index, GLint x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI1i", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI1i, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI2i (GLuint index, GLint x, GLint y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI2i", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI2i, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI3i (GLuint index, GLint x, GLint y, GLint z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI3i", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI3i, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4i (GLuint index, GLint x, GLint y, GLint z, GLint w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4i", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4i, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI1ui (GLuint index, GLuint x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI1ui", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI1ui, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI2ui (GLuint index, GLuint x, GLuint y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI2ui", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI2ui, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI3ui (GLuint index, GLuint x, GLuint y, GLuint z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI3ui", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI3ui, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4ui (GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4ui", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4ui, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI1iv (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI1iv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI1iv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI2iv (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI2iv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI2iv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI3iv (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI3iv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI3iv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4iv (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4iv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4iv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI1uiv (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI1uiv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI1uiv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI2uiv (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI2uiv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI2uiv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI3uiv (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI3uiv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI3uiv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4uiv (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4uiv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4uiv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4bv (GLuint index, const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4bv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4bv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4sv (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4sv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4sv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4ubv (GLuint index, const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4ubv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4ubv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4usv (GLuint index, const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4usv", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4usv, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetUniformuiv (GLuint program, GLint location, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetUniformuiv", __func__, FindTask(NULL)));
    GLCALL(glGetUniformuiv, program, location, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindFragDataLocation (GLuint program, GLuint color, const GLchar * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindFragDataLocation", __func__, FindTask(NULL)));
    GLCALL(glBindFragDataLocation, program, color, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLint glGetFragDataLocation (GLuint program, const GLchar * name)
{
    GLint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetFragDataLocation", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetFragDataLocation, program, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glUniform1ui (GLint location, GLuint v0)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1ui", __func__, FindTask(NULL)));
    GLCALL(glUniform1ui, location, v0);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2ui (GLint location, GLuint v0, GLuint v1)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2ui", __func__, FindTask(NULL)));
    GLCALL(glUniform2ui, location, v0, v1);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3ui (GLint location, GLuint v0, GLuint v1, GLuint v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3ui", __func__, FindTask(NULL)));
    GLCALL(glUniform3ui, location, v0, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4ui (GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4ui", __func__, FindTask(NULL)));
    GLCALL(glUniform4ui, location, v0, v1, v2, v3);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1uiv (GLint location, GLsizei count, const GLuint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1uiv", __func__, FindTask(NULL)));
    GLCALL(glUniform1uiv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2uiv (GLint location, GLsizei count, const GLuint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2uiv", __func__, FindTask(NULL)));
    GLCALL(glUniform2uiv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3uiv (GLint location, GLsizei count, const GLuint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3uiv", __func__, FindTask(NULL)));
    GLCALL(glUniform3uiv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4uiv (GLint location, GLsizei count, const GLuint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4uiv", __func__, FindTask(NULL)));
    GLCALL(glUniform4uiv, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexParameterIiv (GLenum target, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexParameterIiv", __func__, FindTask(NULL)));
    GLCALL(glTexParameterIiv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexParameterIuiv (GLenum target, GLenum pname, const GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexParameterIuiv", __func__, FindTask(NULL)));
    GLCALL(glTexParameterIuiv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexParameterIiv (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexParameterIiv", __func__, FindTask(NULL)));
    GLCALL(glGetTexParameterIiv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexParameterIuiv (GLenum target, GLenum pname, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexParameterIuiv", __func__, FindTask(NULL)));
    GLCALL(glGetTexParameterIuiv, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearBufferiv (GLenum buffer, GLint drawbuffer, const GLint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearBufferiv", __func__, FindTask(NULL)));
    GLCALL(glClearBufferiv, buffer, drawbuffer, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearBufferuiv (GLenum buffer, GLint drawbuffer, const GLuint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearBufferuiv", __func__, FindTask(NULL)));
    GLCALL(glClearBufferuiv, buffer, drawbuffer, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearBufferfv (GLenum buffer, GLint drawbuffer, const GLfloat * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearBufferfv", __func__, FindTask(NULL)));
    GLCALL(glClearBufferfv, buffer, drawbuffer, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearBufferfi (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearBufferfi", __func__, FindTask(NULL)));
    GLCALL(glClearBufferfi, buffer, drawbuffer, depth, stencil);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

const GLubyte * glGetStringi (GLenum name, GLuint index)
{
    const GLubyte * _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetStringi", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetStringi, name, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glTexBuffer (GLenum target, GLenum internalformat, GLuint buffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexBuffer", __func__, FindTask(NULL)));
    GLCALL(glTexBuffer, target, internalformat, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPrimitiveRestartIndex (GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPrimitiveRestartIndex", __func__, FindTask(NULL)));
    GLCALL(glPrimitiveRestartIndex, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetInteger64i_v (GLenum target, GLuint index, GLint64 * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetInteger64i_v", __func__, FindTask(NULL)));
    GLCALL(glGetInteger64i_v, target, index, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetBufferParameteri64v (GLenum target, GLenum pname, GLint64 * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetBufferParameteri64v", __func__, FindTask(NULL)));
    GLCALL(glGetBufferParameteri64v, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTexture (GLenum target, GLenum attachment, GLuint texture, GLint level)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTexture", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTexture, target, attachment, texture, level);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribDivisor (GLuint index, GLuint divisor)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribDivisor", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribDivisor, index, divisor);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPrimitiveRestartNV ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPrimitiveRestartNV", __func__, FindTask(NULL)));
    GLCALL(glPrimitiveRestartNV);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPrimitiveRestartIndexNV (GLuint index)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPrimitiveRestartIndexNV", __func__, FindTask(NULL)));
    GLCALL(glPrimitiveRestartIndexNV, index);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI1iEXT (GLuint index, GLint x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI1iEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI1iEXT, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI2iEXT (GLuint index, GLint x, GLint y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI2iEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI2iEXT, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI3iEXT (GLuint index, GLint x, GLint y, GLint z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI3iEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI3iEXT, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4iEXT (GLuint index, GLint x, GLint y, GLint z, GLint w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4iEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4iEXT, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI1uiEXT (GLuint index, GLuint x)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI1uiEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI1uiEXT, index, x);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI2uiEXT (GLuint index, GLuint x, GLuint y)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI2uiEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI2uiEXT, index, x, y);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI3uiEXT (GLuint index, GLuint x, GLuint y, GLuint z)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI3uiEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI3uiEXT, index, x, y, z);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4uiEXT (GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4uiEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4uiEXT, index, x, y, z, w);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI1ivEXT (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI1ivEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI1ivEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI2ivEXT (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI2ivEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI2ivEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI3ivEXT (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI3ivEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI3ivEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4ivEXT (GLuint index, const GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4ivEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4ivEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI1uivEXT (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI1uivEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI1uivEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI2uivEXT (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI2uivEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI2uivEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI3uivEXT (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI3uivEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI3uivEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4uivEXT (GLuint index, const GLuint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4uivEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4uivEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4bvEXT (GLuint index, const GLbyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4bvEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4bvEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4svEXT (GLuint index, const GLshort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4svEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4svEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4ubvEXT (GLuint index, const GLubyte * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4ubvEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4ubvEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribI4usvEXT (GLuint index, const GLushort * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribI4usvEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribI4usvEXT, index, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribIPointerEXT (GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribIPointerEXT", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribIPointerEXT, index, size, type, stride, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribIivEXT (GLuint index, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribIivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribIivEXT, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetVertexAttribIuivEXT (GLuint index, GLenum pname, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetVertexAttribIuivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetVertexAttribIuivEXT, index, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetUniformuivEXT (GLuint program, GLint location, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetUniformuivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetUniformuivEXT, program, location, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindFragDataLocationEXT (GLuint program, GLuint color, const GLchar * name)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindFragDataLocationEXT", __func__, FindTask(NULL)));
    GLCALL(glBindFragDataLocationEXT, program, color, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLint glGetFragDataLocationEXT (GLuint program, const GLchar * name)
{
    GLint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetFragDataLocationEXT", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetFragDataLocationEXT, program, name);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glUniform1uiEXT (GLint location, GLuint v0)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1uiEXT", __func__, FindTask(NULL)));
    GLCALL(glUniform1uiEXT, location, v0);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2uiEXT (GLint location, GLuint v0, GLuint v1)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2uiEXT", __func__, FindTask(NULL)));
    GLCALL(glUniform2uiEXT, location, v0, v1);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3uiEXT (GLint location, GLuint v0, GLuint v1, GLuint v2)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3uiEXT", __func__, FindTask(NULL)));
    GLCALL(glUniform3uiEXT, location, v0, v1, v2);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4uiEXT (GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4uiEXT", __func__, FindTask(NULL)));
    GLCALL(glUniform4uiEXT, location, v0, v1, v2, v3);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform1uivEXT (GLint location, GLsizei count, const GLuint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform1uivEXT", __func__, FindTask(NULL)));
    GLCALL(glUniform1uivEXT, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform2uivEXT (GLint location, GLsizei count, const GLuint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform2uivEXT", __func__, FindTask(NULL)));
    GLCALL(glUniform2uivEXT, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform3uivEXT (GLint location, GLsizei count, const GLuint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform3uivEXT", __func__, FindTask(NULL)));
    GLCALL(glUniform3uivEXT, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUniform4uivEXT (GLint location, GLsizei count, const GLuint * value)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUniform4uivEXT", __func__, FindTask(NULL)));
    GLCALL(glUniform4uivEXT, location, count, value);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexParameterIivEXT (GLenum target, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexParameterIivEXT", __func__, FindTask(NULL)));
    GLCALL(glTexParameterIivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexParameterIuivEXT (GLenum target, GLenum pname, const GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexParameterIuivEXT", __func__, FindTask(NULL)));
    GLCALL(glTexParameterIuivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexParameterIivEXT (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexParameterIivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetTexParameterIivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexParameterIuivEXT (GLenum target, GLenum pname, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexParameterIuivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetTexParameterIuivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearColorIiEXT (GLint red, GLint green, GLint blue, GLint alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearColorIiEXT", __func__, FindTask(NULL)));
    GLCALL(glClearColorIiEXT, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearColorIuiEXT (GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearColorIuiEXT", __func__, FindTask(NULL)));
    GLCALL(glClearColorIuiEXT, red, green, blue, alpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glUseShaderProgramEXT (GLenum type, GLuint program)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glUseShaderProgramEXT", __func__, FindTask(NULL)));
    GLCALL(glUseShaderProgramEXT, type, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glActiveProgramEXT (GLuint program)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glActiveProgramEXT", __func__, FindTask(NULL)));
    GLCALL(glActiveProgramEXT, program);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLuint glCreateShaderProgramEXT (GLenum type, const GLchar * string)
{
    GLuint _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCreateShaderProgramEXT", __func__, FindTask(NULL)));
    _ret = GLCALL(glCreateShaderProgramEXT, type, string);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glProgramEnvParameters4fvEXT (GLenum target, GLuint index, GLsizei count, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramEnvParameters4fvEXT", __func__, FindTask(NULL)));
    GLCALL(glProgramEnvParameters4fvEXT, target, index, count, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glProgramLocalParameters4fvEXT (GLenum target, GLuint index, GLsizei count, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glProgramLocalParameters4fvEXT", __func__, FindTask(NULL)));
    GLCALL(glProgramLocalParameters4fvEXT, target, index, count, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendEquationSeparateATI (GLenum modeRGB, GLenum modeA)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendEquationSeparateATI", __func__, FindTask(NULL)));
    GLCALL(glBlendEquationSeparateATI, modeRGB, modeA);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetHistogramEXT (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetHistogramEXT", __func__, FindTask(NULL)));
    GLCALL(glGetHistogramEXT, target, reset, format, type, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetHistogramParameterfvEXT (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetHistogramParameterfvEXT", __func__, FindTask(NULL)));
    GLCALL(glGetHistogramParameterfvEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetHistogramParameterivEXT (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetHistogramParameterivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetHistogramParameterivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMinmaxEXT (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMinmaxEXT", __func__, FindTask(NULL)));
    GLCALL(glGetMinmaxEXT, target, reset, format, type, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMinmaxParameterfvEXT (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMinmaxParameterfvEXT", __func__, FindTask(NULL)));
    GLCALL(glGetMinmaxParameterfvEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetMinmaxParameterivEXT (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetMinmaxParameterivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetMinmaxParameterivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glHistogramEXT (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glHistogramEXT", __func__, FindTask(NULL)));
    GLCALL(glHistogramEXT, target, width, internalformat, sink);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMinmaxEXT (GLenum target, GLenum internalformat, GLboolean sink)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMinmaxEXT", __func__, FindTask(NULL)));
    GLCALL(glMinmaxEXT, target, internalformat, sink);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glResetHistogramEXT (GLenum target)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glResetHistogramEXT", __func__, FindTask(NULL)));
    GLCALL(glResetHistogramEXT, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glResetMinmaxEXT (GLenum target)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glResetMinmaxEXT", __func__, FindTask(NULL)));
    GLCALL(glResetMinmaxEXT, target);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionFilter1DEXT (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionFilter1DEXT", __func__, FindTask(NULL)));
    GLCALL(glConvolutionFilter1DEXT, target, internalformat, width, format, type, image);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionFilter2DEXT (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionFilter2DEXT", __func__, FindTask(NULL)));
    GLCALL(glConvolutionFilter2DEXT, target, internalformat, width, height, format, type, image);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionParameterfEXT (GLenum target, GLenum pname, GLfloat params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionParameterfEXT", __func__, FindTask(NULL)));
    GLCALL(glConvolutionParameterfEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionParameterfvEXT (GLenum target, GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionParameterfvEXT", __func__, FindTask(NULL)));
    GLCALL(glConvolutionParameterfvEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionParameteriEXT (GLenum target, GLenum pname, GLint params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionParameteriEXT", __func__, FindTask(NULL)));
    GLCALL(glConvolutionParameteriEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glConvolutionParameterivEXT (GLenum target, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glConvolutionParameterivEXT", __func__, FindTask(NULL)));
    GLCALL(glConvolutionParameterivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyConvolutionFilter1DEXT (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyConvolutionFilter1DEXT", __func__, FindTask(NULL)));
    GLCALL(glCopyConvolutionFilter1DEXT, target, internalformat, x, y, width);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyConvolutionFilter2DEXT (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyConvolutionFilter2DEXT", __func__, FindTask(NULL)));
    GLCALL(glCopyConvolutionFilter2DEXT, target, internalformat, x, y, width, height);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetConvolutionFilterEXT (GLenum target, GLenum format, GLenum type, GLvoid * image)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetConvolutionFilterEXT", __func__, FindTask(NULL)));
    GLCALL(glGetConvolutionFilterEXT, target, format, type, image);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetConvolutionParameterfvEXT (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetConvolutionParameterfvEXT", __func__, FindTask(NULL)));
    GLCALL(glGetConvolutionParameterfvEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetConvolutionParameterivEXT (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetConvolutionParameterivEXT", __func__, FindTask(NULL)));
    GLCALL(glGetConvolutionParameterivEXT, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetSeparableFilterEXT (GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetSeparableFilterEXT", __func__, FindTask(NULL)));
    GLCALL(glGetSeparableFilterEXT, target, format, type, row, column, span);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSeparableFilter2DEXT (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSeparableFilter2DEXT", __func__, FindTask(NULL)));
    GLCALL(glSeparableFilter2DEXT, target, internalformat, width, height, format, type, row, column);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorTableSGI (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorTableSGI", __func__, FindTask(NULL)));
    GLCALL(glColorTableSGI, target, internalformat, width, format, type, table);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorTableParameterfvSGI (GLenum target, GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorTableParameterfvSGI", __func__, FindTask(NULL)));
    GLCALL(glColorTableParameterfvSGI, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorTableParameterivSGI (GLenum target, GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorTableParameterivSGI", __func__, FindTask(NULL)));
    GLCALL(glColorTableParameterivSGI, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyColorTableSGI (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyColorTableSGI", __func__, FindTask(NULL)));
    GLCALL(glCopyColorTableSGI, target, internalformat, x, y, width);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetColorTableSGI (GLenum target, GLenum format, GLenum type, GLvoid * table)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetColorTableSGI", __func__, FindTask(NULL)));
    GLCALL(glGetColorTableSGI, target, format, type, table);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetColorTableParameterfvSGI (GLenum target, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetColorTableParameterfvSGI", __func__, FindTask(NULL)));
    GLCALL(glGetColorTableParameterfvSGI, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetColorTableParameterivSGI (GLenum target, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetColorTableParameterivSGI", __func__, FindTask(NULL)));
    GLCALL(glGetColorTableParameterivSGI, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelTexGenSGIX (GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelTexGenSGIX", __func__, FindTask(NULL)));
    GLCALL(glPixelTexGenSGIX, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelTexGenParameteriSGIS (GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelTexGenParameteriSGIS", __func__, FindTask(NULL)));
    GLCALL(glPixelTexGenParameteriSGIS, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelTexGenParameterivSGIS (GLenum pname, const GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelTexGenParameterivSGIS", __func__, FindTask(NULL)));
    GLCALL(glPixelTexGenParameterivSGIS, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelTexGenParameterfSGIS (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelTexGenParameterfSGIS", __func__, FindTask(NULL)));
    GLCALL(glPixelTexGenParameterfSGIS, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPixelTexGenParameterfvSGIS (GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPixelTexGenParameterfvSGIS", __func__, FindTask(NULL)));
    GLCALL(glPixelTexGenParameterfvSGIS, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetPixelTexGenParameterivSGIS (GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetPixelTexGenParameterivSGIS", __func__, FindTask(NULL)));
    GLCALL(glGetPixelTexGenParameterivSGIS, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetPixelTexGenParameterfvSGIS (GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetPixelTexGenParameterfvSGIS", __func__, FindTask(NULL)));
    GLCALL(glGetPixelTexGenParameterfvSGIS, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSampleMaskSGIS (GLclampf value, GLboolean invert)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSampleMaskSGIS", __func__, FindTask(NULL)));
    GLCALL(glSampleMaskSGIS, value, invert);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSamplePatternSGIS (GLenum pattern)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSamplePatternSGIS", __func__, FindTask(NULL)));
    GLCALL(glSamplePatternSGIS, pattern);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameterfSGIS (GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameterfSGIS", __func__, FindTask(NULL)));
    GLCALL(glPointParameterfSGIS, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glPointParameterfvSGIS (GLenum pname, const GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glPointParameterfvSGIS", __func__, FindTask(NULL)));
    GLCALL(glPointParameterfvSGIS, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glColorSubTableEXT (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glColorSubTableEXT", __func__, FindTask(NULL)));
    GLCALL(glColorSubTableEXT, target, start, count, format, type, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glCopyColorSubTableEXT (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glCopyColorSubTableEXT", __func__, FindTask(NULL)));
    GLCALL(glCopyColorSubTableEXT, target, start, x, y, width);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendFuncSeparateINGR (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendFuncSeparateINGR", __func__, FindTask(NULL)));
    GLCALL(glBlendFuncSeparateINGR, sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiModeDrawArraysIBM (const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiModeDrawArraysIBM", __func__, FindTask(NULL)));
    GLCALL(glMultiModeDrawArraysIBM, mode, first, count, primcount, modestride);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glMultiModeDrawElementsIBM (const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glMultiModeDrawElementsIBM", __func__, FindTask(NULL)));
    GLCALL(glMultiModeDrawElementsIBM, mode, count, type, indices, primcount, modestride);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSampleMaskEXT (GLclampf value, GLboolean invert)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSampleMaskEXT", __func__, FindTask(NULL)));
    GLCALL(glSampleMaskEXT, value, invert);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSamplePatternEXT (GLenum pattern)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSamplePatternEXT", __func__, FindTask(NULL)));
    GLCALL(glSamplePatternEXT, pattern);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteFencesNV (GLsizei n, const GLuint * fences)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteFencesNV", __func__, FindTask(NULL)));
    GLCALL(glDeleteFencesNV, n, fences);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenFencesNV (GLsizei n, GLuint * fences)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenFencesNV", __func__, FindTask(NULL)));
    GLCALL(glGenFencesNV, n, fences);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsFenceNV (GLuint fence)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsFenceNV", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsFenceNV, fence);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

GLboolean glTestFenceNV (GLuint fence)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTestFenceNV", __func__, FindTask(NULL)));
    _ret = GLCALL(glTestFenceNV, fence);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glGetFenceivNV (GLuint fence, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetFenceivNV", __func__, FindTask(NULL)));
    GLCALL(glGetFenceivNV, fence, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFinishFenceNV (GLuint fence)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFinishFenceNV", __func__, FindTask(NULL)));
    GLCALL(glFinishFenceNV, fence);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSetFenceNV (GLuint fence, GLenum condition)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSetFenceNV", __func__, FindTask(NULL)));
    GLCALL(glSetFenceNV, fence, condition);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glActiveStencilFaceEXT (GLenum face)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glActiveStencilFaceEXT", __func__, FindTask(NULL)));
    GLCALL(glActiveStencilFaceEXT, face);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBindVertexArrayAPPLE (GLuint array)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindVertexArrayAPPLE", __func__, FindTask(NULL)));
    GLCALL(glBindVertexArrayAPPLE, array);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteVertexArraysAPPLE (GLsizei n, const GLuint * arrays)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteVertexArraysAPPLE", __func__, FindTask(NULL)));
    GLCALL(glDeleteVertexArraysAPPLE, n, arrays);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenVertexArraysAPPLE (GLsizei n, GLuint * arrays)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenVertexArraysAPPLE", __func__, FindTask(NULL)));
    GLCALL(glGenVertexArraysAPPLE, n, arrays);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsVertexArrayAPPLE (GLuint array)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsVertexArrayAPPLE", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsVertexArrayAPPLE, array);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glStencilOpSeparateATI (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glStencilOpSeparateATI", __func__, FindTask(NULL)));
    GLCALL(glStencilOpSeparateATI, face, sfail, dpfail, dppass);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glStencilFuncSeparateATI (GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glStencilFuncSeparateATI", __func__, FindTask(NULL)));
    GLCALL(glStencilFuncSeparateATI, frontfunc, backfunc, ref, mask);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDepthBoundsEXT (GLclampd zmin, GLclampd zmax)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDepthBoundsEXT", __func__, FindTask(NULL)));
    GLCALL(glDepthBoundsEXT, zmin, zmax);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendEquationSeparateEXT (GLenum modeRGB, GLenum modeAlpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendEquationSeparateEXT", __func__, FindTask(NULL)));
    GLCALL(glBlendEquationSeparateEXT, modeRGB, modeAlpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlitFramebufferEXT (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlitFramebufferEXT", __func__, FindTask(NULL)));
    GLCALL(glBlitFramebufferEXT, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetQueryObjecti64vEXT (GLuint id, GLenum pname, GLint64EXT * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetQueryObjecti64vEXT", __func__, FindTask(NULL)));
    GLCALL(glGetQueryObjecti64vEXT, id, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetQueryObjectui64vEXT (GLuint id, GLenum pname, GLuint64EXT * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetQueryObjectui64vEXT", __func__, FindTask(NULL)));
    GLCALL(glGetQueryObjectui64vEXT, id, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBufferParameteriAPPLE (GLenum target, GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBufferParameteriAPPLE", __func__, FindTask(NULL)));
    GLCALL(glBufferParameteriAPPLE, target, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFlushMappedBufferRangeAPPLE (GLenum target, GLintptr offset, GLsizeiptr size)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFlushMappedBufferRangeAPPLE", __func__, FindTask(NULL)));
    GLCALL(glFlushMappedBufferRangeAPPLE, target, offset, size);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTextureRangeAPPLE (GLenum target, GLsizei length, const GLvoid * pointer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTextureRangeAPPLE", __func__, FindTask(NULL)));
    GLCALL(glTextureRangeAPPLE, target, length, pointer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetTexParameterPointervAPPLE (GLenum target, GLenum pname, GLvoid *  * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetTexParameterPointervAPPLE", __func__, FindTask(NULL)));
    GLCALL(glGetTexParameterPointervAPPLE, target, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClampColorARB (GLenum target, GLenum clamp)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClampColorARB", __func__, FindTask(NULL)));
    GLCALL(glClampColorARB, target, clamp);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glFramebufferTextureLayerARB (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glFramebufferTextureLayerARB", __func__, FindTask(NULL)));
    GLCALL(glFramebufferTextureLayerARB, target, attachment, texture, level, layer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glVertexAttribDivisorARB (GLuint index, GLuint divisor)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glVertexAttribDivisorARB", __func__, FindTask(NULL)));
    GLCALL(glVertexAttribDivisorARB, index, divisor);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTexBufferARB (GLenum target, GLenum internalformat, GLuint buffer)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTexBufferARB", __func__, FindTask(NULL)));
    GLCALL(glTexBufferARB, target, internalformat, buffer);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDrawElementsInstancedBaseVertex (GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei primcount, GLint basevertex)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDrawElementsInstancedBaseVertex", __func__, FindTask(NULL)));
    GLCALL(glDrawElementsInstancedBaseVertex, mode, count, type, indices, primcount, basevertex);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendEquationiARB (GLuint buf, GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendEquationiARB", __func__, FindTask(NULL)));
    GLCALL(glBlendEquationiARB, buf, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendEquationSeparateiARB (GLuint buf, GLenum modeRGB, GLenum modeAlpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendEquationSeparateiARB", __func__, FindTask(NULL)));
    GLCALL(glBlendEquationSeparateiARB, buf, modeRGB, modeAlpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendFunciARB (GLuint buf, GLenum src, GLenum dst)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendFunciARB", __func__, FindTask(NULL)));
    GLCALL(glBlendFunciARB, buf, src, dst);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendFuncSeparateiARB (GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendFuncSeparateiARB", __func__, FindTask(NULL)));
    GLCALL(glBlendFuncSeparateiARB, buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGenSamplers (GLsizei count, GLuint * samplers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGenSamplers", __func__, FindTask(NULL)));
    GLCALL(glGenSamplers, count, samplers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDeleteSamplers (GLsizei count, const GLuint * samplers)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDeleteSamplers", __func__, FindTask(NULL)));
    GLCALL(glDeleteSamplers, count, samplers);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLboolean glIsSampler (GLuint sampler)
{
    GLboolean _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glIsSampler", __func__, FindTask(NULL)));
    _ret = GLCALL(glIsSampler, sampler);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glBindSampler (GLuint unit, GLuint sampler)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBindSampler", __func__, FindTask(NULL)));
    GLCALL(glBindSampler, unit, sampler);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSamplerParameteri (GLuint sampler, GLenum pname, GLint param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSamplerParameteri", __func__, FindTask(NULL)));
    GLCALL(glSamplerParameteri, sampler, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSamplerParameteriv (GLuint sampler, GLenum pname, const GLint * param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSamplerParameteriv", __func__, FindTask(NULL)));
    GLCALL(glSamplerParameteriv, sampler, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSamplerParameterf (GLuint sampler, GLenum pname, GLfloat param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSamplerParameterf", __func__, FindTask(NULL)));
    GLCALL(glSamplerParameterf, sampler, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSamplerParameterfv (GLuint sampler, GLenum pname, const GLfloat * param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSamplerParameterfv", __func__, FindTask(NULL)));
    GLCALL(glSamplerParameterfv, sampler, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSamplerParameterIiv (GLuint sampler, GLenum pname, const GLint * param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSamplerParameterIiv", __func__, FindTask(NULL)));
    GLCALL(glSamplerParameterIiv, sampler, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glSamplerParameterIuiv (GLuint sampler, GLenum pname, const GLuint * param)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glSamplerParameterIuiv", __func__, FindTask(NULL)));
    GLCALL(glSamplerParameterIuiv, sampler, pname, param);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetSamplerParameteriv (GLuint sampler, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetSamplerParameteriv", __func__, FindTask(NULL)));
    GLCALL(glGetSamplerParameteriv, sampler, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetSamplerParameterIiv (GLuint sampler, GLenum pname, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetSamplerParameterIiv", __func__, FindTask(NULL)));
    GLCALL(glGetSamplerParameterIiv, sampler, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetSamplerParameterfv (GLuint sampler, GLenum pname, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetSamplerParameterfv", __func__, FindTask(NULL)));
    GLCALL(glGetSamplerParameterfv, sampler, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetSamplerParameterIuiv (GLuint sampler, GLenum pname, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetSamplerParameterIuiv", __func__, FindTask(NULL)));
    GLCALL(glGetSamplerParameterIuiv, sampler, pname, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glReleaseShaderCompiler ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glReleaseShaderCompiler", __func__, FindTask(NULL)));
    GLCALL(glReleaseShaderCompiler);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glShaderBinary (GLsizei count, const GLuint * shaders, GLenum binaryformat, const GLvoid * binary, GLsizei length)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glShaderBinary", __func__, FindTask(NULL)));
    GLCALL(glShaderBinary, count, shaders, binaryformat, binary, length);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetShaderPrecisionFormat (GLenum shadertype, GLenum precisiontype, GLint * range, GLint * precision)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetShaderPrecisionFormat", __func__, FindTask(NULL)));
    GLCALL(glGetShaderPrecisionFormat, shadertype, precisiontype, range, precision);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glDepthRangef (GLclampf n, GLclampf f)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glDepthRangef", __func__, FindTask(NULL)));
    GLCALL(glDepthRangef, n, f);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glClearDepthf (GLclampf d)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glClearDepthf", __func__, FindTask(NULL)));
    GLCALL(glClearDepthf, d);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

GLenum glGetGraphicsResetStatusARB ()
{
    GLenum _ret;
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetGraphicsResetStatusARB", __func__, FindTask(NULL)));
    _ret = GLCALL(glGetGraphicsResetStatusARB);
    D(bug(" ->returned\n"));
    HOSTGL_POST
    return _ret;
}

void glGetnMapdvARB (GLenum target, GLenum query, GLsizei bufSize, GLdouble * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnMapdvARB", __func__, FindTask(NULL)));
    GLCALL(glGetnMapdvARB, target, query, bufSize, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnMapfvARB (GLenum target, GLenum query, GLsizei bufSize, GLfloat * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnMapfvARB", __func__, FindTask(NULL)));
    GLCALL(glGetnMapfvARB, target, query, bufSize, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnMapivARB (GLenum target, GLenum query, GLsizei bufSize, GLint * v)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnMapivARB", __func__, FindTask(NULL)));
    GLCALL(glGetnMapivARB, target, query, bufSize, v);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnPixelMapfvARB (GLenum map, GLsizei bufSize, GLfloat * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnPixelMapfvARB", __func__, FindTask(NULL)));
    GLCALL(glGetnPixelMapfvARB, map, bufSize, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnPixelMapuivARB (GLenum map, GLsizei bufSize, GLuint * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnPixelMapuivARB", __func__, FindTask(NULL)));
    GLCALL(glGetnPixelMapuivARB, map, bufSize, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnPixelMapusvARB (GLenum map, GLsizei bufSize, GLushort * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnPixelMapusvARB", __func__, FindTask(NULL)));
    GLCALL(glGetnPixelMapusvARB, map, bufSize, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnPolygonStippleARB (GLsizei bufSize, GLubyte * pattern)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnPolygonStippleARB", __func__, FindTask(NULL)));
    GLCALL(glGetnPolygonStippleARB, bufSize, pattern);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnColorTableARB (GLenum target, GLenum format, GLenum type, GLsizei bufSize, GLvoid * table)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnColorTableARB", __func__, FindTask(NULL)));
    GLCALL(glGetnColorTableARB, target, format, type, bufSize, table);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnConvolutionFilterARB (GLenum target, GLenum format, GLenum type, GLsizei bufSize, GLvoid * image)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnConvolutionFilterARB", __func__, FindTask(NULL)));
    GLCALL(glGetnConvolutionFilterARB, target, format, type, bufSize, image);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnSeparableFilterARB (GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, GLvoid * row, GLsizei columnBufSize, GLvoid * column, GLvoid * span)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnSeparableFilterARB", __func__, FindTask(NULL)));
    GLCALL(glGetnSeparableFilterARB, target, format, type, rowBufSize, row, columnBufSize, column, span);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnHistogramARB (GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, GLvoid * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnHistogramARB", __func__, FindTask(NULL)));
    GLCALL(glGetnHistogramARB, target, reset, format, type, bufSize, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnMinmaxARB (GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, GLvoid * values)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnMinmaxARB", __func__, FindTask(NULL)));
    GLCALL(glGetnMinmaxARB, target, reset, format, type, bufSize, values);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnTexImageARB (GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, GLvoid * img)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnTexImageARB", __func__, FindTask(NULL)));
    GLCALL(glGetnTexImageARB, target, level, format, type, bufSize, img);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glReadnPixelsARB (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, GLvoid * data)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glReadnPixelsARB", __func__, FindTask(NULL)));
    GLCALL(glReadnPixelsARB, x, y, width, height, format, type, bufSize, data);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnCompressedTexImageARB (GLenum target, GLint lod, GLsizei bufSize, GLvoid * img)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnCompressedTexImageARB", __func__, FindTask(NULL)));
    GLCALL(glGetnCompressedTexImageARB, target, lod, bufSize, img);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnUniformfvARB (GLuint program, GLint location, GLsizei bufSize, GLfloat * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnUniformfvARB", __func__, FindTask(NULL)));
    GLCALL(glGetnUniformfvARB, program, location, bufSize, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnUniformivARB (GLuint program, GLint location, GLsizei bufSize, GLint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnUniformivARB", __func__, FindTask(NULL)));
    GLCALL(glGetnUniformivARB, program, location, bufSize, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnUniformuivARB (GLuint program, GLint location, GLsizei bufSize, GLuint * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnUniformuivARB", __func__, FindTask(NULL)));
    GLCALL(glGetnUniformuivARB, program, location, bufSize, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glGetnUniformdvARB (GLuint program, GLint location, GLsizei bufSize, GLdouble * params)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glGetnUniformdvARB", __func__, FindTask(NULL)));
    GLCALL(glGetnUniformdvARB, program, location, bufSize, params);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendFuncIndexedAMD (GLuint buf, GLenum src, GLenum dst)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendFuncIndexedAMD", __func__, FindTask(NULL)));
    GLCALL(glBlendFuncIndexedAMD, buf, src, dst);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendFuncSeparateIndexedAMD (GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendFuncSeparateIndexedAMD", __func__, FindTask(NULL)));
    GLCALL(glBlendFuncSeparateIndexedAMD, buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendEquationIndexedAMD (GLuint buf, GLenum mode)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendEquationIndexedAMD", __func__, FindTask(NULL)));
    GLCALL(glBlendEquationIndexedAMD, buf, mode);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glBlendEquationSeparateIndexedAMD (GLuint buf, GLenum modeRGB, GLenum modeAlpha)
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glBlendEquationSeparateIndexedAMD", __func__, FindTask(NULL)));
    GLCALL(glBlendEquationSeparateIndexedAMD, buf, modeRGB, modeAlpha);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

void glTextureBarrierNV ()
{
    HOSTGL_PRE
    D(bug("[HostGL] %s: TASK: 0x%x, ->glTextureBarrierNV", __func__, FindTask(NULL)));
    GLCALL(glTextureBarrierNV);
    D(bug(" ->returned\n"));
    HOSTGL_POST
}

