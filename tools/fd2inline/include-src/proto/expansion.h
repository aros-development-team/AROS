#ifndef PROTO_EXPANSION_H
#define PROTO_EXPANSION_H

#ifndef DOS_FILEHANDLER_H
#include <dos/filehandler.h>
#endif /* !DOS_FILEHANDLER_H */
#ifndef LIBRARIES_CONFIGVARS_H
#include <libraries/configvars.h>
#endif /* !LIBRARIES_CONFIGVARS_H */

#include <clib/expansion_protos.h>

#ifdef __GNUC__
#include <inline/expansion.h>
#endif /* __GNUC__ */

#ifndef __NOLIBBASE__
extern struct ExpansionBase *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
ExpansionBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_EXPANSION_H */
