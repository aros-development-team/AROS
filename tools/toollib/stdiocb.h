#ifndef _STDIOCB_H
#define _STDIOCB_H

#include <stdio.h>
#ifndef _TOOLLIB_H
#   include <toollib.h>
#endif
#ifndef _CALLBACK_H
#   include <callback.h>
#endif

typedef struct
{
    FILE * fh;
    char * name;
    int    line;
}
StdioStream;

#define STRCB_GETCHAR  0x00010000
#define _STRCB_UNGETC	0x00020000
#define STRCB_UNGETC(c) (_STRCB_UNGETC | ((unsigned int)c & 0xFFFF))
#define STRCB_CMDMASK  0xFFFF0000

extern int StdioGetCharCB PARAMS ((void * obj, int cmd, CBD data));

extern StdioStream * StdStr_New     PARAMS ((const char * path, const char * mode));
extern void	     StdStr_Delete  PARAMS ((StdioStream *));
extern const char  * StdStr_GetName PARAMS ((StdioStream *));
extern int	     StdStr_GetLine PARAMS ((StdioStream *));

#endif /* _STDIOCB_H */
