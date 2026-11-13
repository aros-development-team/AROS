#ifndef PROTO_WB_H
#define PROTO_WB_H

#ifndef DOS_DOSEXTENS_H
#include <dos/dosextens.h>
#endif /* !DOS_DOSEXTENS_H */

#include <clib/wb_protos.h>

#ifdef __GNUC__
#include <inline/wb.h>
#endif /* __GNUC__ */

#ifndef __NOLIBBASE__
extern struct Library *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
WorkbenchBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_WB_H */
