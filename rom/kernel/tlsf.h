#ifndef _TLSF_H
#define _TLSF_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

typedef APTR (*autogrow_get)(APTR, IPTR *);
typedef VOID (*autogrow_release)(APTR, APTR, IPTR);

/* Initialization and memory management */
APTR tlsf_init(void * ptr, IPTR size);
APTR tlsf_init_autogrow(void * ptr, IPTR size, IPTR puddle_size, autogrow_get grow_function, autogrow_release release_function, APTR autogrow_data);
VOID tlsf_destroy(APTR tlsf);
VOID tlsf_add_memory(APTR tlsf, APTR memory, IPTR size);
VOID tlsf_add_memory_and_merge(APTR tlsf, APTR memory, IPTR size);

/* Allocation functions */
APTR tlsf_malloc(APTR tlsf, IPTR size);
VOID tlsf_free(APTR tlsf, APTR ptr);
APTR tlsf_realloc(APTR tlsf, APTR ptr, IPTR new_size);
APTR tlsf_allocabs(APTR tlsf, APTR ptr, IPTR size);

/* Query functions */
IPTR tlsf_avail(APTR tlsf, ULONG requirements);
BOOL tlsf_in_bounds(APTR tlsf, APTR begin, APTR end);

/* Debug functions */
VOID tlsf_print(APTR tlsf);
VOID tlsf_print_all_blocks(APTR tlsf);

#endif /* _TLSF_H */
