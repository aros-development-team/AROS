/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1998/07/31 19:28:23  nlorentz
    A start at the console device

    Revision 1.1  1996/08/23 17:32:23  digulla
    Implementation of the console.device


    Desc:
    Lang:
*/
#ifndef CONSOLE_GCC_H
#define CONSOLE_GCC_H

#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

#ifndef DEVICES_CONUNIT_H
#   include <devices/conunit.h>
#endif

#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif

/* Predeclaration */
struct ConsoleBase;

/* Constants */
#define COTASK_STACKSIZE 8192
#define COTASK_PRIORITY  10


/* Class TagIDs */
struct Coord
{
    WORD *XPtr;
    WORD *YPtr;
};

#define CA_Unit		TAG_USER + 1

#define CM_DoCommand	TAG_USER + 2

#define CONSOLECLASSPTR		(ConsoleDevice->consoleclass)
#define STDCONCLASSPTR		(ConsoleDevice->stdconclass)
#define CHARMAPCLASSPTR		(ConsoleDevice->charmapclass)
#define SNIPMAPCLASSPTR		(ConsoleDevice->snipmapclass)

#define CU(x) ((struct ConUnit *)x)

/* Console write commands */
enum 
{
    C_ASCII = 0,
    
    C_ESC,
    C_BELL,
    C_BACKSPACE,
    C_HTAB,
    C_LINEFEED,
    C_VTAB,
    C_FORMFEED,
    C_CARRIAGE_RETURN,
    C_SHIFT_IN,
    C_SHIFT_OUT,
    C_INDEX,
    C_NEXT_LINE,
    C_H_TAB_SET,
    C_REVERSE_IDX,
    
    C_SET_LF_MODE,
    C_RESET_NEWLINE_MODE,
    C_DEVICE_STATUS_REPORT,
    
    C_INSERT_CHAR,
    C_CURSOR_UP,
    C_CURSOR_DOWN,
    C_CURSOR_FORWARD,
    C_CURSOR_BACKWARD,
    C_CURSOR_NEXT_LINE,
    C_CURSOR_PREV_LINE,
    C_CURSOR_POS,
    C_CURSOR_HTAB,
    C_ERASE_IN_DISPLAY,
    C_ERASE_IN_LINE,
    C_INSERT_LINE,
    C_DELETE_LINE,
    C_DELETE_CHAR,
    C_SCROLL_UP,
    C_SCROLL_DOWN,
    C_CURSOR_TAB_CTRL,
    C_CURSOR_BACKTAB
};


/* structs */
struct CoTaskParams
{
    struct ConsoleBase *ConsoleDevice;
    struct Task	*Caller;
    ULONG Signal;
};



APTR  CreateCharMap(ULONG numchars, struct ConsoleBase *ConsoleDevice);
VOID  FreeCharMap  (APTR map, struct ConsoleBase *ConsoleDevice);
ULONG RewindChars  (APTR map, ULONG num);
ULONG ForwardChars (APTR map, ULONG num);
UBYTE GetChar      (APTR map);
BOOL  NextChar     (APTR map, UBYTE *char_ptr);
BOOL  LastChar     (APTR map, UBYTE *char_ptr);
BOOL  InsertChar   (APTR map, UBYTE c, struct ConsoleBase *ConsoleDevice);

/* Prototypes */
VOID write2console(struct IOStdReq *ioreq, struct ConsoleBase *ConsoleDevice);

Class *makeconsoleclass(struct ConsoleBase *ConsoleDevice);

struct ConsoleBase
{
    struct Device device;
    struct ExecBase * sysBase;
    BPTR seglist;
    struct GfxBase *gfxbase;
    struct IntuitionBase *intuitionbase;
    struct Library *boopsibase;
    struct Library *utilitybase;
    
    Class *consoleclass;
    Class *stdconclass;
    Class *charmapclass;
    Class *snipmapclass;
};

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct GfxBase GraphicsBase;
typedef struct IntuitionBase IntuiBase;

#define expunge() \
__AROS_LC0(BPTR, expunge, struct emulbase *, emulbase, 3, emul_handler)

#ifdef SysBase
#   undef SysBase
#endif
#define SysBase ConsoleDevice->sysBase

#ifdef GfxBase
#   undef GfxBase
#endif
#define GfxBase ConsoleDevice->gfxbase

#ifdef IntuitionBase
#   undef IntuitionBase
#endif
#define IntuitionBase ConsoleDevice->intuitionbase

#ifdef BOOPSIBase
#   undef BOOPSIBase
#endif
#define BOOPSIBase ConsoleDevice->boopsibase

#ifdef UtilityBase
#   undef UtilityBase
#endif
#define UtilityBase ConsoleDevice->utilitybase

#endif /* CONSOLE_GCC_H */

