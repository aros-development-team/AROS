/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.4.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../memory.h"
#include "../datatype.h"
#include "../cxref.h"
#include "query.h"


/*+ The command line switch that sets the amount of cross referencing to do. +*/
extern int option_xref;

extern File *files;             /*+ The files that are queried. +*/
extern int n_files;             /*+ The number of files referenced. +*/

extern Function *functions;     /*+ The functions that are queried. +*/
extern int n_functions;         /*+ The number of functions referenced. +*/

extern Variable *variables;     /*+ The variables that are queried. +*/
extern int n_variables;         /*+ The number of variables referenced. +*/

extern Typedef *typedefs;       /*+ The type definitions that are queried. +*/
extern int n_typedefs;          /*+ The number of typedefs referenced. +*/

/* Local fuctions */

static void OutputFile(File file);
static void OutputInclude(Include incl,int depth);
static void OutputFunction(Function func);
static void OutputVariable(Variable var);
static void OutputTypedef(Typedef type);


/*++++++++++++++++++++++++++++++++++++++
  Ouput the cross references for the named thing.

  char* name The name of the object to ouput the cross references for.
  ++++++++++++++++++++++++++++++++++++++*/

void OutputCrossRef(char* name)
{
 int i,any=0;

 for(i=0;i<n_files;i++)
    if(!strcmp(name,files[i]->name))
       {OutputFile(files[i]);any++;}

 for(i=0;i<n_typedefs;i++)
    if(!strcmp(name,typedefs[i]->name))
      {OutputTypedef(typedefs[i]);any++;}

 for(i=0;i<n_variables;i++)
    if(!strcmp(name,variables[i]->name))
      {OutputVariable(variables[i]);any++;}

 for(i=0;i<n_functions;i++)
    if(!strcmp(name,functions[i]->name))
      {OutputFunction(functions[i]);any++;}

 if(!any)
    printf("cxref-query: No match for '%s'.\n",name);
}

/*++++++++++++++++++++++++++++++++++++++
  Print out the information for a file.

  File file The file to output the information for.
  ++++++++++++++++++++++++++++++++++++++*/

static void OutputFile(File file)
{
 int i;
 Include inc;
 Function func;
 Variable var;
 Typedef type;

 printf("File: %s\n\n",file->name);

 for(inc=file->includes,i=0;inc;inc=inc->next,i++)
   {
    printf("    %s %c%s%c\n",i?"          ":"Includes: ",inc->scope==GLOBAL?'<':'"',inc->name,inc->scope==GLOBAL?'>':'"');
    OutputInclude(inc,1);
   }
 if(file->includes)
    printf("\n");

 for(func=file->functions,i=0;func;func=func->next,i++)
    printf("    %s %s\n",i?"          ":"Functions:",func->name);
 if(file->functions)
    printf("\n");

 for(var=file->variables,i=0;var;var=var->next,i++)
    printf("    %s %s\n",i?"          ":"Variables:",var->name);
 if(file->variables)
    printf("\n");

 for(type=file->typedefs,i=0;type;type=type->next,i++)
    printf("    %s %s\n",i?"          ":"Types:    ",type->name);
 if(file->typedefs)
    printf("\n");

 if(option_xref&XREF_FILE)
   {
    for(i=0;i<file->inc_in->n;i++)
       printf("   %s %s\n",i?"            ":"Included in:",file->inc_in->s[i]);
    if(file->inc_in->n)
       printf("\n");
   }

 if(option_xref&XREF_FUNC)
   {
    for(i=0;i<file->f_refs->n;i++)
       printf("   %s %s : %s\n",i?"            ":"Refs func:  ",file->f_refs->s1[i],file->f_refs->s2[i]);
    if(file->f_refs->n)
       printf("\n");
   }

 if(option_xref&XREF_VAR)
   {
    for(i=0;i<file->v_refs->n;i++)
       printf("   %s %s : %s\n",i?"            ":"Refs var:   ",file->v_refs->s1[i],file->v_refs->s2[i]);
    if(file->v_refs->n)
       printf("\n");
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Print out the information for an include file.

  Include incl The include file to output the information for.

  int depth The depth of the includes.
  ++++++++++++++++++++++++++++++++++++++*/

static void OutputInclude(Include incl,int depth)
{
 int c,i;
 Include inc;

 for(inc=incl->includes,c=0;inc;inc=inc->next,c++)
   {
    for(i=0;i<depth;i++) fputs("   ",stdout);
    printf("               %c%s%c\n",inc->scope==GLOBAL?'<':'"',inc->name,inc->scope==GLOBAL?'>':'"');
    OutputInclude(inc,depth+1);
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Print out the information for a typedef.

  Typedef type The typedef to output the information for.
  ++++++++++++++++++++++++++++++++++++++*/

static void OutputTypedef(Typedef type)
{
 printf("In file: %s\n",type->comment);

 if(type->type)
    printf("Typedef: %s = %s\n",type->name,type->type);
 else
    printf("Type:    %s\n",type->name);

 printf("\n");
}


/*++++++++++++++++++++++++++++++++++++++
  Print out the information for a variable.

  Variable var The variable to output the information for.
  ++++++++++++++++++++++++++++++++++++++*/

static void OutputVariable(Variable var)
{
 int i,done=0;

 printf("In file: %s\n",var->comment);

 printf("Variable: %s [",var->name);
 if(var->scope&LOCAL)    done=printf("Local");
 if(var->scope&GLOBAL)   done=printf("%sGlobal definition",done?" and ":"");
 if(var->scope&EXTERNAL) done=printf("%sExternal",done?" and ":"");
 if(var->scope&EXTERN_H) done=printf("%sExternal from header file",done?" and ":"");
 if(var->scope&EXTERN_F) done=printf("%sExternal within function",done?" and ":"");
 printf("]\n");

 if(option_xref&XREF_VAR)
   {
    if(var->scope&(GLOBAL|LOCAL))
       for(i=0;i<var->visible->n;i++)
          printf("   %s %s : %s\n",i?"            ":"Visible in: ",var->visible->s1[i],var->visible->s2[i]);

    for(i=0;i<var->used->n;i++)
       if(var->used->s1[i][0]=='$')
          printf("   %s %s\n",i?"            ":"Used in:    ",var->used->s2[i]);
       else
          printf("   %s %s : %s\n",i?"            ":"Used in:    ",var->used->s1[i],var->used->s2[i]);
   }

 printf("\n");
}


/*++++++++++++++++++++++++++++++++++++++
  Print out the information for a function.

  Function func The function to output the information for.
  ++++++++++++++++++++++++++++++++++++++*/

static void OutputFunction(Function func)
{
 int i;

 printf("In file: %s\n",func->comment);

 printf("Function: %s ",func->name);

 switch(func->scope)
   {
   case LOCAL:           printf("[Local]\n"); break;
   case GLOBAL:          printf("[Global]\n"); break;
   case LOCAL+INLINED:   printf("[Local inline]\n"); break;
   case INLINED:         printf("[inline]\n"); break;
   default:              ;
   }

 if(option_xref&XREF_FUNC)
   {
    for(i=0;i<func->calls->n;i++)
       printf("   %s %s : %s\n",i?"            ":"Calls:      ",func->calls->s1[i],func->calls->s2[i]?func->calls->s2[i]:"?unknown?");

    for(i=0;i<func->called->n;i++)
       printf("   %s %s : %s\n",i?"            ":"Called from:",func->called->s1[i],func->called->s2[i]);

    for(i=0;i<func->used->n;i++)
      {
       if(func->used->s1[i][0]=='$')
          printf("   %s %s\n",i?"            ":"Used in:    ",func->used->s2[i]);
       else
          printf("   %s %s : %s\n",i?"            ":"Used in:    ",func->used->s1[i],func->used->s2[i]);
      }

    for(i=0;i<func->f_refs->n;i++)
       printf("   %s %s : %s\n",i?"            ":"Refs func:  ",func->f_refs->s1[i],func->f_refs->s2[i]?func->f_refs->s2[i]:"?unknown?");
   }

 if(option_xref&XREF_VAR)
    for(i=0;i<func->v_refs->n;i++)
       printf("   %s %s : %s\n",i?"            ":"Refs var:   ",func->v_refs->s1[i],func->v_refs->s2[i]?func->v_refs->s2[i]:"?unknown?");

 printf("\n");
}
