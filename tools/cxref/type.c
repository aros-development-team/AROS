/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5.

  Collects the typedef stuff.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,99 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

/*+ Control the output of debugging information in this file. +*/
#define DEBUG 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "datatype.h"
#include "parse-yy.h"
#include "cxref.h"

/*+ The file that is currently being processed. +*/
extern File CurFile;

/*+ Whether we are parsing a typedef or not. +*/
extern int in_typedef;

/*+ The defined types that we have seen. +*/
static StringList2 typedefs=NULL;

/*+ The current struct / union or enum definition. +*/
static StructUnion cur_su=NULL;

/*+ The current struct / union if seen in a typedef. +*/
static StructUnion cur_type_su=NULL;

/*+ The last typedef seen, used when two types share a typedef statement. +*/
static Typedef last_typedef=NULL;

/*+ The line number that a typedef or structure was seen on. +*/
static int type_lineno=0;

static Typedef NewTypedefType(char *name,char *type);
static StructUnion NewStructUnionType(char *name);
static void DeleteStructUnionType(StructUnion su);
static StructUnion CopyStructUnion(StructUnion su);


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a typedef is seen in the current file. The name of the typedef is stored for future reference.

  char* name The name of the defined type.

  int what_type Set to 1 for normal types or -1 for a function type (not pointer to function).
  ++++++++++++++++++++++++++++++++++++++*/

void SeenTypedefName(char* name,int what_type)
{
#if DEBUG
 printf("#Type.c# Type defined '%s'\n",name);
#endif

 if(!typedefs)
    typedefs=NewStringList2();
 AddToStringList2(typedefs,name,what_type==0?"\0":what_type>0?"n":"f",0,1);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when an IDENTIFIER is seen in the current file, it may be a defined type.

  int IsATypeName Returns 1 if the argument is a type that has been defined.

  char* name The name of the possible defined type.
  ++++++++++++++++++++++++++++++++++++++*/

int IsATypeName(char* name)
{
 int i;

 if(typedefs)
    for(i=0;i<typedefs->n;i++)
       if(!strcmp(name,typedefs->s1[i]))
          return((int)*typedefs->s2[i]);

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when the start of a struct or union or enum definition is seen.

  char* name The name of the struct type.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenStructUnionStart(char* name)
{
#if DEBUG
 printf("#Type.c# Start Struct / Union '%s'\n",name);
#endif

 cur_su=NewStructUnionType(name);

 if(!in_typedef)
    cur_su->comment=MallocString(GetCurrentComment());

 type_lineno=parse_line;
}


/*++++++++++++++++++++++++++++++++++++++
  Function called when a component of a struct / union / enum is seen.

  char* name The name of the struct / union / enum component.

  int depth The depth within the nested struct / union / enum.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenStructUnionComp(char* name,int depth)
{
 StructUnion s,t=cur_su;
 int i;

#if DEBUG
 printf("#Type.c# Struct / Union Component (%d) '%s'\n",depth,name);
#endif

 for(i=1;i<depth;i++,t=s)
    s=t->comps[t->n_comp-1];

 if(t->comps && strchr(name,'{'))
   {
    char* ob=strchr(name,'{');
    char* cb=strchr(name,'}'),*nb;

    while((nb=strchr(cb+1,'}'))) cb=nb;
    ob[1]=0;
    if(strcmp(name,"enum {") && strcmp(name,"union {") && strcmp(name,"struct {"))
      {
       Typedef typdef=NewTypedefType(t->comps[t->n_comp-1]->name,NULL);

       typdef->comment=MallocString(GetCurrentComment());
       t->comps[t->n_comp-1]->comment=MallocString(typdef->comment);

       typdef->sutype=CopyStructUnion(t->comps[t->n_comp-1]);

       AddToLinkedList(CurFile->typedefs,Typedef,typdef);
      }
    else
       t->comps[t->n_comp-1]->comment=MallocString(GetCurrentComment());

    Free(t->comps[t->n_comp-1]->name);
    t->comps[t->n_comp-1]->name=MallocString(ConcatStrings(3,name,"...",cb));
   }
 else
   {
    if(!t->comps)
       t->comps=(StructUnion*)Malloc(8*sizeof(StructUnion));
    else
       if(t->n_comp%8==0)
          t->comps=(StructUnion*)Realloc(t->comps,(t->n_comp+8)*sizeof(StructUnion));

    s=NewStructUnionType(name);
    s->comment=MallocString(GetCurrentComment());

    t->comps[t->n_comp++]=s;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when the end of a struct or union or enum definition is seen.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenStructUnionEnd(void)
{
#if DEBUG
 printf("#Type.c# End Struct / Union\n");
#endif

 if(in_typedef)
    cur_type_su=cur_su;
 else
   {
    Typedef xref=CurFile->typedefs;
    Typedef typdef=NewTypedefType(cur_su->name,NULL);

    while(xref)
      {
       if(xref->type && !strncmp(cur_su->name,xref->type,strlen(cur_su->name)))
          xref->typexref=typdef;
       xref=xref->next;
      }

    typdef->comment=cur_su->comment; cur_su->comment=NULL;
    typdef->sutype=cur_su;

    typdef->lineno=type_lineno;

    AddToLinkedList(CurFile->typedefs,Typedef,typdef);
   }

 cur_su=NULL;
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a typedef is seen in the current file. This is recorded fully for later output.

  char* name The name of the defined type.

  char* type The type that it is defined to be.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenTypedef(char* name,char* type)
{
 Typedef typdef;

 if(!name)
   {
    last_typedef=NULL;
    type_lineno=parse_line;
    return;
   }

#if DEBUG
 printf("#Type.c# Typedef '%s' '%s'\n",name,type);
#endif

 typdef=NewTypedefType(name,type);

 typdef->comment=MallocString(GetCurrentComment());

 if(!cur_type_su)
   {
    Typedef xref=CurFile->typedefs;
    typdef->sutype=NULL;
    typdef->typexref=NULL;
    while(xref)
      {
       if(!strncmp(xref->name,typdef->type,strlen(xref->name)))
          typdef->typexref=xref;
       xref=xref->next;
      }
    if(!typdef->typexref)
       typdef->typexref=last_typedef;
   }
 else
   {
    typdef->sutype=cur_type_su;
    cur_type_su=NULL;
    typdef->typexref=NULL;
   }

 typdef->lineno=type_lineno;

 if(!typdef->typexref)
    last_typedef=typdef;

 AddToLinkedList(CurFile->typedefs,Typedef,typdef);
}


/*++++++++++++++++++++++++++++++++++++++
  Tidy up all of the local variables in case of a problem and abnormal parser termination.
  ++++++++++++++++++++++++++++++++++++++*/

void ResetTypeAnalyser(void)
{
 if(typedefs) DeleteStringList2(typedefs);
 typedefs=NULL;

 cur_su=NULL;

 cur_type_su=NULL;

 last_typedef=NULL;
}


/*++++++++++++++++++++++++++++++++++++++
  Create a new Typedef type.

  Typedef NewTypedefType Returns the new type.

  char *name The name of the type.

  char *type The type of the type.
  ++++++++++++++++++++++++++++++++++++++*/

static Typedef NewTypedefType(char *name,char *type)
{
 Typedef typed=(Typedef)Calloc(1,sizeof(struct _Typedef)); /* clear unused pointers */

 typed->name=MallocString(name);
 typed->type=MallocString(type);

 return(typed);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the specified Typedef type.

  Typedef type The Typedef type to be deleted.
  ++++++++++++++++++++++++++++++++++++++*/

void DeleteTypedefType(Typedef type)
{
 if(type->comment) Free(type->comment);
 if(type->name)    Free(type->name);
 if(type->type)    Free(type->type);
 if(type->sutype)  DeleteStructUnionType(type->sutype);
 Free(type);
}


/*++++++++++++++++++++++++++++++++++++++
  Create a new struct / union type.

  StructUnion NewStructUnionType Return the new StructUnion type.

  char *name The name of the new struct / union.
  ++++++++++++++++++++++++++++++++++++++*/

static StructUnion NewStructUnionType(char *name)
{
 StructUnion su=(StructUnion)Calloc(1,sizeof(struct _StructUnion));

 su->name=MallocString(name);

 return(su);
}


/*++++++++++++++++++++++++++++++++++++++
  Free the memory associated with a Struct / Union structure.

  StructUnion su The struct / union to delete.

  This needs to call itself recursively.
  ++++++++++++++++++++++++++++++++++++++*/

static void DeleteStructUnionType(StructUnion su)
{
 int i;

 if(su->name)    Free(su->name);
 if(su->comment) Free(su->comment);
 for(i=0;i<su->n_comp;i++)
    if(su->comps[i])
       DeleteStructUnionType(su->comps[i]);
 if(su->comps)   Free(su->comps);
 Free(su);
}


/*++++++++++++++++++++++++++++++++++++++
  Make a copy of the specified Struct / Union structure.

  StructUnion CopyStructUnion Returns a malloced copy of the specified struct / union.

  StructUnion su The struct / union to copy.

  This needs to call itself recursively.
  ++++++++++++++++++++++++++++++++++++++*/

static StructUnion CopyStructUnion(StructUnion su)
{
 StructUnion new;
 int i;

 new=NewStructUnionType(su->name);

 new->comment=MallocString(su->comment);
 new->n_comp=su->n_comp;
 if(su->n_comp)
   {
    new->comps=(StructUnion*)Malloc(su->n_comp*sizeof(StructUnion));
    for(i=0;i<su->n_comp;i++)
       new->comps[i]=CopyStructUnion(su->comps[i]);
   }

 return(new);
}
