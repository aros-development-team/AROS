#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/resident.h>

/* Look for ROMTags in specified address ranges and build resident list */
APTR krnRomTagScanner(struct MemHeader *mh, UWORD *ranges[]);
/* Find a resident with specified name */
struct Resident *krnFindResident(struct Resident **resList, const char *name);

/* Find ROMTags and call exec.library early entry point to create ExecBase */
struct ExecBase *krnPrepareExecBase(UWORD *ranges[], struct MemHeader *mh, struct TagItem *bootMsg);

/* Magic. Described in rom/exec/exec_init.c. */
static inline struct ExecBase *krnInitExecBase(struct Resident *res, struct MemHeader *mh, struct TagItem *bootMsg)
{
    return AROS_UFC3(struct ExecBase *, res->rt_Init,
                     AROS_UFCA(struct MemHeader *, mh, D0),
                     AROS_UFCA(struct TagItem *, bootMsg, A0),
 	             AROS_UFCA(struct ExecBase *, NULL, A6));
}
