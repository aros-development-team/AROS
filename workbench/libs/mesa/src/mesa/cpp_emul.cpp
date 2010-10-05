/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

void * operator new(unsigned int size)
{
	return AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
}

void operator delete(void * ptr)
{
	FreeVec(ptr);
}

void * operator new[](unsigned int size)
{
	return AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
}

void operator delete[](void * ptr)
{
	FreeVec(ptr);
}

extern "C" void __cxa_pure_virtual()
{
}
