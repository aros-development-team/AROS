#ifndef PROTO_DOSPATH_H
#define PROTO_DOSPATH_H

#ifndef DOS_DOSEXTENS_H
#include <dos/dosextens.h>
#endif /* !DOS_DOSEXTENS_H */

#include <clib/dospath_protos.h>

#ifdef __GNUC__
#include <inline/dospath.h>
#endif /* __GNUC__ */

#ifndef __NOLIBBASE__
extern struct Library *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
DOSPathBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_DOSPATH_H */
