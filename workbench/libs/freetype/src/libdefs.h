#ifndef _LIBDEFS_H
#define _LIBDEFS_H
#define NAME_STRING      "freetype2.library"
#define NT_TYPE          NT_LIBRARY
#define LC_UNIQUE_PREFIX FreeType2
#define LC_BUILDNAME(n)  FreeType2_ ## n
#define LIBBASE          Library
#define LIBBASETYPE      struct LibHeader
#define LIBBASETYPEPTR   struct LibHeader *
#define VERSION_NUMBER   1
#define REVISION_NUMBER  2
#define BASENAME         FreeType2
#define BASENAME_STRING  "FreeType2"
#define VERSION_STRING   "$VER: freetype2 1.2 (01.12.2002)\r\n"
#define LIBEND           freetype2_end
#define LIBFUNCTABLE     freetype2_functable
#define COPYRIGHT_STRING ""
#endif /* _LIBDEFS_H */
