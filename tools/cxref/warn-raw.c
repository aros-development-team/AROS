/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5f.

  Writes the raw information and / or warnings out.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,99,2001,04 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "datatype.h"
#include "cxref.h"
#include "memory.h"

static void WriteWarnRawFilePart(File file);
static void WriteWarnRawInclude(Include inc);
static void WriteWarnRawSubInclude(Include inc,int depth);
static void WriteWarnRawDefine(Define def);
static void WriteWarnRawTypedef(Typedef type);
static void WriteWarnRawStructUnion(StructUnion su, int depth,StructUnion base);
static void WriteWarnRawVariable(Variable var);
static void WriteWarnRawFunction(Function func);

/*+ Output option. +*/
extern int option_warn,option_raw,option_xref,option_index;

/*+ The name of the current file. +*/
static char* filename=NULL;

/*++++++++++++++++++++++++++++++++++++++
  Write the raw / warning output for a complete File structure and all components.

  File file The File structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

void WriteWarnRawFile(File file)
{
 Include   inc =file->includes;
 Define    def =file->defines;
 Typedef   type=file->typedefs;
 Variable  var=file->variables;
 Function  func=file->functions;

 filename=file->name;

 /*+ The file structure is broken into its components and they are each written out. +*/

 if(option_raw)
    printf("----------------------------------------\n");

 WriteWarnRawFilePart(file);

 while(inc)
   {
    WriteWarnRawInclude(inc);
    inc=inc->next;
   }

 while(def)
   {
    WriteWarnRawDefine(def);
    def=def->next;
   }

 while(type)
   {
    WriteWarnRawTypedef(type);
    type=type->next;
   }

 while(var)
   {
    WriteWarnRawVariable(var);
    var=var->next;
   }

 while(func)
   {
    WriteWarnRawFunction(func);
    func=func->next;
   }

 if(option_raw)
    printf("----------------------------------------\n\n");
}


/*++++++++++++++++++++++++++++++++++++++
  Write a File structure out.

  File file The File structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteWarnRawFilePart(File file)
{
 int i;

 if(option_raw)
    printf("FILE : '%s'\n",file->name);

 if(file->comment && option_raw)
    printf("<<<\n%s\n>>>\n",file->comment);

 if(option_warn&WARN_COMMENT && !file->comment)
    printf("Warning %16s : File does not have a comment.\n",filename);

 if(option_xref&XREF_FILE)
   {
    if(option_raw)
       for(i=0;i<file->inc_in->n;i++)
          printf("Included in %s\n",file->inc_in->s[i]);

    if(option_warn&WARN_XREF)
      {
       int len=strlen(file->name)-2;
       if(!file->inc_in->n && !strcmp(&file->name[len],".h"))
          printf("Warning %16s : Header file '%s' is not included in any files.\n",filename,file->name);
       if( file->inc_in->n && !strcmp(&file->name[len],".c"))
          printf("Warning %16s : Source file '%s' is included in another file.\n",filename,file->name);
      }
   }

 if(option_xref&XREF_FUNC)
    for(i=0;i<file->f_refs->n;i++)
      {
       if(option_raw)
         {
          if(file->f_refs->s2[i])
             printf("References Function %s : %s\n",file->f_refs->s1[i],file->f_refs->s2[i]);
          else
             printf("References Function %s\n",file->f_refs->s1[i]);
         }
       if(option_warn&WARN_XREF && !file->f_refs->s2[i])
          printf("Warning %16s : File references function '%s()' whose definition is unknown.\n",filename,file->f_refs->s1[i]);
      }

 if(option_xref&XREF_VAR)
    for(i=0;i<file->v_refs->n;i++)
      {
       if(option_raw)
         {
          if(file->v_refs->s2[i])
             printf("References Variable %s : %s\n",file->v_refs->s1[i],file->v_refs->s2[i]);
          else
             printf("References Variable %s\n",file->v_refs->s1[i]);
         }
       if(option_warn&WARN_XREF && !file->v_refs->s2[i])
          printf("Warning %16s : File references variable '%s' whose definition is unknown.\n",filename,file->v_refs->s1[i]);
      }
}


/*++++++++++++++++++++++++++++++++++++++
  Write an Include structure out.

  Include inc The Include structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteWarnRawInclude(Include inc)
{
 if(option_raw)
    printf("\nINCLUDES : '%s' [%s file]\n",inc->name,(inc->scope==GLOBAL?"System":"Local"));

 if(inc->comment && option_raw)
    printf("<<<\n%s\n>>>\n",inc->comment);
 if(option_warn&WARN_COMMENT && !inc->comment)
    printf("Warning %16s : #Include '%s' does not have a comment.\n",filename,inc->name);

 if(option_raw && inc->includes)
    WriteWarnRawSubInclude(inc->includes,1);
}


/*++++++++++++++++++++++++++++++++++++++
  Write an Sub-Include structure out.

  Include inc The Include structure to output.

  int depth The depth of the include hierarchy.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteWarnRawSubInclude(Include inc,int depth)
{
 int i;

 while(inc)
   {
    for(i=0;i<depth;i++) printf("   ");
    printf("INCLUDES : '%s' [%s file]\n",inc->name,(inc->scope==GLOBAL?"System":"Local"));

    if(inc->includes)
       WriteWarnRawSubInclude(inc->includes,depth+1);

    inc=inc->next;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Define structure out.

  Define def The Define structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteWarnRawDefine(Define def)
{
 int i;

 if(option_raw)
   {
    printf("\nDEFINES : '%s' ",def->name);

    if(def->value)
       printf("= %s",def->value);

    if(def->args->n)
      {
       printf("(");
       for(i=0;i<def->args->n;i++)
          printf(i?",%s":"%s",def->args->s1[i]);
       printf(")");
      }

    printf("\n");
   }

 if(def->comment && option_raw)
    printf("<<<\n%s\n>>>\n",def->comment);
 if(option_warn&WARN_COMMENT && !def->comment)
    printf("Warning %16s : #Define '%s' does not have a comment.\n",filename,def->name);

 if(option_raw)
    printf("Defined: %s:%d\n",filename,def->lineno);

 for(i=0;i<def->args->n;i++)
   {
    if(option_raw)
      {
       if(def->args->s2[i])
          printf("Arguments: %s <<<%s>>>\n",def->args->s1[i],def->args->s2[i]);
       else
          printf("Arguments: %s\n",def->args->s1[i]);
      }
    if(option_warn&WARN_COMMENT && !def->args->s2[i])
       printf("Warning %16s : #Define '%s' has an argument '%s' with no comment.\n",filename,def->name,def->args->s1[i]);
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Typedef structure out.

  Typedef type The Typedef structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteWarnRawTypedef(Typedef type)
{
 if(option_raw)
   {
    if(type->type)
       printf("\nTYPEDEF : '%s'\n",type->name);
    else
       printf("\nTYPE : '%s'\n",type->name);
   }

 if(type->comment && option_raw)
    printf("<<<\n%s\n>>>\n",type->comment);
 if(option_warn&WARN_COMMENT && !type->comment)
    printf("Warning %16s : Type '%s' does not have a comment.\n",filename,type->name);

 if(option_raw)
    printf("Defined: %s:%d\n",filename,type->lineno);

 if(option_raw)
    if(type->type)
       printf("Type: %s\n",type->type);

 if(option_raw)
    if(type->typexref)
       printf("See: %s %s\n",type->typexref->type?"Typedef":"Type",type->typexref->name);

 if(type->sutype)
    WriteWarnRawStructUnion(type->sutype,0,type->sutype);
}


/*++++++++++++++++++++++++++++++++++++++
  Write a structure / union / enum out.

  StructUnion su The structure / union / enum to write.

  int depth The depth within the structure.

  StructUnion base The base struct union that this one is part of.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteWarnRawStructUnion(StructUnion su, int depth,StructUnion base)
{
 int i;
 char* splitsu=NULL;

 if(option_warn&WARN_COMMENT && depth && !su->comment)
    printf("Warning %16s : Struct/Union component '%s' in '%s' does not have a comment.\n",filename,su->name,base->name);

 splitsu=strstr(su->name,"{...}");
 if(splitsu) splitsu[-1]=0;

 if(option_raw)
   {
    for(i=0;i<depth;i++) printf("   ");
    if(depth && su->comment && !su->comps)
       printf("%s; <<<%s>>>\n",su->name,su->comment);
    else if(!depth || su->comps)
       printf("%s\n",su->name);
    else
       printf("%s;\n",su->name);
   }

 if(!depth || su->comps)
   {
    if(option_raw)
      {
       for(i=0;i<depth;i++) printf("   ");
       printf("  {\n");
      }
    for(i=0;i<su->n_comp;i++)
       WriteWarnRawStructUnion(su->comps[i],depth+1,base);
    if(option_raw)
      {
       for(i=0;i<depth;i++) printf("   ");
       printf("  }\n");
       if(splitsu)
         {
          for(i=0;i<depth;i++) printf("   ");
          if(depth && su->comment)
             printf("%s; <<<%s>>>\n",splitsu[5]?&splitsu[6]:"",su->comment);
          else
             printf("%s;\n",splitsu[5]?&splitsu[6]:"");
         }
      }
   }

 if(splitsu) splitsu[-1]=' ';
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Variable structure out.

  Variable var The Variable structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteWarnRawVariable(Variable var)
{
 int i;

 if(option_raw)
   {
    int done=0;

    printf("\nVARIABLE : %s [",var->name);
    if(var->scope&LOCAL)    done=printf("Local");
    if(var->scope&GLOBAL)   done=printf("Global");
    if(var->scope&EXTERNAL) done=printf("%sExternal",done?" and ":"");
    if(var->scope&EXTERN_H) done=printf("%sExternal from header file",done?" and ":"");
    if(var->scope&EXTERN_F)      printf("%sExternal within function",done?" and ":"");
    printf("]\n");

    if(var->comment)
       printf("<<<\n%s\n>>>\n",var->comment);
   }

 if(option_warn&WARN_COMMENT && !var->comment && (var->scope&(GLOBAL|LOCAL|EXTERNAL|EXTERN_F) || option_raw))
    printf("Warning %16s : Variable '%s' does not have a comment.\n",filename,var->name);

 if(option_raw)
    printf("Defined: %s:%d\n",var->incfrom?var->incfrom:filename,var->lineno);

 if(option_raw)
    printf("Type: %s\n",var->type);

 if(option_raw && var->incfrom)
    printf("Included from: %s\n",var->incfrom);

 if(option_xref&XREF_VAR)
   {
    if(option_raw)
      {
       if(var->scope&(EXTERNAL|EXTERN_F) && var->defined)
          printf("Declared global in '%s'\n",var->defined);

       if(var->scope&(GLOBAL|LOCAL))
         {
          for(i=0;i<var->visible->n;i++)
             if(var->visible->s1[i][0]=='$' && !var->visible->s1[i][1])
                printf("Visible in %s\n",var->visible->s2[i]);
             else
                printf("Visible in %s : %s\n",var->visible->s1[i],var->visible->s2[i]);

          for(i=0;i<var->used->n;i++)
             if(var->used->s1[i][0]=='$' && !var->used->s1[i][1])
                printf("Used in %s\n",var->used->s2[i]);
             else
                printf("Used in %s : %s\n",var->used->s1[i],var->used->s2[i]);
         }
      }

    if(option_warn&WARN_XREF)
      {
       if(var->scope&(EXTERNAL|EXTERN_F) && !var->defined)
          printf("Warning %16s : Variable '%s' has an unknown global definition.\n",filename,var->name);

       if(var->scope&(GLOBAL|LOCAL|EXTERNAL|EXTERN_F) && !var->used->n)
          printf("Warning %16s : Variable '%s' is not used anywhere.\n",filename,var->name);

       if(var->scope&(GLOBAL|EXTERNAL|EXTERN_F) && var->used->n)
         {
          int is_used_elsewhere=0,is_used_here=0;
          for(i=0;i<var->used->n;i++)
             if(!strcmp(filename,var->used->s2[i]))
                is_used_here=1;
             else
                is_used_elsewhere=1;
          if(!is_used_elsewhere)
             printf("Warning %16s : Variable '%s' is %s but only used in this file.\n",filename,var->name,var->scope&GLOBAL?"global":"extern");
          if(!is_used_here)
             printf("Warning %16s : Variable '%s' is %s but not used in this file.\n",filename,var->name,var->scope&GLOBAL?"global":"extern");
         }
      }
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Write a Function structure out.

  Function func The Function structure to output.
  ++++++++++++++++++++++++++++++++++++++*/

static void WriteWarnRawFunction(Function func)
{
 int i;

 if(option_raw)
   {
    int done=0;

    printf("\nFUNCTION : %s [",func->name);
    if(func->scope&LOCAL)    done=printf("Local");
    if(func->scope&GLOBAL)   done=printf("Global");
    if(func->scope&EXTERNAL) done=printf("External");
    if(func->scope&INLINED)       printf("%sInline",done?" and ":"");
    printf("]\n");

    if(func->comment)
       printf("<<<\n%s\n>>>\n",func->comment);
   }

 if(option_warn&WARN_COMMENT && !func->comment)
    printf("Warning %16s : Function '%s()' does not have a comment.\n",filename,func->name);

 if(option_raw)
    printf("Defined: %s:%d\n",func->incfrom?func->incfrom:filename,func->lineno);

 if(option_xref&XREF_FUNC)
   {
    if(func->protofile && option_raw)
       printf("Prototyped in %s\n",func->protofile);
    if(option_warn&WARN_XREF && !func->protofile)
       printf("Warning %16s : Function '%s()' is not prototyped.\n",filename,func->name);
   }

 if(option_raw)
   {
    if(func->cret)
       printf("Type: %s <<<%s>>>\n",func->type,func->cret);
    else
       printf("Type: %s\n",func->type);
   }
 if(option_warn&WARN_COMMENT && !func->cret && strncmp("void ",func->type,5))
    printf("Warning %16s : Function '%s()' has a return value with no comment.\n",filename,func->name);

 for(i=0;i<func->args->n;i++)
   {
    if(option_raw)
      {
       if(func->args->s2[i])
          printf("Arguments: %s <<<%s>>>\n",func->args->s1[i],func->args->s2[i]);
       else
          printf("Arguments: %s\n",func->args->s1[i]);
      }
    if(option_warn&WARN_COMMENT && !func->args->s2[i] && strcmp("void",func->args->s1[i]))
       printf("Warning %16s : Function '%s()' has an argument '%s' with no comment.\n",filename,func->name,func->args->s1[i]);
   }

 if(option_raw && func->incfrom)
    printf("Included from: %s\n",func->incfrom);

 if(option_xref&XREF_FUNC)
   {
    for(i=0;i<func->calls->n;i++)
      {
       if(option_raw)
         {
          if(func->calls->s2[i])
             printf("Calls %s : %s\n",func->calls->s1[i],func->calls->s2[i]);
          else
             printf("Calls %s\n",func->calls->s1[i]);
         }
#if 0 /* Too verbose */
       if(option_warn&WARN_XREF && !func->calls->s2[i])
          printf("Warning %16s : Function '%s()' calls function '%s()' whose definition is unknown.\n",filename,func->name,func->calls->s1[i]);
#endif
      }

    if(option_raw)
       for(i=0;i<func->called->n;i++)
          printf("Called from %s : %s\n",func->called->s1[i],func->called->s2[i]);

    if(option_raw)
       for(i=0;i<func->used->n;i++)
         {
          if(func->used->s1[i][0]=='$' && !func->used->s1[i][1])
             printf("Used in %s\n",func->used->s2[i]);
          else
             printf("Used in %s : %s\n",func->used->s1[i],func->used->s2[i]);
         }

    for(i=0;i<func->f_refs->n;i++)
      {
       if(option_raw)
         {
          if(func->f_refs->s2[i])
             printf("References Function %s : %s\n",func->f_refs->s1[i],func->f_refs->s2[i]);
          else
             printf("References Function %s\n",func->f_refs->s1[i]);
         }
       if(option_warn&WARN_XREF && !func->f_refs->s2[i])
          printf("Warning %16s : Function '%s()' references function '%s()' whose definition is unknown.\n",filename,func->name,func->f_refs->s1[i]);
      }
   }

 if(option_xref&XREF_VAR)
    for(i=0;i<func->v_refs->n;i++)
      {
       if(option_raw)
         {
          if(func->v_refs->s2[i])
             printf("References Variable %s : %s\n",func->v_refs->s1[i],func->v_refs->s2[i]);
          else
             printf("References Variable %s\n",func->v_refs->s1[i]);
         }
       if(option_warn&WARN_XREF && !func->v_refs->s2[i])
          printf("Warning %16s : Function '%s()' references variable '%s' whose definition is unknown.\n",filename,func->name,func->v_refs->s1[i]);
      }


 if(option_warn&WARN_XREF)
   {
    if(!func->used->n && !func->called->n)
       printf("Warning %16s : Function '%s()' is not used anywhere.\n",filename,func->name);

    if(func->scope&(GLOBAL|EXTERNAL) && (func->called->n || func->used->n))
      {
       int is_used_elsewhere=0;
       for(i=0;i<func->called->n;i++)
          if(strcmp(func->called->s2[i],filename))
            {is_used_elsewhere=1;break;}
       for(i=0;i<func->used->n;i++)
          if(strcmp(func->used->s2[i],filename))
            {is_used_elsewhere=1;break;}
       if(!is_used_elsewhere)
          printf("Warning %16s : Function '%s()' is global but is only used in this file.\n",filename,func->name);
      }
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Write out a raw version of the appendix.

  StringList files The list of files to write.

  StringList2 funcs The list of functions to write.

  StringList2 vars The list of variables to write.

  StringList2 types The list of types to write.
  ++++++++++++++++++++++++++++++++++++++*/

void WriteWarnRawAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types)
{
 int i;

 /* Write out the appendix of files. */

 if(option_index&INDEX_FILE)
   {
    if(files->n)
      {
       printf("\nAppendix - Files\n\n");
       for(i=0;i<files->n;i++)
          printf("%s\n",files->s[i]);
      }
    else
       if(option_warn&WARN_XREF)
          printf("Warning Index : No global files to index.\n");
   }

 /* Write out the appendix of functions. */

 if(option_index&INDEX_FUNC)
   {
    if(funcs->n)
      {
       printf("\nAppendix - Global Functions\n\n");
       for(i=0;i<funcs->n;i++)
          printf("%s : %s\n",funcs->s1[i],funcs->s2[i]);
      }
    else
       if(option_warn&WARN_XREF)
          printf("Warning Index : No global functions to index.\n");
   }

 /* Write out the appendix of variables. */

 if(option_index&INDEX_VAR)
   {
    if(vars->n)
      {
       printf("\nAppendix - Global Variables\n\n");
       for(i=0;i<vars->n;i++)
          printf("%s : %s\n",vars->s1[i],vars->s2[i]);
      }
    else
       if(option_warn&WARN_XREF)
          printf("Warning Index : No global variables to index.\n");
   }

 /* Write out the appendix of types. */

 if(option_index&INDEX_TYPE)
   {
    if(types->n)
      {
       printf("\nAppendix - Defined Types\n\n");
       for(i=0;i<types->n;i++)
          printf("%s : %s\n",types->s1[i],types->s2[i]);
      }
    else
       if(option_warn&WARN_XREF)
          printf("Warning Index : No types to index.\n");
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Decide if to copy or skip the next line.

  int CopyOrSkip Returns the number of characters to skip.

  char *string The string that starts the next line.

  char *type The type of file we are outputing.

  int *copy Returns true if we are to copy the line verbatim.

  int *skip Returns true if we are to skip the line.
  ++++++++++++++++++++++++++++++++++++++*/

int CopyOrSkip(char *string,char *type,int *copy,int *skip)
{
 char *p=string;
 int s=0;

 if(*p=='\n')
    p++;
 while(*p==' ' || *p=='\t')
    p++;

 *copy=*skip=0;

 switch(*type)
    {
    case 'h': /* html */
     if(!strncmp(p,"+html+",s=6) || !strncmp(p,"-latex-",s=7) || !strncmp(p,"-sgml-",s=6) || !strncmp(p,"-rtf-",s=5))
        *copy=1;
     if(!strncmp(p,"-html-",s=6) || !strncmp(p,"+latex+",s=7) || !strncmp(p,"+sgml+",s=6) || !strncmp(p,"+rtf+",s=5) || !strncmp(p,"+none+",s=6))
        *skip=1;
     break;
    case 'l': /* latex */
     if(!strncmp(p,"-html-",s=6) || !strncmp(p,"+latex+",s=7) || !strncmp(p,"-sgml-",s=6) || !strncmp(p,"-rtf-",s=5))
        *copy=1;
     if(!strncmp(p,"+html+",s=6) || !strncmp(p,"-latex-",s=7) || !strncmp(p,"+sgml+",s=6) || !strncmp(p,"+rtf+",s=5) || !strncmp(p,"+none+",s=6))
        *skip=1;
     break;
    case 's': /* sgml */
     if(!strncmp(p,"-html-",s=6) || !strncmp(p,"-latex-",s=7) || !strncmp(p,"+sgml+",s=6) || !strncmp(p,"-rtf-",s=5))
        *copy=1;
     if(!strncmp(p,"+html+",s=6) || !strncmp(p,"+latex+",s=7) || !strncmp(p,"-sgml-",s=6) || !strncmp(p,"+rtf+",s=5) || !strncmp(p,"+none+",s=6))
        *skip=1;
     break;
    case 'r': /* rtf */
     if(!strncmp(p,"-html-",s=6) || !strncmp(p,"-latex-",s=7) || !strncmp(p,"-sgml-",s=6) || !strncmp(p,"+rtf+",s=5))
        *copy=1;
     if(!strncmp(p,"+html+",s=6) || !strncmp(p,"+latex+",s=7) || !strncmp(p,"+sgml+",s=6) || !strncmp(p,"-rtf-",s=5) || !strncmp(p,"+none+",s=6))
        *skip=1;
     break;
    }

 if(*copy)
    return(p-string+s);
 else
    return(0);
}
