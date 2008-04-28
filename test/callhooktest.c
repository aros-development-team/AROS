/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: CallHook arguments passing test.
*/

#include <stdio.h>
#include <proto/alib.h>
#include <proto/aros.h>
#include <utility/hooks.h>

struct Params 
{
	STACKED char  p1; STACKED short  p2; STACKED int p3; STACKED long  p4; 
	STACKED char  p5; STACKED short  p6; STACKED int p7; STACKED long  p8; 
	STACKED char  p9; STACKED short  p10; STACKED int p11; STACKED long  p12; 
	STACKED char  p13; STACKED short  p14; STACKED int p15; STACKED long  p16;
};

long TestHook(struct Hook *hook, APTR object, struct Params *params)
{
	printf("TestHook(%p, %p, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", hook, object,
		params->p1, params->p2, params->p3, params->p4, params->p5, params->p6, params->p7, params->p8, 
		params->p9, params->p10, params->p11, params->p12, params->p13, params->p14, params->p15, params->p16);
	return 0xbabadada;
}

int main (int argc, char ** argv)
{
	struct Hook hook;
	hook.h_Entry = &TestHook;
	long res = CallHook(&hook, 0x0, 
		(char) 0x1, (short) -0x1, (int) 0x2, (long) -0x2,
		(char) 0x3, (short) -0x3, (int) 0x4, (long) -0x4,
		(char) 0x5, (short) -0x5, (int) 0x6, (long) -0x6,
		(char) 0x7, (short) -0x7, (int) 0x8, (long) -0x8
	);
	printf("CallHook result: %p\n", res);
}
