#ifndef __COMPILER_H
#define __COMPILER_H

#include <assert.h>
#define ASSERT(x)   assert(x)

#if defined __GNUC__ && defined __GNUC_MINOR__
#    define __GNUC_PREREQ(maj, min) \
         ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#    define __GNUC_PREREQ(maj, min) 0
#endif

#if __GNUC_PREREQ(3,3)
#    define __mayalias  __attribute__((__may_alias__))
#else
#    define __mayalias
#endif

#endif
