#ifndef SDI_MISC_H
#define SDI_MISC_H

/* Includeheader

        Name:           SDI_misc.h
        Versionstring:  $VER: SDI_misc.h 1.0 (17.05.2005)
        Author:         Guido Mersmann
        Distribution:   PD
        Project page:   http://www.sf.net/projects/sditools/
        Description:    defines to hide compiler specific function stuff.
                        This header is ment to keep all minor functions
                        like PutChProc() used by RawDoFMT().

 1.0   17.05.05 : inspired by the SDI_#?.h files made by Jens Langner
                  and Dirk Stöcker I created this file to handle rawdofmt()
                  functions.
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
** Guido Mersmann <geit@gmx.de>
**
*/

#include "SDI_compiler.h"

/*
** Function macros to handle the creation of functions for different
** Operating System versions.
** Currently AmigaOS and MorphOS is supported.
**
** Currently the following macros are available:
**
** PUTCHARPROTO
**
** When using RawDoFmt() from exec.library you need this function to
** store the output created by the function.
**
** For more information about RawDoFmt() take a look into the
** relevant descriptions in exec.library autodocs.
**
** Example:
**
** struct SPrintfStream
** {
**     char    *Target;
**     ULONG    TargetSize;
** };
**
** PUTCHARPROTO( SPrintf_DoChar, char c, struct SPrintfStream *s )
** {
**     *(s->Target++) = c;
** }
**
** ULONG SPrintf(char *format, char *target, ULONG *args)
** {
** struct SPrintfStream s;
**
**     s.Target  = target;
**
**     RawDoFmt( format, args, ENTRY( SPrintf_DoChar), &s);
**
** return( s.Target - target);
** }
**
** As you can see usage within RawDoFmt() requires using the ENTRY()
** macro.
**
*/

#ifdef __MORPHOS__

#ifndef SDI_TRAP_LIB /* avoid defining this twice */

  #include <proto/alib.h>
  #include <emul/emulregs.h>

  #define SDI_TRAP_LIB 0xFF00 /* SDI prefix to reduce conflicts */

  struct SDI_EmulLibEntry
  {
    UWORD Trap;
    UWORD pad;
    APTR  Func;
  };

#endif

  #define PUTCHARPROTO(name, chr, buffer)                                   \
	SAVEDS ASM void name( chr, buffer);                                     \
	static void Trampoline_##name(void) { name( (char) REG_D0,              \
	(APTR) REG_A3);}                                                        \
    static const struct SDI_EmulLibEntry Gate_##name = {SDI_TRAP_LIB, 0,    \
    (APTR) Trampoline_##name};                                              \
	SAVEDS ASM void name( chr, buffer)

  #define ENTRY(func) (APTR)&Gate_##func

#else

  #define PUTCHARPROTO(name, chr, buffer)                                   \
    SAVEDS ASM void name(REG(d0, chr), REG(a3, buffer))

  #define ENTRY(func) (APTR)func

#endif /* __MORPHOS__ */

/*
** JPGCOMPRESS_HOOK   (use with JPG_CompressHook)
** JPGDECOMPRESS_HOOK (use with JPG_DecompressHook)
*/

#define JPGDECOMPRESS_HOOK JPGCOMPRESS_HOOK

#ifdef __MORPHOS__

  #define JPGCOMPRESS_HOOK(name, buffer, line, width, userdata)             \
	SAVEDS ASM void * name( buffer, line, width, userdata);                 \
	static void *Trampoline_##name(void) { name(( buffer) REG_A0,           \
	(line) REG_D0, (width) REG_D1, (userdata) REG_A1);}                     \
    static const struct SDI_EmulLibEntry Gate_##name = {SDI_TRAP_LIB, 0,    \
    (APTR) Trampoline_##name};                                              \
	SAVEDS ASM void * name( buffer, line, width, userdata)

  #define ENTRY(func) (APTR)&Gate_##func

#else

  #define JPGCOMPRESS_HOOK(name, buffer, line, width, userdata)             \
	SAVEDS ASM void *name(REG(a0, buffer), REG(d0, line), REG(d1, width), REG(a1, userdata))

  #define ENTRY(func) (APTR)func

#endif /* __MORPHOS__ */




#endif /* SDI_MISC_H */

