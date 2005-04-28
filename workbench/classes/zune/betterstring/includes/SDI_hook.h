#ifndef SDI_HOOK_H
#define SDI_HOOK_H

/* Includeheader

        Name:           SDI_hook.h
        Versionstring:  $VER: SDI_hook.h 1.9 (08.04.2005)
        Author:         SDI & Jens Langner
        Distribution:   PD
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
                  may dispatcher that are of course non static.
 1.8   07.04.05 : added MakeHookWithData (Sebastian Bauer)
 1.9   08.04.05 : changed MorphOS hooks to use HookEntry (Ilkka Lehtoranta)
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

#if !defined(__MORPHOS__) || !defined(__GNUC__)
  #if defined(__amigaos4__)
  #define HOOKPROTO(name, ret, obj, param) static SAVEDS ASM ret             \
    name(REG(a0, struct Hook *hook), REG(a2, obj), REG(a1, param))
  #define HOOKPROTONO(name, ret, param) static SAVEDS ASM ret                \
    name(REG(a0, struct Hook *hook), REG(a2, UNUSED APTR obj),               \
    REG(a1, param))
  #define HOOKPROTONP(name, ret, obj) static SAVEDS ASM ret                  \
    name(REG(a0, struct Hook *hook), REG(a2, obj),                           \
    REG(a1, UNUSED APTR param))
  #define HOOKPROTONONP(name, ret) static SAVEDS ASM ret                     \
    name(REG(a0, struct Hook *hook), REG(a2, UNUSED APTR obj),               \
    REG(a1, UNUSED APTR param))
  #define HOOKPROTONH(name, ret, obj, param) static SAVEDS ASM ret           \
    name(REG(a0, UNUSED struct Hook *hook), REG(a2, obj), REG(a1, param))
  #define HOOKPROTONHNO(name, ret, param) static SAVEDS ASM ret              \
    name(REG(a0, UNUSED struct Hook *hook), REG(a2, UNUSED APTR obj),        \
    REG(a1, param))
  #define HOOKPROTONHNP(name, ret, obj) static SAVEDS ASM ret                \
    name(REG(a0, UNUSED struct Hook *hook), REG(a2, obj),                    \
    REG(a1, UNUSED APTR param))
  #define HOOKPROTONHNONP(name, ret) static SAVEDS ret name(void)
  #else
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
  #endif
#endif

#if defined(__MORPHOS__)
  #include <proto/alib.h>

  #define SDI_TRAP_LIB 0xFF00 /* SDI prefix to reduce conflicts */

  struct SDI_EmulLibEntry
  {
    UWORD Trap;
    UWORD pad;
    APTR  Func;
  };

  #define MakeHook(hookname, funcname) struct Hook hookname = {{NULL, NULL}, \
    (HOOKFUNC)HookEntry, (HOOKFUNC)funcname, NULL}
  #define MakeHookWithData(hookname, funcname, data) struct Hook hookname =  \
    {{NULL, NULL}, (HOOKFUNC)HookEntry, (HOOKFUNC)funcname, (APTR)data}
  #define MakeStaticHook(hookname, funcname) static struct Hook hookname =   \
    {{NULL, NULL}, (HOOKFUNC)HookEntry, (HOOKFUNC)funcname, NULL}

  #if defined(__GNUC__)
    #define HOOKPROTO(name, ret, obj, param) static SAVEDS ASM ret           \
      name(REG(a0, struct Hook *hook), REG(a2, obj), REG(a1, param))
    #define HOOKPROTONO(name, ret, param) static SAVEDS ASM ret              \
      name(REG(a0, struct Hook *hook), REG(a2, UNUSED APTR obj),             \
      REG(a1, param))
    #define HOOKPROTONP(name, ret, obj) static SAVEDS ASM ret                \
      name(REG(a0, struct Hook *hook), REG(a2, obj),                         \
      REG(a1, UNUSED APTR param))
    #define HOOKPROTONONP(name, ret) static SAVEDS ASM ret                   \
      name(REG(a0, struct Hook *hook), REG(a2, UNUSED APTR obj),             \
      REG(a1, UNUSED APTR param))
    #define HOOKPROTONH(name, ret, obj, param) static SAVEDS ASM ret         \
      name(REG(a0, UNUSED struct Hook *hook), REG(a2, obj), REG(a1, param))
    #define HOOKPROTONHNO(name, ret, param) static SAVEDS ASM ret            \
      name(REG(a0, UNUSED struct Hook *hook), REG(a2, UNUSED APTR obj),      \
      REG(a1, param))
    #define HOOKPROTONHNP(name, ret, obj) static SAVEDS ASM ret              \
      name(REG(a0, UNUSED struct Hook *hook), REG(a2, obj),                  \
      REG(a1, UNUSED APTR param))
    #define HOOKPROTONHNONP(name, ret) static SAVEDS ret name(void)
    #define DISPATCHERPROTO(name)                                            \
      struct IClass;                                                         \
      ULONG name(struct IClass * cl, Object * obj, Msg msg);                 \
      static ULONG Trampoline_##name(void) {return name((struct IClass *)    \
      REG_A0, (Object *) REG_A2, (Msg) REG_A1);}                             \
      static const struct SDI_EmulLibEntry Gate_##name = {SDI_TRAP_LIB, 0,   \
      (void(*)())Trampoline_##name};                                         \
      ULONG name(struct IClass * cl, Object * obj, Msg msg)
  #else
    #define DISPATCHERPROTO(name)                                            \
      struct IClass;                                                         \
      ASM ULONG  name(REG(a0,                                                \
      struct IClass * cl), REG(a2, Object * obj), REG(a1, Msg msg));         \
      static const struct SDI_EmulLibEntry Gate_##name = {SDI_TRAP_LIB, 0,   \
      (APTR)name};                                                           \
      ASM ULONG  name(REG(a0,                                                \
      struct IClass * cl), REG(a2, Object * obj), REG(a1, Msg msg))
  #endif

  #define ENTRY(func) (APTR)&Gate_##func
#else
  #define DISPATCHERPROTO(name) SAVEDS ASM ULONG  name(REG(a0,               \
    struct IClass * cl), REG(a2, Object * obj), REG(a1, Msg msg))
  #define ENTRY(func) (APTR)func
  #define MakeHook(hookname, funcname) struct Hook hookname = {{NULL, NULL}, \
    (HOOKFUNC)funcname, NULL, NULL}
  #define MakeHookWithData(hookname, funcname, data) struct Hook hookname =  \
    {{NULL, NULL}, (HOOKFUNC)funcname, NULL, (APTR)data}
  #define MakeStaticHook(hookname, funcname) static struct Hook hookname =   \
    {{NULL, NULL}, (HOOKFUNC)funcname, NULL, NULL}
#endif

#define InitHook(hook, orighook, data) ((hook)->h_Entry = (orighook).h_Entry,\
  (hook)->h_SubEntry = (orighook).h_SubEntry,(hook)->h_Data = (APTR)(data))

#endif /* SDI_HOOK_H */
