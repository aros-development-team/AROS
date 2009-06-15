#ifndef SDI_HOOK_H
#define SDI_HOOK_H

/* Includeheader

        Name:           SDI_hook.h
        Versionstring:  $VER: SDI_hook.h 1.21 (19.05.2009)
        Author:         SDI & Jens Langner
        Distribution:   PD
        Project page:   http://www.sf.net/projects/sditools/
        Description:    defines to hide compiler specific hook stuff

 1.0   21.06.02 : based on the work made for freeciv and YAM with
                  additional texts partly taken from YAM_hook.h changes made
                  by Jens Langner, largely reworked the mechanism
 1.1   07.10.02 : added HOOKPROTONP and HOOKPROTONONP requested by Jens
 1.2   18.10.02 : reverted to old MorphOS-method for GCC
 1.3   08.02.04 : modified to get it compatible to AmigaOS4
 1.4   17.02.04 : modified to get compatible to latest SDI_compiler.h changes
 1.5   02.03.04 : added UNUSED define to OS4 hook specification so that the
                  compiler can ignore some warnings.
 1.6   02.03.04 : added (APTR) casts to MorphOS prototype definition to
                  reduce compiler warnings.
 1.7   04.07.04 : removed static from all DISPATCHERPROTO definitions as there
                  may be dispatchers that are of course non static.
 1.8   07.04.05 : added MakeHookWithData (Sebastian Bauer)
 1.9   08.04.05 : changed MorphOS hooks to use HookEntry (Ilkka Lehtoranta)
 1.10  16.05.05 : simplified and fixed for vbcc/MorphOS (Frank Wille)
 1.11  17.05.05 : changed cast in DISPATCHERPROTO from (void(*)()) to APTR
                  cause SDI version of the EmulLibEntry uses APTR for easy
                  usage.
                  Added #ifndef SDI_TRAP_LIB to avoid double defines when
                  combining with SDI_interrupt.h or SDI_misc.h (Guido
                  Mersmann)
 1.12  18.05.05 : DISPATCHERPROTO wasn't working, because of the missing REG_Ax
                  definitions. Added include <emul/emulregs.h> (Guido Mersmann)
 1.13  11.12.05 : fixed a minor typo in the PPC HOOKPROTONP macro.
                  (Jens Langner)
 1.14  20.04.06 : unified static of MorphOs with non-MorphOS vesion
 1.15  30.04.06 : modified to get it compatible to AROS. (Guido Mersmann)
 1.16  06.10.06 : added new DISPATCHER() macro and separated it from the
                  DISPATCHERPROTO() definition. Now the DISPATCHERPROTO() should
                  only be used to get the correct prototype and the plain
                  DISPATCHER() for defining the dispatcher itself.
 1.17  14.07.08 : added "_" to all UNUSED variable specifications to make sure
                  a user does not use those definition on accident.
 1.18  20.03.09 : modified macros to be somewhat more compatible for an AROS
                  usage (Pavel Fedin)
 1.19  25.03.09 : fixed the DISPATCHERPROTO() macros for x86_64 AROS.
 1.20  26.03.09 : fixed m68k define checks.
 1.21  19.05.09 : added SDISPATCHER() to generate a static dispatcher.
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
** Dirk Stoecker <soft@dstoecker.de>
*/

#include "SDI_compiler.h"

/*
** Hook macros to handle the creation of Hooks/Dispatchers for different
** Operating System versions.
** Currently AmigaOS and MorphOS is supported.
**
** For more information about hooks see include file <utility/hooks.h> or
** the relevant descriptions in utility.library autodocs.
**
** Example:
**
** Creates a hook with the name "TestHook" that calls a corresponding
** function "TestFunc" that will be called with a pointer "text"
** (in register A1) and returns a long.
**
** HOOKPROTONHNO(TestFunc, LONG, STRPTR text)
** {
**   Printf(text);
**   return 0;
** }
** MakeHook(TestHook, TestFunc);
**
** Every function that is created with HOOKPROTO* must have a MakeHook() or
** MakeStaticHook() to create the corresponding hook. Best is to call this
** directly after the hook function. This is required by the GCC macros.
**
** The naming convention for the Hook Prototype macros is as followed:
**
** HOOKPROTO[NH][NO][NP]
**           ^^  ^^  ^^
**      NoHook   |    NoParameter
**            NoObject
**
** So a plain HOOKPROTO() creates you a Hook function that requires
** 4 parameters, the "name" of the hookfunction, the "obj" in REG_A2,
** the "param" in REG_A1 and a "hook" in REG_A0. Usually you will always
** use NH, as the hook structure itself is nearly never required.
**
** The DISPATCHERPROTO macro is for MUI dispatcher functions. It gets the
** functionname as argument. To supply this function for use by MUI, use
** The ENTRY macro, which also gets the function name as argument.
*/

#if defined(_M68000) || defined(__M68000) || defined(__mc68000)
  #define HOOKPROTO(name, ret, obj, param) static SAVEDS ASM ret             \
    name(REG(a0, struct Hook *hook), REG(a2, obj), REG(a1, param))
  #define HOOKPROTONO(name, ret, param) static SAVEDS ASM ret                \
    name(REG(a0, struct Hook *hook), REG(a1, param))
  #define HOOKPROTONP(name, ret, obj) static SAVEDS ASM ret                  \
    name(REG(a0, struct Hook *hook), REG(a2, obj))
  #define HOOKPROTONONP(name, ret) static SAVEDS ASM ret                     \
    name(REG(a0, struct Hook *hook))
  #define HOOKPROTONH(name, ret, obj, param) static SAVEDS ASM ret           \
    name(REG(a2, obj), REG(a1, param))
  #define HOOKPROTONHNO(name, ret, param) static SAVEDS ASM ret              \
    name(REG(a1, param))
  #define HOOKPROTONHNP(name, ret, obj) static SAVEDS ASM ret                \
    name(REG(a2, obj))
  #define HOOKPROTONHNONP(name, ret) static SAVEDS ret name(void)
#else
  #define HOOKPROTO(name, ret, obj, param) static SAVEDS ret                 \
    name(struct Hook *hook, obj, param)
  #define HOOKPROTONO(name, ret, param) static SAVEDS ret                    \
    name(struct Hook *hook, UNUSED APTR _obj, param)
  #define HOOKPROTONP(name, ret, obj) static SAVEDS ret                      \
    name(struct Hook *hook, obj, UNUSED APTR _param)
  #define HOOKPROTONONP(name, ret) static SAVEDS ret                         \
    name(struct Hook *hook, UNUSED APTR _obj, UNUSED APTR _param)
  #define HOOKPROTONH(name, ret, obj, param) static SAVEDS ret               \
    name(UNUSED struct Hook *_hook, obj, param)
  #define HOOKPROTONHNO(name, ret, param) static SAVEDS ret                  \
    name(UNUSED struct Hook *_hook, UNUSED APTR _obj, param)
  #define HOOKPROTONHNP(name, ret, obj) static SAVEDS ret                    \
    name(UNUSED struct Hook *_hook, obj, UNUSED APTR _param) 
  #define HOOKPROTONHNONP(name, ret) static SAVEDS ret name(void)
#endif

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

  #define MakeHook(hookname, funcname) struct Hook hookname = {{NULL, NULL}, \
    (HOOKFUNC)HookEntry, (HOOKFUNC)funcname, NULL}
  #define MakeHookWithData(hookname, funcname, data) struct Hook hookname =  \
    {{NULL, NULL}, (HOOKFUNC)HookEntry, (HOOKFUNC)funcname, (APTR)data}
  #define MakeStaticHook(hookname, funcname) static struct Hook hookname =   \
    {{NULL, NULL}, (HOOKFUNC)HookEntry, (HOOKFUNC)funcname, NULL}
  #define DISPATCHERPROTO(name) ULONG name(struct IClass * cl, Object * obj, \
    Msg msg);                                                                \
    extern const struct SDI_EmulLibEntry Gate_##name
  #define DISPATCHER(name)                                                   \
    struct IClass;                                                           \
    ULONG name(struct IClass * cl, Object * obj, Msg msg);                   \
    static ULONG Trampoline_##name(void) {return name((struct IClass *)      \
    REG_A0, (Object *) REG_A2, (Msg) REG_A1);}                               \
    const struct SDI_EmulLibEntry Gate_##name = {SDI_TRAP_LIB, 0,            \
    (APTR) Trampoline_##name};                                               \
    ULONG name(struct IClass * cl, Object * obj, Msg msg)
  #define SDISPATCHER(name)                                                  \
    struct IClass;                                                           \
    static ULONG name(struct IClass * cl, Object * obj, Msg msg);            \
    static ULONG Trampoline_##name(void) {return name((struct IClass *)      \
    REG_A0, (Object *) REG_A2, (Msg) REG_A1);}                               \
    static const struct SDI_EmulLibEntry Gate_##name = {SDI_TRAP_LIB, 0,     \
    (APTR) Trampoline_##name};                                               \
    static ULONG name(struct IClass * cl, Object * obj, Msg msg)
  #define ENTRY(func) (APTR)&Gate_##func

#else /* !__MORPHOS__ */
  #define MakeHook(hookname, funcname) struct Hook hookname = {{NULL, NULL}, \
    (HOOKFUNC)funcname, NULL, NULL}
  #define MakeHookWithData(hookname, funcname, data) struct Hook hookname =  \
    {{NULL, NULL}, (HOOKFUNC)funcname, NULL, (APTR)data}
  #define MakeStaticHook(hookname, funcname) static struct Hook hookname =   \
    {{NULL, NULL}, (HOOKFUNC)funcname, NULL, NULL}
  #define ENTRY(func) (APTR)func
  #define DISPATCHERPROTO(name) SAVEDS ASM IPTR name(REG(a0,                 \
    struct IClass * cl), REG(a2, Object * obj), REG(a1, Msg msg))
  #define DISPATCHER(name) DISPATCHERPROTO(name)
  #define SDISPATCHER(name) static DISPATCHERPROTO(name)

#endif

#define InitHook(hook, orighook, data) ((hook)->h_Entry = (orighook).h_Entry,\
  (hook)->h_SubEntry = (orighook).h_SubEntry,(hook)->h_Data = (APTR)(data))

#endif /* SDI_HOOK_H */
