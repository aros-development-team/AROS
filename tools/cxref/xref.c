/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5e.

  Cross referencing of functions.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,98,99,2000,01,02 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

/*+ The names of the function cross reference files. +*/
#define XREF_FUNC_FILE   ".function"
#define XREF_FUNC_BACKUP ".function~"

/*+ The names of the variable cross reference files. +*/
#define XREF_VAR_FILE    ".variable"
#define XREF_VAR_BACKUP  ".variable~"

/*+ The names of the include cross reference files. +*/
#define XREF_INC_FILE    ".include"
#define XREF_INC_BACKUP  ".include~"

/*+ The names of the type cross reference files. +*/
#define XREF_TYPE_FILE   ".typedef"
#define XREF_TYPE_BACKUP ".typedef~"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define TYPE_MAX_LEN 256        /*+ The maximum type definition length +*/
#define FUNC_MAX_LEN 64         /*+ The maximum function name length. +*/
#if defined(PATH_MAX) && defined(NAME_MAX)
#define FILE_MAX_LEN (PATH_MAX+NAME_MAX) /*+ The maximum filename length. +*/
#elif defined(PATH_MAX)
#define FILE_MAX_LEN (PATH_MAX+256) /*+ The maximum filename length. +*/
#else
#define FILE_MAX_LEN 512        /*+ The maximum filename length. +*/
#endif

#include "memory.h"
#include "datatype.h"
#include "cxref.h"

/*+ The name of the directory for the output. +*/
extern char* option_odir;

/*+ The base name of the file for the output. +*/
extern char* option_name;

/*+ The option for cross referencing. +*/
extern int option_xref;

/*+ The option for indexing. +*/
extern int option_index;

static void check_for_called(File file,char* called,char* caller,char* filename);
static void check_for_caller(File file,char* called,char* filename);
static void check_for_var(File file,char* variable,char* filename,int scope,char* funcname);
static int  check_for_var_func(File file,Variable var,Function func);
static void fixup_extern_var(Variable var,StringList2 refs);

/*++++++++++++++++++++++++++++++++++++++
  Cross reference the functions, variables and includes that are used in this file
  with the global functions, variables and includes. The types that are defined are also listed here.

  File file The file structure containing the information.

  int outputs Set to true if any cross referencing to produce outputs is required.
  ++++++++++++++++++++++++++++++++++++++*/

void CrossReference(File file,int outputs)
{
 FILE *in,*out;
 char *ifile,*ofile;

 /* Format: filename [[%]include1] [[%]include2] ... : Files include1, include2, ... are included in filename;
    those with a % are local. */

 if(option_xref&XREF_FILE) /* First do the files */
   {
    Include inc;

    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_INC_FILE);
    ofile=ConcatStrings(4,option_odir,"/",option_name,XREF_INC_BACKUP);

    in =fopen(ifile,"r");
    out=fopen(ofile,"w");

    if(!out)
      {fprintf(stderr,"cxref: Failed to open the include cross reference file '%s'\n",ofile);exit(1);}

    fprintf(out,"%s",file->name);
    for(inc=file->includes;inc;inc=inc->next)
       fprintf(out," %s%s",inc->scope==LOCAL?"%":"",inc->name);
    fprintf(out,"\n");

    if(in)
      {
       char include[FILE_MAX_LEN+1],filename[FILE_MAX_LEN+1],ch;

       while(fscanf(in,"%s%c",filename,&ch)==2)
         {
          int diff_file=strcmp(filename,file->name);

          if(diff_file)
             fprintf(out,"%s",filename);

          while(ch==' ')
            {
             fscanf(in,"%s%c",include,&ch);

             if(diff_file)
                fprintf(out," %s",include);

             if(outputs)
                if(include[0]=='%' && !strcmp(&include[1],file->name))
                   AddToStringList(file->inc_in,filename,1,1);
            }

          if(diff_file)
             fprintf(out,"\n");
         }

       fclose(in);
       unlink(ifile);
      }

    fclose(out);
    rename(ofile,ifile);
   }

 /* Format: filename funcname scope [[%][&]funcname1] [[%][&]funcname2] ... : The function funcname in file filename
    calls or references functions funcname1, funcname2 ... ; those with a % are local, with a & are references. */
 /* Format: filename $ 0 [[%]&funcname1] [[%]&funcname2] ... : The file references functions funcname1, funcname2 ... ;
    those with a % are local.  */

 if(option_xref&XREF_FUNC) /* Now do the functions */
   {
    Function func;
    int i;

    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_FUNC_FILE);
    ofile=ConcatStrings(4,option_odir,"/",option_name,XREF_FUNC_BACKUP);

    in =fopen(ifile,"r");
    out=fopen(ofile,"w");

    if(!out)
      {fprintf(stderr,"cxref: Failed to open the functional cross reference file '%s'\n",ofile);exit(1);}

    for(i=0;i<file->f_refs->n;i++)
       check_for_called(file,ConcatStrings(2,"&",file->f_refs->s1[i]),NULL,file->name);

    for(func=file->functions;func;func=func->next)
      {
       for(i=0;i<func->calls->n;i++)
          check_for_called(file,func->calls->s1[i],func->name,file->name);
       for(i=0;i<func->f_refs->n;i++)
          check_for_called(file,ConcatStrings(2,"&",func->f_refs->s1[i]),func->name,file->name);
      }

    for(func=file->functions;func;func=func->next)
       check_for_caller(file,func->name,file->name);

    if(file->f_refs->n)
      {
       fprintf(out,"%s $ 0",file->name);
       for(i=0;i<file->f_refs->n;i++)
         {
          if(file->f_refs->s2[i])
             fprintf(out," %%&%s",file->f_refs->s1[i]);
          else
             fprintf(out," &%s",file->f_refs->s1[i]);
         }
       fprintf(out,"\n");
      }

    for(func=file->functions;func;func=func->next)
      {
       fprintf(out,"%s %s %d",file->name,func->name,func->scope);
       for(i=0;i<func->calls->n;i++)
         {
          if(func->calls->s2[i])
             fprintf(out," %%%s",func->calls->s1[i]);
          else
             fprintf(out," %s",func->calls->s1[i]);
         }
       for(i=0;i<func->f_refs->n;i++)
         {
          if(func->f_refs->s2[i])
             fprintf(out," %%&%s",func->f_refs->s1[i]);
          else
             fprintf(out," &%s",func->f_refs->s1[i]);
         }
       fprintf(out,"\n");
      }

    if(in)
      {
       char ch,funcname[FUNC_MAX_LEN+1],filename[FILE_MAX_LEN+1],called[FUNC_MAX_LEN+1];
       int scope;

       while(fscanf(in,"%s %s %d%c",filename,funcname,&scope,&ch)==4)
         {
          int diff_file=strcmp(filename,file->name);

          if(diff_file)
            {
             if(outputs)
                if(funcname[0]!='$' || funcname[1]!=0)
                   check_for_caller(file,funcname,filename);
             fprintf(out,"%s %s %d",filename,funcname,scope);
            }

          while(ch==' ')
            {
             fscanf(in,"%s%c",called,&ch);

             if(diff_file)
               {
                if(outputs)
                  {
                   if(called[0]!='%')
                     {
                      if(funcname[0]!='$' || funcname[1]!=0)
                         check_for_called(file,called,funcname,filename);
                      else
                         check_for_called(file,called,NULL,filename);
                     }
                  }
                fprintf(out," %s",called);
               }
            }

          if(diff_file)
             fprintf(out,"\n");
         }

       fclose(in);
       unlink(ifile);
      }

    fclose(out);
    rename(ofile,ifile);
   }

 /* Format: filename varname scope [$] [[%]funcname1] [[%]funcname2] ... : variable varname is used in
    the file filename if $, and functions funcname1, funcname2 ... Those with a % are local.  */

 if(option_xref&XREF_VAR) /* Now do the variables */
   {
    Variable var;
    Function func;

    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_VAR_FILE);
    ofile=ConcatStrings(4,option_odir,"/",option_name,XREF_VAR_BACKUP);

    in =fopen(ifile,"r");
    out=fopen(ofile,"w");

    if(!out)
      {fprintf(stderr,"cxref: Failed to open the variable cross reference file '%s'\n",ofile);exit(1);}

    for(var=file->variables;var;var=var->next)
      {
       check_for_var(file,var->name,file->name,var->scope,NULL);
       fprintf(out,"%s %s %d",file->name,var->name,var->scope);
       if(check_for_var_func(file,var,NULL))
          fprintf(out," $");
       for(func=file->functions;func;func=func->next)
          if(check_for_var_func(file,var,func))
             fprintf(out," %s%s",func->scope==LOCAL?"%":"",func->name);
       fprintf(out,"\n");
      }

    if(in)
      {
       char varname[FUNC_MAX_LEN+1],filename[FILE_MAX_LEN+1],funcname[FUNC_MAX_LEN+1],ch;
       int scope;

       while(fscanf(in,"%s %s %d%c",filename,varname,&scope,&ch)==4)
         {
          int diff_file=strcmp(filename,file->name);

          if(diff_file)
            {
             if(outputs)
                if(!(scope&LOCAL))
                   check_for_var(file,varname,filename,scope,NULL);
             fprintf(out,"%s %s %d",filename,varname,scope);
            }

          while(ch==' ')
            {
             fscanf(in,"%s%c",funcname,&ch);

             if(diff_file)
               {
                if(outputs)
                  {
                   if(!(scope&LOCAL))
                     {
                      if(funcname[0]=='%')
                         check_for_var(file,varname,filename,scope,&funcname[1]);
                      else
                         check_for_var(file,varname,filename,scope,funcname);
                     }
                  }
                fprintf(out," %s",funcname);
               }
            }

          if(diff_file)
             fprintf(out,"\n");
         }

       fclose(in);
       unlink(ifile);
      }

    /* We must fix the location of the extern variables now since it was not known earlier. */

    if(outputs)
      {
       fixup_extern_var(file->variables,file->v_refs);
       for(func=file->functions;func;func=func->next)
          fixup_extern_var(file->variables,func->v_refs);
      }

    fclose(out);
    rename(ofile,ifile);
   }

 /* Format: filename typename type... : For a typedef type.     */
 /* Format: filename #        type... : For a non typedef type. */

 if(option_xref&XREF_TYPE) /* Now do the types */
   {
    Typedef type;

    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_TYPE_FILE);
    ofile=ConcatStrings(4,option_odir,"/",option_name,XREF_TYPE_BACKUP);

    in =fopen(ifile,"r");
    out=fopen(ofile,"w");

    if(!out)
      {fprintf(stderr,"cxref: Failed to open the typedef reference file '%s'\n",ofile);exit(1);}

    for(type=file->typedefs;type;type=type->next)
       if(type->type)
          fprintf(out,"%s %s %s\n",file->name,type->name,type->type);
       else
          fprintf(out,"%s # %s\n",file->name,type->name);

    if(in)
      {
       char typename[TYPE_MAX_LEN+1],filename[FILE_MAX_LEN+1];

       while(fscanf(in,"%s %s",filename,typename)==2)
         {
          int diff_file=strcmp(filename,file->name);

          if(diff_file)
             fprintf(out,"%s %s",filename,typename);

          fgets(typename,TYPE_MAX_LEN,in);

          if(diff_file)
             fputs(typename,out);
         }

       fclose(in);
       unlink(ifile);
      }

    fclose(out);
    rename(ofile,ifile);
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Check through all of the functions in this file to see if any of them are called or referenced.

  File file The file structure.

  char* called The function that is called.

  char* caller The function that the called function is called from.

  char* filename The file that the function is called from.
  ++++++++++++++++++++++++++++++++++++++*/

static void check_for_called(File file,char* called,char* caller,char* filename)
{
 Function func;

 /* Check for function calls */

 if(called[0]!='&')
    for(func=file->functions;func;func=func->next)
      {
       if(!strcmp(called,func->name))
          AddToStringList2(func->called,caller,filename,1,1);
      }

 /* Check for function references */

 else
    for(func=file->functions;func;func=func->next)
      {
       if(!strcmp(&called[1],func->name))
         {
          if(caller)
             AddToStringList2(func->used,caller,filename,1,1);
          else
             AddToStringList2(func->used,"$",filename,1,0);
         }
      }
}


/*++++++++++++++++++++++++++++++++++++++
  Check through all of the functions in this file to see if any of them are callers or referencers.

  File file The file structure.

  char* called The function that is called.

  char* filename The file that the called function is in.
  ++++++++++++++++++++++++++++++++++++++*/

static void check_for_caller(File file,char* called,char* filename)
{
 int i;
 Function func;

 /* Check the functions that are called. */

 for(func=file->functions;func;func=func->next)
    for(i=0;i<func->calls->n;i++)
       if(!strcmp(called,func->calls->s1[i]))
          if(!func->calls->s2[i])
             func->calls->s2[i]=MallocString(filename);

 /* Check the functions that are referenced. */

 for(i=0;i<file->f_refs->n;i++)
    if(!strcmp(called,file->f_refs->s1[i]))
       if(!file->f_refs->s2[i])
          file->f_refs->s2[i]=MallocString(filename);

 for(func=file->functions;func;func=func->next)
    for(i=0;i<func->f_refs->n;i++)
       if(!strcmp(called,func->f_refs->s1[i]))
          if(!func->f_refs->s2[i])
             func->f_refs->s2[i]=MallocString(filename);
}


/*++++++++++++++++++++++++++++++++++++++
  Check through all of the variables in this file to see if any of them are extern usage of others.

  File file The file structure.

  char* variable The global variable name.

  char* filename The file that the variable is used in.

  int scope The scope of the variable in the foreign file.

  char* funcname The name of a function that uses the variable.
  ++++++++++++++++++++++++++++++++++++++*/

static void check_for_var(File file,char* variable,char* filename,int scope,char* funcname)
{
 Variable var;

 if(!funcname)
   {
    if(!(scope&(GLOBAL|EXTERNAL|EXTERN_H)))
       return;

    for(var=file->variables;var;var=var->next)
       if((scope&GLOBAL && var->scope&(EXTERNAL|EXTERN_H|EXTERN_F)) ||
          (scope&(GLOBAL|EXTERNAL|EXTERN_H) && var->scope&GLOBAL))
          if(!strcmp(variable,var->name))
            {
             if(scope&GLOBAL && var->scope&(EXTERNAL|EXTERN_H|EXTERN_F))
                var->defined=MallocString(filename);

             if(scope&(GLOBAL|EXTERNAL|EXTERN_H) && var->scope&GLOBAL)
                AddToStringList2(var->visible,"$",filename,1,0);
            }
   }
 else
   {
    for(var=file->variables;var;var=var->next)
       if(!strcmp(variable,var->name))
         {
          if(funcname[0]=='$' && !funcname[1])
             AddToStringList2(var->used,"$",filename,1,0);
          else
            {
             AddToStringList2(var->used,funcname,filename,1,1);

             if(scope&EXTERN_F && var->scope&GLOBAL)
                AddToStringList2(var->visible,funcname,filename,1,1);
            }
         }
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Check through the function to see if it uses the variable, if func is NULL then check the file.

  int check_for_var_func Returns 1 if the variable is referenced from the function or file.

  File file The file that the function belongs to.

  Variable var The variable that may be referenced.

  Function func The function that is to be checked.
  ++++++++++++++++++++++++++++++++++++++*/

static int check_for_var_func(File file,Variable var,Function func)
{
 int i;

 if(func)
   {
    for(i=0;i<func->v_refs->n;i++)
       if(!strcmp(var->name,func->v_refs->s1[i]))
         {
          AddToStringList2(var->used,func->name,file->name,1,1);
          if(var->scope&(GLOBAL|LOCAL))
             func->v_refs->s2[i]=MallocString(file->name);
          else
            {
             if(func->v_refs->s2[i]) Free(func->v_refs->s2[i]);
             func->v_refs->s2[i]=MallocString("$");
            }
          return(1);
         }
   }
 else
   {
    for(i=0;i<file->v_refs->n;i++)
       if(!strcmp(var->name,file->v_refs->s1[i]))
         {
          AddToStringList2(var->used,"$",file->name,1,0);
          if(var->scope&(GLOBAL|LOCAL))
             file->v_refs->s2[i]=MallocString(file->name);
          else
            {
             if(file->v_refs->s2[i]) Free(file->v_refs->s2[i]);
             file->v_refs->s2[i]=MallocString("$");
            }
          return(1);
         }
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  We can only now put in the location of the external variables that we found were used.
  Previously we did not know the location of their global definition.

  Variable var The list of variables for this file.

  StringList2 refs A list of variable references from a file or a function.
  ++++++++++++++++++++++++++++++++++++++*/

static void fixup_extern_var(Variable var,StringList2 refs)
{
 int i;
 Variable v;

 for(i=0;i<refs->n;i++)
   {
    if(refs->s2[i][0]=='$' && !refs->s2[i][1])
       for(v=var;v;v=v->next)
          if(v->scope&(EXTERNAL|EXTERN_H|EXTERN_F) && !strcmp(refs->s1[i],v->name))
            {
             if(v->defined)
               {
                Free(refs->s2[i]);
                refs->s2[i]=MallocString(v->defined);
               }
             else
               {
                Free(refs->s1[i]);
                refs->s1[i]=MallocString(v->name);
                Free(refs->s2[i]);
                refs->s2[i]=NULL;
               }

             break;
            }
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Create the appendix of files, global functions, global variables and types.

  StringList files The list of files to create.

  StringList2 funcs The list of functions to create.

  StringList2 vars The list of variables to create.

  StringList2 types The list of types to create.
  ++++++++++++++++++++++++++++++++++++++*/

void CreateAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types)
{
 FILE *in;
 char *ifile;

 if(option_index&INDEX_FILE) /* First do the files */
   {
    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_INC_FILE);

    in =fopen(ifile,"r");

    if(in)
      {
       char include[FILE_MAX_LEN+1],filename[FILE_MAX_LEN+1],ch;

       while(fscanf(in,"%s%c",filename,&ch)==2)
         {
          AddToStringList(files,filename,1,1);
          while(ch==' ')
             fscanf(in,"%s%c",include,&ch);
         }

       fclose(in);
      }
   }

 if(option_index&INDEX_FUNC) /* Now do the functions */
   {
    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_FUNC_FILE);

    in =fopen(ifile,"r");

    if(in)
      {
       char ch,caller[FUNC_MAX_LEN+1],filename[FILE_MAX_LEN+1],called[FUNC_MAX_LEN+1];
       int scope;

       while(fscanf(in,"%s %s %d%c",filename,caller,&scope,&ch)==4)
         {
          if(scope&GLOBAL)
             AddToStringList2(funcs,caller,filename,1,1);
          while(ch==' ')
             fscanf(in,"%s%c",called,&ch);
         }

       fclose(in);
      }
   }

 if(option_index&INDEX_VAR) /* Now do the variables */
   {
    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_VAR_FILE);

    in =fopen(ifile,"r");

    if(in)
      {
       char variable[FUNC_MAX_LEN+1],filename[FILE_MAX_LEN+1],funcname[FUNC_MAX_LEN+1],ch;
       int scope;

       while(fscanf(in,"%s %s %d%c",filename,variable,&scope,&ch)==4)
         {
          if(scope&GLOBAL)
             AddToStringList2(vars,variable,filename,1,1);
          while(ch==' ')
             fscanf(in,"%s%c",funcname,&ch);
         }

       fclose(in);
      }
   }

 if(option_index&INDEX_TYPE) /* Now do the types */
   {
    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_TYPE_FILE);

    in =fopen(ifile,"r");

    if(in)
      {
       char typename[TYPE_MAX_LEN],filename[FILE_MAX_LEN+1];

       while(fscanf(in,"%s %s",filename,typename)==2)
         {
          if(typename[0]=='#')
            {
             fgets(typename,TYPE_MAX_LEN,in);
             typename[strlen(typename)-1]=0;
             AddToStringList2(types,&typename[1],filename,1,1);
            }
          else
            {
             AddToStringList2(types,typename,filename,1,1);
             fgets(typename,TYPE_MAX_LEN,in);
            }
         }

       fclose(in);
      }
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the named file from the cross reference database.

  char *name The name of the file that is to be deleted.
  ++++++++++++++++++++++++++++++++++++++*/

void CrossReferenceDelete(char *name)
{
 FILE *in,*out;
 char *ifile,*ofile;

 /* First do the files */

 ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_INC_FILE);
 ofile=ConcatStrings(4,option_odir,"/",option_name,XREF_INC_BACKUP);

 in =fopen(ifile,"r");
 out=fopen(ofile,"w");

 if(in && !out)
   {fprintf(stderr,"cxref: Failed to open the include cross reference file '%s'\n",ofile);fclose(in);}
 else if(in)
   {
    char include[FILE_MAX_LEN+1],filename[FILE_MAX_LEN+1],ch;

    while(fscanf(in,"%s%c",filename,&ch)==2)
      {
       int diff_file=strcmp(filename,name);

       if(diff_file)
          fprintf(out,"%s",filename);

       while(ch==' ')
         {
          fscanf(in,"%s%c",include,&ch);

          if(diff_file)
             fprintf(out," %s",include);
         }

       if(diff_file)
          fprintf(out,"\n");
      }

    fclose(in);
    unlink(ifile);

    fclose(out);
    rename(ofile,ifile);
   }
 else if(out)
   {
    fclose(out);
    unlink(ofile);
   }

 /* Now do the functions */

 ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_FUNC_FILE);
 ofile=ConcatStrings(4,option_odir,"/",option_name,XREF_FUNC_BACKUP);

 in =fopen(ifile,"r");
 out=fopen(ofile,"w");

 if(in && !out)
   {fprintf(stderr,"cxref: Failed to open the functional cross reference file '%s'\n",ofile);fclose(in);}
 else if(in)
   {
    char ch,funcname[FUNC_MAX_LEN+1],filename[FILE_MAX_LEN+1],called[FUNC_MAX_LEN+1];
    int scope;

    while(fscanf(in,"%s %s %d%c",filename,funcname,&scope,&ch)==4)
      {
       int diff_file=strcmp(filename,name);

       if(diff_file)
          fprintf(out,"%s %s %d",filename,funcname,scope);

       while(ch==' ')
         {
          fscanf(in,"%s%c",called,&ch);
          if(diff_file)
             fprintf(out," %s",called);
         }

       if(diff_file)
          fprintf(out,"\n");
      }

    fclose(in);
    unlink(ifile);

    fclose(out);
    rename(ofile,ifile);
   }
 else if(out)
   {
    fclose(out);
    unlink(ofile);
   }

 /* Now do the variables */

 ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_VAR_FILE);
 ofile=ConcatStrings(4,option_odir,"/",option_name,XREF_VAR_BACKUP);

 in =fopen(ifile,"r");
 out=fopen(ofile,"w");

 if(in && !out)
   {fprintf(stderr,"cxref: Failed to open the variable cross reference file '%s'\n",ofile);fclose(in);}
 else if(in)
   {
    char varname[FUNC_MAX_LEN+1],filename[FILE_MAX_LEN+1],funcname[FUNC_MAX_LEN+1],ch;
    int scope;

    while(fscanf(in,"%s %s %d%c",filename,varname,&scope,&ch)==4)
      {
       int diff_file=strcmp(filename,name);

       if(diff_file)
          fprintf(out,"%s %s %d",filename,varname,scope);

       while(ch==' ')
         {
          fscanf(in,"%s%c",funcname,&ch);

          if(diff_file)
             fprintf(out," %s",funcname);
         }

       if(diff_file)
          fprintf(out,"\n");
      }

    fclose(in);
    unlink(ifile);

    fclose(out);
    rename(ofile,ifile);
   }
 else if(out)
   {
    fclose(out);
    unlink(ofile);
   }

 /* Now do the types */

 ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_TYPE_FILE);
 ofile=ConcatStrings(4,option_odir,"/",option_name,XREF_TYPE_BACKUP);

 in =fopen(ifile,"r");
 out=fopen(ofile,"w");

 if(in && !out)
   {fprintf(stderr,"cxref: Failed to open the typedef reference file '%s'\n",ofile);fclose(in);}
 else if(in)
   {
    char typename[TYPE_MAX_LEN+1],filename[FILE_MAX_LEN+1];

    while(fscanf(in,"%s %s",filename,typename)==2)
      {
       int diff_file=strcmp(filename,name);

       if(diff_file)
          fprintf(out,"%s %s",filename,typename);

       fgets(typename,TYPE_MAX_LEN,in);

       if(diff_file)
          fputs(typename,out);
      }

    fclose(in);
    unlink(ifile);

    fclose(out);
    rename(ofile,ifile);
   }
 else if(out)
   {
    fclose(out);
    unlink(ofile);
   }
}
