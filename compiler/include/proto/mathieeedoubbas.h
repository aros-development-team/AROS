#ifndef PROTO_MATHIEEEDOUBBAS_H
#define PROTO_MATHIEEEDOUBBAS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/mathieeedoubbas_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/mathieeedoubbas.h>
#else
#include <defines/mathieeedoubbas.h>
#endif

#endif /* PROTO_MATHIEEEDOUBBAS_H */
