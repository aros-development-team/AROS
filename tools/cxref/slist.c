/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.4.

  Handle lists of strings.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

/*+ Control the debugging information from this file. +*/
#define DEBUG 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "datatype.h"
#include "cxref.h"

/*++++++++++++++++++++++++++++++++++++++
  Called to initialise a new string list.

  StringList NewStringList Returns an initialised string list.
  ++++++++++++++++++++++++++++++++++++++*/

StringList NewStringList(void)
{
 StringList sl=(StringList)Calloc(1,sizeof(struct _StringList));

#if DEBUG
 printf("#Slist.c# Initialise string list\n");
#endif

 return(sl);
}


/*++++++++++++++++++++++++++++++++++++++
  Called to initialise a new string list 2.

  StringList2 NewStringList2 Returns an initialised string list 2.
  ++++++++++++++++++++++++++++++++++++++*/

StringList2 NewStringList2(void)
{
 StringList2 sl=(StringList2)Calloc(1,sizeof(struct _StringList2));

#if DEBUG
 printf("#Slist.c# Initialise string list 2\n");
#endif

 return(sl);
}


/*++++++++++++++++++++++++++++++++++++++
  Add a string to the string list, the list stores a Malloced copy of str.

  StringList sl The string list to add to.

  char* str The string to add.

  int alphalist If true then the list is sorted into alphabetical order.

  int uniqlist If true then duplicated entries are not allowed to be added.
  ++++++++++++++++++++++++++++++++++++++*/

void AddToStringList(StringList sl,char* str,int alphalist,int uniqlist)
{
 int i;

#if DEBUG
 printf("#Slist.c# Add string %s to the string list\n",str);
#endif

 if(uniqlist)
    for(i=0;i<sl->n;i++)
       if(!strcmp(str,sl->s[i]))
          return;

 if(!sl->n)
    sl->s=(char**)Malloc(8*sizeof(char*));
 else
    if(sl->n%8==0)
       sl->s=(char**)Realloc(sl->s,(sl->n+8)*sizeof(char*));

 if(alphalist)
   {
    char *shuffle=NULL;

    for(i=0;i<sl->n;i++)
       if(shuffle)
         {
          char* temp=sl->s[i];
          sl->s[i]=shuffle;
          shuffle=temp;
         }
       else
          if(strcmp(str,sl->s[i])<0)
            {
             shuffle=sl->s[i];
             sl->s[i]=MallocString(str);
            }

    if(shuffle)
       sl->s[sl->n]=shuffle;
    else
       sl->s[sl->n]=MallocString(str);
   }
 else
    sl->s[sl->n]=MallocString(str);

 sl->n++;
}


/*++++++++++++++++++++++++++++++++++++++
  Add a pair of strings to the string list 2, the list stores a Malloced copy of the arguments.

  StringList2 sl The string list 2 to add to.

  char* str1 The first string to add.

  char* str2 The second string to add.

  int alphalist If true then the list is sorted into alphabetical order of the first string, then second string.

  int uniqlist If true then duplicated entries of the first string are not allowed to be added.
  ++++++++++++++++++++++++++++++++++++++*/

void AddToStringList2(StringList2 sl,char* str1,char* str2,int alphalist,int uniqlist)
{
 int i;

#if DEBUG
 printf("#Slist.c# Add strings %s and %s to the string list 2\n",str1,str2);
#endif

 if(uniqlist)
    for(i=0;i<sl->n;i++)
       if(!strcmp(str1,sl->s1[i]))
          return;

 if(!sl->n)
   {
    sl->s1=(char**)Malloc(8*sizeof(char*));
    sl->s2=(char**)Malloc(8*sizeof(char*));
   }
 else
    if(sl->n%8==0)
      {
       sl->s1=(char**)Realloc(sl->s1,(sl->n+8)*sizeof(char*));
       sl->s2=(char**)Realloc(sl->s2,(sl->n+8)*sizeof(char*));
      }

 if(alphalist)
   {
    char *shuffle1=NULL;
    char *shuffle2=NULL;

    for(i=0;i<sl->n;i++)
       if(shuffle1)
         {
          char* temp1=sl->s1[i];
          char* temp2=sl->s2[i];
          sl->s1[i]=shuffle1;
          sl->s2[i]=shuffle2;
          shuffle1=temp1;
          shuffle2=temp2;
         }
       else
          if(strcmp(str1,sl->s1[i])<0 ||
             (str2 && sl->s2[i] && strcmp(str1,sl->s1[i])==0 && strcmp(str2,sl->s2[i])<0))
            {
             shuffle1=sl->s1[i];
             shuffle2=sl->s2[i];
             sl->s1[i]=MallocString(str1);
             sl->s2[i]=MallocString(str2);
            }

    if(shuffle1)
      {
       sl->s1[sl->n]=shuffle1;
       sl->s2[sl->n]=shuffle2;
      }
    else
      {
       sl->s1[sl->n]=MallocString(str1);
       sl->s2[sl->n]=MallocString(str2);
      }
   }
 else
   {
    sl->s1[sl->n]=MallocString(str1);
    sl->s2[sl->n]=MallocString(str2);
   }

 sl->n++;
}


/*++++++++++++++++++++++++++++++++++++++
  Delete a string list.

  StringList sl The string list to delete.
  ++++++++++++++++++++++++++++++++++++++*/

void DeleteStringList(StringList sl)
{
 int i;

 for(i=0;i<sl->n;i++)
    Free(sl->s[i]);

 if(sl->s)
    Free(sl->s);

 Free(sl);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete a string list 2.

  StringList2 sl The string list 2 to delete.
  ++++++++++++++++++++++++++++++++++++++*/

void DeleteStringList2(StringList2 sl)
{
 int i;

 for(i=0;i<sl->n;i++)
   {
    Free(sl->s1[i]);
    if(sl->s2[i])
       Free(sl->s2[i]);
   }

 if(sl->s1)
    Free(sl->s1);
 if(sl->s2)
    Free(sl->s2);

 Free(sl);
}
