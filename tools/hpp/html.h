#ifndef HTML_H
#define HTML_H

#ifndef _TOOLLIB_H
#   include <toollib.h>
#endif
#ifndef PARSE_HTML_H
#   include "parse_html.h"
#endif

extern void HTML_Init  PARAMS ((void));
extern void HTML_Exit  PARAMS ((void));
extern int  HTML_Parse PARAMS ((CB getc, void * stream, CBD data));

#endif /* HTML_H */
