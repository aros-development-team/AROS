/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.0

  Memory management functions
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/


#ifndef MEMORY_H
#define MEMORY_H   /*+ To stop multiple inclusions. +*/

/*+ malloc(), calloc(), realloc() and free() replacements +*/

#define Malloc(s)    SafeMalloc (  s,__FILE__,__LINE__)
#define Calloc(n,s)  SafeCalloc (n,s,__FILE__,__LINE__)
#define Realloc(p,s) SafeRealloc(p,s,__FILE__,__LINE__)
#define Free(p)      SafeFree   (p  ,__FILE__,__LINE__)

void* SafeMalloc(unsigned int size,char* file,int line);
void* SafeCalloc(unsigned int n,unsigned int size,char* file,int line);
void* SafeRealloc(void* ptr,unsigned int size,char* file,int line);
void  SafeFree(void* ptr,char* file,int line);

/*+ String manipulation functions on public heap +*/

#define MallocString(p) SafeMallocString (p,__FILE__,__LINE__)

char* SafeMallocString(char* x,char* file,int line);

/* String manipulation functions on private memory heap */

char* CopyString(char* x);
char* ConcatStrings(int n,char* s, ...);
void TidyMemory(void);

/* Internal Functions */

void PrintMemoryStatistics(void);

/* Memory handling concepts *

   0) Memory that is private only lasts for the period of parsing the file (including cross-referencing).
   1) All storage for File, Function etc. data types is to be public (permanent).
   2) Data used during parsing is to be private.
   3) Copying of strings is only to be performed if needed, all of the following use the private heap.
      a) Lex code passes constant values to Yacc code where possible.
      b) Lex code is to pass copies of strings to Yacc code since it disappears otherwise.
      c) Yacc code concatanates strings for internal manipulation.
      d) Lex passes pointers (not copies) to comment.c where private copies are made.
   4) Comments from comment.c are passed as private data, the receiver must Malloc a copy of them.

 * Memory handling concepts */

#endif
