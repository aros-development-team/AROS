/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5f.

  Writes the RTF output.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,98,2001,04 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "memory.h"
#include "datatype.h"
#include "cxref.h"

/*+ The name of the output rtf file. +*/
#define RTF_FILE        ".rtf"
#define RTF_FILE_BACKUP ".rtf~"

/*+ The name of the output rtf file that contains the appendix. +*/
#define RTF_APDX        ".apdx"

#define STYLE_NORM "\\s0\\f0\\fs24"
#define STYLE_H1   "\\s1\\f0\\fs40\\b\\sb400\\sa200\\keepn\\keep"
#define STYLE_H2   "\\s2\\f0\\fs32\\b\\sb200\\sa100\\keepn\\keep"
#define STYLE_H3   "\\s3\\f0\\fs28\\b\\sb100\\sa100\\keepn\\keep"
#define STYLE_H4   "\\s4\\f0\\fs24\\b\\sb100\\sa50\\keepn\\keep"
#define STYLE_TT   "\\s5\\f1\\fs20\\ql\\sb50\\sa50"
#define STYLE_IND  "\\s6\\f0\\fs24\\ql\\li720"

/*+ The comments are to be inserted verbatim. +*/
extern int option_verbatim_comments;

/*+ The name of the directory for the output. +*/
extern char* option_odir;

/*+ The base name of the file for the output. +*/
extern char* option_name;

/*+ The information about the cxref run, +*/
extern char *run_command,       /*+ the command line options. +*/
            *run_cpp_command;   /*+ the cpp command and options. +*/

static void WriteRTFFilePart(File file);
static void WriteRTFInclude(Include inc);
static void WriteRTFSubInclude(Include inc,int depth);
static void WriteRTFDefine(Define def);
static void WriteRTFTypedef(Typedef type,char* filename);
static void WriteRTFStructUnion(StructUnion su,int depth);
static void WriteRTFVariable(Variable var,char* filename);
static void WriteRTFFunction(Function func,char* filename);
static void WriteRTFPreamble(FILE *f);
static void WriteRTFPostamble(FILE *f);

static char* rtf(char* c,int verbatim);

/*+ The output file for the RTF. +*/
static FILE* of;


/*++++++++++++++++++++++++++++++++++++++
  Write an RTF file for a complete File structure and all components.

  File file The File structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

void WriteRTFFile(File file)
{
 char* ofile;

 /* Open the file */

 ofile=ConcatStrings(4,option_odir,"/",file->name,RTF_FILE);

 of=fopen(ofile,"w");
 if(!of)
   {
    struct stat stat_buf;
    int i,ofl=strlen(ofile);

    for(i=strlen(option_odir)+1;i<ofl;i++)
       if(ofile[i]=='/')
         {
          ofile[i]=0;
          if(stat(ofile,&stat_buf))
             mkdir(ofile,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
          ofile[i]='/';
         }

    of=fopen(ofile,"w");
   }

 if(!of)
   {fprintf(stderr,"cxref: Failed to open the RTF output file '%s'\r\n",ofile);exit(1);}

 /* Write out a header. */

 WriteRTFPreamble(of);

 /*+ The file structure is broken into its components and they are each written out. +*/

 WriteRTFFilePart(file);

 if(file->includes)
   {
    Include inc =file->includes;
    fprintf(of,"{" STYLE_H2 " Included Files\\par}\r\n");
    do{
       WriteRTFInclude(inc);
      }
    while((inc=inc->next));
   }

 if(file->defines)
   {
    Define def =file->defines;
    fprintf(of,"{" STYLE_H2 " Preprocessor definitions\\par}\r\n");
    do{
       WriteRTFDefine(def);
      }
    while((def=def->next));
   }

 if(file->typedefs)
   {
    Typedef type=file->typedefs;
    fprintf(of,"{" STYLE_H2 " Type definitions\\par}\r\n");
    do{
       WriteRTFTypedef(type,file->name);
      }
    while((type=type->next));
   }

 if(file->variables)
   {
    int any_to_mention=0;
    Variable var=file->variables;

    do{
       if(var->scope&(GLOBAL|LOCAL|EXTERNAL|EXTERN_F))
          any_to_mention=1;
      }
    while((var=var->next));

    if(any_to_mention)
      {
       Variable var=file->variables;
       fprintf(of,"{" STYLE_H2 " Variables\\par}\r\n");
       do{
          if(var->scope&GLOBAL)
             WriteRTFVariable(var,file->name);
         }
       while((var=var->next));
       var=file->variables;
       do{
          if(var->scope&(EXTERNAL|EXTERN_F) && !(var->scope&GLOBAL))
            {
             fprintf(of,"{" STYLE_H3 " External Variables\\par}\r\n");
             WriteRTFVariable(var,file->name);
            }
         }
       while((var=var->next));
       var=file->variables;
       do{
          if(var->scope&LOCAL)
            {
             fprintf(of,"{" STYLE_H3 " Local Variables\\par}\r\n");
             WriteRTFVariable(var,file->name);
            }
         }
       while((var=var->next));
      }
   }

 if(file->functions)
   {
    Function func=file->functions;
    fprintf(of,"{" STYLE_H2 " Functions\\par}\r\n");
    do{
       if(func->scope&(GLOBAL|EXTERNAL))
          WriteRTFFunction(func,file->name);
      }
    while((func=func->next));
    func=file->functions;
    do{
       if(func->scope&LOCAL)
          WriteRTFFunction(func,file->name);
      }
    while((func=func->next));
   }

 /* Write out a trailer. */

 WriteRTFPostamble(of);

 fclose(of);

 /* Clear the memory in rtf() */

 rtf(NULL,0); rtf(NULL,0); rtf(NULL,0); rtf(NULL,0);
}


/*++++++++++++++++++++++++++++++++++++++
  Write a File structure out.

  File file The File to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFFilePart(File file)
{
 int i;

 fprintf(of,"{" STYLE_H1 " File %s\\par}\r\n",rtf(file->name,0));

 if(file->comment)
   {
    if(option_verbatim_comments)
       fprintf(of,"{" STYLE_TT "%s\\par}\r\n",rtf(file->comment,1));
    else
      {
       char *rcs1=strstr(file->comment,"$Header"),*rcs2=NULL;
       if(rcs1)
         {
          rcs2=strstr(&rcs1[1],"$");
          if(rcs2)
            {
             rcs2[0]=0;
             fprintf(of,"{\\b RCS %s}\\par\r\n",rtf(&rcs1[1],0));
             rcs2[0]='$';
            }
         }
       if(rcs2)
          fprintf(of,"%s\\par\r\n",rtf(&rcs2[2],0));
       else
          fprintf(of,"%s\\par\r\n",rtf(file->comment,0));
      }
   }

 if(file->inc_in->n)
   {
    int i;

    fprintf(of,"\\trowd\\trgaph120\\cellx1440\\cellx9000\r\n\\intbl\\plain\r\n");
    for(i=0;i<file->inc_in->n;i++)
      {
       if(i==0) fprintf(of,"Included in:");
       fprintf(of,"\\cell %s\\cell\\row\r\n",rtf(file->inc_in->s[i],0));
      }
    fprintf(of,"\\intbl0\r\n");
   }

 if(file->f_refs->n || file->v_refs->n)
   {
    fprintf(of,"\\trowd\\trgaph120\\cellx1440\\cellx5220\\cellx9000\r\n\\intbl\\plain\r\n");

    if(file->f_refs->n)
      {
       int others=0;

       fprintf(of,"Refs Func:");

       for(i=0;i<file->f_refs->n;i++)
          if(file->f_refs->s2[i])
             fprintf(of,"\\cell %s()\\cell %s\\cell\\row\r\n",rtf(file->f_refs->s1[i],0),rtf(file->f_refs->s2[i],0));
          else
             others++;

       if(others)
         {
          fprintf(of,"\\cell ");
          for(i=0;i<file->f_refs->n;i++)
             if(!file->f_refs->s2[i])
                fprintf(of,--others?"%s(), ":"%s()",rtf(file->f_refs->s1[i],0));
          fprintf(of,"\\cell\\cell\\row\r\n");
         }
      }

    if(file->v_refs->n)
      {
       int others=0;

       fprintf(of,"Refs Var:");

       for(i=0;i<file->v_refs->n;i++)
          if(file->v_refs->s2[i])
             fprintf(of,"\\cell %s\\cell %s\\cell\\row\r\n",rtf(file->v_refs->s1[i],0),rtf(file->v_refs->s2[i],0));
          else
             others++;

       if(others)
         {
          fprintf(of,"\\cell ");
          for(i=0;i<file->v_refs->n;i++)
             if(!file->v_refs->s2[i])
                fprintf(of,--others?" %s,":" %s",rtf(file->v_refs->s1[i],0));
          fprintf(of,"\\cell\\cell\\row\r\n");
         }
      }
    fprintf(of,"\\intbl0\r\n");
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Write an Include structure out.

  Include inc The Include structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFInclude(Include inc)
{
 if(inc->comment)
    fprintf(of,"%s\\par\r\n",rtf(inc->comment,0));

 if(inc->scope==LOCAL)
    fprintf(of,"{" STYLE_TT " #include \"%s\"\\par}\r\n",rtf(inc->name,0));
 else
    fprintf(of,"{" STYLE_TT " #include <%s>\\par}\r\n",rtf(inc->name,0));

 if(inc->includes)
    WriteRTFSubInclude(inc->includes,1);
}


/*++++++++++++++++++++++++++++++++++++++
  Write an Sub Include structure out. (An include structure that is included from another file.)

  Include inc The Include structure to output.

  int depth The depth of the include hierarchy.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFSubInclude(Include inc,int depth)
{
 int i;

 while(inc)
   {
    for(i=0;i<depth;i++)
       fprintf(of,"\t");

    if(inc->scope==LOCAL)
       fprintf(of,"{" STYLE_TT " #include \"%s\"\\par}\r\n",rtf(inc->name,0));
    else
       fprintf(of,"{" STYLE_TT " #include <%s>\\par}\r\n",rtf(inc->name,0));

    if(inc->includes)
       WriteRTFSubInclude(inc->includes,depth+1);

    inc=inc->next;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Define structure out.

  Define def The Define structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFDefine(Define def)
{
 int i;
 int pargs=0;

 if(def->comment)
    fprintf(of,"%s\\par\r\n",rtf(def->comment,0));

 fprintf(of,"{" STYLE_TT " #define %s",rtf(def->name,0));

 if(def->value)
    fprintf(of," %s",rtf(def->value,0));

 if(def->args->n)
   {
    fprintf(of,"( ");
    for(i=0;i<def->args->n;i++)
       fprintf(of,i?", %s":"%s",rtf(def->args->s1[i],0));
    fprintf(of," )");
   }
 fprintf(of,"\\par}\r\n");

 for(i=0;i<def->args->n;i++)
    if(def->args->s2[i])
       pargs=1;

 if(pargs)
   {
    for(i=0;i<def->args->n;i++)
       fprintf(of,"{" STYLE_TT "%s\\par}\r\n{" STYLE_IND " %s\\par}\r\n",rtf(def->args->s1[i],0),def->args->s2[i]?rtf(def->args->s2[i],0):"");
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Typedef structure out.

  Typedef type The Typedef structure to output.

  char* filename The name of the file that is being processed (required for the cross reference label).
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFTypedef(Typedef type,char* filename)
{
 if(type->type)
    fprintf(of,"{" STYLE_H3 " Typedef %s\\par}\r\n",rtf(type->name,0));
 else
    fprintf(of,"{" STYLE_H3 " Type %s\\par}\r\n",rtf(type->name,0));

 if(type->comment)
    fprintf(of,"%s\\par\r\n",rtf(type->comment,0));

 if(type->type)
    fprintf(of,"{" STYLE_TT " typedef %s\\par}\r\n",rtf(type->type,0));

 if(type->sutype)
   {
    fprintf(of,"\\trowd\\trgaph120\\cellx2880\\cellx9000\r\n\\intbl\\plain\r\n");
    WriteRTFStructUnion(type->sutype,0);
    fprintf(of,"\\intbl0\r\n");
   }
 else
    if(type->typexref)
      {
       if(type->typexref->type)
          fprintf(of,"See:\tTypedef %s\\par\r\n",rtf(type->typexref->name,0));
       else
          if(!strncmp("enum",type->typexref->name,4))
             fprintf(of,"See\tType %s\\par\r\n",rtf(type->typexref->name,0));
          else
             if(!strncmp("union",type->typexref->name,5))
                fprintf(of,"See:\tType %s\\par\r\n",rtf(type->typexref->name,0));
             else
                if(!strncmp("struct",type->typexref->name,6))
                   fprintf(of,"See:\tType %s\\par\r\n",rtf(type->typexref->name,0));
      }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a structure / union structure out.

  StructUnion su The structure / union to write.

  int depth The current depth within the structure.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFStructUnion(StructUnion su, int depth)
{
 int i;
 char* splitsu=NULL;

 splitsu=strstr(su->name,"{...}");
 if(splitsu) splitsu[-1]=0;

 for(i=0;i<depth;i++)
    fprintf(of,"\t");

 if(depth && su->comment && !su->comps)
    fprintf(of,"{" STYLE_TT " %s;}\\cell %s\\cell\\row\r\n",rtf(su->name,0),rtf(su->comment,0));
 else if(!depth || su->comps)
    fprintf(of,"{" STYLE_TT " %s}\\cell\\cell\\row\r\n",rtf(su->name,0));
 else
    fprintf(of,"{" STYLE_TT " %s;}\\cell\\cell\\row\r\n",rtf(su->name,0));

 if(!depth || su->comps)
   {
    for(i=0;i<depth;i++)
       fprintf(of,"\t");
    fprintf(of,"{" STYLE_TT " \\{}\\cell\\cell\\row\r\n");

    for(i=0;i<su->n_comp;i++)
       WriteRTFStructUnion(su->comps[i],depth+1);

    for(i=0;i<depth;i++)
       fprintf(of,"\t");
    fprintf(of,"{" STYLE_TT " \\}}\\cell\\cell\\row\r\n");
    if(splitsu)
      {
       for(i=0;i<depth;i++)
          fprintf(of,"\t");
       if(depth && su->comment)
          fprintf(of,"{" STYLE_TT " %s;}\\cell %s\\par\r\n",splitsu[5]?rtf(&splitsu[6],0):"",rtf(su->comment,0));
       else
          fprintf(of,"{" STYLE_TT " %s;}\\cell\\cell\\row\r\n",splitsu[5]?rtf(&splitsu[6],0):"");
      }
   }

 if(splitsu) splitsu[-1]=' ';
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Variable structure out.

  Variable var The Variable structure to output.

  char* filename The name of the file that is being processed (required for the cross reference label).
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFVariable(Variable var,char* filename)
{
 int i;

 if(var->scope&GLOBAL)
    fprintf(of,"{" STYLE_H3 " Variable %s\\par}\r\n",rtf(var->name,0));
 else
    fprintf(of,"{" STYLE_H4 " Variable %s\\par}\r\n",rtf(var->name,0));

 if(var->comment)
    fprintf(of,"%s\\par\r\n",rtf(var->comment,0));

 fprintf(of,"{" STYLE_TT " ");

 if(var->scope&LOCAL)
    fprintf(of,"static ");
 else
    if(!(var->scope&GLOBAL) && var->scope&(EXTERNAL|EXTERN_F))
       fprintf(of,"extern ");

 fprintf(of,"%s\\par}\r\n",rtf(var->type,0));

 if(var->scope&(GLOBAL|LOCAL))
   {
    if(var->incfrom || var->used->n || var->visible->n)
      {
       fprintf(of,"\\trowd\\trgaph120\\cellx1440\\cellx5220\\cellx9000\r\n\\intbl\\plain\r\n");

       if(var->incfrom)
          fprintf(of,"Inc. from:\\cell %s\\cell\\row\r\n",rtf(var->incfrom,0));

       for(i=0;i<var->visible->n;i++)
         {
          if(i==0) fprintf(of,"Visible in:");
          if(var->visible->s1[i][0]=='$' && !var->visible->s1[i][1])
             fprintf(of,"\\cell %s\\cell\\cell\\row\r\n",rtf(var->visible->s2[i],0));
          else
             fprintf(of,"\\cell %s()\\cell %s\\cell\\row\r\n",rtf(var->visible->s1[i],0),rtf(var->visible->s2[i],0));
         }

       for(i=0;i<var->used->n;i++)
         {
          if(i==0) fprintf(of,"Used in:");
          if(var->used->s1[i][0]=='$' && !var->used->s1[i][1])
             fprintf(of,"\\cell %s\\cell\\cell\\row\r\n",rtf(var->used->s2[i],0));
          else
             if(var->scope&LOCAL)
                fprintf(of,"\\cell %s()\\cell\\cell\\row\r\n",rtf(var->used->s1[i],0));
             else
                fprintf(of,"\\cell %s()\\cell %s\\cell\\row\r\n",rtf(var->used->s1[i],0),rtf(var->used->s2[i],0));
         }
       fprintf(of,"\\intbl0\r\n");
      }
   }
 else
    if(var->scope&(EXTERNAL|EXTERN_F) && var->defined)
      {
       fprintf(of,"\\trowd\\trgaph120\\cellx1440\\cellx5220\r\n\\intbl\\plain\r\n");
       fprintf(of,"Defined in:\\cell %s\\cell\\row\r\n",rtf(var->defined,0));
       fprintf(of,"\\intbl0\r\n");
      }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Function structure out.

  Function func The Function structure to output.

  char* filename The name of the file that is being processed (required for the cross reference label).
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFFunction(Function func,char* filename)
{
 int i,pret,pargs;
 char* comment2=NULL,*type;

 if(func->scope&(GLOBAL|EXTERNAL))
    fprintf(of,"{" STYLE_H3 " Global Function %s()\\par}\r\n",rtf(func->name,0));
 else
    fprintf(of,"{" STYLE_H3 " Local Function %s()\\par}\r\n",rtf(func->name,0));

 if(func->comment)
   {
    if(option_verbatim_comments)
       fprintf(of,"{" STYLE_TT "%s\\par}\r\n",rtf(func->comment,1));
    else
      {
       comment2=strstr(func->comment,"\r\n\r\n");
       if(comment2)
          comment2[0]=0;
       fprintf(of,"%s\\par\r\n",rtf(func->comment,0));
      }
   }

 fprintf(of,"{" STYLE_TT " ");

 if(func->scope&LOCAL)
    fprintf(of,"static ");
 if(func->scope&INLINED)
   fprintf(of,"inline ");

 if((type=strstr(func->type,"()")))
    type[0]=0;
 fprintf(of,"%s ( ",rtf(func->type,0));

 for(i=0;i<func->args->n;i++)
    fprintf(of,i?", %s":"%s",rtf(func->args->s1[i],0));

 if(type)
   {fprintf(of," %s\\par}\r\n",&type[1]);type[0]='(';}
 else
    fprintf(of," )\\par}\r\n");

 pret =strncmp("void ",func->type,5) && func->cret;
 for(pargs=0,i=0;i<func->args->n;i++)
    pargs = pargs || ( strcmp("void",func->args->s1[i]) && func->args->s2[i] );

 if(pret || pargs)
   {
    if(pret)
       fprintf(of,"{" STYLE_TT " %s\\par}\r\n{" STYLE_IND " %s\\par}\r\n",rtf(func->type,0),func->cret?rtf(func->cret,0):"");
    if(pargs)
       for(i=0;i<func->args->n;i++)
          fprintf(of,"{" STYLE_TT " %s\\par}\r\n{" STYLE_IND " %s\\par}\r\n",rtf(func->args->s1[i],0),func->args->s2[i]?rtf(func->args->s2[i],0):"");
   }

 if(comment2)
   {
    fprintf(of,"%s\\par\r\n",rtf(&comment2[2],0));
    comment2[0]='\n';
   }

 if(func->protofile || func->incfrom || func->calls->n || func->called->n || func->used->n || func->f_refs->n || func->v_refs->n)
   {
    fprintf(of,"\\trowd\\trgaph120\\cellx1440\\cellx5220\\cellx9000\r\n\\intbl\\plain\r\n");

    if(func->protofile)
       fprintf(of,"Prototype:\\cell %s\\cell\\cell\\row\r\n",rtf(func->protofile,0));

    if(func->incfrom)
       fprintf(of,"Inc. from:\\cell %s\\cell\\cell\\row\r\n",rtf(func->incfrom,0));

    if(func->calls->n)
      {
       int others=0;

       fprintf(of,"Calls: ");

       for(i=0;i<func->calls->n;i++)
          if(func->calls->s2[i])
             fprintf(of,"\\cell %s()\\cell %s\\cell\\row\r\n",rtf(func->calls->s1[i],0),rtf(func->calls->s2[i],0));
          else
             others++;

       if(others)
         {
          fprintf(of,"\\cell ");
          for(i=0;i<func->calls->n;i++)
             if(!func->calls->s2[i])
                fprintf(of,--others?" %s(),":" %s()",rtf(func->calls->s1[i],0));
          fprintf(of,"\\cell\\cell\\row\r\n");
         }
      }

    if(func->called->n)
      {
       for(i=0;i<func->called->n;i++)
         {
          if(i==0)
             fprintf(of,"Called by:");
          fprintf(of,"\\cell %s()\\cell %s\\cell\\row\r\n",rtf(func->called->s1[i],0),rtf(func->called->s2[i],0));
         }
      }

    if(func->used->n)
      {
       for(i=0;i<func->used->n;i++)
         {
          if(i==0)
             fprintf(of,"Used in:");
          if(func->used->s1[i][0]=='$' && !func->used->s1[i][1])
             fprintf(of,"\\cell %s\\cell\\cell\\row\r\n",rtf(func->used->s2[i],0));
          else
             fprintf(of,"\\cell %s()\\cell %s\\cell\\row\r\n",rtf(func->used->s1[i],0),rtf(func->used->s2[i],0));
         }
      }

    if(func->f_refs->n)
      {
       int others=0;

       fprintf(of,"Refs Func:");

       for(i=0;i<func->f_refs->n;i++)
          if(func->f_refs->s2[i])
             fprintf(of,"\\cell %s()\\cell %s\\cell\\row\r\n",rtf(func->f_refs->s1[i],0),rtf(func->f_refs->s2[i],0));
          else
             others++;

       if(others)
         {
          fprintf(of,"\\cell ");
          for(i=0;i<func->f_refs->n;i++)
             if(!func->f_refs->s2[i])
                fprintf(of,--others?" %s(),":" %s()",rtf(func->f_refs->s1[i],0));
          fprintf(of,"\\cell\\cell\\row\r\n");
         }
      }

    if(func->v_refs->n)
      {
       int others=0;

       fprintf(of,"Refs Var:");

       for(i=0;i<func->v_refs->n;i++)
          if(func->v_refs->s2[i])
             fprintf(of,"\\cell %s\\cell %s\\cell\\row\r\n",rtf(func->v_refs->s1[i],0),rtf(func->v_refs->s2[i],0));
          else
             others++;

       if(others)
         {
          fprintf(of,"\\cell ");
          for(i=0;i<func->v_refs->n;i++)
             if(!func->v_refs->s2[i])
                fprintf(of,--others?" %s,":" %s",rtf(func->v_refs->s1[i],0));
          fprintf(of,"\\cell\\cell\\row\r\n");
         }
      }
    fprintf(of,"\\intbl0\r\n");
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Write out the appendix information.

  StringList files The list of files to write.

  StringList2 funcs The list of functions to write.

  StringList2 vars The list of variables to write.

  StringList2 types The list of types to write.
  ++++++++++++++++++++++++++++++++++++++*/

void WriteRTFAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types)
{
 char* ofile;
 int i;

 /* Open the file */

 ofile=ConcatStrings(5,option_odir,"/",option_name,RTF_APDX,RTF_FILE);

 of=fopen(ofile,"w");

 if(!of)
   {fprintf(stderr,"cxref: Failed to open the RTF appendix file '%s'\r\n",ofile);exit(1);}

 /* Write the header out */

 WriteRTFPreamble(of);

 fprintf(of,"{" STYLE_H1 " Cross References\\par}\r\n");

 /* Write out the appendix of files. */

 if(files->n)
   {
    fprintf(of,"{" STYLE_H2 " Files\\par}\r\n");
    fprintf(of,"\\trowd\\trgaph120\\cellx4500\r\n\\intbl\\plain\r\n");
    for(i=0;i<files->n;i++)
       fprintf(of,"%s\\cell\\row\r\n",rtf(files->s[i],0));
    fprintf(of,"\\intbl0\r\n");
   }

 /* Write out the appendix of functions. */

 if(funcs->n)
   {
    fprintf(of,"{" STYLE_H2 " Global Functions\\par}\r\n");
    fprintf(of,"\\trowd\\trgaph120\\cellx4500\\cellx9000\r\n\\intbl\\plain\r\n");
    for(i=0;i<funcs->n;i++)
       fprintf(of,"%s\\cell %s\\cell\\row\r\n",rtf(funcs->s1[i],0),rtf(funcs->s2[i],0));
    fprintf(of,"\\intbl0\r\n");
   }

 /* Write out the appendix of variables. */

 if(vars->n)
   {
    fprintf(of,"{" STYLE_H2 " Global Variables\\par}\r\n");
    fprintf(of,"\\trowd\\trgaph120\\cellx4500\\cellx9000\r\n\\intbl\\plain\r\n");
    for(i=0;i<vars->n;i++)
       fprintf(of,"%s\\cell %s\\cell\\row\r\n",rtf(vars->s1[i],0),rtf(vars->s2[i],0));
    fprintf(of,"\\intbl0\r\n");
   }

 /* Write out the appendix of types. */

 if(types->n)
   {
    fprintf(of,"{" STYLE_H2 " Defined Types\\par}\r\n");
    fprintf(of,"\\trowd\\trgaph120\\cellx4500\\cellx9000\r\n\\intbl\\plain\r\n");
    for(i=0;i<types->n;i++)
      {
       if(!strncmp("enum",types->s1[i],4))
          fprintf(of,"%s\\cell %s\\cell\\row\r\n",rtf(types->s1[i],0),rtf(types->s2[i],0));
       else
          if(!strncmp("union",types->s1[i],5))
             fprintf(of,"%s\\cell %s\\cell\\row\r\n",rtf(types->s1[i],0),rtf(types->s2[i],0));
          else
             if(!strncmp("struct",types->s1[i],6))
                fprintf(of,"%s\\cell %s\\cell\\row\r\n",rtf(types->s1[i],0),rtf(types->s2[i],0));
             else
                fprintf(of,"%s\\cell %s\\cell\\row\r\n",rtf(types->s1[i],0),rtf(types->s2[i],0));
      }
    fprintf(of,"\\intbl0\r\n");
   }

 /* Finish up. */

 WriteRTFPostamble(of);

 fclose(of);

 /* Clear the memory in rtf(,0) */

 rtf(NULL,0); rtf(NULL,0); rtf(NULL,0); rtf(NULL,0);
}


/*++++++++++++++++++++++++++++++++++++++
  Write out the head of an RTF file.

  FILE *f The file to write to.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFPreamble(FILE *f)
{
 fputs("{\\rtf\\ansi\r\n",f);
 fputs("\\deff0\r\n",f);
 fputs("{\\fonttbl\r\n",f);
 fputs("{\\f0\\froman Times New Roman;}\r\n",f);
 fputs("{\\f1\\fmodern Courier New;}\r\n",f);
 fputs("}\r\n",f);
 fputs("{\\stylesheet\r\n",f);
 fputs("{" STYLE_NORM " Normal;}\r\n",f);
 fputs("{" STYLE_H1 " Heading 1;}\r\n",f);
 fputs("{" STYLE_H2 " Heading 2;}\r\n",f);
 fputs("{" STYLE_H3 " Heading 3;}\r\n",f);
 fputs("{" STYLE_H4 " Heading 4;}\r\n",f);
 fputs("{" STYLE_TT " Code;}\r\n",f);
 fputs("}\r\n",f);

 fputs("{\\info{\\comment This RTF file generated by cxref. cxref program (c) Andrew M. Bishop 1995,96,97,98,99.}}\r\n",f);

 if(!strcmp("A4",PAGE))
    fputs("\\paperw11880\\paperh16848\\margl1440\\margr1440\\margt1440\\margb1440\r\n",f);
 else
    fputs("\\paperw12240\\paperh15840\\margl1440\\margr1440\\margt1440\\margb1440\r\n",f);

 fputs("\\sectd\\plain\r\n" STYLE_NORM "\r\n",f);
}


/*++++++++++++++++++++++++++++++++++++++
  Write out the tail of an RTF file.

  FILE *f The file to write to.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteRTFPostamble(FILE *f)
{
 fputs("}\r\n",f);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the RTF file and main file reference that belong to the named file.

  char *name The name of the file to delete.
  ++++++++++++++++++++++++++++++++++++++*/

void WriteRTFFileDelete(char *name)
{
 char *ofile;

 ofile=ConcatStrings(4,option_odir,"/",name,RTF_FILE);
 unlink(ofile);
}


/*++++++++++++++++++++++++++++++++++++++
  Make the input string safe to output as RTF ( not \, { or } ).

  char* rtf Returns a safe RTF string.

  char* c A non-safe RTF string.

  int verbatim Set to true inside a verbatim environment.

  The function can only be called four times in each fprintf() since it returns one of only four static strings.
  ++++++++++++++++++++++++++++++++++++++*/

static char* rtf(char* c,int verbatim)
{
 static char safe[4][256],*malloced[4]={NULL,NULL,NULL,NULL};
 static int which=0;
 int copy=0,skip=0;
 int i=0,j=0,delta=4,len=256-delta;
 char *ret;

 which=(which+1)%4;
 ret=safe[which];

 safe[which][0]=0;

 if(malloced[which])
   {Free(malloced[which]);malloced[which]=NULL;}

 if(c)
   {
    i=CopyOrSkip(c,"rtf",&copy,&skip);

    while(1)
      {
       for(;j<len && c[i];i++)
         {
          if(copy)
            {ret[j++]=c[i]; if(c[i]=='\n') copy=0;}
          else if(skip)
            {               if(c[i]=='\n') skip=0;}
          else if(!verbatim && (j==0 || ret[j-1]==' ') && (c[i]==' ' || c[i]=='\t' || c[i]=='\n'))
            ;
          else
             switch(c[i])
               {
               case '\\':
               case '{':
               case '}':
                ret[j++]='\\';
                ret[j++]=c[i];
                break;
               case '\t':
                if(!verbatim)
                   ret[j++]=c[i];
                else
                   ret[j++]=' ';
                break;
               case '\n':
                if(verbatim)
                   ret[j++]='\\',ret[j++]='p',ret[j++]='a',ret[j++]='r';
                else
                   ret[j++]=' ';
                break;
               default:
                ret[j++]=c[i];
               }
          if(c[i]=='\n')
             i+=CopyOrSkip(c+i,"rtf",&copy,&skip);
         }

       if(c[i])                 /* Not finished */
         {
          if(malloced[which])
             malloced[which]=Realloc(malloced[which],len+delta+256);
          else
            {malloced[which]=Malloc(len+delta+256); strncpy(malloced[which],ret,(unsigned)j);}
          ret=malloced[which];
          len+=256;
         }
       else
         {
          ret[j]=0;

          if(!verbatim && j--)
             while(ret[j]==' ')
                ret[j--]=0;

          break;
         }
      }
   }

 return(ret);
}
