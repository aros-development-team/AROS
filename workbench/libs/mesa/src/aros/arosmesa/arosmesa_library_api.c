/*
    Copyright 2009-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glext.h>
#include <exec/libraries.h>
#include <aros/libcall.h>

AROS_LH1(void, glClearIndex,
    AROS_LHA(GLfloat, c, D0),
    struct Library *, MesaBase, 35, Mesa)
{
    AROS_LIBFUNC_INIT

    glClearIndex(c);

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

    glClearColor(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClear,
    AROS_LHA(GLbitfield, mask, D0),
    struct Library *, MesaBase, 37, Mesa)
{
    AROS_LIBFUNC_INIT

    glClear(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexMask,
    AROS_LHA(GLuint, mask, D0),
    struct Library *, MesaBase, 38, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexMask(mask);

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

    glColorMask(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAlphaFunc,
    AROS_LHA(GLenum, func, D0),
    AROS_LHA(GLclampf, ref, D1),
    struct Library *, MesaBase, 40, Mesa)
{
    AROS_LIBFUNC_INIT

    glAlphaFunc(func, ref);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendFunc,
    AROS_LHA(GLenum, sfactor, D0),
    AROS_LHA(GLenum, dfactor, D1),
    struct Library *, MesaBase, 41, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendFunc(sfactor, dfactor);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLogicOp,
    AROS_LHA(GLenum, opcode, D0),
    struct Library *, MesaBase, 42, Mesa)
{
    AROS_LIBFUNC_INIT

    glLogicOp(opcode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCullFace,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 43, Mesa)
{
    AROS_LIBFUNC_INIT

    glCullFace(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFrontFace,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 44, Mesa)
{
    AROS_LIBFUNC_INIT

    glFrontFace(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPointSize,
    AROS_LHA(GLfloat, size, D0),
    struct Library *, MesaBase, 45, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointSize(size);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLineWidth,
    AROS_LHA(GLfloat, width, D0),
    struct Library *, MesaBase, 46, Mesa)
{
    AROS_LIBFUNC_INIT

    glLineWidth(width);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLineStipple,
    AROS_LHA(GLint, factor, D0),
    AROS_LHA(GLushort, pattern, D1),
    struct Library *, MesaBase, 47, Mesa)
{
    AROS_LIBFUNC_INIT

    glLineStipple(factor, pattern);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPolygonMode,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 48, Mesa)
{
    AROS_LIBFUNC_INIT

    glPolygonMode(face, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPolygonOffset,
    AROS_LHA(GLfloat, factor, D0),
    AROS_LHA(GLfloat, units, D1),
    struct Library *, MesaBase, 49, Mesa)
{
    AROS_LIBFUNC_INIT

    glPolygonOffset(factor, units);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPolygonStipple,
    AROS_LHA(const GLubyte *, mask, A0),
    struct Library *, MesaBase, 50, Mesa)
{
    AROS_LIBFUNC_INIT

    glPolygonStipple(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glGetPolygonStipple,
    AROS_LHA(GLubyte *, mask, A0),
    struct Library *, MesaBase, 51, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetPolygonStipple(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEdgeFlag,
    AROS_LHA(GLboolean, flag, D0),
    struct Library *, MesaBase, 52, Mesa)
{
    AROS_LIBFUNC_INIT

    glEdgeFlag(flag);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEdgeFlagv,
    AROS_LHA(const GLboolean *, flag, A0),
    struct Library *, MesaBase, 53, Mesa)
{
    AROS_LIBFUNC_INIT

    glEdgeFlagv(flag);

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

    glScissor(x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glClipPlane,
    AROS_LHA(GLenum, plane, D0),
    AROS_LHA(const GLdouble *, equation, A0),
    struct Library *, MesaBase, 55, Mesa)
{
    AROS_LIBFUNC_INIT

    glClipPlane(plane, equation);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetClipPlane,
    AROS_LHA(GLenum, plane, D0),
    AROS_LHA(GLdouble *, equation, A0),
    struct Library *, MesaBase, 56, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetClipPlane(plane, equation);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDrawBuffer,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 57, Mesa)
{
    AROS_LIBFUNC_INIT

    glDrawBuffer(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glReadBuffer,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 58, Mesa)
{
    AROS_LIBFUNC_INIT

    glReadBuffer(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnable,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 59, Mesa)
{
    AROS_LIBFUNC_INIT

    glEnable(cap);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisable,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 60, Mesa)
{
    AROS_LIBFUNC_INIT

    glDisable(cap);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsEnabled,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 61, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsEnabled(cap);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnableClientState,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 62, Mesa)
{
    AROS_LIBFUNC_INIT

    glEnableClientState(cap);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisableClientState,
    AROS_LHA(GLenum, cap, D0),
    struct Library *, MesaBase, 63, Mesa)
{
    AROS_LIBFUNC_INIT

    glDisableClientState(cap);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetBooleanv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLboolean *, params, A0),
    struct Library *, MesaBase, 64, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetBooleanv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetDoublev,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 65, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetDoublev(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetFloatv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 66, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetFloatv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetIntegerv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 67, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetIntegerv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPushAttrib,
    AROS_LHA(GLbitfield, mask, D0),
    struct Library *, MesaBase, 68, Mesa)
{
    AROS_LIBFUNC_INIT

    glPushAttrib(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopAttrib,
    struct Library *, MesaBase, 69, Mesa)
{
    AROS_LIBFUNC_INIT

    glPopAttrib();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPushClientAttrib,
    AROS_LHA(GLbitfield, mask, D0),
    struct Library *, MesaBase, 70, Mesa)
{
    AROS_LIBFUNC_INIT

    glPushClientAttrib(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopClientAttrib,
    struct Library *, MesaBase, 71, Mesa)
{
    AROS_LIBFUNC_INIT

    glPopClientAttrib();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLint, glRenderMode,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 72, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = glRenderMode(mode);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLenum, glGetError,
    struct Library *, MesaBase, 73, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = glGetError();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(const GLubyte *, glGetString,
    AROS_LHA(GLenum, name, D0),
    struct Library *, MesaBase, 74, Mesa)
{
    AROS_LIBFUNC_INIT

    const GLubyte * _return = glGetString(name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glFinish,
    struct Library *, MesaBase, 75, Mesa)
{
    AROS_LIBFUNC_INIT

    glFinish();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glFlush,
    struct Library *, MesaBase, 76, Mesa)
{
    AROS_LIBFUNC_INIT

    glFlush();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glHint,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 77, Mesa)
{
    AROS_LIBFUNC_INIT

    glHint(target, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClearDepth,
    AROS_LHA(GLclampd, depth, D0),
    struct Library *, MesaBase, 78, Mesa)
{
    AROS_LIBFUNC_INIT

    glClearDepth(depth);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDepthFunc,
    AROS_LHA(GLenum, func, D0),
    struct Library *, MesaBase, 79, Mesa)
{
    AROS_LIBFUNC_INIT

    glDepthFunc(func);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDepthMask,
    AROS_LHA(GLboolean, flag, D0),
    struct Library *, MesaBase, 80, Mesa)
{
    AROS_LIBFUNC_INIT

    glDepthMask(flag);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDepthRange,
    AROS_LHA(GLclampd, near_val, D0),
    AROS_LHA(GLclampd, far_val, D1),
    struct Library *, MesaBase, 81, Mesa)
{
    AROS_LIBFUNC_INIT

    glDepthRange(near_val, far_val);

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

    glClearAccum(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAccum,
    AROS_LHA(GLenum, op, D0),
    AROS_LHA(GLfloat, value, D1),
    struct Library *, MesaBase, 83, Mesa)
{
    AROS_LIBFUNC_INIT

    glAccum(op, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMatrixMode,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 84, Mesa)
{
    AROS_LIBFUNC_INIT

    glMatrixMode(mode);

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

    glOrtho(left, right, bottom, top, near_val, far_val);

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

    glFrustum(left, right, bottom, top, near_val, far_val);

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

    glViewport(x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPushMatrix,
    struct Library *, MesaBase, 88, Mesa)
{
    AROS_LIBFUNC_INIT

    glPushMatrix();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopMatrix,
    struct Library *, MesaBase, 89, Mesa)
{
    AROS_LIBFUNC_INIT

    glPopMatrix();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glLoadIdentity,
    struct Library *, MesaBase, 90, Mesa)
{
    AROS_LIBFUNC_INIT

    glLoadIdentity();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 91, Mesa)
{
    AROS_LIBFUNC_INIT

    glLoadMatrixd(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 92, Mesa)
{
    AROS_LIBFUNC_INIT

    glLoadMatrixf(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 93, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultMatrixd(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 94, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultMatrixf(m);

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

    glRotated(angle, x, y, z);

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

    glRotatef(angle, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glScaled,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 97, Mesa)
{
    AROS_LIBFUNC_INIT

    glScaled(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glScalef,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 98, Mesa)
{
    AROS_LIBFUNC_INIT

    glScalef(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTranslated,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 99, Mesa)
{
    AROS_LIBFUNC_INIT

    glTranslated(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTranslatef,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 100, Mesa)
{
    AROS_LIBFUNC_INIT

    glTranslatef(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsList,
    AROS_LHA(GLuint, list, D0),
    struct Library *, MesaBase, 101, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsList(list);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteLists,
    AROS_LHA(GLuint, list, D0),
    AROS_LHA(GLsizei, range, D1),
    struct Library *, MesaBase, 102, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteLists(list, range);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLuint, glGenLists,
    AROS_LHA(GLsizei, range, D0),
    struct Library *, MesaBase, 103, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = glGenLists(range);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glNewList,
    AROS_LHA(GLuint, list, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 104, Mesa)
{
    AROS_LIBFUNC_INIT

    glNewList(list, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndList,
    struct Library *, MesaBase, 105, Mesa)
{
    AROS_LIBFUNC_INIT

    glEndList();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCallList,
    AROS_LHA(GLuint, list, D0),
    struct Library *, MesaBase, 106, Mesa)
{
    AROS_LIBFUNC_INIT

    glCallList(list);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glCallLists,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(const GLvoid *, lists, A0),
    struct Library *, MesaBase, 107, Mesa)
{
    AROS_LIBFUNC_INIT

    glCallLists(n, type, lists);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glListBase,
    AROS_LHA(GLuint, base, D0),
    struct Library *, MesaBase, 108, Mesa)
{
    AROS_LIBFUNC_INIT

    glListBase(base);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBegin,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 109, Mesa)
{
    AROS_LIBFUNC_INIT

    glBegin(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEnd,
    struct Library *, MesaBase, 110, Mesa)
{
    AROS_LIBFUNC_INIT

    glEnd();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 111, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex2d(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 112, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex2f(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 113, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex2i(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertex2s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 114, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex2s(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 115, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex3d(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 116, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex3f(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 117, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex3i(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertex3s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 118, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex3s(x, y, z);

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

    glVertex4d(x, y, z, w);

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

    glVertex4f(x, y, z, w);

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

    glVertex4i(x, y, z, w);

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

    glVertex4s(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 123, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex2dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 124, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex2fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 125, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex2iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 126, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex2sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 127, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 128, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 129, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 130, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 131, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex4dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 132, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex4fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 133, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex4iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glVertex4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 134, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertex4sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3b,
    AROS_LHA(GLbyte, nx, D0),
    AROS_LHA(GLbyte, ny, D1),
    AROS_LHA(GLbyte, nz, D2),
    struct Library *, MesaBase, 135, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3b(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3d,
    AROS_LHA(GLdouble, nx, D0),
    AROS_LHA(GLdouble, ny, D1),
    AROS_LHA(GLdouble, nz, D2),
    struct Library *, MesaBase, 136, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3d(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3f,
    AROS_LHA(GLfloat, nx, D0),
    AROS_LHA(GLfloat, ny, D1),
    AROS_LHA(GLfloat, nz, D2),
    struct Library *, MesaBase, 137, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3f(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3i,
    AROS_LHA(GLint, nx, D0),
    AROS_LHA(GLint, ny, D1),
    AROS_LHA(GLint, nz, D2),
    struct Library *, MesaBase, 138, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3i(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormal3s,
    AROS_LHA(GLshort, nx, D0),
    AROS_LHA(GLshort, ny, D1),
    AROS_LHA(GLshort, nz, D2),
    struct Library *, MesaBase, 139, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3s(nx, ny, nz);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 140, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3bv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 141, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 142, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 143, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glNormal3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 144, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormal3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexd,
    AROS_LHA(GLdouble, c, D0),
    struct Library *, MesaBase, 145, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexd(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexf,
    AROS_LHA(GLfloat, c, D0),
    struct Library *, MesaBase, 146, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexf(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexi,
    AROS_LHA(GLint, c, D0),
    struct Library *, MesaBase, 147, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexi(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexs,
    AROS_LHA(GLshort, c, D0),
    struct Library *, MesaBase, 148, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexs(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexub,
    AROS_LHA(GLubyte, c, D0),
    struct Library *, MesaBase, 149, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexub(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexdv,
    AROS_LHA(const GLdouble *, c, A0),
    struct Library *, MesaBase, 150, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexdv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexfv,
    AROS_LHA(const GLfloat *, c, A0),
    struct Library *, MesaBase, 151, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexfv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexiv,
    AROS_LHA(const GLint *, c, A0),
    struct Library *, MesaBase, 152, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexiv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexsv,
    AROS_LHA(const GLshort *, c, A0),
    struct Library *, MesaBase, 153, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexsv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glIndexubv,
    AROS_LHA(const GLubyte *, c, A0),
    struct Library *, MesaBase, 154, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexubv(c);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3b,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    struct Library *, MesaBase, 155, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3b(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3d,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    struct Library *, MesaBase, 156, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3d(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3f,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    struct Library *, MesaBase, 157, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3f(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3i,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    struct Library *, MesaBase, 158, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3i(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3s,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    struct Library *, MesaBase, 159, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3s(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3ub,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    struct Library *, MesaBase, 160, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3ub(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3ui,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    struct Library *, MesaBase, 161, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3ui(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColor3us,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    struct Library *, MesaBase, 162, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3us(red, green, blue);

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

    glColor4b(red, green, blue, alpha);

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

    glColor4d(red, green, blue, alpha);

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

    glColor4f(red, green, blue, alpha);

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

    glColor4i(red, green, blue, alpha);

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

    glColor4s(red, green, blue, alpha);

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

    glColor4ub(red, green, blue, alpha);

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

    glColor4ui(red, green, blue, alpha);

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

    glColor4us(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 171, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3bv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 172, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 173, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 174, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 175, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3ubv,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 176, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3ubv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3uiv,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 177, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3uiv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor3usv,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 178, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor3usv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 179, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor4bv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 180, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor4dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 181, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor4fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 182, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor4iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 183, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor4sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4ubv,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 184, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor4ubv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4uiv,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 185, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor4uiv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glColor4usv,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 186, Mesa)
{
    AROS_LIBFUNC_INIT

    glColor4usv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1d,
    AROS_LHA(GLdouble, s, D0),
    struct Library *, MesaBase, 187, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord1d(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1f,
    AROS_LHA(GLfloat, s, D0),
    struct Library *, MesaBase, 188, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord1f(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1i,
    AROS_LHA(GLint, s, D0),
    struct Library *, MesaBase, 189, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord1i(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1s,
    AROS_LHA(GLshort, s, D0),
    struct Library *, MesaBase, 190, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord1s(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2d,
    AROS_LHA(GLdouble, s, D0),
    AROS_LHA(GLdouble, t, D1),
    struct Library *, MesaBase, 191, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord2d(s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2f,
    AROS_LHA(GLfloat, s, D0),
    AROS_LHA(GLfloat, t, D1),
    struct Library *, MesaBase, 192, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord2f(s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2i,
    AROS_LHA(GLint, s, D0),
    AROS_LHA(GLint, t, D1),
    struct Library *, MesaBase, 193, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord2i(s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexCoord2s,
    AROS_LHA(GLshort, s, D0),
    AROS_LHA(GLshort, t, D1),
    struct Library *, MesaBase, 194, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord2s(s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3d,
    AROS_LHA(GLdouble, s, D0),
    AROS_LHA(GLdouble, t, D1),
    AROS_LHA(GLdouble, r, D2),
    struct Library *, MesaBase, 195, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord3d(s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3f,
    AROS_LHA(GLfloat, s, D0),
    AROS_LHA(GLfloat, t, D1),
    AROS_LHA(GLfloat, r, D2),
    struct Library *, MesaBase, 196, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord3f(s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3i,
    AROS_LHA(GLint, s, D0),
    AROS_LHA(GLint, t, D1),
    AROS_LHA(GLint, r, D2),
    struct Library *, MesaBase, 197, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord3i(s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexCoord3s,
    AROS_LHA(GLshort, s, D0),
    AROS_LHA(GLshort, t, D1),
    AROS_LHA(GLshort, r, D2),
    struct Library *, MesaBase, 198, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord3s(s, t, r);

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

    glTexCoord4d(s, t, r, q);

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

    glTexCoord4f(s, t, r, q);

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

    glTexCoord4i(s, t, r, q);

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

    glTexCoord4s(s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 203, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord1dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 204, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord1fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 205, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord1iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord1sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 206, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord1sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 207, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord2dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 208, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord2fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 209, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord2iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 210, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord2sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 211, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 212, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 213, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 214, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 215, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord4dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 216, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord4fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 217, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord4iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glTexCoord4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 218, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexCoord4sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 219, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos2d(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 220, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos2f(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 221, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos2i(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRasterPos2s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 222, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos2s(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 223, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos3d(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 224, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos3f(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 225, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos3i(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glRasterPos3s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 226, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos3s(x, y, z);

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

    glRasterPos4d(x, y, z, w);

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

    glRasterPos4f(x, y, z, w);

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

    glRasterPos4i(x, y, z, w);

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

    glRasterPos4s(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 231, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos2dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 232, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos2fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 233, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos2iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 234, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos2sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 235, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 236, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 237, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 238, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 239, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos4dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 240, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos4fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 241, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos4iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glRasterPos4sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 242, Mesa)
{
    AROS_LIBFUNC_INIT

    glRasterPos4sv(v);

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

    glRectd(x1, y1, x2, y2);

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

    glRectf(x1, y1, x2, y2);

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

    glRecti(x1, y1, x2, y2);

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

    glRects(x1, y1, x2, y2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectdv,
    AROS_LHA(const GLdouble *, v1, A0),
    AROS_LHA(const GLdouble *, v2, A1),
    struct Library *, MesaBase, 247, Mesa)
{
    AROS_LIBFUNC_INIT

    glRectdv(v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectfv,
    AROS_LHA(const GLfloat *, v1, A0),
    AROS_LHA(const GLfloat *, v2, A1),
    struct Library *, MesaBase, 248, Mesa)
{
    AROS_LIBFUNC_INIT

    glRectfv(v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectiv,
    AROS_LHA(const GLint *, v1, A0),
    AROS_LHA(const GLint *, v2, A1),
    struct Library *, MesaBase, 249, Mesa)
{
    AROS_LIBFUNC_INIT

    glRectiv(v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRectsv,
    AROS_LHA(const GLshort *, v1, A0),
    AROS_LHA(const GLshort *, v2, A1),
    struct Library *, MesaBase, 250, Mesa)
{
    AROS_LIBFUNC_INIT

    glRectsv(v1, v2);

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

    glVertexPointer(size, type, stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glNormalPointer,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 252, Mesa)
{
    AROS_LIBFUNC_INIT

    glNormalPointer(type, stride, ptr);

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

    glColorPointer(size, type, stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glIndexPointer,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 254, Mesa)
{
    AROS_LIBFUNC_INIT

    glIndexPointer(type, stride, ptr);

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

    glTexCoordPointer(size, type, stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEdgeFlagPointer,
    AROS_LHA(GLsizei, stride, D0),
    AROS_LHA(const GLvoid *, ptr, A0),
    struct Library *, MesaBase, 256, Mesa)
{
    AROS_LIBFUNC_INIT

    glEdgeFlagPointer(stride, ptr);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPointerv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLvoid * *, params, A0),
    struct Library *, MesaBase, 257, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetPointerv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glArrayElement,
    AROS_LHA(GLint, i, D0),
    struct Library *, MesaBase, 258, Mesa)
{
    AROS_LIBFUNC_INIT

    glArrayElement(i);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glDrawArrays,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    struct Library *, MesaBase, 259, Mesa)
{
    AROS_LIBFUNC_INIT

    glDrawArrays(mode, first, count);

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

    glDrawElements(mode, count, type, indices);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glInterleavedArrays,
    AROS_LHA(GLenum, format, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 261, Mesa)
{
    AROS_LIBFUNC_INIT

    glInterleavedArrays(format, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glShadeModel,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 262, Mesa)
{
    AROS_LIBFUNC_INIT

    glShadeModel(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLightf,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 263, Mesa)
{
    AROS_LIBFUNC_INIT

    glLightf(light, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLighti,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 264, Mesa)
{
    AROS_LIBFUNC_INIT

    glLighti(light, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLightfv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 265, Mesa)
{
    AROS_LIBFUNC_INIT

    glLightfv(light, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glLightiv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 266, Mesa)
{
    AROS_LIBFUNC_INIT

    glLightiv(light, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetLightfv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 267, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetLightfv(light, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetLightiv,
    AROS_LHA(GLenum, light, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 268, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetLightiv(light, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModelf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 269, Mesa)
{
    AROS_LIBFUNC_INIT

    glLightModelf(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModeli,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 270, Mesa)
{
    AROS_LIBFUNC_INIT

    glLightModeli(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModelfv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 271, Mesa)
{
    AROS_LIBFUNC_INIT

    glLightModelfv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLightModeliv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 272, Mesa)
{
    AROS_LIBFUNC_INIT

    glLightModeliv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMaterialf,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 273, Mesa)
{
    AROS_LIBFUNC_INIT

    glMaterialf(face, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMateriali,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 274, Mesa)
{
    AROS_LIBFUNC_INIT

    glMateriali(face, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMaterialfv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 275, Mesa)
{
    AROS_LIBFUNC_INIT

    glMaterialfv(face, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMaterialiv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 276, Mesa)
{
    AROS_LIBFUNC_INIT

    glMaterialiv(face, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMaterialfv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 277, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetMaterialfv(face, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMaterialiv,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 278, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetMaterialiv(face, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glColorMaterial,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 279, Mesa)
{
    AROS_LIBFUNC_INIT

    glColorMaterial(face, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelZoom,
    AROS_LHA(GLfloat, xfactor, D0),
    AROS_LHA(GLfloat, yfactor, D1),
    struct Library *, MesaBase, 280, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelZoom(xfactor, yfactor);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelStoref,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 281, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelStoref(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelStorei,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 282, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelStorei(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTransferf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 283, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelTransferf(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTransferi,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 284, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelTransferi(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPixelMapfv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, mapsize, D1),
    AROS_LHA(const GLfloat *, values, A0),
    struct Library *, MesaBase, 285, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelMapfv(map, mapsize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPixelMapuiv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, mapsize, D1),
    AROS_LHA(const GLuint *, values, A0),
    struct Library *, MesaBase, 286, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelMapuiv(map, mapsize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPixelMapusv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, mapsize, D1),
    AROS_LHA(const GLushort *, values, A0),
    struct Library *, MesaBase, 287, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelMapusv(map, mapsize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelMapfv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLfloat *, values, A0),
    struct Library *, MesaBase, 288, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetPixelMapfv(map, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelMapuiv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLuint *, values, A0),
    struct Library *, MesaBase, 289, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetPixelMapuiv(map, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelMapusv,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLushort *, values, A0),
    struct Library *, MesaBase, 290, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetPixelMapusv(map, values);

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

    glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);

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

    glReadPixels(x, y, width, height, format, type, pixels);

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

    glDrawPixels(width, height, format, type, pixels);

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

    glCopyPixels(x, y, width, height, type);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glStencilFunc,
    AROS_LHA(GLenum, func, D0),
    AROS_LHA(GLint, ref, D1),
    AROS_LHA(GLuint, mask, D2),
    struct Library *, MesaBase, 295, Mesa)
{
    AROS_LIBFUNC_INIT

    glStencilFunc(func, ref, mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glStencilMask,
    AROS_LHA(GLuint, mask, D0),
    struct Library *, MesaBase, 296, Mesa)
{
    AROS_LIBFUNC_INIT

    glStencilMask(mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glStencilOp,
    AROS_LHA(GLenum, fail, D0),
    AROS_LHA(GLenum, zfail, D1),
    AROS_LHA(GLenum, zpass, D2),
    struct Library *, MesaBase, 297, Mesa)
{
    AROS_LIBFUNC_INIT

    glStencilOp(fail, zfail, zpass);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClearStencil,
    AROS_LHA(GLint, s, D0),
    struct Library *, MesaBase, 298, Mesa)
{
    AROS_LIBFUNC_INIT

    glClearStencil(s);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGend,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble, param, D2),
    struct Library *, MesaBase, 299, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexGend(coord, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGenf,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 300, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexGenf(coord, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGeni,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 301, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexGeni(coord, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGendv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLdouble *, params, A0),
    struct Library *, MesaBase, 302, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexGendv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGenfv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 303, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexGenfv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexGeniv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 304, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexGeniv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexGendv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 305, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexGendv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexGenfv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 306, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexGenfv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexGeniv,
    AROS_LHA(GLenum, coord, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 307, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexGeniv(coord, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnvf,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 308, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexEnvf(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnvi,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 309, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexEnvi(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnvfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 310, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexEnvfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexEnviv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 311, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexEnviv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexEnvfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 312, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexEnvfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexEnviv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 313, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexEnviv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterf,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 314, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexParameterf(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameteri,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 315, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexParameteri(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 316, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 317, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 318, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 319, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexParameteriv(target, pname, params);

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

    glGetTexLevelParameterfv(target, level, pname, params);

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

    glGetTexLevelParameteriv(target, level, pname, params);

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

    glTexImage1D(target, level, internalFormat, width, border, format, type, pixels);

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

    glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);

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

    glGetTexImage(target, level, format, type, pixels);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenTextures,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, textures, A0),
    struct Library *, MesaBase, 325, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenTextures(n, textures);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteTextures,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    struct Library *, MesaBase, 326, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteTextures(n, textures);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindTexture,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, texture, D1),
    struct Library *, MesaBase, 327, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindTexture(target, texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPrioritizeTextures,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(const GLclampf *, priorities, A1),
    struct Library *, MesaBase, 328, Mesa)
{
    AROS_LIBFUNC_INIT

    glPrioritizeTextures(n, textures, priorities);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLboolean, glAreTexturesResident,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(GLboolean *, residences, A1),
    struct Library *, MesaBase, 329, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glAreTexturesResident(n, textures, residences);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsTexture,
    AROS_LHA(GLuint, texture, D0),
    struct Library *, MesaBase, 330, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsTexture(texture);

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

    glTexSubImage1D(target, level, xoffset, width, format, type, pixels);

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

    glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);

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

    glCopyTexImage1D(target, level, internalformat, x, y, width, border);

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

    glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);

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

    glCopyTexSubImage1D(target, level, xoffset, x, y, width);

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

    glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);

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

    glMap1d(target, u1, u2, stride, order, points);

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

    glMap1f(target, u1, u2, stride, order, points);

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

    glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);

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

    glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMapdv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLdouble *, v, A0),
    struct Library *, MesaBase, 341, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetMapdv(target, query, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMapfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLfloat *, v, A0),
    struct Library *, MesaBase, 342, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetMapfv(target, query, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMapiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, query, D1),
    AROS_LHA(GLint *, v, A0),
    struct Library *, MesaBase, 343, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetMapiv(target, query, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1d,
    AROS_LHA(GLdouble, u, D0),
    struct Library *, MesaBase, 344, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalCoord1d(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1f,
    AROS_LHA(GLfloat, u, D0),
    struct Library *, MesaBase, 345, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalCoord1f(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1dv,
    AROS_LHA(const GLdouble *, u, A0),
    struct Library *, MesaBase, 346, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalCoord1dv(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord1fv,
    AROS_LHA(const GLfloat *, u, A0),
    struct Library *, MesaBase, 347, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalCoord1fv(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEvalCoord2d,
    AROS_LHA(GLdouble, u, D0),
    AROS_LHA(GLdouble, v, D1),
    struct Library *, MesaBase, 348, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalCoord2d(u, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEvalCoord2f,
    AROS_LHA(GLfloat, u, D0),
    AROS_LHA(GLfloat, v, D1),
    struct Library *, MesaBase, 349, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalCoord2f(u, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord2dv,
    AROS_LHA(const GLdouble *, u, A0),
    struct Library *, MesaBase, 350, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalCoord2dv(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalCoord2fv,
    AROS_LHA(const GLfloat *, u, A0),
    struct Library *, MesaBase, 351, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalCoord2fv(u);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMapGrid1d,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLdouble, u1, D1),
    AROS_LHA(GLdouble, u2, D2),
    struct Library *, MesaBase, 352, Mesa)
{
    AROS_LIBFUNC_INIT

    glMapGrid1d(un, u1, u2);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMapGrid1f,
    AROS_LHA(GLint, un, D0),
    AROS_LHA(GLfloat, u1, D1),
    AROS_LHA(GLfloat, u2, D2),
    struct Library *, MesaBase, 353, Mesa)
{
    AROS_LIBFUNC_INIT

    glMapGrid1f(un, u1, u2);

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

    glMapGrid2d(un, u1, u2, vn, v1, v2);

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

    glMapGrid2f(un, u1, u2, vn, v1, v2);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEvalPoint1,
    AROS_LHA(GLint, i, D0),
    struct Library *, MesaBase, 356, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalPoint1(i);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEvalPoint2,
    AROS_LHA(GLint, i, D0),
    AROS_LHA(GLint, j, D1),
    struct Library *, MesaBase, 357, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalPoint2(i, j);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glEvalMesh1,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, i1, D1),
    AROS_LHA(GLint, i2, D2),
    struct Library *, MesaBase, 358, Mesa)
{
    AROS_LIBFUNC_INIT

    glEvalMesh1(mode, i1, i2);

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

    glEvalMesh2(mode, i1, i2, j1, j2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 360, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogf(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogi,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 361, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogi(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogfv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 362, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogfv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glFogiv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 363, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogiv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFeedbackBuffer,
    AROS_LHA(GLsizei, size, D0),
    AROS_LHA(GLenum, type, D1),
    AROS_LHA(GLfloat *, buffer, A0),
    struct Library *, MesaBase, 364, Mesa)
{
    AROS_LIBFUNC_INIT

    glFeedbackBuffer(size, type, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPassThrough,
    AROS_LHA(GLfloat, token, D0),
    struct Library *, MesaBase, 365, Mesa)
{
    AROS_LIBFUNC_INIT

    glPassThrough(token);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSelectBuffer,
    AROS_LHA(GLsizei, size, D0),
    AROS_LHA(GLuint *, buffer, A0),
    struct Library *, MesaBase, 366, Mesa)
{
    AROS_LIBFUNC_INIT

    glSelectBuffer(size, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glInitNames,
    struct Library *, MesaBase, 367, Mesa)
{
    AROS_LIBFUNC_INIT

    glInitNames();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadName,
    AROS_LHA(GLuint, name, D0),
    struct Library *, MesaBase, 368, Mesa)
{
    AROS_LIBFUNC_INIT

    glLoadName(name);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPushName,
    AROS_LHA(GLuint, name, D0),
    struct Library *, MesaBase, 369, Mesa)
{
    AROS_LIBFUNC_INIT

    glPushName(name);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPopName,
    struct Library *, MesaBase, 370, Mesa)
{
    AROS_LIBFUNC_INIT

    glPopName();

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

    glDrawRangeElements(mode, start, end, count, type, indices);

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

    glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);

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

    glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);

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

    glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);

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

    glColorTable(target, internalformat, width, format, type, table);

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

    glColorSubTable(target, start, count, format, type, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 377, Mesa)
{
    AROS_LIBFUNC_INIT

    glColorTableParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 378, Mesa)
{
    AROS_LIBFUNC_INIT

    glColorTableParameterfv(target, pname, params);

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

    glCopyColorSubTable(target, start, x, y, width);

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

    glCopyColorTable(target, internalformat, x, y, width);

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

    glGetColorTable(target, format, type, table);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 382, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetColorTableParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 383, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetColorTableParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBlendEquation,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 384, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendEquation(mode);

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

    glBlendColor(red, green, blue, alpha);

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

    glHistogram(target, width, internalformat, sink);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetHistogram,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 387, Mesa)
{
    AROS_LIBFUNC_INIT

    glResetHistogram(target);

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

    glGetHistogram(target, reset, format, type, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 389, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetHistogramParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 390, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetHistogramParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMinmax,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLboolean, sink, D2),
    struct Library *, MesaBase, 391, Mesa)
{
    AROS_LIBFUNC_INIT

    glMinmax(target, internalformat, sink);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetMinmax,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 392, Mesa)
{
    AROS_LIBFUNC_INIT

    glResetMinmax(target);

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

    glGetMinmax(target, reset, format, types, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 394, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetMinmaxParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 395, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetMinmaxParameteriv(target, pname, params);

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

    glConvolutionFilter1D(target, internalformat, width, format, type, image);

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

    glConvolutionFilter2D(target, internalformat, width, height, format, type, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterf,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, params, D2),
    struct Library *, MesaBase, 398, Mesa)
{
    AROS_LIBFUNC_INIT

    glConvolutionParameterf(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 399, Mesa)
{
    AROS_LIBFUNC_INIT

    glConvolutionParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameteri,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, params, D2),
    struct Library *, MesaBase, 400, Mesa)
{
    AROS_LIBFUNC_INIT

    glConvolutionParameteri(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 401, Mesa)
{
    AROS_LIBFUNC_INIT

    glConvolutionParameteriv(target, pname, params);

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

    glCopyConvolutionFilter1D(target, internalformat, x, y, width);

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

    glCopyConvolutionFilter2D(target, internalformat, x, y, width, height);

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

    glGetConvolutionFilter(target, format, type, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameterfv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 405, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetConvolutionParameterfv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 406, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetConvolutionParameteriv(target, pname, params);

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

    glSeparableFilter2D(target, internalformat, width, height, format, type, row, column);

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

    glGetSeparableFilter(target, format, type, row, column, span);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveTexture,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 409, Mesa)
{
    AROS_LIBFUNC_INIT

    glActiveTexture(texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClientActiveTexture,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 410, Mesa)
{
    AROS_LIBFUNC_INIT

    glClientActiveTexture(texture);

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

    glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);

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

    glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);

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

    glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);

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

    glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);

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

    glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);

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

    glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetCompressedTexImage,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, lod, D1),
    AROS_LHA(GLvoid *, img, A0),
    struct Library *, MesaBase, 417, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetCompressedTexImage(target, lod, img);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    struct Library *, MesaBase, 418, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1d(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 419, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1dv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    struct Library *, MesaBase, 420, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1f(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 421, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1fv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    struct Library *, MesaBase, 422, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1i(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 423, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1iv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    struct Library *, MesaBase, 424, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1s(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 425, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1sv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2d,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    struct Library *, MesaBase, 426, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2d(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 427, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2dv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2f,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    struct Library *, MesaBase, 428, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2f(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 429, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2fv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2i,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    struct Library *, MesaBase, 430, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2i(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 431, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2iv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2s,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    struct Library *, MesaBase, 432, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2s(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 433, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2sv(target, v);

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

    glMultiTexCoord3d(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 435, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord3dv(target, v);

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

    glMultiTexCoord3f(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 437, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord3fv(target, v);

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

    glMultiTexCoord3i(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 439, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord3iv(target, v);

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

    glMultiTexCoord3s(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 441, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord3sv(target, v);

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

    glMultiTexCoord4d(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4dv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 443, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord4dv(target, v);

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

    glMultiTexCoord4f(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4fv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 445, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord4fv(target, v);

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

    glMultiTexCoord4i(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4iv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 447, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord4iv(target, v);

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

    glMultiTexCoord4s(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4sv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 449, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord4sv(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 450, Mesa)
{
    AROS_LIBFUNC_INIT

    glLoadTransposeMatrixd(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 451, Mesa)
{
    AROS_LIBFUNC_INIT

    glLoadTransposeMatrixf(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixd,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 452, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultTransposeMatrixd(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixf,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 453, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultTransposeMatrixf(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleCoverage,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 454, Mesa)
{
    AROS_LIBFUNC_INIT

    glSampleCoverage(value, invert);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveTextureARB,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 455, Mesa)
{
    AROS_LIBFUNC_INIT

    glActiveTextureARB(texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClientActiveTextureARB,
    AROS_LHA(GLenum, texture, D0),
    struct Library *, MesaBase, 456, Mesa)
{
    AROS_LIBFUNC_INIT

    glClientActiveTextureARB(texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    struct Library *, MesaBase, 457, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1dARB(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 458, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1dvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    struct Library *, MesaBase, 459, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1fARB(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 460, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1fvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    struct Library *, MesaBase, 461, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1iARB(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 462, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1ivARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    struct Library *, MesaBase, 463, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1sARB(target, s);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord1svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 464, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord1svARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2dARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLdouble, s, D1),
    AROS_LHA(GLdouble, t, D2),
    struct Library *, MesaBase, 465, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2dARB(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 466, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2dvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2fARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLfloat, s, D1),
    AROS_LHA(GLfloat, t, D2),
    struct Library *, MesaBase, 467, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2fARB(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 468, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2fvARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2iARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, s, D1),
    AROS_LHA(GLint, t, D2),
    struct Library *, MesaBase, 469, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2iARB(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 470, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2ivARB(target, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMultiTexCoord2sARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLshort, s, D1),
    AROS_LHA(GLshort, t, D2),
    struct Library *, MesaBase, 471, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2sARB(target, s, t);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord2svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 472, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord2svARB(target, v);

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

    glMultiTexCoord3dARB(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 474, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord3dvARB(target, v);

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

    glMultiTexCoord3fARB(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 476, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord3fvARB(target, v);

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

    glMultiTexCoord3iARB(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 478, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord3ivARB(target, v);

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

    glMultiTexCoord3sARB(target, s, t, r);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord3svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 480, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord3svARB(target, v);

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

    glMultiTexCoord4dARB(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 482, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord4dvARB(target, v);

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

    glMultiTexCoord4fARB(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 484, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord4fvARB(target, v);

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

    glMultiTexCoord4iARB(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4ivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 486, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord4ivARB(target, v);

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

    glMultiTexCoord4sARB(target, s, t, r, q);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glMultiTexCoord4svARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 488, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultiTexCoord4svARB(target, v);

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

    glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordf,
    AROS_LHA(GLfloat, coord, D0),
    struct Library *, MesaBase, 490, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoordf(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordfv,
    AROS_LHA(const GLfloat *, coord, A0),
    struct Library *, MesaBase, 491, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoordfv(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordd,
    AROS_LHA(GLdouble, coord, D0),
    struct Library *, MesaBase, 492, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoordd(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoorddv,
    AROS_LHA(const GLdouble *, coord, A0),
    struct Library *, MesaBase, 493, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoorddv(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFogCoordPointer,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 494, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoordPointer(type, stride, pointer);

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

    glMultiDrawArrays(mode, first, count, primcount);

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

    glMultiDrawElements(mode, count, type, indices, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterf,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 497, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameterf(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 498, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameterfv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameteri,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 499, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameteri(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameteriv,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 500, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameteriv(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3b,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    struct Library *, MesaBase, 501, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3b(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3bv,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 502, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3bv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3d,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    struct Library *, MesaBase, 503, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3d(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 504, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3f,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    struct Library *, MesaBase, 505, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3f(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 506, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3i,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    struct Library *, MesaBase, 507, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3i(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 508, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3s,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    struct Library *, MesaBase, 509, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3s(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 510, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3ub,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    struct Library *, MesaBase, 511, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3ub(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3ubv,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 512, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3ubv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3ui,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    struct Library *, MesaBase, 513, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3ui(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3uiv,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 514, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3uiv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3us,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    struct Library *, MesaBase, 515, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3us(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3usv,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 516, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3usv(v);

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

    glSecondaryColorPointer(size, type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 518, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2d(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 519, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 520, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2f(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 521, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 522, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2i(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 523, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 524, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2s(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 525, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3d,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 526, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3d(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3dv,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 527, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3dv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3f,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 528, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3f(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3fv,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 529, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3fv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3i,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 530, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3i(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3iv,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 531, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3iv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3s,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 532, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3s(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3sv,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 533, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3sv(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenQueries,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, ids, A0),
    struct Library *, MesaBase, 534, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenQueries(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteQueries,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, ids, A0),
    struct Library *, MesaBase, 535, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteQueries(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsQuery,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 536, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsQuery(id);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginQuery,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 537, Mesa)
{
    AROS_LIBFUNC_INIT

    glBeginQuery(target, id);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEndQuery,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 538, Mesa)
{
    AROS_LIBFUNC_INIT

    glEndQuery(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 539, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetQueryiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectiv,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 540, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetQueryObjectiv(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectuiv,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 541, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetQueryObjectuiv(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindBuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, buffer, D1),
    struct Library *, MesaBase, 542, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindBuffer(target, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteBuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, buffers, A0),
    struct Library *, MesaBase, 543, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteBuffers(n, buffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenBuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, buffers, A0),
    struct Library *, MesaBase, 544, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenBuffers(n, buffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsBuffer,
    AROS_LHA(GLuint, buffer, D0),
    struct Library *, MesaBase, 545, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsBuffer(buffer);

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

    glBufferData(target, size, data, usage);

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

    glBufferSubData(target, offset, size, data);

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

    glGetBufferSubData(target, offset, size, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLvoid*, glMapBuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, access, D1),
    struct Library *, MesaBase, 549, Mesa)
{
    AROS_LIBFUNC_INIT

    GLvoid* _return = glMapBuffer(target, access);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glUnmapBuffer,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 550, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glUnmapBuffer(target);

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

    glGetBufferParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferPointerv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 552, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetBufferPointerv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationSeparate,
    AROS_LHA(GLenum, modeRGB, D0),
    AROS_LHA(GLenum, modeAlpha, D1),
    struct Library *, MesaBase, 553, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendEquationSeparate(modeRGB, modeAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawBuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLenum *, bufs, A0),
    struct Library *, MesaBase, 554, Mesa)
{
    AROS_LIBFUNC_INIT

    glDrawBuffers(n, bufs);

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

    glStencilOpSeparate(face, sfail, dpfail, dppass);

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

    glStencilFuncSeparate(face, func, ref, mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glStencilMaskSeparate,
    AROS_LHA(GLenum, face, D0),
    AROS_LHA(GLuint, mask, D1),
    struct Library *, MesaBase, 557, Mesa)
{
    AROS_LIBFUNC_INIT

    glStencilMaskSeparate(face, mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAttachShader,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, shader, D1),
    struct Library *, MesaBase, 558, Mesa)
{
    AROS_LIBFUNC_INIT

    glAttachShader(program, shader);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindAttribLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 559, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindAttribLocation(program, index, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCompileShader,
    AROS_LHA(GLuint, shader, D0),
    struct Library *, MesaBase, 560, Mesa)
{
    AROS_LIBFUNC_INIT

    glCompileShader(shader);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLuint, glCreateProgram,
    struct Library *, MesaBase, 561, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = glCreateProgram();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLuint, glCreateShader,
    AROS_LHA(GLenum, type, D0),
    struct Library *, MesaBase, 562, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = glCreateShader(type);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 563, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteProgram(program);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteShader,
    AROS_LHA(GLuint, shader, D0),
    struct Library *, MesaBase, 564, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteShader(shader);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDetachShader,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, shader, D1),
    struct Library *, MesaBase, 565, Mesa)
{
    AROS_LIBFUNC_INIT

    glDetachShader(program, shader);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisableVertexAttribArray,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 566, Mesa)
{
    AROS_LIBFUNC_INIT

    glDisableVertexAttribArray(index);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnableVertexAttribArray,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 567, Mesa)
{
    AROS_LIBFUNC_INIT

    glEnableVertexAttribArray(index);

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

    glGetActiveAttrib(program, index, bufSize, length, size, type, name);

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

    glGetActiveUniform(program, index, bufSize, length, size, type, name);

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

    glGetAttachedShaders(program, maxCount, count, obj);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetAttribLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 571, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = glGetAttribLocation(program, name);

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

    glGetProgramiv(program, pname, params);

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

    glGetProgramInfoLog(program, bufSize, length, infoLog);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetShaderiv,
    AROS_LHA(GLuint, shader, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 574, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetShaderiv(shader, pname, params);

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

    glGetShaderInfoLog(shader, bufSize, length, infoLog);

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

    glGetShaderSource(shader, bufSize, length, source);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetUniformLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 577, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = glGetUniformLocation(program, name);

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

    glGetUniformfv(program, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformiv,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 579, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetUniformiv(program, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribdv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 580, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribdv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribfv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 581, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribfv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 582, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribiv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribPointerv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, pointer, A0),
    struct Library *, MesaBase, 583, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribPointerv(index, pname, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 584, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsProgram(program);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsShader,
    AROS_LHA(GLuint, shader, D0),
    struct Library *, MesaBase, 585, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsShader(shader);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLinkProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 586, Mesa)
{
    AROS_LIBFUNC_INIT

    glLinkProgram(program);

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

    glShaderSource(shader, count, string, length);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glUseProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 588, Mesa)
{
    AROS_LIBFUNC_INIT

    glUseProgram(program);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    struct Library *, MesaBase, 589, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1f(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2f,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    struct Library *, MesaBase, 590, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2f(location, v0, v1);

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

    glUniform3f(location, v0, v1, v2);

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

    glUniform4f(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    struct Library *, MesaBase, 593, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1i(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2i,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    struct Library *, MesaBase, 594, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2i(location, v0, v1);

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

    glUniform3i(location, v0, v1, v2);

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

    glUniform4i(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 597, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1fv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 598, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2fv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 599, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform3fv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4fv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 600, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform4fv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 601, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1iv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 602, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2iv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 603, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform3iv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4iv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 604, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform4iv(location, count, value);

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

    glUniformMatrix2fv(location, count, transpose, value);

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

    glUniformMatrix3fv(location, count, transpose, value);

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

    glUniformMatrix4fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glValidateProgram,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 608, Mesa)
{
    AROS_LIBFUNC_INIT

    glValidateProgram(program);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    struct Library *, MesaBase, 609, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1d(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 610, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1dv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    struct Library *, MesaBase, 611, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1f(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 612, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1fv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    struct Library *, MesaBase, 613, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1s(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 614, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1sv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2d,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    struct Library *, MesaBase, 615, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2d(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 616, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2dv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2f,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    struct Library *, MesaBase, 617, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2f(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 618, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2fv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2s,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    struct Library *, MesaBase, 619, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2s(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 620, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2sv(index, v);

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

    glVertexAttrib3d(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 622, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib3dv(index, v);

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

    glVertexAttrib3f(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 624, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib3fv(index, v);

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

    glVertexAttrib3s(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 626, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib3sv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nbv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 627, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4Nbv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Niv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 628, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4Niv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nsv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 629, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4Nsv(index, v);

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

    glVertexAttrib4Nub(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nubv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 631, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4Nubv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nuiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 632, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4Nuiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4Nusv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 633, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4Nusv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4bv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 634, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4bv(index, v);

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

    glVertexAttrib4d(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4dv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 636, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4dv(index, v);

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

    glVertexAttrib4f(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4fv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 638, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4fv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 639, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4iv(index, v);

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

    glVertexAttrib4s(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 641, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4sv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ubv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 642, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4ubv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 643, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4usv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 644, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4usv(index, v);

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

    glVertexAttribPointer(index, size, type, normalized, stride, pointer);

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

    glUniformMatrix2x3fv(location, count, transpose, value);

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

    glUniformMatrix3x2fv(location, count, transpose, value);

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

    glUniformMatrix2x4fv(location, count, transpose, value);

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

    glUniformMatrix4x2fv(location, count, transpose, value);

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

    glUniformMatrix3x4fv(location, count, transpose, value);

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

    glUniformMatrix4x3fv(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixfARB,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 652, Mesa)
{
    AROS_LIBFUNC_INIT

    glLoadTransposeMatrixfARB(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLoadTransposeMatrixdARB,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 653, Mesa)
{
    AROS_LIBFUNC_INIT

    glLoadTransposeMatrixdARB(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixfARB,
    AROS_LHA(const GLfloat *, m, A0),
    struct Library *, MesaBase, 654, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultTransposeMatrixfARB(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glMultTransposeMatrixdARB,
    AROS_LHA(const GLdouble *, m, A0),
    struct Library *, MesaBase, 655, Mesa)
{
    AROS_LIBFUNC_INIT

    glMultTransposeMatrixdARB(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleCoverageARB,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 656, Mesa)
{
    AROS_LIBFUNC_INIT

    glSampleCoverageARB(value, invert);

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

    glCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imageSize, data);

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

    glCompressedTexImage2DARB(target, level, internalformat, width, height, border, imageSize, data);

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

    glCompressedTexImage1DARB(target, level, internalformat, width, border, imageSize, data);

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

    glCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);

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

    glCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data);

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

    glCompressedTexSubImage1DARB(target, level, xoffset, width, format, imageSize, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetCompressedTexImageARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, level, D1),
    AROS_LHA(GLvoid *, img, A0),
    struct Library *, MesaBase, 663, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetCompressedTexImageARB(target, level, img);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfARB,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 664, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameterfARB(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfvARB,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 665, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameterfvARB(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2dARB,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 666, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2dARB(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2dvARB,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 667, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2dvARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2fARB,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 668, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2fARB(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2fvARB,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 669, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2fvARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2iARB,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 670, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2iARB(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2ivARB,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 671, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2ivARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2sARB,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 672, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2sARB(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2svARB,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 673, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2svARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3dARB,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 674, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3dARB(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3dvARB,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 675, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3dvARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3fARB,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 676, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3fARB(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3fvARB,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 677, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3fvARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3iARB,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 678, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3iARB(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3ivARB,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 679, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3ivARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3sARB,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 680, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3sARB(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3svARB,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 681, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3svARB(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    struct Library *, MesaBase, 682, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1dARB(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 683, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1dvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    struct Library *, MesaBase, 684, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1fARB(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 685, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1fvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    struct Library *, MesaBase, 686, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1sARB(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 687, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1svARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2dARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    struct Library *, MesaBase, 688, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2dARB(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 689, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2dvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2fARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    struct Library *, MesaBase, 690, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2fARB(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 691, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2fvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2sARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    struct Library *, MesaBase, 692, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2sARB(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 693, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2svARB(index, v);

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

    glVertexAttrib3dARB(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 695, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib3dvARB(index, v);

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

    glVertexAttrib3fARB(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 697, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib3fvARB(index, v);

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

    glVertexAttrib3sARB(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 699, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib3svARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NbvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 700, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4NbvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 701, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4NivARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NsvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 702, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4NsvARB(index, v);

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

    glVertexAttrib4NubARB(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NubvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 704, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4NubvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NuivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 705, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4NuivARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4NusvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 706, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4NusvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4bvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 707, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4bvARB(index, v);

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

    glVertexAttrib4dARB(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4dvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 709, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4dvARB(index, v);

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

    glVertexAttrib4fARB(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4fvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 711, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4fvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 712, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4ivARB(index, v);

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

    glVertexAttrib4sARB(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4svARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 714, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4svARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ubvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 715, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4ubvARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4uivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 716, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4uivARB(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4usvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 717, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4usvARB(index, v);

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

    glVertexAttribPointerARB(index, size, type, normalized, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEnableVertexAttribArrayARB,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 719, Mesa)
{
    AROS_LIBFUNC_INIT

    glEnableVertexAttribArrayARB(index);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDisableVertexAttribArrayARB,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 720, Mesa)
{
    AROS_LIBFUNC_INIT

    glDisableVertexAttribArrayARB(index);

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

    glProgramStringARB(target, format, len, string);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindProgramARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, program, D1),
    struct Library *, MesaBase, 722, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindProgramARB(target, program);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteProgramsARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    struct Library *, MesaBase, 723, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteProgramsARB(n, programs);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenProgramsARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, programs, A0),
    struct Library *, MesaBase, 724, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenProgramsARB(n, programs);

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

    glProgramEnvParameter4dARB(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramEnvParameter4dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLdouble *, params, A0),
    struct Library *, MesaBase, 726, Mesa)
{
    AROS_LIBFUNC_INIT

    glProgramEnvParameter4dvARB(target, index, params);

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

    glProgramEnvParameter4fARB(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramEnvParameter4fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 728, Mesa)
{
    AROS_LIBFUNC_INIT

    glProgramEnvParameter4fvARB(target, index, params);

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

    glProgramLocalParameter4dARB(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramLocalParameter4dvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLdouble *, params, A0),
    struct Library *, MesaBase, 730, Mesa)
{
    AROS_LIBFUNC_INIT

    glProgramLocalParameter4dvARB(target, index, params);

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

    glProgramLocalParameter4fARB(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramLocalParameter4fvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 732, Mesa)
{
    AROS_LIBFUNC_INIT

    glProgramLocalParameter4fvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramEnvParameterdvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 733, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetProgramEnvParameterdvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramEnvParameterfvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 734, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetProgramEnvParameterfvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramLocalParameterdvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 735, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetProgramLocalParameterdvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramLocalParameterfvARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 736, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetProgramLocalParameterfvARB(target, index, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 737, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetProgramivARB(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramStringARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *, string, A0),
    struct Library *, MesaBase, 738, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetProgramStringARB(target, pname, string);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribdvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 739, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribdvARB(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribfvARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 740, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribfvARB(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribivARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 741, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribivARB(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribPointervARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, pointer, A0),
    struct Library *, MesaBase, 742, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribPointervARB(index, pname, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsProgramARB,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 743, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsProgramARB(program);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindBufferARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, buffer, D1),
    struct Library *, MesaBase, 744, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindBufferARB(target, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteBuffersARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, buffers, A0),
    struct Library *, MesaBase, 745, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteBuffersARB(n, buffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenBuffersARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, buffers, A0),
    struct Library *, MesaBase, 746, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenBuffersARB(n, buffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsBufferARB,
    AROS_LHA(GLuint, buffer, D0),
    struct Library *, MesaBase, 747, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsBufferARB(buffer);

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

    glBufferDataARB(target, size, data, usage);

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

    glBufferSubDataARB(target, offset, size, data);

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

    glGetBufferSubDataARB(target, offset, size, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLvoid*, glMapBufferARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, access, D1),
    struct Library *, MesaBase, 751, Mesa)
{
    AROS_LIBFUNC_INIT

    GLvoid* _return = glMapBufferARB(target, access);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glUnmapBufferARB,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 752, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glUnmapBufferARB(target);

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

    glGetBufferParameterivARB(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferPointervARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 754, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetBufferPointervARB(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenQueriesARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, ids, A0),
    struct Library *, MesaBase, 755, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenQueriesARB(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteQueriesARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, ids, A0),
    struct Library *, MesaBase, 756, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteQueriesARB(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsQueryARB,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 757, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsQueryARB(id);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginQueryARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 758, Mesa)
{
    AROS_LIBFUNC_INIT

    glBeginQueryARB(target, id);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glEndQueryARB,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 759, Mesa)
{
    AROS_LIBFUNC_INIT

    glEndQueryARB(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryivARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 760, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetQueryivARB(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectivARB,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 761, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetQueryObjectivARB(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectuivARB,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 762, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetQueryObjectuivARB(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteObjectARB,
    AROS_LHA(GLhandleARB, obj, D0),
    struct Library *, MesaBase, 763, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteObjectARB(obj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLhandleARB, glGetHandleARB,
    AROS_LHA(GLenum, pname, D0),
    struct Library *, MesaBase, 764, Mesa)
{
    AROS_LIBFUNC_INIT

    GLhandleARB _return = glGetHandleARB(pname);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDetachObjectARB,
    AROS_LHA(GLhandleARB, containerObj, D0),
    AROS_LHA(GLhandleARB, attachedObj, D1),
    struct Library *, MesaBase, 765, Mesa)
{
    AROS_LIBFUNC_INIT

    glDetachObjectARB(containerObj, attachedObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLhandleARB, glCreateShaderObjectARB,
    AROS_LHA(GLenum, shaderType, D0),
    struct Library *, MesaBase, 766, Mesa)
{
    AROS_LIBFUNC_INIT

    GLhandleARB _return = glCreateShaderObjectARB(shaderType);

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

    glShaderSourceARB(shaderObj, count, string, length);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glCompileShaderARB,
    AROS_LHA(GLhandleARB, shaderObj, D0),
    struct Library *, MesaBase, 768, Mesa)
{
    AROS_LIBFUNC_INIT

    glCompileShaderARB(shaderObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLhandleARB, glCreateProgramObjectARB,
    struct Library *, MesaBase, 769, Mesa)
{
    AROS_LIBFUNC_INIT

    GLhandleARB _return = glCreateProgramObjectARB();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glAttachObjectARB,
    AROS_LHA(GLhandleARB, containerObj, D0),
    AROS_LHA(GLhandleARB, obj, D1),
    struct Library *, MesaBase, 770, Mesa)
{
    AROS_LIBFUNC_INIT

    glAttachObjectARB(containerObj, obj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glLinkProgramARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    struct Library *, MesaBase, 771, Mesa)
{
    AROS_LIBFUNC_INIT

    glLinkProgramARB(programObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glUseProgramObjectARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    struct Library *, MesaBase, 772, Mesa)
{
    AROS_LIBFUNC_INIT

    glUseProgramObjectARB(programObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glValidateProgramARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    struct Library *, MesaBase, 773, Mesa)
{
    AROS_LIBFUNC_INIT

    glValidateProgramARB(programObj);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    struct Library *, MesaBase, 774, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1fARB(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2fARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLfloat, v0, D1),
    AROS_LHA(GLfloat, v1, D2),
    struct Library *, MesaBase, 775, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2fARB(location, v0, v1);

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

    glUniform3fARB(location, v0, v1, v2);

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

    glUniform4fARB(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    struct Library *, MesaBase, 778, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1iARB(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2iARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLint, v0, D1),
    AROS_LHA(GLint, v1, D2),
    struct Library *, MesaBase, 779, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2iARB(location, v0, v1);

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

    glUniform3iARB(location, v0, v1, v2);

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

    glUniform4iARB(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 782, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1fvARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 783, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2fvARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 784, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform3fvARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4fvARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 785, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform4fvARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 786, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1ivARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 787, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2ivARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 788, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform3ivARB(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4ivARB,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 789, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform4ivARB(location, count, value);

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

    glUniformMatrix2fvARB(location, count, transpose, value);

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

    glUniformMatrix3fvARB(location, count, transpose, value);

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

    glUniformMatrix4fvARB(location, count, transpose, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetObjectParameterfvARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 793, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetObjectParameterfvARB(obj, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetObjectParameterivARB,
    AROS_LHA(GLhandleARB, obj, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 794, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetObjectParameterivARB(obj, pname, params);

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

    glGetInfoLogARB(obj, maxLength, length, infoLog);

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

    glGetAttachedObjectsARB(containerObj, maxCount, count, obj);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetUniformLocationARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(const GLcharARB *, name, A0),
    struct Library *, MesaBase, 797, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = glGetUniformLocationARB(programObj, name);

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

    glGetActiveUniformARB(programObj, index, maxLength, length, size, type, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformfvARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 799, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetUniformfvARB(programObj, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformivARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 800, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetUniformivARB(programObj, location, params);

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

    glGetShaderSourceARB(obj, maxLength, length, source);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindAttribLocationARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLcharARB *, name, A0),
    struct Library *, MesaBase, 802, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindAttribLocationARB(programObj, index, name);

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

    glGetActiveAttribARB(programObj, index, maxLength, length, size, type, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetAttribLocationARB,
    AROS_LHA(GLhandleARB, programObj, D0),
    AROS_LHA(const GLcharARB *, name, A0),
    struct Library *, MesaBase, 804, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = glGetAttribLocationARB(programObj, name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawBuffersARB,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLenum *, bufs, A0),
    struct Library *, MesaBase, 805, Mesa)
{
    AROS_LIBFUNC_INIT

    glDrawBuffersARB(n, bufs);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsRenderbuffer,
    AROS_LHA(GLuint, renderbuffer, D0),
    struct Library *, MesaBase, 806, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsRenderbuffer(renderbuffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindRenderbuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, renderbuffer, D1),
    struct Library *, MesaBase, 807, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindRenderbuffer(target, renderbuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteRenderbuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 808, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteRenderbuffers(n, renderbuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenRenderbuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 809, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenRenderbuffers(n, renderbuffers);

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

    glRenderbufferStorage(target, internalformat, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetRenderbufferParameteriv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 811, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetRenderbufferParameteriv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsFramebuffer,
    AROS_LHA(GLuint, framebuffer, D0),
    struct Library *, MesaBase, 812, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsFramebuffer(framebuffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindFramebuffer,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, framebuffer, D1),
    struct Library *, MesaBase, 813, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindFramebuffer(target, framebuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteFramebuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 814, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteFramebuffers(n, framebuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenFramebuffers,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 815, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenFramebuffers(n, framebuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLenum, glCheckFramebufferStatus,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 816, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = glCheckFramebufferStatus(target);

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

    glFramebufferTexture1D(target, attachment, textarget, texture, level);

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

    glFramebufferTexture2D(target, attachment, textarget, texture, level);

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

    glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);

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

    glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);

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

    glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glGenerateMipmap,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 822, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenerateMipmap(target);

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

    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);

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

    glRenderbufferStorageMultisample(target, samples, internalformat, width, height);

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

    glFramebufferTextureLayer(target, attachment, texture, level, layer);

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

    glBlendColorEXT(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPolygonOffsetEXT,
    AROS_LHA(GLfloat, factor, D0),
    AROS_LHA(GLfloat, bias, D1),
    struct Library *, MesaBase, 827, Mesa)
{
    AROS_LIBFUNC_INIT

    glPolygonOffsetEXT(factor, bias);

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

    glTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, pixels);

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

    glTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);

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

    glTexSubImage1DEXT(target, level, xoffset, width, format, type, pixels);

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

    glTexSubImage2DEXT(target, level, xoffset, yoffset, width, height, format, type, pixels);

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

    glCopyTexImage1DEXT(target, level, internalformat, x, y, width, border);

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

    glCopyTexImage2DEXT(target, level, internalformat, x, y, width, height, border);

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

    glCopyTexSubImage1DEXT(target, level, xoffset, x, y, width);

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

    glCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height);

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

    glCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLboolean, glAreTexturesResidentEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    AROS_LHA(GLboolean *, residences, A1),
    struct Library *, MesaBase, 837, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glAreTexturesResidentEXT(n, textures, residences);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindTextureEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, texture, D1),
    struct Library *, MesaBase, 838, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindTextureEXT(target, texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteTexturesEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, textures, A0),
    struct Library *, MesaBase, 839, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteTexturesEXT(n, textures);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenTexturesEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, textures, A0),
    struct Library *, MesaBase, 840, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenTexturesEXT(n, textures);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsTextureEXT,
    AROS_LHA(GLuint, texture, D0),
    struct Library *, MesaBase, 841, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsTextureEXT(texture);

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

    glPrioritizeTexturesEXT(n, textures, priorities);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glArrayElementEXT,
    AROS_LHA(GLint, i, D0),
    struct Library *, MesaBase, 843, Mesa)
{
    AROS_LIBFUNC_INIT

    glArrayElementEXT(i);

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

    glColorPointerEXT(size, type, stride, count, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glDrawArraysEXT,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLint, first, D1),
    AROS_LHA(GLsizei, count, D2),
    struct Library *, MesaBase, 845, Mesa)
{
    AROS_LIBFUNC_INIT

    glDrawArraysEXT(mode, first, count);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glEdgeFlagPointerEXT,
    AROS_LHA(GLsizei, stride, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLboolean *, pointer, A0),
    struct Library *, MesaBase, 846, Mesa)
{
    AROS_LIBFUNC_INIT

    glEdgeFlagPointerEXT(stride, count, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPointervEXT,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 847, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetPointervEXT(pname, params);

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

    glIndexPointerEXT(type, stride, count, pointer);

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

    glNormalPointerEXT(type, stride, count, pointer);

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

    glTexCoordPointerEXT(size, type, stride, count, pointer);

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

    glVertexPointerEXT(size, type, stride, count, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBlendEquationEXT,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 852, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendEquationEXT(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfEXT,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 853, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameterfEXT(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfvEXT,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 854, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameterfvEXT(pname, params);

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

    glColorTableEXT(target, internalFormat, width, format, type, table);

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

    glGetColorTableEXT(target, format, type, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 857, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetColorTableParameterivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 858, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetColorTableParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glLockArraysEXT,
    AROS_LHA(GLint, first, D0),
    AROS_LHA(GLsizei, count, D1),
    struct Library *, MesaBase, 859, Mesa)
{
    AROS_LIBFUNC_INIT

    glLockArraysEXT(first, count);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glUnlockArraysEXT,
    struct Library *, MesaBase, 860, Mesa)
{
    AROS_LIBFUNC_INIT

    glUnlockArraysEXT();

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

    glDrawRangeElementsEXT(mode, start, end, count, type, indices);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3bEXT,
    AROS_LHA(GLbyte, red, D0),
    AROS_LHA(GLbyte, green, D1),
    AROS_LHA(GLbyte, blue, D2),
    struct Library *, MesaBase, 862, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3bEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3bvEXT,
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 863, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3bvEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3dEXT,
    AROS_LHA(GLdouble, red, D0),
    AROS_LHA(GLdouble, green, D1),
    AROS_LHA(GLdouble, blue, D2),
    struct Library *, MesaBase, 864, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3dEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3dvEXT,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 865, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3dvEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3fEXT,
    AROS_LHA(GLfloat, red, D0),
    AROS_LHA(GLfloat, green, D1),
    AROS_LHA(GLfloat, blue, D2),
    struct Library *, MesaBase, 866, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3fEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3fvEXT,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 867, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3fvEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3iEXT,
    AROS_LHA(GLint, red, D0),
    AROS_LHA(GLint, green, D1),
    AROS_LHA(GLint, blue, D2),
    struct Library *, MesaBase, 868, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3iEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3ivEXT,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 869, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3ivEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3sEXT,
    AROS_LHA(GLshort, red, D0),
    AROS_LHA(GLshort, green, D1),
    AROS_LHA(GLshort, blue, D2),
    struct Library *, MesaBase, 870, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3sEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3svEXT,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 871, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3svEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3ubEXT,
    AROS_LHA(GLubyte, red, D0),
    AROS_LHA(GLubyte, green, D1),
    AROS_LHA(GLubyte, blue, D2),
    struct Library *, MesaBase, 872, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3ubEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3ubvEXT,
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 873, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3ubvEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3uiEXT,
    AROS_LHA(GLuint, red, D0),
    AROS_LHA(GLuint, green, D1),
    AROS_LHA(GLuint, blue, D2),
    struct Library *, MesaBase, 874, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3uiEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3uivEXT,
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 875, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3uivEXT(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSecondaryColor3usEXT,
    AROS_LHA(GLushort, red, D0),
    AROS_LHA(GLushort, green, D1),
    AROS_LHA(GLushort, blue, D2),
    struct Library *, MesaBase, 876, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3usEXT(red, green, blue);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSecondaryColor3usvEXT,
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 877, Mesa)
{
    AROS_LIBFUNC_INIT

    glSecondaryColor3usvEXT(v);

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

    glSecondaryColorPointerEXT(size, type, stride, pointer);

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

    glMultiDrawArraysEXT(mode, first, count, primcount);

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

    glMultiDrawElementsEXT(mode, count, type, indices, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordfEXT,
    AROS_LHA(GLfloat, coord, D0),
    struct Library *, MesaBase, 881, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoordfEXT(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoordfvEXT,
    AROS_LHA(const GLfloat *, coord, A0),
    struct Library *, MesaBase, 882, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoordfvEXT(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoorddEXT,
    AROS_LHA(GLdouble, coord, D0),
    struct Library *, MesaBase, 883, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoorddEXT(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFogCoorddvEXT,
    AROS_LHA(const GLdouble *, coord, A0),
    struct Library *, MesaBase, 884, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoorddvEXT(coord);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFogCoordPointerEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLsizei, stride, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 885, Mesa)
{
    AROS_LIBFUNC_INIT

    glFogCoordPointerEXT(type, stride, pointer);

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

    glBlendFuncSeparateEXT(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glFlushVertexArrayRangeNV,
    struct Library *, MesaBase, 887, Mesa)
{
    AROS_LIBFUNC_INIT

    glFlushVertexArrayRangeNV();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexArrayRangeNV,
    AROS_LHA(GLsizei, length, D0),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 888, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexArrayRangeNV(length, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameterfvNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 889, Mesa)
{
    AROS_LIBFUNC_INIT

    glCombinerParameterfvNV(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameterfNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 890, Mesa)
{
    AROS_LIBFUNC_INIT

    glCombinerParameterfNV(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameterivNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 891, Mesa)
{
    AROS_LIBFUNC_INIT

    glCombinerParameterivNV(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glCombinerParameteriNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 892, Mesa)
{
    AROS_LIBFUNC_INIT

    glCombinerParameteriNV(pname, param);

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

    glCombinerInputNV(stage, portion, variable, input, mapping, componentUsage);

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

    glCombinerOutputNV(stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum);

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

    glFinalCombinerInputNV(variable, input, mapping, componentUsage);

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

    glGetCombinerInputParameterfvNV(stage, portion, variable, pname, params);

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

    glGetCombinerInputParameterivNV(stage, portion, variable, pname, params);

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

    glGetCombinerOutputParameterfvNV(stage, portion, pname, params);

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

    glGetCombinerOutputParameterivNV(stage, portion, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetFinalCombinerInputParameterfvNV,
    AROS_LHA(GLenum, variable, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 900, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetFinalCombinerInputParameterfvNV(variable, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetFinalCombinerInputParameterivNV,
    AROS_LHA(GLenum, variable, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 901, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetFinalCombinerInputParameterivNV(variable, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glResizeBuffersMESA,
    struct Library *, MesaBase, 902, Mesa)
{
    AROS_LIBFUNC_INIT

    glResizeBuffersMESA();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2dMESA,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    struct Library *, MesaBase, 903, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2dMESA(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2dvMESA,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 904, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2dvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2fMESA,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    struct Library *, MesaBase, 905, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2fMESA(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2fvMESA,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 906, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2fvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2iMESA,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    struct Library *, MesaBase, 907, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2iMESA(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2ivMESA,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 908, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2ivMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glWindowPos2sMESA,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    struct Library *, MesaBase, 909, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2sMESA(x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos2svMESA,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 910, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos2svMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3dMESA,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, z, D2),
    struct Library *, MesaBase, 911, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3dMESA(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3dvMESA,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 912, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3dvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3fMESA,
    AROS_LHA(GLfloat, x, D0),
    AROS_LHA(GLfloat, y, D1),
    AROS_LHA(GLfloat, z, D2),
    struct Library *, MesaBase, 913, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3fMESA(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3fvMESA,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 914, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3fvMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3iMESA,
    AROS_LHA(GLint, x, D0),
    AROS_LHA(GLint, y, D1),
    AROS_LHA(GLint, z, D2),
    struct Library *, MesaBase, 915, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3iMESA(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3ivMESA,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 916, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3ivMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glWindowPos3sMESA,
    AROS_LHA(GLshort, x, D0),
    AROS_LHA(GLshort, y, D1),
    AROS_LHA(GLshort, z, D2),
    struct Library *, MesaBase, 917, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3sMESA(x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos3svMESA,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 918, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos3svMESA(v);

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

    glWindowPos4dMESA(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4dvMESA,
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 920, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos4dvMESA(v);

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

    glWindowPos4fMESA(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4fvMESA,
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 922, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos4fvMESA(v);

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

    glWindowPos4iMESA(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4ivMESA,
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 924, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos4ivMESA(v);

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

    glWindowPos4sMESA(x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glWindowPos4svMESA,
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 926, Mesa)
{
    AROS_LIBFUNC_INIT

    glWindowPos4svMESA(v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLboolean, glAreProgramsResidentNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    AROS_LHA(GLboolean *, residences, A1),
    struct Library *, MesaBase, 927, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glAreProgramsResidentNV(n, programs, residences);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindProgramNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 928, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindProgramNV(target, id);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteProgramsNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    struct Library *, MesaBase, 929, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteProgramsNV(n, programs);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glExecuteProgramNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 930, Mesa)
{
    AROS_LIBFUNC_INIT

    glExecuteProgramNV(target, id, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenProgramsNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, programs, A0),
    struct Library *, MesaBase, 931, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenProgramsNV(n, programs);

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

    glGetProgramParameterdvNV(target, index, pname, params);

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

    glGetProgramParameterfvNV(target, index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramivNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 934, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetProgramivNV(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetProgramStringNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLubyte *, program, A0),
    struct Library *, MesaBase, 935, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetProgramStringNV(id, pname, program);

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

    glGetTrackMatrixivNV(target, address, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribdvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLdouble *, params, A0),
    struct Library *, MesaBase, 937, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribdvNV(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribfvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 938, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribfvNV(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribivNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 939, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribivNV(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribPointervNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, pointer, A0),
    struct Library *, MesaBase, 940, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribPointervNV(index, pname, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsProgramNV,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 941, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsProgramNV(id);

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

    glLoadProgramNV(target, id, len, program);

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

    glProgramParameter4dNV(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramParameter4dvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 944, Mesa)
{
    AROS_LIBFUNC_INIT

    glProgramParameter4dvNV(target, index, v);

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

    glProgramParameter4fNV(target, index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramParameter4fvNV,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 946, Mesa)
{
    AROS_LIBFUNC_INIT

    glProgramParameter4fvNV(target, index, v);

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

    glProgramParameters4dvNV(target, index, count, v);

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

    glProgramParameters4fvNV(target, index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glRequestResidentProgramsNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, programs, A0),
    struct Library *, MesaBase, 949, Mesa)
{
    AROS_LIBFUNC_INIT

    glRequestResidentProgramsNV(n, programs);

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

    glTrackMatrixNV(target, address, matrix, transform);

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

    glVertexAttribPointerNV(index, fsize, type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    struct Library *, MesaBase, 952, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1dNV(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 953, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1dvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    struct Library *, MesaBase, 954, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1fNV(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 955, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1fvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    struct Library *, MesaBase, 956, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1sNV(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib1svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 957, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib1svNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2dNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLdouble, x, D1),
    AROS_LHA(GLdouble, y, D2),
    struct Library *, MesaBase, 958, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2dNV(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 959, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2dvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2fNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLfloat, x, D1),
    AROS_LHA(GLfloat, y, D2),
    struct Library *, MesaBase, 960, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2fNV(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 961, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2fvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttrib2sNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLshort, x, D1),
    AROS_LHA(GLshort, y, D2),
    struct Library *, MesaBase, 962, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2sNV(index, x, y);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib2svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 963, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib2svNV(index, v);

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

    glVertexAttrib3dNV(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 965, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib3dvNV(index, v);

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

    glVertexAttrib3fNV(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 967, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib3fvNV(index, v);

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

    glVertexAttrib3sNV(index, x, y, z);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib3svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 969, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib3svNV(index, v);

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

    glVertexAttrib4dNV(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 971, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4dvNV(index, v);

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

    glVertexAttrib4fNV(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 973, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4fvNV(index, v);

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

    glVertexAttrib4sNV(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 975, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4svNV(index, v);

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

    glVertexAttrib4ubNV(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttrib4ubvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 977, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttrib4ubvNV(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs1dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 978, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs1dvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs1fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 979, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs1fvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs1svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 980, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs1svNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs2dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 981, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs2dvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs2fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 982, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs2fvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs2svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 983, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs2svNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs3dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 984, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs3dvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs3fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 985, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs3fvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs3svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 986, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs3svNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4dvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLdouble *, v, A0),
    struct Library *, MesaBase, 987, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs4dvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4fvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLfloat *, v, A0),
    struct Library *, MesaBase, 988, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs4fvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4svNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 989, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs4svNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribs4ubvNV,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 990, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribs4ubvNV(index, count, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexBumpParameterivATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, param, A0),
    struct Library *, MesaBase, 991, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexBumpParameterivATI(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glTexBumpParameterfvATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, param, A0),
    struct Library *, MesaBase, 992, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexBumpParameterfvATI(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetTexBumpParameterivATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint *, param, A0),
    struct Library *, MesaBase, 993, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexBumpParameterivATI(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetTexBumpParameterfvATI,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat *, param, A0),
    struct Library *, MesaBase, 994, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexBumpParameterfvATI(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLuint, glGenFragmentShadersATI,
    AROS_LHA(GLuint, range, D0),
    struct Library *, MesaBase, 995, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = glGenFragmentShadersATI(range);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBindFragmentShaderATI,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 996, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindFragmentShaderATI(id);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteFragmentShaderATI,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 997, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteFragmentShaderATI(id);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glBeginFragmentShaderATI,
    struct Library *, MesaBase, 998, Mesa)
{
    AROS_LIBFUNC_INIT

    glBeginFragmentShaderATI();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndFragmentShaderATI,
    struct Library *, MesaBase, 999, Mesa)
{
    AROS_LIBFUNC_INIT

    glEndFragmentShaderATI();

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glPassTexCoordATI,
    AROS_LHA(GLuint, dst, D0),
    AROS_LHA(GLuint, coord, D1),
    AROS_LHA(GLenum, swizzle, D2),
    struct Library *, MesaBase, 1000, Mesa)
{
    AROS_LIBFUNC_INIT

    glPassTexCoordATI(dst, coord, swizzle);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSampleMapATI,
    AROS_LHA(GLuint, dst, D0),
    AROS_LHA(GLuint, interp, D1),
    AROS_LHA(GLenum, swizzle, D2),
    struct Library *, MesaBase, 1001, Mesa)
{
    AROS_LIBFUNC_INIT

    glSampleMapATI(dst, interp, swizzle);

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

    glColorFragmentOp1ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod);

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

    glColorFragmentOp2ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);

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

    glColorFragmentOp3ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);

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

    glAlphaFragmentOp1ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod);

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

    glAlphaFragmentOp2ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);

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

    glAlphaFragmentOp3ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSetFragmentShaderConstantATI,
    AROS_LHA(GLuint, dst, D0),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 1008, Mesa)
{
    AROS_LIBFUNC_INIT

    glSetFragmentShaderConstantATI(dst, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameteriNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 1009, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameteriNV(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterivNV,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1010, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameterivNV(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawBuffersATI,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLenum *, bufs, A0),
    struct Library *, MesaBase, 1011, Mesa)
{
    AROS_LIBFUNC_INIT

    glDrawBuffersATI(n, bufs);

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

    glProgramNamedParameter4fNV(id, len, name, x, y, z, w);

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

    glProgramNamedParameter4dNV(id, len, name, x, y, z, w);

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

    glProgramNamedParameter4fvNV(id, len, name, v);

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

    glProgramNamedParameter4dvNV(id, len, name, v);

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

    glGetProgramNamedParameterfvNV(id, len, name, params);

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

    glGetProgramNamedParameterdvNV(id, len, name, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsRenderbufferEXT,
    AROS_LHA(GLuint, renderbuffer, D0),
    struct Library *, MesaBase, 1018, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsRenderbufferEXT(renderbuffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindRenderbufferEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, renderbuffer, D1),
    struct Library *, MesaBase, 1019, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindRenderbufferEXT(target, renderbuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteRenderbuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 1020, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteRenderbuffersEXT(n, renderbuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenRenderbuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, renderbuffers, A0),
    struct Library *, MesaBase, 1021, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenRenderbuffersEXT(n, renderbuffers);

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

    glRenderbufferStorageEXT(target, internalformat, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetRenderbufferParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1023, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetRenderbufferParameterivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsFramebufferEXT,
    AROS_LHA(GLuint, framebuffer, D0),
    struct Library *, MesaBase, 1024, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsFramebufferEXT(framebuffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindFramebufferEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, framebuffer, D1),
    struct Library *, MesaBase, 1025, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindFramebufferEXT(target, framebuffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteFramebuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 1026, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteFramebuffersEXT(n, framebuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenFramebuffersEXT,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, framebuffers, A0),
    struct Library *, MesaBase, 1027, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenFramebuffersEXT(n, framebuffers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLenum, glCheckFramebufferStatusEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 1028, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = glCheckFramebufferStatusEXT(target);

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

    glFramebufferTexture1DEXT(target, attachment, textarget, texture, level);

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

    glFramebufferTexture2DEXT(target, attachment, textarget, texture, level);

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

    glFramebufferTexture3DEXT(target, attachment, textarget, texture, level, zoffset);

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

    glFramebufferRenderbufferEXT(target, attachment, renderbuffertarget, renderbuffer);

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

    glGetFramebufferAttachmentParameterivEXT(target, attachment, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glGenerateMipmapEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 1034, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenerateMipmapEXT(target);

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

    glFramebufferTextureLayerEXT(target, attachment, texture, level, layer);

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

    GLvoid* _return = glMapBufferRange(target, offset, length, access);

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

    glFlushMappedBufferRange(target, offset, length);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBindVertexArray,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 1038, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindVertexArray(array);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteVertexArrays,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, arrays, A0),
    struct Library *, MesaBase, 1039, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteVertexArrays(n, arrays);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenVertexArrays,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, arrays, A0),
    struct Library *, MesaBase, 1040, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenVertexArrays(n, arrays);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsVertexArray,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 1041, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsVertexArray(array);

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

    glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLsync, glFenceSync,
    AROS_LHA(GLenum, condition, D0),
    AROS_LHA(GLbitfield, flags, D1),
    struct Library *, MesaBase, 1043, Mesa)
{
    AROS_LIBFUNC_INIT

    GLsync _return = glFenceSync(condition, flags);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsSync,
    AROS_LHA(GLsync, sync, D0),
    struct Library *, MesaBase, 1044, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsSync(sync);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glDeleteSync,
    AROS_LHA(GLsync, sync, D0),
    struct Library *, MesaBase, 1045, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteSync(sync);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLenum, glClientWaitSync,
    AROS_LHA(GLsync, sync, D0),
    AROS_LHA(GLbitfield, flags, D1),
    AROS_LHA(GLuint64, timeout, D2),
    struct Library *, MesaBase, 1046, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = glClientWaitSync(sync, flags, timeout);

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

    glWaitSync(sync, flags, timeout);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetInteger64v,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint64 *, params, A0),
    struct Library *, MesaBase, 1048, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetInteger64v(pname, params);

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

    glGetSynciv(sync, pname, bufSize, length, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glProvokingVertexEXT,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 1050, Mesa)
{
    AROS_LIBFUNC_INIT

    glProvokingVertexEXT(mode);

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

    glDrawElementsBaseVertex(mode, count, type, indices, basevertex);

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

    glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);

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

    glMultiDrawElementsBaseVertex(mode, count, type, indices, primcount, basevertex);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glProvokingVertex,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 1054, Mesa)
{
    AROS_LIBFUNC_INIT

    glProvokingVertex(mode);

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

    glRenderbufferStorageMultisampleEXT(target, samples, internalformat, width, height);

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

    glColorMaskIndexedEXT(index, r, g, b, a);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBooleanIndexedvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLboolean *, data, A0),
    struct Library *, MesaBase, 1057, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetBooleanIndexedvEXT(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetIntegerIndexedvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLint *, data, A0),
    struct Library *, MesaBase, 1058, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetIntegerIndexedvEXT(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEnableIndexedEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1059, Mesa)
{
    AROS_LIBFUNC_INIT

    glEnableIndexedEXT(target, index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDisableIndexedEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1060, Mesa)
{
    AROS_LIBFUNC_INIT

    glDisableIndexedEXT(target, index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLboolean, glIsEnabledIndexedEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1061, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsEnabledIndexedEXT(target, index);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginConditionalRenderNV,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 1062, Mesa)
{
    AROS_LIBFUNC_INIT

    glBeginConditionalRenderNV(id, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndConditionalRenderNV,
    struct Library *, MesaBase, 1063, Mesa)
{
    AROS_LIBFUNC_INIT

    glEndConditionalRenderNV();

    AROS_LIBFUNC_EXIT
}

AROS_LH3(GLenum, glObjectPurgeableAPPLE,
    AROS_LHA(GLenum, objectType, D0),
    AROS_LHA(GLuint, name, D1),
    AROS_LHA(GLenum, option, D2),
    struct Library *, MesaBase, 1064, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = glObjectPurgeableAPPLE(objectType, name, option);

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

    GLenum _return = glObjectUnpurgeableAPPLE(objectType, name, option);

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

    glGetObjectParameterivAPPLE(objectType, name, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBeginTransformFeedback,
    AROS_LHA(GLenum, primitiveMode, D0),
    struct Library *, MesaBase, 1067, Mesa)
{
    AROS_LIBFUNC_INIT

    glBeginTransformFeedback(primitiveMode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndTransformFeedback,
    struct Library *, MesaBase, 1068, Mesa)
{
    AROS_LIBFUNC_INIT

    glEndTransformFeedback();

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

    glBindBufferRange(target, index, buffer, offset, size);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindBufferBase,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    struct Library *, MesaBase, 1070, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindBufferBase(target, index, buffer);

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

    glTransformFeedbackVaryings(program, count, varyings, bufferMode);

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

    glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);

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

    glDrawArraysInstanced(mode, first, count, primcount);

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

    glDrawElementsInstanced(mode, count, type, indices, primcount);

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

    glDrawArraysInstancedARB(mode, first, count, primcount);

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

    glDrawElementsInstancedARB(mode, count, type, indices, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glProgramParameteriARB,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, value, D2),
    struct Library *, MesaBase, 1077, Mesa)
{
    AROS_LIBFUNC_INIT

    glProgramParameteriARB(program, pname, value);

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

    glFramebufferTextureARB(target, attachment, texture, level);

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

    glFramebufferTextureFaceARB(target, attachment, texture, level, face);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindTransformFeedback,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 1080, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindTransformFeedback(target, id);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteTransformFeedbacks,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, ids, A0),
    struct Library *, MesaBase, 1081, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteTransformFeedbacks(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenTransformFeedbacks,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, ids, A0),
    struct Library *, MesaBase, 1082, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenTransformFeedbacks(n, ids);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsTransformFeedback,
    AROS_LHA(GLuint, id, D0),
    struct Library *, MesaBase, 1083, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsTransformFeedback(id);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPauseTransformFeedback,
    struct Library *, MesaBase, 1084, Mesa)
{
    AROS_LIBFUNC_INIT

    glPauseTransformFeedback();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glResumeTransformFeedback,
    struct Library *, MesaBase, 1085, Mesa)
{
    AROS_LIBFUNC_INIT

    glResumeTransformFeedback();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDrawTransformFeedback,
    AROS_LHA(GLenum, mode, D0),
    AROS_LHA(GLuint, id, D1),
    struct Library *, MesaBase, 1086, Mesa)
{
    AROS_LIBFUNC_INIT

    glDrawTransformFeedback(mode, id);

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

    glDrawArraysInstancedEXT(mode, start, count, primcount);

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

    glDrawElementsInstancedEXT(mode, count, type, indices, primcount);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBeginTransformFeedbackEXT,
    AROS_LHA(GLenum, primitiveMode, D0),
    struct Library *, MesaBase, 1089, Mesa)
{
    AROS_LIBFUNC_INIT

    glBeginTransformFeedbackEXT(primitiveMode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndTransformFeedbackEXT,
    struct Library *, MesaBase, 1090, Mesa)
{
    AROS_LIBFUNC_INIT

    glEndTransformFeedbackEXT();

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

    glBindBufferRangeEXT(target, index, buffer, offset, size);

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

    glBindBufferOffsetEXT(target, index, buffer, offset);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindBufferBaseEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLuint, buffer, D2),
    struct Library *, MesaBase, 1093, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindBufferBaseEXT(target, index, buffer);

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

    glTransformFeedbackVaryingsEXT(program, count, varyings, bufferMode);

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

    glGetTransformFeedbackVaryingEXT(program, index, bufSize, length, size, type, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEGLImageTargetTexture2DOES,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLeglImageOES, image, D1),
    struct Library *, MesaBase, 1096, Mesa)
{
    AROS_LIBFUNC_INIT

    glEGLImageTargetTexture2DOES(target, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEGLImageTargetRenderbufferStorageOES,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLeglImageOES, image, D1),
    struct Library *, MesaBase, 1097, Mesa)
{
    AROS_LIBFUNC_INIT

    glEGLImageTargetRenderbufferStorageOES(target, image);

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

    glColorMaski(index, r, g, b, a);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBooleani_v,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLboolean *, data, A0),
    struct Library *, MesaBase, 1099, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetBooleani_v(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetIntegeri_v,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLint *, data, A0),
    struct Library *, MesaBase, 1100, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetIntegeri_v(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glEnablei,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1101, Mesa)
{
    AROS_LIBFUNC_INIT

    glEnablei(target, index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDisablei,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1102, Mesa)
{
    AROS_LIBFUNC_INIT

    glDisablei(target, index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLboolean, glIsEnabledi,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1103, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsEnabledi(target, index);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glClampColor,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, clamp, D1),
    struct Library *, MesaBase, 1104, Mesa)
{
    AROS_LIBFUNC_INIT

    glClampColor(target, clamp);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBeginConditionalRender,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 1105, Mesa)
{
    AROS_LIBFUNC_INIT

    glBeginConditionalRender(id, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glEndConditionalRender,
    struct Library *, MesaBase, 1106, Mesa)
{
    AROS_LIBFUNC_INIT

    glEndConditionalRender();

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

    glVertexAttribIPointer(index, size, type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribIiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1108, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribIiv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribIuiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1109, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribIuiv(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1i,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    struct Library *, MesaBase, 1110, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI1i(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribI2i,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    AROS_LHA(GLint, y, D2),
    struct Library *, MesaBase, 1111, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI2i(index, x, y);

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

    glVertexAttribI3i(index, x, y, z);

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

    glVertexAttribI4i(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1ui,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    struct Library *, MesaBase, 1114, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI1ui(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribI2ui,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    AROS_LHA(GLuint, y, D2),
    struct Library *, MesaBase, 1115, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI2ui(index, x, y);

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

    glVertexAttribI3ui(index, x, y, z);

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

    glVertexAttribI4ui(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1118, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI1iv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI2iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1119, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI2iv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI3iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1120, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI3iv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4iv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1121, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4iv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1122, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI1uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI2uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1123, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI2uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI3uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1124, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI3uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4uiv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1125, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4uiv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4bv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 1126, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4bv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4sv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 1127, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4sv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4ubv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 1128, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4ubv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4usv,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 1129, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4usv(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformuiv,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1130, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetUniformuiv(program, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindFragDataLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, color, D1),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 1131, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindFragDataLocation(program, color, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetFragDataLocation,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 1132, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = glGetFragDataLocation(program, name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1ui,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    struct Library *, MesaBase, 1133, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1ui(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2ui,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    AROS_LHA(GLuint, v1, D2),
    struct Library *, MesaBase, 1134, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2ui(location, v0, v1);

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

    glUniform3ui(location, v0, v1, v2);

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

    glUniform4ui(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1uiv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1137, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1uiv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2uiv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1138, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2uiv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3uiv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1139, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform3uiv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4uiv,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1140, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform4uiv(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterIiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1141, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexParameterIiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterIuiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLuint *, params, A0),
    struct Library *, MesaBase, 1142, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexParameterIuiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterIiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1143, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexParameterIiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterIuiv,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1144, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexParameterIuiv(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glClearBufferiv,
    AROS_LHA(GLenum, buffer, D0),
    AROS_LHA(GLint, drawbuffer, D1),
    AROS_LHA(const GLint *, value, A0),
    struct Library *, MesaBase, 1145, Mesa)
{
    AROS_LIBFUNC_INIT

    glClearBufferiv(buffer, drawbuffer, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glClearBufferuiv,
    AROS_LHA(GLenum, buffer, D0),
    AROS_LHA(GLint, drawbuffer, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1146, Mesa)
{
    AROS_LIBFUNC_INIT

    glClearBufferuiv(buffer, drawbuffer, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glClearBufferfv,
    AROS_LHA(GLenum, buffer, D0),
    AROS_LHA(GLint, drawbuffer, D1),
    AROS_LHA(const GLfloat *, value, A0),
    struct Library *, MesaBase, 1147, Mesa)
{
    AROS_LIBFUNC_INIT

    glClearBufferfv(buffer, drawbuffer, value);

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

    glClearBufferfi(buffer, drawbuffer, depth, stencil);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(const GLubyte *, glGetStringi,
    AROS_LHA(GLenum, name, D0),
    AROS_LHA(GLuint, index, D1),
    struct Library *, MesaBase, 1149, Mesa)
{
    AROS_LIBFUNC_INIT

    const GLubyte * _return = glGetStringi(name, index);

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

    glTexBuffer(target, internalformat, buffer);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPrimitiveRestartIndex,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 1151, Mesa)
{
    AROS_LIBFUNC_INIT

    glPrimitiveRestartIndex(index);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetInteger64i_v,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLuint, index, D1),
    AROS_LHA(GLint64 *, data, A0),
    struct Library *, MesaBase, 1152, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetInteger64i_v(target, index, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetBufferParameteri64v,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint64 *, params, A0),
    struct Library *, MesaBase, 1153, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetBufferParameteri64v(target, pname, params);

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

    glFramebufferTexture(target, attachment, texture, level);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribDivisor,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, divisor, D1),
    struct Library *, MesaBase, 1155, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribDivisor(index, divisor);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glPrimitiveRestartNV,
    struct Library *, MesaBase, 1156, Mesa)
{
    AROS_LIBFUNC_INIT

    glPrimitiveRestartNV();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPrimitiveRestartIndexNV,
    AROS_LHA(GLuint, index, D0),
    struct Library *, MesaBase, 1157, Mesa)
{
    AROS_LIBFUNC_INIT

    glPrimitiveRestartIndexNV(index);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1iEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    struct Library *, MesaBase, 1158, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI1iEXT(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribI2iEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLint, x, D1),
    AROS_LHA(GLint, y, D2),
    struct Library *, MesaBase, 1159, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI2iEXT(index, x, y);

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

    glVertexAttribI3iEXT(index, x, y, z);

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

    glVertexAttribI4iEXT(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1uiEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    struct Library *, MesaBase, 1162, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI1uiEXT(index, x);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glVertexAttribI2uiEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, x, D1),
    AROS_LHA(GLuint, y, D2),
    struct Library *, MesaBase, 1163, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI2uiEXT(index, x, y);

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

    glVertexAttribI3uiEXT(index, x, y, z);

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

    glVertexAttribI4uiEXT(index, x, y, z, w);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1ivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1166, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI1ivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI2ivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1167, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI2ivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI3ivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1168, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI3ivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4ivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLint *, v, A0),
    struct Library *, MesaBase, 1169, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4ivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI1uivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1170, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI1uivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI2uivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1171, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI2uivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI3uivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1172, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI3uivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4uivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLuint *, v, A0),
    struct Library *, MesaBase, 1173, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4uivEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4bvEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLbyte *, v, A0),
    struct Library *, MesaBase, 1174, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4bvEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4svEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLshort *, v, A0),
    struct Library *, MesaBase, 1175, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4svEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4ubvEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLubyte *, v, A0),
    struct Library *, MesaBase, 1176, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4ubvEXT(index, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribI4usvEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(const GLushort *, v, A0),
    struct Library *, MesaBase, 1177, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribI4usvEXT(index, v);

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

    glVertexAttribIPointerEXT(index, size, type, stride, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribIivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1179, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribIivEXT(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetVertexAttribIuivEXT,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1180, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetVertexAttribIuivEXT(index, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetUniformuivEXT,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLint, location, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1181, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetUniformuivEXT(program, location, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBindFragDataLocationEXT,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(GLuint, color, D1),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 1182, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindFragDataLocationEXT(program, color, name);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLint, glGetFragDataLocationEXT,
    AROS_LHA(GLuint, program, D0),
    AROS_LHA(const GLchar *, name, A0),
    struct Library *, MesaBase, 1183, Mesa)
{
    AROS_LIBFUNC_INIT

    GLint _return = glGetFragDataLocationEXT(program, name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUniform1uiEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    struct Library *, MesaBase, 1184, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1uiEXT(location, v0);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2uiEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLuint, v0, D1),
    AROS_LHA(GLuint, v1, D2),
    struct Library *, MesaBase, 1185, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2uiEXT(location, v0, v1);

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

    glUniform3uiEXT(location, v0, v1, v2);

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

    glUniform4uiEXT(location, v0, v1, v2, v3);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform1uivEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1188, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform1uivEXT(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform2uivEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1189, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform2uivEXT(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform3uivEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1190, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform3uivEXT(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glUniform4uivEXT,
    AROS_LHA(GLint, location, D0),
    AROS_LHA(GLsizei, count, D1),
    AROS_LHA(const GLuint *, value, A0),
    struct Library *, MesaBase, 1191, Mesa)
{
    AROS_LIBFUNC_INIT

    glUniform4uivEXT(location, count, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterIivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1192, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexParameterIivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexParameterIuivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLuint *, params, A0),
    struct Library *, MesaBase, 1193, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexParameterIuivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterIivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1194, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexParameterIivEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterIuivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1195, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexParameterIuivEXT(target, pname, params);

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

    glClearColorIiEXT(red, green, blue, alpha);

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

    glClearColorIuiEXT(red, green, blue, alpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glUseShaderProgramEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(GLuint, program, D1),
    struct Library *, MesaBase, 1198, Mesa)
{
    AROS_LIBFUNC_INIT

    glUseShaderProgramEXT(type, program);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveProgramEXT,
    AROS_LHA(GLuint, program, D0),
    struct Library *, MesaBase, 1199, Mesa)
{
    AROS_LIBFUNC_INIT

    glActiveProgramEXT(program);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLuint, glCreateShaderProgramEXT,
    AROS_LHA(GLenum, type, D0),
    AROS_LHA(const GLchar *, string, A0),
    struct Library *, MesaBase, 1200, Mesa)
{
    AROS_LIBFUNC_INIT

    GLuint _return = glCreateShaderProgramEXT(type, string);

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

    glProgramEnvParameters4fvEXT(target, index, count, params);

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

    glProgramLocalParameters4fvEXT(target, index, count, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationSeparateATI,
    AROS_LHA(GLenum, modeRGB, D0),
    AROS_LHA(GLenum, modeA, D1),
    struct Library *, MesaBase, 1203, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendEquationSeparateATI(modeRGB, modeA);

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

    glGetHistogramEXT(target, reset, format, type, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1205, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetHistogramParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetHistogramParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1206, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetHistogramParameterivEXT(target, pname, params);

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

    glGetMinmaxEXT(target, reset, format, type, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1208, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetMinmaxParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetMinmaxParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1209, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetMinmaxParameterivEXT(target, pname, params);

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

    glHistogramEXT(target, width, internalformat, sink);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glMinmaxEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLboolean, sink, D2),
    struct Library *, MesaBase, 1211, Mesa)
{
    AROS_LIBFUNC_INIT

    glMinmaxEXT(target, internalformat, sink);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetHistogramEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 1212, Mesa)
{
    AROS_LIBFUNC_INIT

    glResetHistogramEXT(target);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glResetMinmaxEXT,
    AROS_LHA(GLenum, target, D0),
    struct Library *, MesaBase, 1213, Mesa)
{
    AROS_LIBFUNC_INIT

    glResetMinmaxEXT(target);

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

    glConvolutionFilter1DEXT(target, internalformat, width, format, type, image);

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

    glConvolutionFilter2DEXT(target, internalformat, width, height, format, type, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterfEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, params, D2),
    struct Library *, MesaBase, 1216, Mesa)
{
    AROS_LIBFUNC_INIT

    glConvolutionParameterfEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1217, Mesa)
{
    AROS_LIBFUNC_INIT

    glConvolutionParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameteriEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, params, D2),
    struct Library *, MesaBase, 1218, Mesa)
{
    AROS_LIBFUNC_INIT

    glConvolutionParameteriEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glConvolutionParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1219, Mesa)
{
    AROS_LIBFUNC_INIT

    glConvolutionParameterivEXT(target, pname, params);

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

    glCopyConvolutionFilter1DEXT(target, internalformat, x, y, width);

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

    glCopyConvolutionFilter2DEXT(target, internalformat, x, y, width, height);

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

    glGetConvolutionFilterEXT(target, format, type, image);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameterfvEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1223, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetConvolutionParameterfvEXT(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetConvolutionParameterivEXT,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1224, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetConvolutionParameterivEXT(target, pname, params);

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

    glGetSeparableFilterEXT(target, format, type, row, column, span);

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

    glSeparableFilter2DEXT(target, internalformat, width, height, format, type, row, column);

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

    glColorTableSGI(target, internalformat, width, format, type, table);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameterfvSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1228, Mesa)
{
    AROS_LIBFUNC_INIT

    glColorTableParameterfvSGI(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glColorTableParameterivSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1229, Mesa)
{
    AROS_LIBFUNC_INIT

    glColorTableParameterivSGI(target, pname, params);

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

    glCopyColorTableSGI(target, internalformat, x, y, width);

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

    glGetColorTableSGI(target, format, type, table);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterfvSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1232, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetColorTableParameterfvSGI(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetColorTableParameterivSGI,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1233, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetColorTableParameterivSGI(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glPixelTexGenSGIX,
    AROS_LHA(GLenum, mode, D0),
    struct Library *, MesaBase, 1234, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelTexGenSGIX(mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTexGenParameteriSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint, param, D1),
    struct Library *, MesaBase, 1235, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelTexGenParameteriSGIS(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTexGenParameterivSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLint *, params, A0),
    struct Library *, MesaBase, 1236, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelTexGenParameterivSGIS(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTexGenParameterfSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 1237, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelTexGenParameterfSGIS(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPixelTexGenParameterfvSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1238, Mesa)
{
    AROS_LIBFUNC_INIT

    glPixelTexGenParameterfvSGIS(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelTexGenParameterivSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1239, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetPixelTexGenParameterivSGIS(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetPixelTexGenParameterfvSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1240, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetPixelTexGenParameterfvSGIS(pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleMaskSGIS,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 1241, Mesa)
{
    AROS_LIBFUNC_INIT

    glSampleMaskSGIS(value, invert);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSamplePatternSGIS,
    AROS_LHA(GLenum, pattern, D0),
    struct Library *, MesaBase, 1242, Mesa)
{
    AROS_LIBFUNC_INIT

    glSamplePatternSGIS(pattern);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(GLfloat, param, D1),
    struct Library *, MesaBase, 1243, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameterfSGIS(pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glPointParameterfvSGIS,
    AROS_LHA(GLenum, pname, D0),
    AROS_LHA(const GLfloat *, params, A0),
    struct Library *, MesaBase, 1244, Mesa)
{
    AROS_LIBFUNC_INIT

    glPointParameterfvSGIS(pname, params);

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

    glColorSubTableEXT(target, start, count, format, type, data);

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

    glCopyColorSubTableEXT(target, start, x, y, width);

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

    glBlendFuncSeparateINGR(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);

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

    glMultiModeDrawArraysIBM(mode, first, count, primcount, modestride);

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

    glMultiModeDrawElementsIBM(mode, count, type, indices, primcount, modestride);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSampleMaskEXT,
    AROS_LHA(GLclampf, value, D0),
    AROS_LHA(GLboolean, invert, D1),
    struct Library *, MesaBase, 1250, Mesa)
{
    AROS_LIBFUNC_INIT

    glSampleMaskEXT(value, invert);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glSamplePatternEXT,
    AROS_LHA(GLenum, pattern, D0),
    struct Library *, MesaBase, 1251, Mesa)
{
    AROS_LIBFUNC_INIT

    glSamplePatternEXT(pattern);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteFencesNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, fences, A0),
    struct Library *, MesaBase, 1252, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteFencesNV(n, fences);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenFencesNV,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, fences, A0),
    struct Library *, MesaBase, 1253, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenFencesNV(n, fences);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsFenceNV,
    AROS_LHA(GLuint, fence, D0),
    struct Library *, MesaBase, 1254, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsFenceNV(fence);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glTestFenceNV,
    AROS_LHA(GLuint, fence, D0),
    struct Library *, MesaBase, 1255, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glTestFenceNV(fence);

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

    glGetFenceivNV(fence, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glFinishFenceNV,
    AROS_LHA(GLuint, fence, D0),
    struct Library *, MesaBase, 1257, Mesa)
{
    AROS_LIBFUNC_INIT

    glFinishFenceNV(fence);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glSetFenceNV,
    AROS_LHA(GLuint, fence, D0),
    AROS_LHA(GLenum, condition, D1),
    struct Library *, MesaBase, 1258, Mesa)
{
    AROS_LIBFUNC_INIT

    glSetFenceNV(fence, condition);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glActiveStencilFaceEXT,
    AROS_LHA(GLenum, face, D0),
    struct Library *, MesaBase, 1259, Mesa)
{
    AROS_LIBFUNC_INIT

    glActiveStencilFaceEXT(face);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glBindVertexArrayAPPLE,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 1260, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindVertexArrayAPPLE(array);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteVertexArraysAPPLE,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(const GLuint *, arrays, A0),
    struct Library *, MesaBase, 1261, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteVertexArraysAPPLE(n, arrays);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenVertexArraysAPPLE,
    AROS_LHA(GLsizei, n, D0),
    AROS_LHA(GLuint *, arrays, A0),
    struct Library *, MesaBase, 1262, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenVertexArraysAPPLE(n, arrays);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsVertexArrayAPPLE,
    AROS_LHA(GLuint, array, D0),
    struct Library *, MesaBase, 1263, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsVertexArrayAPPLE(array);

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

    glStencilOpSeparateATI(face, sfail, dpfail, dppass);

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

    glStencilFuncSeparateATI(frontfunc, backfunc, ref, mask);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDepthBoundsEXT,
    AROS_LHA(GLclampd, zmin, D0),
    AROS_LHA(GLclampd, zmax, D1),
    struct Library *, MesaBase, 1266, Mesa)
{
    AROS_LIBFUNC_INIT

    glDepthBoundsEXT(zmin, zmax);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationSeparateEXT,
    AROS_LHA(GLenum, modeRGB, D0),
    AROS_LHA(GLenum, modeAlpha, D1),
    struct Library *, MesaBase, 1267, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendEquationSeparateEXT(modeRGB, modeAlpha);

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

    glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjecti64vEXT,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint64EXT *, params, A0),
    struct Library *, MesaBase, 1269, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetQueryObjecti64vEXT(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetQueryObjectui64vEXT,
    AROS_LHA(GLuint, id, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint64EXT *, params, A0),
    struct Library *, MesaBase, 1270, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetQueryObjectui64vEXT(id, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBufferParameteriAPPLE,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 1271, Mesa)
{
    AROS_LIBFUNC_INIT

    glBufferParameteriAPPLE(target, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glFlushMappedBufferRangeAPPLE,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLintptr, offset, D1),
    AROS_LHA(GLsizeiptr, size, D2),
    struct Library *, MesaBase, 1272, Mesa)
{
    AROS_LIBFUNC_INIT

    glFlushMappedBufferRangeAPPLE(target, offset, size);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTextureRangeAPPLE,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLsizei, length, D1),
    AROS_LHA(const GLvoid *, pointer, A0),
    struct Library *, MesaBase, 1273, Mesa)
{
    AROS_LIBFUNC_INIT

    glTextureRangeAPPLE(target, length, pointer);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetTexParameterPointervAPPLE,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLvoid *  *, params, A0),
    struct Library *, MesaBase, 1274, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetTexParameterPointervAPPLE(target, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glClampColorARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, clamp, D1),
    struct Library *, MesaBase, 1275, Mesa)
{
    AROS_LIBFUNC_INIT

    glClampColorARB(target, clamp);

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

    glFramebufferTextureLayerARB(target, attachment, texture, level, layer);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glVertexAttribDivisorARB,
    AROS_LHA(GLuint, index, D0),
    AROS_LHA(GLuint, divisor, D1),
    struct Library *, MesaBase, 1277, Mesa)
{
    AROS_LIBFUNC_INIT

    glVertexAttribDivisorARB(index, divisor);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glTexBufferARB,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLenum, internalformat, D1),
    AROS_LHA(GLuint, buffer, D2),
    struct Library *, MesaBase, 1278, Mesa)
{
    AROS_LIBFUNC_INIT

    glTexBufferARB(target, internalformat, buffer);

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

    glDrawElementsInstancedBaseVertex(mode, count, type, indices, primcount, basevertex);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationiARB,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 1280, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendEquationiARB(buf, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBlendEquationSeparateiARB,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, modeRGB, D1),
    AROS_LHA(GLenum, modeAlpha, D2),
    struct Library *, MesaBase, 1281, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendEquationSeparateiARB(buf, modeRGB, modeAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBlendFunciARB,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, src, D1),
    AROS_LHA(GLenum, dst, D2),
    struct Library *, MesaBase, 1282, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendFunciARB(buf, src, dst);

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

    glBlendFuncSeparateiARB(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGenSamplers,
    AROS_LHA(GLsizei, count, D0),
    AROS_LHA(GLuint *, samplers, A0),
    struct Library *, MesaBase, 1284, Mesa)
{
    AROS_LIBFUNC_INIT

    glGenSamplers(count, samplers);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDeleteSamplers,
    AROS_LHA(GLsizei, count, D0),
    AROS_LHA(const GLuint *, samplers, A0),
    struct Library *, MesaBase, 1285, Mesa)
{
    AROS_LIBFUNC_INIT

    glDeleteSamplers(count, samplers);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GLboolean, glIsSampler,
    AROS_LHA(GLuint, sampler, D0),
    struct Library *, MesaBase, 1286, Mesa)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = glIsSampler(sampler);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBindSampler,
    AROS_LHA(GLuint, unit, D0),
    AROS_LHA(GLuint, sampler, D1),
    struct Library *, MesaBase, 1287, Mesa)
{
    AROS_LIBFUNC_INIT

    glBindSampler(unit, sampler);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameteri,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint, param, D2),
    struct Library *, MesaBase, 1288, Mesa)
{
    AROS_LIBFUNC_INIT

    glSamplerParameteri(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameteriv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, param, A0),
    struct Library *, MesaBase, 1289, Mesa)
{
    AROS_LIBFUNC_INIT

    glSamplerParameteriv(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameterf,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat, param, D2),
    struct Library *, MesaBase, 1290, Mesa)
{
    AROS_LIBFUNC_INIT

    glSamplerParameterf(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameterfv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLfloat *, param, A0),
    struct Library *, MesaBase, 1291, Mesa)
{
    AROS_LIBFUNC_INIT

    glSamplerParameterfv(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameterIiv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLint *, param, A0),
    struct Library *, MesaBase, 1292, Mesa)
{
    AROS_LIBFUNC_INIT

    glSamplerParameterIiv(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glSamplerParameterIuiv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(const GLuint *, param, A0),
    struct Library *, MesaBase, 1293, Mesa)
{
    AROS_LIBFUNC_INIT

    glSamplerParameterIuiv(sampler, pname, param);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetSamplerParameteriv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1294, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetSamplerParameteriv(sampler, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetSamplerParameterIiv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLint *, params, A0),
    struct Library *, MesaBase, 1295, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetSamplerParameterIiv(sampler, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetSamplerParameterfv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLfloat *, params, A0),
    struct Library *, MesaBase, 1296, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetSamplerParameterfv(sampler, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetSamplerParameterIuiv,
    AROS_LHA(GLuint, sampler, D0),
    AROS_LHA(GLenum, pname, D1),
    AROS_LHA(GLuint *, params, A0),
    struct Library *, MesaBase, 1297, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetSamplerParameterIuiv(sampler, pname, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glReleaseShaderCompiler,
    struct Library *, MesaBase, 1298, Mesa)
{
    AROS_LIBFUNC_INIT

    glReleaseShaderCompiler();

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

    glShaderBinary(count, shaders, binaryformat, binary, length);

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

    glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glDepthRangef,
    AROS_LHA(GLclampf, n, D0),
    AROS_LHA(GLclampf, f, D1),
    struct Library *, MesaBase, 1301, Mesa)
{
    AROS_LIBFUNC_INIT

    glDepthRangef(n, f);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, glClearDepthf,
    AROS_LHA(GLclampf, d, D0),
    struct Library *, MesaBase, 1302, Mesa)
{
    AROS_LIBFUNC_INIT

    glClearDepthf(d);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLenum, glGetGraphicsResetStatusARB,
    struct Library *, MesaBase, 1303, Mesa)
{
    AROS_LIBFUNC_INIT

    GLenum _return = glGetGraphicsResetStatusARB();

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

    glGetnMapdvARB(target, query, bufSize, v);

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

    glGetnMapfvARB(target, query, bufSize, v);

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

    glGetnMapivARB(target, query, bufSize, v);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetnPixelMapfvARB,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLfloat *, values, A0),
    struct Library *, MesaBase, 1307, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetnPixelMapfvARB(map, bufSize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetnPixelMapuivARB,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLuint *, values, A0),
    struct Library *, MesaBase, 1308, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetnPixelMapuivARB(map, bufSize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glGetnPixelMapusvARB,
    AROS_LHA(GLenum, map, D0),
    AROS_LHA(GLsizei, bufSize, D1),
    AROS_LHA(GLushort *, values, A0),
    struct Library *, MesaBase, 1309, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetnPixelMapusvARB(map, bufSize, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glGetnPolygonStippleARB,
    AROS_LHA(GLsizei, bufSize, D0),
    AROS_LHA(GLubyte *, pattern, A0),
    struct Library *, MesaBase, 1310, Mesa)
{
    AROS_LIBFUNC_INIT

    glGetnPolygonStippleARB(bufSize, pattern);

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

    glGetnColorTableARB(target, format, type, bufSize, table);

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

    glGetnConvolutionFilterARB(target, format, type, bufSize, image);

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

    glGetnSeparableFilterARB(target, format, type, rowBufSize, row, columnBufSize, column, span);

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

    glGetnHistogramARB(target, reset, format, type, bufSize, values);

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

    glGetnMinmaxARB(target, reset, format, type, bufSize, values);

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

    glGetnTexImageARB(target, level, format, type, bufSize, img);

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

    glReadnPixelsARB(x, y, width, height, format, type, bufSize, data);

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

    glGetnCompressedTexImageARB(target, lod, bufSize, img);

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

    glGetnUniformfvARB(program, location, bufSize, params);

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

    glGetnUniformivARB(program, location, bufSize, params);

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

    glGetnUniformuivARB(program, location, bufSize, params);

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

    glGetnUniformdvARB(program, location, bufSize, params);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBlendFuncIndexedAMD,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, src, D1),
    AROS_LHA(GLenum, dst, D2),
    struct Library *, MesaBase, 1323, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendFuncIndexedAMD(buf, src, dst);

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

    glBlendFuncSeparateIndexedAMD(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, glBlendEquationIndexedAMD,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, mode, D1),
    struct Library *, MesaBase, 1325, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendEquationIndexedAMD(buf, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, glBlendEquationSeparateIndexedAMD,
    AROS_LHA(GLuint, buf, D0),
    AROS_LHA(GLenum, modeRGB, D1),
    AROS_LHA(GLenum, modeAlpha, D2),
    struct Library *, MesaBase, 1326, Mesa)
{
    AROS_LIBFUNC_INIT

    glBlendEquationSeparateIndexedAMD(buf, modeRGB, modeAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, glTextureBarrierNV,
    struct Library *, MesaBase, 1327, Mesa)
{
    AROS_LIBFUNC_INIT

    glTextureBarrierNV();

    AROS_LIBFUNC_EXIT
}

