#pragma once
typedef unsigned char UBYTE; typedef unsigned short UWORD;
typedef unsigned int ULONG; typedef signed int LONG;
typedef unsigned long IPTR; typedef signed long SIPTR;
typedef int BOOL; typedef void *APTR; typedef char *STRPTR;
typedef const char *CONST_STRPTR;
#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL ((void*)0)
#endif
