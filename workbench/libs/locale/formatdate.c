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
      state = FOUND_FORMAT;
    
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
        char * buffer = &buf[0];
        ULONG width = 0;
        BOOL printit = TRUE;
        ULONG hours;
        ULONG minutes;
        ULONG seconds;
        ULONG day;
        ULONG week;
        ULONG year;
        int i;
        
        template_pos++;
        
        switch (fmtTemplate[template_pos])
        {
          case 'a':
            buffer = GetLocaleStr(locale, calendar_weekday(date) + DAY_1);
            width = strlen(buffer);
          break;
          
          case 'A':
            buffer = GetLocaleStr(locale, calendar_weekday(date) + ABDAY_1);
            width = strlen(buffer);
          break;
          
          case 'b':
            buffer = GetLocaleStr(locale, calendar_month(date) + ABMON_1);
            width = strlen(buffer);
          break;
          
          case 'B':
            buffer = GetLocaleStr(locale ,calendar_month(date) + MON_1);
            width = strlen(buffer);
          break;
          
          case 'c':
            FormatDate(locale, 
                       "%a %b %d %H:%M:%S %Y", 
                       date,
                       &recursion_hook);
            printit = FALSE;
          break;
          
          case 'C':
            FormatDate(locale, 
                       "%a %b %e %T %Z %Y", 
                       date,
                       &recursion_hook);
            printit = FALSE;
          break;
          
          case 'd':
            day = calendar_day(date);
            buf[0] = day / 10 + '0';
            buf[1] = day % 10 + '0';
            width = 2;
          break;
          
          case 'D':
            FormatDate(locale, 
                       "%m/%d/%y", 
                       date,
                       &recursion_hook);
            printit = FALSE;

          break;
          
          case 'e':
            day = calendar_day(date);
            if (day > 9)
              buf[0] = day / 10 + '0';
            else
              buf[0] = ' ';
              
            buf[1] = day % 10 + '0';
            width = 2;
          break;
          
          case 'h':
            buffer = GetLocaleStr(locale, calendar_month(date) + ABMON_1);
            width = strlen(buffer);
          break;
          
          case 'H': /* hour using 24-hour style with leading 0s */
          {
            hours   = date->ds_Minute / 60;
            
            buf[0]  = hours / 10 + '0';
            buf[1]  = hours % 10 + '0';
            width   = 2;
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
          }
          break;
          
          case 'j':
          break;
          
          case 'm':
            day = calendar_month(date) + 1;
            buf[0] = day / 10 + '0';
            buf[1] = day % 10 + '0';
            width = 2;
          break;
          
          case 'M':
          {
            minutes = date->ds_Minute % 60;
            
            buf[0] = minutes / 10 + '0';
            buf[1] = minutes % 10 + '0';
            width = 2;
          }
          break;
          
          case 'n':
          {
            width = 1;
            buf[0] = '\n';
          }
          break;
          
          case 'p':
          {
            hours   = date->ds_Minute / 60;
            if (hours > 12)
            {
              /* PM */
              buffer = GetLocaleStr(locale, PM_STR);
            }
            else
            {
              /* AM */
              buffer = GetLocaleStr(locale, AM_STR);
            }
            width = strlen(buffer);
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
            
          break;
          
          case 'r':
            FormatDate(locale, 
                       "%I:%M:%S %p", 
                       date,
                       &recursion_hook);
            printit = FALSE;
          break;
          
          case 'R':
            FormatDate(locale, 
                       "%H:%M",
                       date,
                       &recursion_hook);
            printit = FALSE;
          break;
          
          case 'S':
            seconds = date->ds_Tick / 50;
            
            buf[0] = seconds / 10 + '0';
            buf[1] = seconds % 10 + '0';
            width = 2;
          break;
          
          case 't':
            buf[0] = '\t';
            width = 1;
          break;
          
          case 'T':
            FormatDate(locale, 
                       "%H:%M:%S",
                       date,
                       &recursion_hook);
            printit = FALSE;
          break;
          
          case 'U':
            i = 0;
            week = calendar_week(date);
            if (week > 9)
            {
              buf[0] = day / 10 + '0';
              i = 1;
            }
              
            buf[i++] = day % 10 + '0';
            width = i;
          break;
          
          case 'w':
            day = calendar_weekday(date);
            buf[0] = day + '0';
            width = 1;
          break;
          
          case 'W':
            i = 0;
            week = calendar_weekmonday(date);
            if (week > 9)
            {
              buf[0] = day / 10 + '0';
              i = 1;
            }
              
            buf[i++] = day % 10 + '0';
            width = i;
          break;
          
          case 'x':
            FormatDate(locale, 
                       "%m/%d/%y",
                       date,
                       &recursion_hook);
            printit = FALSE;
          break;
          
          case 'X':
            FormatDate(locale, 
                       "%H:%M:%S",
                       date,
                       &recursion_hook);
            printit = FALSE;
          break;
          
          case 'y':
          {
            buf[0] = buf[1] = '0';
            year = calendar_year(date);
            year = year % 1000;
            year = year % 100;

            buf[2] = year / 10 + '0';
            year = year % 10;
            buf[3] = year + '0';
           
            width = 4;
          }
          break;
          
          case 'Y':
          {
            year = calendar_year(date);
            buf[0] = year / 1000 + '0';
            year = year % 1000;
            buf[1] = year / 100 + '0';
            year = year % 100;
            buf[2] = year / 10 + '0';
            year = year % 10;
            buf[3] = year + '0';
           
            width = 4;
          }          
          break;
          
          default: 
            printit = FALSE;
        }
        
        if (TRUE == printit)
        {
          for (i = 0; i < width; i++)
            AROS_UFC3(VOID, putCharFunc->h_Entry,
              AROS_UFCA(struct Hook *,   putCharFunc,                A0),
              AROS_UFCA(char,            buffer[i],                  A1),
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
