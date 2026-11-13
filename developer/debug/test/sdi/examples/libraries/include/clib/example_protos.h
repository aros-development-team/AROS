/* Automatically generated header! Do not edit! */

#ifndef CLIB_EXAMPLE_PROTOS_H
#define CLIB_EXAMPLE_PROTOS_H

/*
**	$VER: example_protos.h 30792 $Id$
**
**	C prototypes. For use with 32 bit integers only.
**
**	Public Domain
**	    All Rights Reserved
*/

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char * SayHelloOS4(void);
char * SayHelloOS3(void);
char * SayHelloMOS(void);
char * Uppercase(char *txt);
char * SPrintfA(char *buf, char *format, APTR args);
char * SPrintf(char *buf, char *format, APTR args, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_EXAMPLE_PROTOS_H */
