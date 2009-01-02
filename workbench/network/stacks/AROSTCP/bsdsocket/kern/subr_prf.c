/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#define USE_INLINE_STDARG

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <syslog.h>
#include <sys/time.h>

#include <kern/amiga_includes.h>
#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <kern/amiga_log.h>
#include <stdarg.h>
#include <intuition/intuition.h>

#include <proto/intuition.h>

#include <kern/amiga_main_protos.h>
#include <dos/rdargs.h>		/* CSource */

extern void exit(int);

void
cs_putchar(unsigned char ch, struct CSource * cs)
{
  if (cs->CS_CurChr < cs->CS_Length 
      && (cs->CS_Buffer[cs->CS_CurChr] = ch))
    cs->CS_CurChr++;
}

/****i* bsdsocket.library/panic ******************************************
*
*   NAME	
*	panic -- Inform user from serious failure.
*
*   SYNOPSIS
*	panic(Message, Arguments...)
*
*	void panic( STRPTR, ... )
*
*   FUNCTION
*	Calls api_setfunctions() with no arguments to stop programs using
*	AmiTCP. Writes message to log file. Sets up User Requester to
*	inform user about situation. Avoids self-loops.
*
*
*   INPUTS
*    	Messagestring - A pointer to string containing message to show
*	    to user and to write to log. It should describe problem so
*	    that user can take correcting action if it is failure with
*	    his configuration or is able to write bug report if it is
*	    a bug withn program.
*
*       Arguments - as in c-library printf()
*
*   RESULT
*    	This function does not return.
*
*   EXAMPLE
*    	if(Everything==WRONG)
*	    panic("Everything is wrong\nGoto sleep");
*
*   NOTES
*    	As panic does not return, it should be used only in extreme
*	cases
*
*   BUGS
*
*   SEE ALSO
*	log()
*
******************************************************************************
*
*/

#define PANICBUFFERSIZE 512

void
panic(const char *fmt,...)
{
  struct EasyStruct panicES = {
    sizeof( struct EasyStruct),
    NULL,
    "TCP/IP PANIC",
    "panic: %s" ,
    "Shut Down TCP/IP"
  };
  static in_panic = 0;
  struct CSource cs;
  char buffer[PANICBUFFERSIZE];
  va_list ap;
  struct Library *IntuitionBase = NULL; /* local intuitionbase */
  extern struct Task *AROSTCP_Task;

  if (!in_panic){
				/* If we're called previously.. */
    in_panic++;			/* We're in panic now */
    api_setfunctions();		/* Set libraries to return error code */

    cs.CS_Buffer = buffer;
    cs.CS_CurChr = 0;
    cs.CS_Length = PANICBUFFERSIZE;

    va_start(ap, fmt);
    vcsprintf(&cs, fmt, ap);
    va_end(ap);
    
    __log(LOG_EMERG, "panic: %s", buffer); /* Write to log */
  }
  in_panic--;
	
  /*
   * Inform user (if log system has failed...)
   * by opening a requester to the default public screen.
   *
   * Open a local IntuitionBase for the EasyRequest()
   */
  if ((IntuitionBase = OpenLibrary("intuition.library", 37L)) != NULL) {
    EasyRequest(NULL, &panicES, NULL, (ULONG)buffer);
    CloseLibrary(IntuitionBase);
    IntuitionBase = NULL;
  }
    
  /*
   * If the caller is not the AmiTCP task, sleep forever. 
   * This should go to API or where ever we came in AmiTCP code
   */
  if (FindTask(NULL) != AROSTCP_Task)
    Wait(0);

  Wait(SIGBREAKF_CTRL_F);	/* AmiTCP/IP waits here */

  /*
   * Following code is not safe. Probably we should wait infinetely long too...
   */
  deinit_all();			/* This returns !! */
  exit(20);			/* This should be safe... */
}

/****i* bsdsockets.library/log ******************************************
*
*   NAME	
*	log -- Write log message to log and/or console.
*
*   SYNOPSIS
*    	log(Level, FormatString, arguments...)
*
*	void log( ULONG, STRPTR, ...)
*
*   FUNCTION
* 	Writes message given as format string and arguments
*	(printf-style) to both log and console (except if level is
*       LOG_EMERG, LOG_EMERG is generated only by panic and it is
*       written only to the log file as panic() generates an User
*       Requester informing user.
*
*       This function can be called from interrupts.
*
*   INPUTS
*    	Level - indicates type of logged message. These levels are
*	    defined in sys/syslog.h.
*	FormatString - This is a printf-style format string with some
*	    restrictions (notably the floats are not handled).
*	arguments - as in printf() 
*
*   RESULT
*    	Returns no value.
*
*   EXAMPLE
*    	log(LOG_ERR,
*	    "arp: ether address is broadcast for IP address %x!\n",
*            ntohl(isaddr.s_addr));
*	fail=TRUE;
*	break;
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*	Your C-compilers printf() documentation.
*
******************************************************************************
*
*/

void
__log(unsigned long level, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vlog(level, "kernel", fmt, ap);		/* Call actual function */
  va_end(ap);
}

int
vlog(unsigned long level, const char *tag, const char *fmt, va_list ap)
{
  struct SysLogPacket *msg;
  struct timeval now;

  msg = (struct SysLogPacket *)GetLogMsg(&logReplyPort);
  if (msg) {
    ULONG ret;
    struct CSource cs;
    if (tag) {
      cs.CS_Length = log_cnf.log_buf_len-strlen(tag)-1;
      msg->Tag = &msg->String[cs.CS_Length];
      strcpy(msg->Tag, tag);
    } else {
      cs.CS_Length = log_cnf.log_buf_len;
      msg->Tag = NULL;
    }
    cs.CS_Buffer = msg->String;
    cs.CS_CurChr = 0;

    GetSysTime(&now);

    vcsprintf(&cs, fmt, ap);
    msg->Level = level & (LOG_FACMASK | LOG_PRIMASK);	/* Level of message */
    ret = cs.CS_CurChr;
    msg->Time = now.tv_secs;
    DSYSLOG(KPrintF("Putting message = 0x%08x, tag: %s, text: %s\n",msg, msg->Tag, msg->String );)
    PutMsg(logPort, (struct Message *)msg);
    return ret;
  }
  return 0;
}

/****i* bsdsockets.library/printf ******************************************
*
*   NAME	
*	printf -- print to log
*
*   SYNOPSIS
*    	printf(FormatString, Arguments...)
*
*	ULONG printf( const char *, ...)
*
*   FUNCTION
*   	Prints messages to log
*       As log, also this is callable from interrupts.
*
*       Specially for debugging prints.
*
*   INPUTS
*	FormatString - This is a printf-style format string as for vcsprintf().
*	Arguments - as in C-librarys printf() 
*
*   RESULT
*       Number of characters printed.
*
*   EXAMPLE
*
*		printf("line=%d, val=%x\n", 
*                      __LINE__, very.interesting->value);
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*	vcsprintf(), vlog()
*
******************************************************************************
*
*/
int 
printf(const char *fmt, ...)
{
#if 1
  ULONG ret;
  va_list ap;

  va_start(ap, fmt);
  ret = vlog(LOG_INFO, "kernel", fmt, ap);
  va_end(ap);
  
  return ret;
#else
  va_list ap;
  struct SysLogPacket *msg;
  struct timeval now;

  if (msg = GetLogMsg(&logReplyPort)) {	 /* Get next free message */
    struct CSource cs;

    GetSysTime(&now);

    cs.CS_Buffer = msg->String;
    cs.CS_Length = log_cnf.log_buf_len;
    cs.CS_CurChr = 0;

    va_start(ap, fmt);
    vcsprintf(&cs, fmt, ap);
    va_end(ap);

    msg->Level = LOG_INFO;
    msg->chars = cs.CS_CurChr;
    msg->Time = now.tv_secs;
    PutMsg(logPort,(struct Message *)msg);

    return (ULONG)cs.CS_CurChr;
  } else
    return 0;
#endif
}

/****i* bsdsocket.library/sprintf ******************************************
*
*   NAME	
*	sprintf -- print to buffer
*
*   SYNOPSIS
*    	len = sprintf(Buffer, FormatString, Arguments...)
*       len = csprintf(cs, FormatString, Arguments...)
*
*	ULONG sprintf(STRPTR, const char*, ...)
*       ULONG csprintf(struct CSource *, const char*, ...)
*
*   FUNCTION
* 	Prints to a simple buffer or to a CSource buffer. These functions
*       are similar to C-library sprintf() with restricted formatting.
*
*   INPUTS
*       Buffer       - Pointer to buffer.
*	FormatString - This is a printf()-style format string with some
*                      restrictions. Numbers are being taken as 'long' by
*                      default, however.
*	Arguments    - as in printf() .
*       cs           - Pointer to CSource structure.
*
*   RESULT
*       Number of characters printed.
*
*   EXAMPLE
*	sprintf(mybuf, "line=%d, val=%x\n", 
*               __LINE__, very.interesting->value);
*
*   BUGS
*       Function sprintf() assumes that no print is longer than 1024 chars.
*       It does not check for buffer owerflow (there no way to check, the
*       definition of sprintf misses it).
*
*       sprintf strings are truncated to maximum of 1024 chars (including
*       final NUL)
*
*   SEE ALSO
*	vcsprintf()
*
****************************************************************************
*
*/

int 
vcsprintf(struct CSource *cs, const char *fmt, va_list ap)
{
  ULONG start = cs->CS_CurChr;
  char *p, ch, padc;
  u_long ul;
  int n, base, lflag, tmp, width, precision, leftjustify;
  char buf[sizeof(long) * NBBY / 3 + 2]; /* A long in base 8, plus '\0'. */

  if (cs->CS_Length && cs->CS_CurChr < cs->CS_Length) {

    for (;;) {
      padc = ' ';
      width = 0; 
      precision = -1;
      leftjustify = FALSE;
      while ((ch = *fmt++) != '%') {
	if (ch == '\0') {
	  goto end;
	}
	cs_putchar(ch, cs);
      }
      lflag = 0;
reswitch:	
      switch (ch = *fmt++) {
      case '-':
	leftjustify = TRUE;
	goto reswitch;
      case '0':
	padc = '0';
	goto reswitch;
      case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	for (width = 0;; ++fmt) {
	  width = width * 10 + ch - '0';
	  ch = *fmt;
	  if (ch < '0' || ch > '9')
	    break;
	}
	goto reswitch;
      case '.': /* precision */
	for (precision = 0; (ch = *fmt) >= '0' && ch <= '9'; ++fmt) {
	  precision = precision * 10 + ch - '0';
	}
	goto reswitch;
      case 'l':
	lflag = 1;
	goto reswitch;
      case 'b':
	{
	  char *tp;

	  ul = va_arg(ap, int);
	  p = va_arg(ap, char *);
	  for (tp = csprintn(ul, *p++, buf, sizeof(buf)); ch = *tp++;)
	    cs_putchar(ch, cs);
	
	  if (!ul)
	    break;

	  for (tmp = 0; n = *p++;) {
	    if (ul & (1 << (n - 1))) {
	      cs_putchar(tmp ? ',' : '<', cs);
	      for (; (n = *p) > ' '; ++p)
		cs_putchar(n, cs);
	      tmp = 1;
	    } else
	      for (; *p > ' '; ++p)
		;
	  }
	  if (tmp)
	    cs_putchar('>', cs);
	}
	break;
      case 'r':
	p = va_arg(ap, char *);
	vcsprintf(cs, p, va_arg(ap, void *));
/*	vcsprintf(cs, p, va_arg(ap, va_list));*/
	break;
      case 'c':
	*buf = va_arg(ap, int);
	buf[1] = '\0';
	p = buf;
	goto textout;
      case 's':
	p = va_arg(ap, char *);
textout:
	/*
	 * Set width to the maximum width, if maximum width is set, and
	 * width exceeds it.
	 */
	if (precision > 0 && width > precision)
	  width = precision;
	/*
	 * spit out necessary pad characters to fill on left, if necessary
	 */
	if (width > 0 && !leftjustify && (width -= strlen(p)) > 0)
	  while (width--)
	    cs_putchar(padc, cs);
	/* take a copy of the original pointer */
	ul = (u_long)p;
	/*
	 * Copy the characters, pay attention to the fact that precision
         * may not be exceeded.
	 */
	while ((ch = *p++) && precision-- != 0)
	  cs_putchar(ch, cs);
	/*
	 * spit out necessary pad characters to fill on right, if necessary
	 */
	if (width > 0 && leftjustify && (width -= ((u_long)p - ul - 1)) > 0)
	  while (width--)
	    cs_putchar(' ', cs);
	break;
      case 'd':
      case 'i':
        ul = lflag ? va_arg(ap, long) : va_arg(ap, int);
        if ((long)ul < 0) {
	  cs_putchar('-', cs);
	  ul = -(long)ul;
	}
	base = 10;
	goto number;
      case 'o':
	ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
	base = 8;
	goto number;
      case 'u':
	ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
	base = 10;
	goto number;
      case 'p': /* pointers */
      case 'P':
	width = 8;
	padc = '0';
	/* FALLTHROUGH */
      case 'x':
      case 'X':
	ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
	base = 16;
number:
	p = csprintn(ul, base, buf, sizeof(buf));
	tmp = (buf + sizeof(buf) - 1) - p; /* length */
	if (width > 0 && !leftjustify && (width -= tmp) > 0)
	  while (width--)
	    cs_putchar(padc, cs);
	while (ch = *p++)
	  cs_putchar(ch, cs);
	if (width > 0 && leftjustify && (width -= tmp) > 0)
	  while (width--)
	    cs_putchar(' ', cs);
	break;
      default:
	cs_putchar('%', cs);
	if (lflag)
	  cs_putchar('l', cs);
	/* FALLTHROUGH */
      case '%':
	cs_putchar(ch, cs);
      }
    }

end:

    if (cs->CS_CurChr == cs->CS_Length) {
      cs->CS_CurChr--;			/* must NUL terminate */
    }      
    cs->CS_Buffer[cs->CS_CurChr] = '\0';
    
    return cs->CS_CurChr - start;
  } else {
    /* A pathological case */
    return 0;
  }
}

int csprintf(struct CSource *cs, const char *fmt, ...)
{
  va_list ap;
  ULONG len;

  va_start(ap, fmt);
  len = vcsprintf(cs, fmt, ap);
  va_end(ap);
  return len;
}

int vsprintf(char *buf, const char *fmt, va_list ap)
{
  struct CSource cs;

  cs.CS_Buffer = buf;
  cs.CS_CurChr = 0;
  cs.CS_Length = 1024;	  /* This is not probably the cleanest way :-) */

  return vcsprintf(&cs, fmt, ap);
}

int sprintf(char *buf, const char *fmt, ...)
{
  va_list ap;
  ULONG len;

  va_start(ap, fmt);
  len = vsprintf(buf, fmt, ap);
  va_end(ap);
  return len;
}

char *
csprintn(u_long n, int base, char *buf, int buflen)
{
  register char *cp = buf + buflen - 1;

  *cp = 0;
  do {
    cp--;
    *cp = "0123456789abcdef"[n % base];
  } while (n /= base);
  return (cp);
}
