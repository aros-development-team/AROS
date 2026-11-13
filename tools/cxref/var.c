/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5f.

  Collects the variable definition stuff.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,99,2004 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

/*+ Control the output of debugging information from this file. +*/
#define DEBUG 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "datatype.h"
#include "parse-yy.h"
#include "cxref.h"

/*+ The file that is currently being documented. +*/
extern File CurFile;

/*+ When in a header file make a note of which one for the included variables. +*/
extern int in_header;

/*+ A list of the variables found at each level of the scope. +*/
static StringList2 *variable;

/*+ The number of levels of scope depth allocated. +*/
static int max_scope=0;

/*+ The current scope depth. +*/
static int cur_scope=-1;


static Variable NewVariableType(char *name,char *type);


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a variable definition is seen.

  char* name The name of the variable.

  char* type The type of the variable.

  int scope The scope of variable that has been seen.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenVariableDefinition(char* name,char* type,int scope)
{
 Variable var;
 int seen=0;

#if DEBUG
 printf("#Var.c# Variable definition for '%s'\n",name);
#endif

 for(var=CurFile->variables;var;var=var->next)
    if(!strcmp(var->name,name))
      {
       var->scope|=scope;
       seen=1;
       if(!in_header && var->scope&EXTERN_H)
         {
          if(var->comment)
             Free(var->comment);
          var->comment=MallocString(GetCurrentComment());
          var->lineno=parse_line;
         }
       break;
      }

 if(!seen)
   {
    var=NewVariableType(name,type);

    var->comment=MallocString(GetCurrentComment());
    var->scope=scope;

    var->lineno=parse_line;

    if(in_header && !(scope&EXTERN_H))
       var->incfrom=MallocString(parse_file);

    AddToLinkedList(CurFile->variables,Variable,var);
   }
}

/*++++++++++++++++++++++++++++++++++++++
  Called when a new scope is entered.
  ++++++++++++++++++++++++++++++++++++++*/

void UpScope(void)
{
 cur_scope++;

#if DEBUG
 printf("#Var.c# Scope ++ (%2d)\n",cur_scope);
#endif

 if(cur_scope>=max_scope)
   {
    if(max_scope==0)
       variable=Malloc(16*sizeof(StringList2));
    else
       variable=Realloc(variable,(max_scope+16)*sizeof(StringList2));
    max_scope+=16;
   }

 variable[cur_scope]=NewStringList2();
}


/*++++++++++++++++++++++++++++++++++++++
  Called when an old scope is exited.
  ++++++++++++++++++++++++++++++++++++++*/

void DownScope(void)
{
#if DEBUG
 printf("#Var.c# Scope -- (%2d)\n",cur_scope);
#endif

 DeleteStringList2(variable[cur_scope]);

 cur_scope--;
}


/*++++++++++++++++++++++++++++++++++++++
  Add a variable to the list of known variables.

  char* name The name of the variable.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenScopeVariable(char* name)
{
#if DEBUG
 printf("#Var.c# Scope Variable depth %2d '%s'\n",cur_scope,name);
#endif

 AddToStringList2(variable[cur_scope],name,NULL,0,0);
}


/*++++++++++++++++++++++++++++++++++++++
  Check through the scope variables to look for the named one.

  int IsAScopeVariable Returns 1 if the name does refer to a variable that is scoped.

  char* name The name of the variable to search for.
  ++++++++++++++++++++++++++++++++++++++*/

int IsAScopeVariable(char* name)
{
 int i,scope;

#if DEBUG
 printf("#Var.c# Lookup variable '%s'\n",name);
#endif

 for(scope=cur_scope;scope>=0;scope--)
    for(i=0;i<variable[scope]->n;i++)
       if(!strcmp(variable[scope]->s1[i],name))
          return(1);

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Tidy up all of the local variables in case of a problem and abnormal parser termination.
  ++++++++++++++++++++++++++++++++++++++*/

void ResetVariableAnalyser(void)
{
 while(cur_scope>=0)
   {
    DeleteStringList2(variable[cur_scope]);
    cur_scope--;
   }

 if(variable) Free(variable);
 variable=NULL;

 max_scope=0;
 cur_scope=-1;
}


/*++++++++++++++++++++++++++++++++++++++
  Create a new variable type.

  Variable NewVariableType Returns a new Variable type.

  char *name The name of the variable.

  char *type The type of the variable.
  ++++++++++++++++++++++++++++++++++++++*/

static Variable NewVariableType(char *name,char *type)
{
 Variable var=(Variable)Calloc(1,sizeof(struct _Variable)); /* clear unused pointers */

 var->name   =MallocString(name);
 var->type   =MallocString(type);
 var->visible=NewStringList2();
 var->used   =NewStringList2();

 return(var);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the specified Variable type.

  Variable var The Variable type to be deleted.
  ++++++++++++++++++++++++++++++++++++++*/

void DeleteVariableType(Variable var)
{
 if(var->comment) Free(var->comment);
 if(var->name)    Free(var->name);
 if(var->type)    Free(var->type);
 if(var->defined) Free(var->defined);
 if(var->incfrom) Free(var->incfrom);
 if(var->visible) DeleteStringList2(var->visible);
 if(var->used)    DeleteStringList2(var->used);
 Free(var);
}
