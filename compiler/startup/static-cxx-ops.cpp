/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.
    $Id:$
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
