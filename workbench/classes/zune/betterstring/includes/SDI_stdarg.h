#ifndef SDI_STDARG_H
#define SDI_STDARG_H

/* Includeheader

        Name:           SDI_stdarg.h
        Versionstring:  $VER: SDI_stdarg.h 1.0 (05.07.2004)
        Author:         Jens Langner
        Distribution:   PD
        Description:    defines to hide OS specific variable arguments
                        function definitions

 1.0   05.07.04 : initial version

*/

/*
** This is PD (Public Domain). This means you can do with it whatever you want
** without any restrictions. I only ask you to tell me improvements, so I may
** fix the main line of this files as well.
**
** To keep confusion level low: When changing this file, please note it in
** above history list and indicate that the change was not made by myself
** (e.g. add your name or nick name).
**
** Jens Langner <Jens.Langner@light-speed.de> and
** Dirk Stöcker <stoecker@epost.de>
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
  #define VA_END(va)          va_end((va))
#elif defined(__MORPHOS__)
  #define VA_LIST             va_list
  #define VA_START(va, start) va_start((va), (start))
  #define VA_ARG(va, type)    (va)->overflow_arg_area
  #define VA_END(va)          va_end((va))
#else
  #define VA_LIST             va_list
  #define VA_START(va, start) va_start((va), (start))
  #define VA_ARG(va, type)    (va)
  #define VA_END(va)          va_end((va))
#endif

#endif /* SDI_STDARG_H */
