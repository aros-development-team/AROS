/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5f.

  Writes the SGML output.
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

/*+ The name of the output tex file that includes each of the others. +*/
#define SGML_FILE        ".sgml"
#define SGML_FILE_BACKUP ".sgml~"

/*+ The name of the output tex file that contains the appendix. +*/
#define SGML_APDX        ".apdx"

/*+ The comments are to be inserted verbatim. +*/
extern int option_verbatim_comments;

/*+ The name of the directory for the output. +*/
extern char* option_odir;

/*+ The base name of the file for the output. +*/
extern char* option_name;

/*+ The information about the cxref run, +*/
extern char *run_command,       /*+ the command line options. +*/
            *run_cpp_command;   /*+ the cpp command and options. +*/

/*+ The directories to go back to get to the base output directory. +*/
static char* goback=NULL;

static void WriteSGMLFilePart(File file);
static void WriteSGMLInclude(Include inc);
static void WriteSGMLSubInclude(Include inc,int depth);
static void WriteSGMLDefine(Define def);
static void WriteSGMLTypedef(Typedef type);
static void WriteSGMLStructUnion(StructUnion su,int depth);
static void WriteSGMLVariable(Variable var);
static void WriteSGMLFunction(Function func);

static void WriteSGMLPreamble(FILE* f,char* title);
static void WriteSGMLPostamble(FILE* f);

static char* sgml(char* c,int verbatim);

/*+ The output file for the SGML. +*/
static FILE* of;

/*+ The name of the file. +*/
static char *filename;


/*++++++++++++++++++++++++++++++++++++++
  Write an sgml file for a complete File structure and all components.

  File file The File structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

void WriteSGMLFile(File file)
{
 char* ofile;
 int i;

 filename=file->name;

 /* Open the file */

 ofile=ConcatStrings(4,option_odir,"/",file->name,SGML_FILE);

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
   {fprintf(stderr,"cxref: Failed to open the SGML output file '%s'\n",ofile);exit(1);}

 for(goback="",i=strlen(file->name);i>0;i--)
    if(file->name[i]=='/')
       goback=ConcatStrings(2,goback,"../");

 /* Write out a header. */

 WriteSGMLPreamble(of,ConcatStrings(5,"Cross reference for ",file->name," of ",option_name,"."));

 /*+ The file structure is broken into its components and they are each written out. +*/

 WriteSGMLFilePart(file);

 if(file->includes)
   {
    Include inc =file->includes;
    fprintf(of,"\n<sect1>Included Files\n\n<p>\n");
    do{
       WriteSGMLInclude(inc);
      }
    while((inc=inc->next));
   }

 if(file->defines)
   {
    Define def =file->defines;
    fprintf(of,"\n<sect1>Preprocessor definitions\n\n<p>\n");
    do{
       if(def!=file->defines)
          fprintf(of,"<p>\n");
       WriteSGMLDefine(def);
      }
    while((def=def->next));
   }

 if(file->typedefs)
   {
    Typedef type=file->typedefs;
    do{
       WriteSGMLTypedef(type);
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
       int first_ext=1,first_local=1;
       Variable var=file->variables;
       do{
          if(var->scope&GLOBAL)
             WriteSGMLVariable(var);
         }
       while((var=var->next));
       var=file->variables;
       do{
          if(var->scope&(EXTERNAL|EXTERN_F) && !(var->scope&GLOBAL))
            {
             if(first_ext)
               {fprintf(of,"\n<sect1>External Variables\n\n"); first_ext=0;}
             fprintf(of,"<p>\n");
             WriteSGMLVariable(var);
            }
         }
       while((var=var->next));
       var=file->variables;
       do{
          if(var->scope&LOCAL)
            {
             if(first_local)
               {fprintf(of,"\n<sect1>Local Variables\n\n"); first_local=0;}
             fprintf(of,"<p>\n");
             WriteSGMLVariable(var);
            }
         }
       while((var=var->next));
      }
   }

 if(file->functions)
   {
    Function func=file->functions;
    do{
       if(func->scope&(GLOBAL|EXTERNAL))
          WriteSGMLFunction(func);
      }
    while((func=func->next));
    func=file->functions;
    do{
       if(func->scope&LOCAL)
          WriteSGMLFunction(func);
      }
    while((func=func->next));
   }

 WriteSGMLPostamble(of);

 fclose(of);

 /* Clear the memory in sgml() */

 sgml(NULL,0); sgml(NULL,0); sgml(NULL,0); sgml(NULL,0);
}


/*++++++++++++++++++++++++++++++++++++++
  Write a File structure out.

  File file The File to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLFilePart(File file)
{
 int i;

 fprintf(of,"<sect>File %s\n",sgml(file->name,0));

 if(file->comment)
   {
    if(option_verbatim_comments)
       fprintf(of,"<p><tscreen><verb>\n%s\n</verb></tscreen>\n\n",sgml(file->comment,1));
    else
      {
       char *rcs1=strstr(file->comment,"$Header"),*rcs2=NULL;
       if(rcs1)
         {
          rcs2=strstr(&rcs1[1],"$");
          if(rcs2)
            {
             rcs2[0]=0;
             fprintf(of,"<bf>RCS %s</bf>\n<p>\n",sgml(&rcs1[1],0));
             rcs2[0]='$';
            }
         }
       if(rcs2)
          fprintf(of,"%s\n<p>\n",sgml(&rcs2[2],0));
       else
          fprintf(of,"%s\n<p>\n",sgml(file->comment,0));
      }
   }

 if(file->inc_in->n)
   {
    int i;

    fprintf(of,"<p>\n<descrip>\n<tag>Included in:</tag>\n<itemize>\n");
    for(i=0;i<file->inc_in->n;i++)
       fprintf(of,"<item>%s\n",sgml(file->inc_in->s[i],0));
    fprintf(of,"</itemize>\n</descrip>\n");
   }

 if(file->f_refs->n || file->v_refs->n)
    fprintf(of,"<descrip>\n");

 if(file->f_refs->n)
   {
    int others=0;
    fprintf(of,"<tag>References Functions:</tag>\n<itemize>\n");
    for(i=0;i<file->f_refs->n;i++)
       if(file->f_refs->s2[i])
          fprintf(of,"<item>%s()  :  %s\n",sgml(file->f_refs->s1[i],0),sgml(file->f_refs->s2[i],0));
       else
          others++;

    if(others)
      {
       fprintf(of,"<item>");
       for(i=0;i<file->f_refs->n;i++)
          if(!file->f_refs->s2[i])
             fprintf(of,--others?" %s(),":" %s()",sgml(file->f_refs->s1[i],0));
       fprintf(of,"\n");
      }
    fprintf(of,"</itemize>\n");
   }

 if(file->v_refs->n)
   {
    int others=0;
    fprintf(of,"<tag>References Variables:</tag>\n<itemize>\n");
    for(i=0;i<file->v_refs->n;i++)
      {
       if(file->v_refs->s2[i])
          fprintf(of,"<item>%s  :  %s\n",sgml(file->v_refs->s1[i],0),sgml(file->v_refs->s2[i],0));
       else
          others++;
      }

    if(others)
      {
       fprintf(of,"<item>");
       for(i=0;i<file->v_refs->n;i++)
          if(!file->v_refs->s2[i])
             fprintf(of,--others?" %s,":" %s",sgml(file->v_refs->s1[i],0));
       fprintf(of,"\n");
      }
    fprintf(of,"</itemize>\n");
   }

 if(file->f_refs->n || file->v_refs->n)
    fprintf(of,"</descrip>\n");
}


/*++++++++++++++++++++++++++++++++++++++
  Write an Include structure out.

  Include inc The Include structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLInclude(Include inc)
{
 if(inc->comment)
    fprintf(of,"%s\n<p>\n",sgml(inc->comment,0));

 fprintf(of,"<itemize>\n");

 if(inc->scope==LOCAL)
    fprintf(of,"<item><tt>#include &quot;%s&quot;</tt>\n",sgml(inc->name,0));
 else
    fprintf(of,"<item><tt>#include &lt;%s&gt;</tt>\n",sgml(inc->name,0));

 if(inc->includes)
    WriteSGMLSubInclude(inc->includes,1);

 fprintf(of,"</itemize>\n");
}


/*++++++++++++++++++++++++++++++++++++++
  Write an Sub Include structure out. (An include structure that is included from another file.)

  Include inc The Include structure to output.

  int depth The depth of the include hierarchy.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLSubInclude(Include inc,int depth)
{
 fprintf(of,"<itemize>\n");

 while(inc)
   {
    if(inc->scope==LOCAL)
       fprintf(of,"<item><tt>#include &quot;%s&quot;</tt>\n",sgml(inc->name,0));
    else
       fprintf(of,"<item><tt>#include &lt;%s&gt;</tt>\n",sgml(inc->name,0));

    if(inc->includes)
       WriteSGMLSubInclude(inc->includes,depth+1);

    inc=inc->next;
   }

 fprintf(of,"</itemize>\n");
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Define structure out.

  Define def The Define structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLDefine(Define def)
{
 int i;
 int pargs=0;

 if(def->comment)
    fprintf(of,"%s\n<p>\n",sgml(def->comment,0));

 fprintf(of,"<tt>#define %s",sgml(def->name,0));

 if(def->value)
    fprintf(of," %s",sgml(def->value,0));

 if(def->args->n)
   {
    fprintf(of,"( ");
    for(i=0;i<def->args->n;i++)
       fprintf(of,i?", %s":"%s",sgml(def->args->s1[i],0));
    fprintf(of," )");
   }
 fprintf(of,"</tt><newline>\n");

 for(i=0;i<def->args->n;i++)
    if(def->args->s2[i])
       pargs=1;

 if(pargs)
   {
    fprintf(of,"<descrip>\n");
    for(i=0;i<def->args->n;i++)
       fprintf(of,"<tag><tt>%s</tt></tag>\n%s\n",sgml(def->args->s1[i],0),def->args->s2[i]?sgml(def->args->s2[i],0):"");
    fprintf(of,"</descrip>\n");
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Typedef structure out.

  Typedef type The Typedef structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLTypedef(Typedef type)
{
 fprintf(of,"\n<sect1>");

 if(type->type)
    fprintf(of,"Typedef %s",sgml(type->name,0));
 else
    fprintf(of,"Type %s",sgml(type->name,0));

 fprintf(of,"\n\n<p>\n");

 if(type->comment)
    fprintf(of,"%s\n<p>\n",sgml(type->comment,0));

 if(type->type)
    fprintf(of,"<tt>typedef %s</tt><newline>\n",sgml(type->type,0));

 if(type->sutype)
   {
    fprintf(of,"<itemize>\n");
    WriteSGMLStructUnion(type->sutype,0);
    fprintf(of,"</itemize>\n");
   }
 else
    if(type->typexref)
      {
       fprintf(of,"<descrip>\n<tag>See:</tag>\n<itemize>\n");
       if(type->typexref->type)
          fprintf(of,"<item>Typedef %s\n",sgml(type->typexref->name,0));
       else
          if(!strncmp("enum",type->typexref->name,4))
             fprintf(of,"<item>Type %s\n",sgml(type->typexref->name,0));
          else
             if(!strncmp("union",type->typexref->name,5))
                fprintf(of,"<item>Type %s\n",sgml(type->typexref->name,0));
             else
                if(!strncmp("struct",type->typexref->name,6))
                   fprintf(of,"<item>Type %s\n",sgml(type->typexref->name,0));
       fprintf(of,"</itemize>\n</descrip>\n");
      }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a structure / union structure out.

  StructUnion su The structure / union to write.

  int depth The current depth within the structure.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLStructUnion(StructUnion su, int depth)
{
 int i;
 char* splitsu=NULL;

 splitsu=strstr(su->name,"{...}");
 if(splitsu) splitsu[-1]=0;

 if(depth && su->comment && !su->comps)
    fprintf(of,"<item><tt>%s;      </tt>%s<newline>\n",sgml(su->name,0),sgml(su->comment,0));
 else if(!depth || su->comps)
    fprintf(of,"<item><tt>%s</tt><newline>\n",sgml(su->name,0));
 else
    fprintf(of,"<item><tt>%s;</tt><newline>\n",sgml(su->name,0));

 if(!depth || su->comps)
   {
    fprintf(of,"<itemize>\n");

    fprintf(of,"<item><tt>{</tt><newline>\n");

    for(i=0;i<su->n_comp;i++)
       WriteSGMLStructUnion(su->comps[i],depth+1);

    fprintf(of,"<item><tt>}</tt><newline>\n");

    fprintf(of,"</itemize>\n");

    if(splitsu)
      {
       if(depth && su->comment)
          fprintf(of,"<item><tt>%s;      </tt>%s<newline>\n",splitsu[5]?sgml(&splitsu[6],0):"",sgml(su->comment,0));
       else
          fprintf(of,"<item><tt>%s;</tt><newline>\n",splitsu[5]?sgml(&splitsu[6],0):"");
      }
   }

 if(splitsu) splitsu[-1]=' ';
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Variable structure out.

  Variable var The Variable structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLVariable(Variable var)
{
 int i;

 if(var->scope&GLOBAL)
    fprintf(of,"\n<sect1>Global Variable %s\n\n<p>\n",sgml(var->name,0));
 else
    fprintf(of,"<bf>%s</bf><newline>\n",sgml(var->name,0));

 if(var->comment)
    fprintf(of,"%s\n<p>\n",sgml(var->comment,0));

 fprintf(of,"<tt>");

 if(var->scope&LOCAL)
    fprintf(of,"static ");
 else
    if(!(var->scope&GLOBAL) && var->scope&(EXTERNAL|EXTERN_F))
       fprintf(of,"extern ");

 fprintf(of,"%s</tt><newline>\n",sgml(var->type,0));

 if(var->scope&(GLOBAL|LOCAL))
   {
    if(var->incfrom || var->visible->n || var->used->n)
       fprintf(of,"<descrip>\n");

    if(var->incfrom)
      {
       fprintf(of,"<tag>Included from:</tag>\n<itemize>\n");
       fprintf(of,"<item>%s\n",sgml(var->incfrom,0));
       fprintf(of,"</itemize>\n");
      }

    if(var->visible->n)
      {
       fprintf(of,"<tag>Visible in:</tag>\n<itemize>\n");
       for(i=0;i<var->visible->n;i++)
          if(var->visible->s1[i][0]=='$' && !var->visible->s1[i][1])
             fprintf(of,"<item>%s\n",sgml(var->visible->s2[i],0));
          else
             fprintf(of,"<item>%s()  :  %s\n",sgml(var->visible->s1[i],0),sgml(var->visible->s2[i],0));
       fprintf(of,"</itemize>\n");
      }

    if(var->used->n)
      {
       fprintf(of,"<tag>Used in:</tag>\n<itemize>\n");
       for(i=0;i<var->used->n;i++)
         {
          if(var->used->s1[i][0]=='$' && !var->used->s1[i][1])
             fprintf(of,"<item>%s\n",sgml(var->used->s2[i],0));
          else
            {
             if(var->scope&LOCAL)
                fprintf(of,"<item>%s()\n",sgml(var->used->s1[i],0));
             else
                fprintf(of,"<item>%s()  :  %s\n",sgml(var->used->s1[i],0),sgml(var->used->s2[i],0));
            }
         }
       fprintf(of,"</itemize>\n");
      }

    if(var->incfrom || var->visible->n || var->used->n)
       fprintf(of,"</descrip>\n");
   }
 else
    if(var->scope&(EXTERNAL|EXTERN_F) && var->defined)
      {
       fprintf(of,"<descrip>\n<tag>Defined in:</tag>\n<itemize>\n");
       fprintf(of,"<item>%s",sgml(var->name,0));
       fprintf(of,"</itemize>\n</descrip>\n");
      }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Function structure out.

  Function func The Function structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLFunction(Function func)
{
 int i,pret,pargs;
 char* comment2=NULL,*type;

 if(func->scope&(GLOBAL|EXTERNAL))
    fprintf(of,"\n<sect1>Global Function %s()\n\n<p>",sgml(func->name,0));
 else
    fprintf(of,"\n<sect1>Local Function %s()\n\n<p>",sgml(func->name,0));

 if(func->comment)
   {
    if(option_verbatim_comments)
       fprintf(of,"<tscreen><verb>\n%s\n</verb></tscreen>\n\n",sgml(func->comment,1));
    else
      {
       comment2=strstr(func->comment,"\n\n");
       if(comment2)
          comment2[0]=0;
       fprintf(of,"%s\n<p>\n",sgml(func->comment,0));
      }
   }

 fprintf(of,"<tt>");

 if(func->scope&LOCAL)
    fprintf(of,"static ");
 if(func->scope&INLINED)
   fprintf(of,"inline ");

 if((type=strstr(func->type,"()")))
    type[0]=0;
 fprintf(of,"%s ( ",sgml(func->type,0));

 for(i=0;i<func->args->n;i++)
    fprintf(of,i?", %s":"%s",sgml(func->args->s1[i],0));

 if(type)
   {fprintf(of," %s</tt><newline>\n",sgml(&type[1],0));type[0]='(';}
 else
    fprintf(of," )</tt><newline>\n");

 pret =strncmp("void ",func->type,5) && func->cret;
 for(pargs=0,i=0;i<func->args->n;i++)
    pargs = pargs || ( strcmp("void",func->args->s1[i]) && func->args->s2[i] );

 if(pret || pargs)
   {
    fprintf(of,"<descrip>\n");
    if(pret)
       fprintf(of,"<tag><tt>%s</tt></tag>\n%s\n",sgml(func->type,0),func->cret?sgml(func->cret,0):"&nbsp;");
    if(pargs)
       for(i=0;i<func->args->n;i++)
          fprintf(of,"<tag><tt>%s</tt></tag>\n%s\n",sgml(func->args->s1[i],0),func->args->s2[i]?sgml(func->args->s2[i],0):"&nbsp;");
    fprintf(of,"</descrip>\n");
   }

 if(comment2)
   {
    fprintf(of,"%s\n<p>\n",sgml(&comment2[2],0));
    comment2[0]='\n';
   }

 if(func->protofile || func->incfrom || func->calls->n || func->called->n || func->used->n || func->f_refs->n || func->v_refs->n)
    fprintf(of,"<descrip>\n");

 if(func->protofile)
   {
    fprintf(of,"<tag>Prototyped in:</tag>\n<itemize>\n");
    fprintf(of,"<item>%s\n",sgml(func->protofile,0));
    fprintf(of,"</itemize>\n");
   }

 if(func->incfrom)
   {
    fprintf(of,"<tag>Included from:</tag>\n<itemize>\n");
    fprintf(of,"<item>%s\n",sgml(func->incfrom,0));
    fprintf(of,"</itemize>\n");
   }

 if(func->calls->n)
   {
    int others=0;
    fprintf(of,"<tag>Calls:</tag>\n<itemize>\n");
    for(i=0;i<func->calls->n;i++)
      {
       if(func->calls->s2[i])
          fprintf(of,"<item>%s()  :  %s\n",sgml(func->calls->s1[i],0),sgml(func->calls->s2[i],0));
       else
          others++;
      }

    if(others)
      {
       fprintf(of,"<item>");
       for(i=0;i<func->calls->n;i++)
          if(!func->calls->s2[i])
             fprintf(of,--others?"%s(), ":"%s()",sgml(func->calls->s1[i],0));
       fprintf(of,"\n");
      }
    fprintf(of,"</itemize>\n");
   }

 if(func->called->n)
   {
    fprintf(of,"<tag>Called by:</tag>\n<itemize>\n");
    for(i=0;i<func->called->n;i++)
       fprintf(of,"<item>%s()  :  %s\n",sgml(func->called->s1[i],0),sgml(func->called->s2[i],0));
    fprintf(of,"</itemize>\n");
   }

 if(func->used->n)
   {
    fprintf(of,"<tag>Used in:</tag>\n<itemize>\n");
    for(i=0;i<func->used->n;i++)
      {
       if(func->used->s1[i][0]=='$' && !func->used->s1[i][1])
          fprintf(of,"<item>%s\n",sgml(func->used->s2[i],0));
       else
          fprintf(of,"<item>%s()  :  %s\n",sgml(func->used->s1[i],0),sgml(func->used->s2[i],0));
      }
    fprintf(of,"</itemize>\n");
   }

 if(func->f_refs->n)
   {
    int others=0;
    fprintf(of,"<tag>References Functions:</tag>\n<itemize>\n");
    for(i=0;i<func->f_refs->n;i++)
      {
       if(func->f_refs->s2[i])
          fprintf(of,"<item>%s()  :  %s\n",sgml(func->f_refs->s1[i],0),sgml(func->f_refs->s2[i],0));
       else
          others++;
      }

    if(others)
      {
       fprintf(of,"<item>");
       for(i=0;i<func->f_refs->n;i++)
          if(!func->f_refs->s2[i])
             fprintf(of,--others?"%s(), ":"%s()",sgml(func->f_refs->s1[i],0));
       fprintf(of,"\n");
      }
    fprintf(of,"</itemize>\n");
   }

 if(func->v_refs->n)
   {
    int others=0;
    fprintf(of,"<tag>References Variables:</tag>\n<itemize>\n");
    for(i=0;i<func->v_refs->n;i++)
      {
       if(func->v_refs->s2[i])
          fprintf(of,"<item>%s  :  %s\n",sgml(func->v_refs->s1[i],0),sgml(func->v_refs->s2[i],0));
       else
          others++;
      }

    if(others)
      {
       fprintf(of,"<item>");
       for(i=0;i<func->v_refs->n;i++)
          if(!func->v_refs->s2[i])
             fprintf(of,--others?"%s, ":"%s",sgml(func->v_refs->s1[i],0));
       fprintf(of,"\n");
      }
    fprintf(of,"</itemize>\n");
   }

 if(func->protofile || func->incfrom || func->calls->n || func->called->n || func->used->n || func->f_refs->n || func->v_refs->n)
    fprintf(of,"</descrip>\n");
}


/*++++++++++++++++++++++++++++++++++++++
  Write out a standard pre-amble.

  FILE* f The file to write the pre amble to.

  char* title The title of the file.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLPreamble(FILE* f,char* title)
{
 fputs("<!DOCTYPE LINUXDOC SYSTEM>\n",f);
 fputs("\n",f);
 fputs("<!-- This SGML file generated by cxref. -->\n",f);
 fputs("<!-- cxref program (c) Andrew M. Bishop 1995,96,97,98,99. -->\n",f);
 fputs("\n",f);
 fputs("<!--\n",f);
 if(filename)
    fprintf(f,"Cxref: %s %s\n",run_command,filename);
 else
    fprintf(f,"Cxref: %s\n",run_command);
 fprintf(f,"CPP  : %s\n",run_cpp_command);
 fputs("-->\n",f);
 fputs("\n",f);
 fputs("<article>\n",f);
 fputs("\n",f);
 fputs("<title>",f);
 fputs(title,f);
 fputs("\n",f);
 fputs("<author>cxref\n",f);
 fputs("\n",f);
}


/*++++++++++++++++++++++++++++++++++++++
  Write out a standard post-amble. This includes the end of document marker.

  FILE* f The file to write the post amble to.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteSGMLPostamble(FILE* f)
{
 fputs("\n",f);
 fputs("</article>\n",f);
}


/*++++++++++++++++++++++++++++++++++++++
  Write out the appendix information.

  StringList files The list of files to write.

  StringList2 funcs The list of functions to write.

  StringList2 vars The list of variables to write.

  StringList2 types The list of types to write.
  ++++++++++++++++++++++++++++++++++++++*/

void WriteSGMLAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types)
{
 char* ofile;
 int i;

 filename=NULL;

 /* Open the file */

 ofile=ConcatStrings(5,option_odir,"/",option_name,SGML_APDX,SGML_FILE);

 of=fopen(ofile,"w");

 if(!of)
   {fprintf(stderr,"cxref: Failed to open the SGML appendix file '%s'\n",ofile);exit(1);}

 /* Write the file structure out */

 WriteSGMLPreamble(of,ConcatStrings(3,"Cross reference index of ",option_name,"."));

 fprintf(of,"<sect>Cross References\n");

 /* Write out the appendix of files. */

 if(files->n)
   {
    fprintf(of,"\n<sect1>Files\n\n<p>\n");
    fprintf(of,"<itemize>\n");
    for(i=0;i<files->n;i++)
       fprintf(of,"<item>%s>\n",sgml(files->s[i],0));
    fprintf(of,"</itemize>\n");
   }

 /* Write out the appendix of functions. */

 if(funcs->n)
   {
    fprintf(of,"\n<sect1>Global Functions\n\n<p>\n");
    fprintf(of,"<itemize>\n");
    for(i=0;i<funcs->n;i++)
       fprintf(of,"<item>%s()  :  %s\n",sgml(funcs->s1[i],0),sgml(funcs->s2[i],0));
    fprintf(of,"</itemize>\n");
   }

 /* Write out the appendix of variables. */

 if(vars->n)
   {
    fprintf(of,"\n<sect1>Global Variables\n\n<p>\n");
    fprintf(of,"<itemize>\n");
    for(i=0;i<vars->n;i++)
       fprintf(of,"<item>%s  :  %s\n",sgml(vars->s1[i],0),sgml(vars->s2[i],0));
    fprintf(of,"</itemize>\n");
   }

 /* Write out the appendix of types. */

 if(types->n)
   {
    fprintf(of,"\n<sect1>Defined Types\n\n<p>\n");
    fprintf(of,"<itemize>\n");
    for(i=0;i<types->n;i++)
       if(!strncmp("enum",types->s1[i],4))
          fprintf(of,"<item>%s  :  %s\n",sgml(types->s1[i],0),sgml(types->s2[i],0));
       else
          if(!strncmp("union",types->s1[i],5))
             fprintf(of,"<item>%s  :  %s\n",sgml(types->s1[i],0),sgml(types->s2[i],0));
          else
             if(!strncmp("struct",types->s1[i],6))
                fprintf(of,"<item>%s  :  %s\n",sgml(types->s1[i],0),sgml(types->s2[i],0));
             else
                fprintf(of,"<item>%s  :  %s\n",sgml(types->s1[i],0),sgml(types->s2[i],0));
    fprintf(of,"</itemize>\n");
   }

 WriteSGMLPostamble(of);

 fclose(of);

 /* Clear the memory in sgml(,0) */

 sgml(NULL,0); sgml(NULL,0); sgml(NULL,0); sgml(NULL,0);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the SGML file and main file reference that belong to the named file.

  char *name The name of the file to delete.
  ++++++++++++++++++++++++++++++++++++++*/

void WriteSGMLFileDelete(char *name)
{
 char *ofile;

 ofile=ConcatStrings(4,option_odir,"/",name,SGML_FILE);
 unlink(ofile);
}


/*++++++++++++++++++++++++++++++++++++++
  Make the input string safe to output as SGML ( not <,  >,  &,  ",  $,  #,  % or ~ ).

  char* sgml Returns a safe SGML string.

  char* c A non-safe SGML string.

  int verbatim Set to true inside a verbatim environment.

  The function can only be called four times in each fprintf() since it returns one of only four static strings.
  ++++++++++++++++++++++++++++++++++++++*/

static char* sgml(char* c,int verbatim)
{
 static char safe[4][256],*malloced[4]={NULL,NULL,NULL,NULL};
 static int which=0;
 int copy=0,skip=0;
 int i=0,j=0,delta=10,len=256-delta;
 char* ret;

 which=(which+1)%4;
 ret=safe[which];

 safe[which][0]=0;

 if(malloced[which])
   {Free(malloced[which]);malloced[which]=NULL;}

 if(c)
   {
    i=CopyOrSkip(c,"sgml",&copy,&skip);

    while(1)
      {
       for(;j<len && c[i];i++)
         {
          if(copy)
            {ret[j++]=c[i]; if(c[i]=='\n') copy=0;}
          else if(skip)
            {               if(c[i]=='\n') skip=0;}
          else if(verbatim)
             switch(c[i])
               {
               case '&':
                strcpy(&ret[j],"&ero;");j+=5;
                break;
               case '<':
                if(c[i+1]=='/')
                  {strcpy(&ret[j],"&etago;");j+=7; break;}
               default:
                ret[j++]=c[i];
               }
          else
             switch(c[i])
               {
               case '<':
                if(c[i+1]=='/')
                  {strcpy(&ret[j],"&etago;");j+=7;}
                else
                  {strcpy(&ret[j],"&lt;");j+=4;}
                break;
               case '>':
                strcpy(&ret[j],"&gt;");j+=4;
                break;
               case '"':
                strcpy(&ret[j],"&quot;");j+=6;
                break;
               case '&':
                strcpy(&ret[j],"&amp;");j+=5;
                break;
               case '$':
                strcpy(&ret[j],"&dollar;");j+=8;
                break;
               case '#':
                strcpy(&ret[j],"&num;");j+=5;
                break;
               case '%':
                strcpy(&ret[j],"&percnt;");j+=8;
                break;
               case '~':
                strcpy(&ret[j],"&tilde;");j+=7;
                break;
               case '\n':
                if(j && ret[j-1]=='\n')
                  {
                   strcpy(&ret[j],"<newline>");j+=9;
                  }
                ret[j++]=c[i];
                break;
               default:
                ret[j++]=c[i];
               }
          if(c[i]=='\n')
             i+=CopyOrSkip(c+i,"sgml",&copy,&skip);
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
         {ret[j]=0; break;}
      }
   }

 return(ret);
}
