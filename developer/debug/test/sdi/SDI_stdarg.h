#ifndef SDI_STDARG_H
#define SDI_STDARG_H

/* Includeheader

        Name:           SDI_stdarg.h
        Versionstring:  $VER: SDI_stdarg.h 1.3 (06.08.2016)
        Author:         Jens Maus
        Distribution:   PD
        Project page:   https://github.com/adtools/SDI
        Description:    defines to hide OS specific variable arguments
                        function definitions

 1.0   05.07.2004 : initial version
 1.1   06.06.2014 : added a type cast to VA_ARG() result
 1.2   27.03.2016 : when using GCC4/5 for MorphOS overflow_arg_area is not
                    supported anymore (Jens Maus)
 1.3   06.08.2016 : implemented VA_COPY() and SDI_VACAST type helper macros
                    to fight the problem that GCC5 for MorphOS doesn't support
                    VARARGS68K and thus we need type casts to please the
                    compiler on complaining about the GCC inline macros that
                    can be used to partly replace it.

*/

/*
** This is PD (Public Domain). This means you can do with it whatever you want
** without any restrictions. I only ask you to tell me improvements, so I may
** fix the main line of this files as well.
**
** To keep confusion level low: When changing this file, please note it in
** above history list and indicate that the change was not made by myself
** (e.g. add your name or nick name).
**
** Find the latest version of this file at:
** https://github.com/adtools/SDI
**
** Jens Maus <mail@jens-maus.de>
** Dirk Stöcker <soft@dstoecker.de>
*/

#include "SDI_compiler.h"

/*
** Variable arguments function macros to allow specification of the
** variable arguments typical functions like va_list/va_start/va_end in
** an operating system independent fashion.
**
** With help of the following macro definition a developer might define
** variable arguments functions for different types of operating
** system implementations without having to clutter the sources with
** multiple "#ifdef" defines just because all of these operating systems
** come with different varable arguments support functions.
**
** Example:
**
** Instead of using the standard va_list, va_start and va_end functions
** of <stdarg.h>, a developer might specify the following sprintf()
** function to make it automatically compatible with AmigaOS3, AmigaOS4
** and also MorphOS.
**
** int VARARGS68K sprintf(char *buf, char *fmt, ...)
** {
**   VA_LIST args;
**
**   VA_START(args, fmt);
**   RawDoFmt(fmt, VA_ARG(args, void *), NULL, buf);
**   VA_END(args);
**
**   return(strlen(buf));
** }
**
** Please note the uppercase letters of the macros in contrast to the
** official varargs functions specified in <stdarg.h>.
**
** By using this schema a developer might ensure full source code backward
** compatibility to AmigaOS3 without having to introduce dozens of #ifdef
** statements in his code.
*/

#include <stdarg.h>

#ifdef VA_LIST
#undef VA_LIST
#endif
#ifdef VA_START
#undef VA_START
#endif
#ifdef VA_ARG
#undef VA_ARG
#endif
#ifdef VA_END
#undef VA_END
#endif

#if defined(__amigaos4__)
  #define VA_LIST             va_list
  #define VA_START(va, start) va_startlinear((va), (start))
  #define VA_ARG(va, type)    va_getlinearva((va), type)
  #define VA_COPY(d, s)       (d) = (s)
  #define VA_END(va)          va_end((va))
#elif defined(__MORPHOS__)
  #define VA_LIST             va_list
  #define VA_START(va, start) va_start((va), (start))
  #if __GNUC__ == 2
    #define VA_ARG(va, type)    (type)((va)->overflow_arg_area)
  #else
    #define VA_ARG(va, type)    va_arg(va, type)
  #endif
  #define VA_COPY(d, s)       __va_copy(d, s)
  #define VA_END(va)          va_end((va))
#else
  #define VA_LIST             va_list
  #define VA_START(va, start) va_start((va), (start))
  #define VA_ARG(va, type)    (type)(va)
  #if defined(__AROS__)
    #define VA_COPY(d, s)     va_copy(d, s)
  #else
    #define VA_COPY(d, s)     (d) = (s)
  #endif
  #define VA_END(va)          va_end((va))
#endif

/* This counts the number of args */
#define SDI_NARGS_SEQ( _1, _2, _3, _4, _5, _6, _7, _8, _9,_10,\
                      _11,_12,_13,_14,_15,_16,_17,_18,_19,_20,\
                      _21,_22,_23,_24,_25,_26,_27,_28,_29,_30,\
                      N,...) N
#define SDI_NARGS(...) SDI_NARGS_SEQ(__VA_ARGS__,\
                       30,29,28,27,26,25,24,23,22,21,\
                       20,19,18,17,16,15,14,13,12,11,\
                       10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

/* This will let macros expand before concating them */
#define SDI_PRIMITIVE_CAT(x, y) x ## y
#define SDI_CAT(x, y) SDI_PRIMITIVE_CAT(x, y)

/* This will call a macro on each argument passed in */
#define SDI_VACAST(...) SDI_CAT(SDI_CAST_, SDI_NARGS(__VA_ARGS__))((ULONG), __VA_ARGS__)
#define SDI_CAST_1(m,x1) m(x1)
#define SDI_CAST_2(m,x1,x2) m(x1),m(x2)
#define SDI_CAST_3(m,x1,x2,x3) m(x1),m(x2),m(x3)
#define SDI_CAST_4(m,x1,x2,x3,x4) m(x1),m(x2),m(x3),m(x4)
#define SDI_CAST_5(m,x1,x2,x3,x4,x5) m(x1),m(x2),m(x3),m(x4),m(x5)
#define SDI_CAST_6(m,x1,x2,x3,x4,x5,x6) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6)
#define SDI_CAST_7(m,x1,x2,x3,x4,x5,x6,x7) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7)
#define SDI_CAST_8(m,x1,x2,x3,x4,x5,x6,x7,x8) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8)
#define SDI_CAST_9(m,x1,x2,x3,x4,x5,x6,x7,x8,x9) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9)
#define SDI_CAST_10(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10)
#define SDI_CAST_11(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11)
#define SDI_CAST_12(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12)
#define SDI_CAST_13(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13)
#define SDI_CAST_14(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14)
#define SDI_CAST_15(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15)
#define SDI_CAST_16(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16)
#define SDI_CAST_17(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17)
#define SDI_CAST_18(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18)
#define SDI_CAST_19(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19)
#define SDI_CAST_20(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20)
#define SDI_CAST_21(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21)
#define SDI_CAST_22(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21),m(x22)
#define SDI_CAST_23(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21),m(x22),m(x23)
#define SDI_CAST_24(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21),m(x22),m(x23),m(x24)
#define SDI_CAST_25(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24,x25) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21),m(x22),m(x23),m(x24),m(x25)
#define SDI_CAST_26(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24,x25,x26) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21),m(x22),m(x23),m(x24),m(x25),m(x26)
#define SDI_CAST_27(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24,x25,x26,x27) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21),m(x22),m(x23),m(x24),m(x25),m(x26),m(x27)
#define SDI_CAST_28(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24,x25,x26,x27,x28) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21),m(x22),m(x23),m(x24),m(x25),m(x26),m(x27),m(x28)
#define SDI_CAST_29(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24,x25,x26,x27,x28,x29) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21),m(x22),m(x23),m(x24),m(x25),m(x26),m(x27),m(x28),m(x29)
#define SDI_CAST_30(m,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24,x25,x26,x27,x28,x29,x30) m(x1),m(x2),m(x3),m(x4),m(x5),m(x6),m(x7),m(x8),m(x9),m(x10),m(x11),m(x12),m(x13),m(x14),m(x15),m(x16),m(x17),m(x18),m(x19),m(x20),m(x21),m(x22),m(x23),m(x24),m(x25),m(x26),m(x27),m(x28),m(x29),m(x30)

#endif /* SDI_STDARG_H */
