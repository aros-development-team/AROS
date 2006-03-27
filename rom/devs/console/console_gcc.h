/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

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
#define COTASK_STACKSIZE (AROS_STACKSIZE + 4)
#define COTASK_PRIORITY  10


/* Minimum x & y char positions */

#define DEF_CHAR_XMIN 0
#define DEF_CHAR_YMIN 0


#define CHAR_XMIN(o) DEF_CHAR_XMIN
#define CHAR_YMIN(o) DEF_CHAR_YMIN

#define CHAR_XMAX(o) (CU(o)->cu_XMax)
#define CHAR_YMAX(o) (CU(o)->cu_YMax)


#define XCP (CU(o)->cu_XCP) /* Character X pos */
#define YCP (CU(o)->cu_YCP) /* Character Y pos */

#define XCCP (CU(o)->cu_XCCP) /* Cursor X pos */
#define YCCP (CU(o)->cu_YCCP) /* Cusror Y pos */

#define XRSIZE (CU(o)->cu_XRSize)
#define YRSIZE (CU(o)->cu_YRSize)

#define CP_X(o) (GFX_X(o, CU(o)->cu_XCCP))
#define CP_Y(o) (GFX_Y(o, CU(o)->cu_YCCP))

/* Macros that convert from char to gfx coords */

#define GFX_X(o, x) (CU(o)->cu_XROrigin + ((x) * CU(o)->cu_XRSize))
#define GFX_Y(o, y) (CU(o)->cu_YROrigin + ((y) * CU(o)->cu_YRSize))

#define GFX_XMIN(o) (GFX_X((o), CHAR_XMIN(o)))
#define GFX_YMIN(o) (GFX_Y((o), CHAR_YMIN(o)))

#define GFX_XMAX(o) ((GFX_X((o), CHAR_XMAX(o) + 1)) - 1)
#define GFX_YMAX(o) ((GFX_Y((o), CHAR_YMAX(o) + 1)) - 1)

/* Macros to set/reset/check rawevents */

#define SET_RAWEVENT(o, which) (CU(o)->cu_RawEvents[(which) / 8] |= (1 << ((which) & 7)))
#define RESET_RAWEVENT(o, which) (CU(o)->cu_RawEvents[(which) / 8] &= ~(1 << ((which) & 7)))
#define CHECK_RAWEVENT(o, which) (CU(o)->cu_RawEvents[(which) / 8] & (1 << ((which) & 7)))

#define SET_MODE(o, which) (CU(o)->cu_Modes[(which) / 8] |= (1 << ((which) & 7)))
#define CLEAR_MODE(o, which) (CU(o)->cu_Modes[(which) / 8] &= ~(1 << ((which) & 7)))
#define CHECK_MODE(o, which) (CU(o)->cu_Modes[(which) / 8] & (1 << ((which) & 7)))

#define CONSOLECLASSPTR		(ConsoleDevice->consoleClass)
#define STDCONCLASSPTR		(ConsoleDevice->stdConClass)
#define CHARMAPCLASSPTR		(ConsoleDevice->charMapClass)
#define SNIPMAPCLASSPTR		(ConsoleDevice->snipMapClass)

#define CU(x) ((struct ConUnit *)x)

#define ICU(x) ((struct intConUnit *)x)

#define WINDOW(o)	CU(o)->cu_Window 
#define RASTPORT(o)	WINDOW(o)->RPort


#define MAX(a, b) ((a) > (b) ? a : b)
#define MIN(a, b) ((a) < (b) ? a : b)

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
    C_RESET_LF_MODE,
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

    C_SELECT_GRAPHIC_RENDITION,
    
    C_CURSOR_VISIBLE,
    C_CURSOR_INVISIBLE,
    
    C_SET_RAWEVENTS,
    C_RESET_RAWEVENTS,
    
    C_SET_AUTOWRAP_MODE,
    C_RESET_AUTOWRAP_MODE,
    C_SET_AUTOSCROLL_MODE,
    C_RESET_AUTOSCROLL_MODE,
    C_SET_PAGE_LENGTH,
    C_SET_LINE_LENGTH,
    C_SET_LEFT_OFFSET,
    C_SET_TOP_OFFSET,

    C_ASCII_STRING,
    
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

    /* Buffer where characters received from the console input handler
       will be stored
    */
    UBYTE inputBuf[CON_INPUTBUF_SIZE];
    /* Number of charcters currently stored in the buffer */
    ULONG numStoredChars;
        
};

/* The conFlags */
#define CF_DELAYEDDISPOSE	(1L << 0)
#define CF_DISPOSE		(1L << 1)

#if 0
/* Determining whether linefeed (LF==LF+CR) mode is on */
#define CF_LF_MODE_ON		(1L << 2)
#endif

struct cdihMessage
{
    struct Message msg;
    /* The unit that the user input should go to */
    Object	*unit;

    struct InputEvent ie;
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

VOID consoleTaskEntry(struct ConsoleBase *ConsoleDevice);

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
ULONG writeToConsole(struct ConUnit *unit, STRPTR buf, ULONG towrite
	, struct ConsoleBase *ConsoleDevice);

Class *makeConsoleClass(struct ConsoleBase *ConsoleDevice);
Class *makeStdConClass(struct ConsoleBase *ConsoleDevice);


VOID printstring(STRPTR string, ULONG len, struct ConsoleBase *ConsoleDevice);

struct ConsoleBase
{
    struct Device device;
    struct ExecBase * sysBase;
    BPTR seglist;
    
    struct MinList unitList;
    struct SignalSemaphore unitListLock;
    struct SignalSemaphore consoleTaskLock;
    
    struct Interrupt	*inputHandler;
    struct Task		*consoleTask;
    struct MsgPort	 commandPort;
    
    /* Queued read requests */
    struct MinList	readRequests;
    
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


#undef CB
#define CB(x) ((struct ConsoleBase *)x)

#endif /* CONSOLE_GCC_H */

