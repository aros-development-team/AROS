/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#ifndef TEST_EXEC_LIBRARIES_H
#define TEST_EXEC_LIBRARIES_H
#include <exec/types.h>
/* Only the accessed field matters to this adapter fixture; not a library ABI claim. */
struct Library { uint16_t lib_NegSize; };
#define LIB_VECTSIZE 16
struct Library *OpenLibrary(CONST_STRPTR, ULONG);
void CloseLibrary(struct Library *);
#endif
