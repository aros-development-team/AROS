/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  1999/09/20 17:33:09  stegerg
    GFX_XMAX/GFX_YMAX macro fixed

    Revision 1.7  1999/03/18 20:16:11  nlorentz
    Reworked escape sequence parsing. Microemacs does not crash anymore, but it sends some strange undocumented sequences

    Revision 1.6  1998/11/07 10:41:21  nlorentz
    Various small bugfixes

    Revision 1.5  1998/10/20 16:44:17  hkiel
    Amiga Research OS

    Revision 1.4  1998/10/03 12:02:33  nlorentz
    Bugfixes, but still buggy

    Revision 1.3  1998/08/24 13:32:43  nlorentz
    Update, now demowin works again

    Revision 1.2  1998/08/01 17:53:39  nlorentz
    Now able to text to the console

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
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

/* Predeclaration */
struct ConsoleBase;

/* Constants */
#define COTASK_STACKSIZE 8192
#define COTASK_PRIORITY  10


/* Minimum x & y char positions */

#define DEF_CHAR_XMIN 0
#define DEF_CHAR_YMIN 0


#define CHAR_XMIN(o) DEF_CHAR_XMIN
#define CHAR_YMIN(o) DEF_CHAR_YMIN

#define CHAR_XMAX(o) (CU(o)->cu_XMax)
#define CHAR_YMAX(o) (CU(o)->cu_YMax)


#define CP_X(o) (GFX_X(o, CU(o)->cu_XCCP))
#define CP_Y(o) (GFX_Y(o, CU(o)->cu_YCCP))

/* Macros that convert from char to gfx coords */
#define GFX_X(o, x) (CU(o)->cu_XROrigin + ((x) * CU(o)->cu_XRSize))
#define GFX_Y(o, y) (CU(o)->cu_YROrigin + ((y) * CU(o)->cu_YRSize))

#define GFX_XMIN(o) (GFX_X((o), CHAR_XMIN(o)))
#define GFX_YMIN(o) (GFX_Y((o), CHAR_YMIN(o)))

#define GFX_XMAX(o) ((GFX_X((o), CHAR_XMAX(o) + 1)) - 1)
#define GFX_YMAX(o) ((GFX_Y((o), CHAR_YMAX(o) + 1)) - 1)



#define CONSOLECLASSPTR		(ConsoleDevice->consoleClass)
#define STDCONCLASSPTR		(ConsoleDevice->stdConClass)
#define CHARMAPCLASSPTR		(ConsoleDevice->charMapClass)
#define SNIPMAPCLASSPTR		(ConsoleDevice->snipMapClass)

#define CU(x) ((struct ConUnit *)x)

#define ICU(x) ((struct intConUnit *)x)


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
    C_CURSOR_BACKTAB,
    
    C_SELECT_GRAPHIC_RENDATION,
    
    NUM_CONSOLE_COMMANDS
};

/**************
**  structs  **
**************/

struct coTaskParams
{
    struct ConsoleBase *consoleDevice;
    struct Task	*parentTask;
    ULONG initSignal;
};

#define CON_INPUTBUF_SIZE 512
struct intConUnit
{
    struct ConUnit unit;
    ULONG conFlags;

    UBYTE inputBuf[CON_INPUTBUF_SIZE];
        
};

/* The conFlags */
#define CF_DELAYEDDISPOSE	(1L << 0)
#define CF_DISPOSE		(1L << 1)

/* Determining whether linefeed (LF==LF+CR) mode is on */
#define CF_LF_MODE_ON		(1L << 2)

/* Message passed from console input handler to console device task */
#define MSGBUFSIZE 200

struct cdihMessage
{
    struct Message msg;
    
    /* The unit that the user input should go to */
    Object	*unit;
    
    /* The input to pass to the user */
    UBYTE 	inputBuf[MSGBUFSIZE];
    
    /* Number of bytes in the input buffer */
    ULONG	numBytes;
    
};

/* Data passed to the console device input handler */

struct cdihData 
{
    struct ConsoleBase	*consoleDevice;
    /* Port to which we send input to the console device
       from the console device input handler.
    */
    struct MsgPort	*inputPort;
    
    /* The port the console.device task replies to telling
       the console.device input handler that it is finished
       with the output.
    */
    struct MsgPort	*cdihReplyPort;
    
    /* The message struct used to pass user input to the console device */
    struct cdihMessage	*cdihMsg;
    
    
};



/*****************
**  Prototypes  **
*****************/

struct Interrupt *initCDIH(struct ConsoleBase *ConsoleDevice);
VOID cleanupCDIH(struct Interrupt *cdihandler, struct ConsoleBase *ConsoleDevice);

VOID consoleTaskEntry(struct coTaskParams *param);

struct Task *createConsoleTask(APTR taskparams, struct ConsoleBase *ConsoleDevice);

APTR  CreateCharMap(ULONG numchars, struct ConsoleBase *ConsoleDevice);
VOID  FreeCharMap  (APTR map, struct ConsoleBase *ConsoleDevice);
ULONG RewindChars  (APTR map, ULONG num);
ULONG ForwardChars (APTR map, ULONG num);
UBYTE GetChar      (APTR map);
BOOL  NextChar     (APTR map, UBYTE *char_ptr);
BOOL  LastChar     (APTR map, UBYTE *char_ptr);
BOOL  InsertChar   (APTR map, UBYTE c, struct ConsoleBase *ConsoleDevice);

/* Prototypes */
ULONG writeToConsole(struct IOStdReq *ioreq, struct ConsoleBase *ConsoleDevice);

Class *makeConsoleClass(struct ConsoleBase *ConsoleDevice);
Class *makeStdConClass(struct ConsoleBase *ConsoleDevice);

struct ConsoleBase
{
    struct Device device;
    struct ExecBase * sysBase;
    BPTR seglist;
    struct GfxBase *gfxBase;
    struct IntuitionBase *intuitionBase;
    struct Library *boopsiBase;
    struct Library *utilityBase;
    
    struct MinList unitList;
    struct SignalSemaphore unitListLock;
    
    struct Interrupt *inputHandler;
    struct Task *consoleTask;
    
    Class *consoleClass;
    Class *stdConClass;
    Class *charMapClass;
    Class *snipMapClass;
};

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct GfxBase GraphicsBase;
typedef struct IntuitionBase IntuiBase;

#define expunge() \
__AROS_LC0(BPTR, expunge, struct ConsoleBase *, ConsoleDevice, 3, Console)

#ifdef SysBase
#   undef SysBase
#endif
#define SysBase ConsoleDevice->sysBase

#ifdef GfxBase
#   undef GfxBase
#endif
#define GfxBase ConsoleDevice->gfxBase

#ifdef IntuitionBase
#   undef IntuitionBase
#endif
#define IntuitionBase ConsoleDevice->intuitionBase

#ifdef BOOPSIBase
#   undef BOOPSIBase
#endif
#define BOOPSIBase ConsoleDevice->boopsiBase

#ifdef UtilityBase
#   undef UtilityBase
#endif
#define UtilityBase ConsoleDevice->utilityBase

#endif /* CONSOLE_GCC_H */

