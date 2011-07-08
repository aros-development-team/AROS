#include "VG/openvg.h"
#include "VG/vgu.h"
#include "vgapim.h"
#include "vega_intern.h"

AROS_LH0(VGErrorCode, vgGetError,
    struct Library *, VegaBase, 35, Vega)
{
    AROS_LIBFUNC_INIT

    VGErrorCode _return = mvgGetError();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, vgFlush,
    struct Library *, VegaBase, 36, Vega)
{
    AROS_LIBFUNC_INIT

    mvgFlush();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, vgFinish,
    struct Library *, VegaBase, 37, Vega)
{
    AROS_LIBFUNC_INIT

    mvgFinish();

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgSetf,
    AROS_LHA(VGParamType, type, D0),
    AROS_LHA(VGfloat, value, D1),
    struct Library *, VegaBase, 38, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetf(type, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgSeti,
    AROS_LHA(VGParamType, type, D0),
    AROS_LHA(VGint, value, D1),
    struct Library *, VegaBase, 39, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSeti(type, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, vgSetfv,
    AROS_LHA(VGParamType, type, D0),
    AROS_LHA(VGint, count, D1),
    AROS_LHA(const VGfloat *, values, A0),
    struct Library *, VegaBase, 40, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetfv(type, count, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, vgSetiv,
    AROS_LHA(VGParamType, type, D0),
    AROS_LHA(VGint, count, D1),
    AROS_LHA(const VGint *, values, A0),
    struct Library *, VegaBase, 41, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetiv(type, count, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VGfloat, vgGetf,
    AROS_LHA(VGParamType, type, D0),
    struct Library *, VegaBase, 42, Vega)
{
    AROS_LIBFUNC_INIT

    VGfloat _return = mvgGetf(type);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VGint, vgGeti,
    AROS_LHA(VGParamType, type, D0),
    struct Library *, VegaBase, 43, Vega)
{
    AROS_LIBFUNC_INIT

    VGint _return = mvgGeti(type);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VGint, vgGetVectorSize,
    AROS_LHA(VGParamType, type, D0),
    struct Library *, VegaBase, 44, Vega)
{
    AROS_LIBFUNC_INIT

    VGint _return = mvgGetVectorSize(type);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, vgGetfv,
    AROS_LHA(VGParamType, type, D0),
    AROS_LHA(VGint, count, D1),
    AROS_LHA(VGfloat *, values, A0),
    struct Library *, VegaBase, 45, Vega)
{
    AROS_LIBFUNC_INIT

    mvgGetfv(type, count, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, vgGetiv,
    AROS_LHA(VGParamType, type, D0),
    AROS_LHA(VGint, count, D1),
    AROS_LHA(VGint *, values, A0),
    struct Library *, VegaBase, 46, Vega)
{
    AROS_LIBFUNC_INIT

    mvgGetiv(type, count, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, vgSetParameterf,
    AROS_LHA(VGHandle, object, D0),
    AROS_LHA(VGint, paramType, D1),
    AROS_LHA(VGfloat, value, D2),
    struct Library *, VegaBase, 47, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetParameterf(object, paramType, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, vgSetParameteri,
    AROS_LHA(VGHandle, object, D0),
    AROS_LHA(VGint, paramType, D1),
    AROS_LHA(VGint, value, D2),
    struct Library *, VegaBase, 48, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetParameteri(object, paramType, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, vgSetParameterfv,
    AROS_LHA(VGHandle, object, D0),
    AROS_LHA(VGint, paramType, D1),
    AROS_LHA(VGint, count, D2),
    AROS_LHA(const VGfloat *, values, A0),
    struct Library *, VegaBase, 49, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetParameterfv(object, paramType, count, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, vgSetParameteriv,
    AROS_LHA(VGHandle, object, D0),
    AROS_LHA(VGint, paramType, D1),
    AROS_LHA(VGint, count, D2),
    AROS_LHA(const VGint *, values, A0),
    struct Library *, VegaBase, 50, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetParameteriv(object, paramType, count, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(VGfloat, vgGetParameterf,
    AROS_LHA(VGHandle, object, D0),
    AROS_LHA(VGint, paramType, D1),
    struct Library *, VegaBase, 51, Vega)
{
    AROS_LIBFUNC_INIT

    VGfloat _return = mvgGetParameterf(object, paramType);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(VGint, vgGetParameteri,
    AROS_LHA(VGHandle, object, D0),
    AROS_LHA(VGint, paramType, D1),
    struct Library *, VegaBase, 52, Vega)
{
    AROS_LIBFUNC_INIT

    VGint _return = mvgGetParameteri(object, paramType);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(VGint, vgGetParameterVectorSize,
    AROS_LHA(VGHandle, object, D0),
    AROS_LHA(VGint, paramType, D1),
    struct Library *, VegaBase, 53, Vega)
{
    AROS_LIBFUNC_INIT

    VGint _return = mvgGetParameterVectorSize(object, paramType);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, vgGetParameterfv,
    AROS_LHA(VGHandle, object, D0),
    AROS_LHA(VGint, paramType, D1),
    AROS_LHA(VGint, count, D2),
    AROS_LHA(VGfloat *, values, A0),
    struct Library *, VegaBase, 54, Vega)
{
    AROS_LIBFUNC_INIT

    mvgGetParameterfv(object, paramType, count, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, vgGetParameteriv,
    AROS_LHA(VGHandle, object, D0),
    AROS_LHA(VGint, paramType, D1),
    AROS_LHA(VGint, count, D2),
    AROS_LHA(VGint *, values, A0),
    struct Library *, VegaBase, 55, Vega)
{
    AROS_LIBFUNC_INIT

    mvgGetParameteriv(object, paramType, count, values);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, vgLoadIdentity,
    struct Library *, VegaBase, 56, Vega)
{
    AROS_LIBFUNC_INIT

    mvgLoadIdentity();

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgLoadMatrix,
    AROS_LHA(const VGfloat *, m, A0),
    struct Library *, VegaBase, 57, Vega)
{
    AROS_LIBFUNC_INIT

    mvgLoadMatrix(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgGetMatrix,
    AROS_LHA(VGfloat *, m, A0),
    struct Library *, VegaBase, 58, Vega)
{
    AROS_LIBFUNC_INIT

    mvgGetMatrix(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgMultMatrix,
    AROS_LHA(const VGfloat *, m, A0),
    struct Library *, VegaBase, 59, Vega)
{
    AROS_LIBFUNC_INIT

    mvgMultMatrix(m);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgTranslate,
    AROS_LHA(VGfloat, tx, D0),
    AROS_LHA(VGfloat, ty, D1),
    struct Library *, VegaBase, 60, Vega)
{
    AROS_LIBFUNC_INIT

    mvgTranslate(tx, ty);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgScale,
    AROS_LHA(VGfloat, sx, D0),
    AROS_LHA(VGfloat, sy, D1),
    struct Library *, VegaBase, 61, Vega)
{
    AROS_LIBFUNC_INIT

    mvgScale(sx, sy);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgShear,
    AROS_LHA(VGfloat, shx, D0),
    AROS_LHA(VGfloat, shy, D1),
    struct Library *, VegaBase, 62, Vega)
{
    AROS_LIBFUNC_INIT

    mvgShear(shx, shy);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgRotate,
    AROS_LHA(VGfloat, angle, D0),
    struct Library *, VegaBase, 63, Vega)
{
    AROS_LIBFUNC_INIT

    mvgRotate(angle);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, vgMask,
    AROS_LHA(VGHandle, mask, D0),
    AROS_LHA(VGMaskOperation, operation, D1),
    AROS_LHA(VGint, x, D2),
    AROS_LHA(VGint, y, D3),
    AROS_LHA(VGint, width, D4),
    AROS_LHA(VGint, height, D5),
    struct Library *, VegaBase, 64, Vega)
{
    AROS_LIBFUNC_INIT

    mvgMask(mask, operation, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, vgRenderToMask,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGbitfield, paintModes, D1),
    AROS_LHA(VGMaskOperation, operation, D2),
    struct Library *, VegaBase, 65, Vega)
{
    AROS_LIBFUNC_INIT

    mvgRenderToMask(path, paintModes, operation);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(VGMaskLayer, vgCreateMaskLayer,
    AROS_LHA(VGint, width, D0),
    AROS_LHA(VGint, height, D1),
    struct Library *, VegaBase, 66, Vega)
{
    AROS_LIBFUNC_INIT

    VGMaskLayer _return = mvgCreateMaskLayer(width, height);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgDestroyMaskLayer,
    AROS_LHA(VGMaskLayer, maskLayer, D0),
    struct Library *, VegaBase, 67, Vega)
{
    AROS_LIBFUNC_INIT

    mvgDestroyMaskLayer(maskLayer);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, vgFillMaskLayer,
    AROS_LHA(VGMaskLayer, maskLayer, D0),
    AROS_LHA(VGint, x, D1),
    AROS_LHA(VGint, y, D2),
    AROS_LHA(VGint, width, D3),
    AROS_LHA(VGint, height, D4),
    AROS_LHA(VGfloat, value, D5),
    struct Library *, VegaBase, 68, Vega)
{
    AROS_LIBFUNC_INIT

    mvgFillMaskLayer(maskLayer, x, y, width, height, value);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, vgCopyMask,
    AROS_LHA(VGMaskLayer, maskLayer, D0),
    AROS_LHA(VGint, dx, D1),
    AROS_LHA(VGint, dy, D2),
    AROS_LHA(VGint, sx, D3),
    AROS_LHA(VGint, sy, D4),
    AROS_LHA(VGint, width, D5),
    AROS_LHA(VGint, height, D6),
    struct Library *, VegaBase, 69, Vega)
{
    AROS_LIBFUNC_INIT

    mvgCopyMask(maskLayer, dx, dy, sx, sy, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, vgClear,
    AROS_LHA(VGint, x, D0),
    AROS_LHA(VGint, y, D1),
    AROS_LHA(VGint, width, D2),
    AROS_LHA(VGint, height, D3),
    struct Library *, VegaBase, 70, Vega)
{
    AROS_LIBFUNC_INIT

    mvgClear(x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(VGPath, vgCreatePath,
    AROS_LHA(VGint, pathFormat, D0),
    AROS_LHA(VGPathDatatype, datatype, D1),
    AROS_LHA(VGfloat, scale, D2),
    AROS_LHA(VGfloat, bias, D3),
    AROS_LHA(VGint, segmentCapacityHint, D4),
    AROS_LHA(VGint, coordCapacityHint, D5),
    AROS_LHA(VGbitfield, capabilities, D6),
    struct Library *, VegaBase, 71, Vega)
{
    AROS_LIBFUNC_INIT

    VGPath _return = mvgCreatePath(pathFormat, datatype, scale, bias, segmentCapacityHint, coordCapacityHint, capabilities);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgClearPath,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGbitfield, capabilities, D1),
    struct Library *, VegaBase, 72, Vega)
{
    AROS_LIBFUNC_INIT

    mvgClearPath(path, capabilities);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgDestroyPath,
    AROS_LHA(VGPath, path, D0),
    struct Library *, VegaBase, 73, Vega)
{
    AROS_LIBFUNC_INIT

    mvgDestroyPath(path);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgRemovePathCapabilities,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGbitfield, capabilities, D1),
    struct Library *, VegaBase, 74, Vega)
{
    AROS_LIBFUNC_INIT

    mvgRemovePathCapabilities(path, capabilities);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VGbitfield, vgGetPathCapabilities,
    AROS_LHA(VGPath, path, D0),
    struct Library *, VegaBase, 75, Vega)
{
    AROS_LIBFUNC_INIT

    VGbitfield _return = mvgGetPathCapabilities(path);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgAppendPath,
    AROS_LHA(VGPath, dstPath, D0),
    AROS_LHA(VGPath, srcPath, D1),
    struct Library *, VegaBase, 76, Vega)
{
    AROS_LIBFUNC_INIT

    mvgAppendPath(dstPath, srcPath);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, vgAppendPathData,
    AROS_LHA(VGPath, dstPath, D0),
    AROS_LHA(VGint, numSegments, D1),
    AROS_LHA(const VGubyte *, pathSegments, A0),
    AROS_LHA(const void *, pathData, A1),
    struct Library *, VegaBase, 77, Vega)
{
    AROS_LIBFUNC_INIT

    mvgAppendPathData(dstPath, numSegments, pathSegments, pathData);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, vgModifyPathCoords,
    AROS_LHA(VGPath, dstPath, D0),
    AROS_LHA(VGint, startIndex, D1),
    AROS_LHA(VGint, numSegments, D2),
    AROS_LHA(const void *, pathData, A0),
    struct Library *, VegaBase, 78, Vega)
{
    AROS_LIBFUNC_INIT

    mvgModifyPathCoords(dstPath, startIndex, numSegments, pathData);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgTransformPath,
    AROS_LHA(VGPath, dstPath, D0),
    AROS_LHA(VGPath, srcPath, D1),
    struct Library *, VegaBase, 79, Vega)
{
    AROS_LIBFUNC_INIT

    mvgTransformPath(dstPath, srcPath);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(VGboolean, vgInterpolatePath,
    AROS_LHA(VGPath, dstPath, D0),
    AROS_LHA(VGPath, startPath, D1),
    AROS_LHA(VGPath, endPath, D2),
    AROS_LHA(VGfloat, amount, D3),
    struct Library *, VegaBase, 80, Vega)
{
    AROS_LIBFUNC_INIT

    VGboolean _return = mvgInterpolatePath(dstPath, startPath, endPath, amount);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(VGfloat, vgPathLength,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGint, startSegment, D1),
    AROS_LHA(VGint, numSegments, D2),
    struct Library *, VegaBase, 81, Vega)
{
    AROS_LIBFUNC_INIT

    VGfloat _return = mvgPathLength(path, startSegment, numSegments);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, vgPointAlongPath,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGint, startSegment, D1),
    AROS_LHA(VGint, numSegments, D2),
    AROS_LHA(VGfloat, distance, D3),
    AROS_LHA(VGfloat *, x, A0),
    AROS_LHA(VGfloat *, y, A1),
    AROS_LHA(VGfloat *, tangentX, A2),
    AROS_LHA(VGfloat *, tangentY, A3),
    struct Library *, VegaBase, 82, Vega)
{
    AROS_LIBFUNC_INIT

    mvgPointAlongPath(path, startSegment, numSegments, distance, x, y, tangentX, tangentY);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, vgPathBounds,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGfloat *, minX, A0),
    AROS_LHA(VGfloat *, minY, A1),
    AROS_LHA(VGfloat *, width, A2),
    AROS_LHA(VGfloat *, height, A3),
    struct Library *, VegaBase, 83, Vega)
{
    AROS_LIBFUNC_INIT

    mvgPathBounds(path, minX, minY, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, vgPathTransformedBounds,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGfloat *, minX, A0),
    AROS_LHA(VGfloat *, minY, A1),
    AROS_LHA(VGfloat *, width, A2),
    AROS_LHA(VGfloat *, height, A3),
    struct Library *, VegaBase, 84, Vega)
{
    AROS_LIBFUNC_INIT

    mvgPathTransformedBounds(path, minX, minY, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgDrawPath,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGbitfield, paintModes, D1),
    struct Library *, VegaBase, 85, Vega)
{
    AROS_LIBFUNC_INIT

    mvgDrawPath(path, paintModes);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(VGPaint, vgCreatePaint,
    struct Library *, VegaBase, 86, Vega)
{
    AROS_LIBFUNC_INIT

    VGPaint _return = mvgCreatePaint();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgDestroyPaint,
    AROS_LHA(VGPaint, paint, D0),
    struct Library *, VegaBase, 87, Vega)
{
    AROS_LIBFUNC_INIT

    mvgDestroyPaint(paint);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgSetPaint,
    AROS_LHA(VGPaint, paint, D0),
    AROS_LHA(VGbitfield, paintModes, D1),
    struct Library *, VegaBase, 88, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetPaint(paint, paintModes);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VGPaint, vgGetPaint,
    AROS_LHA(VGPaintMode, paintMode, D0),
    struct Library *, VegaBase, 89, Vega)
{
    AROS_LIBFUNC_INIT

    VGPaint _return = mvgGetPaint(paintMode);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgSetColor,
    AROS_LHA(VGPaint, paint, D0),
    AROS_LHA(VGuint, rgba, D1),
    struct Library *, VegaBase, 90, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetColor(paint, rgba);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VGuint, vgGetColor,
    AROS_LHA(VGPaint, paint, D0),
    struct Library *, VegaBase, 91, Vega)
{
    AROS_LIBFUNC_INIT

    VGuint _return = mvgGetColor(paint);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgPaintPattern,
    AROS_LHA(VGPaint, paint, D0),
    AROS_LHA(VGImage, pattern, D1),
    struct Library *, VegaBase, 92, Vega)
{
    AROS_LIBFUNC_INIT

    mvgPaintPattern(paint, pattern);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(VGImage, vgCreateImage,
    AROS_LHA(VGImageFormat, format, D0),
    AROS_LHA(VGint, width, D1),
    AROS_LHA(VGint, height, D2),
    AROS_LHA(VGbitfield, allowedQuality, D3),
    struct Library *, VegaBase, 93, Vega)
{
    AROS_LIBFUNC_INIT

    VGImage _return = mvgCreateImage(format, width, height, allowedQuality);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgDestroyImage,
    AROS_LHA(VGImage, image, D0),
    struct Library *, VegaBase, 94, Vega)
{
    AROS_LIBFUNC_INIT

    mvgDestroyImage(image);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, vgClearImage,
    AROS_LHA(VGImage, image, D0),
    AROS_LHA(VGint, x, D1),
    AROS_LHA(VGint, y, D2),
    AROS_LHA(VGint, width, D3),
    AROS_LHA(VGint, height, D4),
    struct Library *, VegaBase, 95, Vega)
{
    AROS_LIBFUNC_INIT

    mvgClearImage(image, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, vgImageSubData,
    AROS_LHA(VGImage, image, D0),
    AROS_LHA(const void *, data, A0),
    AROS_LHA(VGint, dataStride, D1),
    AROS_LHA(VGImageFormat, dataFormat, D2),
    AROS_LHA(VGint, x, D3),
    AROS_LHA(VGint, y, D4),
    AROS_LHA(VGint, width, D5),
    AROS_LHA(VGint, height, D6),
    struct Library *, VegaBase, 96, Vega)
{
    AROS_LIBFUNC_INIT

    mvgImageSubData(image, data, dataStride, dataFormat, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, vgGetImageSubData,
    AROS_LHA(VGImage, image, D0),
    AROS_LHA(void *, data, A0),
    AROS_LHA(VGint, dataStride, D1),
    AROS_LHA(VGImageFormat, dataFormat, D2),
    AROS_LHA(VGint, x, D3),
    AROS_LHA(VGint, y, D4),
    AROS_LHA(VGint, width, D5),
    AROS_LHA(VGint, height, D6),
    struct Library *, VegaBase, 97, Vega)
{
    AROS_LIBFUNC_INIT

    mvgGetImageSubData(image, data, dataStride, dataFormat, x, y, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(VGImage, vgChildImage,
    AROS_LHA(VGImage, parent, D0),
    AROS_LHA(VGint, x, D1),
    AROS_LHA(VGint, y, D2),
    AROS_LHA(VGint, width, D3),
    AROS_LHA(VGint, height, D4),
    struct Library *, VegaBase, 98, Vega)
{
    AROS_LIBFUNC_INIT

    VGImage _return = mvgChildImage(parent, x, y, width, height);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VGImage, vgGetParent,
    AROS_LHA(VGImage, image, D0),
    struct Library *, VegaBase, 99, Vega)
{
    AROS_LIBFUNC_INIT

    VGImage _return = mvgGetParent(image);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, vgCopyImage,
    AROS_LHA(VGImage, dst, D0),
    AROS_LHA(VGint, dx, D1),
    AROS_LHA(VGint, dy, D2),
    AROS_LHA(VGImage, src, D3),
    AROS_LHA(VGint, sx, D4),
    AROS_LHA(VGint, sy, D5),
    AROS_LHA(VGint, width, D6),
    AROS_LHA(VGint, height, D7),
    AROS_LHA(VGboolean, dither, A0),
    struct Library *, VegaBase, 100, Vega)
{
    AROS_LIBFUNC_INIT

    mvgCopyImage(dst, dx, dy, src, sx, sy, width, height, dither);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgDrawImage,
    AROS_LHA(VGImage, image, D0),
    struct Library *, VegaBase, 101, Vega)
{
    AROS_LIBFUNC_INIT

    mvgDrawImage(image);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, vgSetPixels,
    AROS_LHA(VGint, dx, D0),
    AROS_LHA(VGint, dy, D1),
    AROS_LHA(VGImage, src, D2),
    AROS_LHA(VGint, sx, D3),
    AROS_LHA(VGint, sy, D4),
    AROS_LHA(VGint, width, D5),
    AROS_LHA(VGint, height, D6),
    struct Library *, VegaBase, 102, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetPixels(dx, dy, src, sx, sy, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, vgWritePixels,
    AROS_LHA(const void *, data, A0),
    AROS_LHA(VGint, dataStride, D0),
    AROS_LHA(VGImageFormat, dataFormat, D1),
    AROS_LHA(VGint, dx, D2),
    AROS_LHA(VGint, dy, D3),
    AROS_LHA(VGint, width, D4),
    AROS_LHA(VGint, height, D5),
    struct Library *, VegaBase, 103, Vega)
{
    AROS_LIBFUNC_INIT

    mvgWritePixels(data, dataStride, dataFormat, dx, dy, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, vgGetPixels,
    AROS_LHA(VGImage, dst, D0),
    AROS_LHA(VGint, dx, D1),
    AROS_LHA(VGint, dy, D2),
    AROS_LHA(VGint, sx, D3),
    AROS_LHA(VGint, sy, D4),
    AROS_LHA(VGint, width, D5),
    AROS_LHA(VGint, height, D6),
    struct Library *, VegaBase, 104, Vega)
{
    AROS_LIBFUNC_INIT

    mvgGetPixels(dst, dx, dy, sx, sy, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, vgReadPixels,
    AROS_LHA(void *, data, A0),
    AROS_LHA(VGint, dataStride, D0),
    AROS_LHA(VGImageFormat, dataFormat, D1),
    AROS_LHA(VGint, sx, D2),
    AROS_LHA(VGint, sy, D3),
    AROS_LHA(VGint, width, D4),
    AROS_LHA(VGint, height, D5),
    struct Library *, VegaBase, 105, Vega)
{
    AROS_LIBFUNC_INIT

    mvgReadPixels(data, dataStride, dataFormat, sx, sy, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, vgCopyPixels,
    AROS_LHA(VGint, dx, D0),
    AROS_LHA(VGint, dy, D1),
    AROS_LHA(VGint, sx, D2),
    AROS_LHA(VGint, sy, D3),
    AROS_LHA(VGint, width, D4),
    AROS_LHA(VGint, height, D5),
    struct Library *, VegaBase, 106, Vega)
{
    AROS_LIBFUNC_INIT

    mvgCopyPixels(dx, dy, sx, sy, width, height);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VGFont, vgCreateFont,
    AROS_LHA(VGint, glyphCapacityHint, D0),
    struct Library *, VegaBase, 107, Vega)
{
    AROS_LIBFUNC_INIT

    VGFont _return = mvgCreateFont(glyphCapacityHint);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, vgDestroyFont,
    AROS_LHA(VGFont, font, D0),
    struct Library *, VegaBase, 108, Vega)
{
    AROS_LIBFUNC_INIT

    mvgDestroyFont(font);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, vgSetGlyphToPath,
    AROS_LHA(VGFont, font, D0),
    AROS_LHA(VGuint, glyphIndex, D1),
    AROS_LHA(VGPath, path, D2),
    AROS_LHA(VGboolean, isHinted, D3),
    AROS_LHA(const VGfloat *, glyphOrigin, A0),
    AROS_LHA(const VGfloat *, escapement, A1),
    struct Library *, VegaBase, 109, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetGlyphToPath(font, glyphIndex, path, isHinted, glyphOrigin, escapement);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, vgSetGlyphToImage,
    AROS_LHA(VGFont, font, D0),
    AROS_LHA(VGuint, glyphIndex, D1),
    AROS_LHA(VGImage, image, D2),
    AROS_LHA(const VGfloat *, glyphOrigin, A0),
    AROS_LHA(const VGfloat *, escapement, A1),
    struct Library *, VegaBase, 110, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSetGlyphToImage(font, glyphIndex, image, glyphOrigin, escapement);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, vgClearGlyph,
    AROS_LHA(VGFont, font, D0),
    AROS_LHA(VGuint, glyphIndex, D1),
    struct Library *, VegaBase, 111, Vega)
{
    AROS_LIBFUNC_INIT

    mvgClearGlyph(font, glyphIndex);

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, vgDrawGlyph,
    AROS_LHA(VGFont, font, D0),
    AROS_LHA(VGuint, glyphIndex, D1),
    AROS_LHA(VGbitfield, paintModes, D2),
    AROS_LHA(VGboolean, allowAutoHinting, D3),
    struct Library *, VegaBase, 112, Vega)
{
    AROS_LIBFUNC_INIT

    mvgDrawGlyph(font, glyphIndex, paintModes, allowAutoHinting);

    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, vgDrawGlyphs,
    AROS_LHA(VGFont, font, D0),
    AROS_LHA(VGint, glyphCount, D1),
    AROS_LHA(const VGuint *, glyphIndices, A0),
    AROS_LHA(const VGfloat *, adjustments_x, A1),
    AROS_LHA(const VGfloat *, adjustments_y, A2),
    AROS_LHA(VGbitfield, paintModes, D2),
    AROS_LHA(VGboolean, allowAutoHinting, D3),
    struct Library *, VegaBase, 113, Vega)
{
    AROS_LIBFUNC_INIT

    mvgDrawGlyphs(font, glyphCount, glyphIndices, adjustments_x, adjustments_y, paintModes, allowAutoHinting);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, vgColorMatrix,
    AROS_LHA(VGImage, dst, D0),
    AROS_LHA(VGImage, src, D1),
    AROS_LHA(const VGfloat *, matrix, A0),
    struct Library *, VegaBase, 114, Vega)
{
    AROS_LIBFUNC_INIT

    mvgColorMatrix(dst, src, matrix);

    AROS_LIBFUNC_EXIT
}

AROS_LH10(void, vgConvolve,
    AROS_LHA(VGImage, dst, D0),
    AROS_LHA(VGImage, src, D1),
    AROS_LHA(VGint, kernelWidth, D2),
    AROS_LHA(VGint, kernelHeight, D3),
    AROS_LHA(VGint, shiftX, D4),
    AROS_LHA(VGint, shiftY, D5),
    AROS_LHA(const VGshort *, kernel, A0),
    AROS_LHA(VGfloat, scale, D6),
    AROS_LHA(VGfloat, bias, D7),
    AROS_LHA(VGTilingMode, tilingMode, A1),
    struct Library *, VegaBase, 115, Vega)
{
    AROS_LIBFUNC_INIT

    mvgConvolve(dst, src, kernelWidth, kernelHeight, shiftX, shiftY, kernel, scale, bias, tilingMode);

    AROS_LIBFUNC_EXIT
}

AROS_LH11(void, vgSeparableConvolve,
    AROS_LHA(VGImage, dst, D0),
    AROS_LHA(VGImage, src, D1),
    AROS_LHA(VGint, kernelWidth, D2),
    AROS_LHA(VGint, kernelHeight, D3),
    AROS_LHA(VGint, shiftX, D4),
    AROS_LHA(VGint, shiftY, D5),
    AROS_LHA(const VGshort *, kernelX, A0),
    AROS_LHA(const VGshort *, kernelY, A1),
    AROS_LHA(VGfloat, scale, D6),
    AROS_LHA(VGfloat, bias, D7),
    AROS_LHA(VGTilingMode, tilingMode, A2),
    struct Library *, VegaBase, 116, Vega)
{
    AROS_LIBFUNC_INIT

    mvgSeparableConvolve(dst, src, kernelWidth, kernelHeight, shiftX, shiftY, kernelX, kernelY, scale, bias, tilingMode);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, vgGaussianBlur,
    AROS_LHA(VGImage, dst, D0),
    AROS_LHA(VGImage, src, D1),
    AROS_LHA(VGfloat, stdDeviationX, D2),
    AROS_LHA(VGfloat, stdDeviationY, D3),
    AROS_LHA(VGTilingMode, tilingMode, D4),
    struct Library *, VegaBase, 117, Vega)
{
    AROS_LIBFUNC_INIT

    mvgGaussianBlur(dst, src, stdDeviationX, stdDeviationY, tilingMode);

    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, vgLookup,
    AROS_LHA(VGImage, dst, D0),
    AROS_LHA(VGImage, src, D1),
    AROS_LHA(const VGubyte *, redLUT, A0),
    AROS_LHA(const VGubyte *, greenLUT, A1),
    AROS_LHA(const VGubyte *, blueLUT, A2),
    AROS_LHA(const VGubyte *, alphaLUT, A3),
    AROS_LHA(VGboolean, outputLinear, D2),
    AROS_LHA(VGboolean, outputPremultiplied, D3),
    struct Library *, VegaBase, 118, Vega)
{
    AROS_LIBFUNC_INIT

    mvgLookup(dst, src, redLUT, greenLUT, blueLUT, alphaLUT, outputLinear, outputPremultiplied);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, vgLookupSingle,
    AROS_LHA(VGImage, dst, D0),
    AROS_LHA(VGImage, src, D1),
    AROS_LHA(const VGuint *, lookupTable, A0),
    AROS_LHA(VGImageChannel, sourceChannel, D2),
    AROS_LHA(VGboolean, outputLinear, D3),
    AROS_LHA(VGboolean, outputPremultiplied, D4),
    struct Library *, VegaBase, 119, Vega)
{
    AROS_LIBFUNC_INIT

    mvgLookupSingle(dst, src, lookupTable, sourceChannel, outputLinear, outputPremultiplied);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(VGHardwareQueryResult, vgHardwareQuery,
    AROS_LHA(VGHardwareQueryType, key, D0),
    AROS_LHA(VGint, setting, D1),
    struct Library *, VegaBase, 120, Vega)
{
    AROS_LIBFUNC_INIT

    VGHardwareQueryResult _return = mvgHardwareQuery(key, setting);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(const VGubyte *, vgGetString,
    AROS_LHA(VGStringID, name, D0),
    struct Library *, VegaBase, 121, Vega)
{
    AROS_LIBFUNC_INIT

    const VGubyte * _return = mvgGetString(name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(VGUErrorCode, vguLine,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGfloat, x0, D1),
    AROS_LHA(VGfloat, y0, D2),
    AROS_LHA(VGfloat, x1, D3),
    AROS_LHA(VGfloat, y1, D4),
    struct Library *, VegaBase, 122, Vega)
{
    AROS_LIBFUNC_INIT

    VGUErrorCode _return = mvguLine(path, x0, y0, x1, y1);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(VGUErrorCode, vguPolygon,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(const VGfloat *, points, A0),
    AROS_LHA(VGint, count, D1),
    AROS_LHA(VGboolean, closed, D2),
    struct Library *, VegaBase, 123, Vega)
{
    AROS_LIBFUNC_INIT

    VGUErrorCode _return = mvguPolygon(path, points, count, closed);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(VGUErrorCode, vguRect,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGfloat, x, D1),
    AROS_LHA(VGfloat, y, D2),
    AROS_LHA(VGfloat, width, D3),
    AROS_LHA(VGfloat, height, D4),
    struct Library *, VegaBase, 124, Vega)
{
    AROS_LIBFUNC_INIT

    VGUErrorCode _return = mvguRect(path, x, y, width, height);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH7(VGUErrorCode, vguRoundRect,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGfloat, x, D1),
    AROS_LHA(VGfloat, y, D2),
    AROS_LHA(VGfloat, width, D3),
    AROS_LHA(VGfloat, height, D4),
    AROS_LHA(VGfloat, arcWidth, D5),
    AROS_LHA(VGfloat, arcHeight, D6),
    struct Library *, VegaBase, 125, Vega)
{
    AROS_LIBFUNC_INIT

    VGUErrorCode _return = mvguRoundRect(path, x, y, width, height, arcWidth, arcHeight);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(VGUErrorCode, vguEllipse,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGfloat, cx, D1),
    AROS_LHA(VGfloat, cy, D2),
    AROS_LHA(VGfloat, width, D3),
    AROS_LHA(VGfloat, height, D4),
    struct Library *, VegaBase, 126, Vega)
{
    AROS_LIBFUNC_INIT

    VGUErrorCode _return = mvguEllipse(path, cx, cy, width, height);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH8(VGUErrorCode, vguArc,
    AROS_LHA(VGPath, path, D0),
    AROS_LHA(VGfloat, x, D1),
    AROS_LHA(VGfloat, y, D2),
    AROS_LHA(VGfloat, width, D3),
    AROS_LHA(VGfloat, height, D4),
    AROS_LHA(VGfloat, startAngle, D5),
    AROS_LHA(VGfloat, angleExtent, D6),
    AROS_LHA(VGUArcType, arcType, D7),
    struct Library *, VegaBase, 127, Vega)
{
    AROS_LIBFUNC_INIT

    VGUErrorCode _return = mvguArc(path, x, y, width, height, startAngle, angleExtent, arcType);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH9(VGUErrorCode, vguComputeWarpQuadToSquare,
    AROS_LHA(VGfloat, sx0, D0),
    AROS_LHA(VGfloat, sy0, D1),
    AROS_LHA(VGfloat, sx1, D2),
    AROS_LHA(VGfloat, sy1, D3),
    AROS_LHA(VGfloat, sx2, D4),
    AROS_LHA(VGfloat, sy2, D5),
    AROS_LHA(VGfloat, sx3, D6),
    AROS_LHA(VGfloat, sy3, D7),
    AROS_LHA(VGfloat *, matrix, A0),
    struct Library *, VegaBase, 128, Vega)
{
    AROS_LIBFUNC_INIT

    VGUErrorCode _return = mvguComputeWarpQuadToSquare(sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3, matrix);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH9(VGUErrorCode, vguComputeWarpSquareToQuad,
    AROS_LHA(VGfloat, dx0, D0),
    AROS_LHA(VGfloat, dy0, D1),
    AROS_LHA(VGfloat, dx1, D2),
    AROS_LHA(VGfloat, dy1, D3),
    AROS_LHA(VGfloat, dx2, D4),
    AROS_LHA(VGfloat, dy2, D5),
    AROS_LHA(VGfloat, dx3, D6),
    AROS_LHA(VGfloat, dy3, D7),
    AROS_LHA(VGfloat *, matrix, A0),
    struct Library *, VegaBase, 129, Vega)
{
    AROS_LIBFUNC_INIT

    VGUErrorCode _return = mvguComputeWarpSquareToQuad(dx0, dy0, dx1, dy1, dx2, dy2, dx3, dy3, matrix);

    return _return;

    AROS_LIBFUNC_EXIT
}

