/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <utility/hooks.h>
#include <proto/utility.h>
#include <libraries/locale.h>
#include <aros/asmcall.h>
#include "locale_intern.h"

#include <aros/debug.h>

#if USE_QUADFMT
typedef QUAD  FMTLARGESTTYPE;
typedef UQUAD UFMTLARGESTTYPE;
#else  /* USE_QUADFMT */
typedef LONG  FMTLARGESTTYPE;
typedef ULONG UFMTLARGESTTYPE;
#endif /* USE_QUADFMT */

static const UBYTE hexarray [] = "0123456789abcdef";
static const UBYTE HEXarray [] = "0123456789ABCDEF";

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH4(APTR, FormatString,

/*  SYNOPSIS */
	AROS_LHA(const struct Locale *, locale, A0),
	AROS_LHA(CONST_STRPTR, fmtTemplate, A1),
	AROS_LHA(CONST_APTR           , dataStream, A2),
	AROS_LHA(const struct Hook   *, putCharFunc, A3),

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

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  enum {OUTPUT = 0,
        FOUND_FORMAT} state;

  ULONG template_pos;
  BOOL  end;
  ULONG max_argpos;
  ULONG arg_counter;
  ULONG *stream;
  BOOL  scanning;

#define INDICES 256
  UWORD indices[INDICES];

  if (!fmtTemplate)
    return (APTR)dataStream;

  template_pos = 0;      /* Current position in the template string */
  state        = OUTPUT; /* current state of parsing */
  end          = FALSE;
  max_argpos   = 0;
  arg_counter  = 0;
  stream       = (ULONG *) dataStream;
  scanning     = TRUE;   /* The first time I will go through
                                  and determine the width of the data in the dataStream */

#ifdef __MORPHOS__
  memclr(indices, sizeof(indices));
#else
  memset(indices, 0, sizeof(indices));
#endif

  while (!end)
  {
    /*
    ** A format description starts here?
    */
    if (fmtTemplate[template_pos] == '%')
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
        if (!scanning)
        {
          AROS_UFC3(VOID, putCharFunc->h_Entry,
            AROS_UFCA(const struct Hook *,   putCharFunc,                A0),
            AROS_UFCA(const struct Locale *, locale,                     A2),
            AROS_UFCA(UBYTE,           fmtTemplate[template_pos],  A1));
        }

        /*
        ** End of template string? -> End of this function.
        */
        if (fmtTemplate[template_pos] == '\0')
        {
          if (scanning)
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
              if (indices[i] != 0)
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
        if (fmtTemplate[template_pos] == '%')
        {
          if (!scanning)
          {
            AROS_UFC3(VOID, putCharFunc->h_Entry,
              AROS_UFCA(const struct Hook *  , putCharFunc,                A0),
              AROS_UFCA(const struct Locale *, locale,                     A2),
              AROS_UFCA(UBYTE,           fmtTemplate[template_pos],  A1));
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
          ** datasize size of the datatype
          ** type    b,d,D,u,U,x,X,s,c
          */
          ULONG arg_pos      = 1;
          BOOL  left         = FALSE;  // no flag was found
          UBYTE fill         = ' ';
          ULONG minus;
          ULONG width        = 0;
          ULONG limit        = ~0;
          ULONG buflen        = 0;
          ULONG datasize;
          UFMTLARGESTTYPE tmp= 0;
#define BUFFERSIZE 128
          UBYTE buf[BUFFERSIZE];
          UBYTE *buffer      = buf;

          /*
          ** arg_pos
          */

          //kprintf("next char: %c\n",fmtTemplate[template_pos]);

          if (fmtTemplate[template_pos] >= '0' &&
              fmtTemplate[template_pos] <= '9')
          {
            ULONG old_template_pos = template_pos;

            for (arg_pos = 0; (fmtTemplate[template_pos] >= '0' &&
                               fmtTemplate[template_pos] <= '9'); template_pos++)
            {
              arg_pos = arg_pos * 10 + fmtTemplate[template_pos] - '0';
            }

            if (fmtTemplate[template_pos] == '$')
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
          if (fmtTemplate[template_pos] == '-')
          {
            template_pos++;
            left = TRUE;
          }

          /*
          ** fill character a '0'?
          */
          if (fmtTemplate[template_pos] == '0')
          {
            template_pos++;
            fill = '0';
          }

          /*
          ** width
          */
          if (fmtTemplate[template_pos] >= '0' &&
              fmtTemplate[template_pos] <= '9')
          {
            for (width = 0; (fmtTemplate[template_pos] >= '0' &&
                                fmtTemplate[template_pos] <= '9'); template_pos++)
            {
              width = width * 10 + fmtTemplate[template_pos] - '0';
            }
          }

          /*
          ** limit
          */
          if (fmtTemplate[template_pos] == '.')
          {
            template_pos++;

	    if (fmtTemplate[template_pos] >= '0' &&
	        fmtTemplate[template_pos] <= '9')
	    {
              for (limit = 0; (fmtTemplate[template_pos] >= '0' &&
                                  fmtTemplate[template_pos] <= '9'); template_pos++)
              {
        	limit = limit * 10 + fmtTemplate[template_pos] - '0';
              }
    	    }
          }

          /*
          ** Length
          */
          switch (fmtTemplate[template_pos])
          {
#if USE_QUADFMT
            case 'L':
              datasize = 8;
              template_pos++;
              break;
#endif /* USE_QUADFMT */

            case 'l':
              template_pos++;
#if USE_QUADFMT
              if (fmtTemplate[template_pos] == 'l')
              {
                datasize = 8;
                template_pos++;
              }
              else
#endif /* USE_QUADFMT */
                datasize = 4;
              break;

            default:
              datasize = 2;
              break;
          }

          /*
          ** Print it according to the given type info.
          */
          switch (fmtTemplate[template_pos])
          {
            case 'b': /* BSTR, see autodocs */
              /*
              ** Important parameters:
              ** arg_pos, left, buflen, limit
              */
              if (!scanning)
              {
                BSTR s = (BSTR)*(UBYTE **)(((IPTR)stream)+indices[arg_pos-1]);

                buffer = AROS_BSTR_ADDR(s);
                buflen = AROS_BSTR_strlen(s);

#if !USE_GLOBALLIMIT
                if (buflen > limit)
                  buflen = limit;
#endif /* !USE_GLOBALLIMIT */
              }
              else
                indices[arg_pos-1] = sizeof(BPTR);
            break;

            case 'd': /* signed decimal */
            case 'u': /* unsigned decimal */

              minus = fmtTemplate[template_pos] == 'd';

              if (!scanning)
              {
                switch (datasize)
                {
#if USE_QUADFMT
                  case 8:
                    tmp = *(UQUAD *)(((IPTR)stream)+indices[arg_pos-1]);
                    //buffer = &buf[16+1];
                    minus *= (FMTLARGESTTYPE) tmp < 0;
                    if (minus)
                      tmp = -tmp;
                    break;
#endif /* USE_QUADFMT */

                  case 4:
                    tmp = *(ULONG *)(((IPTR)stream)+indices[arg_pos-1]);
                    //buffer = &buf[8+1];
                    minus *= (LONG) tmp < 0;
                    if (minus)
                      tmp = (ULONG) -tmp;
                    break;

                  default: /* 2 */
                    tmp = *(UWORD *)(((IPTR)stream)+indices[arg_pos-1]);
                    //buffer = &buf[4+1];
                    minus *= (WORD) tmp < 0;
                    if (minus)
                      tmp = (UWORD) -tmp;
                    break;
                }

                buffer = &buf[BUFFERSIZE];
                do
                {
                  *--buffer = (tmp % 10) + '0';
                  tmp /= 10;
                  buflen++;
                }
                while (tmp);

                if (minus)
                {
                  *--buffer = '-';
                  buflen++;
                }

              }
              else
                indices[arg_pos-1] = datasize;
            break;

            case 'D': /* signed decimal with locale's formatting conventions */
            case 'U': /* unsigned decimal with locale's formatting conventions */
              if (!scanning)
              {
                UBYTE groupsize;
                ULONG group_index = 0;

                minus = fmtTemplate[template_pos] == 'D';

                switch (datasize)
                {
#if USE_QUADFMT
                  case 8:
                    tmp = *(UQUAD *)(((IPTR)stream)+indices[arg_pos-1]);
                    minus *= (FMTLARGESTTYPE) tmp < 0;
                    if (minus)
                      tmp = -tmp;
                    break;
#endif /* USE_QUADFMT */

                  case 4:
                    tmp = *(ULONG *)(((IPTR)stream)+indices[arg_pos-1]);
                    minus *= (LONG) tmp < 0;
                    if (minus)
                      tmp = (ULONG) -tmp;
                    break;

                  default: /* 2 */
                    tmp = *(UWORD *)(((IPTR)stream)+indices[arg_pos-1]);
                    minus *= (WORD) tmp < 0;
                    if (minus)
                      tmp = (UWORD) -tmp;
                    break;
                }

                /* BUFFERSIZE should be big enough to format a string
                ** according to locale's formatting conventions
                */
                buffer = &buf[BUFFERSIZE];
                groupsize = locale ? locale->loc_Grouping[group_index] : 255;

                do
                {
                  *--buffer = (tmp % 10) + '0';
                  tmp /= 10;
                  buflen++;

                  groupsize--;

                  if (groupsize == 0 && tmp != 0)
                  {
                    /*
                    ** Write the separator
                    */

                    *--buffer = locale->loc_GroupSeparator[group_index];

                    groupsize = locale->loc_Grouping[group_index+1];

                    if (groupsize == 0)
                    {
                      /*
                      ** Supposed to use the previous element
                      */
                      groupsize = locale->loc_Grouping[group_index];
                    }
                    else
                      group_index++;

                    buflen++;
                  }
                }
                while (tmp);

                if (minus)
                {
                  *--buffer = '-';
                  buflen++;
                }
              }
              else
                indices[arg_pos-1] = datasize;
            break;

            case 'x': /* upper case hexadecimal string */
            case 'X': /* lower case hexadecimal string */
            case 'p': /* lower case pointer string */
            case 'P': /* upper case pointer string */

              if (!scanning)
              {
                const UBYTE *hexa;

                /* %p is always at least natural pointer size (32bit) */
                if (datasize < sizeof(void *) && (fmtTemplate[template_pos] == 'p' ||
                                                  fmtTemplate[template_pos] == 'P'))
                {
                  datasize = sizeof(void *);
                }

                switch (datasize)
                {
#if USE_QUADFMT
                  case 8:
                    tmp = *(UQUAD *)(((IPTR)stream)+indices[arg_pos-1]);
                    //buffer = &buf[16+1];
                    break;
#endif /* USE_QUADFMT */

                  case 4:
                    tmp = *(ULONG *)(((IPTR)stream)+indices[arg_pos-1]);
                    //buffer = &buf[8+1];
                    break;

                  default: /* 2 */
                    tmp = *(UWORD *)(((IPTR)stream)+indices[arg_pos-1]);
                    //buffer = &buf[4+1];
                    break;
                }

                buffer = &buf[BUFFERSIZE];

                /* NOTE: x/X is reverse to printf, coz orig RawDoFmt %lx for uppercase. */
                hexa = (fmtTemplate[template_pos] == 'X' ||
                        fmtTemplate[template_pos] == 'p') ? hexarray : HEXarray;
                do
                {
                  *--buffer = hexa[tmp&0x0f];
                  tmp >>= 4;
                  buflen++;
                }
                while (tmp);
              }
              else
                indices[arg_pos-1] = datasize;
            break;

            case 's': /* NULL terminated string */
            {
              if (!scanning)
              {
                buffer = *(UBYTE **)(((IPTR)stream)+indices[arg_pos-1]);
                if (!buffer)
                {
                    buffer = "(null)";
                    buflen = 7;
                }
                else
                    buflen = strlen(buffer);

#if !USE_GLOBALLIMIT
                if (buflen > limit)
                  buflen = limit;
#endif /* !USE_GLOBALLIMIT */
              }
              else
                indices[arg_pos-1] = sizeof(UBYTE *); /* the pointer has 4 bytes */
            }
            break;

            case 'c': /* Character */
              if (!scanning)
              {
                switch (datasize)
                {
#if USE_QUADFMT
                  case 8:
                    buf[0] = (UBYTE)*(UQUAD *)(((IPTR)stream)+indices[arg_pos-1]);
                    break;
#endif /* USE_QUADFMT */

                  case 4:
                    buf[0] = (UBYTE)*(ULONG *)(((IPTR)stream)+indices[arg_pos-1]);
                    break;

                  default: /* 2 */
                    buf[0] = (UBYTE)*(WORD  *)(((IPTR)stream)+indices[arg_pos-1]);
                    break;
                }

                buflen = 1;
              }
              else
                indices[arg_pos-1] = datasize;
            break;

            default:
              /* Ignore the faulty '%' */

              if (!scanning)
              {
                buf[0] = fmtTemplate[template_pos];
                width = 1;
                buflen = 1;
              }

              arg_pos = --arg_counter;
            break;
          }


          if (!scanning)
          {
            int i;

            /*
               Now everything I need is known:
               buffer  - contains the string to be printed
               buflen  - size of the string
               fill    - the pad character
               left    - is 1 if the string should be left aligned
               width   - is the minimal width of the field
               limit   - maximum number of characters to output from a string, default ~0
            */

#if USE_GLOBALLIMIT
            if (buflen > limit)
               buflen = limit;
#endif /* USE_GLOBALLIMIT */

            /* Print padding if right aligned */
            if (!left)
              for (i = buflen; i < width; i++)
                AROS_UFC3(VOID, putCharFunc->h_Entry,
                  AROS_UFCA(const struct Hook *,   putCharFunc         , A0),
                  AROS_UFCA(const struct Locale *, locale              , A2),
                  AROS_UFCA(UBYTE,           fill                , A1)
                );

            /* Print body up to buflen */
            for (i = 0; i < buflen; i++)
            {
              AROS_UFC3(VOID, putCharFunc->h_Entry,
                AROS_UFCA(const struct Hook *,   putCharFunc         , A0),
                AROS_UFCA(const struct Locale *, locale              , A2),
                AROS_UFCA(UBYTE,           *buffer++           , A1)
              );
            }

            /* Pad right if left aligned */
            if (left)
              for (i = buflen; i < width; i++)
                AROS_UFC3(VOID, putCharFunc->h_Entry,
                  AROS_UFCA(const struct Hook *,   putCharFunc         , A0),
                  AROS_UFCA(const struct Locale *, locale              , A2),
                  AROS_UFCA(UBYTE,           fill                , A1)
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

  return (APTR) (((IPTR)stream) + indices[max_argpos]);

  AROS_LIBFUNC_EXIT
} /* FormatString */
