/*
    Copyright © 2013-2018, The AROS Development Team. All rights reserved.
*/

/* Functions in this include should be used in rest of stdc.library to check
   if functions of a certain library may be used. If not, alternative should
   be provided.
*/

#include <proto/intuition.h>
#include <clib/alib_protos.h>
#include <intuition/intuitionbase.h>

static inline LONG stdcEasyRequest(struct IntuitionBase *IntuitionBase, struct Window *window, struct EasyStruct *easyStruct, ULONG *IDCMP_ptr, ...)
{
    LONG retval;
    AROS_SLOWSTACKFORMAT_PRE_USING(IDCMP_ptr, easyStruct->es_TextFormat)
    retval = EasyRequestArgs(window, easyStruct, IDCMP_ptr, AROS_SLOWSTACKFORMAT_ARG(IDCMP_ptr));
    AROS_SLOWSTACKFORMAT_POST(IDCMP_ptr)
    return retval;
}

int __locale_available(struct StdCIntBase *StdCBase);
int __intuition_available(struct StdCIntBase *StdCBase);
int __optionallibs_close(struct StdCIntBase *StdCBase);
