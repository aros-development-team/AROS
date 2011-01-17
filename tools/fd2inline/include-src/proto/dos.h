#ifndef PROTO_DOS_H
#define PROTO_DOS_H

#ifndef DOS_EXALL_H
#include <dos/exall.h>
#endif /* !DOS_EXALL_H */
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif /* !UTILITY_TAGITEM_H */

#include <clib/dos_protos.h>

#ifdef __GNUC__
#include <inline/dos.h>
#endif /* __GNUC__ */

#ifndef __NOLIBBASE__
extern struct DosLibrary *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
DOSBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_DOS_H */
