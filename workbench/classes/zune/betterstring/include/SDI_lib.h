#ifndef SDI_LIB_H
#define SDI_LIB_H

/* Includeheader

        Name:           SDI_lib.h
        Versionstring:  $VER: SDI_lib.h 1.9 (15.03.2009)
        Author:         Jens Langner
        Distribution:   PD
        Project page:   http://www.sf.net/projects/sditools/
        Description:    defines to hide OS specific library function definitions

 1.0   09.05.04 : initial version which allows to hide OS specific shared
                  library function definition like it has been introduced with
                  the new interface based library system in AmigaOS4.
 1.1   13.05.04 : replaced the LIBENTRY() macro with some more sophisticated
                  LFUNC_ macros which allow to maintain varargs library functions
                  in a much easier manner.
 1.2   22.05.04 : removed the anyway not required SAVEDS and ASM statements
                  from the LIBFUNC macro for OS4. Also removed the LIBFUNC
                  statement in the 68k LIBPROTOVA() macro so that no registers
                  can be used in there (which should be the default).
 1.3   04.07.04 : added empty LIBFUNC define for MorphOS which was missing.
 1.4   05.10.04 : added missing LIBFUNC call to OS3/MOS interface
 1.5   19.05.05 : fixed some documentation glitches (Guido Mersmann)
 1.6   08.06.05 : swapped LIBFUNC and ret within the prototype, because
                  c standard says attributes first and vbcc want them like
                  this. (Guido Mersmann)
                  changed the documentation to explain that LIBFUNC must be
                  set first. (Guido Mersmann)
 1.7   11.12.05 : adapted all macros to be somewhat more compatible to also
                  OS3 and MorphOS. Now in the real use case (codesets.library)
                  it required a fundamental rework of the macros. (Jens Langner)
 1.8   28.02.06 : removed "##" in front of the OS3 __VARARGS__ usage as they
                  causing errors on GCC >= 3.x.
 1.9   15.03.09 : fixed some missing function prototype in LIBPROTOVA()
 1.10  30.04.09 : added approriate LIBPROTOVA() definition for OS3 and MorphOS
                  to at least make the functions known. The same pattern as for
                  LIBSTUBVA() will be used now (Thore Böckelmann)
 1.11  04.05.09 : reverted the faulty LIBPROTOVA() definition to its previous
                  version (Thore Böckelmann)

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
** http://cvs.sourceforge.net/viewcvs.py/sditools/sditools/headers/
**
** Jens Langner <Jens.Langner@light-speed.de> and
** Dirk Stöcker <soft@dstoecker.de>
*/

#include "SDI_compiler.h"

/*
** Library function macros to handle the definition/usage for different
** Operating System versions.
** Currently special library function handling for AmigaOS version 4 is
** supported
**
** Example:
**
** Defines a library jump function "TestFunc" with a ULONG return value and
** which is called by the corresponding library vector of a shared library.
**
** LIBFUNC ULONG TestFunc(REG(d0, text))
** {
**   Printf(text);
**   return 0;
** }
**
** Please note the use of the LIBFUNC macro which defines this function
** automatically as a function which is directly called from within a shared
** library. Since this macro contains compiler attributes it must be
** called first, even if some compiler allow attributes and result type
** mixed, other do not and we want to keep the stuff compiler independent.
**
** If you now require to have some OS/compiler independent prototype
** definition please use the following statement:
**
** LIBPROTO(TestFunc, ULONG, REG(d0, text));
**
** This will ensure that you get a proper prototype for the same function
** where this macro will automatically take care that a libstub_* stub
** will also automatically defines as required if you are generating a
** OS4 shared library which should also be backward compatible to OS3.
**
** So if you then want to add this function to a library interface please
** use the LFUNC_* macros to generate your library function vector
** like this example one:
**
** #define libvector LFUNC_FAS(SomeFunc)    \
**                   LFUNC_FA_(TestFunc)    \
**                   LFUNC_VA_(VarargsFunc)
**
** This way you can then easily put the "libvector" define in your real
** library function vector of your shared library instead of having to
** specify each function with surrounded "#ifdef" defines. These macros
** will then also take automatically care that the varargs functions
** will only be specified on OS versions where these functions are now
** real functions (like with OS4)
**
** Such stub functions can then also be easily specified as followed
** (in analogy to the above example)
**
** LIBPROTO(TestFunc, ULONG, REG(d0, text))
** {
**   return TestFunc(text);
** }
**
** On AmigaOS4 using this mechanism for the definition of the library functions
** will automatically ensure that the "struct Interface *self" pointer is included
** and can be easily referenced as such in the function.
**
** By using this schema a developer might ensure full source code backward
** compatibility to AmigaOS3 without having to introduce dozens of #ifdef
** statements in his code.
*/

#ifdef LIBFUNC
#undef LIBFUNC
#endif

#if defined(__amigaos4__)
  #define LIBFUNC
  #if !defined(__cplusplus) &&                                        \
    (__STDC_VERSION__ >= 199901L || __GNUC__ >= 3 ||                  \
    (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
    #define LIBPROTO(name, ret, ...)                                  \
      LIBFUNC ret name(__VA_ARGS__);                                  \
      LIBFUNC ret libstub_##name(struct Interface *self UNUSED,       \
      ## __VA_ARGS__)
    #define LIBPROTOVA(name, ret, ...)                                \
      /*LIBFUNC ret VARARGS68K name(__VA_ARGS__);*/                   \
      LIBFUNC ret VARARGS68K                                          \
      libstub_##name(struct Interface *self UNUSED, ## __VA_ARGS__)
    #define LIBSTUB(name, ret, ...)                                   \
      LIBFUNC ret name(__VA_ARGS__);                                  \
      LIBFUNC ret libstub_##name(struct Interface *self UNUSED,       \
      ## __VA_ARGS__)
    #define LIBSTUBVA(name, ret, ...)                                 \
      LIBFUNC ret VARARGS68K                                          \
      libstub_##name(struct Interface *self UNUSED, ## __VA_ARGS__)
  #endif
  #define LFUNC_FAS(name) libstub_##name
  #define LFUNC_VAS(name) libstub_##name
  #define LFUNC_FA_(name) ,libstub_##name
  #define LFUNC_VA_(name) ,libstub_##name
  #define LFUNC(name)     libstub_##name
#elif defined(__MORPHOS__)
  #define LIBFUNC
  #if !defined(__cplusplus) &&                                        \
    (__STDC_VERSION__ >= 199901L || __GNUC__ >= 3 ||                  \
    (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
    #define LIBPROTO(name, ret, ...)                                  \
      LIBFUNC ret name(__VA_ARGS__);                                  \
      LIBFUNC ret libstub_##name(void)
    #define LIBPROTOVA(name, ret, ...)                                \
      LIBFUNC ret VARARGS68K name(__VA_ARGS__);
    #define LIBSTUB(name, ret, ...)                                   \
      LIBFUNC ret name(__VA_ARGS__);                                  \
      LIBFUNC ret libstub_##name(void)
    #define LIBSTUBVA(name, ret, ...)                                 \
      LIBFUNC UNUSED ret VARARGS68K libstub_##name(void)
  #endif
  #define LFUNC_FAS(name) libstub_##name
  #define LFUNC_VAS(name)
  #define LFUNC_FA_(name) ,libstub_##name
  #define LFUNC_VA_(name)
  #define LFUNC(name)     libstub_##name
#else
  #define LIBFUNC SAVEDS ASM
  #if !defined(__cplusplus) &&                                        \
    (__STDC_VERSION__ >= 199901L || __GNUC__ >= 3 ||                  \
    (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
    #define LIBPROTO(name, ret, ...)                                  \
      LIBFUNC ret name(__VA_ARGS__)
    #define LIBPROTOVA(name, ret, ...)                                \
      LIBFUNC ret STDARGS VARARGS68K name(__VA_ARGS__);
    #define LIBSTUB(name, ret, ...)                                   \
      LIBFUNC ret name(__VA_ARGS__);                                  \
      LIBFUNC ret libstub_##name(__VA_ARGS__)
    #define LIBSTUBVA(name, ret, ...)                                 \
      LIBFUNC UNUSED ret STDARGS VARARGS68K libstub_##name(__VA_ARGS__)
  #endif
  #define LFUNC_FAS(name) name
  #define LFUNC_VAS(name)
  #define LFUNC_FA_(name) ,name
  #define LFUNC_VA_(name)
  #define LFUNC(name)     name
#endif

#if !defined(LIBPROTO) || !defined(LIBPROTOVA)
  #error "OS or compiler is not yet supported by SDI_lib.h"
#endif

#endif /* SDI_LIB_H */
