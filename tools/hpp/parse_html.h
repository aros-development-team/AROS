#ifndef PARSE_HTML_H
#define PARSE_HTML_H

#ifndef _TOOLLIB_H
#   include <toollib.h>
#endif
#ifndef _CALLBACK_H
#   include <callback.h>
#endif
#ifndef _VSTRING_H
#   include <vstring.h>
#endif

typedef struct
{
    Node   node;
    char * value;
}
HTMLTagArg;

typedef struct
{
    Node   node;
    List   args; /* HTMLTagArg */
}
HTMLTag;

extern void	 HTML_InitParse PARAMS ((void));
extern int	 HTML_ScanText	PARAMS ((String buffer, CB getc, void * stream, CBD data));
extern HTMLTag * HTML_ParseTag	PARAMS ((CB getc, void * stream, CBD data));
extern void	 HTML_FreeTag	PARAMS ((HTMLTag *));
extern void	 HTML_PrintTag	PARAMS ((HTMLTag *));
extern String	 HTML_ReadBody	PARAMS ((CB getc, void * stream, CBD data,
					const char * name, int allowNest));

#endif /* PARSE_HTML_H */

