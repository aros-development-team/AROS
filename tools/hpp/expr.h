#ifndef EXPR_H
#define EXPR_H

#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif
#ifndef TOOLLIB_VSTRING_H
#   include <toollib/vstring.h>
#endif

extern int Expr_Parse  PARAMS ((const char * str, int * result));

#endif /* EXPR_H */
