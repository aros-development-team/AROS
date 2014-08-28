#ifndef LIBPROTO_H
#define LIBPROTO_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/bptr.h>

#include <SDI_lib.h>

#if defined(__amigaos4__)
#define __BASE_OR_IFACE_TYPE	struct ExampleIFace *
#define __BASE_OR_IFACE_VAR		IExample
#else
#define __BASE_OR_IFACE_TYPE	struct LibraryHeader *
#define __BASE_OR_IFACE_VAR		ExampleBase
#endif
#define __BASE_OR_IFACE			__BASE_OR_IFACE_TYPE __BASE_OR_IFACE_VAR

struct LibraryHeader
{
  struct Library  libBase;
  struct Library *sysBase;
  BPTR            segList;
};

// first the prototypes of all our public library functions
LIBPROTO(SayHelloOS4, char *, REG(a6, UNUSED __BASE_OR_IFACE));
LIBPROTO(SayHelloOS3, char *, REG(a6, UNUSED __BASE_OR_IFACE));
LIBPROTO(SayHelloMOS, char *, REG(a6, UNUSED __BASE_OR_IFACE));
LIBPROTO(Uppercase, char *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, char *txt));
LIBPROTO(SPrintfA, char *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, char *buf), REG(a1, char *format), REG(a2, APTR args));
LIBPROTOVA(SPrintf, char *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, char *buf), REG(a1, char *format), ...);

#if defined(__AROS__)
AROS_LD0(char *, SayHelloOS4, struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);
AROS_LD0(char *, SayHelloOS3, struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);
AROS_LD0(char *, SayHelloMOS, struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);
AROS_LD1(char *, Uppercase, AROS_LDA(char *, txt, A0), struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);
AROS_LD3(char *, SPrintfA,
    AROS_LDA(char *, buf, A0),
    AROS_LDA(char *, format, A1),
    AROS_LDA(APTR, args, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);
#endif

#endif
