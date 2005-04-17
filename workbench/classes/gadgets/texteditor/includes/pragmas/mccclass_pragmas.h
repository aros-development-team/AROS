#ifndef POS /* private */
#ifndef _INCLUDE_PRAGMA_MCCCLASS_LIB_H
#define _INCLUDE_PRAGMA_MCCCLASS_LIB_H

#ifndef CLIB_MCCCLASS_PROTOS_H
#include <clib/mccclass_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(MCCClassBase, 0x01e, MCC_Query(d0))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma  libcall MCCClassBase MCC_Query 01e 001
#endif

#endif	/*  _INCLUDE_PRAGMA_MCCCLASS_LIB_H  */
#endif /* private */
