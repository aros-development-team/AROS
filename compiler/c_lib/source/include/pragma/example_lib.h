#ifndef _INCLUDE_PRAGMA_EXAMPLE_LIB_H
#define _INCLUDE_PRAGMA_EXAMPLE_LIB_H

#ifndef CLIB_EXAMPLE_PROTOS_H
#include <clib/Example_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(ExampleBase,0x01E,EXF_TestRequest(d1,d2,d3))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma libcall ExampleBase EXF_TestRequest      01E 32103
#endif
#ifdef __STORM__
#endif
#ifdef __SASC_60
#endif

#endif	/*  _INCLUDE_PRAGMA_EXAMPLE_LIB_H  */