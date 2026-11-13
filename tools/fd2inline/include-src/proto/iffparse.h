#ifndef PROTO_IFFPARSE_H
#define PROTO_IFFPARSE_H

#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif /* !UTILITY_HOOKS_H */

#include <clib/iffparse_protos.h>

#ifdef __GNUC__
#include <inline/iffparse.h>
#endif /* __GNUC__ */

#ifndef __NOLIBBASE__
extern struct Library *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
IFFParseBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_IFFPARSE_H */
