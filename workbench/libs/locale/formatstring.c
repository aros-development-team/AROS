/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <stdlib.h>
#include <exec/types.h>
#include <utility/hooks.h>
#include <proto/utility.h>
#include <libraries/locale.h>
#include <aros/asmcall.h>
#include "locale_intern.h"

#include <aros/debug.h>

char hexarray [] = "0123456789abcdef";
char HEXarray [] = "0123456789ABCDEF";

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH4(APTR, FormatString,

/*  SYNOPSIS */
	AROS_LHA(struct Locale *, locale, A0),
	AROS_LHA(STRPTR         , fmtTemplate, A1),
	AROS_LHA(APTR           , dataStream, A2),
	AROS_LHA(struct Hook   *, putCharFunc, A3),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 11, Locale)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct Library *,LocaleBase)

  enum {OUTPUT = 0,
        FOUND_FORMAT};
  
  ULONG template_pos = 0;      /* Current position in the template string */
  ULONG state        = OUTPUT; /* current state of parsing */
  BOOL  end          = FALSE;
  ULONG max_argpos   = 0;
  ULONG arg_counter  = 0;
  
  ULONG * stream     = (ULONG *)dataStream;
  BOOL  scanning     = TRUE;   /* The first time I will go through
                                  and determine the width of the data in the dataStream */

#define INDICES 256                                  
  UWORD indices[INDICES];


  if (NULL == fmtTemplate)
    return (APTR)stream;

  memset(indices, 0, INDICES*sizeof(UWORD));

  while (FALSE == end)
  {
    /*
    ** A format description starts here?
    */
    if ('%' == fmtTemplate[template_pos])
    {
      arg_counter++;
      state = FOUND_FORMAT;
    }
    
    switch (state)
    {
      case OUTPUT:
        /*
        ** Call the hook for this character
        */
        if (FALSE == scanning)
        {
          AROS_UFC3(VOID, putCharFunc->h_Entry,
            AROS_UFCA(struct Hook *,   putCharFunc,                A0),
            AROS_UFCA(struct Locale *, locale,                     A2),
            AROS_UFCA(char,            fmtTemplate[template_pos],  A1));
        }

        /*
        ** End of template string? -> End of this function.
        */
        if ('\0' == fmtTemplate[template_pos])
        {
          if (TRUE == scanning)
          {
            /* 
            ** The scanning phase is over. Next time we do the output.
            */
            int i, sum;
            scanning = FALSE;
            template_pos = 0;
            arg_counter = 0;
            /*
            ** prepare the indices array
            */
            sum = indices[0];
            indices[0] = 0;

            i = 1;
            
            while (i <= max_argpos)
            {
              int _sum;
              if (0 != indices[i])
                _sum =  sum + indices[i];
              else
                _sum =  sum + 4;
                
              indices[i] =  sum;
              sum        = _sum;
              i++;
            }
          }
          else
          { 
            /*
            ** We already went through the output phase. So this is
            ** the end of it.
            */
            end = TRUE;
          }
        }
        else
          template_pos++;

        //kprintf("OUTPUT: template_pos: %d\n",template_pos);

      break;
      
      case FOUND_FORMAT:
        /*
        ** The '%' was found in the template string
        */
        template_pos++;

        //kprintf("FOUND_FORMAT: template_pos: %d\n",template_pos);
        /*
        ** Does the user want the '%' to be printed?
        */
        if ('%' == fmtTemplate[template_pos])
        {
          if (FALSE == scanning)
          {
            AROS_UFC3(VOID, putCharFunc->h_Entry,
              AROS_UFCA(struct Hook *  , putCharFunc,                A0),
              AROS_UFCA(struct Locale *, locale,                     A2),
              AROS_UFCA(char,            fmtTemplate[template_pos],  A1));
          }
          template_pos++;
	  arg_counter--; //stegerg
        }
        else
        {
          /*
          ** Now parsing...
          ** Template format: %[arg_pos$][flags][width][.limit][length]type
          **
          ** arg_pos specifies the position of the argument in the dataStream
          ** flags   only '-' is allowd
          ** width   
          ** .limit
          ** length  if 'l' is found some datatyes are 32 bit
          ** type    b,d,D,u,U,x,X,s,c
          */
          ULONG arg_pos      = 1;
          BOOL  left         = FALSE;  // no flag was found
          char  fill         = ' '; 
          ULONG minus        = 0;
          ULONG minwidth     = 0;
          ULONG maxwidth     = ~0;
          ULONG width        = 0;
          BOOL  length_found = FALSE;
          ULONG	tmp          = 0;
#define BUFFERSIZE 128
          char  buf[BUFFERSIZE];
          char  *buffer      = buf;
          
          /*
          ** arg_pos 
          */
          
          //kprintf("next char: %c\n",fmtTemplate[template_pos]);
          
          if ('0' <= fmtTemplate[template_pos] &&
              '9' >= fmtTemplate[template_pos])
          {
            ULONG old_template_pos = template_pos;
            
            arg_pos = 0;
            while ('0' <= fmtTemplate[template_pos] &&
                   '9' >= fmtTemplate[template_pos])
              arg_pos = arg_pos * 10 + fmtTemplate[template_pos++] - '0';
            
            if ('$' == fmtTemplate[template_pos])
              template_pos++;
            else
            {
              arg_pos = arg_counter;
              template_pos = old_template_pos;
            }
          }
          else
            arg_pos = arg_counter;
          
          /*
          ** flags
          */
          if ('-' == fmtTemplate[template_pos])
          {
            template_pos ++;
            left = TRUE;
          }
          
          /*
          ** fill character a '0'?
          */
          if ('0' == fmtTemplate[template_pos])
          {
            template_pos++;
            fill = '0';
          }
          
          /*
          ** width
          */
          if ('0' <= fmtTemplate[template_pos] &&
              '9' >= fmtTemplate[template_pos])
          {
            minwidth = 0;
            while ('0' <= fmtTemplate[template_pos] &&
                   '9' >= fmtTemplate[template_pos])
              minwidth = minwidth * 10 + fmtTemplate[template_pos++] - '0';
          }

          /*
          ** limit
          */
          if ('.' == fmtTemplate[template_pos])
          {
            template_pos++;
	    if ('0' <= fmtTemplate[template_pos] &&
	        '9' >= fmtTemplate[template_pos])
	    {
              maxwidth = 0;

              while ('0' <= fmtTemplate[template_pos] &&
                     '9' >= fmtTemplate[template_pos])
        	maxwidth = maxwidth * 10 + fmtTemplate[template_pos++] - '0';
    	    }
          }
          
          /*
          ** Length
          */
          if ('l' == fmtTemplate[template_pos])
          {
            length_found = TRUE;
            template_pos ++;
          }
        
          /*
          ** Print it according to the given type info.
          */
          switch (fmtTemplate[template_pos])
          {
            case 'b': /* BSTR, see autodocs */
              /*
              ** Important parameters:
              ** arg_pos, left, width, limit
              */
              if (FALSE == scanning)
              {
                buffer = (char *)BADDR(*(char **)(((IPTR)stream)+indices[arg_pos-1]));
                
                width = *buffer++;
                
                if (width > maxwidth)
                  width = maxwidth;
              }
              else
              {
                indices[arg_pos-1] = 4;
              }
            break;
           
            case 'd': /* signed decimal */
            case 'u': /* unsigned decimal */
              if (FALSE == scanning)
              {
                if (FALSE == length_found)
                {
                  tmp = *(UWORD *)(((IPTR)stream)+indices[arg_pos-1]);
                  buffer = &buf[4+1];
                }
                else
                {  
                  tmp = *(ULONG *)(((IPTR)stream)+indices[arg_pos-1]);
                  buffer = &buf[8+1];
                }
                
                if ((LONG) tmp < 0 && 'd' == fmtTemplate[template_pos])
                {
                  minus = 1;
                  tmp = -tmp;
                }
                
                buffer = &buf[BUFFERSIZE];
                do
                {
                  *--buffer=(tmp%10) + '0';
                  tmp /= 10;
                  width++;
                }
                while(0 != tmp);
                
              }
              else
              {
                if (TRUE == length_found)
                  indices[arg_pos-1] = 4;
                else
                  indices[arg_pos-1] = 2;
              }
            break;
           
            case 'D': /* signed decimal with locale's formatting conventions */
            case 'U': /* unsigned decimal with locale's formatting conventions */
              if (FALSE == scanning)
              {
                UBYTE groupsize;
                ULONG group_index = 0;
                if (FALSE == length_found)
                {
                  tmp = *(UWORD *)(((IPTR)stream)+indices[arg_pos-1]);
                }
                else
                {  
                  tmp = *(ULONG *)(((IPTR)stream)+indices[arg_pos-1]);
                }
                if ((LONG) tmp < 0 && 'D' == fmtTemplate[template_pos])
                {
                  minus = 1;
                  tmp = -tmp;
                }
                
                /* BUFFERSIZE should be big enough to format a string
                ** according to locale's formatting conventions
                */
                buffer = &buf[BUFFERSIZE];

                if (NULL != locale)
                {
                  groupsize = locale->loc_Grouping[group_index];
                }
                else
                  groupsize = 255;

                
                do
                {
                  *--buffer=(tmp%10) + '0';
                  tmp /= 10;
                  width++;

                  groupsize--;

                  if (0 == groupsize && 0 != tmp)
                  {
                    /*
                    ** Write the separator
                    */
                    
                    *--buffer = locale->loc_GroupSeparator[group_index];
                  
                    groupsize = locale->loc_Grouping[group_index+1];
                  
                    if (0 == groupsize)
                    { 
                      /*
                      ** Supposed to use the previous element
                      */
                      groupsize = locale->loc_Grouping[group_index];
                    }
                    else
                    {
                      group_index++;
                    }
                        
                    width++;
                  }
                }
                while(0 != tmp);
                
              }
              else
              {
                if (TRUE == length_found)
                  indices[arg_pos-1] = 4;
                else
                  indices[arg_pos-1] = 2;
              }
            break;
           
            case 'x': /* upper case hexadecimal string */
            case 'X': /* lower case hexadecimal string */

              if (FALSE == scanning)
              {

                if (FALSE == length_found)
                {
                  tmp = *(UWORD *)(((IPTR)stream)+indices[arg_pos-1]);
                  buffer = &buf[4+1];
                }
                else
                {  
                  tmp = *(ULONG *)(((IPTR)stream)+indices[arg_pos-1]);
                  buffer = &buf[8+1];
                }
                
                if ('X' == fmtTemplate[template_pos] )
                {
                  do
                  {
                    *--buffer = hexarray[tmp&0x0f];
                    tmp >>= 4;
                    width++;
                  }
                  while (tmp);
                }
                else
                {
                  do
                  {
                    *--buffer = HEXarray[tmp&0x0f];
                    tmp >>= 4;
                    width++;
                  }
                  while (tmp);
                }
              }
              else
              {
                if (TRUE == length_found)
                  indices[arg_pos-1] = 4;
                else
                  indices[arg_pos-1] = 2;
              }
            break;
           
            case 's': /* NULL terminated string */
            {
              if (FALSE == scanning)
              {
                buffer = *(char **)(((IPTR)stream)+indices[arg_pos-1]);
                width = strlen(buffer);
                
                if (width > maxwidth)
                  width = maxwidth;
              }
              else
              {
                indices[arg_pos-1] = 4; /* the pointer has 4 bytes */
              }
            }
            break;
           
            case 'c': /* Character */
              if (FALSE == scanning)
              {
                if (TRUE == length_found)
                  buf[0] = (char)*(ULONG *)(((IPTR)stream)+indices[arg_pos-1]);
                else
                  buf[0] = (char)*(WORD  *)(((IPTR)stream)+indices[arg_pos-1]);

                width = 1;
              }
              else
              {
                if (TRUE == length_found)
                  indices[arg_pos-1] = 4;
                else
                  indices[arg_pos-1] = 2;
              }
            break;
         
            default:
              goto error_exit;
          }


          if (FALSE == scanning)
          { 
            int i;

            /*
               Now everything I need is known:
               buffer  - contains the string to be printed
               width   - size of the string
               minus   - is 1 if there is a '-' to print
               fill    - the pad character
               left    - is 1 if the string should be left aligned
               minwidth- is the minimal width of the field
            */
            
            if (FALSE == left)
              for(i = width + minus; i < minwidth; i++)
                AROS_UFC3(VOID, putCharFunc->h_Entry,
                  AROS_UFCA(struct Hook *,   putCharFunc         , A0),
                  AROS_UFCA(struct Locale *, locale              , A2),
                  AROS_UFCA(char,            fill                , A1)
                );

            if (minus)
              AROS_UFC3(VOID, putCharFunc->h_Entry,
                AROS_UFCA(struct Hook *,   putCharFunc         , A0),
                AROS_UFCA(struct Locale *, locale              , A2),
                AROS_UFCA(char,            '-'                 , A1)
              );
            
            /* Print body up to width */
            for (i = 0; i < width; i++)
            {
              AROS_UFC3(VOID, putCharFunc->h_Entry,
                AROS_UFCA(struct Hook *,   putCharFunc         , A0),
                AROS_UFCA(struct Locale *, locale              , A2),
                AROS_UFCA(char,            *buffer++           , A1)
              );
            }
            
            /* Pad right if left aligned */
            if (TRUE == left)
              for (i = width+minus; i < minwidth; i++)
                AROS_UFC3(VOID, putCharFunc->h_Entry,
                  AROS_UFCA(struct Hook *,   putCharFunc         , A0),
                  AROS_UFCA(struct Locale *, locale              , A2),
                  AROS_UFCA(char,            fill                , A1)
                );
          }

          template_pos++;
            
          if (arg_pos > max_argpos)
            max_argpos = arg_pos;
          
        }  
        state = OUTPUT;
      break;
    }
  }
  

error_exit:

  return (APTR)(stream+indices[max_argpos]);

  AROS_LIBFUNC_EXIT
} /* FormatString */
