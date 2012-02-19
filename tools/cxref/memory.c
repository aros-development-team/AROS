/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5.

  Memory management functions
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

/*+ The amount of debugging, non-zero for totals, 2 for logging, 4 for printing each call. +*/
#define DEBUG 0

/* The configure output */

#include "autoconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_STD_ARG
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "memory.h"

/*+ A private memory heap is used to reduce the number of malloc calls that are made, the Heap type is a pointer to this. +*/
typedef struct _Heap *Heap;

/*+ A structure containing all of the information about the private heap in a linked list. +*/
struct _Heap
{
 char* mem;                             /*+ The memory that is private to the heap. +*/
 Heap next;                             /*+ The next Heap structure. +*/
};

/*+ Local variable to control the usage of the private heap; +*/
static Heap first=NULL;                 /*+ the first segment of memory on the private heap. +*/
static int heap_left=0;                 /*+ the amount of space left in the current heap segment. +*/

static char* get_space(unsigned int l);
static Heap add_to_heap(unsigned int l);

#if DEBUG&2
/*+ Variable used for debugging, not a good thing to do. what if more than 16384 mallocs? +*/
static void* addresses[16384];
static char* files[16384];
static int   lines[16384];
#endif
#if DEBUG
/*+ Variable used for debugging. +*/
static int malloc_count=0;
static int realloc_count=0;
static int free_count=0;
#endif


/*++++++++++++++++++++++++++++++++++++++
  A replacement malloc() function.

  void* SafeMalloc Returns the address.

  unsigned int size The size of the memory to allocate.

  char* file The file that the function is called from.

  int line The line number that the function is called from.
  ++++++++++++++++++++++++++++++++++++++*/

void* SafeMalloc(unsigned int size,char* file,int line)
{
 void* rptr=malloc(size);

#if DEBUG&4
 printf("$$Malloc #%5d of %4d bytes at %08lx (%s:%3d)\n",malloc_count+1,size,(long)rptr,file,line);
#endif
#if DEBUG&2
 if(malloc_count==(sizeof(addresses)/sizeof(addresses[0])))
   {fprintf(stderr,"$$Too many Mallocs to log, edit memory.c\n");exit(3);}
 addresses[malloc_count]=(void*)rptr;
 files[malloc_count]=file;
 lines[malloc_count]=line;
#endif
#if DEBUG
 malloc_count++;
 if(!rptr) printf("$$Warning Malloc() returning NULL (%s:%3d)\n",file,line);
#endif
#if !DEBUG
 if(!rptr) printf("Warning Malloc() returning NULL (%s:%3d)\n",file,line);
#endif

 return(rptr);
}


/*++++++++++++++++++++++++++++++++++++++
  A replacement calloc() function.

  void* SafeCalloc Returns the address.

  unsigned int n The number of items to allocate.

  unsigned int size The size of the memory to allocate.

  char* file The file that the function is called from.

  int line The line number that the function is called from.
  ++++++++++++++++++++++++++++++++++++++*/

void* SafeCalloc(unsigned int n,unsigned int size,char* file,int line)
{
 void* rptr=calloc(n,size);

#if DEBUG&4
 printf("$$Calloc #%5d of %4d bytes at %08lx (%s:%3d)\n",malloc_count+1,size,(long)rptr,file,line);
#endif
#if DEBUG&2
 if(malloc_count==(sizeof(addresses)/sizeof(addresses[0])))
   {fprintf(stderr,"$$Too many Mallocs to log, edit memory.c\n");exit(3);}
 addresses[malloc_count]=(void*)rptr;
 files[malloc_count]=file;
 lines[malloc_count]=line;
#endif
#if DEBUG
 malloc_count++;
 if(!rptr) printf("$$Warning Calloc() returning NULL (%s:%3d)\n",file,line);
#endif
#if !DEBUG
 if(!rptr) printf("Warning Calloc() returning NULL (%s:%3d)\n",file,line);
#endif

 return(rptr);
}


/*++++++++++++++++++++++++++++++++++++++
  A replacement realloc() function.

  void* SafeRealloc Returns the address.

  void* ptr The old pointer.

  unsigned int size The size of the new memory to allocate.

  char* file The file that the function is called from.

  int line The line number that the function is called from.
  ++++++++++++++++++++++++++++++++++++++*/

void* SafeRealloc(void* ptr,unsigned int size,char* file,int line)
{
 void* rptr=realloc(ptr,size);

#if DEBUG&4
 printf("$$Realloc #%4d of %4d bytes at %08lx (old %08lx) (%s:%3d)\n",realloc_count+1,size,(long)rptr,(long)ptr,file,line);
#endif
#if DEBUG&2
 {
  int i;
  for(i=0;i<malloc_count;i++)
     if(addresses[i]==(void*)ptr)
       {addresses[i]=rptr;break;}
  if(i==malloc_count)
     printf("$$Realloc() called for a non Malloced pointer %08lx (%s:%3d)\n",(long)ptr,file,line);
 }
#endif
#if DEBUG
 realloc_count++;
 if(!rptr) printf("$$Warning Realloc() returning NULL (%s:%3d)\n",file,line);
#endif
#if !DEBUG
 if(!rptr) printf("Warning Realloc() returning NULL (%s:%3d)\n",file,line);
#endif

 return(rptr);
}


/*++++++++++++++++++++++++++++++++++++++
  A replacement free() function.

  void* ptr The pointer that is to be freed up.

  char* file The file that the function is called from.

  int line The line number that the function is called from.
  ++++++++++++++++++++++++++++++++++++++*/

void SafeFree(void* ptr,char* file,int line)
{
#if DEBUG&4
 printf("$$Free #%5d at %08lx (%s:%3d)\n",free_count+1,(long)ptr,file,line);
#endif
#if DEBUG&2
 {
  int i;
  for(i=0;i<malloc_count;i++)
     if(addresses[i]==(void*)ptr)
       {addresses[i]=(void*)1;break;} 
  if(i==malloc_count)
     printf("$$Free() called for a non Malloced pointer %08lx (%s:%3d)\n",(long)ptr,file,line);
 }
#endif
#if DEBUG
 free_count++;
 if(!ptr) printf("$$Calling Free() on NULL (%s:%3d)\n",file,line);
 else
#endif
#if !DEBUG
 if(!ptr) printf("Calling Free() on NULL (%s:%3d)\n",file,line);
 else
#endif

 free(ptr);
}


/*++++++++++++++++++++++++++++++++++++++
  A function to copy a string on the public global heap.

  char* SafeMallocString Returns the copy of the string.

  char* x The string to be copied.

  char* file The file that the function is called from.

  int line The line number that the function is called from.
  ++++++++++++++++++++++++++++++++++++++*/

char* SafeMallocString(char* x,char* file,int line)
{
 char* t=NULL;

 if(x)
   {
    t=(char*)SafeMalloc(strlen(x)+1,file,line);
    strcpy(t,x);
   }

 return(t);
}


/*++++++++++++++++++++++++++++++++++++++
  A function to copy a string on the local private memory heap.

  char* CopyString Returns the copy of the string.

  char* x The string to be copied.
  ++++++++++++++++++++++++++++++++++++++*/

char* CopyString(char* x)
{
 char* t=NULL;

 if(x)
   {
    t=get_space(strlen(x)+1);
    strcpy(t,x);
   }

 return(t);
}


/*++++++++++++++++++++++++++++++++++++++
  A function to concatenate a number of strings.

  char* ConcatStrings Returns the a pointer to the new string.

  int n The number of strings

  char* s The first string.

  ... The other strings, 'n' including 's'.

  Any of the strings that are inputs can be NULL, in this case they are quietly ignored.
  ++++++++++++++++++++++++++++++++++++++*/

char* ConcatStrings(int n,char* s, ...)
{
 char* t=NULL,*str;
 unsigned int l=0;
 int i;
 va_list ap;

#ifdef USE_STD_ARG
 va_start(ap,s);
#else
 va_start(ap);
#endif

 for(i=0;i<n;i++)
   {
    if(i)
       str=va_arg(ap, char *);
    else
       str=s;

    if(str)
       l+=strlen(str);
   }

 va_end(ap);

 if(l)
   {
    t=get_space(l+1); t[0]=0;

#ifdef USE_STD_ARG
    va_start(ap,s);
#else
    va_start(ap);
#endif

    for(i=0;i<n;i++)
      {
       if(i)
          str=va_arg(ap, char *);
       else
          str=s;

       if(str)
          strcat(t,str);
      }

    va_end(ap);
   }

 return(t);
}


/*++++++++++++++++++++++++++++++++++++++
  Prints out the number of mallocs / reallocs and frees.
  ++++++++++++++++++++++++++++++++++++++*/

void PrintMemoryStatistics(void)
{
#if DEBUG
 printf("\n"
        "$$Memory usage : %5d Malloc()/Calloc() calls\n"
        "$$               %5d Realloc() calls\n"
        "$$               %5d Free() calls\n"
        "$$               %5d Net calls (Malloc-Free)\n",
        malloc_count,realloc_count,free_count,malloc_count-free_count);
#endif

#if DEBUG&2
 {
  int i;
  for(i=0;i<malloc_count;i++)
     if(addresses[i]!=(void*)1)
        printf("$$Malloc #%5d at address %08lx is not freed (%s:%3d) = '%s'\n",i+1,(long)addresses[i],files[i],lines[i],(char*)addresses[i]);
 }
#endif
}


/*++++++++++++++++++++++++++++++++++++++
  Tidies up the local heap of memory.
  ++++++++++++++++++++++++++++++++++++++*/

void TidyMemory(void)
{
 if(first)
   {
    Heap h=first,n;
    do
      {
       n=h->next;
       Free(h->mem);
       Free(h);
       h=n;
      }
    while(h);
   }

 first=NULL;
 heap_left=0;
}

/*+ The size of each of the heap allocations +*/
#define HEAP_INC 8192

/*+ The size of a string that is large enough to have it's own mallocation. +*/
#define SMALL_STRING 256

/*++++++++++++++++++++++++++++++++++++++
  A function to get some memory for a string, allocate a new heap structure if needed.

  char* get_space Returns a pointer to enough space.

  unsigned int l The amount of space that is needed.
  ++++++++++++++++++++++++++++++++++++++*/

static char* get_space(unsigned int l)
{
 static Heap current=NULL;
 char* r=NULL;

 if(l <= SMALL_STRING)
   {
    if(heap_left < l)
      {
       current=add_to_heap(HEAP_INC);
       heap_left=HEAP_INC;
      }

    heap_left-=l;

    r=&current->mem[heap_left]; /* Work downwards */
   }
 else
   {
    Heap h=add_to_heap(l);
    r=h->mem;
   }

 return(r);
}


/*++++++++++++++++++++++++++++++++++++++
  Add some bytes to the privately maintained memory heap.

  Heap add_to_heap Returns a pointer to the required memory.

  unsigned int l The size of the memory that is required.
  ++++++++++++++++++++++++++++++++++++++*/

static Heap add_to_heap(unsigned int l)
{
 Heap* h=&first;

 while(*h)
    h=&(*h)->next;

 *h=(Heap)Malloc(sizeof(struct _Heap));
 (*h)->next=NULL;
 (*h)->mem=(char*)Malloc(l);

 return(*h);
}
