#ifndef _LIBDEFS_H
#define _LIBDEFS_H
#define NAME_STRING      "example.library"
#define NT_TYPE          NT_LIBRARY
#define LC_BUILDNAME(n)  n
#define LIBBASE          ExampleBase
#define LIBBASETYPE      struct ExampleBase
#define LIBBASETYPEPTR   struct ExampleBase *
#define VERSION_NUMBER   37
#define REVISION_NUMBER  15
#define BASENAME         Example
#define BASENAME_STRING  "Example"
#define VERSION_STRING   "$VER: example 37.15 (20.08.1997)\n\r"
#define LIBEND           Example_end
#define LIBFUNCTABLE     Example_functable
#define COPYRIGHT_STRING "(C)opyright 1996-97 by Andreas R. Kleinert. All rights reserved."
#endif /* _LIBDEFS_H */
