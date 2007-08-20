/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: core.h 12747 2001-12-08 20:11:50Z chodorowski $
*/

#ifndef CORE_H
#define CORE_H

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

/* Hardware handlers */

void AndIMask();
void OrIMask();
void IntServer();
void MakeInt();
void StoreCPU(APTR);
void StoreFPU(APTR);
void RestoreCPU(APTR);
void RestoreFPU(APTR);

/* Exec replacements */

void AROS_SLIB_ENTRY(CacheClearE,Exec);
void AROS_SLIB_ENTRY(CacheClearU,Exec);
void AROS_SLIB_ENTRY(CacheControl,Exec);
void AROS_SLIB_ENTRY(CachePreDMA,Exec);
void AROS_SLIB_ENTRY(CachePostDMA,Exec);
void AROS_SLIB_ENTRY(ColdReboot,Exec);
void AROS_SLIB_ENTRY(Disable,Exec);
void AROS_SLIB_ENTRY(Enable,Exec);
void AROS_SLIB_ENTRY(Forbid,Exec);
void AROS_SLIB_ENTRY(Permit,Exec);
void AROS_SLIB_ENTRY(GetCC,Exec);
void AROS_SLIB_ENTRY(SetSR,Exec);
void AROS_SLIB_ENTRY(Supervisor,Exec);
void AROS_SLIB_ENTRY(StackSwap,Exec);

#endif /* CORE_H */
