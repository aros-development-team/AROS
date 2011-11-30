/*
    Copyright 2009-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glext.h>
#include "arosmesaapim.h"
#include "arosmesa_intern.h"


AROS_LH1(void, glClearIndex,
    AROS_LHA(GLfloat, c, D0),
    struct Library *, MesaBase, 35, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearIndex(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glClearColor,
    AROS_LHA(GLclampf, red, D0),
    AROS_LHA(GLclampf, green, D1),
    AROS_LHA(GLclampf, blue, D2),
    AROS_LHA(GLclampf, alpha, D3),
    struct Library *, MesaBase, 36, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearColor(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClear,
    AROS_LHA(GLbitfield, mask, D0),
    struct Library *, MesaBase, 37, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClear(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexMask,
    AROS_LHA(GLuint, mask, D0),
    struct Library *, MesaBase, 38, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexMask(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColorMask,
    AROS_LHA(GLboolean, red, D0),
    AROS_LHA(GLboolean, green, D1),
    AROS_LHA(GLboolean, blue, D2),
    AROS_LHA(GLboolean, alpha, D3),
    struct Library *, MesaBase, 39, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorMask(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAlphaFunc,
    AROS_LHA(GLenum, func, D0),
    AROS_LHA(GLclampf, ref, D1),
    struct Library *, MesaBase, 40, Mesa)
{
    AROS_LIBFUNC_INIT

    mglAlphaFunc(func, ref);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendFunc,
    AROS_LHA(GLenum, sfactor, D0),
    AROS_LHA(GLenum, dfactor, D1),
    struct Library *, MesaBase, 41, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendFunc(sfactor, dfactor);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLogicOp,
    AROS_LHA(GLenum, opcode, D0),
    struct Library *, MesaBase, 42, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLogicOp(opcode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCullFace,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 43, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCullFace(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFrontFace,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 44, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFrontFace(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPointSize,
    AROS_LHA(GLfloat, size, D0),
    struct Library *, MesaBase, 45, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointSize(size);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLineWidth,
    AROS_LHA(GLfloat, width, D0),
    struct Library *, MesaBase, 46, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLineWidth(width);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLineStipple,
    AROS_LHA(GLint, factor, D0),
    AROS_LHA(GLushort, pattern, D1),
    struct Library *, MesaBase, 47, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLineStipple(factor, pattern);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPolygonMode,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 48, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPolygonMode(face, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPolygonOffset,
    AROS_LHA(GLfloat, factor, D0),
    AROS_LHA(GLfloat, units, D1),
    struct Library *, MesaBase, 49, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPolygonOffset(factor, units);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPolygonStipple,
    AROS_LHA(const GLubyte *, mask, A0),
    struct Library *, MesaBase, 50, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPolygonStipple(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glGetPolygonStipple,
    AROS_LHA(GLubyte *, mask, A0),
    struct Library *, MesaBase, 51, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetPolygonStipple(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEdgeFlag,
    AROS_LHA(GLboolean, flag, D0),
    struct Library *, MesaBase, 52, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEdgeFlag(flag);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEdgeFlagv,
    AROS_LHA(const GLboolean *, flag, A0),
    struct Library *, MesaBase, 53, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEdgeFlagv(flag);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glScissor,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    struct Library *, MesaBase, 54, Mesa)
{
    AROS_LIBFUNC_INIT

    mglScissor(x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glClipPlane,
    AROS_LHA(GLenum, plane, D0),
    AROS_LHA(const GLdouble *, equation, A0),
    struct Library *, MesaBase, 55, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClipPlane(plane, equation);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetClipPlane,
    AROS_LHA(GLenum, plane, D0),
    AROS_LHA(GLdouble *, equation, A0),
    struct Library *, MesaBase, 56, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetClipPlane(plane, equation);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDrawBuffer,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 57, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawBuffer(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glReadBuffer,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 58, Mesa)
{
    AROS_LIBFUNC_INIT

    mglReadBuffer(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnable,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 59, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEnable(cap);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisable,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 60, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDisable(cap);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsEnabled,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 61, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsEnabled(cap);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnableClientState,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 62, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEnableClientState(cap);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisableClientState,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 63, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDisableClientState(cap);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetBooleanv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLboolean *, params, A0),
    struct Library *, MesaBase, 64, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBooleanv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetDoublev,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 65, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetDoublev(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetFloatv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 66, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetFloatv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetIntegerv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 67, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetIntegerv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPushAttrib,
    AROS_LHA(GLbitfield, mask, D0),
    struct Library *, MesaBase, 68, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPushAttrib(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopAttrib,
    struct Library *, MesaBase, 69, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPopAttrib();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPushClientAttrib,
    AROS_LHA(GLbitfield, mask, D0),
    struct Library *, MesaBase, 70, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPushClientAttrib(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopClientAttrib,
    struct Library *, MesaBase, 71, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPopClientAttrib();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLint, glRenderMode,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 72, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = mglRenderMode(mode);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLenum, glGetError,
    struct Library *, MesaBase, 73, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = mglGetError();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(const GLubyte *, glGetString,
    AROS_LHA(GLenum, name, D0),
    struct Library *, MesaBase, 74, Mesa)
{
    AROS_LIBFUNC_INIT

    const GLubyte * _return = mglGetString(name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glFinish,
    struct Library *, MesaBase, 75, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFinish();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glFlush,
    struct Library *, MesaBase, 76, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFlush();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glHint,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 77, Mesa)
{
    AROS_LIBFUNC_INIT

    mglHint(target, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClearDepth,
    AROS_LHA(GLclampd, depth, D0),
    struct Library *, MesaBase, 78, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearDepth(depth);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDepthFunc,
    AROS_LHA(GLenum, func, D0),
    struct Library *, MesaBase, 79, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDepthFunc(func);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDepthMask,
    AROS_LHA(GLboolean, flag, D0),
    struct Library *, MesaBase, 80, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDepthMask(flag);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDepthRange,
    AROS_LHA(GLclampd, near_val, D0),
    AROS_LHA(GLclampd, far_val, D1),
    struct Library *, MesaBase, 81, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDepthRange(near_val, far_val);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glClearAccum,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    AROS_LHA(GLfloat, alpha, D3),
    struct Library *, MesaBase, 82, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearAccum(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAccum,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLfloat, value, D1),
    struct Library *, MesaBase, 83, Mesa)
{
    AROS_LIBFUNC_INIT

    mglAccum(op, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMatrixMode,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 84, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMatrixMode(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glOrtho,
    AROS_LHA(GLdouble, left, D0),
    AROS_LHA(GLdouble, right, D1),
    AROS_LHA(GLdouble, bottom, D2),
    AROS_LHA(GLdouble, top, D3),
    AROS_LHA(GLdouble, near_val, D4),
    AROS_LHA(GLdouble, far_val, D5),
    struct Library *, MesaBase, 85, Mesa)
{
    AROS_LIBFUNC_INIT

    mglOrtho(left, right, bottom, top, near_val, far_val);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glFrustum,
    AROS_LHA(GLdouble, left, D0),
    AROS_LHA(GLdouble, right, D1),
    AROS_LHA(GLdouble, bottom, D2),
    AROS_LHA(GLdouble, top, D3),
    AROS_LHA(GLdouble, near_val, D4),
    AROS_LHA(GLdouble, far_val, D5),
    struct Library *, MesaBase, 86, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFrustum(left, right, bottom, top, near_val, far_val);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glViewport,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    struct Library *, MesaBase, 87, Mesa)
{
    AROS_LIBFUNC_INIT

    mglViewport(x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPushMatrix,
    struct Library *, MesaBase, 88, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPushMatrix();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopMatrix,
    struct Library *, MesaBase, 89, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPopMatrix();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glLoadIdentity,
    struct Library *, MesaBase, 90, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLoadIdentity();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 91, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLoadMatrixd(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 92, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLoadMatrixf(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 93, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultMatrixd(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 94, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultMatrixf(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRotated,
    AROS_LHA(GLdouble, angle, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    struct Library *, MesaBase, 95, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRotated(angle, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRotatef,
    AROS_LHA(GLfloat, angle, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    struct Library *, MesaBase, 96, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRotatef(angle, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glScaled,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 97, Mesa)
{
    AROS_LIBFUNC_INIT

    mglScaled(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glScalef,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 98, Mesa)
{
    AROS_LIBFUNC_INIT

    mglScalef(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTranslated,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 99, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTranslated(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTranslatef,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 100, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTranslatef(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsList,
    AROS_LHA(GLuint, list, D0),
    struct Library *, MesaBase, 101, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsList(list);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteLists,
    AROS_LHA(GLuint, list, D0),
    AROS_LHA(GLsizei, range, D1),
    struct Library *, MesaBase, 102, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteLists(list, range);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLuint, glGenLists,
    AROS_LHA(GLsizei, range, D0),
    struct Library *, MesaBase, 103, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = mglGenLists(range);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glNewList,
    AROS_LHA(GLuint, list, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 104, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNewList(list, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndList,
    struct Library *, MesaBase, 105, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEndList();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCallList,
    AROS_LHA(GLuint, list, D0),
    struct Library *, MesaBase, 106, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCallList(list);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glCallLists,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(const GLvoid *, lists, A0),
    struct Library *, MesaBase, 107, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCallLists(n, type, lists);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glListBase,
    AROS_LHA(GLuint, base, D0),
    struct Library *, MesaBase, 108, Mesa)
{
    AROS_LIBFUNC_INIT

    mglListBase(base);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBegin,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 109, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBegin(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEnd,
    struct Library *, MesaBase, 110, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEnd();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 111, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex2d(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 112, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex2f(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 113, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex2i(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 114, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex2s(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 115, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex3d(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 116, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex3f(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 117, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex3i(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 118, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex3s(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertex4d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    AROS_LHA(GLdouble, w, D3),
    struct Library *, MesaBase, 119, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex4d(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertex4f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    AROS_LHA(GLfloat, w, D3),
    struct Library *, MesaBase, 120, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex4f(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertex4i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    AROS_LHA(GLint, w, D3),
    struct Library *, MesaBase, 121, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex4i(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertex4s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    AROS_LHA(GLshort, w, D3),
    struct Library *, MesaBase, 122, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex4s(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 123, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex2dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 124, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex2fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 125, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex2iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 126, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex2sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 127, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 128, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 129, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 130, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 131, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex4dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 132, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex4fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 133, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex4iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 134, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertex4sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3b,
    AROS_LHA(GLbyte, nx, D0),
    AROS_LHA(GLbyte, ny, D1),
    AROS_LHA(GLbyte, nz, D2),
    struct Library *, MesaBase, 135, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3b(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3d,
    AROS_LHA(GLdouble, nx, D0),
    AROS_LHA(GLdouble, ny, D1),
    AROS_LHA(GLdouble, nz, D2),
    struct Library *, MesaBase, 136, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3d(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3f,
    AROS_LHA(GLfloat, nx, D0),
    AROS_LHA(GLfloat, ny, D1),
    AROS_LHA(GLfloat, nz, D2),
    struct Library *, MesaBase, 137, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3f(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3i,
    AROS_LHA(GLint, nx, D0),
    AROS_LHA(GLint, ny, D1),
    AROS_LHA(GLint, nz, D2),
    struct Library *, MesaBase, 138, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3i(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3s,
    AROS_LHA(GLshort, nx, D0),
    AROS_LHA(GLshort, ny, D1),
    AROS_LHA(GLshort, nz, D2),
    struct Library *, MesaBase, 139, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3s(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 140, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3bv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 141, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 142, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 143, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 144, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormal3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexd,
    AROS_LHA(GLdouble, c, D0),
    struct Library *, MesaBase, 145, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexd(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexf,
    AROS_LHA(GLfloat, c, D0),
    struct Library *, MesaBase, 146, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexf(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexi,
    AROS_LHA(GLint, c, D0),
    struct Library *, MesaBase, 147, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexi(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexs,
    AROS_LHA(GLshort, c, D0),
    struct Library *, MesaBase, 148, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexs(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexub,
    AROS_LHA(GLubyte, c, D0),
    struct Library *, MesaBase, 149, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexub(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexdv,
    AROS_LHA(const GLdouble *, c, A0),
    struct Library *, MesaBase, 150, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexdv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexfv,
    AROS_LHA(const GLfloat *, c, A0),
    struct Library *, MesaBase, 151, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexfv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexiv,
    AROS_LHA(const GLint *, c, A0),
    struct Library *, MesaBase, 152, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexiv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexsv,
    AROS_LHA(const GLshort *, c, A0),
    struct Library *, MesaBase, 153, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexsv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexubv,
    AROS_LHA(const GLubyte *, c, A0),
    struct Library *, MesaBase, 154, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexubv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3b,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    struct Library *, MesaBase, 155, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3b(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3d,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    struct Library *, MesaBase, 156, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3d(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3f,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    struct Library *, MesaBase, 157, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3f(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3i,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    struct Library *, MesaBase, 158, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3i(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3s,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    struct Library *, MesaBase, 159, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3s(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3ub,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    struct Library *, MesaBase, 160, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3ub(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3ui,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    struct Library *, MesaBase, 161, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3ui(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3us,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    struct Library *, MesaBase, 162, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3us(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4b,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    AROS_LHA(GLbyte, alpha, D3),
    struct Library *, MesaBase, 163, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4b(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4d,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    AROS_LHA(GLdouble, alpha, D3),
    struct Library *, MesaBase, 164, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4d(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4f,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    AROS_LHA(GLfloat, alpha, D3),
    struct Library *, MesaBase, 165, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4f(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4i,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    AROS_LHA(GLint, alpha, D3),
    struct Library *, MesaBase, 166, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4i(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4s,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    AROS_LHA(GLshort, alpha, D3),
    struct Library *, MesaBase, 167, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4s(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4ub,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    AROS_LHA(GLubyte, alpha, D3),
    struct Library *, MesaBase, 168, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4ub(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4ui,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    AROS_LHA(GLuint, alpha, D3),
    struct Library *, MesaBase, 169, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4ui(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColor4us,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    AROS_LHA(GLushort, alpha, D3),
    struct Library *, MesaBase, 170, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4us(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 171, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3bv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 172, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 173, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 174, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 175, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3ubv,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 176, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3ubv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3uiv,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 177, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3uiv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3usv,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 178, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor3usv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 179, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4bv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 180, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 181, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 182, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 183, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4ubv,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 184, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4ubv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4uiv,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 185, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4uiv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4usv,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 186, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColor4usv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1d,
    AROS_LHA(GLdouble, s, D0),
    struct Library *, MesaBase, 187, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord1d(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1f,
    AROS_LHA(GLfloat, s, D0),
    struct Library *, MesaBase, 188, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord1f(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1i,
    AROS_LHA(GLint, s, D0),
    struct Library *, MesaBase, 189, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord1i(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1s,
    AROS_LHA(GLshort, s, D0),
    struct Library *, MesaBase, 190, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord1s(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2d,
    AROS_LHA(GLdouble, s, D0),
    AROS_LHA(GLdouble, t, D1),
    struct Library *, MesaBase, 191, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord2d(s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2f,
    AROS_LHA(GLfloat, s, D0),
    AROS_LHA(GLfloat, t, D1),
    struct Library *, MesaBase, 192, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord2f(s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2i,
    AROS_LHA(GLint, s, D0),
    AROS_LHA(GLint, t, D1),
    struct Library *, MesaBase, 193, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord2i(s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2s,
    AROS_LHA(GLshort, s, D0),
    AROS_LHA(GLshort, t, D1),
    struct Library *, MesaBase, 194, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord2s(s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3d,
    AROS_LHA(GLdouble, s, D0),
    AROS_LHA(GLdouble, t, D1),
    AROS_LHA(GLdouble, r, D2),
    struct Library *, MesaBase, 195, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord3d(s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3f,
    AROS_LHA(GLfloat, s, D0),
    AROS_LHA(GLfloat, t, D1),
    AROS_LHA(GLfloat, r, D2),
    struct Library *, MesaBase, 196, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord3f(s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3i,
    AROS_LHA(GLint, s, D0),
    AROS_LHA(GLint, t, D1),
    AROS_LHA(GLint, r, D2),
    struct Library *, MesaBase, 197, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord3i(s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3s,
    AROS_LHA(GLshort, s, D0),
    AROS_LHA(GLshort, t, D1),
    AROS_LHA(GLshort, r, D2),
    struct Library *, MesaBase, 198, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord3s(s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoord4d,
    AROS_LHA(GLdouble, s, D0),
    AROS_LHA(GLdouble, t, D1),
    AROS_LHA(GLdouble, r, D2),
    AROS_LHA(GLdouble, q, D3),
    struct Library *, MesaBase, 199, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord4d(s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoord4f,
    AROS_LHA(GLfloat, s, D0),
    AROS_LHA(GLfloat, t, D1),
    AROS_LHA(GLfloat, r, D2),
    AROS_LHA(GLfloat, q, D3),
    struct Library *, MesaBase, 200, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord4f(s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoord4i,
    AROS_LHA(GLint, s, D0),
    AROS_LHA(GLint, t, D1),
    AROS_LHA(GLint, r, D2),
    AROS_LHA(GLint, q, D3),
    struct Library *, MesaBase, 201, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord4i(s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoord4s,
    AROS_LHA(GLshort, s, D0),
    AROS_LHA(GLshort, t, D1),
    AROS_LHA(GLshort, r, D2),
    AROS_LHA(GLshort, q, D3),
    struct Library *, MesaBase, 202, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord4s(s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 203, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord1dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 204, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord1fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 205, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord1iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 206, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord1sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 207, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord2dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 208, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord2fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 209, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord2iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 210, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord2sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 211, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 212, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 213, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 214, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 215, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord4dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 216, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord4fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 217, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord4iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 218, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoord4sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 219, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos2d(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 220, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos2f(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 221, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos2i(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 222, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos2s(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 223, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos3d(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 224, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos3f(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 225, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos3i(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 226, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos3s(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRasterPos4d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    AROS_LHA(GLdouble, w, D3),
    struct Library *, MesaBase, 227, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos4d(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRasterPos4f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    AROS_LHA(GLfloat, w, D3),
    struct Library *, MesaBase, 228, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos4f(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRasterPos4i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    AROS_LHA(GLint, w, D3),
    struct Library *, MesaBase, 229, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos4i(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRasterPos4s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    AROS_LHA(GLshort, w, D3),
    struct Library *, MesaBase, 230, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos4s(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 231, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos2dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 232, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos2fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 233, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos2iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 234, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos2sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 235, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 236, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 237, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 238, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 239, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos4dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 240, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos4fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 241, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos4iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 242, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRasterPos4sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRectd,
    AROS_LHA(GLdouble, x1, D0),
    AROS_LHA(GLdouble, y1, D1),
    AROS_LHA(GLdouble, x2, D2),
    AROS_LHA(GLdouble, y2, D3),
    struct Library *, MesaBase, 243, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRectd(x1, y1, x2, y2);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRectf,
    AROS_LHA(GLfloat, x1, D0),
    AROS_LHA(GLfloat, y1, D1),
    AROS_LHA(GLfloat, x2, D2),
    AROS_LHA(GLfloat, y2, D3),
    struct Library *, MesaBase, 244, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRectf(x1, y1, x2, y2);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRecti,
    AROS_LHA(GLint, x1, D0),
    AROS_LHA(GLint, y1, D1),
    AROS_LHA(GLint, x2, D2),
    AROS_LHA(GLint, y2, D3),
    struct Library *, MesaBase, 245, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRecti(x1, y1, x2, y2);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRects,
    AROS_LHA(GLshort, x1, D0),
    AROS_LHA(GLshort, y1, D1),
    AROS_LHA(GLshort, x2, D2),
    AROS_LHA(GLshort, y2, D3),
    struct Library *, MesaBase, 246, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRects(x1, y1, x2, y2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectdv,
    AROS_LHA(const GLdouble *, v1, A0),
    AROS_LHA(const GLdouble *, v2, A1),
    struct Library *, MesaBase, 247, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRectdv(v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectfv,
    AROS_LHA(const GLfloat *, v1, A0),
    AROS_LHA(const GLfloat *, v2, A1),
    struct Library *, MesaBase, 248, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRectfv(v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectiv,
    AROS_LHA(const GLint *, v1, A0),
    AROS_LHA(const GLint *, v2, A1),
    struct Library *, MesaBase, 249, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRectiv(v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectsv,
    AROS_LHA(const GLshort *, v1, A0),
    AROS_LHA(const GLshort *, v2, A1),
    struct Library *, MesaBase, 250, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRectsv(v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexPointer,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 251, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexPointer(size, type, stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormalPointer,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 252, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormalPointer(type, stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glColorPointer,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 253, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorPointer(size, type, stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glIndexPointer,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 254, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexPointer(type, stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTexCoordPointer,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 255, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoordPointer(size, type, stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEdgeFlagPointer,
    AROS_LHA(GLsizei, stride, D0),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 256, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEdgeFlagPointer(stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPointerv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLvoid * *, params, A0),
    struct Library *, MesaBase, 257, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetPointerv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glArrayElement,
    AROS_LHA(GLint, i, D0),
    struct Library *, MesaBase, 258, Mesa)
{
    AROS_LIBFUNC_INIT

    mglArrayElement(i);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glDrawArrays,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    struct Library *, MesaBase, 259, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawArrays(mode, first, count);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glDrawElements,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    struct Library *, MesaBase, 260, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawElements(mode, count, type, indices);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glInterleavedArrays,
    AROS_LHA(GLenum, format, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 261, Mesa)
{
    AROS_LIBFUNC_INIT

    mglInterleavedArrays(format, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glShadeModel,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 262, Mesa)
{
    AROS_LIBFUNC_INIT

    mglShadeModel(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLightf,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 263, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLightf(light, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLighti,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 264, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLighti(light, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLightfv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 265, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLightfv(light, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLightiv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 266, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLightiv(light, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetLightfv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 267, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetLightfv(light, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetLightiv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 268, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetLightiv(light, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModelf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 269, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLightModelf(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModeli,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 270, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLightModeli(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModelfv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 271, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLightModelfv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModeliv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 272, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLightModeliv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMaterialf,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 273, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMaterialf(face, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMateriali,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 274, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMateriali(face, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMaterialfv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 275, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMaterialfv(face, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMaterialiv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 276, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMaterialiv(face, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMaterialfv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 277, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMaterialfv(face, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMaterialiv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 278, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMaterialiv(face, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glColorMaterial,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 279, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorMaterial(face, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelZoom,
    AROS_LHA(GLfloat, xfactor, D0),
    AROS_LHA(GLfloat, yfactor, D1),
    struct Library *, MesaBase, 280, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelZoom(xfactor, yfactor);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelStoref,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 281, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelStoref(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelStorei,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 282, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelStorei(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTransferf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 283, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelTransferf(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTransferi,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 284, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelTransferi(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPixelMapfv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, mapsize, D1),
    AROS_LHA(const GLfloat *, values, A0),
    struct Library *, MesaBase, 285, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelMapfv(map, mapsize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPixelMapuiv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, mapsize, D1),
    AROS_LHA(const GLuint *, values, A0),
    struct Library *, MesaBase, 286, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelMapuiv(map, mapsize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPixelMapusv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, mapsize, D1),
    AROS_LHA(const GLushort *, values, A0),
    struct Library *, MesaBase, 287, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelMapusv(map, mapsize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelMapfv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLfloat *, values, A0),
    struct Library *, MesaBase, 288, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetPixelMapfv(map, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelMapuiv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLuint *, values, A0),
    struct Library *, MesaBase, 289, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetPixelMapuiv(map, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelMapusv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLushort *, values, A0),
    struct Library *, MesaBase, 290, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetPixelMapusv(map, values);

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
    struct Library *, MesaBase, 291, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);

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
    struct Library *, MesaBase, 292, Mesa)
{
    AROS_LIBFUNC_INIT

    mglReadPixels(x, y, width, height, format, type, pixels);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawPixels,
    AROS_LHA(GLsizei, width, D0),
    AROS_LHA(GLsizei, height, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(const GLvoid *, pixels, A0),
    struct Library *, MesaBase, 293, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawPixels(width, height, format, type, pixels);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyPixels,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, type, D4),
    struct Library *, MesaBase, 294, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyPixels(x, y, width, height, type);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glStencilFunc,
    AROS_LHA(GLenum, func, D0),
    AROS_LHA(GLint, ref, D1),
    AROS_LHA(GLuint, mask, D2),
    struct Library *, MesaBase, 295, Mesa)
{
    AROS_LIBFUNC_INIT

    mglStencilFunc(func, ref, mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glStencilMask,
    AROS_LHA(GLuint, mask, D0),
    struct Library *, MesaBase, 296, Mesa)
{
    AROS_LIBFUNC_INIT

    mglStencilMask(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glStencilOp,
    AROS_LHA(GLenum, fail, D0),
    AROS_LHA(GLenum, zfail, D1),
    AROS_LHA(GLenum, zpass, D2),
    struct Library *, MesaBase, 297, Mesa)
{
    AROS_LIBFUNC_INIT

    mglStencilOp(fail, zfail, zpass);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClearStencil,
    AROS_LHA(GLint, s, D0),
    struct Library *, MesaBase, 298, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearStencil(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGend,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble, param, D2),
    struct Library *, MesaBase, 299, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexGend(coord, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGenf,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 300, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexGenf(coord, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGeni,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 301, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexGeni(coord, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGendv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLdouble *, params, A0),
    struct Library *, MesaBase, 302, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexGendv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGenfv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 303, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexGenfv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGeniv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 304, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexGeniv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexGendv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 305, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexGendv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexGenfv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 306, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexGenfv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexGeniv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 307, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexGeniv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnvf,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 308, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexEnvf(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnvi,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 309, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexEnvi(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnvfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 310, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexEnvfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnviv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 311, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexEnviv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexEnvfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 312, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexEnvfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexEnviv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 313, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexEnviv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterf,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 314, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexParameterf(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameteri,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 315, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexParameteri(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 316, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 317, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 318, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 319, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetTexLevelParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 320, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexLevelParameterfv(target, level, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetTexLevelParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 321, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexLevelParameteriv(target, level, pname, params);

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
    struct Library *, MesaBase, 322, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexImage1D(target, level, internalFormat, width, border, format, type, pixels);

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
    struct Library *, MesaBase, 323, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetTexImage,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(GLvoid *, pixels, A0),
    struct Library *, MesaBase, 324, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexImage(target, level, format, type, pixels);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenTextures,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, textures, A0),
    struct Library *, MesaBase, 325, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenTextures(n, textures);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteTextures,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    struct Library *, MesaBase, 326, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteTextures(n, textures);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindTexture,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, texture, D1),
    struct Library *, MesaBase, 327, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindTexture(target, texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPrioritizeTextures,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(const GLclampf *, priorities, A1),
    struct Library *, MesaBase, 328, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPrioritizeTextures(n, textures, priorities);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLboolean, glAreTexturesResident,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(GLboolean *, residences, A1),
    struct Library *, MesaBase, 329, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglAreTexturesResident(n, textures, residences);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsTexture,
    AROS_LHA(GLuint, texture, D0),
    struct Library *, MesaBase, 330, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsTexture(texture);

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
    struct Library *, MesaBase, 331, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexSubImage1D(target, level, xoffset, width, format, type, pixels);

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
    struct Library *, MesaBase, 332, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);

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
    struct Library *, MesaBase, 333, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexImage1D(target, level, internalformat, x, y, width, border);

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
    struct Library *, MesaBase, 334, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexImage2D(target, level, internalformat, x, y, width, height, border);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glCopyTexSubImage1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, x, D3),
    AROS_LHA(GLint, y, D4),
    AROS_LHA(GLsizei, width, D5),
    struct Library *, MesaBase, 335, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexSubImage1D(target, level, xoffset, x, y, width);

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
    struct Library *, MesaBase, 336, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMap1d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, u1, D1),
    AROS_LHA(GLdouble, u2, D2),
    AROS_LHA(GLint, stride, D3),
    AROS_LHA(GLint, order, D4),
    AROS_LHA(const GLdouble *, points, A0),
    struct Library *, MesaBase, 337, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMap1d(target, u1, u2, stride, order, points);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMap1f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, u1, D1),
    AROS_LHA(GLfloat, u2, D2),
    AROS_LHA(GLint, stride, D3),
    AROS_LHA(GLint, order, D4),
    AROS_LHA(const GLfloat *, points, A0),
    struct Library *, MesaBase, 338, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMap1f(target, u1, u2, stride, order, points);

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
    struct Library *, MesaBase, 339, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);

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
    struct Library *, MesaBase, 340, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMapdv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLdouble *, v, A0),
    struct Library *, MesaBase, 341, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMapdv(target, query, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMapfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLfloat *, v, A0),
    struct Library *, MesaBase, 342, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMapfv(target, query, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMapiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLint *, v, A0),
    struct Library *, MesaBase, 343, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMapiv(target, query, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1d,
    AROS_LHA(GLdouble, u, D0),
    struct Library *, MesaBase, 344, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalCoord1d(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1f,
    AROS_LHA(GLfloat, u, D0),
    struct Library *, MesaBase, 345, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalCoord1f(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1dv,
    AROS_LHA(const GLdouble *, u, A0),
    struct Library *, MesaBase, 346, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalCoord1dv(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1fv,
    AROS_LHA(const GLfloat *, u, A0),
    struct Library *, MesaBase, 347, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalCoord1fv(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEvalCoord2d,
    AROS_LHA(GLdouble, u, D0),
    AROS_LHA(GLdouble, v, D1),
    struct Library *, MesaBase, 348, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalCoord2d(u, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEvalCoord2f,
    AROS_LHA(GLfloat, u, D0),
    AROS_LHA(GLfloat, v, D1),
    struct Library *, MesaBase, 349, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalCoord2f(u, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord2dv,
    AROS_LHA(const GLdouble *, u, A0),
    struct Library *, MesaBase, 350, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalCoord2dv(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord2fv,
    AROS_LHA(const GLfloat *, u, A0),
    struct Library *, MesaBase, 351, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalCoord2fv(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMapGrid1d,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLdouble, u1, D1),
    AROS_LHA(GLdouble, u2, D2),
    struct Library *, MesaBase, 352, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMapGrid1d(un, u1, u2);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMapGrid1f,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLfloat, u1, D1),
    AROS_LHA(GLfloat, u2, D2),
    struct Library *, MesaBase, 353, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMapGrid1f(un, u1, u2);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMapGrid2d,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLdouble, u1, D1),
    AROS_LHA(GLdouble, u2, D2),
    AROS_LHA(GLint, vn, D3),
    AROS_LHA(GLdouble, v1, D4),
    AROS_LHA(GLdouble, v2, D5),
    struct Library *, MesaBase, 354, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMapGrid2d(un, u1, u2, vn, v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMapGrid2f,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLfloat, u1, D1),
    AROS_LHA(GLfloat, u2, D2),
    AROS_LHA(GLint, vn, D3),
    AROS_LHA(GLfloat, v1, D4),
    AROS_LHA(GLfloat, v2, D5),
    struct Library *, MesaBase, 355, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMapGrid2f(un, u1, u2, vn, v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalPoint1,
    AROS_LHA(GLint, i, D0),
    struct Library *, MesaBase, 356, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalPoint1(i);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEvalPoint2,
    AROS_LHA(GLint, i, D0),
    AROS_LHA(GLint, j, D1),
    struct Library *, MesaBase, 357, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalPoint2(i, j);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glEvalMesh1,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, i1, D1),
    AROS_LHA(GLint, i2, D2),
    struct Library *, MesaBase, 358, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalMesh1(mode, i1, i2);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glEvalMesh2,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, i1, D1),
    AROS_LHA(GLint, i2, D2),
    AROS_LHA(GLint, j1, D3),
    AROS_LHA(GLint, j2, D4),
    struct Library *, MesaBase, 359, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEvalMesh2(mode, i1, i2, j1, j2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 360, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogf(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogi,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 361, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogi(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogfv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 362, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogfv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogiv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 363, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogiv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFeedbackBuffer,
    AROS_LHA(GLsizei, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLfloat *, buffer, A0),
    struct Library *, MesaBase, 364, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFeedbackBuffer(size, type, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPassThrough,
    AROS_LHA(GLfloat, token, D0),
    struct Library *, MesaBase, 365, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPassThrough(token);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSelectBuffer,
    AROS_LHA(GLsizei, size, D0),
    AROS_LHA(GLuint *, buffer, A0),
    struct Library *, MesaBase, 366, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSelectBuffer(size, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glInitNames,
    struct Library *, MesaBase, 367, Mesa)
{
    AROS_LIBFUNC_INIT

    mglInitNames();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadName,
    AROS_LHA(GLuint, name, D0),
    struct Library *, MesaBase, 368, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLoadName(name);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPushName,
    AROS_LHA(GLuint, name, D0),
    struct Library *, MesaBase, 369, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPushName(name);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopName,
    struct Library *, MesaBase, 370, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPopName();

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glDrawRangeElements,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLuint, start, D1),
    AROS_LHA(GLuint, end, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, indices, A0),
    struct Library *, MesaBase, 371, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawRangeElements(mode, start, end, count, type, indices);

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
    struct Library *, MesaBase, 372, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);

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
    struct Library *, MesaBase, 373, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);

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
    struct Library *, MesaBase, 374, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glColorTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, table, A0),
    struct Library *, MesaBase, 375, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorTable(target, internalformat, width, format, type, table);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glColorSubTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, start, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 376, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorSubTable(target, start, count, format, type, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 377, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorTableParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 378, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorTableParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyColorSubTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, start, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    struct Library *, MesaBase, 379, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyColorSubTable(target, start, x, y, width);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyColorTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    struct Library *, MesaBase, 380, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyColorTable(target, internalformat, x, y, width);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetColorTable,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, table, A0),
    struct Library *, MesaBase, 381, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetColorTable(target, format, type, table);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 382, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetColorTableParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 383, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetColorTableParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBlendEquation,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 384, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendEquation(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBlendColor,
    AROS_LHA(GLclampf, red, D0),
    AROS_LHA(GLclampf, green, D1),
    AROS_LHA(GLclampf, blue, D2),
    AROS_LHA(GLclampf, alpha, D3),
    struct Library *, MesaBase, 385, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendColor(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glHistogram,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, width, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLboolean, sink, D3),
    struct Library *, MesaBase, 386, Mesa)
{
    AROS_LIBFUNC_INIT

    mglHistogram(target, width, internalformat, sink);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetHistogram,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 387, Mesa)
{
    AROS_LIBFUNC_INIT

    mglResetHistogram(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetHistogram,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLboolean, reset, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(GLvoid *, values, A0),
    struct Library *, MesaBase, 388, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetHistogram(target, reset, format, type, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 389, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetHistogramParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 390, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetHistogramParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMinmax,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLboolean, sink, D2),
    struct Library *, MesaBase, 391, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMinmax(target, internalformat, sink);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetMinmax,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 392, Mesa)
{
    AROS_LIBFUNC_INIT

    mglResetMinmax(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetMinmax,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLboolean, reset, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, types, D3),
    AROS_LHA(GLvoid *, values, A0),
    struct Library *, MesaBase, 393, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMinmax(target, reset, format, types, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 394, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMinmaxParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 395, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMinmaxParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glConvolutionFilter1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, image, A0),
    struct Library *, MesaBase, 396, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionFilter1D(target, internalformat, width, format, type, image);

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
    struct Library *, MesaBase, 397, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionFilter2D(target, internalformat, width, height, format, type, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterf,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, params, D2),
    struct Library *, MesaBase, 398, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionParameterf(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 399, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameteri,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, params, D2),
    struct Library *, MesaBase, 400, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionParameteri(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 401, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyConvolutionFilter1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    struct Library *, MesaBase, 402, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyConvolutionFilter1D(target, internalformat, x, y, width);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glCopyConvolutionFilter2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    AROS_LHA(GLsizei, height, D5),
    struct Library *, MesaBase, 403, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyConvolutionFilter2D(target, internalformat, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetConvolutionFilter,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, image, A0),
    struct Library *, MesaBase, 404, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetConvolutionFilter(target, format, type, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 405, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetConvolutionParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 406, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetConvolutionParameteriv(target, pname, params);

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
    struct Library *, MesaBase, 407, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSeparableFilter2D(target, internalformat, width, height, format, type, row, column);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glGetSeparableFilter,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, row, A0),
    AROS_LHA(GLvoid *, column, A1),
    AROS_LHA(GLvoid *, span, A2),
    struct Library *, MesaBase, 408, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetSeparableFilter(target, format, type, row, column, span);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveTexture,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 409, Mesa)
{
    AROS_LIBFUNC_INIT

    mglActiveTexture(texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClientActiveTexture,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 410, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClientActiveTexture(texture);

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
    struct Library *, MesaBase, 411, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);

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
    struct Library *, MesaBase, 412, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);

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
    struct Library *, MesaBase, 413, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);

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
    struct Library *, MesaBase, 414, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);

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
    struct Library *, MesaBase, 415, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);

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
    struct Library *, MesaBase, 416, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetCompressedTexImage,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, lod, D1),
    AROS_LHA(GLvoid *, img, A0),
    struct Library *, MesaBase, 417, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetCompressedTexImage(target, lod, img);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    struct Library *, MesaBase, 418, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1d(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 419, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1dv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    struct Library *, MesaBase, 420, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1f(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 421, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1fv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    struct Library *, MesaBase, 422, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1i(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 423, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1iv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    struct Library *, MesaBase, 424, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1s(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 425, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1sv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    struct Library *, MesaBase, 426, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2d(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 427, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2dv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    struct Library *, MesaBase, 428, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2f(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 429, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2fv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    struct Library *, MesaBase, 430, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2i(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 431, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2iv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    struct Library *, MesaBase, 432, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2s(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 433, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2sv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    AROS_LHA(GLdouble, r, D3),
    struct Library *, MesaBase, 434, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3d(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 435, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3dv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    AROS_LHA(GLfloat, r, D3),
    struct Library *, MesaBase, 436, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3f(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 437, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3fv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    AROS_LHA(GLint, r, D3),
    struct Library *, MesaBase, 438, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3i(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 439, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3iv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    AROS_LHA(GLshort, r, D3),
    struct Library *, MesaBase, 440, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3s(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 441, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3sv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    AROS_LHA(GLdouble, r, D3),
    AROS_LHA(GLdouble, q, D4),
    struct Library *, MesaBase, 442, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4d(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 443, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4dv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    AROS_LHA(GLfloat, r, D3),
    AROS_LHA(GLfloat, q, D4),
    struct Library *, MesaBase, 444, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4f(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 445, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4fv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    AROS_LHA(GLint, r, D3),
    AROS_LHA(GLint, q, D4),
    struct Library *, MesaBase, 446, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4i(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 447, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4iv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    AROS_LHA(GLshort, r, D3),
    AROS_LHA(GLshort, q, D4),
    struct Library *, MesaBase, 448, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4s(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 449, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4sv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 450, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLoadTransposeMatrixd(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 451, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLoadTransposeMatrixf(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 452, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultTransposeMatrixd(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 453, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultTransposeMatrixf(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleCoverage,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 454, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSampleCoverage(value, invert);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveTextureARB,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 455, Mesa)
{
    AROS_LIBFUNC_INIT

    mglActiveTextureARB(texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClientActiveTextureARB,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 456, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClientActiveTextureARB(texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    struct Library *, MesaBase, 457, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1dARB(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 458, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1dvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    struct Library *, MesaBase, 459, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1fARB(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 460, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1fvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    struct Library *, MesaBase, 461, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1iARB(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 462, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1ivARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    struct Library *, MesaBase, 463, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1sARB(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 464, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord1svARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    struct Library *, MesaBase, 465, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2dARB(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 466, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2dvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    struct Library *, MesaBase, 467, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2fARB(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 468, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2fvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    struct Library *, MesaBase, 469, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2iARB(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 470, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2ivARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    struct Library *, MesaBase, 471, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2sARB(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 472, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord2svARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    AROS_LHA(GLdouble, r, D3),
    struct Library *, MesaBase, 473, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3dARB(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 474, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3dvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    AROS_LHA(GLfloat, r, D3),
    struct Library *, MesaBase, 475, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3fARB(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 476, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3fvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    AROS_LHA(GLint, r, D3),
    struct Library *, MesaBase, 477, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3iARB(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 478, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3ivARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiTexCoord3sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    AROS_LHA(GLshort, r, D3),
    struct Library *, MesaBase, 479, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3sARB(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 480, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord3svARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    AROS_LHA(GLdouble, r, D3),
    AROS_LHA(GLdouble, q, D4),
    struct Library *, MesaBase, 481, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4dARB(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 482, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4dvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    AROS_LHA(GLfloat, r, D3),
    AROS_LHA(GLfloat, q, D4),
    struct Library *, MesaBase, 483, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4fARB(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 484, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4fvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    AROS_LHA(GLint, r, D3),
    AROS_LHA(GLint, q, D4),
    struct Library *, MesaBase, 485, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4iARB(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 486, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4ivARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiTexCoord4sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    AROS_LHA(GLshort, r, D3),
    AROS_LHA(GLshort, q, D4),
    struct Library *, MesaBase, 487, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4sARB(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 488, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiTexCoord4svARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBlendFuncSeparate,
    AROS_LHA(GLenum, sfactorRGB, D0),
    AROS_LHA(GLenum, dfactorRGB, D1),
    AROS_LHA(GLenum, sfactorAlpha, D2),
    AROS_LHA(GLenum, dfactorAlpha, D3),
    struct Library *, MesaBase, 489, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordf,
    AROS_LHA(GLfloat, coord, D0),
    struct Library *, MesaBase, 490, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoordf(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordfv,
    AROS_LHA(const GLfloat *, coord, A0),
    struct Library *, MesaBase, 491, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoordfv(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordd,
    AROS_LHA(GLdouble, coord, D0),
    struct Library *, MesaBase, 492, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoordd(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoorddv,
    AROS_LHA(const GLdouble *, coord, A0),
    struct Library *, MesaBase, 493, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoorddv(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFogCoordPointer,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 494, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoordPointer(type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiDrawArrays,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLint *, first, A0),
    AROS_LHA(const GLsizei *, count, A1),
    AROS_LHA(GLsizei, primcount, D1),
    struct Library *, MesaBase, 495, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiDrawArrays(mode, first, count, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiDrawElements,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLsizei *, count, A0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(const GLvoid *  *, indices, A1),
    AROS_LHA(GLsizei, primcount, D2),
    struct Library *, MesaBase, 496, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiDrawElements(mode, count, type, indices, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 497, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameterf(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 498, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameterfv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameteri,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 499, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameteri(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameteriv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 500, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameteriv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3b,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    struct Library *, MesaBase, 501, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3b(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 502, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3bv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3d,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    struct Library *, MesaBase, 503, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3d(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 504, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3f,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    struct Library *, MesaBase, 505, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3f(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 506, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3i,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    struct Library *, MesaBase, 507, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3i(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 508, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3s,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    struct Library *, MesaBase, 509, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3s(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 510, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3ub,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    struct Library *, MesaBase, 511, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3ub(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3ubv,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 512, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3ubv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3ui,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    struct Library *, MesaBase, 513, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3ui(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3uiv,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 514, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3uiv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3us,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    struct Library *, MesaBase, 515, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3us(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3usv,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 516, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3usv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glSecondaryColorPointer,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 517, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColorPointer(size, type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 518, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2d(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 519, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 520, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2f(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 521, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 522, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2i(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 523, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 524, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2s(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 525, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 526, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3d(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 527, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 528, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3f(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 529, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 530, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3i(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 531, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 532, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3s(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 533, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenQueries,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, ids, A0),
    struct Library *, MesaBase, 534, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenQueries(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteQueries,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, ids, A0),
    struct Library *, MesaBase, 535, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteQueries(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsQuery,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 536, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsQuery(id);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginQuery,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 537, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBeginQuery(target, id);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEndQuery,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 538, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEndQuery(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 539, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetQueryiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectiv,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 540, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetQueryObjectiv(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectuiv,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 541, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetQueryObjectuiv(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindBuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, buffer, D1),
    struct Library *, MesaBase, 542, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindBuffer(target, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteBuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, buffers, A0),
    struct Library *, MesaBase, 543, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteBuffers(n, buffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenBuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, buffers, A0),
    struct Library *, MesaBase, 544, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenBuffers(n, buffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsBuffer,
    AROS_LHA(GLuint, buffer, D0),
    struct Library *, MesaBase, 545, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsBuffer(buffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBufferData,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizeiptr, size, D1),
    AROS_LHA(const GLvoid *, data, A0),
    AROS_LHA(GLenum, usage, D2),
    struct Library *, MesaBase, 546, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBufferData(target, size, data, usage);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBufferSubData,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, size, D2),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 547, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBufferSubData(target, offset, size, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetBufferSubData,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, size, D2),
    AROS_LHA(GLvoid *, data, A0),
    struct Library *, MesaBase, 548, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBufferSubData(target, offset, size, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLvoid*, glMapBuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, access, D1),
    struct Library *, MesaBase, 549, Mesa)
{
    AROS_LIBFUNC_INIT

    GLvoid* _return = mglMapBuffer(target, access);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glUnmapBuffer,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 550, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglUnmapBuffer(target);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 551, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBufferParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferPointerv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 552, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBufferPointerv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationSeparate,
    AROS_LHA(GLenum, modeRGB, D0),
    AROS_LHA(GLenum, modeAlpha, D1),
    struct Library *, MesaBase, 553, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendEquationSeparate(modeRGB, modeAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawBuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLenum *, bufs, A0),
    struct Library *, MesaBase, 554, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawBuffers(n, bufs);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glStencilOpSeparate,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, sfail, D1),
    AROS_LHA(GLenum, dpfail, D2),
    AROS_LHA(GLenum, dppass, D3),
    struct Library *, MesaBase, 555, Mesa)
{
    AROS_LIBFUNC_INIT

    mglStencilOpSeparate(face, sfail, dpfail, dppass);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glStencilFuncSeparate,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, func, D1),
    AROS_LHA(GLint, ref, D2),
    AROS_LHA(GLuint, mask, D3),
    struct Library *, MesaBase, 556, Mesa)
{
    AROS_LIBFUNC_INIT

    mglStencilFuncSeparate(face, func, ref, mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glStencilMaskSeparate,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLuint, mask, D1),
    struct Library *, MesaBase, 557, Mesa)
{
    AROS_LIBFUNC_INIT

    mglStencilMaskSeparate(face, mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAttachShader,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, shader, D1),
    struct Library *, MesaBase, 558, Mesa)
{
    AROS_LIBFUNC_INIT

    mglAttachShader(program, shader);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindAttribLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 559, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindAttribLocation(program, index, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCompileShader,
    AROS_LHA(GLuint, shader, D0),
    struct Library *, MesaBase, 560, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompileShader(shader);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLuint, glCreateProgram,
    struct Library *, MesaBase, 561, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = mglCreateProgram();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLuint, glCreateShader,
    AROS_LHA(GLenum, type, D0),
    struct Library *, MesaBase, 562, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = mglCreateShader(type);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 563, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteProgram(program);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteShader,
    AROS_LHA(GLuint, shader, D0),
    struct Library *, MesaBase, 564, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteShader(shader);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDetachShader,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, shader, D1),
    struct Library *, MesaBase, 565, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDetachShader(program, shader);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisableVertexAttribArray,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 566, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDisableVertexAttribArray(index);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnableVertexAttribArray,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 567, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEnableVertexAttribArray(index);

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
    struct Library *, MesaBase, 568, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetActiveAttrib(program, index, bufSize, length, size, type, name);

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
    struct Library *, MesaBase, 569, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetActiveUniform(program, index, bufSize, length, size, type, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetAttachedShaders,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLsizei, maxCount, D1),
    AROS_LHA(GLsizei *, count, A0),
    AROS_LHA(GLuint *, obj, A1),
    struct Library *, MesaBase, 570, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetAttachedShaders(program, maxCount, count, obj);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetAttribLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 571, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = mglGetAttribLocation(program, name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramiv,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 572, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramiv(program, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramInfoLog,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLchar *, infoLog, A1),
    struct Library *, MesaBase, 573, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramInfoLog(program, bufSize, length, infoLog);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetShaderiv,
    AROS_LHA(GLuint, shader, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 574, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetShaderiv(shader, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetShaderInfoLog,
    AROS_LHA(GLuint, shader, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLchar *, infoLog, A1),
    struct Library *, MesaBase, 575, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetShaderInfoLog(shader, bufSize, length, infoLog);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetShaderSource,
    AROS_LHA(GLuint, shader, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLchar *, source, A1),
    struct Library *, MesaBase, 576, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetShaderSource(shader, bufSize, length, source);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetUniformLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 577, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = mglGetUniformLocation(program, name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformfv,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 578, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetUniformfv(program, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformiv,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 579, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetUniformiv(program, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribdv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 580, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribdv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribfv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 581, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribfv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 582, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribiv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribPointerv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, pointer, A0),
    struct Library *, MesaBase, 583, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribPointerv(index, pname, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 584, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsProgram(program);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsShader,
    AROS_LHA(GLuint, shader, D0),
    struct Library *, MesaBase, 585, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsShader(shader);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLinkProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 586, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLinkProgram(program);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glShaderSource,
    AROS_LHA(GLuint, shader, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLchar *  *, string, A0),
    AROS_LHA(const GLint *, length, A1),
    struct Library *, MesaBase, 587, Mesa)
{
    AROS_LIBFUNC_INIT

    mglShaderSource(shader, count, string, length);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glUseProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 588, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUseProgram(program);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    struct Library *, MesaBase, 589, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1f(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    struct Library *, MesaBase, 590, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2f(location, v0, v1);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    AROS_LHA(GLfloat, v2, D3),
    struct Library *, MesaBase, 591, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3f(location, v0, v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    AROS_LHA(GLfloat, v2, D3),
    AROS_LHA(GLfloat, v3, D4),
    struct Library *, MesaBase, 592, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4f(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    struct Library *, MesaBase, 593, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1i(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    struct Library *, MesaBase, 594, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2i(location, v0, v1);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    AROS_LHA(GLint, v2, D3),
    struct Library *, MesaBase, 595, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3i(location, v0, v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    AROS_LHA(GLint, v2, D3),
    AROS_LHA(GLint, v3, D4),
    struct Library *, MesaBase, 596, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4i(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 597, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1fv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 598, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2fv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 599, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3fv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 600, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4fv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 601, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1iv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 602, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2iv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 603, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3iv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 604, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4iv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix2fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 605, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix2fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix3fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 606, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix3fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix4fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 607, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix4fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glValidateProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 608, Mesa)
{
    AROS_LIBFUNC_INIT

    mglValidateProgram(program);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    struct Library *, MesaBase, 609, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1d(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 610, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1dv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    struct Library *, MesaBase, 611, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1f(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 612, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1fv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    struct Library *, MesaBase, 613, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1s(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 614, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1sv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    struct Library *, MesaBase, 615, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2d(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 616, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2dv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    struct Library *, MesaBase, 617, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2f(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 618, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2fv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    struct Library *, MesaBase, 619, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2s(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 620, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2sv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    struct Library *, MesaBase, 621, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3d(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 622, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3dv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    struct Library *, MesaBase, 623, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3f(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 624, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3fv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    struct Library *, MesaBase, 625, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3s(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 626, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3sv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nbv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 627, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4Nbv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Niv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 628, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4Niv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nsv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 629, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4Nsv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4Nub,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLubyte, x, D1),
    AROS_LHA(GLubyte, y, D2),
    AROS_LHA(GLubyte, z, D3),
    AROS_LHA(GLubyte, w, D4),
    struct Library *, MesaBase, 630, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4Nub(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nubv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 631, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4Nubv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nuiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 632, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4Nuiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nusv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 633, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4Nusv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4bv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 634, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4bv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    AROS_LHA(GLdouble, w, D4),
    struct Library *, MesaBase, 635, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4d(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 636, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4dv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    AROS_LHA(GLfloat, w, D4),
    struct Library *, MesaBase, 637, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4f(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 638, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4fv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 639, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4iv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    AROS_LHA(GLshort, w, D4),
    struct Library *, MesaBase, 640, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4s(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 641, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4sv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ubv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 642, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4ubv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 643, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4usv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 644, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4usv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glVertexAttribPointer,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, size, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLboolean, normalized, D3),
    AROS_LHA(GLsizei, stride, D4),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 645, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribPointer(index, size, type, normalized, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix2x3fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 646, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix2x3fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix3x2fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 647, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix3x2fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix2x4fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 648, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix2x4fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix4x2fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 649, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix4x2fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix3x4fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 650, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix3x4fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix4x3fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 651, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix4x3fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixfARB,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 652, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLoadTransposeMatrixfARB(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixdARB,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 653, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLoadTransposeMatrixdARB(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixfARB,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 654, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultTransposeMatrixfARB(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixdARB,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 655, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultTransposeMatrixdARB(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleCoverageARB,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 656, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSampleCoverageARB(value, invert);

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
    struct Library *, MesaBase, 657, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imageSize, data);

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
    struct Library *, MesaBase, 658, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexImage2DARB(target, level, internalformat, width, height, border, imageSize, data);

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
    struct Library *, MesaBase, 659, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexImage1DARB(target, level, internalformat, width, border, imageSize, data);

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
    struct Library *, MesaBase, 660, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);

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
    struct Library *, MesaBase, 661, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data);

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
    struct Library *, MesaBase, 662, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompressedTexSubImage1DARB(target, level, xoffset, width, format, imageSize, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetCompressedTexImageARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLvoid *, img, A0),
    struct Library *, MesaBase, 663, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetCompressedTexImageARB(target, level, img);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfARB,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 664, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameterfARB(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfvARB,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 665, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameterfvARB(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2dARB,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 666, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2dARB(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2dvARB,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 667, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2dvARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2fARB,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 668, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2fARB(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2fvARB,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 669, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2fvARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2iARB,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 670, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2iARB(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2ivARB,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 671, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2ivARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2sARB,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 672, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2sARB(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2svARB,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 673, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2svARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3dARB,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 674, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3dARB(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3dvARB,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 675, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3dvARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3fARB,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 676, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3fARB(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3fvARB,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 677, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3fvARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3iARB,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 678, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3iARB(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3ivARB,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 679, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3ivARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3sARB,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 680, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3sARB(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3svARB,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 681, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3svARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    struct Library *, MesaBase, 682, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1dARB(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 683, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1dvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    struct Library *, MesaBase, 684, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1fARB(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 685, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1fvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    struct Library *, MesaBase, 686, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1sARB(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 687, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1svARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    struct Library *, MesaBase, 688, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2dARB(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 689, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2dvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    struct Library *, MesaBase, 690, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2fARB(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 691, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2fvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    struct Library *, MesaBase, 692, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2sARB(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 693, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2svARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    struct Library *, MesaBase, 694, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3dARB(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 695, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3dvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    struct Library *, MesaBase, 696, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3fARB(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 697, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3fvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    struct Library *, MesaBase, 698, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3sARB(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 699, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3svARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NbvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 700, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4NbvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 701, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4NivARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NsvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 702, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4NsvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4NubARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLubyte, x, D1),
    AROS_LHA(GLubyte, y, D2),
    AROS_LHA(GLubyte, z, D3),
    AROS_LHA(GLubyte, w, D4),
    struct Library *, MesaBase, 703, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4NubARB(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NubvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 704, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4NubvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NuivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 705, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4NuivARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NusvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 706, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4NusvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4bvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 707, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4bvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    AROS_LHA(GLdouble, w, D4),
    struct Library *, MesaBase, 708, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4dARB(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 709, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4dvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    AROS_LHA(GLfloat, w, D4),
    struct Library *, MesaBase, 710, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4fARB(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 711, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4fvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 712, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4ivARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    AROS_LHA(GLshort, w, D4),
    struct Library *, MesaBase, 713, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4sARB(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 714, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4svARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ubvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 715, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4ubvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4uivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 716, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4uivARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4usvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 717, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4usvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glVertexAttribPointerARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, size, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLboolean, normalized, D3),
    AROS_LHA(GLsizei, stride, D4),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 718, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribPointerARB(index, size, type, normalized, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnableVertexAttribArrayARB,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 719, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEnableVertexAttribArrayARB(index);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisableVertexAttribArrayARB,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 720, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDisableVertexAttribArrayARB(index);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramStringARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLsizei, len, D2),
    AROS_LHA(const GLvoid *, string, A0),
    struct Library *, MesaBase, 721, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramStringARB(target, format, len, string);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindProgramARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, program, D1),
    struct Library *, MesaBase, 722, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindProgramARB(target, program);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteProgramsARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    struct Library *, MesaBase, 723, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteProgramsARB(n, programs);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenProgramsARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, programs, A0),
    struct Library *, MesaBase, 724, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenProgramsARB(n, programs);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramEnvParameter4dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble, x, D2),
    AROS_LHA(GLdouble, y, D3),
    AROS_LHA(GLdouble, z, D4),
    AROS_LHA(GLdouble, w, D5),
    struct Library *, MesaBase, 725, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramEnvParameter4dARB(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramEnvParameter4dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLdouble *, params, A0),
    struct Library *, MesaBase, 726, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramEnvParameter4dvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramEnvParameter4fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat, x, D2),
    AROS_LHA(GLfloat, y, D3),
    AROS_LHA(GLfloat, z, D4),
    AROS_LHA(GLfloat, w, D5),
    struct Library *, MesaBase, 727, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramEnvParameter4fARB(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramEnvParameter4fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 728, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramEnvParameter4fvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramLocalParameter4dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble, x, D2),
    AROS_LHA(GLdouble, y, D3),
    AROS_LHA(GLdouble, z, D4),
    AROS_LHA(GLdouble, w, D5),
    struct Library *, MesaBase, 729, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramLocalParameter4dARB(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramLocalParameter4dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLdouble *, params, A0),
    struct Library *, MesaBase, 730, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramLocalParameter4dvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramLocalParameter4fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat, x, D2),
    AROS_LHA(GLfloat, y, D3),
    AROS_LHA(GLfloat, z, D4),
    AROS_LHA(GLfloat, w, D5),
    struct Library *, MesaBase, 731, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramLocalParameter4fARB(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramLocalParameter4fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 732, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramLocalParameter4fvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramEnvParameterdvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 733, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramEnvParameterdvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramEnvParameterfvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 734, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramEnvParameterfvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramLocalParameterdvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 735, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramLocalParameterdvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramLocalParameterfvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 736, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramLocalParameterfvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 737, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramivARB(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramStringARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *, string, A0),
    struct Library *, MesaBase, 738, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramStringARB(target, pname, string);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribdvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 739, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribdvARB(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribfvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 740, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribfvARB(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 741, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribivARB(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribPointervARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, pointer, A0),
    struct Library *, MesaBase, 742, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribPointervARB(index, pname, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsProgramARB,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 743, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsProgramARB(program);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindBufferARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, buffer, D1),
    struct Library *, MesaBase, 744, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindBufferARB(target, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteBuffersARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, buffers, A0),
    struct Library *, MesaBase, 745, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteBuffersARB(n, buffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenBuffersARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, buffers, A0),
    struct Library *, MesaBase, 746, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenBuffersARB(n, buffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsBufferARB,
    AROS_LHA(GLuint, buffer, D0),
    struct Library *, MesaBase, 747, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsBufferARB(buffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBufferDataARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizeiptrARB, size, D1),
    AROS_LHA(const GLvoid *, data, A0),
    AROS_LHA(GLenum, usage, D2),
    struct Library *, MesaBase, 748, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBufferDataARB(target, size, data, usage);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBufferSubDataARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptrARB, offset, D1),
    AROS_LHA(GLsizeiptrARB, size, D2),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 749, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBufferSubDataARB(target, offset, size, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetBufferSubDataARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptrARB, offset, D1),
    AROS_LHA(GLsizeiptrARB, size, D2),
    AROS_LHA(GLvoid *, data, A0),
    struct Library *, MesaBase, 750, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBufferSubDataARB(target, offset, size, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLvoid*, glMapBufferARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, access, D1),
    struct Library *, MesaBase, 751, Mesa)
{
    AROS_LIBFUNC_INIT

    GLvoid* _return = mglMapBufferARB(target, access);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glUnmapBufferARB,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 752, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglUnmapBufferARB(target);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferParameterivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 753, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBufferParameterivARB(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferPointervARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 754, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBufferPointervARB(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenQueriesARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, ids, A0),
    struct Library *, MesaBase, 755, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenQueriesARB(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteQueriesARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, ids, A0),
    struct Library *, MesaBase, 756, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteQueriesARB(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsQueryARB,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 757, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsQueryARB(id);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginQueryARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 758, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBeginQueryARB(target, id);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEndQueryARB,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 759, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEndQueryARB(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 760, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetQueryivARB(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectivARB,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 761, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetQueryObjectivARB(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectuivARB,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 762, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetQueryObjectuivARB(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteObjectARB,
    AROS_LHA(GLhandleARB, obj, D0),
    struct Library *, MesaBase, 763, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteObjectARB(obj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLhandleARB, glGetHandleARB,
    AROS_LHA(GLenum, pname, D0),
    struct Library *, MesaBase, 764, Mesa)
{
    AROS_LIBFUNC_INIT

    GLhandleARB _return = mglGetHandleARB(pname);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDetachObjectARB,
    AROS_LHA(GLhandleARB, containerObj, D0),
    AROS_LHA(GLhandleARB, attachedObj, D1),
    struct Library *, MesaBase, 765, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDetachObjectARB(containerObj, attachedObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLhandleARB, glCreateShaderObjectARB,
    AROS_LHA(GLenum, shaderType, D0),
    struct Library *, MesaBase, 766, Mesa)
{
    AROS_LIBFUNC_INIT

    GLhandleARB _return = mglCreateShaderObjectARB(shaderType);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glShaderSourceARB,
    AROS_LHA(GLhandleARB, shaderObj, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLcharARB *  *, string, A0),
    AROS_LHA(const GLint *, length, A1),
    struct Library *, MesaBase, 767, Mesa)
{
    AROS_LIBFUNC_INIT

    mglShaderSourceARB(shaderObj, count, string, length);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCompileShaderARB,
    AROS_LHA(GLhandleARB, shaderObj, D0),
    struct Library *, MesaBase, 768, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCompileShaderARB(shaderObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLhandleARB, glCreateProgramObjectARB,
    struct Library *, MesaBase, 769, Mesa)
{
    AROS_LIBFUNC_INIT

    GLhandleARB _return = mglCreateProgramObjectARB();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAttachObjectARB,
    AROS_LHA(GLhandleARB, containerObj, D0),
    AROS_LHA(GLhandleARB, obj, D1),
    struct Library *, MesaBase, 770, Mesa)
{
    AROS_LIBFUNC_INIT

    mglAttachObjectARB(containerObj, obj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLinkProgramARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    struct Library *, MesaBase, 771, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLinkProgramARB(programObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glUseProgramObjectARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    struct Library *, MesaBase, 772, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUseProgramObjectARB(programObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glValidateProgramARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    struct Library *, MesaBase, 773, Mesa)
{
    AROS_LIBFUNC_INIT

    mglValidateProgramARB(programObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    struct Library *, MesaBase, 774, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1fARB(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    struct Library *, MesaBase, 775, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2fARB(location, v0, v1);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    AROS_LHA(GLfloat, v2, D3),
    struct Library *, MesaBase, 776, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3fARB(location, v0, v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    AROS_LHA(GLfloat, v2, D3),
    AROS_LHA(GLfloat, v3, D4),
    struct Library *, MesaBase, 777, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4fARB(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    struct Library *, MesaBase, 778, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1iARB(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    struct Library *, MesaBase, 779, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2iARB(location, v0, v1);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    AROS_LHA(GLint, v2, D3),
    struct Library *, MesaBase, 780, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3iARB(location, v0, v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    AROS_LHA(GLint, v2, D3),
    AROS_LHA(GLint, v3, D4),
    struct Library *, MesaBase, 781, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4iARB(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 782, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1fvARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 783, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2fvARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 784, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3fvARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 785, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4fvARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 786, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1ivARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 787, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2ivARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 788, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3ivARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 789, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4ivARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix2fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 790, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix2fvARB(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix3fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 791, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix3fvARB(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniformMatrix4fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLboolean, transpose, D2),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 792, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniformMatrix4fvARB(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetObjectParameterfvARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 793, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetObjectParameterfvARB(obj, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetObjectParameterivARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 794, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetObjectParameterivARB(obj, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetInfoLogARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLsizei, maxLength, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLcharARB *, infoLog, A1),
    struct Library *, MesaBase, 795, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetInfoLogARB(obj, maxLength, length, infoLog);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetAttachedObjectsARB,
    AROS_LHA(GLhandleARB, containerObj, D0),
    AROS_LHA(GLsizei, maxCount, D1),
    AROS_LHA(GLsizei *, count, A0),
    AROS_LHA(GLhandleARB *, obj, A1),
    struct Library *, MesaBase, 796, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetAttachedObjectsARB(containerObj, maxCount, count, obj);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetUniformLocationARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(const GLcharARB *, name, A0),
    struct Library *, MesaBase, 797, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = mglGetUniformLocationARB(programObj, name);

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
    struct Library *, MesaBase, 798, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetActiveUniformARB(programObj, index, maxLength, length, size, type, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformfvARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 799, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetUniformfvARB(programObj, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformivARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 800, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetUniformivARB(programObj, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetShaderSourceARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLsizei, maxLength, D1),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLcharARB *, source, A1),
    struct Library *, MesaBase, 801, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetShaderSourceARB(obj, maxLength, length, source);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindAttribLocationARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLcharARB *, name, A0),
    struct Library *, MesaBase, 802, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindAttribLocationARB(programObj, index, name);

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
    struct Library *, MesaBase, 803, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetActiveAttribARB(programObj, index, maxLength, length, size, type, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetAttribLocationARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(const GLcharARB *, name, A0),
    struct Library *, MesaBase, 804, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = mglGetAttribLocationARB(programObj, name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawBuffersARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLenum *, bufs, A0),
    struct Library *, MesaBase, 805, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawBuffersARB(n, bufs);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsRenderbuffer,
    AROS_LHA(GLuint, renderbuffer, D0),
    struct Library *, MesaBase, 806, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsRenderbuffer(renderbuffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindRenderbuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, renderbuffer, D1),
    struct Library *, MesaBase, 807, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindRenderbuffer(target, renderbuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteRenderbuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 808, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteRenderbuffers(n, renderbuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenRenderbuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 809, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenRenderbuffers(n, renderbuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRenderbufferStorage,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    struct Library *, MesaBase, 810, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRenderbufferStorage(target, internalformat, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetRenderbufferParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 811, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetRenderbufferParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsFramebuffer,
    AROS_LHA(GLuint, framebuffer, D0),
    struct Library *, MesaBase, 812, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsFramebuffer(framebuffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindFramebuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, framebuffer, D1),
    struct Library *, MesaBase, 813, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindFramebuffer(target, framebuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteFramebuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 814, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteFramebuffers(n, framebuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenFramebuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 815, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenFramebuffers(n, framebuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLenum, glCheckFramebufferStatus,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 816, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = mglCheckFramebufferStatus(target);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTexture1D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    struct Library *, MesaBase, 817, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTexture1D(target, attachment, textarget, texture, level);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTexture2D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    struct Library *, MesaBase, 818, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTexture2D(target, attachment, textarget, texture, level);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glFramebufferTexture3D,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    AROS_LHA(GLint, zoffset, D5),
    struct Library *, MesaBase, 819, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glFramebufferRenderbuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, renderbuffertarget, D2),
    AROS_LHA(GLuint, renderbuffer, D3),
    struct Library *, MesaBase, 820, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetFramebufferAttachmentParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 821, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetFramebufferAttachmentParameteriv(target, attachment, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glGenerateMipmap,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 822, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenerateMipmap(target);

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
    struct Library *, MesaBase, 823, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glRenderbufferStorageMultisample,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, samples, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    struct Library *, MesaBase, 824, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRenderbufferStorageMultisample(target, samples, internalformat, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTextureLayer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    AROS_LHA(GLint, layer, D4),
    struct Library *, MesaBase, 825, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTextureLayer(target, attachment, texture, level, layer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBlendColorEXT,
    AROS_LHA(GLclampf, red, D0),
    AROS_LHA(GLclampf, green, D1),
    AROS_LHA(GLclampf, blue, D2),
    AROS_LHA(GLclampf, alpha, D3),
    struct Library *, MesaBase, 826, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendColorEXT(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPolygonOffsetEXT,
    AROS_LHA(GLfloat, factor, D0),
    AROS_LHA(GLfloat, bias, D1),
    struct Library *, MesaBase, 827, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPolygonOffsetEXT(factor, bias);

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
    struct Library *, MesaBase, 828, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, pixels);

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
    struct Library *, MesaBase, 829, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);

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
    struct Library *, MesaBase, 830, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexSubImage1DEXT(target, level, xoffset, width, format, type, pixels);

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
    struct Library *, MesaBase, 831, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexSubImage2DEXT(target, level, xoffset, yoffset, width, height, format, type, pixels);

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
    struct Library *, MesaBase, 832, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexImage1DEXT(target, level, internalformat, x, y, width, border);

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
    struct Library *, MesaBase, 833, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexImage2DEXT(target, level, internalformat, x, y, width, height, border);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glCopyTexSubImage1DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLint, xoffset, D2),
    AROS_LHA(GLint, x, D3),
    AROS_LHA(GLint, y, D4),
    AROS_LHA(GLsizei, width, D5),
    struct Library *, MesaBase, 834, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexSubImage1DEXT(target, level, xoffset, x, y, width);

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
    struct Library *, MesaBase, 835, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height);

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
    struct Library *, MesaBase, 836, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLboolean, glAreTexturesResidentEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(GLboolean *, residences, A1),
    struct Library *, MesaBase, 837, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglAreTexturesResidentEXT(n, textures, residences);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindTextureEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, texture, D1),
    struct Library *, MesaBase, 838, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindTextureEXT(target, texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteTexturesEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    struct Library *, MesaBase, 839, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteTexturesEXT(n, textures);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenTexturesEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, textures, A0),
    struct Library *, MesaBase, 840, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenTexturesEXT(n, textures);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsTextureEXT,
    AROS_LHA(GLuint, texture, D0),
    struct Library *, MesaBase, 841, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsTextureEXT(texture);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPrioritizeTexturesEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(const GLclampf *, priorities, A1),
    struct Library *, MesaBase, 842, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPrioritizeTexturesEXT(n, textures, priorities);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glArrayElementEXT,
    AROS_LHA(GLint, i, D0),
    struct Library *, MesaBase, 843, Mesa)
{
    AROS_LIBFUNC_INIT

    mglArrayElementEXT(i);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glColorPointerEXT,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 844, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorPointerEXT(size, type, stride, count, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glDrawArraysEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    struct Library *, MesaBase, 845, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawArraysEXT(mode, first, count);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glEdgeFlagPointerEXT,
    AROS_LHA(GLsizei, stride, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLboolean *, pointer, A0),
    struct Library *, MesaBase, 846, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEdgeFlagPointerEXT(stride, count, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPointervEXT,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 847, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetPointervEXT(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glIndexPointerEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 848, Mesa)
{
    AROS_LIBFUNC_INIT

    mglIndexPointerEXT(type, stride, count, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glNormalPointerEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 849, Mesa)
{
    AROS_LIBFUNC_INIT

    mglNormalPointerEXT(type, stride, count, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glTexCoordPointerEXT,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 850, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexCoordPointerEXT(size, type, stride, count, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexPointerEXT,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 851, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexPointerEXT(size, type, stride, count, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBlendEquationEXT,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 852, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendEquationEXT(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfEXT,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 853, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameterfEXT(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfvEXT,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 854, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameterfvEXT(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glColorTableEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalFormat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, table, A0),
    struct Library *, MesaBase, 855, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorTableEXT(target, internalFormat, width, format, type, table);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetColorTableEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, data, A0),
    struct Library *, MesaBase, 856, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetColorTableEXT(target, format, type, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 857, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetColorTableParameterivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 858, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetColorTableParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLockArraysEXT,
    AROS_LHA(GLint, first, D0),
    AROS_LHA(GLsizei, count, D1),
    struct Library *, MesaBase, 859, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLockArraysEXT(first, count);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glUnlockArraysEXT,
    struct Library *, MesaBase, 860, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUnlockArraysEXT();

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glDrawRangeElementsEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLuint, start, D1),
    AROS_LHA(GLuint, end, D2),
    AROS_LHA(GLsizei, count, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, indices, A0),
    struct Library *, MesaBase, 861, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawRangeElementsEXT(mode, start, end, count, type, indices);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3bEXT,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    struct Library *, MesaBase, 862, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3bEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3bvEXT,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 863, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3bvEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3dEXT,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    struct Library *, MesaBase, 864, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3dEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3dvEXT,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 865, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3dvEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3fEXT,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    struct Library *, MesaBase, 866, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3fEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3fvEXT,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 867, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3fvEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3iEXT,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    struct Library *, MesaBase, 868, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3iEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3ivEXT,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 869, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3ivEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3sEXT,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    struct Library *, MesaBase, 870, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3sEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3svEXT,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 871, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3svEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3ubEXT,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    struct Library *, MesaBase, 872, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3ubEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3ubvEXT,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 873, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3ubvEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3uiEXT,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    struct Library *, MesaBase, 874, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3uiEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3uivEXT,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 875, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3uivEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3usEXT,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    struct Library *, MesaBase, 876, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3usEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3usvEXT,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 877, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColor3usvEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glSecondaryColorPointerEXT,
    AROS_LHA(GLint, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLsizei, stride, D2),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 878, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSecondaryColorPointerEXT(size, type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glMultiDrawArraysEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLint *, first, A0),
    AROS_LHA(const GLsizei *, count, A1),
    AROS_LHA(GLsizei, primcount, D1),
    struct Library *, MesaBase, 879, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiDrawArraysEXT(mode, first, count, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiDrawElementsEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLsizei *, count, A0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(const GLvoid *  *, indices, A1),
    AROS_LHA(GLsizei, primcount, D2),
    struct Library *, MesaBase, 880, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiDrawElementsEXT(mode, count, type, indices, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordfEXT,
    AROS_LHA(GLfloat, coord, D0),
    struct Library *, MesaBase, 881, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoordfEXT(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordfvEXT,
    AROS_LHA(const GLfloat *, coord, A0),
    struct Library *, MesaBase, 882, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoordfvEXT(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoorddEXT,
    AROS_LHA(GLdouble, coord, D0),
    struct Library *, MesaBase, 883, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoorddEXT(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoorddvEXT,
    AROS_LHA(const GLdouble *, coord, A0),
    struct Library *, MesaBase, 884, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoorddvEXT(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFogCoordPointerEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 885, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFogCoordPointerEXT(type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBlendFuncSeparateEXT,
    AROS_LHA(GLenum, sfactorRGB, D0),
    AROS_LHA(GLenum, dfactorRGB, D1),
    AROS_LHA(GLenum, sfactorAlpha, D2),
    AROS_LHA(GLenum, dfactorAlpha, D3),
    struct Library *, MesaBase, 886, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendFuncSeparateEXT(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glFlushVertexArrayRangeNV,
    struct Library *, MesaBase, 887, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFlushVertexArrayRangeNV();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexArrayRangeNV,
    AROS_LHA(GLsizei, length, D0),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 888, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexArrayRangeNV(length, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameterfvNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 889, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCombinerParameterfvNV(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameterfNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 890, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCombinerParameterfNV(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameterivNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 891, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCombinerParameterivNV(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameteriNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 892, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCombinerParameteriNV(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glCombinerInputNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, variable, D2),
    AROS_LHA(GLenum, input, D3),
    AROS_LHA(GLenum, mapping, D4),
    AROS_LHA(GLenum, componentUsage, D5),
    struct Library *, MesaBase, 893, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCombinerInputNV(stage, portion, variable, input, mapping, componentUsage);

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
    AROS_LHA(GLboolean, abDotProduct, A0),
    AROS_LHA(GLboolean, cdDotProduct, A1),
    AROS_LHA(GLboolean, muxSum, A2),
    struct Library *, MesaBase, 894, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCombinerOutputNV(stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glFinalCombinerInputNV,
    AROS_LHA(GLenum, variable, D0),
    AROS_LHA(GLenum, input, D1),
    AROS_LHA(GLenum, mapping, D2),
    AROS_LHA(GLenum, componentUsage, D3),
    struct Library *, MesaBase, 895, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFinalCombinerInputNV(variable, input, mapping, componentUsage);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetCombinerInputParameterfvNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, variable, D2),
    AROS_LHA(GLenum, pname, D3),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 896, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetCombinerInputParameterfvNV(stage, portion, variable, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetCombinerInputParameterivNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, variable, D2),
    AROS_LHA(GLenum, pname, D3),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 897, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetCombinerInputParameterivNV(stage, portion, variable, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetCombinerOutputParameterfvNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 898, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetCombinerOutputParameterfvNV(stage, portion, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetCombinerOutputParameterivNV,
    AROS_LHA(GLenum, stage, D0),
    AROS_LHA(GLenum, portion, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 899, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetCombinerOutputParameterivNV(stage, portion, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetFinalCombinerInputParameterfvNV,
    AROS_LHA(GLenum, variable, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 900, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetFinalCombinerInputParameterfvNV(variable, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetFinalCombinerInputParameterivNV,
    AROS_LHA(GLenum, variable, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 901, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetFinalCombinerInputParameterivNV(variable, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glResizeBuffersMESA,
    struct Library *, MesaBase, 902, Mesa)
{
    AROS_LIBFUNC_INIT

    mglResizeBuffersMESA();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2dMESA,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 903, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2dMESA(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2dvMESA,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 904, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2dvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2fMESA,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 905, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2fMESA(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2fvMESA,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 906, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2fvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2iMESA,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 907, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2iMESA(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2ivMESA,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 908, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2ivMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2sMESA,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 909, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2sMESA(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2svMESA,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 910, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos2svMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3dMESA,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 911, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3dMESA(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3dvMESA,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 912, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3dvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3fMESA,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 913, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3fMESA(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3fvMESA,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 914, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3fvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3iMESA,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 915, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3iMESA(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3ivMESA,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 916, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3ivMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3sMESA,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 917, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3sMESA(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3svMESA,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 918, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos3svMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glWindowPos4dMESA,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    AROS_LHA(GLdouble, w, D3),
    struct Library *, MesaBase, 919, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos4dMESA(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4dvMESA,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 920, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos4dvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glWindowPos4fMESA,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    AROS_LHA(GLfloat, w, D3),
    struct Library *, MesaBase, 921, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos4fMESA(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4fvMESA,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 922, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos4fvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glWindowPos4iMESA,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    AROS_LHA(GLint, w, D3),
    struct Library *, MesaBase, 923, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos4iMESA(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4ivMESA,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 924, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos4ivMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glWindowPos4sMESA,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    AROS_LHA(GLshort, w, D3),
    struct Library *, MesaBase, 925, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos4sMESA(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4svMESA,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 926, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWindowPos4svMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLboolean, glAreProgramsResidentNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    AROS_LHA(GLboolean *, residences, A1),
    struct Library *, MesaBase, 927, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglAreProgramsResidentNV(n, programs, residences);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindProgramNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 928, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindProgramNV(target, id);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteProgramsNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    struct Library *, MesaBase, 929, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteProgramsNV(n, programs);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glExecuteProgramNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 930, Mesa)
{
    AROS_LIBFUNC_INIT

    mglExecuteProgramNV(target, id, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenProgramsNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, programs, A0),
    struct Library *, MesaBase, 931, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenProgramsNV(n, programs);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramParameterdvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 932, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramParameterdvNV(target, index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramParameterfvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 933, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramParameterfvNV(target, index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramivNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 934, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramivNV(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramStringNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLubyte *, program, A0),
    struct Library *, MesaBase, 935, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramStringNV(id, pname, program);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetTrackMatrixivNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, address, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 936, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTrackMatrixivNV(target, address, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribdvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 937, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribdvNV(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribfvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 938, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribfvNV(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribivNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 939, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribivNV(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribPointervNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, pointer, A0),
    struct Library *, MesaBase, 940, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribPointervNV(index, pname, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsProgramNV,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 941, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsProgramNV(id);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glLoadProgramNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    AROS_LHA(GLsizei, len, D2),
    AROS_LHA(const GLubyte *, program, A0),
    struct Library *, MesaBase, 942, Mesa)
{
    AROS_LIBFUNC_INIT

    mglLoadProgramNV(target, id, len, program);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramParameter4dNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble, x, D2),
    AROS_LHA(GLdouble, y, D3),
    AROS_LHA(GLdouble, z, D4),
    AROS_LHA(GLdouble, w, D5),
    struct Library *, MesaBase, 943, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramParameter4dNV(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramParameter4dvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 944, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramParameter4dvNV(target, index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glProgramParameter4fNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat, x, D2),
    AROS_LHA(GLfloat, y, D3),
    AROS_LHA(GLfloat, z, D4),
    AROS_LHA(GLfloat, w, D5),
    struct Library *, MesaBase, 945, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramParameter4fNV(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramParameter4fvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 946, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramParameter4fvNV(target, index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramParameters4dvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 947, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramParameters4dvNV(target, index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramParameters4fvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 948, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramParameters4fvNV(target, index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRequestResidentProgramsNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    struct Library *, MesaBase, 949, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRequestResidentProgramsNV(n, programs);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTrackMatrixNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, address, D1),
    AROS_LHA(GLenum, matrix, D2),
    AROS_LHA(GLenum, transform, D3),
    struct Library *, MesaBase, 950, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTrackMatrixNV(target, address, matrix, transform);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttribPointerNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, fsize, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLsizei, stride, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 951, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribPointerNV(index, fsize, type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    struct Library *, MesaBase, 952, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1dNV(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 953, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1dvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    struct Library *, MesaBase, 954, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1fNV(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 955, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1fvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    struct Library *, MesaBase, 956, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1sNV(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 957, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib1svNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    struct Library *, MesaBase, 958, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2dNV(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 959, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2dvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    struct Library *, MesaBase, 960, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2fNV(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 961, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2fvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    struct Library *, MesaBase, 962, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2sNV(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 963, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib2svNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    struct Library *, MesaBase, 964, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3dNV(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 965, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3dvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    struct Library *, MesaBase, 966, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3fNV(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 967, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3fvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttrib3sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    struct Library *, MesaBase, 968, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3sNV(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 969, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib3svNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    AROS_LHA(GLdouble, z, D3),
    AROS_LHA(GLdouble, w, D4),
    struct Library *, MesaBase, 970, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4dNV(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 971, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4dvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    AROS_LHA(GLfloat, z, D3),
    AROS_LHA(GLfloat, w, D4),
    struct Library *, MesaBase, 972, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4fNV(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 973, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4fvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    AROS_LHA(GLshort, z, D3),
    AROS_LHA(GLshort, w, D4),
    struct Library *, MesaBase, 974, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4sNV(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 975, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4svNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttrib4ubNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLubyte, x, D1),
    AROS_LHA(GLubyte, y, D2),
    AROS_LHA(GLubyte, z, D3),
    AROS_LHA(GLubyte, w, D4),
    struct Library *, MesaBase, 976, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4ubNV(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ubvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 977, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttrib4ubvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs1dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 978, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs1dvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs1fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 979, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs1fvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs1svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 980, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs1svNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs2dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 981, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs2dvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs2fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 982, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs2fvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs2svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 983, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs2svNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs3dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 984, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs3dvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs3fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 985, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs3fvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs3svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 986, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs3svNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 987, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs4dvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 988, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs4fvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 989, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs4svNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4ubvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 990, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribs4ubvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexBumpParameterivATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, param, A0),
    struct Library *, MesaBase, 991, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexBumpParameterivATI(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexBumpParameterfvATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, param, A0),
    struct Library *, MesaBase, 992, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexBumpParameterfvATI(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetTexBumpParameterivATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint *, param, A0),
    struct Library *, MesaBase, 993, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexBumpParameterivATI(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetTexBumpParameterfvATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat *, param, A0),
    struct Library *, MesaBase, 994, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexBumpParameterfvATI(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLuint, glGenFragmentShadersATI,
    AROS_LHA(GLuint, range, D0),
    struct Library *, MesaBase, 995, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = mglGenFragmentShadersATI(range);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBindFragmentShaderATI,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 996, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindFragmentShaderATI(id);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteFragmentShaderATI,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 997, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteFragmentShaderATI(id);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glBeginFragmentShaderATI,
    struct Library *, MesaBase, 998, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBeginFragmentShaderATI();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndFragmentShaderATI,
    struct Library *, MesaBase, 999, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEndFragmentShaderATI();

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPassTexCoordATI,
    AROS_LHA(GLuint, dst, D0),
    AROS_LHA(GLuint, coord, D1),
    AROS_LHA(GLenum, swizzle, D2),
    struct Library *, MesaBase, 1000, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPassTexCoordATI(dst, coord, swizzle);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSampleMapATI,
    AROS_LHA(GLuint, dst, D0),
    AROS_LHA(GLuint, interp, D1),
    AROS_LHA(GLenum, swizzle, D2),
    struct Library *, MesaBase, 1001, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSampleMapATI(dst, interp, swizzle);

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
    struct Library *, MesaBase, 1002, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorFragmentOp1ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod);

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
    struct Library *, MesaBase, 1003, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorFragmentOp2ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);

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
    struct Library *, MesaBase, 1004, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorFragmentOp3ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glAlphaFragmentOp1ATI,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLuint, dst, D1),
    AROS_LHA(GLuint, dstMod, D2),
    AROS_LHA(GLuint, arg1, D3),
    AROS_LHA(GLuint, arg1Rep, D4),
    AROS_LHA(GLuint, arg1Mod, D5),
    struct Library *, MesaBase, 1005, Mesa)
{
    AROS_LIBFUNC_INIT

    mglAlphaFragmentOp1ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod);

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
    struct Library *, MesaBase, 1006, Mesa)
{
    AROS_LIBFUNC_INIT

    mglAlphaFragmentOp2ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);

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
    struct Library *, MesaBase, 1007, Mesa)
{
    AROS_LIBFUNC_INIT

    mglAlphaFragmentOp3ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSetFragmentShaderConstantATI,
    AROS_LHA(GLuint, dst, D0),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 1008, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSetFragmentShaderConstantATI(dst, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameteriNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 1009, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameteriNV(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterivNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1010, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameterivNV(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawBuffersATI,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLenum *, bufs, A0),
    struct Library *, MesaBase, 1011, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawBuffersATI(n, bufs);

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
    struct Library *, MesaBase, 1012, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramNamedParameter4fNV(id, len, name, x, y, z, w);

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
    struct Library *, MesaBase, 1013, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramNamedParameter4dNV(id, len, name, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramNamedParameter4fvNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(const GLfloat *, v, A1),
    struct Library *, MesaBase, 1014, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramNamedParameter4fvNV(id, len, name, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramNamedParameter4dvNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(const GLdouble *, v, A1),
    struct Library *, MesaBase, 1015, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramNamedParameter4dvNV(id, len, name, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramNamedParameterfvNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(GLfloat *, params, A1),
    struct Library *, MesaBase, 1016, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramNamedParameterfvNV(id, len, name, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetProgramNamedParameterdvNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLsizei, len, D1),
    AROS_LHA(const GLubyte *, name, A0),
    AROS_LHA(GLdouble *, params, A1),
    struct Library *, MesaBase, 1017, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetProgramNamedParameterdvNV(id, len, name, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsRenderbufferEXT,
    AROS_LHA(GLuint, renderbuffer, D0),
    struct Library *, MesaBase, 1018, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsRenderbufferEXT(renderbuffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindRenderbufferEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, renderbuffer, D1),
    struct Library *, MesaBase, 1019, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindRenderbufferEXT(target, renderbuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteRenderbuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 1020, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteRenderbuffersEXT(n, renderbuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenRenderbuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 1021, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenRenderbuffersEXT(n, renderbuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glRenderbufferStorageEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    struct Library *, MesaBase, 1022, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRenderbufferStorageEXT(target, internalformat, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetRenderbufferParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1023, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetRenderbufferParameterivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsFramebufferEXT,
    AROS_LHA(GLuint, framebuffer, D0),
    struct Library *, MesaBase, 1024, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsFramebufferEXT(framebuffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindFramebufferEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, framebuffer, D1),
    struct Library *, MesaBase, 1025, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindFramebufferEXT(target, framebuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteFramebuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 1026, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteFramebuffersEXT(n, framebuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenFramebuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 1027, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenFramebuffersEXT(n, framebuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLenum, glCheckFramebufferStatusEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 1028, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = mglCheckFramebufferStatusEXT(target);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTexture1DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    struct Library *, MesaBase, 1029, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTexture1DEXT(target, attachment, textarget, texture, level);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTexture2DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    struct Library *, MesaBase, 1030, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTexture2DEXT(target, attachment, textarget, texture, level);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glFramebufferTexture3DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, textarget, D2),
    AROS_LHA(GLuint, texture, D3),
    AROS_LHA(GLint, level, D4),
    AROS_LHA(GLint, zoffset, D5),
    struct Library *, MesaBase, 1031, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTexture3DEXT(target, attachment, textarget, texture, level, zoffset);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glFramebufferRenderbufferEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, renderbuffertarget, D2),
    AROS_LHA(GLuint, renderbuffer, D3),
    struct Library *, MesaBase, 1032, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferRenderbufferEXT(target, attachment, renderbuffertarget, renderbuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetFramebufferAttachmentParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1033, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetFramebufferAttachmentParameterivEXT(target, attachment, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glGenerateMipmapEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 1034, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenerateMipmapEXT(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTextureLayerEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    AROS_LHA(GLint, layer, D4),
    struct Library *, MesaBase, 1035, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTextureLayerEXT(target, attachment, texture, level, layer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(GLvoid*, glMapBufferRange,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, length, D2),
    AROS_LHA(GLbitfield, access, D3),
    struct Library *, MesaBase, 1036, Mesa)
{
    AROS_LIBFUNC_INIT

    GLvoid* _return = mglMapBufferRange(target, offset, length, access);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFlushMappedBufferRange,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, length, D2),
    struct Library *, MesaBase, 1037, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFlushMappedBufferRange(target, offset, length);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBindVertexArray,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 1038, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindVertexArray(array);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteVertexArrays,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, arrays, A0),
    struct Library *, MesaBase, 1039, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteVertexArrays(n, arrays);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenVertexArrays,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, arrays, A0),
    struct Library *, MesaBase, 1040, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenVertexArrays(n, arrays);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsVertexArray,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 1041, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsVertexArray(array);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyBufferSubData,
    AROS_LHA(GLenum, readTarget, D0),
    AROS_LHA(GLenum, writeTarget, D1),
    AROS_LHA(GLintptr, readOffset, D2),
    AROS_LHA(GLintptr, writeOffset, D3),
    AROS_LHA(GLsizeiptr, size, D4),
    struct Library *, MesaBase, 1042, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLsync, glFenceSync,
    AROS_LHA(GLenum, condition, D0),
    AROS_LHA(GLbitfield, flags, D1),
    struct Library *, MesaBase, 1043, Mesa)
{
    AROS_LIBFUNC_INIT

    GLsync _return = mglFenceSync(condition, flags);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsSync,
    AROS_LHA(GLsync, sync, D0),
    struct Library *, MesaBase, 1044, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsSync(sync);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteSync,
    AROS_LHA(GLsync, sync, D0),
    struct Library *, MesaBase, 1045, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteSync(sync);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLenum, glClientWaitSync,
    AROS_LHA(GLsync, sync, D0),
    AROS_LHA(GLbitfield, flags, D1),
    AROS_LHA(GLuint64, timeout, D2),
    struct Library *, MesaBase, 1046, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = mglClientWaitSync(sync, flags, timeout);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWaitSync,
    AROS_LHA(GLsync, sync, D0),
    AROS_LHA(GLbitfield, flags, D1),
    AROS_LHA(GLuint64, timeout, D2),
    struct Library *, MesaBase, 1047, Mesa)
{
    AROS_LIBFUNC_INIT

    mglWaitSync(sync, flags, timeout);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetInteger64v,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint64 *, params, A0),
    struct Library *, MesaBase, 1048, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetInteger64v(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetSynciv,
    AROS_LHA(GLsync, sync, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLsizei *, length, A0),
    AROS_LHA(GLint *, values, A1),
    struct Library *, MesaBase, 1049, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetSynciv(sync, pname, bufSize, length, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glProvokingVertexEXT,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 1050, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProvokingVertexEXT(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawElementsBaseVertex,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLint, basevertex, D3),
    struct Library *, MesaBase, 1051, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawElementsBaseVertex(mode, count, type, indices, basevertex);

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
    struct Library *, MesaBase, 1052, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMultiDrawElementsBaseVertex,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(const GLsizei *, count, A0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(const GLvoid *  *, indices, A1),
    AROS_LHA(GLsizei, primcount, D2),
    AROS_LHA(const GLint *, basevertex, A2),
    struct Library *, MesaBase, 1053, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiDrawElementsBaseVertex(mode, count, type, indices, primcount, basevertex);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glProvokingVertex,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 1054, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProvokingVertex(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glRenderbufferStorageMultisampleEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, samples, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLsizei, width, D3),
    AROS_LHA(GLsizei, height, D4),
    struct Library *, MesaBase, 1055, Mesa)
{
    AROS_LIBFUNC_INIT

    mglRenderbufferStorageMultisampleEXT(target, samples, internalformat, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glColorMaskIndexedEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLboolean, r, D1),
    AROS_LHA(GLboolean, g, D2),
    AROS_LHA(GLboolean, b, D3),
    AROS_LHA(GLboolean, a, D4),
    struct Library *, MesaBase, 1056, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorMaskIndexedEXT(index, r, g, b, a);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBooleanIndexedvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLboolean *, data, A0),
    struct Library *, MesaBase, 1057, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBooleanIndexedvEXT(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetIntegerIndexedvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLint *, data, A0),
    struct Library *, MesaBase, 1058, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetIntegerIndexedvEXT(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEnableIndexedEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1059, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEnableIndexedEXT(target, index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDisableIndexedEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1060, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDisableIndexedEXT(target, index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLboolean, glIsEnabledIndexedEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1061, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsEnabledIndexedEXT(target, index);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginConditionalRenderNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 1062, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBeginConditionalRenderNV(id, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndConditionalRenderNV,
    struct Library *, MesaBase, 1063, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEndConditionalRenderNV();

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLenum, glObjectPurgeableAPPLE,
    AROS_LHA(GLenum, objectType, D0),
    AROS_LHA(GLuint, name, D1),
    AROS_LHA(GLenum, option, D2),
    struct Library *, MesaBase, 1064, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = mglObjectPurgeableAPPLE(objectType, name, option);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLenum, glObjectUnpurgeableAPPLE,
    AROS_LHA(GLenum, objectType, D0),
    AROS_LHA(GLuint, name, D1),
    AROS_LHA(GLenum, option, D2),
    struct Library *, MesaBase, 1065, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = mglObjectUnpurgeableAPPLE(objectType, name, option);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetObjectParameterivAPPLE,
    AROS_LHA(GLenum, objectType, D0),
    AROS_LHA(GLuint, name, D1),
    AROS_LHA(GLenum, pname, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1066, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetObjectParameterivAPPLE(objectType, name, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBeginTransformFeedback,
    AROS_LHA(GLenum, primitiveMode, D0),
    struct Library *, MesaBase, 1067, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBeginTransformFeedback(primitiveMode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndTransformFeedback,
    struct Library *, MesaBase, 1068, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEndTransformFeedback();

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glBindBufferRange,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    AROS_LHA(GLintptr, offset, D3),
    AROS_LHA(GLsizeiptr, size, D4),
    struct Library *, MesaBase, 1069, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindBufferRange(target, index, buffer, offset, size);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindBufferBase,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    struct Library *, MesaBase, 1070, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindBufferBase(target, index, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTransformFeedbackVaryings,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLchar *  *, varyings, A0),
    AROS_LHA(GLenum, bufferMode, D2),
    struct Library *, MesaBase, 1071, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTransformFeedbackVaryings(program, count, varyings, bufferMode);

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
    struct Library *, MesaBase, 1072, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glDrawArraysInstanced,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 1073, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawArraysInstanced(mode, first, count, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawElementsInstanced,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 1074, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawElementsInstanced(mode, count, type, indices, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glDrawArraysInstancedARB,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 1075, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawArraysInstancedARB(mode, first, count, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawElementsInstancedARB,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 1076, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawElementsInstancedARB(mode, count, type, indices, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramParameteriARB,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, value, D2),
    struct Library *, MesaBase, 1077, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramParameteriARB(program, pname, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glFramebufferTextureARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    struct Library *, MesaBase, 1078, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTextureARB(target, attachment, texture, level);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTextureFaceARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    AROS_LHA(GLenum, face, D4),
    struct Library *, MesaBase, 1079, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTextureFaceARB(target, attachment, texture, level, face);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindTransformFeedback,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 1080, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindTransformFeedback(target, id);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteTransformFeedbacks,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, ids, A0),
    struct Library *, MesaBase, 1081, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteTransformFeedbacks(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenTransformFeedbacks,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, ids, A0),
    struct Library *, MesaBase, 1082, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenTransformFeedbacks(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsTransformFeedback,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 1083, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsTransformFeedback(id);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPauseTransformFeedback,
    struct Library *, MesaBase, 1084, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPauseTransformFeedback();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glResumeTransformFeedback,
    struct Library *, MesaBase, 1085, Mesa)
{
    AROS_LIBFUNC_INIT

    mglResumeTransformFeedback();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawTransformFeedback,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 1086, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawTransformFeedback(mode, id);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glDrawArraysInstancedEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, start, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 1087, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawArraysInstancedEXT(mode, start, count, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glDrawElementsInstancedEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLsizei, primcount, D3),
    struct Library *, MesaBase, 1088, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawElementsInstancedEXT(mode, count, type, indices, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBeginTransformFeedbackEXT,
    AROS_LHA(GLenum, primitiveMode, D0),
    struct Library *, MesaBase, 1089, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBeginTransformFeedbackEXT(primitiveMode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndTransformFeedbackEXT,
    struct Library *, MesaBase, 1090, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEndTransformFeedbackEXT();

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glBindBufferRangeEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    AROS_LHA(GLintptr, offset, D3),
    AROS_LHA(GLsizeiptr, size, D4),
    struct Library *, MesaBase, 1091, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindBufferRangeEXT(target, index, buffer, offset, size);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBindBufferOffsetEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    AROS_LHA(GLintptr, offset, D3),
    struct Library *, MesaBase, 1092, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindBufferOffsetEXT(target, index, buffer, offset);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindBufferBaseEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    struct Library *, MesaBase, 1093, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindBufferBaseEXT(target, index, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glTransformFeedbackVaryingsEXT,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLchar *  *, varyings, A0),
    AROS_LHA(GLenum, bufferMode, D2),
    struct Library *, MesaBase, 1094, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTransformFeedbackVaryingsEXT(program, count, varyings, bufferMode);

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
    struct Library *, MesaBase, 1095, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTransformFeedbackVaryingEXT(program, index, bufSize, length, size, type, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEGLImageTargetTexture2DOES,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLeglImageOES, image, D1),
    struct Library *, MesaBase, 1096, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEGLImageTargetTexture2DOES(target, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEGLImageTargetRenderbufferStorageOES,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLeglImageOES, image, D1),
    struct Library *, MesaBase, 1097, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEGLImageTargetRenderbufferStorageOES(target, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glColorMaski,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLboolean, r, D1),
    AROS_LHA(GLboolean, g, D2),
    AROS_LHA(GLboolean, b, D3),
    AROS_LHA(GLboolean, a, D4),
    struct Library *, MesaBase, 1098, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorMaski(index, r, g, b, a);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBooleani_v,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLboolean *, data, A0),
    struct Library *, MesaBase, 1099, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBooleani_v(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetIntegeri_v,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLint *, data, A0),
    struct Library *, MesaBase, 1100, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetIntegeri_v(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEnablei,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1101, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEnablei(target, index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDisablei,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1102, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDisablei(target, index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLboolean, glIsEnabledi,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1103, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsEnabledi(target, index);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glClampColor,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, clamp, D1),
    struct Library *, MesaBase, 1104, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClampColor(target, clamp);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginConditionalRender,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 1105, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBeginConditionalRender(id, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndConditionalRender,
    struct Library *, MesaBase, 1106, Mesa)
{
    AROS_LIBFUNC_INIT

    mglEndConditionalRender();

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttribIPointer,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, size, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLsizei, stride, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 1107, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribIPointer(index, size, type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribIiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1108, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribIiv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribIuiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1109, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribIuiv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1i,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    struct Library *, MesaBase, 1110, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI1i(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribI2i,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    AROS_LHA(GLint, y, D2),
    struct Library *, MesaBase, 1111, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI2i(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttribI3i,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    AROS_LHA(GLint, y, D2),
    AROS_LHA(GLint, z, D3),
    struct Library *, MesaBase, 1112, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI3i(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttribI4i,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    AROS_LHA(GLint, y, D2),
    AROS_LHA(GLint, z, D3),
    AROS_LHA(GLint, w, D4),
    struct Library *, MesaBase, 1113, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4i(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1ui,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    struct Library *, MesaBase, 1114, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI1ui(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribI2ui,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    AROS_LHA(GLuint, y, D2),
    struct Library *, MesaBase, 1115, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI2ui(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttribI3ui,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    AROS_LHA(GLuint, y, D2),
    AROS_LHA(GLuint, z, D3),
    struct Library *, MesaBase, 1116, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI3ui(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttribI4ui,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    AROS_LHA(GLuint, y, D2),
    AROS_LHA(GLuint, z, D3),
    AROS_LHA(GLuint, w, D4),
    struct Library *, MesaBase, 1117, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4ui(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1118, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI1iv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI2iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1119, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI2iv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI3iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1120, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI3iv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1121, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4iv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1122, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI1uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI2uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1123, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI2uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI3uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1124, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI3uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1125, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4bv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 1126, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4bv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 1127, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4sv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4ubv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 1128, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4ubv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4usv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 1129, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4usv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformuiv,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1130, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetUniformuiv(program, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindFragDataLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, color, D1),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 1131, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindFragDataLocation(program, color, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetFragDataLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 1132, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = mglGetFragDataLocation(program, name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1ui,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    struct Library *, MesaBase, 1133, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1ui(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2ui,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    AROS_LHA(GLuint, v1, D2),
    struct Library *, MesaBase, 1134, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2ui(location, v0, v1);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3ui,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    AROS_LHA(GLuint, v1, D2),
    AROS_LHA(GLuint, v2, D3),
    struct Library *, MesaBase, 1135, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3ui(location, v0, v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4ui,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    AROS_LHA(GLuint, v1, D2),
    AROS_LHA(GLuint, v2, D3),
    AROS_LHA(GLuint, v3, D4),
    struct Library *, MesaBase, 1136, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4ui(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1uiv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1137, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1uiv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2uiv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1138, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2uiv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3uiv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1139, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3uiv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4uiv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1140, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4uiv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterIiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1141, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexParameterIiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterIuiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLuint *, params, A0),
    struct Library *, MesaBase, 1142, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexParameterIuiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterIiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1143, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexParameterIiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterIuiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1144, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexParameterIuiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glClearBufferiv,
    AROS_LHA(GLenum, buffer, D0),
    AROS_LHA(GLint, drawbuffer, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 1145, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearBufferiv(buffer, drawbuffer, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glClearBufferuiv,
    AROS_LHA(GLenum, buffer, D0),
    AROS_LHA(GLint, drawbuffer, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1146, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearBufferuiv(buffer, drawbuffer, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glClearBufferfv,
    AROS_LHA(GLenum, buffer, D0),
    AROS_LHA(GLint, drawbuffer, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 1147, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearBufferfv(buffer, drawbuffer, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glClearBufferfi,
    AROS_LHA(GLenum, buffer, D0),
    AROS_LHA(GLint, drawbuffer, D1),
    AROS_LHA(GLfloat, depth, D2),
    AROS_LHA(GLint, stencil, D3),
    struct Library *, MesaBase, 1148, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearBufferfi(buffer, drawbuffer, depth, stencil);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(const GLubyte *, glGetStringi,
    AROS_LHA(GLenum, name, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1149, Mesa)
{
    AROS_LIBFUNC_INIT

    const GLubyte * _return = mglGetStringi(name, index);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexBuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLuint, buffer, D2),
    struct Library *, MesaBase, 1150, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexBuffer(target, internalformat, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPrimitiveRestartIndex,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 1151, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPrimitiveRestartIndex(index);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetInteger64i_v,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLint64 *, data, A0),
    struct Library *, MesaBase, 1152, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetInteger64i_v(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferParameteri64v,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint64 *, params, A0),
    struct Library *, MesaBase, 1153, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetBufferParameteri64v(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glFramebufferTexture,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    struct Library *, MesaBase, 1154, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTexture(target, attachment, texture, level);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribDivisor,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, divisor, D1),
    struct Library *, MesaBase, 1155, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribDivisor(index, divisor);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPrimitiveRestartNV,
    struct Library *, MesaBase, 1156, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPrimitiveRestartNV();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPrimitiveRestartIndexNV,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 1157, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPrimitiveRestartIndexNV(index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1iEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    struct Library *, MesaBase, 1158, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI1iEXT(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribI2iEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    AROS_LHA(GLint, y, D2),
    struct Library *, MesaBase, 1159, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI2iEXT(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttribI3iEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    AROS_LHA(GLint, y, D2),
    AROS_LHA(GLint, z, D3),
    struct Library *, MesaBase, 1160, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI3iEXT(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttribI4iEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    AROS_LHA(GLint, y, D2),
    AROS_LHA(GLint, z, D3),
    AROS_LHA(GLint, w, D4),
    struct Library *, MesaBase, 1161, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4iEXT(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1uiEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    struct Library *, MesaBase, 1162, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI1uiEXT(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribI2uiEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    AROS_LHA(GLuint, y, D2),
    struct Library *, MesaBase, 1163, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI2uiEXT(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glVertexAttribI3uiEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    AROS_LHA(GLuint, y, D2),
    AROS_LHA(GLuint, z, D3),
    struct Library *, MesaBase, 1164, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI3uiEXT(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttribI4uiEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    AROS_LHA(GLuint, y, D2),
    AROS_LHA(GLuint, z, D3),
    AROS_LHA(GLuint, w, D4),
    struct Library *, MesaBase, 1165, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4uiEXT(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1ivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1166, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI1ivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI2ivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1167, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI2ivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI3ivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1168, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI3ivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4ivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1169, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4ivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1uivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1170, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI1uivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI2uivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1171, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI2uivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI3uivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1172, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI3uivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4uivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1173, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4uivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4bvEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 1174, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4bvEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4svEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 1175, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4svEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4ubvEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 1176, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4ubvEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4usvEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 1177, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribI4usvEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glVertexAttribIPointerEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, size, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLsizei, stride, D3),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 1178, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribIPointerEXT(index, size, type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribIivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1179, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribIivEXT(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribIuivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1180, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetVertexAttribIuivEXT(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformuivEXT,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1181, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetUniformuivEXT(program, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindFragDataLocationEXT,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, color, D1),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 1182, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindFragDataLocationEXT(program, color, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetFragDataLocationEXT,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 1183, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = mglGetFragDataLocationEXT(program, name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1uiEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    struct Library *, MesaBase, 1184, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1uiEXT(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2uiEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    AROS_LHA(GLuint, v1, D2),
    struct Library *, MesaBase, 1185, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2uiEXT(location, v0, v1);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glUniform3uiEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    AROS_LHA(GLuint, v1, D2),
    AROS_LHA(GLuint, v2, D3),
    struct Library *, MesaBase, 1186, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3uiEXT(location, v0, v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glUniform4uiEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    AROS_LHA(GLuint, v1, D2),
    AROS_LHA(GLuint, v2, D3),
    AROS_LHA(GLuint, v3, D4),
    struct Library *, MesaBase, 1187, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4uiEXT(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1uivEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1188, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform1uivEXT(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2uivEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1189, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform2uivEXT(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3uivEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1190, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform3uivEXT(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4uivEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1191, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUniform4uivEXT(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterIivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1192, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexParameterIivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterIuivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLuint *, params, A0),
    struct Library *, MesaBase, 1193, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexParameterIuivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterIivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1194, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexParameterIivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterIuivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1195, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexParameterIuivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glClearColorIiEXT,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    AROS_LHA(GLint, alpha, D3),
    struct Library *, MesaBase, 1196, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearColorIiEXT(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glClearColorIuiEXT,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    AROS_LHA(GLuint, alpha, D3),
    struct Library *, MesaBase, 1197, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearColorIuiEXT(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUseShaderProgramEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLuint, program, D1),
    struct Library *, MesaBase, 1198, Mesa)
{
    AROS_LIBFUNC_INIT

    mglUseShaderProgramEXT(type, program);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveProgramEXT,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 1199, Mesa)
{
    AROS_LIBFUNC_INIT

    mglActiveProgramEXT(program);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLuint, glCreateShaderProgramEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(const GLchar *, string, A0),
    struct Library *, MesaBase, 1200, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = mglCreateShaderProgramEXT(type, string);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramEnvParameters4fvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1201, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramEnvParameters4fvEXT(target, index, count, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glProgramLocalParameters4fvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1202, Mesa)
{
    AROS_LIBFUNC_INIT

    mglProgramLocalParameters4fvEXT(target, index, count, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationSeparateATI,
    AROS_LHA(GLenum, modeRGB, D0),
    AROS_LHA(GLenum, modeA, D1),
    struct Library *, MesaBase, 1203, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendEquationSeparateATI(modeRGB, modeA);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetHistogramEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLboolean, reset, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(GLvoid *, values, A0),
    struct Library *, MesaBase, 1204, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetHistogramEXT(target, reset, format, type, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1205, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetHistogramParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1206, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetHistogramParameterivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetMinmaxEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLboolean, reset, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(GLvoid *, values, A0),
    struct Library *, MesaBase, 1207, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMinmaxEXT(target, reset, format, type, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1208, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMinmaxParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1209, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetMinmaxParameterivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glHistogramEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, width, D1),
    AROS_LHA(GLenum, internalformat, D2),
    AROS_LHA(GLboolean, sink, D3),
    struct Library *, MesaBase, 1210, Mesa)
{
    AROS_LIBFUNC_INIT

    mglHistogramEXT(target, width, internalformat, sink);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMinmaxEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLboolean, sink, D2),
    struct Library *, MesaBase, 1211, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMinmaxEXT(target, internalformat, sink);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetHistogramEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 1212, Mesa)
{
    AROS_LIBFUNC_INIT

    mglResetHistogramEXT(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetMinmaxEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 1213, Mesa)
{
    AROS_LIBFUNC_INIT

    mglResetMinmaxEXT(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glConvolutionFilter1DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, image, A0),
    struct Library *, MesaBase, 1214, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionFilter1DEXT(target, internalformat, width, format, type, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, glConvolutionFilter2DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(const GLvoid *, image, A0),
    struct Library *, MesaBase, 1215, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionFilter2DEXT(target, internalformat, width, height, format, type, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterfEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, params, D2),
    struct Library *, MesaBase, 1216, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionParameterfEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1217, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameteriEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, params, D2),
    struct Library *, MesaBase, 1218, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionParameteriEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1219, Mesa)
{
    AROS_LIBFUNC_INIT

    mglConvolutionParameterivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyConvolutionFilter1DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    struct Library *, MesaBase, 1220, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyConvolutionFilter1DEXT(target, internalformat, x, y, width);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glCopyConvolutionFilter2DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    AROS_LHA(GLsizei, height, D5),
    struct Library *, MesaBase, 1221, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyConvolutionFilter2DEXT(target, internalformat, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetConvolutionFilterEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, image, A0),
    struct Library *, MesaBase, 1222, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetConvolutionFilterEXT(target, format, type, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1223, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetConvolutionParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1224, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetConvolutionParameterivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glGetSeparableFilterEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, row, A0),
    AROS_LHA(GLvoid *, column, A1),
    AROS_LHA(GLvoid *, span, A2),
    struct Library *, MesaBase, 1225, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetSeparableFilterEXT(target, format, type, row, column, span);

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glSeparableFilter2DEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(const GLvoid *, row, A0),
    AROS_LHA(const GLvoid *, column, A1),
    struct Library *, MesaBase, 1226, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSeparableFilter2DEXT(target, internalformat, width, height, format, type, row, column);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glColorTableSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, table, A0),
    struct Library *, MesaBase, 1227, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorTableSGI(target, internalformat, width, format, type, table);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameterfvSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1228, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorTableParameterfvSGI(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameterivSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1229, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorTableParameterivSGI(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyColorTableSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    struct Library *, MesaBase, 1230, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyColorTableSGI(target, internalformat, x, y, width);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetColorTableSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLvoid *, table, A0),
    struct Library *, MesaBase, 1231, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetColorTableSGI(target, format, type, table);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterfvSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1232, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetColorTableParameterfvSGI(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterivSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1233, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetColorTableParameterivSGI(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPixelTexGenSGIX,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 1234, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelTexGenSGIX(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTexGenParameteriSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 1235, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelTexGenParameteriSGIS(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTexGenParameterivSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1236, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelTexGenParameterivSGIS(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTexGenParameterfSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 1237, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelTexGenParameterfSGIS(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTexGenParameterfvSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1238, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPixelTexGenParameterfvSGIS(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelTexGenParameterivSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1239, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetPixelTexGenParameterivSGIS(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelTexGenParameterfvSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1240, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetPixelTexGenParameterfvSGIS(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleMaskSGIS,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 1241, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSampleMaskSGIS(value, invert);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSamplePatternSGIS,
    AROS_LHA(GLenum, pattern, D0),
    struct Library *, MesaBase, 1242, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSamplePatternSGIS(pattern);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 1243, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameterfSGIS(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfvSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1244, Mesa)
{
    AROS_LIBFUNC_INIT

    mglPointParameterfvSGIS(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glColorSubTableEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, start, D1),
    AROS_LHA(GLsizei, count, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const GLvoid *, data, A0),
    struct Library *, MesaBase, 1245, Mesa)
{
    AROS_LIBFUNC_INIT

    mglColorSubTableEXT(target, start, count, format, type, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glCopyColorSubTableEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, start, D1),
    AROS_LHA(GLint, x, D2),
    AROS_LHA(GLint, y, D3),
    AROS_LHA(GLsizei, width, D4),
    struct Library *, MesaBase, 1246, Mesa)
{
    AROS_LIBFUNC_INIT

    mglCopyColorSubTableEXT(target, start, x, y, width);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glBlendFuncSeparateINGR,
    AROS_LHA(GLenum, sfactorRGB, D0),
    AROS_LHA(GLenum, dfactorRGB, D1),
    AROS_LHA(GLenum, sfactorAlpha, D2),
    AROS_LHA(GLenum, dfactorAlpha, D3),
    struct Library *, MesaBase, 1247, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendFuncSeparateINGR(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glMultiModeDrawArraysIBM,
    AROS_LHA(const GLenum *, mode, A0),
    AROS_LHA(const GLint *, first, A1),
    AROS_LHA(const GLsizei *, count, A2),
    AROS_LHA(GLsizei, primcount, D0),
    AROS_LHA(GLint, modestride, D1),
    struct Library *, MesaBase, 1248, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiModeDrawArraysIBM(mode, first, count, primcount, modestride);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glMultiModeDrawElementsIBM,
    AROS_LHA(const GLenum *, mode, A0),
    AROS_LHA(const GLsizei *, count, A1),
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(const GLvoid * const *, indices, A2),
    AROS_LHA(GLsizei, primcount, D1),
    AROS_LHA(GLint, modestride, D2),
    struct Library *, MesaBase, 1249, Mesa)
{
    AROS_LIBFUNC_INIT

    mglMultiModeDrawElementsIBM(mode, count, type, indices, primcount, modestride);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleMaskEXT,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 1250, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSampleMaskEXT(value, invert);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSamplePatternEXT,
    AROS_LHA(GLenum, pattern, D0),
    struct Library *, MesaBase, 1251, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSamplePatternEXT(pattern);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteFencesNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, fences, A0),
    struct Library *, MesaBase, 1252, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteFencesNV(n, fences);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenFencesNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, fences, A0),
    struct Library *, MesaBase, 1253, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenFencesNV(n, fences);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsFenceNV,
    AROS_LHA(GLuint, fence, D0),
    struct Library *, MesaBase, 1254, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsFenceNV(fence);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glTestFenceNV,
    AROS_LHA(GLuint, fence, D0),
    struct Library *, MesaBase, 1255, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglTestFenceNV(fence);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetFenceivNV,
    AROS_LHA(GLuint, fence, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1256, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetFenceivNV(fence, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFinishFenceNV,
    AROS_LHA(GLuint, fence, D0),
    struct Library *, MesaBase, 1257, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFinishFenceNV(fence);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSetFenceNV,
    AROS_LHA(GLuint, fence, D0),
    AROS_LHA(GLenum, condition, D1),
    struct Library *, MesaBase, 1258, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSetFenceNV(fence, condition);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveStencilFaceEXT,
    AROS_LHA(GLenum, face, D0),
    struct Library *, MesaBase, 1259, Mesa)
{
    AROS_LIBFUNC_INIT

    mglActiveStencilFaceEXT(face);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBindVertexArrayAPPLE,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 1260, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindVertexArrayAPPLE(array);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteVertexArraysAPPLE,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, arrays, A0),
    struct Library *, MesaBase, 1261, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteVertexArraysAPPLE(n, arrays);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenVertexArraysAPPLE,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, arrays, A0),
    struct Library *, MesaBase, 1262, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenVertexArraysAPPLE(n, arrays);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsVertexArrayAPPLE,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 1263, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsVertexArrayAPPLE(array);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glStencilOpSeparateATI,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, sfail, D1),
    AROS_LHA(GLenum, dpfail, D2),
    AROS_LHA(GLenum, dppass, D3),
    struct Library *, MesaBase, 1264, Mesa)
{
    AROS_LIBFUNC_INIT

    mglStencilOpSeparateATI(face, sfail, dpfail, dppass);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glStencilFuncSeparateATI,
    AROS_LHA(GLenum, frontfunc, D0),
    AROS_LHA(GLenum, backfunc, D1),
    AROS_LHA(GLint, ref, D2),
    AROS_LHA(GLuint, mask, D3),
    struct Library *, MesaBase, 1265, Mesa)
{
    AROS_LIBFUNC_INIT

    mglStencilFuncSeparateATI(frontfunc, backfunc, ref, mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDepthBoundsEXT,
    AROS_LHA(GLclampd, zmin, D0),
    AROS_LHA(GLclampd, zmax, D1),
    struct Library *, MesaBase, 1266, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDepthBoundsEXT(zmin, zmax);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationSeparateEXT,
    AROS_LHA(GLenum, modeRGB, D0),
    AROS_LHA(GLenum, modeAlpha, D1),
    struct Library *, MesaBase, 1267, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendEquationSeparateEXT(modeRGB, modeAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH10(void, glBlitFramebufferEXT,
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
    struct Library *, MesaBase, 1268, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjecti64vEXT,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint64EXT *, params, A0),
    struct Library *, MesaBase, 1269, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetQueryObjecti64vEXT(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectui64vEXT,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint64EXT *, params, A0),
    struct Library *, MesaBase, 1270, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetQueryObjectui64vEXT(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBufferParameteriAPPLE,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 1271, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBufferParameteriAPPLE(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFlushMappedBufferRangeAPPLE,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, size, D2),
    struct Library *, MesaBase, 1272, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFlushMappedBufferRangeAPPLE(target, offset, size);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTextureRangeAPPLE,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, length, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 1273, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTextureRangeAPPLE(target, length, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterPointervAPPLE,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 1274, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetTexParameterPointervAPPLE(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glClampColorARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, clamp, D1),
    struct Library *, MesaBase, 1275, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClampColorARB(target, clamp);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glFramebufferTextureLayerARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, attachment, D1),
    AROS_LHA(GLuint, texture, D2),
    AROS_LHA(GLint, level, D3),
    AROS_LHA(GLint, layer, D4),
    struct Library *, MesaBase, 1276, Mesa)
{
    AROS_LIBFUNC_INIT

    mglFramebufferTextureLayerARB(target, attachment, texture, level, layer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribDivisorARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, divisor, D1),
    struct Library *, MesaBase, 1277, Mesa)
{
    AROS_LIBFUNC_INIT

    mglVertexAttribDivisorARB(index, divisor);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexBufferARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLuint, buffer, D2),
    struct Library *, MesaBase, 1278, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTexBufferARB(target, internalformat, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glDrawElementsInstancedBaseVertex,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(const GLvoid *, indices, A0),
    AROS_LHA(GLsizei, primcount, D3),
    AROS_LHA(GLint, basevertex, D4),
    struct Library *, MesaBase, 1279, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDrawElementsInstancedBaseVertex(mode, count, type, indices, primcount, basevertex);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationiARB,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 1280, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendEquationiARB(buf, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBlendEquationSeparateiARB,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, modeRGB, D1),
    AROS_LHA(GLenum, modeAlpha, D2),
    struct Library *, MesaBase, 1281, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendEquationSeparateiARB(buf, modeRGB, modeAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBlendFunciARB,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, src, D1),
    AROS_LHA(GLenum, dst, D2),
    struct Library *, MesaBase, 1282, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendFunciARB(buf, src, dst);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glBlendFuncSeparateiARB,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, srcRGB, D1),
    AROS_LHA(GLenum, dstRGB, D2),
    AROS_LHA(GLenum, srcAlpha, D3),
    AROS_LHA(GLenum, dstAlpha, D4),
    struct Library *, MesaBase, 1283, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendFuncSeparateiARB(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenSamplers,
    AROS_LHA(GLsizei, count, D0),
    AROS_LHA(GLuint *, samplers, A0),
    struct Library *, MesaBase, 1284, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGenSamplers(count, samplers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteSamplers,
    AROS_LHA(GLsizei, count, D0),
    AROS_LHA(const GLuint *, samplers, A0),
    struct Library *, MesaBase, 1285, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDeleteSamplers(count, samplers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsSampler,
    AROS_LHA(GLuint, sampler, D0),
    struct Library *, MesaBase, 1286, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mglIsSampler(sampler);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindSampler,
    AROS_LHA(GLuint, unit, D0),
    AROS_LHA(GLuint, sampler, D1),
    struct Library *, MesaBase, 1287, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBindSampler(unit, sampler);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameteri,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 1288, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSamplerParameteri(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameteriv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, param, A0),
    struct Library *, MesaBase, 1289, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSamplerParameteriv(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameterf,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 1290, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSamplerParameterf(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameterfv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, param, A0),
    struct Library *, MesaBase, 1291, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSamplerParameterfv(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameterIiv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, param, A0),
    struct Library *, MesaBase, 1292, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSamplerParameterIiv(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameterIuiv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLuint *, param, A0),
    struct Library *, MesaBase, 1293, Mesa)
{
    AROS_LIBFUNC_INIT

    mglSamplerParameterIuiv(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetSamplerParameteriv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1294, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetSamplerParameteriv(sampler, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetSamplerParameterIiv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1295, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetSamplerParameterIiv(sampler, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetSamplerParameterfv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1296, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetSamplerParameterfv(sampler, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetSamplerParameterIuiv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1297, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetSamplerParameterIuiv(sampler, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glReleaseShaderCompiler,
    struct Library *, MesaBase, 1298, Mesa)
{
    AROS_LIBFUNC_INIT

    mglReleaseShaderCompiler();

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glShaderBinary,
    AROS_LHA(GLsizei, count, D0),
    AROS_LHA(const GLuint *, shaders, A0),
    AROS_LHA(GLenum, binaryformat, D1),
    AROS_LHA(const GLvoid *, binary, A1),
    AROS_LHA(GLsizei, length, D2),
    struct Library *, MesaBase, 1299, Mesa)
{
    AROS_LIBFUNC_INIT

    mglShaderBinary(count, shaders, binaryformat, binary, length);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetShaderPrecisionFormat,
    AROS_LHA(GLenum, shadertype, D0),
    AROS_LHA(GLenum, precisiontype, D1),
    AROS_LHA(GLint *, range, A0),
    AROS_LHA(GLint *, precision, A1),
    struct Library *, MesaBase, 1300, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDepthRangef,
    AROS_LHA(GLclampf, n, D0),
    AROS_LHA(GLclampf, f, D1),
    struct Library *, MesaBase, 1301, Mesa)
{
    AROS_LIBFUNC_INIT

    mglDepthRangef(n, f);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClearDepthf,
    AROS_LHA(GLclampf, d, D0),
    struct Library *, MesaBase, 1302, Mesa)
{
    AROS_LIBFUNC_INIT

    mglClearDepthf(d);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLenum, glGetGraphicsResetStatusARB,
    struct Library *, MesaBase, 1303, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = mglGetGraphicsResetStatusARB();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetnMapdvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLdouble *, v, A0),
    struct Library *, MesaBase, 1304, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnMapdvARB(target, query, bufSize, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetnMapfvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLfloat *, v, A0),
    struct Library *, MesaBase, 1305, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnMapfvARB(target, query, bufSize, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetnMapivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLint *, v, A0),
    struct Library *, MesaBase, 1306, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnMapivARB(target, query, bufSize, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetnPixelMapfvARB,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLfloat *, values, A0),
    struct Library *, MesaBase, 1307, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnPixelMapfvARB(map, bufSize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetnPixelMapuivARB,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLuint *, values, A0),
    struct Library *, MesaBase, 1308, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnPixelMapuivARB(map, bufSize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetnPixelMapusvARB,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLushort *, values, A0),
    struct Library *, MesaBase, 1309, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnPixelMapusvARB(map, bufSize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetnPolygonStippleARB,
    AROS_LHA(GLsizei, bufSize, D0),
    AROS_LHA(GLubyte *, pattern, A0),
    struct Library *, MesaBase, 1310, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnPolygonStippleARB(bufSize, pattern);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetnColorTableARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLsizei, bufSize, D3),
    AROS_LHA(GLvoid *, table, A0),
    struct Library *, MesaBase, 1311, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnColorTableARB(target, format, type, bufSize, table);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glGetnConvolutionFilterARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLsizei, bufSize, D3),
    AROS_LHA(GLvoid *, image, A0),
    struct Library *, MesaBase, 1312, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnConvolutionFilterARB(target, format, type, bufSize, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glGetnSeparableFilterARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, format, D1),
    AROS_LHA(GLenum, type, D2),
    AROS_LHA(GLsizei, rowBufSize, D3),
    AROS_LHA(GLvoid *, row, A0),
    AROS_LHA(GLsizei, columnBufSize, D4),
    AROS_LHA(GLvoid *, column, A1),
    AROS_LHA(GLvoid *, span, A2),
    struct Library *, MesaBase, 1313, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnSeparableFilterARB(target, format, type, rowBufSize, row, columnBufSize, column, span);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glGetnHistogramARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLboolean, reset, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(GLsizei, bufSize, D4),
    AROS_LHA(GLvoid *, values, A0),
    struct Library *, MesaBase, 1314, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnHistogramARB(target, reset, format, type, bufSize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glGetnMinmaxARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLboolean, reset, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(GLsizei, bufSize, D4),
    AROS_LHA(GLvoid *, values, A0),
    struct Library *, MesaBase, 1315, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnMinmaxARB(target, reset, format, type, bufSize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, glGetnTexImageARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLenum, format, D2),
    AROS_LHA(GLenum, type, D3),
    AROS_LHA(GLsizei, bufSize, D4),
    AROS_LHA(GLvoid *, img, A0),
    struct Library *, MesaBase, 1316, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnTexImageARB(target, level, format, type, bufSize, img);

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, glReadnPixelsARB,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(GLsizei, bufSize, D6),
    AROS_LHA(GLvoid *, data, A0),
    struct Library *, MesaBase, 1317, Mesa)
{
    AROS_LIBFUNC_INIT

    mglReadnPixelsARB(x, y, width, height, format, type, bufSize, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetnCompressedTexImageARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, lod, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLvoid *, img, A0),
    struct Library *, MesaBase, 1318, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnCompressedTexImageARB(target, lod, bufSize, img);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetnUniformfvARB,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1319, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnUniformfvARB(program, location, bufSize, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetnUniformivARB,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1320, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnUniformivARB(program, location, bufSize, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetnUniformuivARB,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1321, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnUniformuivARB(program, location, bufSize, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, glGetnUniformdvARB,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLsizei, bufSize, D2),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 1322, Mesa)
{
    AROS_LIBFUNC_INIT

    mglGetnUniformdvARB(program, location, bufSize, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBlendFuncIndexedAMD,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, src, D1),
    AROS_LHA(GLenum, dst, D2),
    struct Library *, MesaBase, 1323, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendFuncIndexedAMD(buf, src, dst);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, glBlendFuncSeparateIndexedAMD,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, srcRGB, D1),
    AROS_LHA(GLenum, dstRGB, D2),
    AROS_LHA(GLenum, srcAlpha, D3),
    AROS_LHA(GLenum, dstAlpha, D4),
    struct Library *, MesaBase, 1324, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendFuncSeparateIndexedAMD(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationIndexedAMD,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 1325, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendEquationIndexedAMD(buf, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBlendEquationSeparateIndexedAMD,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, modeRGB, D1),
    AROS_LHA(GLenum, modeAlpha, D2),
    struct Library *, MesaBase, 1326, Mesa)
{
    AROS_LIBFUNC_INIT

    mglBlendEquationSeparateIndexedAMD(buf, modeRGB, modeAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glTextureBarrierNV,
    struct Library *, MesaBase, 1327, Mesa)
{
    AROS_LIBFUNC_INIT

    mglTextureBarrierNV();

    AROS_LIBFUNC_EXIT
}

