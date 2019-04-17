/*
    Copyright © 2010-2019, The AROS Development Team. All rights reserved.
    $Id: emul_cpp.cpp 36255 2010-12-27 11:33:51Z deadwood $
*/

#include <proto/exec.h>
#include <stdlib.h>

void * operator new(size_t size)
{
	return AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
}

void operator delete(void * ptr)
{
	FreeVec(ptr);
}

void * operator new[](size_t size)
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
