/* Automatically generated header! Do not edit! */

#ifndef _INLINE_EXAMPLE_H
#define _INLINE_EXAMPLE_H

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef EXAMPLE_BASE_NAME
#define EXAMPLE_BASE_NAME ExampleBase
#endif /* !EXAMPLE_BASE_NAME */

#define SayHelloOS4() __SayHelloOS4_WB(EXAMPLE_BASE_NAME)
#define __SayHelloOS4_WB(___base) \
	AROS_LC0(char *, SayHelloOS4, \
	struct Library *, (___base), 5, Example)

#define SayHelloOS3() __SayHelloOS3_WB(EXAMPLE_BASE_NAME)
#define __SayHelloOS3_WB(___base) \
	AROS_LC0(char *, SayHelloOS3, \
	struct Library *, (___base), 6, Example)

#define SayHelloMOS() __SayHelloMOS_WB(EXAMPLE_BASE_NAME)
#define __SayHelloMOS_WB(___base) \
	AROS_LC0(char *, SayHelloMOS, \
	struct Library *, (___base), 7, Example)

#define Uppercase(___txt) __Uppercase_WB(EXAMPLE_BASE_NAME, ___txt)
#define __Uppercase_WB(___base, ___txt) \
	AROS_LC1(char *, Uppercase, \
	AROS_LCA(char *, (___txt), A0), \
	struct Library *, (___base), 8, Example)

#define SPrintfA(___buf, ___format, ___args) __SPrintfA_WB(EXAMPLE_BASE_NAME, ___buf, ___format, ___args)
#define __SPrintfA_WB(___base, ___buf, ___format, ___args) \
	AROS_LC3(char *, SPrintfA, \
	AROS_LCA(char *, (___buf), A0), \
	AROS_LCA(char *, (___format), A1), \
	AROS_LCA(APTR, (___args), A2), \
	struct Library *, (___base), 9, Example)

#ifndef NO_INLINE_VARARGS
#include <proto/alib.h>
struct Library;

#define SPrintf(___buf, ___format, ___args, ...) __SPrintf_WB(EXAMPLE_BASE_NAME, ___buf, ___format, ___args, ## __VA_ARGS__)
static inline char *__SPrintf_WB(struct Library *___base, char *___buf, char *___format, ...)
{
    char *ret;

    AROS_SLOWSTACKFORMAT_PRE(___format);
    ret = __SPrintfA_WB(___base, ___buf, ___format, AROS_SLOWSTACKFORMAT_ARG(___format));
    AROS_SLOWSTACKFORMAT_POST(___format);

    return ret;
}
#endif /* !NO_INLINE_VARARGS */

#endif /* !_INLINE_EXAMPLE_H */
