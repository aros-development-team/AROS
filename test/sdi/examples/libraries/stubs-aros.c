/* Automatically generated gatestubs! Do not edit! */

#include <exec/types.h>

#define _sfdc_strarg(a) _sfdc_strarg2(a)
#define _sfdc_strarg2(a) #a

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <aros/libcall.h>
#include <SDI_lib.h>

#include "libproto.h"

char *
SayHelloOS4(void);

AROS_LH0(char *, SayHelloOS4,
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC_NP(SayHelloOS4);
    AROS_LIBFUNC_EXIT
}

char *
SayHelloOS3(void);

AROS_LH0(char *, SayHelloOS3,
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC_NP(SayHelloOS3);
    AROS_LIBFUNC_EXIT
}

char *
SayHelloMOS(void);

AROS_LH0(char *, SayHelloMOS,
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC_NP(SayHelloMOS);
    AROS_LIBFUNC_EXIT
}

char *
Uppercase(char * ___txt);

AROS_LH1(char *, Uppercase,
    AROS_LHA(char *, ___txt, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(Uppercase,___txt);
    AROS_LIBFUNC_EXIT
}

char *
SPrintfA(char * ___buf, char * ___format, APTR ___args);

AROS_LH3(char *, SPrintfA,
    AROS_LHA(char *, ___buf, A0),
    AROS_LHA(char *, ___format, A1),
    AROS_LHA(APTR, ___args, A2),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(SPrintfA,___buf, ___format, ___args);
    AROS_LIBFUNC_EXIT
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
