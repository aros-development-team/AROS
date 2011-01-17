#ifndef PROTO_DATATYPES_H
#define PROTO_DATATYPES_H

#ifndef DATATYPES_DATATYPESCLASS_H
#include <datatypes/datatypesclass.h>
#endif /* !DATATYPES_DATATYPESCLASS_H */

#include <clib/datatypes_protos.h>

#ifdef __GNUC__
#include <inline/datatypes.h>
#endif /* __GNUC__ */

#ifndef __NOLIBBASE__
extern struct Library *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
DataTypesBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_DATATYPES_H */
