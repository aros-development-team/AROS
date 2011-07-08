/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
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
void AROS_SLIB_ENTRY(CacheClearE,Exec,107);
void AROS_SLIB_ENTRY(CacheClearU,Exec,106);
void AROS_SLIB_ENTRY(CacheControl,Exec,108);
void AROS_SLIB_ENTRY(CachePreDMA,Exec,127);
void AROS_SLIB_ENTRY(CachePostDMA,Exec,128);
void AROS_SLIB_ENTRY(ColdReboot,Exec,121);
void AROS_SLIB_ENTRY(Disable,Exec,20);
void AROS_SLIB_ENTRY(Enable,Exec,21);
void AROS_SLIB_ENTRY(Forbid,Exec,22);
void AROS_SLIB_ENTRY(Permit,Exec,23);
void AROS_SLIB_ENTRY(GetCC,Exec,88);
void AROS_SLIB_ENTRY(SetSR,Exec,24);
void AROS_SLIB_ENTRY(Supervisor,Exec,5);
void AROS_SLIB_ENTRY(StackSwap,Exec,122);

#endif /* CORE_H */
