/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.4.

  Sets up the top level File structure.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

/*+ To control the debugging in this file. +*/
#define DEBUG 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "datatype.h"
#include "cxref.h"

/*+ This contains the File that is currently being documented to allow the other functions access to it. +*/
extern File CurFile;

/*++++++++++++++++++++++++++++++++++++++
  Creates a new File structure.

  File NewFile Returns the new file structure.

  char* name The name of the file.
  ++++++++++++++++++++++++++++++++++++++*/

File NewFile(char* name)
{
 File file=(File)Calloc(1,sizeof(struct _File));

 file->name=MallocString(name);
 file->inc_in=NewStringList();
 file->f_refs=NewStringList2();
 file->v_refs=NewStringList2();

 return(file);
}


/*++++++++++++++++++++++++++++++++++++++
  Called when a file comment has been seen. Only the first of multiple comments in a file are used.

  char* comment The comment for the file.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenFileComment(char* comment)
{
 if(!CurFile->comment)
    CurFile->comment=MallocString(comment);
}


/*++++++++++++++++++++++++++++++++++++++
  Deletes a file structure.

  File file The file structure to be deleted.

  This is required to go through each of the elements in the File structure and delete each of them in turn.
  ++++++++++++++++++++++++++++++++++++++*/

void DeleteFile(File file)
{
 if(file->comment) Free(file->comment);
 if(file->name)    Free(file->name);

 if(file->inc_in)  DeleteStringList(file->inc_in);
 if(file->f_refs)  DeleteStringList2(file->f_refs);
 if(file->v_refs)  DeleteStringList2(file->v_refs);

 if(file->includes)
   {
    Include p=file->includes;
    do{
       Include n=p->next;
       DeleteIncludeType(p);
       p=n;
      }
    while(p);
   }

 if(file->defines)
   {
    Define p=file->defines;
    do{
       Define n=p->next;
       DeleteDefineType(p);
       p=n;
      }
    while(p);
   }

 if(file->typedefs)
   {
    Typedef p=file->typedefs;
    do{
       Typedef n=p->next;
       DeleteTypedefType(p);
       p=n;
      }
    while(p);
   }

 if(file->variables)
   {
    Variable p=file->variables;
    do{
       Variable n=p->next;
       DeleteVariableType(p);
       p=n;
      }
    while(p);
   }

 if(file->functions)
   {
    Function p=file->functions;
    do{
       Function n=p->next;
       DeleteFunctionType(p);
       p=n;
      }
    while(p);
   }

 Free(file);
}
