/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#define MATCHUINT(dest, value)                   \
    if(sscanf(s, "%u%n", &val, &matched) != EOF) \
	dest = value;                            \
    else                                         \
	return NULL;                             \

char *strptime(const char *s, const char *format, struct tm *tm)
{
    int matched;
    int val;
    
    if (format == NULL || s == NULL || tm == NULL) 
    {
	errno = EINVAL;
	return NULL;
    }
    
    while (*format)
    {
        if (*format == '%')
        {
            matched = 0;
            
            switch(*++format)
            {
            case '%':
        	matched = (*s == '%' ? 1 : EOF);
        	break;
            case 'd':
            case 'e':
        	MATCHUINT(tm->tm_mday, val);
        	break;
            case 'H':
            	MATCHUINT(tm->tm_hour, val);
        	break;
            case 'm':
               	MATCHUINT(tm->tm_mon, val - 1);
        	break;
            case 'M':
               	MATCHUINT(tm->tm_min, val);
        	break;
            case 'S':
               	MATCHUINT(tm->tm_sec, val);
        	break;
            case 'w':
               	MATCHUINT(tm->tm_wday, val);
               	break;
            case 'y':
               	MATCHUINT(tm->tm_year, val);
               	break;
            case 'Y':
               	MATCHUINT(tm->tm_year, val - 1900);
               	break;
            case '\0':
                format--;
                break;
            default:
                /* FIXME: Implement remaining conversions */
        	return 0;
            }
            
            s += matched;
        }
        else
        {
            /* whitespace matches zero or more whitespace characters */
            if(isspace(*format))
            {
        	while(isspace(*s))
        	    s++;
            }
            else
            {
        	/* compare characters directly */
                if(*s != *format)
        	    return NULL;
                s++;
            }
        }    
        
        format++;
    }

    return (char *)s;
}
