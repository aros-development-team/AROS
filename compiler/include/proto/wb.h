#ifndef PROTO_WB_H
#define PROTO_WB_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/wb_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/wb.h>
#else
#include <defines/wb.h>
#endif

#endif /* PROTO_WB_H */
