/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

extern void main_init(void * memory, ULONG memSize);

int main(void)
{
	main_init((void *)0x20000,0x4000);
	return 0;
}
