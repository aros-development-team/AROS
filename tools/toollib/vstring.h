#ifndef _VSTRING_H
#define _VSTRING_H

#ifndef _TOOLLIB_H
#   include <toollib.h>
#endif

typedef struct _String
{
    char * buffer;
    int    len;
    int    maxlen;
}
* String;

extern String VS_New	       PARAMS ((const char * str));
extern void   VS_Delete        PARAMS ((String str));
extern void   VS_Clear	       PARAMS ((String str));
extern void   VS_AppendChar    PARAMS ((String str, int c));
extern void   VS_AppendString  PARAMS ((String str, const char * app));
extern void   VS_ToUpper       PARAMS ((String str));
extern void   VS_ToLower       PARAMS ((String str));
extern String VS_SubString     PARAMS ((const char * str, int begin, int len));

extern char * strdupsub   PARAMS((const char * str, int begin, int len));
extern char * stripquotes PARAMS((const char * str));
extern char * stripws	  PARAMS((const char * str));
extern char * stripwsq	  PARAMS((const char * str));
extern char * strupper	  PARAMS((char * str));

#endif /* _VSTRING_H */
