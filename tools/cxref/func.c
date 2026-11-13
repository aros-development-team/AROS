/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5f.

  Handle Function stuff.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,99,2001 Andrew M. Bishop
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
#include "parse-yy.h"
#include "cxref.h"

/*+ The current parsing options. +*/
extern int option_xref;

/*+ The current file that is being processed. +*/
extern File CurFile;

/*+ When in a header file include functions from that file (except inline functions). +*/
extern int in_header;

/*+ The current function, this is initialised by the start of a possible declaration and maintained until all of the
    arguments have been added and confirmation that it is a definition and not a prototype is seen. +*/
static Function cur_func=NULL;

/*+ The list of function prototypes and the files that they are defined in. +*/
static StringList2 prototypes=NULL;

static Function NewFunctionType(char *name,char *type);


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a function prototype is seen.

  char* name The name of the function.

  int in_a_function Whether the reference is from within a function or at the top level of the file.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenFunctionProto(char* name,int in_a_function)
{
 if(!(option_xref&XREF_FUNC))
    return;

#if DEBUG
 printf("#Func.c# Function prototype '%s'\n",name);
#endif

 if(!in_a_function)
   {
    if(!prototypes)
       prototypes=NewStringList2();
    AddToStringList2(prototypes,name,parse_file,0,1);
   }
 else
    AddToStringList(cur_func->protos,name,0,1);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a function declaration is seen.
  This may or may not be a function defintion, we will need to wait and see.

  char* name The name of the function.

  int scope The scope of the function definition.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenFunctionDeclaration(char* name,int scope)
{
#if DEBUG
 printf("#Func.c# Function declaration for '%s()'\n",name);
#endif

 if(cur_func)
    DeleteFunctionType(cur_func);

 cur_func=NewFunctionType(name,NULL);

 cur_func->comment=MallocString(GetCurrentComment());
 cur_func->scope=scope;

 cur_func->lineno=parse_line;

 if(in_header)
    cur_func->incfrom=MallocString(parse_file);
}


/*++++++++++++++++++++++++++++++++++++++
  Called when a possible function definition is confirmed.

  char* type The type of the function, or NULL at the end of a definition.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenFunctionDefinition(char* type)
{
 Function *func=&CurFile->functions;
 int i;

 if(cur_func->scope&INLINED && cur_func->incfrom)
    return;

#if DEBUG
 printf("#Func.c# Function definition %s for '%s()'\n",type?"start":"end",cur_func->name);
#endif

 if(!type)
   {cur_func=NULL;return;}

 cur_func->type=MallocString(type);

 cur_func->cret=MallocString(SplitComment(&cur_func->comment,type));
 if(!cur_func->cret)
    cur_func->cret=MallocString(GetCurrentComment());

 if(option_xref&XREF_FUNC)
    if(prototypes)
       for(i=0;i<prototypes->n;i++)
          if(!strcmp(cur_func->name,prototypes->s1[i]))
            {cur_func->protofile=MallocString(prototypes->s2[i]); break;}

 for(i=0;i<cur_func->args->n;i++)
    if(strcmp(cur_func->args->s1[i],"void") && strcmp(cur_func->args->s1[i],"...") && !strchr(cur_func->args->s1[i],' '))
      {
       char *old=cur_func->args->s1[i];
       cur_func->args->s1[i]=MallocString(ConcatStrings(2,"int ",old));
       cur_func->args->s2[i]=MallocString(SplitComment(&cur_func->comment,cur_func->args->s1[i]));
       Free(old);
      }

 while(*func)
   {
    if(strcmp(cur_func->name,(*func)->name)<0)
      {
       Function temp=*func;
       *func=cur_func;
       cur_func->next=temp;
       break;
      }
    func=&(*func)->next;
   }

 if(!cur_func->next)
    *func=cur_func;
}

/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a function argument is seen in the current function declaration.

  char* name The name of the argument.

  char* type The type of the argument, or NULL if a traditional style function definition.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenFunctionArg(char* name,char *type)
{
#if DEBUG
 printf("#Func.c# Function arg %s '%s' in %s()\n",name?name:"K&R",type?type:"K&R",cur_func->name);
#endif

 if(name)
   {
    if(type && strcmp(type,"void"))
      {
       int i;

       for(i=0;i<cur_func->args->n;i++)
          if(!strcmp(cur_func->args->s1[i],name))
            {
             Free(cur_func->args->s1[i]);
             cur_func->args->s1[i]=MallocString(type);
             cur_func->args->s2[i]=MallocString(SplitComment(&cur_func->comment,type));
             break;
            }
       if(i==cur_func->args->n)
          AddToStringList2(cur_func->args,type,SplitComment(&cur_func->comment,type),0,0);

       if(!cur_func->args->s2[i])
          cur_func->args->s2[i]=MallocString(GetCurrentComment());
      }
    else
       AddToStringList2(cur_func->args,name,NULL,0,0);
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a comment is seen, that may be in a function body.

  int SeenFuncIntComment Returns a true value if the comment was accepted as an function internal comment.

  char* comment The comment that has been seen.
  ++++++++++++++++++++++++++++++++++++++*/

int SeenFuncIntComment(char* comment)
{
 if(!cur_func || !cur_func->type)
    return(0);

#if DEBUG
 printf("#Func.c# Function internal comment '%s' in %s()\n",comment,cur_func->name);
#endif

 if(cur_func->comment)
   {
    char* c=cur_func->comment;

    cur_func->comment=MallocString(ConcatStrings(3,c,"\n\n",comment));
    Free(c);
   }
 else
    cur_func->comment=MallocString(comment);

 return(1);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a function call is seen in the current function.

  char* name The name of the function that is called.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenFunctionCall(char* name)
{
 if(!(option_xref&XREF_FUNC))
    return;

#if DEBUG
 printf("#Func.c# Function call for '%s()' in %s()\n",name,cur_func->name);
#endif

 AddToStringList2(cur_func->calls,name,NULL,1,1);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a function or variable is referenced in the current function.

  char* name The name of the function or variable that is referenced.

  int in_a_function Whether the reference is from within a function or at the top level of the file.
  ++++++++++++++++++++++++++++++++++++++*/

void CheckFunctionVariableRef(char* name,int in_a_function)
{
 Variable var =CurFile->variables;
 Function func=CurFile->functions;
 StringList2 sl=NULL;

 if(!(option_xref&(XREF_VAR|XREF_FUNC)))
    return;

 if(IsAScopeVariable(name))
    return;

#if DEBUG
 printf("#Func.c# Function/Variable reference for '%s' in %s\n",name,in_a_function?cur_func->name:CurFile->name);
#endif

 if(option_xref&XREF_VAR)
    while(var)
      {
       if(!strcmp(var->name,name))
         {
          if(in_a_function)
             sl=cur_func->v_refs;
          else
             sl=CurFile->v_refs;
          break;
         }
       var=var->next;
      }

 if(!sl && option_xref&XREF_FUNC)
    while(func)
      {
       if(!strcmp(func->name,name))
         {
          if(in_a_function)
             sl=cur_func->f_refs;
          else
             sl=CurFile->f_refs;
          break;
         }
       func=func->next;
      }

 if(!sl && option_xref&XREF_FUNC)
   {
    int i;
    if(in_a_function)
       for(i=0;i<cur_func->protos->n;i++)
          if(!strcmp(name,cur_func->protos->s[i]))
            {
             sl=cur_func->f_refs;
             break;
            }

    if(!sl && prototypes)
       for(i=0;i<prototypes->n;i++)
          if(!strcmp(name,prototypes->s1[i]))
            {
             if(in_a_function)
                sl=cur_func->f_refs;
             else
                sl=CurFile->f_refs;
             break;
            }
   }

 /* Now add the function or variable to the Function / File structure. */

 if(sl)
    AddToStringList2(sl,name,NULL,1,1);
}


/*++++++++++++++++++++++++++++++++++++++
  Tidy up all of the local variables in case of a problem and abnormal parser termination.
  ++++++++++++++++++++++++++++++++++++++*/

void ResetFunctionAnalyser(void)
{
 if(prototypes) DeleteStringList2(prototypes);
 prototypes=NULL;

 if(cur_func)
   {
    Function func=CurFile->functions;
    int delete_cur_func=1;

    while(func)
      {
       if(func==cur_func)
          delete_cur_func=0;

       func=func->next;
      }

    if(delete_cur_func)
       DeleteFunctionType(cur_func);

    cur_func=NULL;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Create a new Function Type variable.

  Function NewFunctionType Returns the new Function variable.

  char *name The name of the function.

  char *type The type of the function.
  ++++++++++++++++++++++++++++++++++++++*/

static Function NewFunctionType(char *name,char *type)
{
 Function func=(Function)Calloc(1,sizeof(struct _Function)); /* clear unused pointers */

 func->name  =MallocString(name);
 func->type  =MallocString(type);
 func->args  =NewStringList2();
 func->protos=NewStringList();
 func->calls =NewStringList2();
 func->called=NewStringList2();
 func->used  =NewStringList2();
 func->v_refs=NewStringList2();
 func->f_refs=NewStringList2();

 return(func);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the specified Function type.

  Function func The Function type to be deleted.
  ++++++++++++++++++++++++++++++++++++++*/

void DeleteFunctionType(Function func)
{
 if(func->comment)   Free(func->comment);
 if(func->name)      Free(func->name);
 if(func->type)      Free(func->type);
 if(func->cret)      Free(func->cret);
 if(func->protofile) Free(func->protofile);
 if(func->incfrom)   Free(func->incfrom);
 if(func->args)      DeleteStringList2(func->args);
 if(func->protos)    DeleteStringList(func->protos);
 if(func->calls)     DeleteStringList2(func->calls);
 if(func->called)    DeleteStringList2(func->called);
 if(func->used)      DeleteStringList2(func->used);
 if(func->v_refs)    DeleteStringList2(func->v_refs);
 if(func->f_refs)    DeleteStringList2(func->f_refs);
 Free(func);
}
