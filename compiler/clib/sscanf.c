/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function sscanf()
    Lang: english
*/

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>


/*****************************************************************************

    NAME */

	int sscanf (

/*  SYNOPSIS */
	char        * str,
	const char  * format,
	...)

/*  FUNCTION
	Scan the specified string and convert it into the arguments as
	specified by format.

    INPUTS
	str     - The routine examines this string.
	format - Format string. See scanf() for a description
	...    - Arguments for the result

    RESULT
	The number of converted parameters.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fscanf(), vscanf(), vfscanf(), snscanf(), vsscanf(),
	vnsscanf()

    INTERNALS

    HISTORY
	06.12.1996 digulla created

******************************************************************************/

/* void _sscanf(char * str, const char * format, ... ) */
{
  #define TRUE 1
  #define FALSE 0
  
  long n = 0; /* Counter of the number of processed letters in
                 the input string s */

  char * s_end;
  char * s = str;
  char * format_end;
  long int result;
  double D_result;

  int maxwidth;
  int base;
  int size;
  int assignment;
  
  int retval = 0;

  va_list arg;

  va_start(arg, format);

  /* process the format string as long as there is a character */
  while (*format)
  {
    /* search for the first letter in the format string
       that is not a space */

    while ( isspace(*format) )
      format++;
    
    /* see if there is a command */
    if ('%' == *format)
    {
      /* Examine the next char */
      format++;

      /* Initialize some variables */
      maxwidth = -1;
      base = 10;
      size = 4;
      assignment = TRUE;



      /* Next there can be a number, a letter h,l or L or a * */
      switch (*format )
      {
        case 'h' : size = sizeof(short int);
                   format++;
                 break;

        case 'l' : size = sizeof(long int);
                   format++;
                 break;

        case 'L' : size = sizeof(long double);
                   format++;
                 break;

        case '*' : assignment = FALSE;
                 break;

        default:  /* as default try to find a number for the maxwidth
                     of the field */

      }



      /* now let us see for the format letter */
      switch (*format++)
      {
        case 'd' : /* let us look for a signed integer number in the string */
                 result = strtol(s, &s_end, 10);
                 n += (s_end - s);
                 s = s_end; /* Ptr to the rest of the string in s */

                 if (TRUE == assignment)
                   if (sizeof(short int) == size)
                     *va_arg(arg, short int *) = result;
                   else
                     *va_arg(arg, long  int *) = result;

                 retval++;
               break;



        case 'i' : /* let us look for a signed integer number that can have a
                      different base, i.e. hex or octal. Here we have
                      to examine the next few letters in the format string
                      for the base */
                   base = strtol(format, &format_end, 10);
                   format = format_end;
                   result = strtol(s, &s_end, base);
                   n += (s_end - s);
                   s = s_end; /* Ptr to the rest of the string in s */

                   if (TRUE == assignment)
                     if (sizeof(short int) == size)
                       *va_arg(arg, short int *) = result;
                     else
                       *va_arg(arg, long  int *) = result;

                   retval++;                     
                 break;

        case 'o' : /* let us read in a signed octal number */
                   base = 8;
                   result = strtol(s, &s_end, base);
                   n += (s_end - s);
                   s = s_end; /* Ptr to the rest of the string in s */
                   
                   if (TRUE == assignment)
                     if (sizeof(short int) == size)
                       *va_arg(arg, short int *) = result;
                     else
                       *va_arg(arg, long  int *) = result;
                       
                   retval++;
                 break;
                 
        case 'x' :
        case 'X' : /* let us read in a signed hexadecimal number */
                   base = 16;
                   result = strtol(s, &s_end, base);
                   n+= (s_end - s);
                   s = s_end; /* Ptr to the rest of the string in s */
                   
                   if (TRUE == assignment)
                     if (sizeof(short int) == size)
                       *va_arg(arg, short int *) = result;
                     else
                       *va_arg(arg, long  int *) = result;
                       
                   retval++;
                 break;

        case 'u' : /* let us read in an unsigned integer */
                   base = 10;
                   result = strtoul(s, &s_end, base);
                   n += (s_end - s);
                   s = s_end; /* Ptr to the rest of the string in s */
                   
                   if (TRUE == assignment)
                     if (sizeof(short int) == size)
                       *va_arg(arg, short int *) = result;
                     else
                       *va_arg(arg, long  int *) = result;
                       
                   retval++;
                 break;

        case 'c' : /* let us read in one single character */
                   /* skip whitespaces in s */
                   while (isspace(*s))
                   {
                     s++;
                     n++;
                   }
                   
                   if (TRUE == assignment)
                     *va_arg(arg, char *) = *s++;
                   n++;
                   retval++;
                 break;

        case 's' : /* let us read in a string until the next whitespace comes
                      along */
                   /* skip leading whitespaces in s */
                   while (isspace(*s))
                   {
                     s++;
                     n++;
                   }
                   /* s points to the start of the string */
                   s_end = s;
                   /* let us look for the end of the string in s*/
                   while (*s_end && isspace(*s_end))
                   {
                     s_end++;
                     n++;
                   }

                   /* s_end points to the end of the string */
                   
                   if(TRUE == assignment)
                   {
                     *va_arg(arg, char *) = '\0' ; 
                       /* set the first character in the argument
                          string to zero */
                     strncat((char *)arg, s_end, (long)s_end-(long)s);
                   }
                   
                   retval++;
                 break;

        case 'e' :
        case 'E' :
        case 'f' :
        case 'g' :
        case 'G' : /* a real number with optional sign, opt. decimal point and
                      optional exponent */

                   D_result = strtod(s, &s_end);
                   
                   if (TRUE == assignment)
                     *va_arg(arg, double *) = D_result;
                   n += (long)(s_end - s);
                   s = s_end;
                   
                   retval++;
                 break;
        case 'n' : /* the user wants to know how many letters we already
                      processed on the input (not format!!) string. So
                      we give hime the content of letter variable n */
                   if (TRUE == assignment)
                     *va_arg(arg, long *) = n;
                     
                   retval++;
                 break;

        }
    }
  }
  va_end(args);
  return retval;
}  

/*

#define Test_sscanf1(buffer, format, res1, res2, output) \
   sscanf(buffer, format, &res1); \
  _sscanf(buffer, format, &res2); \
  printf(output,res1,res2); \
  printf("\n");  

#define Test_sscanf2(buffer, format, res11, res12, res21, res22, out1, out2) \
   sscanf(buffer, format, &res11, &res12); \
  _sscanf(buffer, format, &res21, &res22); \
  printf(out1, res11, res21); \
  printf(out2, res12, res22); \
  printf("\n");  

#define Test_sscanf3(buffer, format, res11, res12, res13, res21, res22, res23, out1, out2, out3) \
   sscanf(buffer, format, &res11, &res12, &res13); \
  _sscanf(buffer, format, &res21, &res22, &res23); \
  printf(out1, res11, res21); \
  printf(out2, res12, res22); \
  printf(out3, res13, res23); \
  printf("\n"); 

  
void main(void)
{
  short int si1,si2;
  long int li1,li2;
  double d11,d12,d21,d22;
  Test_sscanf1("100","%hi", si1, si2, "%i = %i\n");
  Test_sscanf1(" 100","%hi",si1, si2, "%i = %i\n");
 
  Test_sscanf1("123456789","%li", li1, li2, "%i = %i\n");
  Test_sscanf1("1.234","%le", d11, d21, "%f = %f\n");
  Test_sscanf1("1.234","%lE", d11, d21, "%f = %f\n");
  
  Test_sscanf2("100 200","%hi %li", si1, li1, si2, li2, "%i = %i\n", "%i = %i\n");

  Test_sscanf2("1.234E1 0.5E2","%le %le", d11, d12, d21, d22, "%e = %e\n", "%f = %f\n");
  Test_sscanf3("1.234E1 1234","%le %hi %n", d11, si1, li1, d21, si2, li2, "%e = %e\n", "%i = %i\n","%i = %i\n");
}

*/