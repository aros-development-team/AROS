#ifndef PARSE_H
#define PARSE_H

#include <stdio.h>

enum token
{
    T_ERROR = -2,
    T_TEXT = 256,
    T_OK,
    T_HTML_TAG,
    T_LAST
};

#endif /* PARSE_H */
