#ifndef _FLOATTEXT_PRIVATE_H_
#define _FLOATTEXT_PRIVATE_H_

#include <exec/types.h>
#include <utility/hooks.h>

struct Floattext_DATA
{
    struct Hook  construct_hook;
    struct Hook  destruct_hook;
    STRPTR       text;
    BOOL         justify;
    STRPTR       skipchars;
    ULONG        tabsize;
};

#endif /* _FLOATTEXT_PRIVATE_H_ */
