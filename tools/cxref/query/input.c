/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5b.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,99 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#include "../memory.h"
#include "../datatype.h"
#include "../cxref.h"
#include "query.h"

/*+ The names of the function cross reference files. +*/
#define XREF_FUNC_FILE   ".function"

/*+ The names of the variable cross reference files. +*/
#define XREF_VAR_FILE    ".variable"

/*+ The names of the include cross reference files. +*/
#define XREF_INC_FILE    ".include"

/*+ The names of the type cross reference files. +*/
#define XREF_TYPE_FILE   ".typedef"

/*+ The command line switch that sets the amount of cross referencing to do. +*/
extern int option_xref;

/*+ The command line switch for the output name, +*/
extern char *option_odir,       /*+ The directory to use. +*/
            *option_name;       /*+ The base part of the name. +*/

extern File *files;             /*+ The files that are queried. +*/
extern int n_files;             /*+ The number of files referenced. +*/

extern Function *functions;     /*+ The functions that are queried. +*/
extern int n_functions;         /*+ The number of functions referenced. +*/

extern Variable *variables;     /*+ The variables that are queried. +*/
extern int n_variables;         /*+ The number of variables referenced. +*/

extern Typedef *typedefs;       /*+ The type definitions that are queried. +*/
extern int n_typedefs;          /*+ The number of typedefs referenced. +*/

/* Local functions */

static void cross_reference_files(void);
static void cross_reference_functions(void);
static void cross_reference_variables(void);

/*++++++++++++++++++++++++++++++++++++++
  Read in all the information from the cross reference files.
  ++++++++++++++++++++++++++++++++++++++*/

void LoadInCrossRefs(void)
{
 FILE *in;
 char *ifile;

 /* Format: filename [[%]include1] [[%]include2] ... : Files include1, include2, ... are included in filename;
    those with a % are local. */

 /* First do the files */
   {
    char include[FILE_MAX_LEN],filename[FILE_MAX_LEN+1],ch;

    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_INC_FILE);

    in =fopen(ifile,"r");

    if(!in)
      {fprintf(stderr,"cxref-query: Failed to open the include cross reference file '%s'\n",ifile);exit(1);}

    while(fscanf(in,"%s%c",filename,&ch)==2)
      {
       if(n_files)
          files=(File*)Realloc(files,(n_files+1)*sizeof(File*));
       else
          files=(File*)Malloc(sizeof(File*));

       files[n_files]=(File)Calloc(1,sizeof(struct _File));
       files[n_files]->name=MallocString(filename);
       files[n_files]->inc_in=NewStringList();
       files[n_files]->f_refs=NewStringList2();
       files[n_files]->v_refs=NewStringList2();

       while(ch==' ')
         {
          Include inc=(Include)Calloc(1,sizeof(struct _Include));

          fscanf(in,"%s%c",include,&ch);

          if(include[0]=='%')
             {inc->scope=LOCAL;
              inc->name=MallocString(&include[1]);}
          else
             {inc->scope=GLOBAL;
              inc->name=MallocString(include);}

          AddToLinkedList(files[n_files]->includes,Include,inc);
         }
       n_files++;
      }

    cross_reference_files();

    fclose(in);
   }

 /* Format: filename funcname scope [[%][&]funcname1] [[%][&]funcname2] ... : The function funcname in file filename
    calls or references functions funcname1, funcname2 ... ; those with a % are local, with a & are references. */
 /* Format: filename $ 0 [[%]&funcname1] [[%]&funcname2] ... : The file references functions funcname1, funcname2 ... ;
    those with a % are local.  */

 /* Now do the functions */
   {
    char ch,funcname[FUNC_MAX_LEN+1],filename[FILE_MAX_LEN+1],called[FUNC_MAX_LEN+1];
    int scope;

    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_FUNC_FILE);

    in =fopen(ifile,"r");

    if(!in)
      {fprintf(stderr,"cxref-query: Failed to open the functional cross reference file '%s'\n",ifile);exit(1);}

    while(fscanf(in,"%s %s %d%c",filename,funcname,&scope,&ch)==4)
      {
       int i;
       StringList2 f_refs;

       for(i=0;i<n_files;i++)
          if(!strcmp(files[i]->name,filename))
             break;

       if(funcname[0]=='$')
          f_refs=files[i]->f_refs;
       else
         {
          if(n_functions)
             functions=(Function*)Realloc(functions,(n_functions+1)*sizeof(Function*));
          else
             functions=(Function*)Malloc(sizeof(Function*));

          functions[n_functions]=(Function)Calloc(1,sizeof(struct _Function));

          AddToLinkedList(files[i]->functions,Function,functions[n_functions]);

          functions[n_functions]->comment=MallocString(filename); /* Use comment field for filename */
          functions[n_functions]->name=MallocString(funcname);
          functions[n_functions]->scope=scope;
          functions[n_functions]->used=NewStringList2();
          functions[n_functions]->calls=NewStringList2();
          functions[n_functions]->called=NewStringList2();
          functions[n_functions]->f_refs=NewStringList2();
          functions[n_functions]->v_refs=NewStringList2();

          f_refs=functions[n_functions]->f_refs;
         }

       while(ch==' ')
         {
          char* c;
          fscanf(in,"%s%c",called,&ch);

          c=called;
          if(c[0]=='%') c++;
          if(c[0]=='&')
            {
             if(c==called)
                AddToStringList2(f_refs,c+1,NULL,1,1);
             else
                AddToStringList2(f_refs,c+1,filename,1,1);
            }
          else
            {
             if(c==called)
                AddToStringList2(functions[n_functions]->calls,c,NULL,1,1);
             else
                AddToStringList2(functions[n_functions]->calls,c,filename,1,1);
            }
         }

       if(funcname[0]!='$')
          n_functions++;
      }

    cross_reference_functions();

    fclose(in);
   }

 /* Format: filename varname scope [$] [[%]funcname1] [[%]funcname2] ... : variable varname is used in
    the file filename if $, and functions funcname1, funcname2 ... Those with a % are local.  */

 /* Now do the variables */
   {
    char varname[FUNC_MAX_LEN+1],filename[FILE_MAX_LEN+1],funcname[FUNC_MAX_LEN+1],ch;
    int scope;

    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_VAR_FILE);

    in =fopen(ifile,"r");

    if(!in)
      {fprintf(stderr,"cxref-query: Failed to open the variable cross reference file '%s'\n",ifile);exit(1);}

    while(fscanf(in,"%s %s %d%c",filename,varname,&scope,&ch)==4)
      {
       int i;

       if(n_variables)
          variables=(Variable*)Realloc(variables,(n_variables+1)*sizeof(Variable*));
       else
          variables=(Variable*)Malloc(sizeof(Variable*));

       variables[n_variables]=(Variable)Calloc(1,sizeof(struct _Variable));

       for(i=0;i<n_files;i++)
          if(!strcmp(files[i]->name,filename))
             AddToLinkedList(files[i]->variables,Variable,variables[n_variables]);

       variables[n_variables]->comment=MallocString(filename); /* Use comment field for filename */
       variables[n_variables]->name=MallocString(varname);
       variables[n_variables]->visible=NewStringList2();
       variables[n_variables]->used=NewStringList2();
       variables[n_variables]->scope=scope;

       while(ch==' ')
         {
          fscanf(in,"%s%c",funcname,&ch);

          if(funcname[0]=='$')
             AddToStringList2(variables[n_variables]->used,"$",filename,1,0);
          else
             if(funcname[0]=='%')
                AddToStringList2(variables[n_variables]->used,&funcname[1],filename,1,1);
             else
                AddToStringList2(variables[n_variables]->used,funcname,NULL,1,1);
         }
       n_variables++;
      }

    cross_reference_variables();

    fclose(in);
   }

 /* Format: filename typename type... : For a typedef type.     */
 /* Format: filename #        type... : For a non typedef type. */

 /* Now do the types */
   {
    char typename[FILE_MAX_LEN+1],filename[FILE_MAX_LEN+1],typetype[TYPE_MAX_LEN+1];

    ifile=ConcatStrings(4,option_odir,"/",option_name,XREF_TYPE_FILE);

    in =fopen(ifile,"r");

    if(!in)
      {fprintf(stderr,"cxref-query: Failed to open the typedef reference file '%s'\n",ifile);exit(1);}

    while(fscanf(in,"%s %s",filename,typename)==2)
      {
       int i;

       fgets(typetype,TYPE_MAX_LEN,in);
       typetype[strlen(typetype)-1]=0;

       if(n_typedefs)
          typedefs=(Typedef*)Realloc(typedefs,(n_typedefs+1)*sizeof(Typedef*));
       else
          typedefs=(Typedef*)Malloc(sizeof(Typedef*));

       typedefs[n_typedefs]=(Typedef)Calloc(1,sizeof(struct _Typedef));

       for(i=0;i<n_files;i++)
          if(!strcmp(files[i]->name,filename))
             AddToLinkedList(files[i]->typedefs,Typedef,typedefs[n_typedefs]);

       typedefs[n_typedefs]->comment=MallocString(filename); /* Use comment field for filename */

       if(typename[0]!='#')
         {
          typedefs[n_typedefs]->name=MallocString(typename);
          typedefs[n_typedefs]->type=MallocString(&typetype[1]);
         }
       else
         {
          typedefs[n_typedefs]->name=MallocString(&typetype[1]);
          typedefs[n_typedefs]->type=NULL;
         }

       n_typedefs++;
      }

    fclose(in);
   }

}


/*++++++++++++++++++++++++++++++++++++++
  Performs all of the cross referencing between files, includes and included in.
  ++++++++++++++++++++++++++++++++++++++*/

static void cross_reference_files(void)
{
 int i;

 for(i=0;i<n_files;i++)
   {
    int j;
    Include inc=files[i]->includes;

    while(inc)
      {
       for(j=0;j<n_files;j++)
          if(!strcmp(inc->name,files[j]->name))
            {
             inc->includes=files[j]->includes;
             AddToStringList(files[j]->inc_in,files[i]->name,1,1);
            }
       inc=inc->next;
      }
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Performs all of the cross referencing between global functions and functions that they call.
  ++++++++++++++++++++++++++++++++++++++*/

static void cross_reference_functions(void)
{
 int i1,j1,i2;

 for(i1=0;i1<n_functions;i1++)
   {
    Function func1=functions[i1];

    for(j1=0;j1<func1->calls->n;j1++)
      {
       if(!func1->calls->s2[j1])
          for(i2=0;i2<n_functions;i2++)
            {
             Function func2=functions[i2];

             if(!strcmp(func1->calls->s1[j1],func2->name))
               {
                func1->calls->s2[j1]=MallocString(func2->comment);
                break;
               }
            }

       if(func1->calls->s2[j1])
          for(i2=0;i2<n_functions;i2++)
            {
             Function func2=functions[i2];

             if(!strcmp(func1->calls->s1[j1],func2->name) && !strcmp(func1->calls->s2[j1],func2->comment))
               {
                AddToStringList2(func2->called,func1->name,func1->comment,1,1);
                break;
               }
            }
      }

    for(j1=0;j1<func1->f_refs->n;j1++)
      {
       if(!func1->f_refs->s2[j1])
          for(i2=0;i2<n_functions;i2++)
            {
             Function func2=functions[i2];

             if(!strcmp(func1->f_refs->s1[j1],func2->name))
               {
                func1->f_refs->s2[j1]=MallocString(func2->comment);
                break;
               }
            }

       if(func1->f_refs->s2[j1])
          for(i2=0;i2<n_functions;i2++)
            {
             Function func2=functions[i2];

             if(!strcmp(func1->f_refs->s1[j1],func2->name) && !strcmp(func1->f_refs->s2[j1],func2->comment))
               {
                AddToStringList2(func2->used,func1->name,func1->comment,1,1);
                break;
               }
            }
      }
   }

 for(i1=0;i1<n_files;i1++)
   {
    File file1=files[i1];

    for(j1=0;j1<file1->f_refs->n;j1++)
      {
       if(!file1->f_refs->s2[j1])
          for(i2=0;i2<n_functions;i2++)
            {
             Function func2=functions[i2];

             if(!strcmp(file1->f_refs->s1[j1],func2->name))
               {
                file1->f_refs->s2[j1]=MallocString(func2->comment);
                break;
               }
            }

       if(file1->f_refs->s2[j1])
          for(i2=0;i2<n_functions;i2++)
            {
             Function func2=functions[i2];

             if(!strcmp(file1->f_refs->s1[j1],func2->name) && !strcmp(file1->f_refs->s2[j1],func2->comment))
               {
                AddToStringList2(func2->used,"$",file1->name,1,1);
                break;
               }
            }
      }
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Performs all of the cross referencing between global variables and functions that use them.
  ++++++++++++++++++++++++++++++++++++++*/

static void cross_reference_variables(void)
{
 int i1,j1,i2;

 for(i1=0;i1<n_variables;i1++)
   {
    Variable var1=variables[i1];

    for(j1=0;j1<var1->used->n;j1++)
      {
       if(var1->used->s1[j1][0]!='$' && !var1->used->s2[j1])
          for(i2=0;i2<n_functions;i2++)
            {
             Function func2=functions[i2];

             if(!strcmp(var1->used->s1[j1],func2->name))
               {
                var1->used->s2[j1]=MallocString(func2->comment);
                break;
               }
            }

       if(var1->used->s1[j1][0]=='$')
          for(i2=0;i2<n_files;i2++)
            {
             File file2=files[i2];

             if(!strcmp(var1->used->s2[j1],file2->name))
               {
                AddToStringList2(file2->v_refs,var1->name,var1->comment,1,1);
                break;
               }
            }
       else if(var1->used->s2[j1])
          for(i2=0;i2<n_functions;i2++)
            {
             Function func2=functions[i2];

             if(!strcmp(var1->used->s1[j1],func2->name) && !strcmp(var1->used->s2[j1],func2->comment))
               {
                AddToStringList2(func2->v_refs,var1->name,var1->comment,1,1);
                break;
               }
            }
      }
   }
}
