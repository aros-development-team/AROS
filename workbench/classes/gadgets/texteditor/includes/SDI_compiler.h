#ifndef SDI_COMPILER_H
#define SDI_COMPILER_H

/* Includeheader

        Name:           SDI_compiler.h
        Versionstring:  $VER: SDI_compiler.h 1.21 (28.02.2005)
        Author:         SDI & Jens Langner
        Distribution:   PD
        Description:    defines to hide compiler stuff

 1.1   25.06.98 : created from data made by Gunter Nikl
 1.2   17.11.99 : added VBCC
 1.3   29.02.00 : fixed VBCC REG define
 1.4   30.03.00 : fixed SAVEDS for VBCC
 1.5   29.07.00 : added #undef statements (needed e.g. for AmiTCP together
                  with vbcc)
 1.6   19.05.01 : added STACKEXT and Dice stuff
 1.7   16.06.01 : added MorphOS specials and VARARGS68K
 1.8   21.09.02 : added MorphOS register stuff
 1.9   26.09.02 : added OFFSET macro. Thanks Frank Wille for suggestion
 1.10  18.10.02 : reverted to old MorphOS-method for GCC
 1.11  09.11.02 : added REGARGS define to MorphOS section
 1.12  18.01.04 : some adaptions for AmigaOS4 compatibility
 1.13  17.02.04 : changed ASM macros to be a simple define and added
                  INTERRUPT, CHIP and FAR
 1.14  02.03.04 : added UNUSED which can be used to specify a function parameter
                  or variable as "unused" which will not cause any compiler warning.
 1.15  02.03.04 : added special INLINE define for gcc > 3.0 version
 1.17  07.03.04 : changed INLINE definition of gcc <= 2.95.3 to be static aswell.
 1.18  21.06.04 : added USED and USED_VAR attribute to allow placing a
                  __attribute__((used)) to a function and a variable, taking care of
                  different compiler versions.
 1.19  04.07.04 : register specification for variables is not supported on MorphOS,
                  so we modified the REG() macro accordingly.
 1.20  28.02.05 : correct INLINE for VBCC.
 1.21  28.02.05 : cleanup __GCC__ case.
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

#undef ASM
#undef REG
#undef LREG
#undef CONST
#undef SAVEDS
#undef INLINE
#undef REGARGS
#undef STDARGS
#undef OFFSET
#undef INTERRUPT
#undef CHIP
#undef FAR
#undef UNUSED
#undef USED
#undef USED_VAR

/* first "exceptions" */

#if defined(__MAXON__)
  #define STDARGS
  #define STACKEXT
  #define REGARGS
  #define SAVEDS
  #define INLINE inline
#elif defined(__VBCC__)
  #define STDARGS
  #define STACKEXT
  #define REGARGS
  #if (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
    #define INLINE inline
  #else
    #define INLINE static
  #endif
  #define OFFSET(p,m) __offsetof(struct p,m)
  #if defined(__MORPHOS__)
    #define REG(reg,arg) __reg(MOS__##reg) arg

    /* NOTE: This assumes "quick native mode" when compiling libraries. */
    #define MOS__a0 "r24"
    #define MOS__a1 "r25"
    #define MOS__a2 "r26"
    #define MOS__a3 "r27"
    #define MOS__a4 "r28"
    #define MOS__a5 "r29"
    #define MOS__a6 "r30"
    /* #define MOS__a7 "r31" */
    #define MOS__d0 "r16"
    #define MOS__d1 "r17"
    #define MOS__d2 "r18"
    #define MOS__d3 "r19"
    #define MOS__d4 "r20"
    #define MOS__d5 "r21"
    #define MOS__d6 "r22"
    #define MOS__d7 "r23"

  #else
    #define REG(reg,arg) __reg(#reg) arg
  #endif
#elif defined(__STORM__)
  #define STDARGS
  #define STACKEXT
  #define REGARGS
  #define INLINE inline
#elif defined(__SASC)
  #define ASM __asm
#elif defined(__GNUC__)
  #define UNUSED __attribute__((unused)) /* for functions, variables and types */
  #define USED   __attribute__((used))   /* for functions only! */
  #if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0)
    #define USED_VAR USED /* for variables only! */
    #define INLINE static __inline __attribute__((always_inline))
  #endif
  /* we have to distinguish between AmigaOS4 and MorphOS */
  #if defined(__PPC__)
    #define REG(reg,arg) arg
    #define SAVEDS
    #define STDARGS
    #define REGARGS
    #define STACKEXT
    #if defined(__MORPHOS__)
      #define VARARGS68K  __attribute__((varargs68k))
    #endif
    #define INTERRUPT
    #define CHIP
  #else
    #define REG(reg,arg) arg __asm(#reg)
    #define LREG(reg,arg) register REG(reg,arg)
  #endif
  #define FAR
#elif defined(_DCC)
  #define REG(reg,arg) __##reg arg
  #define STACKEXT __stkcheck
  #define STDARGS __stkargs
  #define INLINE static
#endif

/* then "common" ones */

#if !defined(ASM)
  #define ASM
#endif
#if !defined(REG)
  #define REG(reg,arg) register __##reg arg
#endif
#if !defined(LREG)
  #define LREG(reg,arg) register arg
#endif
#if !defined(CONST)
  #define CONST const
#endif
#if !defined(SAVEDS)
  #define SAVEDS __saveds
#endif
#if !defined(INLINE)
  #define INLINE static __inline
#endif
#if !defined(REGARGS)
  #define REGARGS __regargs
#endif
#if !defined(STDARGS)
  #define STDARGS __stdargs
#endif
#if !defined(STACKEXT)
  #define STACKEXT __stackext
#endif
#if !defined(VARARGS68K)
  #define VARARGS68K
#endif
#if !defined(OFFSET)
  #define OFFSET(structName, structEntry) \
    ((char *)(&(((struct structName *)0)->structEntry))-(char *)0)
#endif
#if !defined(INTERRUPT)
  #define INTERRUPT __interrupt
#endif
#if !defined(CHIP)
  #define CHIP __chip
#endif
#if !defined(FAR)
  #define FAR __far
#endif
#if !defined(UNUSED)
  #define UNUSED
#endif
#if !defined(USED)
  #define USED
#endif
#if !defined(USED_VAR)
  #define USED_VAR
#endif
#endif /* SDI_COMPILER_H */
