#ifndef ASYNC_DEBUG_C
#define ASYNC_DEBUG_C 1

/*
** activate for stand alone testing, compile with "vc debug.c"

#define DEBUG_TESTMODE 1

**************************************************************************

 Function: Dump debug information over the system debug port

 Copyright in 1991 - 2007 by Guido Mersmann

 $VER: Debug 3.15 (09.01.2007)

**************************************************************************

 Changes:

      (17-05-2002) Guido Mersmann

      First Version

      (15-10-2003) Guido Mersmann

      Fixed decimal output of "0" which was not displayed

      (07-12-2004) Guido Mersmann

      Fixed hex output length generation. %8wx created long output.

      (23-01-2005) Guido Mersmann

      IDGB_PutChar is now replacing any ASCII values between 128 and 32 (Except
      0x09 and 0x10) by a "." character. This avoids unreadable output files
      and especially when dealing with damaged string pointers which are
      dumping trash into the output and forcing output into alternated view.

      (29-01-2005) Guido Mersmann

      %nls is now a special string truncate output. "l" enabled the "length
      mode" and the value defines the maximum number of chars to display.
      This is very handy, during line parsing debug and having damaged
      or unterminated output.

      (01-02-2005) Guido Mersmann

      Added ASCII output behind the %nnth commands. Now the hexdump is more
      monitor like and more useful, when dealing with unknown data.
      Added memory address pointer before %nnth output. I noticed, that I
      always did that and since I am lasy, here is the result. :)

      (26-04-2005) Guido Mersmann

      HexDump method is now dumping correct number of bytes. Prior version
      dumped number * type, so %10lh dumped 10 longs and %10bh 10 bytes.
      Now it will always dump 10 bytes.

      NOTE: If there is a non dividable length specified up to two byte
      additional bytes will be shown. e.g. "%31lh" and "%30lh" will
      show 32 bytes long oriented. Shouldn't be a problem. Use byte mode
      for exact sized outputs.

      (24-09-2005) Guido Mersmann

      HexDump method failed on non dividable sizes. So "%3lh" caused an
      endless loop.

      Fixed HexDump test code.

	  (04-05-2006) Guido Mersmann

	  Added debug support for AmigaOS4 and AROS

	  (27-05-2006) Guido Mersmann

	  Replaced several "char" by "unsigned char".
	  Modified the example code to make it even more useful.
	  Made functions static
	  Removed SysBase access by accessing (4L)
	  Some cleanups.

	  (07-06-2006) Guido Mersmann

	  Added new feature: Timer wait! e.g. %4T is waiting 4 seconds before
	  exiting debug function. MUST NOT be used in interrupts and other
	  simular states, because timer.device is used.

	  (09-01-2007) Guido Mersmann

	  Added global var "debug_disable" which allows to deactivate debug
	  output by the application without making the code unreadable. Simply
	  set to 1 to disable debug.

**************************************************************************

 WHY:

      Well, the normal kprintf() isn't very good. In the most cases the
      debugging line is more complicated than the code to debug.
      Especially when dealing with single characters and hex dumps this
      stuff is more handy than the kprintf. The multiple feature is
      also great, because it allows to dump lines without having them
      as string within the code.

 ToDo:

	  ° This stuff still isn't perfect. The implemented commands are
		working perfect, but some features are missing.

      ° There is no direct serial output for Amithlon. If you want to
        implement, then insert in IDGB_PutChar(), where all outputs
        come together. (Will be implemented when needed!)

 NOTE: If you want to redirect the serial output, execute eg.

       run >nil: sushi >"con:50/50/400/160/sushi/auto" NOPROMPT

 CONFIG:

        #define DEBUG 1

        Enables the entire debug code.

        #define DEBUG_TESTMODE 1

		To test these functions use this statement and compile this file stand
        alone.

        This will add the main() function defined at the end of this file and
        demonstrates the features of this stuff.

**************************************************************************

   types and sizes supported by this debug function: ( case independent)

                    %ld =  32 bit signed decimal   (e.g. '124444' or '-1333323')
                    %lx =  32 bit hex              (e.g. '12adfe45')
                    %lc =  32 bit char             (e.g. 'ILBM')
                    %lb =  32 bit binary           (e.g. '1010 1010 1010 1010 1101 0101 0110 0000')
                    %lh =  32 bit hex dump         (e.g. '04095138: 444da320 26914926 'DM. &.I&'')

                    %wd =  16 bit signed decimal   (e.g. '1222' or '-1223')
                    %wx =  16 bit hex              (e.g. 'abcd')
                    %wc =  16 bit char             (e.g. 'BM')
                    %wb =  16 bit binary           (e.g. '1010 1010 1010 1010')
                    %wh =  16 bit hex dump         (e.g. '04095138: 444d a320 2691 4926 'DM. &.I&'')

                    %bd =   8 bit signed decimal   (e.g. '12' or '-123')
                    %bx =   8 bit hex              (e.g. 'ad')
                    %bc =   8 bit char             (e.g. 'M')
                    %bb =   8 bit binary           (e.g. '1010 1010')
                    %bh =   8 bit hex dump         (e.g. '04095138: 44 4d a3 20 'DM. '')

    without the size indicator these modes are valid. (Note a single 'b' is not allowed here)

                    %d  =  16 bit signed decimal   (e.g. '1222' or '-1223')
                    %x  =  16 bit hex              (e.g. 'abcd')
                    %c  =   8 bit char             (e.g. 'M')
                    %wh =  16 bit hex dump         (e.g. '04095138: 444d a320 2691 4926 'DM. &.I&'')

                    %s  =  NULL terminated string pointer.
                    %ls =  NULL terminated string pointer. Will be truncated
                           when size reaches the size indicator field. ( e.g %3ls will
                           make the string "hello" be displayed as "hel")

                    %m? =  mark creates a line of char. Use number of characters
                           to select number of line chars. ? represents the char
                           you want to use.        (e.g. "%12m-" creates "------------")

					%t  =  wait n seconds by using timer. Useful when searching for a crashpoint.
						   Only the last value is used and only one wait is done per debug call.
						   (e.g. "Where is the crash? %4t\n" creates 4 second delay)

					%v  =  dumps a datestamp as D%08lx,M%08lx,T%08lx.

   size indicator   The first char after the "%" is treated as size indicator.
                    Valid chars are "b", "w", "l" and their upper case versions.
                    if it's missing a default value is used. (see above)

   modifiers        0   =  leading spaces for decimal and string, 0's for hex
               (1-9999) =  number of characters to output. By default the
                           outstanding chars will be filled with spaces.
                    -   =  center right mode. Instead of adding spaces at the
                           end the spaces will be added in front of the string.

   parameters       anything that fits into a 32 bit argument


   example  debug( "decimal=%ld, hex=$%0wx, multiple: %5m~",27,27);

            prints: 'decimal= 27, hex=$001b, multiple: ~~~~~'


**************************************************************************/
#ifdef DEBUG_TESTMODE

  #ifndef DEBUG
    #define DEBUG 1
  #endif

#include "debug.h"

#endif
/**************************************************************************/

#ifdef DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <exec/types.h>
#include <devices/timer.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <SDI/SDI_compiler.h>

/*************************************************************************/

static void IDGB_String( signed char *string, LONG size, ULONG flags, long counter);
static void IDGB_RepeatChar(signed char chr, long count);
static void IDGB_PutString( signed char *string, long count);
static void IDGB_PutChar( signed char chr);
static void IDGB_MethodChar( long value, ULONG flags, long count);
static void IDGB_MethodHex( long value, ULONG flags, long count);
static void IDGB_MethodDec( long value, ULONG flags, long count);
static void IDGB_MethodBin( long value, ULONG flags, long count);
static void IDGB_MethodHexDump( long value, ULONG flags, long count);
static void IDGB_MethodDateStamp( long value, ULONG flags, long count);
static void IDGB_MethodTimerWait( long secs );

/* These are the flags used for internal operation. */

#define BUGB_LEAD0     0
#define BUGB_LEADMINUS 1
#define BUGB_LONG      2
#define BUGB_WORD      3
#define BUGB_BYTE      4

#define BUGF_LEAD0     (1 << BUGB_LEAD0)
#define BUGF_LEADMINUS (1 << BUGB_LEADMINUS)
#define BUGF_LONG      (1 << BUGB_LONG)
#define BUGF_WORD      (1 << BUGB_WORD)
#define BUGF_BYTE      (1 << BUGB_BYTE)

/*
** some defaults
*/

#ifndef HEXDUMP_NIBBLESPERLINE
#define HEXDUMP_NIBBLESPERLINE  32          /* defines line size of hex dumps in nibbles per line */
#endif

#ifndef DEBUG_MAXSTRINGSIZE
#define DEBUG_MAXSTRINGSIZE 100
#endif

#define DEBUG_FILTERCHAR        46          /* defines the character which is used for unprintable characters */

#define ASCII_TAB       0x09
#define ASCII_RETURN    0x0a

/*
** global debug vars
*/

ULONG debug_disable;
#undef debug /* kill prototype macro, if exists */

/* /// "RawPutChar defines for all OS versions" */

/*************************************************************************/

/* For 68K we need to create RawPutChar inline depending on compiler */
#if !defined(__MORPHOS__) && !defined(__amigaos4__) && !defined(__AROS__)
    #ifdef __VBCC__
         void __RawPutChar(REG( a6, void *), REG(d0, char chr) )="\tjsr\t-516(a6)";
		 #define RawPutChar(chr) __RawPutChar( (*((struct ExecBase **) 4)) , (chr))
    #endif
#endif

#ifdef __amigaos4__
	#include <clib/debug_protos.h>
#define RawPutChar(chr) kputc( chr )
#endif /* __amigaos4__ */

/* \\\ */

/* /// "Debug"
**
** Function: This is the routine you should call from inside your code.
** 			 Use like printf() just with more debug specific features.
** 			 For more information browse to the head of the document.
*/

void STDARGS debug( char *text, ...)
{

ULONG i = 0;
UBYTE chr;
char *string;
ULONG si;
ULONG timersecs = 0; /* wait after debug in seconds, default off */
va_list va;

	if( debug_disable ) return;

    va_start(va, text);

    while( (chr = text[i++]) != 0x00 ){

        if( chr == '%') {

                ULONG flags = 0;
                ULONG count = 0;

                if( !(chr = text[i]) ) {
                    return;
                }

                switch( chr ) {
                    case '0':
                        flags |= BUGF_LEAD0;
                        i++;
                        break;
                    case '-':
                        flags |= BUGF_LEADMINUS;
                        i++;
                        break;
                }
/* value check */
                while( 1) {

                    if( !(chr = text[i++]) ) {
                        return;
                    }

                    if( chr < '0' || chr > '9') {
                        break;
                    }

                    count = (count * 10) + (chr - 0x30);
                }

                i--;
/* Type check */

                if( !(chr = text[i]) ) {
                    return;
                }

                switch( chr ) {

                    case 'l':
                    case 'L':
                        flags |= BUGF_LONG;
                        i++;
                        break;
                    case 'w':
                    case 'W':
                        flags |= BUGF_WORD;
                        i++;
                        break;
                    case 'b':
                    case 'B':
                        flags |= BUGF_BYTE;
                        i++;
                        break;
                }
/* Method */
                if( !(chr = text[i++]) ) {
                    return;
                }

                switch( chr) {

                    case 's':
                    case 'S':

                        if( (string = va_arg(va, char *)) != NULL ) {
                            si = 0;
                            while( string[si++] ) {}
                            si--;
                            /* The "l" flag allows size limited strings (%20ls) */
                            if( flags & BUGF_LONG && count && si > count ) {
                                IDGB_PutString( string, count);
                            } else {
                                IDGB_String( string, si, flags, count);
                            }
                        }
                        break;
                    case 'x':
                    case 'X':
                        IDGB_MethodHex( va_arg(va, long), flags, count);
                        break;
                    case 'd':
                    case 'D':
                        IDGB_MethodDec( va_arg(va, long), flags, count);
                        break;
                    case 'b':
                    case 'B':
                        IDGB_MethodBin( va_arg(va, long), flags, count);
                        break;
                    case 'c':
                    case 'C':
                        IDGB_MethodChar( va_arg(va, long), flags, count);
                        break;
                    case 'm':
                    case 'M':

                        count = count ? count : 30;  /* setup default value if count is NULL */

                        if( !(chr = text[i++]) ) {   /* get line character */
                            return;
                        }
                            IDGB_RepeatChar( chr, count ); /* print line */
                        break;
                    case 'h':
                    case 'H':
                        IDGB_MethodHexDump( va_arg(va, long), flags, count);
                        break;
					case 't':
					case 'T':
						timersecs = count;
						break;
					case 'v':
					case 'V':
						IDGB_MethodDateStamp( va_arg(va, long), flags, count);
						break;
                    default:
                        IDGB_PutChar( chr ); /* unknown method, so just print it! */
                        break;
                }

        } else {
            IDGB_PutChar( chr );
        }
    }
    va_end(va);

	IDGB_MethodTimerWait( timersecs );

}
/* \\\ */

/* /// "IDGB_String" */

/*************************************************************************/

static void IDGB_String( signed char *string, LONG size, ULONG flags, long counter)
{

    if( flags & BUGF_LEADMINUS) {
        IDGB_RepeatChar(  ' ', counter - size);
    }

    IDGB_PutString( string, size);

    if( !(flags & BUGF_LEADMINUS)) {
        IDGB_RepeatChar(  ' ', counter - size);
    }
}
/* \\\ */

/* /// "IDGB_RepeatChar" */

/*************************************************************************/

static void IDGB_RepeatChar( signed char chr, long count)
{
/*
** it's not required to check (count <= 0), this is used as feature
** and filtered by while()
*/
	if( count > DEBUG_MAXSTRINGSIZE) {
		debug("\n\nRepeatChar: Warning!! Reduced count from %ld to 100\n\n", count);
		count = DEBUG_MAXSTRINGSIZE;
    }

    while( count-- > 0 ) {
        IDGB_PutChar( chr );
    }
}
/* \\\ */

/* /// "IDGB_PutString" */

/*************************************************************************/

static void IDGB_PutString( signed char *string, long count)
{
/*
** it's not required to check (count <= 0), this is used as feature
** and filtered by while()
*/
	if( count > DEBUG_MAXSTRINGSIZE ) {
		debug("\n\nPutString: Warning!! Reduced count from %ld to %ld\n\n", count, DEBUG_MAXSTRINGSIZE);
		count = DEBUG_MAXSTRINGSIZE;
    }

    while( count-- > 0 ) {
        IDGB_PutChar( *string++ );
    }
}
/* \\\ */

/* /// "IDGB_PutChar" */

/*************************************************************************/

/* INSERT DIREKT COM OUTPUT HERE */
static void IDGB_PutChar( signed char chr)
{
    switch( chr ) {
        case ASCII_RETURN:
        case ASCII_TAB:
            break;
        default:
            if( chr < 32 ) { /* 128 - 32 will be filtered */
                chr = DEBUG_FILTERCHAR;
            }
            break;
    }
    RawPutChar( chr );
}
/* \\\ */

/* /// "IDGB_MethodChar" */

/*************************************************************************/

static void IDGB_MethodChar( long value, ULONG flags, long count)
{
ULONG bytes;

    bytes = 1;
    if( flags & BUGF_LONG) {
        bytes = 4;
    }
    if( flags & BUGF_WORD) {
        bytes = 2;
    }

/* value specified by user overrides BWL mode. */

    if( count) {
        bytes = count;
    }

/* Leading ZERO indicated fillup to 4! */

    if( (flags & BUGF_LEAD0)) {
        if( count > 0 && count <= 4) {
            IDGB_RepeatChar( ' ', 4 - count);
        }
    }

    switch( bytes) {
        case 3:
            value = value << 8;
            break;
        case 2:
            value = value << 16;
            break;
        case 1:
            value = value <<24;
            break;
    }


    while( bytes--) {
        IDGB_PutChar( value >> 24);
        value = value << 8;
    }
}
/* \\\ */

/* /// "IDGB_MethodHex" */

/*************************************************************************/

static const char hextable[] = { "0123456789abcdef" };

static void IDGB_MethodHex( long value, ULONG flags, long count)
{
ULONG nibbles = 4;
ULONG i = 0;
char chr[9];

    if( flags & BUGF_LONG) {
        nibbles = 8;
    }
    if( flags & BUGF_BYTE) {
        nibbles = 2;
    }

    value = value << ( (8 - nibbles) * 4 );

    while( nibbles--) {

        chr[i] = hextable[ (value >> 28) & 0x0f ];

        if( chr[i] == '0') {
            if( flags & BUGF_LEAD0) {
                i++;
            }
        } else {
            flags |= BUGF_LEAD0;
            i++;
        }
        value = value << 4;
    }
    if( i == 0 ) {
        chr[i++] = '0';
    }
    IDGB_String( chr, i, flags, count);
}
/* \\\ */

/* /// "IDGB_MethodDec" */

/*************************************************************************/

static void IDGB_MethodDec( long value, ULONG flags, long count)
{
ULONG divide;
ULONG i,o;
char chr[16];
const ULONG dectable[] = { 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1,0 };

    i = 0;
    o = 0;

    if( !(flags & BUGF_LONG) ) {

        if( flags & BUGF_BYTE) {
            value = (long) (BYTE) value;
        } else {
            value = (long) (WORD) value;
        }
    }

    if( value < 0) {
        value *= -1;
        chr[o++] = '-';
    }

    flags &= ~BUGF_LEAD0;


    while( (divide = dectable[i++]) != 0 ) {

        if( (chr[o] = (value / divide) | '0') != '0' ) {
            flags |= BUGF_LEAD0;
            o++;
        } else {
            if( flags & BUGF_LEAD0 ) {
                o++;
            }
        }
        value = value % divide;
    }


    if( o == 0 || (o == 1 && chr[0] == '-' ) ) {
        chr[0] = '0';
        o = 1;
    }
    IDGB_String( chr, o, flags, count);
}
/* \\\ */

/* /// "IDGB_MethodBin" */

/*************************************************************************/

static void IDGB_MethodBin( long value, ULONG flags, long count)
{
ULONG bits = 8-1;


    if( flags & BUGF_LONG) {
        bits = 32-1;
    }
    if( flags & BUGF_WORD) {
        bits = 16-1;
    }

    if( count > 0 && count < 33 ) {
        bits = count-1;
    }

    do {
        IDGB_PutChar( ( (value & (1<<bits) ) ? '1' : '0')  );
        if( (bits & 3) == 0 && bits ) {
            IDGB_PutChar( ' ' );
        }
    } while ( bits-- > 0 );

}
/* \\\ */

/* /// "IDGB_MethodHexDump" */

/*************************************************************************/

static void IDGB_MethodHexDump( long value, ULONG flags, long count)
{
long output;
long nibbles;
STRPTR str;
char chr;

    IDGB_PutChar( '\n' );

    nibbles = HEXDUMP_NIBBLESPERLINE;
    str = (STRPTR) value;

/* check is the number of values is 0 or negative, then for default! */

    if( count <= 0) {
        count = 16;  /* default is 16 bytes */
    }

/* default is word, so if no value is specified force word. */

    if( !(flags & ( BUGF_LONG | BUGF_BYTE | BUGF_WORD)) ) {
        flags |= BUGF_WORD;
    }

/* This is required for leading zero output */

    flags |= BUGF_LEAD0;

    while( count > 0) {

/* if nibbles are at maximum, we display the memory adress */
        if( nibbles == HEXDUMP_NIBBLESPERLINE ) {
            IDGB_MethodHex( value, BUGF_LONG|BUGF_LEAD0, 0 );
            IDGB_PutChar( ':' );
            IDGB_PutChar( ' ' );
        }
/* now we read the output value depending on the requested mode */

        switch( flags & ( BUGF_LONG | BUGF_BYTE | BUGF_WORD) ) {

            case BUGF_LONG:
                output   = *((LONG *) value);
                value   += 4;
                nibbles -= 8;
                count   -= 4;
                break;

            case BUGF_WORD:
                output   = *((WORD *) value);
                value   += 2;
                nibbles -= 4;
                count   -= 2;
                break;

            case BUGF_BYTE:
                output   = *((BYTE *) value);
                value   += 1;
                nibbles -= 2;
                count   -= 1;
                break;

            default:
                output   = 0;
                value   += 1;
                nibbles -= 2;
                count   -= 1;
                break;

        }
/* and dump in as hex */

        IDGB_MethodHex( output, flags, 0 );
        IDGB_PutChar( ' ' );

/* at line end or if count get 0, we need to dump the ascii output */

        if( nibbles <= 0 || count <= 0) {
            while( nibbles > 0 ) {
                switch( flags & ( BUGF_LONG | BUGF_BYTE | BUGF_WORD) ) {
                    case BUGF_LONG:
                        IDGB_PutChar( ' ' );
                        IDGB_PutChar( ' ' );
                        IDGB_PutChar( ' ' );
                        IDGB_PutChar( ' ' );
                        nibbles -= 4;
                    case BUGF_WORD:
                        IDGB_PutChar( ' ' );
                        IDGB_PutChar( ' ' );
                        nibbles-= 2;
                    case BUGF_BYTE:
                        IDGB_PutChar( ' ' );
                        IDGB_PutChar( ' ' );
                        nibbles-= 2;
                }
                IDGB_PutChar( ' ' );
            }
/* now we can dump the line data as ASCII */
            IDGB_PutChar( 39 );
			while( ( (ULONG) str < (ULONG) value ) ) {
				chr = *((BYTE *) str++ );
                if( chr != ASCII_RETURN && chr != ASCII_TAB ) {
                    IDGB_PutChar( chr);
                } else {
                    IDGB_PutChar( DEBUG_FILTERCHAR );
                }
            }
            IDGB_PutChar( 39 );
/* finally we terminate the output and restart the nibble count */
            IDGB_PutChar( '\n' );
            nibbles = HEXDUMP_NIBBLESPERLINE;
        }

   }
}
/* \\\ */

/* /// "IDGB_MethodTimerWait" */

/*************************************************************************/

static void IDGB_MethodTimerWait( long secs )
{
struct IORequest *IORequest;
struct MsgPort *MessagePort;

	if( secs ) {
		if( ( MessagePort = CreateMsgPort() ) ) {
			if( ( IORequest = CreateIORequest( MessagePort, sizeof( struct timerequest) ) ) ) {
				if( !OpenDevice("timer.device", UNIT_MICROHZ, IORequest, 0L ) ) {
				   IORequest->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

					((struct timerequest *) IORequest)->tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
					((struct timerequest *) IORequest)->tr_time.tv_secs  = secs;
					((struct timerequest *) IORequest)->tr_time.tv_micro = 0;

					((struct timerequest *) IORequest)->tr_node.io_Command = TR_ADDREQUEST;

					DoIO( IORequest );

					while( !CheckIO( IORequest) ) {
						AbortIO( IORequest);
						WaitIO( IORequest );  /* wait for outstanding timer */
					}
					CloseDevice( IORequest );
				}
				DeleteIORequest( IORequest );
		    }
			DeleteMsgPort( MessagePort );
		}
	}
}
/* \\\ */

/* /// "IDGB_MethodDateStamp" */

/*************************************************************************/

static void IDGB_MethodDateStamp( long value UNUSED, ULONG flags UNUSED, long count UNUSED )
{
struct Library *DOSBase;
struct DateStamp ds;

	if( (DOSBase = OpenLibrary("dos.library", 30)) ) {
		DateStamp( &ds );
		CloseLibrary( DOSBase );
	}
	IDGB_PutChar('D');
	IDGB_MethodHex( ds.ds_Days  , BUGF_LONG|BUGF_LEAD0, 8 );
	IDGB_PutChar(',');
	IDGB_PutChar('M');
	IDGB_MethodHex( ds.ds_Minute, BUGF_LONG|BUGF_LEAD0, 8 );
	IDGB_PutChar(',');
	IDGB_PutChar('T');
	IDGB_MethodHex( ds.ds_Tick  , BUGF_LONG|BUGF_LEAD0, 8 );
}
/* \\\ */

#endif


/* /// TEST CODE! */

/*************************************************************************/

#ifdef DEBUG_TESTMODE

int main ( void )
{
#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

const char T_String[] = { "teststring"};

BYTE hexdata[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
                     40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,
                     77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,
                     111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127};

#define T_GUID MAKE_ID('G','U','I','D')

long arg1 = 0x12345678;
long arg2 = 0x00000678;
long arg3 = -12;
long arg4 = -12345678;
long arg5 = 0xa50EC05a;

	debug ("%70m**\n");

    debug (" \nDebug Include Test Code\n");
    debug ("By Guido Mersmann\n");

    debug ("\nFirst in each line is the statement. Second is what the result should\nbe and right the real result created by the code\n\n");

	debug ("\n%70m**\n");

	debug ("\n String: (Argument: '%s')\n\n", T_String);
    debug ("%%s    'teststring'           '%s'\n", T_String);
    debug ("%%-20s '          teststring' '%-20s'\n", T_String);

	debug ("\n%70m~\n");

	debug ("\n Char: (Argument: %lc)\n\n",T_GUID);
    debug ("%%c    'D'    '%c'\n",T_GUID);
    debug ("%%bc   'D'    '%bc'\n",T_GUID);
    debug ("%%wc   'ID'   '%wc'\n",T_GUID);
    debug ("%%lc   'GUID' '%lc'\n",T_GUID);
    debug ("%%3c   'UID'  '%3c'\n",T_GUID);
    debug ("%%03c  ' UID' '%03c'\n",T_GUID);

	debug ("\n%70m~\n");

	debug ("\n Hex: (Argument: 0x%lx)\n\n", arg1);
    debug ("%%x     '5678'      '%x'\n", arg1 );
    debug ("%%bx    '78'        '%bx'\n", arg1 );
    debug ("%%wx    '5678'      '%wx'\n", arg1 );
    debug ("%%lx    '12345678'  '%lx'\n", arg1 );
    debug ("%%0lx   '00000678'  '%0lx'\n", arg2 );

    debug ("%%8wx   '5678    '  '%8wx'\n", arg1 );
    debug ("%%04wx  '5678'      '%04wx'\n", arg1 );
    debug ("%%-04wx '    5678'      '%-04wx'\n", arg1 );

    debug ("%%-8bx  '      78'  '%-8bx'\n", arg1 );
    debug ("%%8bx   '78      '  '%8bx'\n", arg1 );

	debug ("\n%70m~\n");

	debug ("\n Decimal: (Argument: 0x%lx, %ld, %ld)\n\n", arg1, arg3, arg4 );
    debug ("%%d     '22136'     '%d'\n", arg1);
    debug ("%%bd    '120'       '%bd'\n", arg1);
    debug ("%%wd    '22136'     '%wd'\n", arg1);
    debug ("%%ld    '305419896' '%ld'\n", arg1);
    debug ("%%03d   '22136'     '%03d'\n", arg1);
    debug ("%%0d    '-12'       '%0d'\n", arg3);
    debug ("%%07bd  '-12    '   '%07bd'\n", arg3);
    debug ("%%-07bd '    -12'   '%-07bd'\n", arg3);
    debug ("%%ld '  '-12345678' '%ld'\n", arg4);

	debug ("\n%70m~\n");

	debug ("\n Binary: (Argument: 0x%lx)\n\n", arg5);

    debug ("%%bb  '0101 1010'                               '%bb'\n", arg5);
    debug ("%%wb  '1100 0000 0101 1010'                     '%wb'\n", arg5);
    debug ("%%lb  '1010 0101 0000 1110 1100 0000 0101 1010' '%lb'\n", arg5);

    debug ("%%6bb '01 1010'                                 '%6bb'\n", arg5);

	debug ("\n%70m~\n");

	debug ("\n Multiple:\n\n");

    debug ("%%10m~ '~~~~~~~~~~' '%10m~'\n");
    debug ("%%10m+ '++++++++++' '%10m+'\n");
    debug ("%%10m- '----------' '%10m-'\n");

	debug ("\n%70m~\n");

	debug ("\n Hex Dump ( First line is what the result should be, second the real output )\n\n");
	debug ("%%3h       4041 4243                               '@ABC' %3h\n"  , &hexdata[64] );
	debug ("%%3wh      4041 4243                               '@ABC' %3wh\n" , &hexdata[64] );
	debug ("%%3lh      40414243                            '@ABC'   %3lh\n" , &hexdata[64] );
	debug ("%%3bh      40 41 42                                        '@AB'   %3bh\n" , &hexdata[64] );

	debug ("Multi line byte hex: %30bh\n", &hexdata[64] );
	debug ("Multi line word hex: %30wh\n", &hexdata[64] );
	debug ("Multi line long hex: %30lh\n", &hexdata[64] );

	debug ("Now we wait 5 Seconds to continue...%5T");
	debug ("... Done!\n");


	debug ("\n%70m**\n");

}

#endif

/* \\\ TEST CODE! */


#endif /* ASYNC_DEBUG_C */

