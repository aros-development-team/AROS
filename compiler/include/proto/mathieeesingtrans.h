#ifndef PROTO_MATHIEEESINGTRANS_H
#define PROTO_MATHIEEESINGTRANS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/mathieeesingtrans_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/mathieeesingtrans.h>
#else
#include <defines/mathieeesingtrans.h>
#endif

#endif /* PROTO_MATHIEEESINGTRANS_H */
