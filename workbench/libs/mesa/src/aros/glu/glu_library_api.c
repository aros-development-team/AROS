#include <GL/glu.h>
#include "gluapim.h"
#include "glu_intern.h"

AROS_LH1(void, gluBeginCurve,
    AROS_LHA(GLUnurbs *, nurb, A0),
    struct Library *, GLUBase, 35, Glu)
{
    AROS_LIBFUNC_INIT

    mgluBeginCurve(nurb);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluBeginPolygon,
    AROS_LHA(GLUtesselator *, tess, A0),
    struct Library *, GLUBase, 36, Glu)
{
    AROS_LIBFUNC_INIT

    mgluBeginPolygon(tess);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluBeginSurface,
    AROS_LHA(GLUnurbs *, nurb, A0),
    struct Library *, GLUBase, 37, Glu)
{
    AROS_LIBFUNC_INIT

    mgluBeginSurface(nurb);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluBeginTrim,
    AROS_LHA(GLUnurbs *, nurb, A0),
    struct Library *, GLUBase, 38, Glu)
{
    AROS_LIBFUNC_INIT

    mgluBeginTrim(nurb);

    AROS_LIBFUNC_EXIT
}

AROS_LH9(GLint, gluBuild1DMipmapLevels,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, internalFormat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(GLint, level, D5),
    AROS_LHA(GLint, base, D6),
    AROS_LHA(GLint, max, D7),
    AROS_LHA(const void *, data, A0),
    struct Library *, GLUBase, 39, Glu)
{
    AROS_LIBFUNC_INIT

    GLint _return = mgluBuild1DMipmapLevels(target, internalFormat, width, format, type, level, base, max, data);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH6(GLint, gluBuild1DMipmaps,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, internalFormat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLenum, format, D3),
    AROS_LHA(GLenum, type, D4),
    AROS_LHA(const void *, data, A0),
    struct Library *, GLUBase, 40, Glu)
{
    AROS_LIBFUNC_INIT

    GLint _return = mgluBuild1DMipmaps(target, internalFormat, width, format, type, data);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH10(GLint, gluBuild2DMipmapLevels,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, internalFormat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(GLint, level, D6),
    AROS_LHA(GLint, base, D7),
    AROS_LHA(GLint, max, A0),
    AROS_LHA(const void *, data, A1),
    struct Library *, GLUBase, 41, Glu)
{
    AROS_LIBFUNC_INIT

    GLint _return = mgluBuild2DMipmapLevels(target, internalFormat, width, height, format, type, level, base, max, data);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH7(GLint, gluBuild2DMipmaps,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, internalFormat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLenum, format, D4),
    AROS_LHA(GLenum, type, D5),
    AROS_LHA(const void *, data, A0),
    struct Library *, GLUBase, 42, Glu)
{
    AROS_LIBFUNC_INIT

    GLint _return = mgluBuild2DMipmaps(target, internalFormat, width, height, format, type, data);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH11(GLint, gluBuild3DMipmapLevels,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, internalFormat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLsizei, depth, D4),
    AROS_LHA(GLenum, format, D5),
    AROS_LHA(GLenum, type, D6),
    AROS_LHA(GLint, level, D7),
    AROS_LHA(GLint, base, A0),
    AROS_LHA(GLint, max, A1),
    AROS_LHA(const void *, data, A2),
    struct Library *, GLUBase, 43, Glu)
{
    AROS_LIBFUNC_INIT

    GLint _return = mgluBuild3DMipmapLevels(target, internalFormat, width, height, depth, format, type, level, base, max, data);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH8(GLint, gluBuild3DMipmaps,
    AROS_LHA(GLenum, target, D0),
    AROS_LHA(GLint, internalFormat, D1),
    AROS_LHA(GLsizei, width, D2),
    AROS_LHA(GLsizei, height, D3),
    AROS_LHA(GLsizei, depth, D4),
    AROS_LHA(GLenum, format, D5),
    AROS_LHA(GLenum, type, D6),
    AROS_LHA(const void *, data, A0),
    struct Library *, GLUBase, 44, Glu)
{
    AROS_LIBFUNC_INIT

    GLint _return = mgluBuild3DMipmaps(target, internalFormat, width, height, depth, format, type, data);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GLboolean, gluCheckExtension,
    AROS_LHA(const GLubyte *, extName, A0),
    AROS_LHA(const GLubyte *, extString, A1),
    struct Library *, GLUBase, 45, Glu)
{
    AROS_LIBFUNC_INIT

    GLboolean _return = mgluCheckExtension(extName, extString);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, gluCylinder,
    AROS_LHA(GLUquadric *, quad, A0),
    AROS_LHA(GLdouble, base, D0),
    AROS_LHA(GLdouble, top, D1),
    AROS_LHA(GLdouble, height, D2),
    AROS_LHA(GLint, slices, D3),
    AROS_LHA(GLint, stacks, D4),
    struct Library *, GLUBase, 46, Glu)
{
    AROS_LIBFUNC_INIT

    mgluCylinder(quad, base, top, height, slices, stacks);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluDeleteNurbsRenderer,
    AROS_LHA(GLUnurbs *, nurb, A0),
    struct Library *, GLUBase, 47, Glu)
{
    AROS_LIBFUNC_INIT

    mgluDeleteNurbsRenderer(nurb);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluDeleteQuadric,
    AROS_LHA(GLUquadric *, quad, A0),
    struct Library *, GLUBase, 48, Glu)
{
    AROS_LIBFUNC_INIT

    mgluDeleteQuadric(quad);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluDeleteTess,
    AROS_LHA(GLUtesselator *, tess, A0),
    struct Library *, GLUBase, 49, Glu)
{
    AROS_LIBFUNC_INIT

    mgluDeleteTess(tess);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, gluDisk,
    AROS_LHA(GLUquadric *, quad, A0),
    AROS_LHA(GLdouble, inner, D0),
    AROS_LHA(GLdouble, outer, D1),
    AROS_LHA(GLint, slices, D2),
    AROS_LHA(GLint, loops, D3),
    struct Library *, GLUBase, 50, Glu)
{
    AROS_LIBFUNC_INIT

    mgluDisk(quad, inner, outer, slices, loops);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluEndCurve,
    AROS_LHA(GLUnurbs *, nurb, A0),
    struct Library *, GLUBase, 51, Glu)
{
    AROS_LIBFUNC_INIT

    mgluEndCurve(nurb);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluEndPolygon,
    AROS_LHA(GLUtesselator *, tess, A0),
    struct Library *, GLUBase, 52, Glu)
{
    AROS_LIBFUNC_INIT

    mgluEndPolygon(tess);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluEndSurface,
    AROS_LHA(GLUnurbs *, nurb, A0),
    struct Library *, GLUBase, 53, Glu)
{
    AROS_LIBFUNC_INIT

    mgluEndSurface(nurb);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluEndTrim,
    AROS_LHA(GLUnurbs *, nurb, A0),
    struct Library *, GLUBase, 54, Glu)
{
    AROS_LIBFUNC_INIT

    mgluEndTrim(nurb);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(const GLubyte *, gluErrorString,
    AROS_LHA(GLenum, error, D0),
    struct Library *, GLUBase, 55, Glu)
{
    AROS_LIBFUNC_INIT

    const GLubyte * _return = mgluErrorString(error);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, gluGetNurbsProperty,
    AROS_LHA(GLUnurbs *, nurb, A0),
    AROS_LHA(GLenum, property, D0),
    AROS_LHA(GLfloat *, data, A1),
    struct Library *, GLUBase, 56, Glu)
{
    AROS_LIBFUNC_INIT

    mgluGetNurbsProperty(nurb, property, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(const GLubyte *, gluGetString,
    AROS_LHA(GLenum, name, D0),
    struct Library *, GLUBase, 57, Glu)
{
    AROS_LIBFUNC_INIT

    const GLubyte * _return = mgluGetString(name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, gluGetTessProperty,
    AROS_LHA(GLUtesselator *, tess, A0),
    AROS_LHA(GLenum, which, D0),
    AROS_LHA(GLdouble *, data, A1),
    struct Library *, GLUBase, 58, Glu)
{
    AROS_LIBFUNC_INIT

    mgluGetTessProperty(tess, which, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, gluLoadSamplingMatrices,
    AROS_LHA(GLUnurbs *, nurb, A0),
    AROS_LHA(const GLfloat *, model, A1),
    AROS_LHA(const GLfloat *, perspective, A2),
    AROS_LHA(const GLint *, view, A3),
    struct Library *, GLUBase, 59, Glu)
{
    AROS_LIBFUNC_INIT

    mgluLoadSamplingMatrices(nurb, model, perspective, view);

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, gluLookAt,
    AROS_LHA(GLdouble, eyeX, D0),
    AROS_LHA(GLdouble, eyeY, D1),
    AROS_LHA(GLdouble, eyeZ, D2),
    AROS_LHA(GLdouble, centerX, D3),
    AROS_LHA(GLdouble, centerY, D4),
    AROS_LHA(GLdouble, centerZ, D5),
    AROS_LHA(GLdouble, upX, D6),
    AROS_LHA(GLdouble, upY, D7),
    AROS_LHA(GLdouble, upZ, A0),
    struct Library *, GLUBase, 60, Glu)
{
    AROS_LIBFUNC_INIT

    mgluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLUnurbs*, gluNewNurbsRenderer,
    struct Library *, GLUBase, 61, Glu)
{
    AROS_LIBFUNC_INIT

    GLUnurbs* _return = mgluNewNurbsRenderer();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLUquadric*, gluNewQuadric,
    struct Library *, GLUBase, 62, Glu)
{
    AROS_LIBFUNC_INIT

    GLUquadric* _return = mgluNewQuadric();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(GLUtesselator*, gluNewTess,
    struct Library *, GLUBase, 63, Glu)
{
    AROS_LIBFUNC_INIT

    GLUtesselator* _return = mgluNewTess();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, gluNextContour,
    AROS_LHA(GLUtesselator *, tess, A0),
    AROS_LHA(GLenum, type, D0),
    struct Library *, GLUBase, 64, Glu)
{
    AROS_LIBFUNC_INIT

    mgluNextContour(tess, type);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, gluNurbsCallback,
    AROS_LHA(GLUnurbs *, nurb, A0),
    AROS_LHA(GLenum, which, D0),
    AROS_LHA(_GLUfuncptr, CallBackFunc, D1),
    struct Library *, GLUBase, 65, Glu)
{
    AROS_LIBFUNC_INIT

    mgluNurbsCallback(nurb, which, CallBackFunc);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, gluNurbsCallbackData,
    AROS_LHA(GLUnurbs *, nurb, A0),
    AROS_LHA(GLvoid *, userData, A1),
    struct Library *, GLUBase, 66, Glu)
{
    AROS_LIBFUNC_INIT

    mgluNurbsCallbackData(nurb, userData);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, gluNurbsCallbackDataEXT,
    AROS_LHA(GLUnurbs *, nurb, A0),
    AROS_LHA(GLvoid *, userData, A1),
    struct Library *, GLUBase, 67, Glu)
{
    AROS_LIBFUNC_INIT

    mgluNurbsCallbackDataEXT(nurb, userData);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, gluNurbsCurve,
    AROS_LHA(GLUnurbs *, nurb, A0),
    AROS_LHA(GLint, knotCount, D0),
    AROS_LHA(GLfloat *, knots, A1),
    AROS_LHA(GLint, stride, D1),
    AROS_LHA(GLfloat *, control, A2),
    AROS_LHA(GLint, order, D2),
    AROS_LHA(GLenum, type, D3),
    struct Library *, GLUBase, 68, Glu)
{
    AROS_LIBFUNC_INIT

    mgluNurbsCurve(nurb, knotCount, knots, stride, control, order, type);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, gluNurbsProperty,
    AROS_LHA(GLUnurbs *, nurb, A0),
    AROS_LHA(GLenum, property, D0),
    AROS_LHA(GLfloat, value, D1),
    struct Library *, GLUBase, 69, Glu)
{
    AROS_LIBFUNC_INIT

    mgluNurbsProperty(nurb, property, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH11(void, gluNurbsSurface,
    AROS_LHA(GLUnurbs *, nurb, A0),
    AROS_LHA(GLint, sKnotCount, D0),
    AROS_LHA(GLfloat *, sKnots, A1),
    AROS_LHA(GLint, tKnotCount, D1),
    AROS_LHA(GLfloat *, tKnots, A2),
    AROS_LHA(GLint, sStride, D2),
    AROS_LHA(GLint, tStride, D3),
    AROS_LHA(GLfloat *, control, A3),
    AROS_LHA(GLint, sOrder, D4),
    AROS_LHA(GLint, tOrder, D5),
    AROS_LHA(GLenum, type, D6),
    struct Library *, GLUBase, 70, Glu)
{
    AROS_LIBFUNC_INIT

    mgluNurbsSurface(nurb, sKnotCount, sKnots, tKnotCount, tKnots, sStride, tStride, control, sOrder, tOrder, type);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, gluOrtho2D,
    AROS_LHA(GLdouble, left, D0),
    AROS_LHA(GLdouble, right, D1),
    AROS_LHA(GLdouble, bottom, D2),
    AROS_LHA(GLdouble, top, D3),
    struct Library *, GLUBase, 71, Glu)
{
    AROS_LIBFUNC_INIT

    mgluOrtho2D(left, right, bottom, top);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, gluPartialDisk,
    AROS_LHA(GLUquadric *, quad, A0),
    AROS_LHA(GLdouble, inner, D0),
    AROS_LHA(GLdouble, outer, D1),
    AROS_LHA(GLint, slices, D2),
    AROS_LHA(GLint, loops, D3),
    AROS_LHA(GLdouble, start, D4),
    AROS_LHA(GLdouble, sweep, D5),
    struct Library *, GLUBase, 72, Glu)
{
    AROS_LIBFUNC_INIT

    mgluPartialDisk(quad, inner, outer, slices, loops, start, sweep);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, gluPerspective,
    AROS_LHA(GLdouble, fovy, D0),
    AROS_LHA(GLdouble, aspect, D1),
    AROS_LHA(GLdouble, zNear, D2),
    AROS_LHA(GLdouble, zFar, D3),
    struct Library *, GLUBase, 73, Glu)
{
    AROS_LIBFUNC_INIT

    mgluPerspective(fovy, aspect, zNear, zFar);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, gluPickMatrix,
    AROS_LHA(GLdouble, x, D0),
    AROS_LHA(GLdouble, y, D1),
    AROS_LHA(GLdouble, delX, D2),
    AROS_LHA(GLdouble, delY, D3),
    AROS_LHA(GLint *, viewport, A0),
    struct Library *, GLUBase, 74, Glu)
{
    AROS_LIBFUNC_INIT

    mgluPickMatrix(x, y, delX, delY, viewport);

    AROS_LIBFUNC_EXIT
}

AROS_LH9(GLint, gluProject,
    AROS_LHA(GLdouble, objX, D0),
    AROS_LHA(GLdouble, objY, D1),
    AROS_LHA(GLdouble, objZ, D2),
    AROS_LHA(const GLdouble *, model, A0),
    AROS_LHA(const GLdouble *, proj, A1),
    AROS_LHA(const GLint *, view, A2),
    AROS_LHA(GLdouble *, winX, A3),
    AROS_LHA(GLdouble *, winY, A4),
    AROS_LHA(GLdouble *, winZ, A5),
    struct Library *, GLUBase, 75, Glu)
{
    AROS_LIBFUNC_INIT

    GLint _return = mgluProject(objX, objY, objZ, model, proj, view, winX, winY, winZ);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, gluPwlCurve,
    AROS_LHA(GLUnurbs *, nurb, A0),
    AROS_LHA(GLint, count, D0),
    AROS_LHA(GLfloat *, data, A1),
    AROS_LHA(GLint, stride, D1),
    AROS_LHA(GLenum, type, D2),
    struct Library *, GLUBase, 76, Glu)
{
    AROS_LIBFUNC_INIT

    mgluPwlCurve(nurb, count, data, stride, type);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, gluQuadricCallback,
    AROS_LHA(GLUquadric *, quad, A0),
    AROS_LHA(GLenum, which, D0),
    AROS_LHA(_GLUfuncptr, CallBackFunc, D1),
    struct Library *, GLUBase, 77, Glu)
{
    AROS_LIBFUNC_INIT

    mgluQuadricCallback(quad, which, CallBackFunc);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, gluQuadricDrawStyle,
    AROS_LHA(GLUquadric *, quad, A0),
    AROS_LHA(GLenum, draw, D0),
    struct Library *, GLUBase, 78, Glu)
{
    AROS_LIBFUNC_INIT

    mgluQuadricDrawStyle(quad, draw);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, gluQuadricNormals,
    AROS_LHA(GLUquadric *, quad, A0),
    AROS_LHA(GLenum, normal, D0),
    struct Library *, GLUBase, 79, Glu)
{
    AROS_LIBFUNC_INIT

    mgluQuadricNormals(quad, normal);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, gluQuadricOrientation,
    AROS_LHA(GLUquadric *, quad, A0),
    AROS_LHA(GLenum, orientation, D0),
    struct Library *, GLUBase, 80, Glu)
{
    AROS_LIBFUNC_INIT

    mgluQuadricOrientation(quad, orientation);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, gluQuadricTexture,
    AROS_LHA(GLUquadric *, quad, A0),
    AROS_LHA(GLboolean, texture, D0),
    struct Library *, GLUBase, 81, Glu)
{
    AROS_LIBFUNC_INIT

    mgluQuadricTexture(quad, texture);

    AROS_LIBFUNC_EXIT
}

AROS_LH9(GLint, gluScaleImage,
    AROS_LHA(GLenum, format, D0),
    AROS_LHA(GLsizei, wIn, D1),
    AROS_LHA(GLsizei, hIn, D2),
    AROS_LHA(GLenum, typeIn, D3),
    AROS_LHA(const void *, dataIn, A0),
    AROS_LHA(GLsizei, wOut, D4),
    AROS_LHA(GLsizei, hOut, D5),
    AROS_LHA(GLenum, typeOut, D6),
    AROS_LHA(GLvoid *, dataOut, A1),
    struct Library *, GLUBase, 82, Glu)
{
    AROS_LIBFUNC_INIT

    GLint _return = mgluScaleImage(format, wIn, hIn, typeIn, dataIn, wOut, hOut, typeOut, dataOut);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, gluSphere,
    AROS_LHA(GLUquadric *, quad, A0),
    AROS_LHA(GLdouble, radius, D0),
    AROS_LHA(GLint, slices, D1),
    AROS_LHA(GLint, stacks, D2),
    struct Library *, GLUBase, 83, Glu)
{
    AROS_LIBFUNC_INIT

    mgluSphere(quad, radius, slices, stacks);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluTessBeginContour,
    AROS_LHA(GLUtesselator *, tess, A0),
    struct Library *, GLUBase, 84, Glu)
{
    AROS_LIBFUNC_INIT

    mgluTessBeginContour(tess);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, gluTessBeginPolygon,
    AROS_LHA(GLUtesselator *, tess, A0),
    AROS_LHA(GLvoid *, data, A1),
    struct Library *, GLUBase, 85, Glu)
{
    AROS_LIBFUNC_INIT

    mgluTessBeginPolygon(tess, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, gluTessCallback,
    AROS_LHA(GLUtesselator *, tess, A0),
    AROS_LHA(GLenum, which, D0),
    AROS_LHA(_GLUfuncptr, CallBackFunc, D1),
    struct Library *, GLUBase, 86, Glu)
{
    AROS_LIBFUNC_INIT

    mgluTessCallback(tess, which, CallBackFunc);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluTessEndContour,
    AROS_LHA(GLUtesselator *, tess, A0),
    struct Library *, GLUBase, 87, Glu)
{
    AROS_LIBFUNC_INIT

    mgluTessEndContour(tess);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, gluTessEndPolygon,
    AROS_LHA(GLUtesselator *, tess, A0),
    struct Library *, GLUBase, 88, Glu)
{
    AROS_LIBFUNC_INIT

    mgluTessEndPolygon(tess);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, gluTessNormal,
    AROS_LHA(GLUtesselator *, tess, A0),
    AROS_LHA(GLdouble, valueX, D0),
    AROS_LHA(GLdouble, valueY, D1),
    AROS_LHA(GLdouble, valueZ, D2),
    struct Library *, GLUBase, 89, Glu)
{
    AROS_LIBFUNC_INIT

    mgluTessNormal(tess, valueX, valueY, valueZ);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, gluTessProperty,
    AROS_LHA(GLUtesselator *, tess, A0),
    AROS_LHA(GLenum, which, D0),
    AROS_LHA(GLdouble, data, D1),
    struct Library *, GLUBase, 90, Glu)
{
    AROS_LIBFUNC_INIT

    mgluTessProperty(tess, which, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, gluTessVertex,
    AROS_LHA(GLUtesselator *, tess, A0),
    AROS_LHA(GLdouble *, location, A1),
    AROS_LHA(GLvoid *, data, A2),
    struct Library *, GLUBase, 91, Glu)
{
    AROS_LIBFUNC_INIT

    mgluTessVertex(tess, location, data);

    AROS_LIBFUNC_EXIT
}

AROS_LH9(GLint, gluUnProject,
    AROS_LHA(GLdouble, winX, D0),
    AROS_LHA(GLdouble, winY, D1),
    AROS_LHA(GLdouble, winZ, D2),
    AROS_LHA(const GLdouble *, model, A0),
    AROS_LHA(const GLdouble *, proj, A1),
    AROS_LHA(const GLint *, view, A2),
    AROS_LHA(GLdouble *, objX, A3),
    AROS_LHA(GLdouble *, objY, A4),
    AROS_LHA(GLdouble *, objZ, A5),
    struct Library *, GLUBase, 92, Glu)
{
    AROS_LIBFUNC_INIT

    GLint _return = mgluUnProject(winX, winY, winZ, model, proj, view, objX, objY, objZ);

    return _return;

    AROS_LIBFUNC_EXIT
}

