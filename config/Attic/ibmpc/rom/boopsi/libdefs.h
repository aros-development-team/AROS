#ifndef _LIBDEFS_H
#define _LIBDEFS_H
#define NAME_STRING      "boopsi.library"
#define NT_TYPE          NT_LIBRARY
#define NOEXPUNGE
#define ROMBASED
#define LC_UNIQUE_PREFIX BOOPSI
#define LC_BUILDNAME(n)  BOOPSI ## n
#define LIBBASE          BOOPSIBase
#define LIBBASETYPE      struct Library
#define LIBBASETYPEPTR   struct Library *
#define VERSION_NUMBER   41
#define REVISION_NUMBER  1
#define BASENAME         BOOPSI
#define BASENAME_STRING  "BOOPSI"
#define VERSION_STRING   "$VER: boopsi 41.1 (01.11.1998)\r\n"
#define LIBEND           BOOPSI_end
#define LIBFUNCTABLE     BOOPSI_functable
#define COPYRIGHT_STRING ""
#endif /* _LIBDEFS_H */
