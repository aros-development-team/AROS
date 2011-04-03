/*
**	$VER: TextExampleLib.c 37.1 (4.12.96)
**
**	Demo program for example.library
**
**	(C) Copyright 1996 Andreas R. Kleinert
**	All Rights Reserved.
*/

#include <exec/types.h>
#include <exec/memory.h>

#include <example/example.h>

#include <proto/exec.h>
#include <proto/example.h>

#include <stdio.h>
#include <stdlib.h>

void main(long argc, char **argv)
{
    struct ExampleBase *ExampleBase = NULL;

    ExampleBase = (APTR) OpenLibrary(EXAMPLENAME, 37);

    if(ExampleBase)
    {
	EXF_TestRequest("Test Message", "It works!", "OK");

	CloseLibrary((APTR) ExampleBase);

	exit(0);

    }
    else
	printf ("\nLibrary opening failed\n");

    exit(20);
}
