/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sscanf().
*/

#define sscanf sscanf

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>


/*****************************************************************************

    NAME */

	int sscanf (

/*  SYNOPSIS */
	const char  *str,
	const char  *format,
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
	fscanf(), vscanf(), vfscanf(), vsscanf()

    INTERNALS

******************************************************************************/
/*
  int _sscanf(char * str, const char * format, ... ) 
*/
{
#if 1
    int     retval;
    va_list args;

    va_start(args, format);
    retval = vsscanf(str, format, args);
    va_end(args);
    
    return retval;
  
#else

  #define TRUE 1
  #define FALSE 0
  
  int n = 0; /* Counter of the number of processed letters in
                 the input string s */

  char * s_end;
  char * s = str;
  char * format_end;
  long int result;
#ifndef AROS_NOFPU
  double D_result;
#endif

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
      
      /* The next char can be a '*' */

      if ('*' == *format)
      {
        /* No assignment to variable */
        assignment = FALSE;
        /* Advance to next character */
        format++;
      }
      else
      {
        assignment = TRUE;
      }

      /* Next there can be a number, a letter h,l or L or a * */
      switch (*format)
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

      }



      /* now let us see for the format letter */
      switch (*format++)
      {
        case 'd' : /* let us look for a signed integer number in the string */
                 result = strtol(s, &s_end, 10);
                 n += (s_end - s);
                 s = s_end; /* Ptr to the rest of the string in s */

                 if (TRUE == assignment)
                 {
                   retval++;
                   if (sizeof(short int) == size)
                     *va_arg(arg, short int *) = result;
                   else
                     *va_arg(arg, long  int *) = result;
                 }

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
                   {
                     retval++;
                     if (sizeof(short int) == size)
                       *va_arg(arg, short int *) = result;
                     else
                       *va_arg(arg, long  int *) = result;
                   }

                 break;

        case 'o' : /* let us read in a signed octal number */
                   base = 8;
                   result = strtol(s, &s_end, base);
                   n += (s_end - s);
                   s = s_end; /* Ptr to the rest of the string in s */
                   
                   if (TRUE == assignment)
                   {
                     retval++;
                     if (sizeof(short int) == size)
                       *va_arg(arg, short int *) = result;
                     else
                       *va_arg(arg, long  int *) = result;
                   }
                       
                 break;
                 
        case 'x' :
        case 'X' : /* let us read in a signed hexadecimal number */
                   base = 16;
                   result = strtol(s, &s_end, base);
                   n+= (s_end - s);
                   s = s_end; /* Ptr to the rest of the string in s */
                   
                   if (TRUE == assignment)
                   {
                     retval++;
                     if (sizeof(short int) == size)
                       *va_arg(arg, short int *) = result;
                     else
                       *va_arg(arg, long  int *) = result;
                   }
                       
                 break;

        case 'u' : /* let us read in an unsigned integer */
                   base = 10;
                   result = strtoul(s, &s_end, base);
                   n += (s_end - s);
                   s = s_end; /* Ptr to the rest of the string in s */
                   
                   if (TRUE == assignment)
                   {
                     retval++;
                     if (sizeof(short int) == size)
                       *va_arg(arg, short int *) = result;
                     else
                       *va_arg(arg, long  int *) = result;
                   }
                       
                 break;

        case 'c' : /* let us read in one single character */
                   /* do not skip whitespaces in s */
                   
                   if (TRUE == assignment)
                   {
                     retval++;
                     *va_arg(arg, char *) = *s++;
                   }
                   
                   n++;
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
                   /* let us look for the end of the string in s */
                   while (*s_end && isalpha(*s_end))
                   {
                     s_end++;
                     n++;
                   }

                   /* s_end points to the end of the string */
                   
                   if(TRUE == assignment)
                   {
                     char * dest = va_arg(arg, char *);
                     retval++;
                     strncpy(dest, s, (long)s_end-(long)s);
                     *(dest+((long)s_end-(long)s))='\0';
                   }
                   s = s_end;
                 break;

#ifndef AROS_NOFPU
        case 'e' :
        case 'E' :
        case 'f' :
        case 'g' :
        case 'G' : /* a real number with optional sign, opt. decimal point and
                      optional exponent */

                   D_result = strtod(s, &s_end);
                   
                   if (TRUE == assignment)
                   {
                     retval++;
                     *va_arg(arg, double *) = D_result;
                   }
                   
                   n += (long)(s_end - s);
                   s = s_end;
                   
                 break;
#endif                 
        case 'n' : /* the user wants to know how many letters we already
                      processed on the input (not format!!) string. So
                      we give hime the content of letter variable n */
                   if (TRUE == assignment)
                   {
                     /* NO retval++; here!! */
                     
                     *va_arg(arg, long *) = n;
                   }
                     
                 break;

        default : /* no known letter -> error!! */
                 return retval;
      }
    } /* if */
    else
      return retval;
  } /* while() */
  va_end(arg);
  return retval;

#endif

}  

/*

#define Test_sscanf1(buffer, format, res1, res2, output) \
  { \
    int retval1 =  sscanf(buffer, format, &res1); \
    int retval2 = _sscanf(buffer, format, &res2); \
    printf(output,res1,res2); \
    if (retval1 != retval2) printf("wrong returnvalue (%i!=%i)",retval1,retval2); \
    printf("\n"); \
  }  

#define Test_sscanfStr2(buffer, format, res11, res12, res21, res22, out1, out2) \
  { \
    int retval1 = _sscanf(buffer, format, res11, &res12); \
    int retval2 = _sscanf(buffer, format, res21, &res22); \
    printf(out1, res11, res21); \
    printf(out2, res12, res22); \
    if (retval1 != retval2) printf("wrong returnvalue (%i!=%i)",retval1,retval2); \
    printf("\n"); \
  }  



#define Test_sscanf2(buffer, format, res11, res12, res21, res22, out1, out2) \
  { \
    int retval1 =  sscanf(buffer, format, &res11, &res12); \
    int retval2 = _sscanf(buffer, format, &res21, &res22); \
    printf(out1, res11, res21); \
    printf(out2, res12, res22); \
    if (retval1 != retval2) printf("wrong returnvalue (%i!=%i)",retval1,retval2); \
    printf("\n"); \
  } 

#define Test_sscanf3(buffer, format, res11, res12, res13, res21, res22, res23, out1, out2, out3) \
  { \
    int retval1 =  sscanf(buffer, format, &res11, &res12, &res13); \
    int retval2 = _sscanf(buffer, format, &res21, &res22, &res23); \
    printf(out1, res11, res21); \
    printf(out2, res12, res22); \
    printf(out3, res13, res23); \
    if (retval1 != retval2) printf("wrong returnvalue (%i!=%i)",retval1,retval2); \
    printf("\n"); \
  } 

  
void main(void)
{
  short int si1,si2;
  long int li1,li2;
  double d11,d12,d21,d22;
  float f1,f2;
  char c11,c12,c21,c22;
  
  char * str1 = (char *) malloc(100);
  char * str2 = (char *) malloc(100);
  Test_sscanf1("100","%hi", si1, si2, "%i = %i (100)\n");
  Test_sscanf1(" 100","%hi",si1, si2, "%i = %i (100)\n");

  Test_sscanf1(" ABCDEF","%x",li1, li2, "%i = %i (100)\n");

  Test_sscanf1(" FEDCBA","%X",li1, li2, "%i = %i (100)\n");
 
  Test_sscanf1("123456789","%li", li1, li2, "%i = %i (123456789)\n");
  Test_sscanf1("1.234","%le", d11, d21, "%f = %f (1.234)\n");
  Test_sscanf1("1.234","%lE", d11, d21, "%f = %f (1.234)\n");

  Test_sscanf2("100 200","%hi %li", si1, li1, 
                                    si2, li2, "%i = %i (100)\n", "%i = %i (200)\n");

  Test_sscanf2(" 1","%c%c", c11, c12, 
                            c21, c22, "%c = %c\n", "%c = %c\n");

  Test_sscanf3("AC","%c%c%n", c11, c12, li1,
                              c21, c22, li2, "%c = %c\n", "%c = %c\n", "%i = %i\n");


  Test_sscanf2("1.234E1 0.5E2","%le %le", d11, d12, 
                                          d21, d22, "%e = %e\n", "%f = %f\n");
  
  si1=0;si2=0;li1=0;li2=0;
  Test_sscanf3("1.234E1 1234","%le%n%hi", d11, li1, si1, 
                                          d21, li2, si2, "%e = %e\n", "%i = %i\n","%i = %i\n");
  
  Test_sscanf2("100 1111","%*hi %li", si1, li1, 
                                      si2, li2, "%i = %i (should NOT be 100 )\n","%i = %i (1111)\n");
  
  Test_sscanfStr2("ABCDEFGH 23","%s %li", str1, li1, 
                                          str2, li2,"%s = %s (ABCDEFGH)\n","%i = %i\n");

  free(str1);	  
  free(str2);
}

*/

