#ifndef _AROS_TYPES_TIMEVAL_S_H_
#define _AROS_TYPES_TIMEVAL_S_H_

#include <aros/cpu.h>

/* Version of timeval structure that guarantees 32-bit sizes and can be used
    then 32-bit is enough */
struct timeval32
{
    unsigned AROS_32BIT_TYPE tv_secs;
    unsigned AROS_32BIT_TYPE tv_micro;
};

/* Note: after moving to 64-bit time, "struct timeval" will always be 64-bit
   for compability with POSIX-style structure. All new compiled code will be
   using 64-bit wide fields for both AROS-style and POSIX-style functions */

/* The following structure is composed of two anonymous unions so that it
   can be transparently used by both AROS-style programs and POSIX-style
   ones. For binary compatibility reasons the fields in the unions MUST
   have the same size, however they can have different signs (as it is
   the case for microseconds).  */

#ifndef NO_AROS_TIMEVAL

__extension__ struct timeval
{
    union  /* Seconds passed. */
    {
        unsigned AROS_32BIT_TYPE tv_secs;   /* AROS field */
        unsigned AROS_32BIT_TYPE tv_sec;    /* POSIX field */
    };
    union /* Microseconds passed in the current second. */
    {
        unsigned AROS_32BIT_TYPE tv_micro; /* AROS field */
        signed   AROS_32BIT_TYPE tv_usec;  /* POSIX field */
    };
};

#endif /* !NO_AROS_TIMEVAL */

#endif /* ! _AROS_TYPES_TIMEVAL_S_H_ */
