#ifndef _LIBDEFS_H
#define _LIBDEFS_H
#define NAME_STRING      "vga.hidd"
#define NT_TYPE          NT_HIDD
#define NOEXPUNGE
#define ROMBASED
#define LC_UNIQUE_PREFIX vga
#define LC_BUILDNAME(n)  vga ## n
#define LIBBASE          vgaBase
#define LIBBASETYPE      struct vgabase
#define LIBBASETYPEPTR   struct vgabase *
#define VERSION_NUMBER   1
#define REVISION_NUMBER  0
#define BASENAME         vga
#define BASENAME_STRING  "vga"
#define VERSION_STRING   "$VER: vga 1.0 (09.02.2000)\r\n"
#define LIBEND           vga_end
#define LIBFUNCTABLE     vga_functable
#define COPYRIGHT_STRING ""
#endif /* _LIBDEFS_H */
