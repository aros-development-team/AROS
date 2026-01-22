#include <exec/types.h>
#undef GLOBAL
#define GLOBAL
#define DO_INIT
#include "defs.h"

#ifdef __GNUC__
#include <inline/muimaster.h>
#endif


void InitHook (struct Hook *hook, HOOKFUNC func)

{
#if defined(__GNUC__) && !defined(__AROS__)

extern void CallHook_C (void);

hook->h_Entry = (HOOKFUNC)CallHook_C;
hook->h_SubEntry = func;

#else

hook->h_Entry = func;

#endif
}

ULONG MaxAvailMem (ULONG flags)

{
/* Determines the amount of memory of a given type */
struct MemHeader *cur_mh;
ULONG amount;

amount = 0;
for (cur_mh = (struct MemHeader*)SysBase->MemList.lh_Head; 
     cur_mh->mh_Node.ln_Succ!= NULL;
     cur_mh = (struct MemHeader*)cur_mh->mh_Node.ln_Succ)
  {
  if ((cur_mh->mh_Attributes & flags) == flags)
    amount += (IPTR)cur_mh->mh_Upper - (IPTR)cur_mh->mh_Lower;
  }

return (amount);
}
