/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, EscapeString,

/*  SYNOPSIS */ 
      AROS_LHA(STRPTR, String, A0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 44, Arp)

/*  NAME
        EscapeString -- convert escape characters in string.
 
    SYNOPSIS
        NewLength = EscapeString( "String" )
            D0                       A0
 
    FUNCTION
        This function scans the string, replacing all escaped characters
        with the correct byte value.  This function uses the value for ESCAPE
        set in ESCChar in ArpBase, which is maintained elsewhere; this value
 	      defaults to the BCPL '*' (RSN), or may be the normal '\'.
 
        This function currently recognizes the following special characters:
 
            N   - newline
            T   - horizontal tab
            V   - vertical tab
            B   - backspace
            R   - return
            F   - formfeed
            E   - escape (ascii 27 decimal)
            Xnn - character representd by hex value nn.
 
        The above may be either upper or lower case.  If this function
        finds an escaped character which is not one of the above, it will
        return the character value (i.e. '\A' will be replaced by the single
        character 'A'. The sequence '\\' is replaced by the single
        character '\', and so on.)
 
        For sending hexcodes, the \x argument may be followed by either one
        or two hex digits, in either upper or lower case.
 
    INPUTS
        string - pointer to a null terminated ASCII string.
 
    RESULT
        A new string, starting at the same memory location, but with the
        escaped characters (if any) changed to their actual byte values.
        This string will be null terminated.
 
        NewCount -- The new length of the string.
 
    ADDITIONAL NOTES:
        For easy use in assembly language parsers, this function will
        return with the A0 register unchanged.
 
    BUGS
        Routine uses the region of memory pointed to by A0 as a work area,
        so if you wish to retain a pristine copy of the string, copy it first.
 
 
    SEE ALSO
        GADS

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  ULONG NewCount = 0, i = 0;
  
  /* process the input string until there is a NULL */
  while (String[i] != 0)
  {
    if (ArpBase->ESCChar == String[i])
    {
      i++;
      /* let me see what the next char is */
      switch (String[i++])
      {
        /* N,n */
        case 0x6e:
        case 0x4e: String[NewCount++] = 0x0a;
        break;
        
        /* T,t */
        case 0x74:
        case 0x54: String[NewCount++] = 0x09;
        break;

        /* V */
        case 0x76:
        case 0x56: String[NewCount++] = 0x0b;
        break;
        
        /* B */
        case 0x62:
        case 0x42: String[NewCount++] = 0x08;
        break;
        
        /* R */
        case 0x72:
        case 0x52: String[NewCount++] = 0x0d;
        break;
        
        /* F */
        case 0x66:
        case 0x46: String[NewCount++] = 0x0c;
        break;
        
        /* E */
        case 0x65:
        case 0x45: String[NewCount++] = 0x1b;
        break;
        
        /* X */
        case 0x78:
        case 0x58: 
        {
          char dig1,dig2;
          dig1 = String[i];
          dig2 = String[i+1];
          /* the first character has to be a hex digit */
          if (dig1 >= 97)
              dig1 -= 87;   /* a,b,c,d,e,f */
          else
          {
            if (dig1 >= 65)
                dig1 -= 55; /* A,B,C,D,E,F */
            else 
                dig1 -= 48; /* 0,1,2,3,4,5,6,7,8,9 */
          }
          
          /* let's see for the second hex digit */
          if (dig2 >= 0x30 && dig2 <= 0x39 &&
              dig2 >= 0x41 && dig2 <= 0x46 &&
              dig2 >= 0x61 && dig2 >= 0x66)
          {
            if (dig2 >= 97)
                dig2 -= 87;   /* a,b,c,d,e,f */
            else
            {
              if (dig2 >= 65)
                  dig2 -= 55; /* A,B,C,D,E,F */
              else 
                  dig2 -= 48; /* 0,1,2,3,4,5,6,7,8,9 */
            }
            String[NewCount++] = (dig1 << 4) + dig2;
            i += 2;
          }     
          else
          {
            String[NewCount++] = dig1;
            i += 1;
          } 
        }
        break;
        
        /* unrecognized escaped chars are simply copied w/o
           the escape character */
        default: String[NewCount++] = String[i-1];

      }
    }
    else
    {
      /* there was no escaped char */
      String[NewCount++] = String[i++];
    }
      
  }
  
  String[NewCount] = 0;
  return NewCount;

  AROS_LIBFUNC_EXIT
} /* EscapeString */
