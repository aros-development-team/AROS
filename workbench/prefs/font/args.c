/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>

#include <stdio.h>

#include "global.h"

extern IPTR argArray[NUM_ARGS];

struct RDArgs * getArguments(void)
{
 UBYTE a;
 struct RDArgs *tmpReadArgs;

 /* Let's clear the array. Do we really need this or will ReadArgs() set
 every non given argument pointer to NULL? Check it up! */
 /*yes, you need it :) [by falemagn] */

 for(a = 0; a < NUM_ARGS; a++)
  argArray[a] = NULL;

 /* We need an AROS "IPTR" class pointer here! An IPTR is guaranteed to be AmigaOS "ULONG" compatible? */

 if((tmpReadArgs = ReadArgs("FROM,EDIT/S,USE/S,SAVE/S,PUBSCREEN/K", argArray, NULL)))
 {
  for(a = 0; a < NUM_ARGS; a++)
  {
   if(argArray[a])
   {
    printf("%d is set!\n", a);
    if(a == ARG_PUBSCREEN)
     printf("pubscreen is %s!\n", (char *)argArray[ARG_PUBSCREEN]);
   }
  }
  return(tmpReadArgs);
 }
 else
  return(NULL);
}
