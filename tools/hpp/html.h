#ifndef HTML_H
#define HTML_H

#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif
#ifndef TOOLLIB_MYSTREAM_H
#   include <toollib/mystream.h>
#endif
#ifndef PARSE_HTML_H
#   include "parse_html.h"
#endif

typedef struct
{
    MyStream stream;
    void   * data;
}
HTMLStream;

extern void HTML_Init  PARAMS ((void));
extern void HTML_Exit  PARAMS ((void));
extern int  HTML_Parse PARAMS ((MyStream * in, MyStream * out, CBD data));

extern int HTMLOutCB PARAMS ((HTMLStream * str, int dummy, CBD data));

#endif /* HTML_H */
