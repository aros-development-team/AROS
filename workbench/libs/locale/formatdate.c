/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <utility/hooks.h>
#include <proto/utility.h>
#include "locale_intern.h"


AROS_UFH3(VOID, rec_putCharFunc,
  AROS_UFHA(struct Hook *,   rec_putCharFunc,            A0),
  AROS_UFHA(char,            c,                          A1),
  AROS_UFHA(struct Locale *, locale,                     A2))
{
  struct Hook * recHook = (struct Hook *)rec_putCharFunc->h_Data;
  if (recHook && recHook->h_Entry)
  {
     AROS_UFC3(VOID, recHook->h_Entry,
       AROS_UFCA(struct Hook *,   recHook,                   A0),
       AROS_UFCA(char,            c,                         A1),
       AROS_UFCA(struct Locale *, locale,                    A2)
     );
  }
}

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH4(void, FormatDate,

/*  SYNOPSIS */
	AROS_LHA(struct Locale    *, locale, A0),
	AROS_LHA(STRPTR            , fmtTemplate, A1),
	AROS_LHA(struct DateStamp *, date, A2),
	AROS_LHA(struct Hook      *, putCharFunc, A3),

/*  LOCATION */
	struct Locale *, LocaleBase, 10, Locale)

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
  struct Hook recursion_hook;
  
  recursion_hook.h_Entry = (VOID *)rec_putCharFunc;
  recursion_hook.h_Data  = putCharFunc;
  
  
  if (NULL == fmtTemplate)
  {
     AROS_UFC3(VOID, putCharFunc->h_Entry,
       AROS_UFCA(struct Hook *,   putCharFunc,                A0),
       AROS_UFCA(char,            '\0',                       A1),
       AROS_UFCA(struct Locale *, locale,                     A2)
     );
     return;
  }

  while (FALSE == end)
  {
    /*
    ** A format description starts here?
    */
    if ('%' == fmtTemplate[template_pos])
    {
      state = FOUND_FORMAT;
    }
    
    switch (state)
    {
      case OUTPUT:
        /*
        ** Call the hook for this character
        */
        AROS_UFC3(VOID, putCharFunc->h_Entry,
            AROS_UFCA(struct Hook *,   putCharFunc,                A0),
            AROS_UFCA(char,            fmtTemplate[template_pos],  A1),
            AROS_UFCA(struct Locale *, locale,                     A2));

        /*
        ** End of template string? -> End of this function.
        */
        if ('\0' == fmtTemplate[template_pos])
            end = TRUE;
        else
          template_pos++;

      break;
      
      case FOUND_FORMAT:
      {
        char  buf[256];
//        char * buffer = &buf[0];
        ULONG width;
        BOOL printit = FALSE;
        ULONG hours;
        ULONG minutes;
        ULONG seconds;
        int i;
        
        template_pos++;
        
        switch (fmtTemplate[template_pos])
        {
          case 'a':
          break;
          
          case 'A':
          break;
          
          case 'b':
          break;
          
          case 'B':
          break;
          
          case 'c':
            FormatDate(locale, 
                       "%a %b %d %H:%M:%S %Y", 
                       date,
                       &recursion_hook);
          break;
          
          case 'C':
            FormatDate(locale, 
                       "%a %b %e %T %Z %Y", 
                       date,
                       &recursion_hook);
          break;
          
          case 'd':
          break;
          
          case 'D':
            FormatDate(locale, 
                       "%m/%d/%y", 
                       date,
                       &recursion_hook);

          break;
          
          case 'e':
          break;
          
          case 'h':
          break;
          
          case 'H': /* hour using 24-hour style with leading 0s */
          {
            hours   = date->ds_Minute / 60;
            
            buf[0]  = hours / 10 + '0';
            buf[1]  = hours % 10 + '0';
            width   = 2;
            
            printit = TRUE;
          }
          break;
          
          case 'I': /* hour using 12-hour style with leading 0s */
          {
            hours   = date->ds_Minute / 60;
            if (hours > 12)
              hours -= 12;
              
            minutes = date->ds_Minute % 60;
            
            buf[0]  = hours / 10 + '0';
            buf[1]  = hours % 10 + '0';

            width = 2;
            printit = TRUE;
          }
          break;
          
          case 'j':
          break;
          
          case 'm':
          break;
          
          case 'M':
          {
            minutes = date->ds_Minute % 60;
            
            buf[0] = minutes / 10 + '0';
            buf[1] = minutes % 10 + '0';
            width = 2;
            printit = TRUE;
          }
          break;
          
          case 'n':
          {
            width = 1;
            buf[0] = '\n';
            printit = TRUE;
          }
          break;
          
          case 'p':
          {
            hours   = date->ds_Minute / 60;
            if (hours > 12)
            {
              /* PM */
            }
            else
            {
              /* AM */
            }
          }
          break;
          
          case 'q':
          {
            i = 0;
            hours   = date->ds_Minute / 60;
            
            if (hours > 9)
            {
              buf[0]  = hours / 10 + '0';
              i = 1;
            }
            buf[i++]  = hours % 10 + '0';
            
            width = i;
            printit = TRUE;
          }
          break;
          
          case 'Q':
            i = 0;
            hours   = date->ds_Minute / 60;
            if (hours > 12)
              hours -= 12;
              
            if (hours > 9)
            {
              buf[0]  = hours / 10 + '0';
              i = 1;
            }
            buf[i++]  = hours % 10 + '0';
            
            width = i;
            
            printit = TRUE; 
          break;
          
          case 'r':
            FormatDate(locale, 
                       "%I:%M:%S %p", 
                       date,
                       &recursion_hook);
          break;
          
          case 'R':
            FormatDate(locale, 
                       "%H:%M",
                       date,
                       &recursion_hook);
          break;
          
          case 'S':
          {
            seconds = date->ds_Tick / 50;
            
            buf[0] = seconds / 10 + '0';
            buf[1] = seconds % 10 + '0';
            width = 2;
            printit = TRUE;
          }
          break;
          
          case 't':
          {
            buf[0] = '\t';
            width = 1;
            printit = TRUE;
          }
          break;
          
          case 'T':
            FormatDate(locale, 
                       "%H:%M:%S",
                       date,
                       &recursion_hook);
          break;
          
          case 'U':
          break;
          
          case 'w':
          break;
          
          case 'W':
          break;
          
          case 'x':
            FormatDate(locale, 
                       "%m/%d/%y",
                       date,
                       &recursion_hook);
          break;
          
          case 'X':
            FormatDate(locale, 
                       "%H:%M:%S",
                       date,
                       &recursion_hook);
          break;
          
          case 'y':
          break;
          
          case 'Y':
          break;
        }
        
        if (TRUE == printit)
        {
          for (i = 0; i < width; i++)
            AROS_UFC3(VOID, putCharFunc->h_Entry,
              AROS_UFCA(struct Hook *,   putCharFunc,                A0),
              AROS_UFCA(char,            buf[i],                     A1),
              AROS_UFCA(struct Locale *, locale,                     A2)
            );
          
        }
        
        template_pos++;
        state = OUTPUT;
      }
      break;
    }
  }
  


  AROS_LIBFUNC_EXIT
} /* FormatDate */
