#ifndef _LIBDEFS_H
#define _LIBDEFS_H
#define NAME_STRING      "graphics.hidd"
#define NT_TYPE          NT_HIDD
#define NOEXPUNGE
#define ROMBASED
#define LC_UNIQUE_PREFIX GraphicsHidd
#define LC_BUILDNAME(n)  GraphicsHidd ## n
#define LIBBASE          IntHIDDGraphicsBase
#define LIBBASETYPE      struct IntHIDDGraphicsBase
#define LIBBASETYPEPTR   struct IntHIDDGraphicsBase *
#define VERSION_NUMBER   1
#define REVISION_NUMBER  0
#define BASENAME         GraphicsHidd
#define BASENAME_STRING  "GraphicsHidd"
#define VERSION_STRING   "$VER: graphics 1.0 (03.03.2002)\r\n"
#define LIBEND           GraphicsHidd_end
#define LIBFUNCTABLE     GraphicsHidd_functable
#define COPYRIGHT_STRING ""
#endif /* _LIBDEFS_H */
