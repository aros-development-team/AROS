#ifndef PARSE_HTML_H
#define PARSE_HTML_H

#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif
#ifndef TOOLLIB_MYSTREAM_H
#   include <toollib/mystream.h>
#endif
#ifndef TOOLLIB_VSTRING_H
#   include <toollib/vstring.h>
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

extern int	 HTML_Get	PARAMS ((MyStream * stream, CBD data));
extern void	 HTML_InitParse PARAMS ((void));
extern int	 HTML_ScanText	PARAMS ((String buffer, MyStream * stream, CBD data));
extern HTMLTag * HTML_ParseTag	PARAMS ((MyStream * stream, CBD data));
extern void	 HTML_FreeTag	PARAMS ((HTMLTag *));
extern void	 HTML_PrintTag	PARAMS ((HTMLTag *));
extern String	 HTML_ReadBody	PARAMS ((MyStream * stream, CBD data,
					const char * name, int allowNest));

#endif /* PARSE_HTML_H */

