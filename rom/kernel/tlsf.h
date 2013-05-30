#ifndef _TLSF_H
#define _TLSF_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_MEMHEADEREXT_H
#include <exec/memheaderext.h>
#endif

typedef APTR (*autogrow_get)(APTR, IPTR *);
typedef VOID (*autogrow_release)(APTR, APTR, IPTR);

/* Initialization and memory management */
APTR tlsf_init(struct MemHeaderExt * mhe);
APTR tlsf_init_autogrow(struct MemHeaderExt * mhe, IPTR puddle_size, autogrow_get grow_function, autogrow_release release_function, APTR autogrow_data);
VOID tlsf_destroy(struct MemHeaderExt * mhe);
VOID tlsf_add_memory(struct MemHeaderExt * mhe, APTR memory, IPTR size);
VOID tlsf_add_memory_and_merge(struct MemHeaderExt * mhe, APTR memory, IPTR size);

/* Allocation functions */
APTR tlsf_malloc(struct MemHeaderExt * mhe, IPTR size, ULONG * flags);
VOID tlsf_freemem(struct MemHeaderExt * mhe, APTR ptr, IPTR size);
VOID tlsf_freevec(struct MemHeaderExt * mhe, APTR ptr);
APTR tlsf_realloc(struct MemHeaderExt * mhe, APTR ptr, IPTR new_size);
APTR tlsf_allocabs(struct MemHeaderExt * mhe, IPTR size, APTR ptr);

/* Query functions */
IPTR tlsf_avail(struct MemHeaderExt * mhe, ULONG requirements);
BOOL tlsf_in_bounds(struct MemHeaderExt * mhe, APTR begin, APTR end);

/* Initialization of MemHeader */
void krnCreateTLSFMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags);
struct MemHeader * krnConvertMemHeaderToTLSF(struct MemHeader * source);

#endif /* _TLSF_H */
