#ifndef PROTO_GRAPHICS_H
#define PROTO_GRAPHICS_H

#ifndef GRAPHICS_SCALE_H
#include <graphics/scale.h>
#endif /* !GRAPHICS_SCALE_H */

#include <clib/graphics_protos.h>

#ifdef __GNUC__
#undef GetOutlinePen
#include <inline/graphics.h>
#endif /* __GNUC__ */

#ifndef __NOLIBBASE__
extern struct GfxBase *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
GfxBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_GRAPHICS_H */
