#ifndef SDI_INTERRUPT_H
#define SDI_INTERRUPT_H

/* Includeheader

        Name:           SDI_interrupt.h
        Versionstring:  $VER: SDI_interrupt.h 1.1 (25.04.2006)
        Author:         Guido Mersmann
        Distribution:   PD
        Project page:   http://www.sf.net/projects/sditools/
        Description:    defines to hide compiler specific interrupt and
                        handler stuff

 1.0   17.05.05 : inspired by the SDI_#?.h files made by Jens Langner
                  and Dirk Stöcker I created files to handle interrupt
                  and handler functions in an API compatible way.
 1.1   25.04.06 : fixed MakeInterrupt() and MakeHandler() macro. (geit)

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
** The INTERRUPTPROTO macro is for creating interrupts functions and the
** HANDLERPROTO macro is for handler type setup like used by the
** input.device.
**
** The usage is simular to the DISPATCHERPROTO macro provided by SDI_hook.h.
**
** It gets the function name as argument. To supply this function for use by
** an interrupt structure or handler argument, use the ENTRY macro, which also
** gets the function name as argument. There is also a MakeInterrupt and a
** MakeHandler macro for easy structure setup.
**
** Example:
**
** We create a handler function for the input.device.
**
** HANDLERPROTO( handlerfunc, ULONG, struct InputEvent *inputevent, APTR userdata)
** {
** ... Modify/parse input stream here
** ...
** return( (ULONG) inputevent );
** }
** MakeHandler( handlerstruct, handlerfunc, "TestHandler", &our_user_data);
**
** As you can see usage is as simple as using SDI hooks. To create interrupt
** use INTERRUPTPROTO and MakeInterrupt. Internally both macros are identically.
**
** There are also functions to create more specific interrupt and handler
** structures. By default type is NT_INTERRUPT and priority is 0.
**
** MakeHandlerType     - additional argument for type
** MakeHandlerPri      - additional argument for pri
** MakeHandlerTypePri  - additional arguments for type and pri
**
**
** Notes: Since the interrupt structure is used for handlers and also foreign
**        handlers are using this structure I keeped the arguments definition
**        on the user side.
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

  #define INTERRUPTPROTO(name, ret, obj, data)                               \
    SAVEDS ASM ret name( obj, data);                                         \
    static ret Trampoline_##name(void) {return name(( obj) REG_A0,           \
    (data) REG_A1);}                                                         \
    static const struct SDI_EmulLibEntry Gate_##name = {SDI_TRAP_LIB, 0,     \
    (APTR) Trampoline_##name};                                               \
    SAVEDS ASM ret name( obj, data)

  #define HANDLERPROTO(name, ret, obj, data)                                 \
    SAVEDS ASM ret name( obj, data);                                         \
    static ret Trampoline_##name(void) {return name(( obj) REG_A0,           \
    (data) REG_A1);}                                                         \
    static const struct SDI_EmulLibEntry Gate_##name = {SDI_TRAP_LIB, 0,     \
    (APTR) Trampoline_##name};                                               \
    SAVEDS ASM ret name( obj, data)

  #define ENTRY(func) (APTR)&Gate_##func

#elif defined(__AROS__)

  /* This is the same prototype for both Cause() and
   * AddIntServer() functions
   */
  #define INTERRUPTPROTO(name, ret, obj, data) \
      ret name(obj, data, struct ExecBase *SysBase); \
      AROS_UFH5(int, name##_wrapper, \
          AROS_UFHA(APTR, is_Data, A1), \
          AROS_UFHA(APTR, is_Code, A5), \
          AROS_UFHA(struct ExecBase *, sysBase, A6), \
          AROS_UFHA(APTR, mask, D1), \
          AROS_UFHA(APTR, custom, A0)) \
      { AROS_USERFUNC_INIT \
        name(custom, is_Data, sysBase ); return 0; \
        AROS_USERFUNC_EXIT }  \
      ret name(obj, data, struct ExecBase *SysBase)

#else

  #define INTERRUPTPROTO(name, ret, obj, data)                               \
    SAVEDS ASM ret name(REG(a0, obj), REG(a1, data))
  #define HANDLERPROTO(name, ret, obj, data)                                 \
    SAVEDS ASM ret name(REG(a0, obj), REG(a1, data))

  #define ENTRY(func) (APTR)func

#endif /* __MORPHOS__ */

/* some structure creating macros for easy and more specific usage */

#define MakeInterrupt( name, func, title, isdata ) \
  static struct Interrupt name = {{ NULL, NULL, NT_INTERRUPT, 0, (STRPTR) title}, (APTR) isdata, (void (*)()) ENTRY(func) }

#define MakeInterruptPri( name, func, title, isdata, pri ) \
  static struct Interrupt name = {{ NULL, NULL, NT_INTERRUPT, pri, (STRPTR) title}, (APTR) isdata, (void (*)()) ENTRY(func) }

#define MakeInterruptType( name, func, title, isdata, type ) \
  static struct Interrupt name = {{ NULL, NULL, type, 0, (STRPTR) title}, (APTR) isdata, (void (*)()) ENTRY(func) }

#define MakeInterruptTypePri( name, func, title, isdata, type, pri ) \
  static struct Interrupt name = {{ NULL, NULL, type, pri, (STRPTR) title}, (APTR) isdata, (void (*)()) ENTRY(func) }

#define MakeHandler( name, func, title, isdata ) \
  static struct Interrupt name = {{ NULL, NULL, NT_INTERRUPT, 0, (STRPTR) title}, (APTR) isdata, (void (*)()) ENTRY(func) }

#define MakeHandlerPri( name, func, title, isdata, pri ) \
  static struct Interrupt name = {{ NULL, NULL, NT_INTERRUPT, pri, (STRPTR) title}, (APTR) isdata, (void (*)()) ENTRY(func) }

#define MakeHandlerType( name, func, title, isdata, type ) \
  static struct Interrupt name = {{ NULL, NULL, type, 0, (STRPTR) title}, (APTR) isdata, (void (*)()) ENTRY(func) }

#define MakeHandlerTypePri( name, func, title, isdata, type, pri ) \
  static struct Interrupt name = {{ NULL, NULL, type, pri, (STRPTR) title}, (APTR) isdata, (void (*)()) ENTRY(func) }


#endif /* SDI_INTERRUPT_H */

