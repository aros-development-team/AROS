#ifndef _STRINGCB_H
#define _STRINGCB_H

#ifndef _TOOLLIB_H
#   include <toollib.h>
#endif
#ifndef _CALLBACK_H
#   include <callback.h>
#endif
#ifndef _STDIOCB_H
#   include <stdiocb.h>
#endif

typedef struct
{
    const char * string;
    int 	 pos,
		 max,
		 line;
}
StringStream;

extern int StringGetCharCB PARAMS ((void * obj, int cmd, CBD data));

extern StringStream * StrStr_New     PARAMS ((const char * string));
extern void	      StrStr_Delete  PARAMS ((StringStream *));
extern const char   * StrStr_GetName PARAMS ((StringStream *));
extern int	      StrStr_GetLine PARAMS ((StringStream *));

#endif /* _STRINGCB_H */
