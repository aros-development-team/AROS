/* includes/isc/util.h - AROS stub for ISC utility macros used by ISC DHCP 4.x */
#ifndef ISC_UTIL_H
#define ISC_UTIL_H 1

#include <assert.h>

/* INSIST is used as a debug assertion in socket.c, parse.c, execute.c */
#define INSIST(cond)   assert(cond)
#define REQUIRE(cond)  assert(cond)
#define ENSURE(cond)   assert(cond)
#define INVARIANT(cond) assert(cond)

#define UNUSED(x) ((void)(x))

#endif /* ISC_UTIL_H */
