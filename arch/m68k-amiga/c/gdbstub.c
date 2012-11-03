/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: m68k-amiga gdb stub
    Lang: english
 */
/* Adapted from public domain code from:
 * ftp://ftp.jyu.fi/pub/PalmOS/ryeham/ALPHA/m68k-gdbstub.c
 *
 *  vvvvvvv The below are the original comments from m68k-gdbstub.c vvvvvvv
 */
/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $                   
 *
 *  Module name: remcom.c $  
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *  
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $ 
 *
 *  NOTES:           See Below $
 * 
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing a trap #1.  The breakpoint instruction
 *  is hardwired to trap #1 because not to do so is a compatibility problem--
 *  there either should be a standard breakpoint instruction, or the protocol
 *  should be extended to provide some means to communicate which breakpoint
 *  instruction is in use (or have the stub insert the breakpoint).
 *  
 *  Some explanation is probably necessary to explain how exceptions are
 *  handled.  When an exception is encountered the 68000 pushes the current
 *  program counter and status register onto the supervisor stack and then
 *  transfers execution to a location specified in it's vector table.
 *  The handlers for the exception vectors are hardwired to jmp to an address
 *  given by the relation:  (exception - 256) * 6.  These are decending 
 *  addresses starting from -6, -12, -18, ...  By allowing 6 bytes for
 *  each entry, a jsr, jmp, bsr, ... can be used to enter the exception 
 *  handler.  Using a jsr to handle an exception has an added benefit of
 *  allowing a single handler to service several exceptions and use the
 *  return address as the key differentiation.  The vector number can be
 *  computed from the return address by [ exception = (addr + 1530) / 6 ].
 *  The sole purpose of the routine catchException is to compute the
 *  exception number and push it on the stack in place of the return address.
 *  The external function exceptionHandler() is
 *  used to attach a specific handler to a specific m68k exception.
 *  For 68020 machines, the ability to have a return address around just
 *  so the vector can be determined is not necessary because the '020 pushes an
 *  extra word onto the stack containing the vector offset
 * 
 *  Because gdb will sometimes write to the stack area to execute function
 *  calls, this program cannot rely on using the supervisor stack so it
 *  uses it's own stack area reserved in the int array remcomStack.  
 * 
 *************
 *
 *    The following gdb commands are supported:
 * 
 * command          function                               Return value
 * 
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 * 
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 * 
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 * 
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 * 
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 * 
 * All commands and responses are sent with a packet which includes a 
 * checksum.  A packet consists of 
 * 
 * $<packet info>#<checksum>.
 * 
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 * 
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 * 
 * Example:
 * 
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 * 
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

/************************************************************************
 *
 * external low-level support routines 
 */
typedef void (*ExceptionHook)(int);   /* pointer to function with int parm */
typedef void (*Function)();           /* pointer to a function */

extern int DebugPutChar(register int x);   /* write a single character      */
extern int DebugGetChar();   /* read and return a single char */

void exceptionHandler(int n, ExceptionHook a);  /* assign an exception handler */

/************************/
/* FORWARD DECLARATIONS */
/************************/

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 400

static char initialized;  /* boolean flag. != 0 means we've been initialized */

int     remote_debug;
/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */ 

static const char hexchars[]="0123456789abcdef";

/* there are 180 bytes of registers on a 68020 w/68881      */
/* many of the fpa registers are 12 byte (96 bit) registers */
#define NUMREGBYTES 180
enum regnames {D0,D1,D2,D3,D4,D5,D6,D7, 
               A0,A1,A2,A3,A4,A5,A6,A7, 
               PS,PC,
               FP0,FP1,FP2,FP3,FP4,FP5,FP6,FP7,
               FPCONTROL,FPSTATUS,FPIADDR
              };

/*
 * these should not be static cuz they can be used outside this module
 */
int registers[NUMREGBYTES/4];
int superStack;

#define STACKSIZE 1024
int remcomStack[STACKSIZE/sizeof(int)];

#ifdef mc68020
/* the size of the exception stack on the 68020 varies with the type of
 * exception.  The following table is the number of WORDS used
 * for each exception format.
 */
const short exceptionSize[] = { 4,4,6,4,4,4,4,4,29,10,16,46,12,4,4,4 };
#endif

#ifdef mc68332
const short exceptionSize[] = { 4,4,6,4,4,4,4,4,4,4,4,4,16,4,4,4 };
#endif

/************* jump buffer used for setjmp/longjmp **************************/
jmp_buf remcomEnv;

/***************************  ASSEMBLY CODE MACROS *************************/
/* 									   */

#define BREAKPOINT() asm("   trap #1");

int hex(ch)
char ch;
{
  if ((ch >= 'a') && (ch <= 'f')) return (ch-'a'+10);
  if ((ch >= '0') && (ch <= '9')) return (ch-'0');
  if ((ch >= 'A') && (ch <= 'F')) return (ch-'A'+10);
  return (-1);
}


/* scan for the sequence $<data>#<checksum>     */
void getpacket(buffer)
char * buffer;
{
  unsigned char checksum;
  unsigned char xmitcsum;
  int  i;
  int  count;
  char ch;
  
  do {
    /* wait around for the start character, ignore all other characters */
    while ((ch = (DebugGetChar() & 0x7f)) != '$'); 
    checksum = 0;
    xmitcsum = -1;
    
    count = 0;
    
    /* now, read until a # or end of buffer is found */
    while (count < BUFMAX) {
      ch = DebugGetChar() & 0x7f;
      if (ch == '#') break;
      checksum = checksum + ch;
      buffer[count] = ch;
      count = count + 1;
      }
    buffer[count] = 0;

    if (ch == '#') {
      xmitcsum = hex(DebugGetChar() & 0x7f) << 4;
      xmitcsum += hex(DebugGetChar() & 0x7f);
#ifdef HAVE_STDIO
      if ((remote_debug ) && (checksum != xmitcsum)) {
        fprintf (stderr,"bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
						     checksum,xmitcsum,buffer);
      }
#endif
      
      if (checksum != xmitcsum) DebugPutChar('-');  /* failed checksum */ 
      else {
	 DebugPutChar('+');  /* successful transfer */
	 /* if a sequence char is present, reply the sequence ID */
	 if (buffer[2] == ':') {
	    DebugPutChar( buffer[0] );
	    DebugPutChar( buffer[1] );
	    /* remove sequence chars from buffer */
	    count = strlen(buffer);
	    for (i=3; i <= count; i++) buffer[i-3] = buffer[i];
	 } 
      } 
    } 
  } while (checksum != xmitcsum);
  
}

/* send the packet in buffer.  The host get's one chance to read it.  
   This routine does not wait for a positive acknowledge.  */


void putpacket(buffer)
char * buffer;
{
  unsigned char checksum;
  int  count;
  char ch;
  
  /*  $<packet info>#<checksum>. */
  do {
  DebugPutChar('$');
  checksum = 0;
  count    = 0;
  
  while ((ch=buffer[count]) != 0) {
    if (! DebugPutChar(ch)) return;
    checksum += ch;
    count += 1;
  }
  
  DebugPutChar('#');
  DebugPutChar(hexchars[checksum >> 4]);
  DebugPutChar(hexchars[checksum % 16]);

  } while (DebugGetChar() != '+');
  
}

char  remcomInBuffer[BUFMAX];
char  remcomOutBuffer[BUFMAX];
static short error;


void debug_error(format, parm)
char * format;
char * parm;
{
#ifdef HAVE_STDIO
  if (remote_debug) fprintf (stderr,format,parm);
#endif
}

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
char* mem2hex(mem, buf, count)
char* mem;
char* buf;
int   count;
{
      int i;
      unsigned char ch;
      for (i=0;i<count;i++) {
          ch = *mem++;
          *buf++ = hexchars[ch >> 4];
          *buf++ = hexchars[ch % 16];
      }
      *buf = 0; 
      return(buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
char* hex2mem(buf, mem, count)
char* buf;
char* mem;
int   count;
{
      int i;
      unsigned char ch;
      for (i=0;i<count;i++) {
          ch = hex(*buf++) << 4;
          ch = ch + hex(*buf++);
          *mem++ = ch;
      }
      return(mem);
}

/* a bus error has occurred, perform a longjmp
   to return execution and allow handling of the error */

void handle_buserror()
{
  longjmp(remcomEnv,1);
}

/* this function takes the 68000 exception number and attempts to 
   translate this number into a unix compatible signal value */
int computeSignal( exceptionVector )
int exceptionVector;
{
  int sigval;
  switch (exceptionVector) {
    case 2 : sigval = 10; break; /* bus error           */
    case 3 : sigval = 10; break; /* address error       */
    case 4 : sigval = 4;  break; /* illegal instruction */
    case 5 : sigval = 8;  break; /* zero divide         */
    case 6 : sigval = 8; break; /* chk instruction     */
    case 7 : sigval = 8; break; /* trapv instruction   */
    case 8 : sigval = 11; break; /* privilege violation */
    case 9 : sigval = 5;  break; /* trace trap          */
    case 10: sigval = 4;  break; /* line 1010 emulator  */
    case 11: sigval = 4;  break; /* line 1111 emulator  */

      /* Coprocessor protocol violation.  Using a standard MMU or FPU
	 this cannot be triggered by software.  Call it a SIGBUS.  */
    case 13: sigval = 10;  break;

    case 31: sigval = 2;  break; /* interrupt           */
    case 32+1: sigval = 5;  break; /* trap #1 breakpoint  */
    case 32+15: sigval = 5;  break; /* trap #15 breakpoint  */

    case 48: sigval = 8;  break; /* floating point err  */
    case 49: sigval = 8;  break; /* floating point err  */
    case 50: sigval = 8;  break; /* zero divide         */
    case 51: sigval = 8;  break; /* underflow           */
    case 52: sigval = 8;  break; /* operand error       */
    case 53: sigval = 8;  break; /* overflow            */
    case 54: sigval = 8;  break; /* NAN                 */
    default: 
      sigval = 7;         /* "software generated"*/
  }
  return (sigval);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
int hexToInt(char **ptr, int *intValue)
{
    int numChars = 0;
    int hexValue;
    
    *intValue = 0;

    while (**ptr)
    {
        hexValue = hex(**ptr);
        if (hexValue >=0)
        {
            *intValue = (*intValue <<4) | hexValue;
            numChars ++;
        }
        else
            break;
        
        (*ptr)++;
    }

    return (numChars);
}

/*
 * This function does all command procesing for interfacing to gdb.
 */
void handle_exception(int exceptionVector)
{
  int    sigval;
  int    addr, length;
  char * ptr;

  if ((exceptionVector == (32+1)) ||
      (exceptionVector == (32+15)))
      registers[ PC ] -= 2;

#ifdef HAVE_STDIO
  if (remote_debug) printf("vector=%d, sr=0x%x, pc=0x%x\n", 
			    exceptionVector,
			    registers[ PS ], 
			    registers[ PC ]);
#endif
  /* reply to host that an exception has occurred */
  sigval = computeSignal( exceptionVector );
  remcomOutBuffer[0] = 'S';
  remcomOutBuffer[1] =  hexchars[sigval >> 4];
  remcomOutBuffer[2] =  hexchars[sigval % 16];
  remcomOutBuffer[3] = 0;

  putpacket(remcomOutBuffer); 

  while (1==1) { 
    error = 0;
    remcomOutBuffer[0] = 0;
    getpacket(remcomInBuffer);
    switch (remcomInBuffer[0]) {
      case '?' :   remcomOutBuffer[0] = 'S';
                   remcomOutBuffer[1] =  hexchars[sigval >> 4];
                   remcomOutBuffer[2] =  hexchars[sigval % 16];
                   remcomOutBuffer[3] = 0;
                 break; 
      case 'd' : remote_debug = !(remote_debug);  /* toggle debug flag */
                 break; 
      case 'g' : /* return the value of the CPU registers */
                mem2hex((char*) registers, remcomOutBuffer, NUMREGBYTES);
                break;
      case 'G' : /* set the value of the CPU registers - return OK */
                hex2mem(&remcomInBuffer[1], (char*) registers, NUMREGBYTES);
                strcpy(remcomOutBuffer,"OK");
                break;
      
      /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
      case 'm' : 
	        if (setjmp(remcomEnv) == 0)
                {
                    exceptionHandler(2,handle_buserror); 

		    /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
                    ptr = &remcomInBuffer[1];
                    if (hexToInt(&ptr,&addr))
                        if (*(ptr++) == ',')
                            if (hexToInt(&ptr,&length)) 
                            {
                                ptr = 0;
                                mem2hex((char*) addr, remcomOutBuffer, length);
                            }

                    if (ptr)
                    {
		      strcpy(remcomOutBuffer,"E01");
		      debug_error("malformed read memory command: %s",remcomInBuffer);
                  }     
                } 
		else {
		  exceptionHandler(2,handle_exception);   
		  strcpy(remcomOutBuffer,"E03");
		  debug_error("bus error");
		  }     
                
		/* restore handler for bus error */
		exceptionHandler(2,handle_exception);   
		break;
      
      /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
      case 'M' : 
	        if (setjmp(remcomEnv) == 0) {
		    exceptionHandler(2,handle_buserror); 
                    
		    /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
                    ptr = &remcomInBuffer[1];
                    if (hexToInt(&ptr,&addr))
                        if (*(ptr++) == ',')
                            if (hexToInt(&ptr,&length))
                                if (*(ptr++) == ':')
                                {
                                    hex2mem(ptr, (char*) addr, length);
                                    ptr = 0;
                                    strcpy(remcomOutBuffer,"OK");
                                }
                    if (ptr)
                    {
		      strcpy(remcomOutBuffer,"E02");
		      debug_error("malformed write memory command: %s",remcomInBuffer);
		      }     
                } 
		else {
		  exceptionHandler(2,handle_exception);   
		  strcpy(remcomOutBuffer,"E03");
		  debug_error("bus error");
		  }     

                /* restore handler for bus error */
                exceptionHandler(2,handle_exception);   
                break;

     /* cAA..AA    Continue at address AA..AA(optional) */
     /* sAA..AA   Step one instruction from AA..AA(optional) */
     case 'c' : 
     case 's' : 
          /* try to read optional parameter, pc unchanged if no parm */
         ptr = &remcomInBuffer[1];
         if (hexToInt(&ptr,&addr))
             registers[ PC ] = addr;
             
         /* clear the trace bit */
         registers[ PS ] &= 0x7fff;
          
         /* set the trace bit if we're stepping */
         if (remcomInBuffer[0] == 's') registers[ PS ] |= 0x8000;

         /* Don't send a reply. The trap will signal GDB. */
         return;
          
      /* kill the program */
      case 'k' :  /* do nothing */
                break;
      } /* switch */ 
    
    /* reply to the request */
    putpacket(remcomOutBuffer); 
    }
}

/* this function is used to set up exception handlers for tracing and 
   breakpoints */
void set_debug_traps()
{
  initialized = 1;

  DebugPutChar('\n');
  DebugPutChar('\r');
  DebugPutChar('g');
  DebugPutChar('d');
  DebugPutChar('b');
  DebugPutChar(' ');
  DebugPutChar('s');
  DebugPutChar('t');
  DebugPutChar('u');
  DebugPutChar('b');
}

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */
   
void breakpoint()
{
  if (initialized) BREAKPOINT();
}

void gdbstub()
{
  set_debug_traps();
}

/**************** end of gdbstub.c, rest is AROS interface ***********/

#include <aros/detach.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/execbase.h>

/* write a single character      */
int DebugPutChar(register int x)
{
    RawPutChar(x);

    return 1;
}

/* read and return a single char */
int DebugGetChar()
{
    LONG c;
    while ((c = RawMayGetChar()) < 0);

    return (int)c;
}

static ExceptionHook exceptionTable[256];

void exceptionHandler(int n, ExceptionHook a)
{
    exceptionTable[n] = a;
}

union M68K_Exception_Frame {
    struct {
	UWORD status;
	ULONG access;
	UWORD sr;
	ULONG pc;
    } m68000_bus;
    struct {
	UWORD sr;
	ULONG pc;
    } m68000;
    struct {
	UWORD sr;
	ULONG pc;
	UWORD vector;
	UWORD data[0];
    } m68010;
};

void trapHandler_(union M68K_Exception_Frame *frame, ULONG id)
{
    if (SysBase->AttnFlags & AFF_68010) {
    	/* M68010+, any trap */
    	registers[PS] = frame->m68010.sr;
    	registers[PC] = frame->m68010.pc;
    } else if (id == 2 || id == 3) {
    	/* M68000 Bus/Address trap */
    	registers[PS] = frame->m68000_bus.sr;
    	registers[PC] = frame->m68000_bus.pc;
    } else {
    	/* M68000 other traps */
    	registers[PS] = frame->m68000.sr;
    	registers[PC] = frame->m68000.pc;
    }

    /* TODO: Save FPU state */

    if (id > 256 || exceptionTable[id] == NULL) {
    	handle_exception(id);
    } else {
    	(exceptionTable[id])(id);
    }

    /* Restore registers */
    if (SysBase->AttnFlags & AFF_68010) {
    	/* M68010+, any trap */
    	frame->m68010.sr = registers[PS];
    	frame->m68010.pc = registers[PC];
    } else if (id == 2 || id == 3) {
    	/* M68000 Bus/Address trap */
    	frame->m68000_bus.sr = registers[PS];
    	frame->m68000_bus.pc = registers[PC];
    } else {
    	/* M68000 other traps */
    	frame->m68000.sr = registers[PS];
    	frame->m68000.pc = registers[PC];
    }
}

/* Wrapper for the trapHandler_ */
extern void trapHandler(void);
asm (
	"	.text\n"
	"	.global trapHandler\n"
	"trapHandler:\n"
	"	oriw    #0x0700,%sr\n"      /* Disable interrupts */
	"	movem.l %d0-%d7/%a0-%a6,registers\n"
	"	move.l  %usp,%a0\n"        /* registers[A7] = USP */
	"	lea.l   registers,%a1\n"
	"	move.l  %a0,%a1@(15*4)\n"
	"	lea.l   %sp@(+4),%a0\n"    /* A0 = exception frame */
	"	move.l  %a0,%sp@-\n"       /* Stack = Frame *, ID */
	"	jsr     trapHandler_\n"    /* Call C routine */
	"	addq    #8,%sp\n"          /* Pop off stack args */
	"	lea.l   registers,%a1\n"
	"	move.l  %a1@(15*4),%a0\n"
	"	move.l  %a0,%usp\n"        /* Save new USP */
	"	movem.l registers,%d0-%d7/%a0-%a6\n"	/* Restore regs */
	"	rte\n"                     /* Done! */
);

static APTR oldAlert;
AROS_UFH2(void, myAlert,
	AROS_UFHA(ULONG, alertNum, D7),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    breakpoint();

    AROS_USERFUNC_EXIT
}

static APTR oldAddTask;
AROS_UFH4(APTR, myAddTask,
	AROS_UFHA(struct Task *,     task,      A1),
	AROS_UFHA(APTR,              initialPC, A2),
	AROS_UFHA(APTR,              finalPC,   A3),
	AROS_UFHA(struct ExecBase *, SysBase,   A6))
{
    AROS_USERFUNC_INIT

    APTR ret;

    ret = AROS_UFC4(APTR, oldAddTask,
	AROS_UFCA(struct Task *,     task,      A1),
	AROS_UFCA(APTR,              initialPC, A2),
	AROS_UFCA(APTR,              finalPC,   A3),
	AROS_UFCA(struct ExecBase *, SysBase,   A6));

    /* Set us up as the trap handler */
    task->tc_TrapCode = trapHandler;

    return ret;

    AROS_USERFUNC_EXIT
}

static APTR UpdateTrapCode(APTR newHandler)
{
    APTR oldHandler;
    struct Task *task;

    /* Update SysBase default */
    oldHandler = SysBase->TaskTrapCode;
    SysBase->TaskTrapCode = newHandler;

    /* And ourselves. */
    task = FindTask(NULL);
    task->tc_TrapCode = newHandler;

    /* And any other tasks in the system.
     */
    ForeachNode(&SysBase->TaskReady, task) {
	if (task->tc_TrapCode == oldHandler)
	    task->tc_TrapCode = newHandler;
    }
    ForeachNode(&SysBase->TaskWait, task) {
	if (task->tc_TrapCode == oldHandler)
	    task->tc_TrapCode = newHandler;
    }

    return oldHandler;
}

int main(int argc, char **argv)
{
    APTR DOSBase;
    APTR oldTaskTrapCode;

    if ((DOSBase = OpenLibrary("dos.library",36))) {
    	const char msg[] = "GDB trapping enabled on the serial port\n";
    	/* We need to patch AddTask() to set
    	 * us up as the default stub for tc_TrapCode.
    	 *
    	 * Although this is not needed (yet) on AROS,
    	 * the Shell or Workbench may set tc_TrapCode
    	 * before AddTask() on AOS.
    	 *
    	 * So instead of modifying SysBase->TrapCode,
    	 * we have to inject this into the Exec Library.
    	 */
    	/* Set up new handler
    	 */
    	Disable();
    	*(UWORD *)0 = 0x4e41;
    	oldTaskTrapCode = UpdateTrapCode(trapHandler);
    	oldAddTask  = SetFunction((struct Library *)SysBase, -47 * LIB_VECTSIZE, myAddTask);
    	/* Patch Alert() to generate a breakpoint */
    	oldAlert = SetFunction((struct Library *)SysBase, -18 * LIB_VECTSIZE, myAlert);
    	Enable();

    	gdbstub();
    	Write(Output(), msg, sizeof(msg)-1);
    	Detach();
    	Wait(SIGBREAKF_CTRL_C);

    	/* Restore traps. Not really safe, but better than nothing
    	 */
    	Disable();
    	SetFunction((struct Library *)SysBase, -47 * LIB_VECTSIZE, oldAddTask);
    	SetFunction((struct Library *)SysBase, -18 * LIB_VECTSIZE, oldAlert);
    	UpdateTrapCode(oldTaskTrapCode);
    	Enable();

    	CloseLibrary(DOSBase);
    } else {
    	return RETURN_ERROR;
    }
    return RETURN_OK;
}
