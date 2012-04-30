#ifndef PROTO_MPEGA_H
#define PROTO_MPEGA_H

#include <exec/types.h>
#include <aros/system.h>

#include <clib/mpega_protos.h>

#if !defined(MPEGABase) && !defined(__NOLIBBASE__) && !defined(__MPEGA_NOLIBBASE__)
extern struct Library *MPEGABase;
#endif

#if !defined(NOLIBDEFINES) && !defined(MPEGA_NOLIBDEFINES)
#   include <defines/mpega.h>
#endif

#endif /* PROTO_MPEGA_H */
