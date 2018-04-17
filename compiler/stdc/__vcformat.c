/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Function to format a string like printf().
*/

/*
 * This function is used by debug functions during early startup.
 * Please keep it self-contained, at least when compiled with -DSTDC_STATIC.
 */

/* Original source from libnix */
#include <dos/bptr.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef AROS_NO_LIMITS_H
#	include <limits.h>
#else
#	define ULONG_MAX   4294967295UL
#endif
#include <ctype.h>
#include <math.h>
#include <float.h>

#ifndef STDC_STATIC
#define FULL_SPECIFIERS
#endif

#ifndef BITSPERBYTE
#   define BITSPERBYTE 8
#endif

#if (__WORDSIZE == 64)
/* On 64-bit machines long and long long are the same, so we don't need separate processing for long long */
#undef AROS_HAVE_LONG_LONG
#endif

/* a little macro to make life easier */
#define OUT(c)  do                           \
		{ if((*outc)((unsigned char)(c),data)==EOF)   \
		    return outcount;	     \
		  outcount++;		     \
		}while(0)

#define MINFLOATSIZE (DBL_DIG+1) /* Why not 1 more - it's 97% reliable */
#define MININTSIZE (sizeof(unsigned long)*BITSPERBYTE/3+1)
#define MINPOINTSIZE (sizeof(void *)*BITSPERBYTE/4+1)
#define REQUIREDBUFFER (MININTSIZE>MINPOINTSIZE? \
			(MININTSIZE>MINFLOATSIZE?MININTSIZE:MINFLOATSIZE): \
			(MINPOINTSIZE>MINFLOATSIZE?MINPOINTSIZE:MINFLOATSIZE))

#define ALTERNATEFLAG 1  /* '#' is set */
#define ZEROPADFLAG   2  /* '0' is set */
#define LALIGNFLAG    4  /* '-' is set */
#define BLANKFLAG     8  /* ' ' is set */
#define SIGNFLAG      16 /* '+' is set */
#define LDBLFLAG      64 /* Processing a long double */

const unsigned char *const __decimalpoint = ".";

static size_t format_long(char *buffer, char type, int base, unsigned long v)
{
    size_t size = 0;
    char hex = 'a' - 10;
    unsigned char mask  = 0;
    unsigned char shift = 0;

    switch (type)
    {
    case 'X':
        hex = 'A' - 10;
    case 'x':
        shift   = 4;
        mask    = 0x0F;
        if (base == 10)
            base = 16;
        break;

    case 'o':
    	shift   = 3;
    	mask    = 0x07;
        if (base == 10)
            base = 8;
    	break;

    default:	/* 'd' and 'u' */
    	/* Use slow divide operations for decimal numbers */
    	do
    	{
    	    char c = v % base;

            *--buffer = c + '0';
            v /= base;
            size++;
    	} while (v);

    	return size;
    }

    /* Divisor is a power of 2, so use fast shifts for division */
    do
    {
	char c = v & mask;

	*--buffer = (c < 10) ? c + '0' : c + hex;
	v >>= shift;
	size++;
    } while (v);

    return size;
}

#ifdef AROS_HAVE_LONG_LONG

/*
 * This is the same as format_long(), but takes long long argument.
 * This is used to process long long values on 32-bit machines. 64-bit
 * operations are performed slower there, and may need to call libgcc routines.
 */
static size_t format_longlong(char *buffer, char type, int base, unsigned long long v)
{
    size_t size = 0;
    char hex = 'a' - 10;
    unsigned char mask  = 0;
    unsigned char shift = 0;

    switch (type)
    {
    case 'X':
        hex = 'A' - 10;

    case 'x':
        shift = 4;
        mask = 0x0F;
        if (base == 10)
            base = 16;
        break;

    case 'o':
    	shift = 3;
    	mask  = 0x07;
        if (base == 10)
            base = 8;
    	break;

    default:
/*
 * FIXME: this is not compiled for $(GENDIR)/lib32/librom.a because this requires
 * __umoddi3() and __udivdi3() from 32-bit version of libgcc which is not supplied
 * with 64-bit AROS gcc.
 * Perhaps these routines needs to be implemented explicitly for the bootstrap. Or
 * this code needs to be rewritten without these division operations, implemenging
 * decimal division explicitly.
 * As a consequence, %llu and %lld do not work in x86-64 bootstrap. Use hexadecimal
 * output or fix this.
 */
#ifndef STDC_LIB32
    	do
    	{
    	    char c = v % base;

            *--buffer = c + '0';
            v /= base;
            size++;
    	} while (v);
#endif

    	return size;
    }

    do
    {
	char c = v & mask;

	*--buffer = (c < 10) ? c + '0' : c + hex;
	v >>= shift;
	size++;
    } while (v);

    return size;
}

#endif

/*****************************************************************************

    NAME */

	int __vcformat (

/*  SYNOPSIS */
	void	   * data,
	int	  (* outc)(int, void *),
	const char * format,
	va_list      args)

/*  FUNCTION
	Format a list of arguments and call a function for each char
	to print.

    INPUTS
	data - This is passed to the usercallback outc
	outc - Call this function for every character that should be
		emitted. The function should return EOF on error and
		> 0 otherwise.
	format - A printf() format string.
	args - A list of arguments for the format string.

    RESULT
	The number of characters written.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
  size_t outcount=0;

  while(*format)
  {
    if(*format=='%')
    {
      static const char flagc[] = { '#','0','-',' ','+' };
      size_t width=0,preci=ULONG_MAX,flags=0; /* Specifications */
      char type,subtype='i';
#ifdef AROS_HAVE_LONG_LONG
      char lltype=0;
#endif
      char buffer1[2];		   /* Signs and that like */
      char buffer[REQUIREDBUFFER]; /* The body */
      char *buffer2=buffer;	   /* So we can set this to any other strings */
      size_t size1 = 0, size2 = 0;/* How many chars in buffer? */
      const char *ptr=format+1;    /* pointer to format string */
      size_t i,pad;		   /* Some temporary variables */
      union {			   /* floating point arguments %[aAeEfFgG] */
	double dbl;
	long double ldbl;
      } fparg;

      do /* read flags */
	for(i=0;i<sizeof(flagc);i++)
	  if(flagc[i]==*ptr)
	  { flags|=1<<i;
	    ptr++;
	    break; }
      while(i<sizeof(flagc));

      if(*ptr=='*') /* read width from arguments */
      { signed int a;
	ptr++;
	a=va_arg(args,signed int);
	if(a<0)
	{ flags|=LALIGNFLAG;
	  width=-a; }
	else
	  width=a;
      }else
	while(isdigit(*ptr))
	  width=width*10+(*ptr++-'0');

      if(*ptr=='.')
      { ptr++;
	if(*ptr=='*') /* read precision from arguments */
	{ signed int a;
	  ptr++;
	  a=va_arg(args,signed int);
	  if(a>=0)
	    preci=a;
	}else
	{ preci=0;
	  while(isdigit(*ptr))
	    preci=preci*10+(*ptr++-'0');
	}
      }

      if (*ptr == 'h' || *ptr == 'l' || *ptr == 'L' || *ptr == 'z')
	subtype=*ptr++;

      if (*ptr == 'l' || *ptr == 'q')
      {
#ifdef AROS_HAVE_LONG_LONG
          lltype = 1;
          subtype = 'l';
#endif
          ptr++;
      }
      
      type = *ptr++;

      switch(type)
      {
      case 'd':
      case 'i':
      case 'o':
      case 'p':
      case 'u':
      case 'x':
      case 'X':
      {
#ifdef AROS_HAVE_LONG_LONG
          unsigned long long llv = 0;
#endif
          unsigned long v = 0;
          int base = 10;

	  if (type=='p') /* This is written as 0x08lx (or 0x016lx on 64 bits) */
	  {
	      subtype = 'l';
	      type = 'x';
	      if (!width)
	          width = sizeof(void *) * 2;
	      flags |= ZEROPADFLAG;
	  }

	  if (type=='d' || type=='i') /* These are signed */
	  {
              signed long v2;

	      if (subtype=='l')
              {
#ifdef AROS_HAVE_LONG_LONG
              	  if (lltype)
              	  {
	              signed long long llv2;
	              
	              llv2 = va_arg(args, signed long long);
	              if (llv2 < 0)
	              {
	            	  llv = - llv2;
	              	  v2 = -1;	/* Assign a dummy value to v2 in order to process sign below */
	              }
	              else
	              {
	            	  llv = llv2;
	             	  v2  = llv2 ? 1 : 0;
	              }
	          }
                  else
#endif
		      v2=va_arg(args, signed long);
	      }
	      else if (subtype=='z')
	          v2 = va_arg(args,size_t);
	      else
	          v2 = va_arg(args,signed int);

	      if (v2 < 0)
	      {
	          buffer1[size1++]='-';
	          v = -v2;
	      }
	      else
	      {
	          if (flags & SIGNFLAG)
		      buffer1[size1++] = '+';
		  else if (flags & BLANKFLAG)
		      buffer1[size1++] = ' ';
	      	  v = v2;
	      }
	  }
	  else 		   /* These are unsigned */
	  {
	      if (subtype=='l')
              {
#ifdef AROS_HAVE_LONG_LONG
                  if (lltype)
	              llv = va_arg(args, unsigned long long);
                  else
#endif
	              v = va_arg(args,unsigned long);
	      }
	      else if (subtype == 'z')
	          v = va_arg(args, size_t);
	      else
	          v = va_arg(args, unsigned int);

	      if (flags & ALTERNATEFLAG)
	      {
	          if (type == 'o' && preci && v)
		      buffer1[size1++] = '0';
	          if ((type == 'x' || type == 'X') && v)
	          {
	              buffer1[size1++] = '0';
		      buffer1[size1++] = type;
	          }
	      }
	  }

	  buffer2 = &buffer[sizeof(buffer)]; /* Calculate body string */

#ifdef AROS_HAVE_LONG_LONG
	  /*
	   * For long long type we have actual value in llv.
	   * For long we have actual value in v.
	   * This avoids slow 64-bit operations on 32-bit processors
	   * when not needed.
	   */
	  if (lltype)
	      size2 = format_longlong(buffer2, type, base, llv);
	  else
#endif
	      size2 = format_long(buffer2, type, base, v);
	  /* Position to the beginning of the string */
	  buffer2 -= size2;

	  if (preci == ULONG_MAX) /* default */
	      preci = 0;
	  else
	      flags &= ~ZEROPADFLAG;
	  break;
      }

      case 'c':
	  if (subtype=='l')
          {
#ifdef AROS_HAVE_LONG_LONG
              if (lltype)
	          *buffer2 = va_arg(args, long long);
              else
#endif
	          *buffer2 = va_arg(args, long);
	  }
	  else
	      *buffer2 = va_arg(args, int);

	  size2 = 1;
	  preci = 0;
	  break;

      case 's':
	  buffer2 = va_arg(args, char *);
	  if (!buffer2)
	      buffer2 = "(null)";
	  size2 = strlen(buffer2);
	  size2 = size2 <= preci ? size2 : preci;
	  preci = 0;
	  break;

      case 'b':
      	  buffer2 = BADDR(va_arg(args, BPTR));
      	  if (buffer2)
#if AROS_FAST_BSTR
	      size2 = strlen(buffer2);
#else
	      size2 = *(unsigned char *)buffer2++;
#endif
	  else
	  {
	      buffer2 = "(null)";
	      size2 = 6;
	  }

	  size2 = size2 <= preci ? size2 : preci;
	  preci = 0;
	  break;

#ifdef FULL_SPECIFIERS
      case 'a':
      case 'A':
      case 'f':
      case 'e':
      case 'E':
      case 'g':
      case 'G':
      {
	  char killzeros=0,sign=0; /* some flags */
	  int ex1,ex2; /* Some temporary variables */
	  size_t size,dnum,dreq;
	  char *udstr=NULL;

	  if (subtype=='L')
	  { flags|=LDBLFLAG;
            fparg.ldbl=va_arg(args,long double);
          }else
	  { flags&=~LDBLFLAG;
            fparg.dbl=va_arg(args,double);
          }

	  if(isinf(fparg.dbl))
	  { if(fparg.dbl>0)
	      udstr="+inf";
	    else
	      udstr="-inf";
	  }else if(isnan(fparg.dbl))
	    udstr="NaN";

	  if(udstr!=NULL)
	  { size2=strlen(udstr);
	    preci=0;
	    buffer2=udstr;
	    break; }

	  if(preci==ULONG_MAX) /* old default */
	    preci=6; /* new default */

	  if(fparg.dbl<0.0)
	  { sign='-';
	    fparg.dbl=-fparg.dbl;
	  }else
	  { if(flags&SIGNFLAG)
	      sign='+';
	    else if(flags&BLANKFLAG)
	      sign=' ';
	  }

	  ex1=0;
	  if(fparg.dbl!=0.0)
	  { ex1=log10(fparg.dbl);
	    if(fparg.dbl<1.0)
	      fparg.dbl=fparg.dbl*pow(10,- --ex1); /* Caution: (int)log10(.5)!=-1 */
	    else
	      fparg.dbl=fparg.dbl/pow(10,ex1);
	    if(fparg.dbl<1.0) /* adjust if we are too low (log10(.1)=-.999999999) */
	    { fparg.dbl*=10.0; /* luckily this cannot happen with FLT_MAX and FLT_MIN */
	      ex1--; } /* The case too high (log(10.)=.999999999) is done later */
	  }

	  ex2=preci;
	  if(type=='f')
	    ex2+=ex1;
	  if(tolower(type)=='g')
	    ex2--;
	  fparg.dbl+=.5/pow(10,ex2<MINFLOATSIZE?ex2:MINFLOATSIZE); /* Round up */

	  if(fparg.dbl>=10.0) /* Adjusts log10(10.)=.999999999 too */
	  { fparg.dbl/=10.0;
	    ex1++; }

	  if(tolower(type)=='g') /* This changes to one of the other types */
	  { if(ex1<(signed long)preci&&ex1>=-4)
	    { type='f';
	      preci-=ex1;
	    }else
	      type=type=='g'?'e':'E';
	    preci--;
	    if(!(flags&ALTERNATEFLAG))
	      killzeros=1; /* set flag to kill trailing zeros */
	  }

	  dreq=preci+1; /* Calculate number of decimal places required */
	  if(type=='f')
	    dreq+=ex1;	 /* even more before the decimal point */

	  dnum=0;
	  while(dnum<dreq&&dnum<MINFLOATSIZE) /* Calculate all decimal places needed */
	  { buffer[dnum++]=(char)fparg.dbl+'0';
	    fparg.dbl=(fparg.dbl-(double)(char)fparg.dbl)*10.0; }

	  if(killzeros) /* Kill trailing zeros if possible */
	    while(preci&&(dreq-->dnum||buffer[dreq]=='0'))
	      preci--;

	  if(type=='f')/* Calculate actual size of string (without sign) */
	  { size=preci+1; /* numbers after decimal point + 1 before */
	    if(ex1>0)
	      size+=ex1; /* numbers >= 10 */
	    if(preci||flags&ALTERNATEFLAG)
	      size++; /* 1 for decimal point */
	  }else
	  { size=preci+5; /* 1 for the number before the decimal point, and 4 for the exponent */
	    if(preci||flags&ALTERNATEFLAG)
	      size++;
	    if(ex1>99||ex1<-99)
	      size++; /* exponent needs an extra decimal place */
	  }

	  pad=size+(sign!=0);
	  pad=pad>=width?0:width-pad;

	  if(sign&&flags&ZEROPADFLAG)
	    OUT(sign);

	  if(!(flags&LALIGNFLAG))
	    for(i=0;i<pad;i++)
	      OUT(flags&ZEROPADFLAG?'0':' ');

	  if(sign&&!(flags&ZEROPADFLAG))
	    OUT(sign);

	  dreq=0;
	  if(type=='f')
	  { if(ex1<0)
	      OUT('0');
	    else
	      while(ex1>=0)
	      { OUT(dreq<dnum?buffer[dreq++]:'0');
		ex1--; }
	    if(preci||flags&ALTERNATEFLAG)
	    { OUT(__decimalpoint[0]);
	      while(preci--)
		if(++ex1<0)
		  OUT('0');
		else
		  OUT(dreq<dnum?buffer[dreq++]:'0');
	    }
	  }else
	  { OUT(buffer[dreq++]);
	    if(preci||flags&ALTERNATEFLAG)
	    { OUT(__decimalpoint[0]);
	      while(preci--)
		OUT(dreq<dnum?buffer[dreq++]:'0');
	    }
	    OUT(type);
	    if(ex1<0)
	    { OUT('-');
	      ex1=-ex1; }
	    else
	      OUT('+');
	    if(ex1>99)
	      OUT(ex1/100+'0');
	    OUT(ex1/10%10+'0');
	    OUT(ex1%10+'0');
	  }

	  if(flags&LALIGNFLAG)
	    for(i=0;i<pad;i++)
	      OUT(' ');

	  width=preci=0; /* Everything already done */
	  break;
	}
#endif
	case '%':
	  buffer2="%";
	  size2=1;
	  preci=0;
	  break;
	case 'n':
	  *va_arg(args,int *)=outcount;
	  width=preci=0;
	  break;
	default:
	  if(!type)
	    ptr--; /* We've gone too far - step one back */
	  buffer2=(char *)format;
	  size2=ptr-format;
	  width=preci=0;
	  break;
      }
      pad=size1+(size2>=preci?size2:preci); /* Calculate the number of characters */
      pad=pad>=width?0:width-pad; /* and the number of resulting pad bytes */

      if(flags&ZEROPADFLAG) /* print sign and that like */
	for(i=0;i<size1;i++)
	  OUT(buffer1[i]);

      if(!(flags&LALIGNFLAG)) /* Pad left */
	for(i=0;i<pad;i++)
	  OUT(flags&ZEROPADFLAG?'0':' ');

      if(!(flags&ZEROPADFLAG)) /* print sign if not zero padded */
	for(i=0;i<size1;i++)
	  OUT(buffer1[i]);

      for(i=size2;i<preci;i++) /* extend to precision */
	OUT('0');

      for(i=0;i<size2;i++) /* print body */
	OUT(buffer2[i]);

      if(flags&LALIGNFLAG) /* Pad right */
	for(i=0;i<pad;i++)
	  OUT(' ');

      format=ptr;
    }
    else
      OUT(*format++);
  }
  return outcount;
}

