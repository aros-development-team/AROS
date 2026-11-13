/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5f.

  Collects the pre-processing instruction stuff.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,99,2000,01,02,03,04 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

/*+ Control the output of debugging information for this file. +*/
#define DEBUG 0 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <limits.h>
#include <sys/stat.h>

#include "memory.h"
#include "datatype.h"
#include "parse-yy.h"
#include "cxref.h"

/*+ The file that is currently being processed. +*/
extern File CurFile;

/*+ The name of the include directories specified on the command line. +*/
extern char **option_incdirs;

/*+ The number of include directories on the command line. +*/
extern int option_nincdirs;

/*+ When in a header file, this is set to 1, to allow most of the stuff to be skipped. +*/
int in_header=0;

/*+ The current #include we are looking at. +*/
static Include cur_inc=NULL;

/*+ The current #define we are looking at. +*/
static Define cur_def=NULL;

/*+ The depth of includes. +*/
static int inc_depth=0;

/*+ The type of include at this depth. +*/
static char *inc_type=NULL;

/*+ The name of the include file at this depth. +*/
static char **inc_name=NULL;

/*+ The working directory. +*/
static char *cwd=NULL;


static Include NewIncludeType(char *name);
static Define NewDefineType(char *name);


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when an included file is seen in the current file.

  char *name The name of the file from the source code.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenInclude(char *name)
{
#if DEBUG
 printf("#Preproc.c# #include %s\n",name);
#endif

 if(!inc_type || inc_depth==0 || (inc_depth > 0 && inc_type[inc_depth-1]==LOCAL))
   {
    Include inc,*t=&CurFile->includes;
    int inc_scope=(*name=='"')?LOCAL:GLOBAL;
    int i;

    name++;
    name[strlen(name)-1]=0;

    if(inc_scope==LOCAL && option_nincdirs)
       for(i=0;i<option_nincdirs;i++)
         {
          char *newname=CanonicaliseName(ConcatStrings(3,option_incdirs[i],"/",name));
          struct stat buf;

          if(!lstat(newname,&buf))
            {name=newname;break;}
         }

    for(i=0;i<inc_depth;i++)
      {
       while(*t && (*t)->next)
          t=&(*t)->next;
       t=&(*t)->includes;
      }

    inc=NewIncludeType(name);

    inc->comment=MallocString(GetCurrentComment());
    inc->scope=inc_scope;

    AddToLinkedList(*t,Include,inc);

    cur_inc=inc;
   }
 else
    cur_inc=NULL;
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a comment is seen following a #include.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenIncludeComment(void)
{
 char* comment=GetCurrentComment();

#if DEBUG
 printf("#Preproc.c# #include trailing comment '%s' for %s\n",comment,cur_inc->name);
#endif

 if(!cur_inc->comment)
    cur_inc->comment=MallocString(comment);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a change in current file is seen.

  char *SeenFileChange Returns the filename that we are now in.

  char *name The pathname of the included file as determined by gcc.

  int flag The flags that GCC leaves in the file
  ++++++++++++++++++++++++++++++++++++++*/

char *SeenFileChange(char *name,int flag)
{
 if(!cwd)
   {
    cwd=(char*)Malloc(PATH_MAX+1);
    if(!getcwd(cwd,PATH_MAX))
       cwd[0]=0;
   }

 /* Special gcc-3.x fake names for built-in #defines. */

/* jmj: the fake names differ from locale to locale, but are always bracketed like this */
/* old: if(!strcmp(name,"<built-in>") || !strcmp(name,"<command line>"))*/
 if (*name == '<' && name[strlen(name) - 1] == '>')
   {
    in_header=1;
    return(NULL);
   }
 else if(flag==-1)
   {
    in_header=0;
    return(CurFile->name);
   }

 name=CanonicaliseName(name);

 if(!strncmp(name,cwd,strlen(cwd)))
    name=name+strlen(cwd);

 if(flag&4)
   {
    if(inc_depth>=2)
       name=inc_name[inc_depth-2];
    else
       name=CurFile->name;
   }

#if DEBUG
 printf("#Preproc.c# FileChange - %s %s (flag=%d)\n",flag&2?"Included ":"Return to",name,flag);
#endif

 /* Store the information. */

 if(flag&2 && (!inc_type || inc_depth==0 || (inc_depth > 0 && inc_type[inc_depth-1]==LOCAL)))
   {
    if(!cur_inc)
      {
       if(flag&8)
          SeenInclude(ConcatStrings(3,"<",name,">"));
       else
          SeenInclude(ConcatStrings(3,"\"",name,"\""));
      }
    else if(!(flag&8))
      {
       Free(cur_inc->name);
       cur_inc->name=MallocString(name);
      }
   }

 if(flag&2)
   {
    inc_depth++;

    if(!inc_type)
      {
       inc_type=(char*)Malloc(16);
       inc_name=(char**)Malloc(16*sizeof(char*));
      }
    else
       if(!(inc_depth%16))
         {
          inc_type=(char*)Realloc(inc_type,(unsigned)(inc_depth+16));
          inc_name=(char**)Realloc(inc_name,(unsigned)(sizeof(char*)*(inc_depth+16)));
         }

    if(inc_depth>1 && inc_type[inc_depth-2]==GLOBAL)
       inc_type[inc_depth-1]=GLOBAL;
    else if (inc_depth > 0)
       inc_type[inc_depth-1]=cur_inc?cur_inc->scope:(flag&8)?GLOBAL:LOCAL;

    inc_name[inc_depth-1]=CopyString(name);
   }
 else
    inc_depth--;

 if(inc_type && inc_depth>0)
    in_header=inc_type[inc_depth-1];
 else
    in_header=0;

 SetCurrentComment(NULL);

 cur_inc=NULL;

 return(name);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a #define is seen in the current file.

  char* name The name of the #defined symbol.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenDefine(char* name)
{
 Define def;

#if DEBUG
 printf("#Preproc.c# Defined name '%s'\n",name);
#endif

 def=NewDefineType(name);

 def->comment=MallocString(GetCurrentComment());

 def->lineno=parse_line;

 AddToLinkedList(CurFile->defines,Define,def); 

 cur_def=def;
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a comment is seen in a #define definition.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenDefineComment(void)
{
 char* comment=GetCurrentComment();

#if DEBUG
 printf("#Preproc.c# #define inline comment '%s' in %s\n",comment,cur_def->name);
#endif

 if(!cur_def->comment)
    cur_def->comment=MallocString(comment);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a #define value is seen in the current file.

  char* value The value of the #defined symbol.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenDefineValue(char* value)
{
#if DEBUG
 printf("#Preproc.c# #define value '%s' for %s\n",value,cur_def->name);
#endif

 cur_def->value=MallocString(value);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a #define function argument is seen in the current definition.

  char* name The argument.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenDefineFunctionArg(char* name)
{
#if DEBUG
 printf("#Preproc.c# #define Function arg '%s' in %s()\n",name,cur_def->name);
#endif

 AddToStringList2(cur_def->args,name,SplitComment(&cur_def->comment,name),0,0);
}


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a comment is seen in a #define function definition.
  ++++++++++++++++++++++++++++++++++++++*/

void SeenDefineFuncArgComment(void)
{
 char* comment=GetCurrentComment();

#if DEBUG
 printf("#Preproc.c# #define Function arg comment '%s' in %s()\n",comment,cur_def->name);
#endif

 if(!cur_def->args->s2[cur_def->args->n-1])
    cur_def->args->s2[cur_def->args->n-1]=MallocString(comment);
}


/*++++++++++++++++++++++++++++++++++++++
  Tidy up all of the local variables in case of a problem and abnormal parser termination.
  ++++++++++++++++++++++++++++++++++++++*/

void ResetPreProcAnalyser(void)
{
 in_header=0;

 cur_inc=NULL;
 cur_def=NULL;

 inc_depth=0;

 if(inc_type) Free(inc_type);
 inc_type=NULL;
 if(inc_name) Free(inc_name);
 inc_name=NULL;

 if(cwd) Free(cwd);
 cwd=NULL;
}


/*++++++++++++++++++++++++++++++++++++++
  Create a new Include datatype.

  Include NewIncludeType Return the new Include type.

  char *name The name of the new include.
  ++++++++++++++++++++++++++++++++++++++*/

static Include NewIncludeType(char *name)
{
 Include inc=(Include)Calloc(1,sizeof(struct _Include));

 inc->name=MallocString(name);

 return(inc);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the specified Include type.

  Include inc The Include type to be deleted.
  ++++++++++++++++++++++++++++++++++++++*/

void DeleteIncludeType(Include inc)
{
 if(inc->comment) Free(inc->comment);
 if(inc->name)    Free(inc->name);
 if(inc->includes)
   {
    Include p=inc->includes;
    do{
       Include n=p->next;
       DeleteIncludeType(p);
       p=n;
      }
    while(p);
   }
 Free(inc);
}


/*++++++++++++++++++++++++++++++++++++++
  Create a new Define datatype.

  Define NewDefineType Return the new Define type.

  char *name The name of the new define.
  ++++++++++++++++++++++++++++++++++++++*/

static Define NewDefineType(char *name)
{
 Define def=(Define)Calloc(1,sizeof(struct _Define)); /* clear unused pointers */

 def->name=MallocString(name);
 def->args=NewStringList2();

 return(def);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the specified Define type.

  Define def The Define type to be deleted.
  ++++++++++++++++++++++++++++++++++++++*/

void DeleteDefineType(Define def)
{
 if(def->comment) Free(def->comment);
 if(def->name)    Free(def->name);
 if(def->value)   Free(def->value);
 if(def->args)    DeleteStringList2(def->args);
 Free(def);
}
