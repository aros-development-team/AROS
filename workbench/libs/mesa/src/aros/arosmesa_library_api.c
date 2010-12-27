/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define GL_GLEXT_PROTOTYPES

#include "GL/gl.h"

    /* 
    Now we have prototypes of mglXXX functions, but
    the defines from gl_mangle.h are still active so
    they will redefine our glXXX functions and we will
    get multiple declaractions. Undef the evil defines
    */

#include "mangle_undef.h"

#include "arosmesa_intern.h"

#include "glapi/glapi.h"

AROS_LH1(void, glClearIndex,
    AROS_LHA(GLfloat, c, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglClearIndex(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glClearColor,
    AROS_LHA(GLclampf, red, D0),
    AROS_LHA(GLclampf, green, D1),
    AROS_LHA(GLclampf, blue, D2),
    AROS_LHA(GLclampf, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglClearColor(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClear,
    AROS_LHA(GLbitfield, mask, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglClear(mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexMask,
    AROS_LHA(GLuint, mask, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexMask(mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColorMask,
    AROS_LHA(GLboolean, red, D0),
    AROS_LHA(GLboolean, green, D1),
    AROS_LHA(GLboolean, blue, D2),
    AROS_LHA(GLboolean, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorMask(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAlphaFunc,
    AROS_LHA(GLenum, func, D0),
    AROS_LHA(GLclampf, ref, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglAlphaFunc(func, ref);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendFunc,
    AROS_LHA(GLenum, sfactor, D0),
    AROS_LHA(GLenum, dfactor, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBlendFunc(sfactor, dfactor);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLogicOp,
    AROS_LHA(GLenum, opcode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLogicOp(opcode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCullFace,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCullFace(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFrontFace,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFrontFace(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPointSize,
    AROS_LHA(GLfloat, size, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointSize(size);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLineWidth,
    AROS_LHA(GLfloat, width, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLineWidth(width);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLineStipple,
    AROS_LHA(GLint, factor, D0),
    AROS_LHA(GLushort, pattern, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLineStipple(factor, pattern);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPolygonMode,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPolygonMode(face, mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPolygonOffset,
    AROS_LHA(GLfloat, factor, D0),
    AROS_LHA(GLfloat, units, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPolygonOffset(factor, units);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPolygonStipple,
    AROS_LHA(const GLubyte *, mask, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPolygonStipple(mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glGetPolygonStipple,
    AROS_LHA(GLubyte *, mask, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetPolygonStipple(mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEdgeFlag,
    AROS_LHA(GLboolean, flag, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEdgeFlag(flag);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEdgeFlagv,
    AROS_LHA(const GLboolean *, flag, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEdgeFlagv(flag);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glScissor,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglScissor(x, y, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glClipPlane,
    AROS_LHA(GLenum, plane, D0),
    AROS_LHA(const GLdouble *, equation, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglClipPlane(plane, equation);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetClipPlane,
    AROS_LHA(GLenum, plane, D0),
    AROS_LHA(GLdouble *, equation, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetClipPlane(plane, equation);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDrawBuffer,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawBuffer(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glReadBuffer,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglReadBuffer(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnable,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEnable(cap);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisable,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDisable(cap);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsEnabled,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsEnabled(cap);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnableClientState,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEnableClientState(cap);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisableClientState,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDisableClientState(cap);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetBooleanv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLboolean *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetBooleanv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetDoublev,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetDoublev(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetFloatv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetFloatv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetIntegerv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetIntegerv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPushAttrib,
    AROS_LHA(GLbitfield, mask, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPushAttrib(mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopAttrib,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPopAttrib();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPushClientAttrib,
    AROS_LHA(GLbitfield, mask, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPushClientAttrib(mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopClientAttrib,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPopClientAttrib();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLint, glRenderMode,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLint _return = mglRenderMode(mode);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLenum, glGetError,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLenum _return = mglGetError();

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(const GLubyte *, glGetString,
    AROS_LHA(GLenum, name, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    const GLubyte * _return = mglGetString(name);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glFinish,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFinish();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glFlush,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFlush();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glHint,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglHint(target, mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClearDepth,
    AROS_LHA(GLclampd, depth, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglClearDepth(depth);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDepthFunc,
    AROS_LHA(GLenum, func, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDepthFunc(func);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDepthMask,
    AROS_LHA(GLboolean, flag, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDepthMask(flag);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDepthRange,
    AROS_LHA(GLclampd, near_val, D0),
    AROS_LHA(GLclampd, far_val, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDepthRange(near_val, far_val);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glClearAccum,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    AROS_LHA(GLfloat, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglClearAccum(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAccum,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLfloat, value, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglAccum(op, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMatrixMode,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMatrixMode(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glOrtho,
    AROS_LHA(GLdouble, left, D0),
    AROS_LHA(GLdouble, right, D1),
    AROS_LHA(GLdouble, bottom, D2),
    AROS_LHA(GLdouble, top, D3),
    AROS_LHA(GLdouble, near_val, D4),
    AROS_LHA(GLdouble, far_val, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglOrtho(left, right, bottom, top, near_val, far_val);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glFrustum,
    AROS_LHA(GLdouble, left, D0),
    AROS_LHA(GLdouble, right, D1),
    AROS_LHA(GLdouble, bottom, D2),
    AROS_LHA(GLdouble, top, D3),
    AROS_LHA(GLdouble, near_val, D4),
    AROS_LHA(GLdouble, far_val, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFrustum(left, right, bottom, top, near_val, far_val);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glViewport,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglViewport(x, y, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPushMatrix,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPushMatrix();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopMatrix,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPopMatrix();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glLoadIdentity,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLoadIdentity();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLoadMatrixd(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLoadMatrixf(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultMatrixd(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultMatrixf(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRotated,
    AROS_LHA(GLdouble, angle, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRotated(angle, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRotatef,
    AROS_LHA(GLfloat, angle, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRotatef(angle, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glScaled,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglScaled(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glScalef,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglScalef(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTranslated,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTranslated(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTranslatef,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTranslatef(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsList,
    AROS_LHA(GLuint, list, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsList(list);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteLists,
    AROS_LHA(GLuint, list, D0),
    AROS_LHA(GLsizei, range, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteLists(list, range);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLuint, glGenLists,
    AROS_LHA(GLsizei, range, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLuint _return = mglGenLists(range);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glNewList,
    AROS_LHA(GLuint, list, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNewList(list, mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndList,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEndList();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCallList,
    AROS_LHA(GLuint, list, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCallList(list);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glCallLists,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(const GLvoid *, lists, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCallLists(n, type, lists);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glListBase,
    AROS_LHA(GLuint, base, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglListBase(base);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBegin,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBegin(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEnd,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEnd();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex2d(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex2f(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex2i(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex2s(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex3d(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex3f(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex3i(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex3s(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertex4d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    AROS_LHA(GLdouble, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex4d(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertex4f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    AROS_LHA(GLfloat, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex4f(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertex4i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    AROS_LHA(GLint, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex4i(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertex4s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    AROS_LHA(GLshort, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex4s(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex2dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex2fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex2iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex2sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex3dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex3fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex3iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex3sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex4dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex4fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex4iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertex4sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3b,
    AROS_LHA(GLbyte, nx, D0),
    AROS_LHA(GLbyte, ny, D1),
    AROS_LHA(GLbyte, nz, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3b(nx, ny, nz);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3d,
    AROS_LHA(GLdouble, nx, D0),
    AROS_LHA(GLdouble, ny, D1),
    AROS_LHA(GLdouble, nz, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3d(nx, ny, nz);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3f,
    AROS_LHA(GLfloat, nx, D0),
    AROS_LHA(GLfloat, ny, D1),
    AROS_LHA(GLfloat, nz, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3f(nx, ny, nz);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3i,
    AROS_LHA(GLint, nx, D0),
    AROS_LHA(GLint, ny, D1),
    AROS_LHA(GLint, nz, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3i(nx, ny, nz);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3s,
    AROS_LHA(GLshort, nx, D0),
    AROS_LHA(GLshort, ny, D1),
    AROS_LHA(GLshort, nz, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3s(nx, ny, nz);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3bv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormal3sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexd,
    AROS_LHA(GLdouble, c, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexd(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexf,
    AROS_LHA(GLfloat, c, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexf(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexi,
    AROS_LHA(GLint, c, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexi(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexs,
    AROS_LHA(GLshort, c, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexs(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexub,
    AROS_LHA(GLubyte, c, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexub(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexdv,
    AROS_LHA(const GLdouble *, c, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexdv(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexfv,
    AROS_LHA(const GLfloat *, c, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexfv(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexiv,
    AROS_LHA(const GLint *, c, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexiv(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexsv,
    AROS_LHA(const GLshort *, c, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexsv(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexubv,
    AROS_LHA(const GLubyte *, c, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexubv(c);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3b,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3b(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3d,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3d(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3f,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3f(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3i,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3i(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3s,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3s(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3ub,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3ub(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3ui,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3ui(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3us,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3us(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4b,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    AROS_LHA(GLbyte, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4b(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4d,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    AROS_LHA(GLdouble, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4d(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4f,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    AROS_LHA(GLfloat, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4f(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4i,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    AROS_LHA(GLint, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4i(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4s,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    AROS_LHA(GLshort, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4s(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4ub,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    AROS_LHA(GLubyte, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4ub(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4ui,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    AROS_LHA(GLuint, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4ui(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4us,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    AROS_LHA(GLushort, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4us(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3bv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3ubv,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3ubv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3uiv,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3uiv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3usv,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor3usv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4bv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4ubv,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4ubv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4uiv,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4uiv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4usv,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColor4usv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1d,
    AROS_LHA(GLdouble, s, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord1d(s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1f,
    AROS_LHA(GLfloat, s, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord1f(s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1i,
    AROS_LHA(GLint, s, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord1i(s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1s,
    AROS_LHA(GLshort, s, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord1s(s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2d,
    AROS_LHA(GLdouble, s, D0),
    AROS_LHA(GLdouble, t, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord2d(s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2f,
    AROS_LHA(GLfloat, s, D0),
    AROS_LHA(GLfloat, t, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord2f(s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2i,
    AROS_LHA(GLint, s, D0),
    AROS_LHA(GLint, t, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord2i(s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2s,
    AROS_LHA(GLshort, s, D0),
    AROS_LHA(GLshort, t, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord2s(s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3d,
    AROS_LHA(GLdouble, s, D0),
    AROS_LHA(GLdouble, t, D1),
    AROS_LHA(GLdouble, r, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord3d(s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3f,
    AROS_LHA(GLfloat, s, D0),
    AROS_LHA(GLfloat, t, D1),
    AROS_LHA(GLfloat, r, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord3f(s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3i,
    AROS_LHA(GLint, s, D0),
    AROS_LHA(GLint, t, D1),
    AROS_LHA(GLint, r, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord3i(s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3s,
    AROS_LHA(GLshort, s, D0),
    AROS_LHA(GLshort, t, D1),
    AROS_LHA(GLshort, r, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord3s(s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoord4d,
    AROS_LHA(GLdouble, s, D0),
    AROS_LHA(GLdouble, t, D1),
    AROS_LHA(GLdouble, r, D2),
    AROS_LHA(GLdouble, q, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord4d(s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoord4f,
    AROS_LHA(GLfloat, s, D0),
    AROS_LHA(GLfloat, t, D1),
    AROS_LHA(GLfloat, r, D2),
    AROS_LHA(GLfloat, q, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord4f(s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoord4i,
    AROS_LHA(GLint, s, D0),
    AROS_LHA(GLint, t, D1),
    AROS_LHA(GLint, r, D2),
    AROS_LHA(GLint, q, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord4i(s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoord4s,
    AROS_LHA(GLshort, s, D0),
    AROS_LHA(GLshort, t, D1),
    AROS_LHA(GLshort, r, D2),
    AROS_LHA(GLshort, q, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord4s(s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord1dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord1fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord1iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord1sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord2dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord2fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord2iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord2sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord3dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord3fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord3iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord3sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord4dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord4fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord4iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoord4sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos2d(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos2f(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos2i(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos2s(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos3d(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos3f(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos3i(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos3s(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRasterPos4d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    AROS_LHA(GLdouble, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos4d(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRasterPos4f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    AROS_LHA(GLfloat, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos4f(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRasterPos4i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    AROS_LHA(GLint, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos4i(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRasterPos4s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    AROS_LHA(GLshort, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos4s(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos2dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos2fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos2iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos2sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos3dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos3fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos3iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos3sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos4dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos4fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos4iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRasterPos4sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRectd,
    AROS_LHA(GLdouble, x1, D0),
    AROS_LHA(GLdouble, y1, D1),
    AROS_LHA(GLdouble, x2, D2),
    AROS_LHA(GLdouble, y2, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRectd(x1, y1, x2, y2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRectf,
    AROS_LHA(GLfloat, x1, D0),
    AROS_LHA(GLfloat, y1, D1),
    AROS_LHA(GLfloat, x2, D2),
    AROS_LHA(GLfloat, y2, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRectf(x1, y1, x2, y2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRecti,
    AROS_LHA(GLint, x1, D0),
    AROS_LHA(GLint, y1, D1),
    AROS_LHA(GLint, x2, D2),
    AROS_LHA(GLint, y2, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRecti(x1, y1, x2, y2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRects,
    AROS_LHA(GLshort, x1, D0),
    AROS_LHA(GLshort, y1, D1),
    AROS_LHA(GLshort, x2, D2),
    AROS_LHA(GLshort, y2, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRects(x1, y1, x2, y2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectdv,
    AROS_LHA(const GLdouble *, v1, A0),
    AROS_LHA(const GLdouble *, v2, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRectdv(v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectfv,
    AROS_LHA(const GLfloat *, v1, A0),
    AROS_LHA(const GLfloat *, v2, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRectfv(v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectiv,
    AROS_LHA(const GLint *, v1, A0),
    AROS_LHA(const GLint *, v2, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRectiv(v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectsv,
    AROS_LHA(const GLshort *, v1, A0),
    AROS_LHA(const GLshort *, v2, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRectsv(v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexPointer,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexPointer(size, type, stride, ptr);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormalPointer,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormalPointer(type, stride, ptr);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColorPointer,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorPointer(size, type, stride, ptr);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glIndexPointer,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexPointer(type, stride, ptr);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoordPointer,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoordPointer(size, type, stride, ptr);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEdgeFlagPointer,
    AROS_LHA(GLsizei, stride, D0),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEdgeFlagPointer(stride, ptr);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPointerv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLvoid * *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetPointerv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glArrayElement,
    AROS_LHA(GLint, i, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglArrayElement(i);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glDrawArrays,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawArrays(mode, first, count);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glDrawElements,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawElements(mode, count, type, indices);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glInterleavedArrays,
    AROS_LHA(GLenum, format, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglInterleavedArrays(format, stride, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glShadeModel,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglShadeModel(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLightf,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLightf(light, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLighti,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLighti(light, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLightfv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLightfv(light, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLightiv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLightiv(light, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetLightfv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetLightfv(light, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetLightiv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetLightiv(light, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModelf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLightModelf(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModeli,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLightModeli(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModelfv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLightModelfv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModeliv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLightModeliv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMaterialf,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMaterialf(face, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMateriali,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMateriali(face, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMaterialfv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMaterialfv(face, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMaterialiv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMaterialiv(face, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMaterialfv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetMaterialfv(face, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMaterialiv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetMaterialiv(face, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glColorMaterial,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorMaterial(face, mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelZoom,
    AROS_LHA(GLfloat, xfactor, D0),
    AROS_LHA(GLfloat, yfactor, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPixelZoom(xfactor, yfactor);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelStoref,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPixelStoref(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelStorei,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPixelStorei(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTransferf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPixelTransferf(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTransferi,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPixelTransferi(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPixelMapfv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, mapsize, D1),
    AROS_LHA(const GLfloat *, values, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPixelMapfv(map, mapsize, values);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPixelMapuiv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, mapsize, D1),
    AROS_LHA(const GLuint *, values, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPixelMapuiv(map, mapsize, values);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPixelMapusv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, mapsize, D1),
    AROS_LHA(const GLushort *, values, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPixelMapusv(map, mapsize, values);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelMapfv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLfloat *, values, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetPixelMapfv(map, values);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelMapuiv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLuint *, values, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetPixelMapuiv(map, values);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelMapusv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLushort *, values, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetPixelMapusv(map, values);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glBitmap,
    AROS_LHA(GLsizei, width, D0),
    AROS_LHA(GLsizei, height, D1),
    AROS_LHA(GLfloat, xorig, D2),
    AROS_LHA(GLfloat, yorig, D3),
    AROS_LHA(GLfloat, xmove, D4),
    AROS_LHA(GLfloat, ymove, D5),
    AROS_LHA(const GLubyte *, bitmap, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glReadPixels,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(GLvoid *, pixels, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglReadPixels(x, y, width, height, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawPixels,
    AROS_LHA(GLsizei, width, D0),
    AROS_LHA(GLsizei, height, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(const GLvoid *, pixels, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawPixels(width, height, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyPixels,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, type, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyPixels(x, y, width, height, type);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glStencilFunc,
    AROS_LHA(GLenum, func, D0),
    AROS_LHA(GLint, ref, D1),
    AROS_LHA(GLuint, mask, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglStencilFunc(func, ref, mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glStencilMask,
    AROS_LHA(GLuint, mask, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglStencilMask(mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glStencilOp,
    AROS_LHA(GLenum, fail, D0),
    AROS_LHA(GLenum, zfail, D1),
    AROS_LHA(GLenum, zpass, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglStencilOp(fail, zfail, zpass);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClearStencil,
    AROS_LHA(GLint, s, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglClearStencil(s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGend,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexGend(coord, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGenf,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexGenf(coord, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGeni,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexGeni(coord, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGendv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexGendv(coord, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGenfv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexGenfv(coord, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGeniv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexGeniv(coord, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexGendv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexGendv(coord, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexGenfv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexGenfv(coord, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexGeniv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexGeniv(coord, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnvf,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexEnvf(target, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnvi,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexEnvi(target, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnvfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexEnvfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnviv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexEnviv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexEnvfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexEnvfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexEnviv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexEnviv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterf,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexParameterf(target, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameteri,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexParameteri(target, pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexParameterfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexParameterfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetTexLevelParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexLevelParameterfv(target, level, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetTexLevelParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexLevelParameteriv(target, level, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glTexImage1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, internalFormat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLint, border, D4),
    AROS_LHA(GLenum, format, D5),
    AROS_LHA(GLenum, type, D6),
    AROS_LHA(const GLvoid *, pixels, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexImage1D(target, level, internalFormat, width, border, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glTexImage2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, internalFormat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    AROS_LHA(GLint, border, D5),
    AROS_LHA(GLenum, format, D6),
    AROS_LHA(GLenum, type, D7),
    AROS_LHA(const GLvoid *, pixels, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetTexImage,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(GLvoid *, pixels, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexImage(target, level, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenTextures,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, textures, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenTextures(n, textures);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteTextures,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteTextures(n, textures);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindTexture,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, texture, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindTexture(target, texture);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPrioritizeTextures,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(const GLclampf *, priorities, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPrioritizeTextures(n, textures, priorities);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLboolean, glAreTexturesResident,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(GLboolean *, residences, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglAreTexturesResident(n, textures, residences);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsTexture,
    AROS_LHA(GLuint, texture, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsTexture(texture);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glTexSubImage1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(const GLvoid *, pixels, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexSubImage1D(target, level, xoffset, width, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glTexSubImage2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLsizei, width, D4),
    AROS_LHA(GLsizei, height, D5),
    AROS_LHA(GLenum, format, D6),
    AROS_LHA(GLenum, type, D7),
    AROS_LHA(const GLvoid *, pixels, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glCopyTexImage1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLint, x, D3),
    AROS_LHA(GLint, y, D4),
    AROS_LHA(GLsizei, width, D5),
    AROS_LHA(GLint, border, D6),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexImage1D(target, level, internalformat, x, y, width, border);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glCopyTexImage2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLint, x, D3),
    AROS_LHA(GLint, y, D4),
    AROS_LHA(GLsizei, width, D5),
    AROS_LHA(GLsizei, height, D6),
    AROS_LHA(GLint, border, D7),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexImage2D(target, level, internalformat, x, y, width, height, border);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glCopyTexSubImage1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, x, D3),
    AROS_LHA(GLint, y, D4),
    AROS_LHA(GLsizei, width, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexSubImage1D(target, level, xoffset, x, y, width);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glCopyTexSubImage2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLint, x, D4),
    AROS_LHA(GLint, y, D5),
    AROS_LHA(GLsizei, width, D6),
    AROS_LHA(GLsizei, height, D7),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMap1d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, u1, D1),
    AROS_LHA(GLdouble, u2, D2),
    AROS_LHA(GLint, stride, D3),
    AROS_LHA(GLint, order, D4),
    AROS_LHA(const GLdouble *, points, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMap1d(target, u1, u2, stride, order, points);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMap1f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, u1, D1),
    AROS_LHA(GLfloat, u2, D2),
    AROS_LHA(GLint, stride, D3),
    AROS_LHA(GLint, order, D4),
    AROS_LHA(const GLfloat *, points, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMap1f(target, u1, u2, stride, order, points);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH10(void, glMap2d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, u1, D1),
    AROS_LHA(GLdouble, u2, D2),
    AROS_LHA(GLint, ustride, D3),
    AROS_LHA(GLint, uorder, D4),
    AROS_LHA(GLdouble, v1, D5),
    AROS_LHA(GLdouble, v2, D6),
    AROS_LHA(GLint, vstride, D7),
    AROS_LHA(GLint, vorder, A0),
    AROS_LHA(const GLdouble *, points, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH10(void, glMap2f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, u1, D1),
    AROS_LHA(GLfloat, u2, D2),
    AROS_LHA(GLint, ustride, D3),
    AROS_LHA(GLint, uorder, D4),
    AROS_LHA(GLfloat, v1, D5),
    AROS_LHA(GLfloat, v2, D6),
    AROS_LHA(GLint, vstride, D7),
    AROS_LHA(GLint, vorder, A0),
    AROS_LHA(const GLfloat *, points, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMapdv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetMapdv(target, query, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMapfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetMapfv(target, query, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMapiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetMapiv(target, query, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1d,
    AROS_LHA(GLdouble, u, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalCoord1d(u);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1f,
    AROS_LHA(GLfloat, u, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalCoord1f(u);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1dv,
    AROS_LHA(const GLdouble *, u, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalCoord1dv(u);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1fv,
    AROS_LHA(const GLfloat *, u, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalCoord1fv(u);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEvalCoord2d,
    AROS_LHA(GLdouble, u, D0),
    AROS_LHA(GLdouble, v, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalCoord2d(u, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEvalCoord2f,
    AROS_LHA(GLfloat, u, D0),
    AROS_LHA(GLfloat, v, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalCoord2f(u, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord2dv,
    AROS_LHA(const GLdouble *, u, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalCoord2dv(u);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord2fv,
    AROS_LHA(const GLfloat *, u, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalCoord2fv(u);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMapGrid1d,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLdouble, u1, D1),
    AROS_LHA(GLdouble, u2, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMapGrid1d(un, u1, u2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMapGrid1f,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLfloat, u1, D1),
    AROS_LHA(GLfloat, u2, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMapGrid1f(un, u1, u2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMapGrid2d,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLdouble, u1, D1),
    AROS_LHA(GLdouble, u2, D2),
    AROS_LHA(GLint, vn, D3),
    AROS_LHA(GLdouble, v1, D4),
    AROS_LHA(GLdouble, v2, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMapGrid2d(un, u1, u2, vn, v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMapGrid2f,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLfloat, u1, D1),
    AROS_LHA(GLfloat, u2, D2),
    AROS_LHA(GLint, vn, D3),
    AROS_LHA(GLfloat, v1, D4),
    AROS_LHA(GLfloat, v2, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMapGrid2f(un, u1, u2, vn, v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalPoint1,
    AROS_LHA(GLint, i, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalPoint1(i);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEvalPoint2,
    AROS_LHA(GLint, i, D0),
    AROS_LHA(GLint, j, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalPoint2(i, j);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glEvalMesh1,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, i1, D1),
    AROS_LHA(GLint, i2, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalMesh1(mode, i1, i2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glEvalMesh2,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, i1, D1),
    AROS_LHA(GLint, i2, D2),
    AROS_LHA(GLint, j1, D3),
    AROS_LHA(GLint, j2, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEvalMesh2(mode, i1, i2, j1, j2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogf(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogi,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogi(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogfv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogfv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogiv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogiv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFeedbackBuffer,
    AROS_LHA(GLsizei, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLfloat *, buffer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFeedbackBuffer(size, type, buffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPassThrough,
    AROS_LHA(GLfloat, token, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPassThrough(token);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSelectBuffer,
    AROS_LHA(GLsizei, size, D0),
    AROS_LHA(GLuint *, buffer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSelectBuffer(size, buffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glInitNames,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglInitNames();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadName,
    AROS_LHA(GLuint, name, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLoadName(name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPushName,
    AROS_LHA(GLuint, name, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPushName(name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopName,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPopName();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glDrawRangeElements,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLuint, start, D1),
    AROS_LHA(GLuint, end, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, indices, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawRangeElements(mode, start, end, count, type, indices);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH10(void, glTexImage3D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, internalFormat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    AROS_LHA(GLsizei, depth, D5),
    AROS_LHA(GLint, border, D6),
    AROS_LHA(GLenum, format, D7),
    AROS_LHA(GLenum, type, A0),
    AROS_LHA(const GLvoid *, pixels, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH11(void, glTexSubImage3D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLint, zoffset, D4),
    AROS_LHA(GLsizei, width, D5),
    AROS_LHA(GLsizei, height, D6),
    AROS_LHA(GLsizei, depth, D7),
    AROS_LHA(GLenum, format, A0),
    AROS_LHA(GLenum, type, A1),
    AROS_LHA(const GLvoid *, pixels, A2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glCopyTexSubImage3D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLint, zoffset, D4),
    AROS_LHA(GLint, x, D5),
    AROS_LHA(GLint, y, D6),
    AROS_LHA(GLsizei, width, D7),
    AROS_LHA(GLsizei, height, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glColorTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, table, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorTable(target, internalformat, width, format, type, table);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glColorSubTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, start, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorSubTable(target, start, count, format, type, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorTableParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorTableParameterfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyColorSubTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, start, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyColorSubTable(target, start, x, y, width);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyColorTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyColorTable(target, internalformat, x, y, width);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetColorTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, table, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetColorTable(target, format, type, table);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetColorTableParameterfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetColorTableParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBlendEquation,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBlendEquation(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBlendColor,
    AROS_LHA(GLclampf, red, D0),
    AROS_LHA(GLclampf, green, D1),
    AROS_LHA(GLclampf, blue, D2),
    AROS_LHA(GLclampf, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBlendColor(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glHistogram,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, width, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLboolean, sink, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglHistogram(target, width, internalformat, sink);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetHistogram,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglResetHistogram(target);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetHistogram,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLboolean, reset, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(GLvoid *, values, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetHistogram(target, reset, format, type, values);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetHistogramParameterfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetHistogramParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMinmax,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLboolean, sink, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMinmax(target, internalformat, sink);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetMinmax,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglResetMinmax(target);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetMinmax,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLboolean, reset, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, types, D3),
    AROS_LHA(GLvoid *, values, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetMinmax(target, reset, format, types, values);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetMinmaxParameterfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetMinmaxParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glConvolutionFilter1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, image, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglConvolutionFilter1D(target, internalformat, width, format, type, image);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glConvolutionFilter2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(const GLvoid *, image, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglConvolutionFilter2D(target, internalformat, width, height, format, type, image);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterf,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, params, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglConvolutionParameterf(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglConvolutionParameterfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameteri,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, params, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglConvolutionParameteri(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglConvolutionParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyConvolutionFilter1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyConvolutionFilter1D(target, internalformat, x, y, width);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glCopyConvolutionFilter2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    AROS_LHA(GLsizei, height, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyConvolutionFilter2D(target, internalformat, x, y, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetConvolutionFilter,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, image, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetConvolutionFilter(target, format, type, image);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetConvolutionParameterfv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetConvolutionParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glSeparableFilter2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(const GLvoid *, row, A0),
    AROS_LHA(const GLvoid *, column, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSeparableFilter2D(target, internalformat, width, height, format, type, row, column);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glGetSeparableFilter,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, row, A0),
    AROS_LHA(GLvoid *, column, A1),
    AROS_LHA(GLvoid *, span, A2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetSeparableFilter(target, format, type, row, column, span);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveTexture,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglActiveTexture(texture);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClientActiveTexture,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglClientActiveTexture(texture);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glCompressedTexImage1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLint, border, D4),
    AROS_LHA(GLsizei, imageSize, D5),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glCompressedTexImage2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    AROS_LHA(GLint, border, D5),
    AROS_LHA(GLsizei, imageSize, D6),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glCompressedTexImage3D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    AROS_LHA(GLsizei, depth, D5),
    AROS_LHA(GLint, border, D6),
    AROS_LHA(GLsizei, imageSize, D7),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glCompressedTexSubImage1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLsizei, imageSize, D5),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glCompressedTexSubImage2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLsizei, width, D4),
    AROS_LHA(GLsizei, height, D5),
    AROS_LHA(GLenum, format, D6),
    AROS_LHA(GLsizei, imageSize, D7),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH11(void, glCompressedTexSubImage3D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLint, zoffset, D4),
    AROS_LHA(GLsizei, width, D5),
    AROS_LHA(GLsizei, height, D6),
    AROS_LHA(GLsizei, depth, D7),
    AROS_LHA(GLenum, format, A0),
    AROS_LHA(GLsizei, imageSize, A1),
    AROS_LHA(const GLvoid *, data, A2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetCompressedTexImage,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, lod, D1),
    AROS_LHA(GLvoid *, img, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetCompressedTexImage(target, lod, img);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1d(target, s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1dv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1f(target, s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1fv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1i(target, s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1iv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1s(target, s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1sv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2d(target, s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2dv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2f(target, s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2fv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2i(target, s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2iv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2s(target, s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2sv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    AROS_LHA(GLdouble, r, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3d(target, s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3dv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    AROS_LHA(GLfloat, r, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3f(target, s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3fv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    AROS_LHA(GLint, r, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3i(target, s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3iv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    AROS_LHA(GLshort, r, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3s(target, s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3sv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    AROS_LHA(GLdouble, r, D3),
    AROS_LHA(GLdouble, q, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4d(target, s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4dv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    AROS_LHA(GLfloat, r, D3),
    AROS_LHA(GLfloat, q, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4f(target, s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4fv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    AROS_LHA(GLint, r, D3),
    AROS_LHA(GLint, q, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4i(target, s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4iv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    AROS_LHA(GLshort, r, D3),
    AROS_LHA(GLshort, q, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4s(target, s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4sv(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLoadTransposeMatrixd(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLoadTransposeMatrixf(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultTransposeMatrixd(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultTransposeMatrixf(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleCoverage,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSampleCoverage(value, invert);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveTextureARB,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglActiveTextureARB(texture);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClientActiveTextureARB,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglClientActiveTextureARB(texture);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1dARB(target, s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1dvARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1fARB(target, s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1fvARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1iARB(target, s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1ivARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1sARB(target, s);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord1svARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2dARB(target, s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2dvARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2fARB(target, s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2fvARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2iARB(target, s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2ivARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2sARB(target, s, t);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord2svARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    AROS_LHA(GLdouble, r, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3dARB(target, s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3dvARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    AROS_LHA(GLfloat, r, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3fARB(target, s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3fvARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    AROS_LHA(GLint, r, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3iARB(target, s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3ivARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    AROS_LHA(GLshort, r, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3sARB(target, s, t, r);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord3svARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    AROS_LHA(GLdouble, r, D3),
    AROS_LHA(GLdouble, q, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4dARB(target, s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4dvARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    AROS_LHA(GLfloat, r, D3),
    AROS_LHA(GLfloat, q, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4fARB(target, s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4fvARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    AROS_LHA(GLint, r, D3),
    AROS_LHA(GLint, q, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4iARB(target, s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4ivARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    AROS_LHA(GLshort, r, D3),
    AROS_LHA(GLshort, q, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4sARB(target, s, t, r, q);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiTexCoord4svARB(target, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBlendFuncSeparate,
    AROS_LHA(GLenum, sfactorRGB, D0),
    AROS_LHA(GLenum, dfactorRGB, D1),
    AROS_LHA(GLenum, sfactorAlpha, D2),
    AROS_LHA(GLenum, dfactorAlpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordf,
    AROS_LHA(GLfloat, coord, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoordf(coord);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordfv,
    AROS_LHA(const GLfloat *, coord, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoordfv(coord);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordd,
    AROS_LHA(GLdouble, coord, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoordd(coord);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoorddv,
    AROS_LHA(const GLdouble *, coord, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoorddv(coord);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFogCoordPointer,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoordPointer(type, stride, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiDrawArrays,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLint *, first, A0),
    AROS_LHA(const GLsizei *, count, A1),
    AROS_LHA(GLsizei, primcount, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiDrawArrays(mode, first, count, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiDrawElements,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLsizei *, count, A0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(const GLvoid *  *, indices, A1),
    AROS_LHA(GLsizei, primcount, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiDrawElements(mode, count, type, indices, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameterf(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameterfv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameteri,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameteri(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameteriv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameteriv(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3b,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3b(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3bv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3d,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3d(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3f,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3f(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3i,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3i(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3s,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3s(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3ub,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3ub(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3ubv,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3ubv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3ui,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3ui(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3uiv,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3uiv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3us,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3us(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3usv,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3usv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glSecondaryColorPointer,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColorPointer(size, type, stride, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2d(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2f(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2i(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2s(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3d(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3dv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3f(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3fv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3i(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3iv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3s(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3sv(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenQueries,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, ids, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenQueries(n, ids);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteQueries,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, ids, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteQueries(n, ids);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsQuery,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsQuery(id);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginQuery,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBeginQuery(target, id);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEndQuery,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEndQuery(target);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetQueryiv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectiv,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetQueryObjectiv(id, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectuiv,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetQueryObjectuiv(id, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindBuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, buffer, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindBuffer(target, buffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteBuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, buffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteBuffers(n, buffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenBuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, buffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenBuffers(n, buffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsBuffer,
    AROS_LHA(GLuint, buffer, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsBuffer(buffer);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBufferData,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizeiptr, size, D1),
    AROS_LHA(const GLvoid *, data, A0),
    AROS_LHA(GLenum, usage, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBufferData(target, size, data, usage);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBufferSubData,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, size, D2),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBufferSubData(target, offset, size, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetBufferSubData,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, size, D2),
    AROS_LHA(GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetBufferSubData(target, offset, size, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLvoid*, glMapBuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, access, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLvoid* _return = mglMapBuffer(target, access);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glUnmapBuffer,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglUnmapBuffer(target);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetBufferParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferPointerv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetBufferPointerv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationSeparate,
    AROS_LHA(GLenum, modeRGB, D0),
    AROS_LHA(GLenum, modeAlpha, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBlendEquationSeparate(modeRGB, modeAlpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawBuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLenum *, bufs, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawBuffers(n, bufs);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glStencilOpSeparate,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, sfail, D1),
    AROS_LHA(GLenum, dpfail, D2),
    AROS_LHA(GLenum, dppass, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglStencilOpSeparate(face, sfail, dpfail, dppass);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glStencilFuncSeparate,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, func, D1),
    AROS_LHA(GLint, ref, D2),
    AROS_LHA(GLuint, mask, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglStencilFuncSeparate(face, func, ref, mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glStencilMaskSeparate,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLuint, mask, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglStencilMaskSeparate(face, mask);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAttachShader,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, shader, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglAttachShader(program, shader);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindAttribLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindAttribLocation(program, index, name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCompileShader,
    AROS_LHA(GLuint, shader, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompileShader(shader);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLuint, glCreateProgram,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLuint _return = mglCreateProgram();

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLuint, glCreateShader,
    AROS_LHA(GLenum, type, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLuint _return = mglCreateShader(type);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteProgram(program);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteShader,
    AROS_LHA(GLuint, shader, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteShader(shader);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDetachShader,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, shader, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDetachShader(program, shader);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisableVertexAttribArray,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDisableVertexAttribArray(index);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnableVertexAttribArray,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEnableVertexAttribArray(index);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glGetActiveAttrib,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLint *, size, A1),
    AROS_LHA(GLenum *, type, A2),
    AROS_LHA(GLchar *, name, A3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetActiveAttrib(program, index, bufSize, length, size, type, name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glGetActiveUniform,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLint *, size, A1),
    AROS_LHA(GLenum *, type, A2),
    AROS_LHA(GLchar *, name, A3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetActiveUniform(program, index, bufSize, length, size, type, name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetAttachedShaders,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLsizei, maxCount, D1),
    AROS_LHA(GLsizei *, count, A0),
    AROS_LHA(GLuint *, obj, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetAttachedShaders(program, maxCount, count, obj);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetAttribLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLint _return = mglGetAttribLocation(program, name);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramiv,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramiv(program, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramInfoLog,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLchar *, infoLog, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramInfoLog(program, bufSize, length, infoLog);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetShaderiv,
    AROS_LHA(GLuint, shader, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetShaderiv(shader, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetShaderInfoLog,
    AROS_LHA(GLuint, shader, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLchar *, infoLog, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetShaderInfoLog(shader, bufSize, length, infoLog);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetShaderSource,
    AROS_LHA(GLuint, shader, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLchar *, source, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetShaderSource(shader, bufSize, length, source);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetUniformLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLint _return = mglGetUniformLocation(program, name);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformfv,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetUniformfv(program, location, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformiv,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetUniformiv(program, location, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribdv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribdv(index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribfv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribfv(index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribiv(index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribPointerv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribPointerv(index, pname, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsProgram(program);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsShader,
    AROS_LHA(GLuint, shader, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsShader(shader);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLinkProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLinkProgram(program);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glShaderSource,
    AROS_LHA(GLuint, shader, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLchar *  *, string, A0),
    AROS_LHA(const GLint *, length, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglShaderSource(shader, count, string, length);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glUseProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUseProgram(program);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform1f(location, v0);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform2f(location, v0, v1);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    AROS_LHA(GLfloat, v2, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform3f(location, v0, v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    AROS_LHA(GLfloat, v2, D3),
    AROS_LHA(GLfloat, v3, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform4f(location, v0, v1, v2, v3);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform1i(location, v0);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform2i(location, v0, v1);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    AROS_LHA(GLint, v2, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform3i(location, v0, v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    AROS_LHA(GLint, v2, D3),
    AROS_LHA(GLint, v3, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform4i(location, v0, v1, v2, v3);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform1fv(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform2fv(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform3fv(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform4fv(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform1iv(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform2iv(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform3iv(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform4iv(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix2fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix2fv(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix3fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix3fv(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix4fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix4fv(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glValidateProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglValidateProgram(program);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1d(index, x);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1dv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1f(index, x);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1fv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1s(index, x);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1sv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2d(index, x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2dv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2f(index, x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2fv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2s(index, x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2sv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3d(index, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3dv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3f(index, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3fv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3s(index, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3sv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nbv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4Nbv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Niv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4Niv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nsv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4Nsv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4Nub,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLubyte, x, D1),
    AROS_LHA(GLubyte, y, D2),
    AROS_LHA(GLubyte, z, D3),
    AROS_LHA(GLubyte, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4Nub(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nubv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4Nubv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nuiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4Nuiv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nusv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4Nusv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4bv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4bv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    AROS_LHA(GLdouble, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4d(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4dv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    AROS_LHA(GLfloat, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4f(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4fv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4iv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    AROS_LHA(GLshort, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4s(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4sv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ubv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4ubv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4uiv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4usv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4usv(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glVertexAttribPointer,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, size, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLboolean, normalized, D3),
    AROS_LHA(GLsizei, stride, D4),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribPointer(index, size, type, normalized, stride, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix2x3fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix2x3fv(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix3x2fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix3x2fv(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix2x4fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix2x4fv(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix4x2fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix4x2fv(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix3x4fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix3x4fv(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix4x3fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix4x3fv(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixfARB,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLoadTransposeMatrixfARB(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixdARB,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLoadTransposeMatrixdARB(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixfARB,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultTransposeMatrixfARB(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixdARB,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultTransposeMatrixdARB(m);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleCoverageARB,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSampleCoverageARB(value, invert);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glCompressedTexImage3DARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    AROS_LHA(GLsizei, depth, D5),
    AROS_LHA(GLint, border, D6),
    AROS_LHA(GLsizei, imageSize, D7),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glCompressedTexImage2DARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    AROS_LHA(GLint, border, D5),
    AROS_LHA(GLsizei, imageSize, D6),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexImage2DARB(target, level, internalformat, width, height, border, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glCompressedTexImage1DARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLint, border, D4),
    AROS_LHA(GLsizei, imageSize, D5),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexImage1DARB(target, level, internalformat, width, border, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH11(void, glCompressedTexSubImage3DARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLint, zoffset, D4),
    AROS_LHA(GLsizei, width, D5),
    AROS_LHA(GLsizei, height, D6),
    AROS_LHA(GLsizei, depth, D7),
    AROS_LHA(GLenum, format, A0),
    AROS_LHA(GLsizei, imageSize, A1),
    AROS_LHA(const GLvoid *, data, A2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glCompressedTexSubImage2DARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLsizei, width, D4),
    AROS_LHA(GLsizei, height, D5),
    AROS_LHA(GLenum, format, D6),
    AROS_LHA(GLsizei, imageSize, D7),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glCompressedTexSubImage1DARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLsizei, imageSize, D5),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompressedTexSubImage1DARB(target, level, xoffset, width, format, imageSize, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetCompressedTexImageARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLvoid *, img, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetCompressedTexImageARB(target, level, img);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfARB,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameterfARB(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfvARB,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameterfvARB(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2dARB,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2dARB(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2dvARB,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2dvARB(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2fARB,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2fARB(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2fvARB,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2fvARB(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2iARB,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2iARB(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2ivARB,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2ivARB(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2sARB,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2sARB(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2svARB,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2svARB(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3dARB,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3dARB(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3dvARB,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3dvARB(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3fARB,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3fARB(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3fvARB,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3fvARB(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3iARB,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3iARB(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3ivARB,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3ivARB(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3sARB,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3sARB(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3svARB,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3svARB(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1dARB(index, x);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1dvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1fARB(index, x);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1fvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1sARB(index, x);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1svARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2dARB(index, x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2dvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2fARB(index, x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2fvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2sARB(index, x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2svARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3dARB(index, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3dvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3fARB(index, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3fvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3sARB(index, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3svARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NbvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4NbvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4NivARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NsvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4NsvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4NubARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLubyte, x, D1),
    AROS_LHA(GLubyte, y, D2),
    AROS_LHA(GLubyte, z, D3),
    AROS_LHA(GLubyte, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4NubARB(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NubvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4NubvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NuivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4NuivARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NusvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4NusvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4bvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4bvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    AROS_LHA(GLdouble, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4dARB(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4dvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    AROS_LHA(GLfloat, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4fARB(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4fvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4ivARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    AROS_LHA(GLshort, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4sARB(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4svARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ubvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4ubvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4uivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4uivARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4usvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4usvARB(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glVertexAttribPointerARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, size, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLboolean, normalized, D3),
    AROS_LHA(GLsizei, stride, D4),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribPointerARB(index, size, type, normalized, stride, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnableVertexAttribArrayARB,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEnableVertexAttribArrayARB(index);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisableVertexAttribArrayARB,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDisableVertexAttribArrayARB(index);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramStringARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLsizei, len, D2),
    AROS_LHA(const GLvoid *, string, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramStringARB(target, format, len, string);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindProgramARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, program, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindProgramARB(target, program);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteProgramsARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteProgramsARB(n, programs);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenProgramsARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, programs, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenProgramsARB(n, programs);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramEnvParameter4dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble, x, D2),
    AROS_LHA(GLdouble, y, D3),
    AROS_LHA(GLdouble, z, D4),
    AROS_LHA(GLdouble, w, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramEnvParameter4dARB(target, index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramEnvParameter4dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramEnvParameter4dvARB(target, index, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramEnvParameter4fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat, x, D2),
    AROS_LHA(GLfloat, y, D3),
    AROS_LHA(GLfloat, z, D4),
    AROS_LHA(GLfloat, w, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramEnvParameter4fARB(target, index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramEnvParameter4fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramEnvParameter4fvARB(target, index, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramLocalParameter4dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble, x, D2),
    AROS_LHA(GLdouble, y, D3),
    AROS_LHA(GLdouble, z, D4),
    AROS_LHA(GLdouble, w, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramLocalParameter4dARB(target, index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramLocalParameter4dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramLocalParameter4dvARB(target, index, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramLocalParameter4fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat, x, D2),
    AROS_LHA(GLfloat, y, D3),
    AROS_LHA(GLfloat, z, D4),
    AROS_LHA(GLfloat, w, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramLocalParameter4fARB(target, index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramLocalParameter4fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramLocalParameter4fvARB(target, index, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramEnvParameterdvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramEnvParameterdvARB(target, index, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramEnvParameterfvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramEnvParameterfvARB(target, index, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramLocalParameterdvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramLocalParameterdvARB(target, index, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramLocalParameterfvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramLocalParameterfvARB(target, index, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramivARB(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramStringARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *, string, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramStringARB(target, pname, string);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribdvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribdvARB(index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribfvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribfvARB(index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribivARB(index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribPointervARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribPointervARB(index, pname, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsProgramARB,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsProgramARB(program);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindBufferARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, buffer, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindBufferARB(target, buffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteBuffersARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, buffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteBuffersARB(n, buffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenBuffersARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, buffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenBuffersARB(n, buffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsBufferARB,
    AROS_LHA(GLuint, buffer, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsBufferARB(buffer);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBufferDataARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizeiptrARB, size, D1),
    AROS_LHA(const GLvoid *, data, A0),
    AROS_LHA(GLenum, usage, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBufferDataARB(target, size, data, usage);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBufferSubDataARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptrARB, offset, D1),
    AROS_LHA(GLsizeiptrARB, size, D2),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBufferSubDataARB(target, offset, size, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetBufferSubDataARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptrARB, offset, D1),
    AROS_LHA(GLsizeiptrARB, size, D2),
    AROS_LHA(GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetBufferSubDataARB(target, offset, size, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLvoid*, glMapBufferARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, access, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLvoid* _return = mglMapBufferARB(target, access);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glUnmapBufferARB,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglUnmapBufferARB(target);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferParameterivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetBufferParameterivARB(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferPointervARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetBufferPointervARB(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenQueriesARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, ids, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenQueriesARB(n, ids);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteQueriesARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, ids, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteQueriesARB(n, ids);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsQueryARB,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsQueryARB(id);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginQueryARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBeginQueryARB(target, id);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEndQueryARB,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEndQueryARB(target);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetQueryivARB(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectivARB,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetQueryObjectivARB(id, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectuivARB,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetQueryObjectuivARB(id, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteObjectARB,
    AROS_LHA(GLhandleARB, obj, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteObjectARB(obj);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLhandleARB, glGetHandleARB,
    AROS_LHA(GLenum, pname, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLhandleARB _return = mglGetHandleARB(pname);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDetachObjectARB,
    AROS_LHA(GLhandleARB, containerObj, D0),
    AROS_LHA(GLhandleARB, attachedObj, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDetachObjectARB(containerObj, attachedObj);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLhandleARB, glCreateShaderObjectARB,
    AROS_LHA(GLenum, shaderType, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLhandleARB _return = mglCreateShaderObjectARB(shaderType);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glShaderSourceARB,
    AROS_LHA(GLhandleARB, shaderObj, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLcharARB *  *, string, A0),
    AROS_LHA(const GLint *, length, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglShaderSourceARB(shaderObj, count, string, length);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCompileShaderARB,
    AROS_LHA(GLhandleARB, shaderObj, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCompileShaderARB(shaderObj);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLhandleARB, glCreateProgramObjectARB,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLhandleARB _return = mglCreateProgramObjectARB();

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAttachObjectARB,
    AROS_LHA(GLhandleARB, containerObj, D0),
    AROS_LHA(GLhandleARB, obj, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglAttachObjectARB(containerObj, obj);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLinkProgramARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLinkProgramARB(programObj);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glUseProgramObjectARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUseProgramObjectARB(programObj);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glValidateProgramARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglValidateProgramARB(programObj);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform1fARB(location, v0);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform2fARB(location, v0, v1);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    AROS_LHA(GLfloat, v2, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform3fARB(location, v0, v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    AROS_LHA(GLfloat, v2, D3),
    AROS_LHA(GLfloat, v3, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform4fARB(location, v0, v1, v2, v3);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform1iARB(location, v0);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform2iARB(location, v0, v1);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    AROS_LHA(GLint, v2, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform3iARB(location, v0, v1, v2);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    AROS_LHA(GLint, v2, D3),
    AROS_LHA(GLint, v3, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform4iARB(location, v0, v1, v2, v3);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform1fvARB(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform2fvARB(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform3fvARB(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform4fvARB(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform1ivARB(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform2ivARB(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform3ivARB(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniform4ivARB(location, count, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix2fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix2fvARB(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix3fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix3fvARB(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix4fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUniformMatrix4fvARB(location, count, transpose, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetObjectParameterfvARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetObjectParameterfvARB(obj, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetObjectParameterivARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetObjectParameterivARB(obj, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetInfoLogARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLsizei, maxLength, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLcharARB *, infoLog, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetInfoLogARB(obj, maxLength, length, infoLog);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetAttachedObjectsARB,
    AROS_LHA(GLhandleARB, containerObj, D0),
    AROS_LHA(GLsizei, maxCount, D1),
    AROS_LHA(GLsizei *, count, A0),
    AROS_LHA(GLhandleARB *, obj, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetAttachedObjectsARB(containerObj, maxCount, count, obj);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetUniformLocationARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(const GLcharARB *, name, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLint _return = mglGetUniformLocationARB(programObj, name);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glGetActiveUniformARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, maxLength, D2),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLint *, size, A1),
    AROS_LHA(GLenum *, type, A2),
    AROS_LHA(GLcharARB *, name, A3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetActiveUniformARB(programObj, index, maxLength, length, size, type, name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformfvARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetUniformfvARB(programObj, location, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformivARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetUniformivARB(programObj, location, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetShaderSourceARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLsizei, maxLength, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLcharARB *, source, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetShaderSourceARB(obj, maxLength, length, source);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindAttribLocationARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLcharARB *, name, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindAttribLocationARB(programObj, index, name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glGetActiveAttribARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, maxLength, D2),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLint *, size, A1),
    AROS_LHA(GLenum *, type, A2),
    AROS_LHA(GLcharARB *, name, A3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetActiveAttribARB(programObj, index, maxLength, length, size, type, name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetAttribLocationARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(const GLcharARB *, name, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLint _return = mglGetAttribLocationARB(programObj, name);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawBuffersARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLenum *, bufs, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawBuffersARB(n, bufs);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsRenderbuffer,
    AROS_LHA(GLuint, renderbuffer, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsRenderbuffer(renderbuffer);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindRenderbuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, renderbuffer, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindRenderbuffer(target, renderbuffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteRenderbuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteRenderbuffers(n, renderbuffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenRenderbuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenRenderbuffers(n, renderbuffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRenderbufferStorage,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRenderbufferStorage(target, internalformat, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetRenderbufferParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetRenderbufferParameteriv(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsFramebuffer,
    AROS_LHA(GLuint, framebuffer, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsFramebuffer(framebuffer);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindFramebuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, framebuffer, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindFramebuffer(target, framebuffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteFramebuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteFramebuffers(n, framebuffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenFramebuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenFramebuffers(n, framebuffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLenum, glCheckFramebufferStatus,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLenum _return = mglCheckFramebufferStatus(target);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTexture1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTexture1D(target, attachment, textarget, texture, level);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTexture2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTexture2D(target, attachment, textarget, texture, level);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glFramebufferTexture3D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    AROS_LHA(GLint, zoffset, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glFramebufferRenderbuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, renderbuffertarget, D2),
    AROS_LHA(GLuint, renderbuffer, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetFramebufferAttachmentParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetFramebufferAttachmentParameteriv(target, attachment, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glGenerateMipmap,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenerateMipmap(target);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH10(void, glBlitFramebuffer,
    AROS_LHA(GLint, srcX0, D0),
    AROS_LHA(GLint, srcY0, D1),
    AROS_LHA(GLint, srcX1, D2),
    AROS_LHA(GLint, srcY1, D3),
    AROS_LHA(GLint, dstX0, D4),
    AROS_LHA(GLint, dstY0, D5),
    AROS_LHA(GLint, dstX1, D6),
    AROS_LHA(GLint, dstY1, D7),
    AROS_LHA(GLbitfield, mask, A0),
    AROS_LHA(GLenum, filter, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glRenderbufferStorageMultisample,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, samples, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRenderbufferStorageMultisample(target, samples, internalformat, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTextureLayer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    AROS_LHA(GLint, layer, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTextureLayer(target, attachment, texture, level, layer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBlendColorEXT,
    AROS_LHA(GLclampf, red, D0),
    AROS_LHA(GLclampf, green, D1),
    AROS_LHA(GLclampf, blue, D2),
    AROS_LHA(GLclampf, alpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBlendColorEXT(red, green, blue, alpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPolygonOffsetEXT,
    AROS_LHA(GLfloat, factor, D0),
    AROS_LHA(GLfloat, bias, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPolygonOffsetEXT(factor, bias);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH10(void, glTexImage3DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    AROS_LHA(GLsizei, depth, D5),
    AROS_LHA(GLint, border, D6),
    AROS_LHA(GLenum, format, D7),
    AROS_LHA(GLenum, type, A0),
    AROS_LHA(const GLvoid *, pixels, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH11(void, glTexSubImage3DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLint, zoffset, D4),
    AROS_LHA(GLsizei, width, D5),
    AROS_LHA(GLsizei, height, D6),
    AROS_LHA(GLsizei, depth, D7),
    AROS_LHA(GLenum, format, A0),
    AROS_LHA(GLenum, type, A1),
    AROS_LHA(const GLvoid *, pixels, A2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glTexSubImage1DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(const GLvoid *, pixels, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexSubImage1DEXT(target, level, xoffset, width, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glTexSubImage2DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLsizei, width, D4),
    AROS_LHA(GLsizei, height, D5),
    AROS_LHA(GLenum, format, D6),
    AROS_LHA(GLenum, type, D7),
    AROS_LHA(const GLvoid *, pixels, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexSubImage2DEXT(target, level, xoffset, yoffset, width, height, format, type, pixels);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glCopyTexImage1DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLint, x, D3),
    AROS_LHA(GLint, y, D4),
    AROS_LHA(GLsizei, width, D5),
    AROS_LHA(GLint, border, D6),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexImage1DEXT(target, level, internalformat, x, y, width, border);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glCopyTexImage2DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLint, x, D3),
    AROS_LHA(GLint, y, D4),
    AROS_LHA(GLsizei, width, D5),
    AROS_LHA(GLsizei, height, D6),
    AROS_LHA(GLint, border, D7),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexImage2DEXT(target, level, internalformat, x, y, width, height, border);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glCopyTexSubImage1DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, x, D3),
    AROS_LHA(GLint, y, D4),
    AROS_LHA(GLsizei, width, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexSubImage1DEXT(target, level, xoffset, x, y, width);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glCopyTexSubImage2DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLint, x, D4),
    AROS_LHA(GLint, y, D5),
    AROS_LHA(GLsizei, width, D6),
    AROS_LHA(GLsizei, height, D7),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glCopyTexSubImage3DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, yoffset, D3),
    AROS_LHA(GLint, zoffset, D4),
    AROS_LHA(GLint, x, D5),
    AROS_LHA(GLint, y, D6),
    AROS_LHA(GLsizei, width, D7),
    AROS_LHA(GLsizei, height, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLboolean, glAreTexturesResidentEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(GLboolean *, residences, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglAreTexturesResidentEXT(n, textures, residences);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindTextureEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, texture, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindTextureEXT(target, texture);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteTexturesEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteTexturesEXT(n, textures);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenTexturesEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, textures, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenTexturesEXT(n, textures);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsTextureEXT,
    AROS_LHA(GLuint, texture, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsTextureEXT(texture);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPrioritizeTexturesEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(const GLclampf *, priorities, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPrioritizeTexturesEXT(n, textures, priorities);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glArrayElementEXT,
    AROS_LHA(GLint, i, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglArrayElementEXT(i);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glColorPointerEXT,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorPointerEXT(size, type, stride, count, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glDrawArraysEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawArraysEXT(mode, first, count);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glEdgeFlagPointerEXT,
    AROS_LHA(GLsizei, stride, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLboolean *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEdgeFlagPointerEXT(stride, count, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPointervEXT,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetPointervEXT(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glIndexPointerEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglIndexPointerEXT(type, stride, count, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glNormalPointerEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglNormalPointerEXT(type, stride, count, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glTexCoordPointerEXT,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexCoordPointerEXT(size, type, stride, count, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexPointerEXT,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexPointerEXT(size, type, stride, count, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBlendEquationEXT,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBlendEquationEXT(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfEXT,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameterfEXT(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfvEXT,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameterfvEXT(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glColorTableEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalFormat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, table, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorTableEXT(target, internalFormat, width, format, type, table);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetColorTableEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetColorTableEXT(target, format, type, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetColorTableParameterivEXT(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetColorTableParameterfvEXT(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLockArraysEXT,
    AROS_LHA(GLint, first, D0),
    AROS_LHA(GLsizei, count, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLockArraysEXT(first, count);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glUnlockArraysEXT,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglUnlockArraysEXT();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glDrawRangeElementsEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLuint, start, D1),
    AROS_LHA(GLuint, end, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, indices, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawRangeElementsEXT(mode, start, end, count, type, indices);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3bEXT,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3bEXT(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3bvEXT,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3bvEXT(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3dEXT,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3dEXT(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3dvEXT,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3dvEXT(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3fEXT,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3fEXT(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3fvEXT,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3fvEXT(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3iEXT,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3iEXT(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3ivEXT,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3ivEXT(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3sEXT,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3sEXT(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3svEXT,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3svEXT(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3ubEXT,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3ubEXT(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3ubvEXT,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3ubvEXT(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3uiEXT,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3uiEXT(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3uivEXT,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3uivEXT(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3usEXT,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3usEXT(red, green, blue);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3usvEXT,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColor3usvEXT(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glSecondaryColorPointerEXT,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSecondaryColorPointerEXT(size, type, stride, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiDrawArraysEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLint *, first, A0),
    AROS_LHA(const GLsizei *, count, A1),
    AROS_LHA(GLsizei, primcount, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiDrawArraysEXT(mode, first, count, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiDrawElementsEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLsizei *, count, A0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(const GLvoid *  *, indices, A1),
    AROS_LHA(GLsizei, primcount, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiDrawElementsEXT(mode, count, type, indices, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordfEXT,
    AROS_LHA(GLfloat, coord, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoordfEXT(coord);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordfvEXT,
    AROS_LHA(const GLfloat *, coord, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoordfvEXT(coord);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoorddEXT,
    AROS_LHA(GLdouble, coord, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoorddEXT(coord);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoorddvEXT,
    AROS_LHA(const GLdouble *, coord, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoorddvEXT(coord);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFogCoordPointerEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFogCoordPointerEXT(type, stride, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBlendFuncSeparateEXT,
    AROS_LHA(GLenum, sfactorRGB, D0),
    AROS_LHA(GLenum, dfactorRGB, D1),
    AROS_LHA(GLenum, sfactorAlpha, D2),
    AROS_LHA(GLenum, dfactorAlpha, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBlendFuncSeparateEXT(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glFlushVertexArrayRangeNV,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFlushVertexArrayRangeNV();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexArrayRangeNV,
    AROS_LHA(GLsizei, length, D0),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexArrayRangeNV(length, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameterfvNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCombinerParameterfvNV(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameterfNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCombinerParameterfNV(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameterivNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCombinerParameterivNV(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameteriNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCombinerParameteriNV(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glCombinerInputNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, variable, D2),
    AROS_LHA(GLenum, input, D3),
    AROS_LHA(GLenum, mapping, D4),
    AROS_LHA(GLenum, componentUsage, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCombinerInputNV(stage, portion, variable, input, mapping, componentUsage);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH10(void, glCombinerOutputNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, abOutput, D2),
    AROS_LHA(GLenum, cdOutput, D3),
    AROS_LHA(GLenum, sumOutput, D4),
    AROS_LHA(GLenum, scale, D5),
    AROS_LHA(GLenum, bias, D6),
    AROS_LHA(GLboolean, abDotProduct, D7),
    AROS_LHA(GLboolean, cdDotProduct, A0),
    AROS_LHA(GLboolean, muxSum, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCombinerOutputNV(stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glFinalCombinerInputNV,
    AROS_LHA(GLenum, variable, D0),
    AROS_LHA(GLenum, input, D1),
    AROS_LHA(GLenum, mapping, D2),
    AROS_LHA(GLenum, componentUsage, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFinalCombinerInputNV(variable, input, mapping, componentUsage);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetCombinerInputParameterfvNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, variable, D2),
    AROS_LHA(GLenum, pname, D3),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetCombinerInputParameterfvNV(stage, portion, variable, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetCombinerInputParameterivNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, variable, D2),
    AROS_LHA(GLenum, pname, D3),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetCombinerInputParameterivNV(stage, portion, variable, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetCombinerOutputParameterfvNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetCombinerOutputParameterfvNV(stage, portion, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetCombinerOutputParameterivNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetCombinerOutputParameterivNV(stage, portion, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetFinalCombinerInputParameterfvNV,
    AROS_LHA(GLenum, variable, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetFinalCombinerInputParameterfvNV(variable, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetFinalCombinerInputParameterivNV,
    AROS_LHA(GLenum, variable, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetFinalCombinerInputParameterivNV(variable, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glResizeBuffersMESA,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglResizeBuffersMESA();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2dMESA,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2dMESA(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2dvMESA,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2dvMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2fMESA,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2fMESA(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2fvMESA,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2fvMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2iMESA,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2iMESA(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2ivMESA,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2ivMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2sMESA,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2sMESA(x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2svMESA,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos2svMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3dMESA,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3dMESA(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3dvMESA,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3dvMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3fMESA,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3fMESA(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3fvMESA,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3fvMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3iMESA,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3iMESA(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3ivMESA,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3ivMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3sMESA,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3sMESA(x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3svMESA,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos3svMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glWindowPos4dMESA,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    AROS_LHA(GLdouble, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos4dMESA(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4dvMESA,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos4dvMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glWindowPos4fMESA,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    AROS_LHA(GLfloat, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos4fMESA(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4fvMESA,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos4fvMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glWindowPos4iMESA,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    AROS_LHA(GLint, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos4iMESA(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4ivMESA,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos4ivMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glWindowPos4sMESA,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    AROS_LHA(GLshort, w, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos4sMESA(x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4svMESA,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWindowPos4svMESA(v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLboolean, glAreProgramsResidentNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    AROS_LHA(GLboolean *, residences, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglAreProgramsResidentNV(n, programs, residences);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindProgramNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindProgramNV(target, id);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteProgramsNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteProgramsNV(n, programs);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glExecuteProgramNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglExecuteProgramNV(target, id, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenProgramsNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, programs, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenProgramsNV(n, programs);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramParameterdvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramParameterdvNV(target, index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramParameterfvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramParameterfvNV(target, index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramivNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramivNV(id, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramStringNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLubyte *, program, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramStringNV(id, pname, program);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetTrackMatrixivNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, address, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTrackMatrixivNV(target, address, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribdvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribdvNV(index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribfvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribfvNV(index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribivNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribivNV(index, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribPointervNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetVertexAttribPointervNV(index, pname, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsProgramNV,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsProgramNV(id);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glLoadProgramNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    AROS_LHA(GLsizei, len, D2),
    AROS_LHA(const GLubyte *, program, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglLoadProgramNV(target, id, len, program);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramParameter4dNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble, x, D2),
    AROS_LHA(GLdouble, y, D3),
    AROS_LHA(GLdouble, z, D4),
    AROS_LHA(GLdouble, w, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramParameter4dNV(target, index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramParameter4dvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramParameter4dvNV(target, index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramParameter4fNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat, x, D2),
    AROS_LHA(GLfloat, y, D3),
    AROS_LHA(GLfloat, z, D4),
    AROS_LHA(GLfloat, w, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramParameter4fNV(target, index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramParameter4fvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramParameter4fvNV(target, index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramParameters4dvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, count, D2),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramParameters4dvNV(target, index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramParameters4fvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, count, D2),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramParameters4fvNV(target, index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRequestResidentProgramsNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRequestResidentProgramsNV(n, programs);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTrackMatrixNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, address, D1),
    AROS_LHA(GLenum, matrix, D2),
    AROS_LHA(GLenum, transform, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTrackMatrixNV(target, address, matrix, transform);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttribPointerNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, fsize, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLsizei, stride, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribPointerNV(index, fsize, type, stride, pointer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1dNV(index, x);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1dvNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1fNV(index, x);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1fvNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1sNV(index, x);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib1svNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2dNV(index, x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2dvNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2fNV(index, x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2fvNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2sNV(index, x, y);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib2svNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3dNV(index, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3dvNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3fNV(index, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3fvNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3sNV(index, x, y, z);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib3svNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    AROS_LHA(GLdouble, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4dNV(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4dvNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    AROS_LHA(GLfloat, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4fNV(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4fvNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    AROS_LHA(GLshort, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4sNV(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4svNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4ubNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLubyte, x, D1),
    AROS_LHA(GLubyte, y, D2),
    AROS_LHA(GLubyte, z, D3),
    AROS_LHA(GLubyte, w, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4ubNV(index, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ubvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttrib4ubvNV(index, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs1dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs1dvNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs1fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs1fvNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs1svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs1svNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs2dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs2dvNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs2fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs2fvNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs2svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs2svNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs3dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs3dvNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs3fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs3fvNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs3svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs3svNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs4dvNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs4fvNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs4svNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4ubvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglVertexAttribs4ubvNV(index, count, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexBumpParameterivATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, param, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexBumpParameterivATI(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexBumpParameterfvATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, param, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTexBumpParameterfvATI(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetTexBumpParameterivATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint *, param, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexBumpParameterivATI(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetTexBumpParameterfvATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat *, param, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTexBumpParameterfvATI(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLuint, glGenFragmentShadersATI,
    AROS_LHA(GLuint, range, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLuint _return = mglGenFragmentShadersATI(range);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBindFragmentShaderATI,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindFragmentShaderATI(id);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteFragmentShaderATI,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteFragmentShaderATI(id);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glBeginFragmentShaderATI,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBeginFragmentShaderATI();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndFragmentShaderATI,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEndFragmentShaderATI();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPassTexCoordATI,
    AROS_LHA(GLuint, dst, D0),
    AROS_LHA(GLuint, coord, D1),
    AROS_LHA(GLenum, swizzle, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPassTexCoordATI(dst, coord, swizzle);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSampleMapATI,
    AROS_LHA(GLuint, dst, D0),
    AROS_LHA(GLuint, interp, D1),
    AROS_LHA(GLenum, swizzle, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSampleMapATI(dst, interp, swizzle);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glColorFragmentOp1ATI,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLuint, dst, D1),
    AROS_LHA(GLuint, dstMask, D2),
    AROS_LHA(GLuint, dstMod, D3),
    AROS_LHA(GLuint, arg1, D4),
    AROS_LHA(GLuint, arg1Rep, D5),
    AROS_LHA(GLuint, arg1Mod, D6),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorFragmentOp1ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH10(void, glColorFragmentOp2ATI,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLuint, dst, D1),
    AROS_LHA(GLuint, dstMask, D2),
    AROS_LHA(GLuint, dstMod, D3),
    AROS_LHA(GLuint, arg1, D4),
    AROS_LHA(GLuint, arg1Rep, D5),
    AROS_LHA(GLuint, arg1Mod, D6),
    AROS_LHA(GLuint, arg2, D7),
    AROS_LHA(GLuint, arg2Rep, A0),
    AROS_LHA(GLuint, arg2Mod, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorFragmentOp2ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH13(void, glColorFragmentOp3ATI,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLuint, dst, D1),
    AROS_LHA(GLuint, dstMask, D2),
    AROS_LHA(GLuint, dstMod, D3),
    AROS_LHA(GLuint, arg1, D4),
    AROS_LHA(GLuint, arg1Rep, D5),
    AROS_LHA(GLuint, arg1Mod, D6),
    AROS_LHA(GLuint, arg2, D7),
    AROS_LHA(GLuint, arg2Rep, A0),
    AROS_LHA(GLuint, arg2Mod, A1),
    AROS_LHA(GLuint, arg3, A2),
    AROS_LHA(GLuint, arg3Rep, A3),
    AROS_LHA(GLuint, arg3Mod, A4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorFragmentOp3ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glAlphaFragmentOp1ATI,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLuint, dst, D1),
    AROS_LHA(GLuint, dstMod, D2),
    AROS_LHA(GLuint, arg1, D3),
    AROS_LHA(GLuint, arg1Rep, D4),
    AROS_LHA(GLuint, arg1Mod, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglAlphaFragmentOp1ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, glAlphaFragmentOp2ATI,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLuint, dst, D1),
    AROS_LHA(GLuint, dstMod, D2),
    AROS_LHA(GLuint, arg1, D3),
    AROS_LHA(GLuint, arg1Rep, D4),
    AROS_LHA(GLuint, arg1Mod, D5),
    AROS_LHA(GLuint, arg2, D6),
    AROS_LHA(GLuint, arg2Rep, D7),
    AROS_LHA(GLuint, arg2Mod, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglAlphaFragmentOp2ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH12(void, glAlphaFragmentOp3ATI,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLuint, dst, D1),
    AROS_LHA(GLuint, dstMod, D2),
    AROS_LHA(GLuint, arg1, D3),
    AROS_LHA(GLuint, arg1Rep, D4),
    AROS_LHA(GLuint, arg1Mod, D5),
    AROS_LHA(GLuint, arg2, D6),
    AROS_LHA(GLuint, arg2Rep, D7),
    AROS_LHA(GLuint, arg2Mod, A0),
    AROS_LHA(GLuint, arg3, A1),
    AROS_LHA(GLuint, arg3Rep, A2),
    AROS_LHA(GLuint, arg3Mod, A3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglAlphaFragmentOp3ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSetFragmentShaderConstantATI,
    AROS_LHA(GLuint, dst, D0),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglSetFragmentShaderConstantATI(dst, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameteriNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameteriNV(pname, param);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterivNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPointParameterivNV(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawBuffersATI,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLenum *, bufs, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawBuffersATI(n, bufs);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glProgramNamedParameter4fNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(GLfloat, x, D2),
    AROS_LHA(GLfloat, y, D3),
    AROS_LHA(GLfloat, z, D4),
    AROS_LHA(GLfloat, w, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramNamedParameter4fNV(id, len, name, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glProgramNamedParameter4dNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(GLdouble, x, D2),
    AROS_LHA(GLdouble, y, D3),
    AROS_LHA(GLdouble, z, D4),
    AROS_LHA(GLdouble, w, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramNamedParameter4dNV(id, len, name, x, y, z, w);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramNamedParameter4fvNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(const GLfloat *, v, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramNamedParameter4fvNV(id, len, name, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramNamedParameter4dvNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(const GLdouble *, v, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramNamedParameter4dvNV(id, len, name, v);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramNamedParameterfvNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(GLfloat *, params, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramNamedParameterfvNV(id, len, name, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramNamedParameterdvNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(GLdouble *, params, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetProgramNamedParameterdvNV(id, len, name, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsRenderbufferEXT,
    AROS_LHA(GLuint, renderbuffer, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsRenderbufferEXT(renderbuffer);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindRenderbufferEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, renderbuffer, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindRenderbufferEXT(target, renderbuffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteRenderbuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteRenderbuffersEXT(n, renderbuffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenRenderbuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenRenderbuffersEXT(n, renderbuffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRenderbufferStorageEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRenderbufferStorageEXT(target, internalformat, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetRenderbufferParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetRenderbufferParameterivEXT(target, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsFramebufferEXT,
    AROS_LHA(GLuint, framebuffer, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsFramebufferEXT(framebuffer);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindFramebufferEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, framebuffer, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindFramebufferEXT(target, framebuffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteFramebuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteFramebuffersEXT(n, framebuffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenFramebuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenFramebuffersEXT(n, framebuffers);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLenum, glCheckFramebufferStatusEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLenum _return = mglCheckFramebufferStatusEXT(target);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTexture1DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTexture1DEXT(target, attachment, textarget, texture, level);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTexture2DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTexture2DEXT(target, attachment, textarget, texture, level);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glFramebufferTexture3DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    AROS_LHA(GLint, zoffset, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTexture3DEXT(target, attachment, textarget, texture, level, zoffset);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glFramebufferRenderbufferEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, renderbuffertarget, D2),
    AROS_LHA(GLuint, renderbuffer, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferRenderbufferEXT(target, attachment, renderbuffertarget, renderbuffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetFramebufferAttachmentParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetFramebufferAttachmentParameterivEXT(target, attachment, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glGenerateMipmapEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenerateMipmapEXT(target);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTextureLayerEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    AROS_LHA(GLint, layer, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTextureLayerEXT(target, attachment, texture, level, layer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(GLvoid*, glMapBufferRange,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, length, D2),
    AROS_LHA(GLbitfield, access, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLvoid* _return = mglMapBufferRange(target, offset, length, access);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFlushMappedBufferRange,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, length, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFlushMappedBufferRange(target, offset, length);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBindVertexArray,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindVertexArray(array);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteVertexArrays,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, arrays, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteVertexArrays(n, arrays);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenVertexArrays,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, arrays, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenVertexArrays(n, arrays);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsVertexArray,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsVertexArray(array);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyBufferSubData,
    AROS_LHA(GLenum, readTarget, D0),
    AROS_LHA(GLenum, writeTarget, D1),
    AROS_LHA(GLintptr, readOffset, D2),
    AROS_LHA(GLintptr, writeOffset, D3),
    AROS_LHA(GLsizeiptr, size, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLsync, glFenceSync,
    AROS_LHA(GLenum, condition, D0),
    AROS_LHA(GLbitfield, flags, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLsync _return = mglFenceSync(condition, flags);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsSync,
    AROS_LHA(GLsync, sync, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsSync(sync);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteSync,
    AROS_LHA(GLsync, sync, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteSync(sync);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLenum, glClientWaitSync,
    AROS_LHA(GLsync, sync, D0),
    AROS_LHA(GLbitfield, flags, D1),
    AROS_LHA(GLuint64, timeout, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLenum _return = mglClientWaitSync(sync, flags, timeout);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWaitSync,
    AROS_LHA(GLsync, sync, D0),
    AROS_LHA(GLbitfield, flags, D1),
    AROS_LHA(GLuint64, timeout, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglWaitSync(sync, flags, timeout);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetInteger64v,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint64 *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetInteger64v(pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetSynciv,
    AROS_LHA(GLsync, sync, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLint *, values, A1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetSynciv(sync, pname, bufSize, length, values);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glProvokingVertexEXT,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProvokingVertexEXT(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawElementsBaseVertex,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLint, basevertex, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawElementsBaseVertex(mode, count, type, indices, basevertex);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glDrawRangeElementsBaseVertex,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLuint, start, D1),
    AROS_LHA(GLuint, end, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLint, basevertex, D5),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMultiDrawElementsBaseVertex,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLsizei *, count, A0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(const GLvoid *  *, indices, A1),
    AROS_LHA(GLsizei, primcount, D2),
    AROS_LHA(const GLint *, basevertex, A2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglMultiDrawElementsBaseVertex(mode, count, type, indices, primcount, basevertex);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glProvokingVertex,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProvokingVertex(mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glRenderbufferStorageMultisampleEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, samples, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglRenderbufferStorageMultisampleEXT(target, samples, internalformat, width, height);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glColorMaskIndexedEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLboolean, r, D1),
    AROS_LHA(GLboolean, g, D2),
    AROS_LHA(GLboolean, b, D3),
    AROS_LHA(GLboolean, a, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglColorMaskIndexedEXT(index, r, g, b, a);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBooleanIndexedvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLboolean *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetBooleanIndexedvEXT(target, index, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetIntegerIndexedvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLint *, data, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetIntegerIndexedvEXT(target, index, data);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEnableIndexedEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEnableIndexedEXT(target, index);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDisableIndexedEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDisableIndexedEXT(target, index);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLboolean, glIsEnabledIndexedEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsEnabledIndexedEXT(target, index);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginConditionalRenderNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBeginConditionalRenderNV(id, mode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndConditionalRenderNV,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEndConditionalRenderNV();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLenum, glObjectPurgeableAPPLE,
    AROS_LHA(GLenum, objectType, D0),
    AROS_LHA(GLuint, name, D1),
    AROS_LHA(GLenum, option, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLenum _return = mglObjectPurgeableAPPLE(objectType, name, option);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLenum, glObjectUnpurgeableAPPLE,
    AROS_LHA(GLenum, objectType, D0),
    AROS_LHA(GLuint, name, D1),
    AROS_LHA(GLenum, option, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLenum _return = mglObjectUnpurgeableAPPLE(objectType, name, option);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetObjectParameterivAPPLE,
    AROS_LHA(GLenum, objectType, D0),
    AROS_LHA(GLuint, name, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetObjectParameterivAPPLE(objectType, name, pname, params);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBeginTransformFeedback,
    AROS_LHA(GLenum, primitiveMode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBeginTransformFeedback(primitiveMode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndTransformFeedback,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEndTransformFeedback();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glBindBufferRange,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    AROS_LHA(GLintptr, offset, D3),
    AROS_LHA(GLsizeiptr, size, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindBufferRange(target, index, buffer, offset, size);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindBufferBase,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindBufferBase(target, index, buffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTransformFeedbackVaryings,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLchar *  *, varyings, A0),
    AROS_LHA(GLenum, bufferMode, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTransformFeedbackVaryings(program, count, varyings, bufferMode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glGetTransformFeedbackVarying,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLsizei *, size, A1),
    AROS_LHA(GLenum *, type, A2),
    AROS_LHA(GLchar *, name, A3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glDrawArraysInstanced,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawArraysInstanced(mode, first, count, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawElementsInstanced,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawElementsInstanced(mode, count, type, indices, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glDrawArraysInstancedARB,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawArraysInstancedARB(mode, first, count, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawElementsInstancedARB,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawElementsInstancedARB(mode, count, type, indices, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramParameteriARB,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, value, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglProgramParameteriARB(program, pname, value);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glFramebufferTextureARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTextureARB(target, attachment, texture, level);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTextureFaceARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    AROS_LHA(GLenum, face, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglFramebufferTextureFaceARB(target, attachment, texture, level, face);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindTransformFeedback,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindTransformFeedback(target, id);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteTransformFeedbacks,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, ids, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDeleteTransformFeedbacks(n, ids);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenTransformFeedbacks,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, ids, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGenTransformFeedbacks(n, ids);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsTransformFeedback,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    GLboolean _return = mglIsTransformFeedback(id);

    RESTORE_REG

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPauseTransformFeedback,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglPauseTransformFeedback();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glResumeTransformFeedback,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglResumeTransformFeedback();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawTransformFeedback,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawTransformFeedback(mode, id);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glDrawArraysInstancedEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, start, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawArraysInstancedEXT(mode, start, count, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawElementsInstancedEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglDrawElementsInstancedEXT(mode, count, type, indices, primcount);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBeginTransformFeedbackEXT,
    AROS_LHA(GLenum, primitiveMode, D0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBeginTransformFeedbackEXT(primitiveMode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndTransformFeedbackEXT,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEndTransformFeedbackEXT();

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glBindBufferRangeEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    AROS_LHA(GLintptr, offset, D3),
    AROS_LHA(GLsizeiptr, size, D4),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindBufferRangeEXT(target, index, buffer, offset, size);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBindBufferOffsetEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    AROS_LHA(GLintptr, offset, D3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindBufferOffsetEXT(target, index, buffer, offset);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindBufferBaseEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglBindBufferBaseEXT(target, index, buffer);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTransformFeedbackVaryingsEXT,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLchar *  *, varyings, A0),
    AROS_LHA(GLenum, bufferMode, D2),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglTransformFeedbackVaryingsEXT(program, count, varyings, bufferMode);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glGetTransformFeedbackVaryingEXT,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLsizei *, size, A1),
    AROS_LHA(GLenum *, type, A2),
    AROS_LHA(GLchar *, name, A3),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglGetTransformFeedbackVaryingEXT(program, index, bufSize, length, size, type, name);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEGLImageTargetTexture2DOES,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLeglImageOES, image, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEGLImageTargetTexture2DOES(target, image);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEGLImageTargetRenderbufferStorageOES,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLeglImageOES, image, D1),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG

    PUT_MESABASE_IN_REG

    mglEGLImageTargetRenderbufferStorageOES(target, image);

    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

