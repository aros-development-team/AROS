/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5g.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,98,99,2000,01,02,03,04 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

#include "parse-yy.h"
#include "memory.h"
#include "datatype.h"
#include "cxref.h"

/*+ The default value of the CPP command. +*/
#ifdef CXREF_CPP
#define CPP_COMMAND CXREF_CPP
#else
#define CPP_COMMAND "gcc -E -C -dD -dI"
#endif

/*+ The name of the file to read the configuration from. +*/
#define CXREF_CONFIG_FILE ".cxref"


static void Usage(int verbose);
static int ParseConfigFile(void);
static int ParseOptions(int nargs,char **args,int fromfile);

static int DocumentTheFile(char* name);
static FILE* popen_execvp(char** command);
static int pclose_execvp(FILE* f);

static char** cpp_command;              /*+ The actual cpp command that is built up, adding -D, -U and -I options. +*/
static int cpp_command_num=0;           /*+ The number of arguments to the cpp command. +*/
static int cpp_argument_num=0;          /*+ The number of arguments to the -CPP argument. +*/

/*+ The command line switch that sets the format of the output, +*/
int option_all_comments=0,              /*+ use all comments. +*/
    option_verbatim_comments=0,         /*+ insert the comments verbatim into the output. +*/
    option_block_comments=0,            /*+ remove the leading block comment marker. +*/
    option_no_comments=0,               /*+ ignore all comments. +*/
    option_xref=0,                      /*+ do cross referencing. +*/
    option_warn=0,                      /*+ produce warnings. +*/
    option_index=0,                     /*+ produce an index. +*/
    option_raw=0,                       /*+ produce raw output. +*/
    option_latex=0,                     /*+ produce LaTeX output. +*/
    option_html=0,                      /*+ produce HTML output. +*/
    option_rtf=0,                       /*+ produce RTF output. +*/
    option_sgml=0;                      /*+ produce SGML output. +*/

/*+ The option to control the mode of operation. +*/
static int option_delete=0;

/*+ The command line switch for the output name, +*/
char *option_odir=NULL,                 /*+ the directory to use. +*/
     *option_name=NULL,                 /*+ the base part of the name. +*/
     *option_root=NULL;                 /*+ the source tree root directory. +*/

/*+ The name of the include directories specified on the command line. +*/
char **option_incdirs=NULL;

/*+ The information about the cxref run, +*/
char *run_command=NULL,         /*+ the command line options. +*/
     *run_cpp_command=NULL;     /*+ the cpp command and options. +*/

/*+ The number of include directories on the command line. +*/
int option_nincdirs=0;

/*+ The names of the files to process. +*/
static char **option_files=NULL;

/*+ The number of files to process. +*/
static int option_nfiles=0;

/*+ The current file that is being processed. +*/
File CurFile=NULL;


/*++++++++++++++++++++++++++++++++++++++
  The main function that calls the parser.

  int main Returns the status, zero for normal termination, else an error.

  int argc The command line number of arguments.

  char** argv The actual command line arguments
  ++++++++++++++++++++++++++++++++++++++*/

int main(int argc,char** argv)
{
 int i;
 char *root_prefix=NULL;
 char here[PATH_MAX+1],there[PATH_MAX+1];

 if(argc==1)
    Usage(1);

 /* Setup the variables. */

 cpp_command=(char**)Malloc(8*sizeof(char*));
 cpp_command[cpp_command_num++]=MallocString(CPP_COMMAND);

 for(i=1;cpp_command[cpp_command_num-1][i];i++)
    if(cpp_command[cpp_command_num-1][i]==' ')
      {
       cpp_command[cpp_command_num-1][i]=0;
       if((cpp_command_num%8)==6)
          cpp_command=(char**)Realloc(cpp_command,(cpp_command_num+10)*sizeof(char*));
       cpp_command[cpp_command_num]=MallocString(&cpp_command[cpp_command_num-1][i+1]);
       cpp_command_num++;
       i=1;
      }

 cpp_argument_num=cpp_command_num;

 option_incdirs=(char**)Malloc(8*sizeof(char*));
 option_incdirs[0]=MallocString(".");
 option_nincdirs=1;

 option_odir=MallocString(".");

 option_name=MallocString("cxref");

 option_files=(char**)Malloc(8*sizeof(char*));

 run_command=argv[0];

 /* Parse the command line options. */

 if(ParseOptions(argc-1,&argv[1],0))
    Usage(0);

 /* Parse the options in .cxref in this directory. */

 if(ParseConfigFile())
    Usage(0);

 /* Change directory. */

 if(option_root)
   {
    if(!getcwd(there,PATH_MAX))
      {fprintf(stderr,"cxref: Error cannot get current working directory (1).\n");exit(1);}
    if(chdir(option_root))
      {fprintf(stderr,"cxref: Error cannot change directory to '%s'.\n",option_root);exit(1);}
   }

 if(!getcwd(here,PATH_MAX))
   {fprintf(stderr,"cxref: Error cannot get current working directory (2).\n");exit(1);}

 if(option_root)
   {
    if(!strcmp(here,there))
       root_prefix=".";
    else if(!strcmp(here,"/"))
       root_prefix=there+1;
    else if(!strncmp(here,there,strlen(here)))
       root_prefix=there+strlen(here)+1;
    else
      {fprintf(stderr,"cxref: Error the -R option has not specified a parent directory of the current one.\n");exit(1);}
   }

 /* Modify the -I options for the new root directory. */

 for(i=1;i<cpp_command_num;i++)
    if(cpp_command[i][0]=='-' && cpp_command[i][1]=='I')
      {
       if(cpp_command[i][2]==0)
         {
          char *old=cpp_command[++i];
          if(cpp_command[i][0]!='/' && root_prefix)
             cpp_command[i]=MallocString(CanonicaliseName(ConcatStrings(3,root_prefix,"/",cpp_command[i])));
          else if(cpp_command[i][0]=='/' && !strcmp(cpp_command[i],here))
             cpp_command[i]=MallocString(".");
          else if(cpp_command[i][0]=='/' && !strcmp(here,"/"))
             cpp_command[i]=MallocString(cpp_command[i]+1);
          else if(cpp_command[i][0]=='/' && !strncmp(cpp_command[i],here,strlen(here)))
             cpp_command[i]=MallocString(cpp_command[i]+strlen(here)+1);
          else
             cpp_command[i]=MallocString(CanonicaliseName(cpp_command[i]));
          Free(old);
         }
       else
         {
          char *old=cpp_command[i];
          if(cpp_command[i][2]!='/' && root_prefix)
             cpp_command[i]=MallocString(ConcatStrings(2,"-I",CanonicaliseName(ConcatStrings(3,root_prefix,"/",cpp_command[i]+2))));
          else if(cpp_command[i][2]=='/' && !strcmp(&cpp_command[i][2],here))
             cpp_command[i]=MallocString("-I.");
          else if(cpp_command[i][2]=='/' && !strcmp(here,"/"))
             cpp_command[i]=MallocString(ConcatStrings(2,"-I",cpp_command[i]+2+1));
          else if(cpp_command[i][2]=='/' && !strncmp(&cpp_command[i][2],here,strlen(here)))
             cpp_command[i]=MallocString(ConcatStrings(2,"-I",cpp_command[i]+2+strlen(here)+1));
          else
             cpp_command[i]=MallocString(ConcatStrings(2,"-I",CanonicaliseName(cpp_command[i]+2)));
          Free(old);
         }
      }

 for(i=0;i<option_nincdirs;i++)
   {
    char *old=option_incdirs[i];
    if(*option_incdirs[i]!='/' && root_prefix)
       option_incdirs[i]=MallocString(CanonicaliseName(ConcatStrings(3,root_prefix,"/",option_incdirs[i])));
    else if(*option_incdirs[i]=='/' && !strcmp(option_incdirs[i],here))
       option_incdirs[i]=MallocString(".");
    else if(*option_incdirs[i]=='/' && !strcmp(here,"/"))
       option_incdirs[i]=MallocString(option_incdirs[i]+strlen(here)+1);
    else if(*option_incdirs[i]=='/' && !strncmp(option_incdirs[i],here,strlen(here)))
       option_incdirs[i]=MallocString(option_incdirs[i]+strlen(here)+1);
    else
       option_incdirs[i]=MallocString(CanonicaliseName(option_incdirs[i]));
    Free(old);
   }

 /* Parse the options in .cxref in the root directory. */

 if(option_root)
    if(ParseConfigFile())
       Usage(0);

 run_command=MallocString(run_command);

 run_cpp_command=cpp_command[0];
 for(i=1;i<cpp_command_num;i++)
    run_cpp_command=ConcatStrings(3,run_cpp_command," ",cpp_command[i]);

 run_cpp_command=MallocString(run_cpp_command);

 TidyMemory();

 /* Check the options for validity */

 if(option_warn&WARN_XREF && !option_xref)
    fprintf(stderr,"cxref: Warning using '-warn-xref' without '-xref'.\n");

 /* Process each file. */

 if(option_files)
    for(i=0;i<option_nfiles;i++)
      {
       char *filename=CanonicaliseName(root_prefix?ConcatStrings(3,root_prefix,"/",option_files[i]):option_files[i]);

       if(!strncmp(filename,"../",3) || *filename=='/')
          fprintf(stderr,"cxref: Error the file %s is outside the cxref root directory.\n",filename);
       else if(!option_delete)
         {
          CurFile=NewFile(filename);

          ResetLexer();
          ResetParser();

          if(!DocumentTheFile(filename))
            {
             if(option_xref)
                CrossReference(CurFile,option_warn||option_raw||option_latex||option_html||option_rtf||option_sgml);

             if(option_raw || option_warn)
                WriteWarnRawFile(CurFile);
             if(option_latex)
                WriteLatexFile(CurFile);
             if(option_html)
                WriteHTMLFile(CurFile);
             if(option_rtf)
                WriteRTFFile(CurFile);
             if(option_sgml)
                WriteSGMLFile(CurFile);
            }

          ResetLexer();
          ResetParser();
          ResetPreProcAnalyser();
          ResetTypeAnalyser();
          ResetVariableAnalyser();
          ResetFunctionAnalyser();

          DeleteComment();

          DeleteFile(CurFile);
          CurFile=NULL;
         }
       else
         {
          CrossReferenceDelete(filename);

          WriteLatexFileDelete(filename);
          WriteHTMLFileDelete(filename);
          WriteRTFFileDelete(filename);
          WriteSGMLFileDelete(filename);
         }

       TidyMemory();
      }

 /* Create the index */

 if(option_index)
   {
    StringList files;
    StringList2 funcs,vars,types;

    files=NewStringList();
    funcs=NewStringList2();
    vars=NewStringList2();
    types=NewStringList2();

    CreateAppendix(files,funcs,vars,types);

    if(option_raw||option_warn)
       WriteWarnRawAppendix(files,funcs,vars,types);
    if(option_latex)
       WriteLatexAppendix(files,funcs,vars,types);
    if(option_html)
       WriteHTMLAppendix(files,funcs,vars,types);
    if(option_rtf)
       WriteRTFAppendix(files,funcs,vars,types);
    if(option_sgml)
       WriteSGMLAppendix(files,funcs,vars,types);

    DeleteStringList(files);
    DeleteStringList2(funcs);
    DeleteStringList2(vars);
    DeleteStringList2(types);

    TidyMemory();
   }

 /* Tidy up */

 Free(option_odir);
 Free(option_name);
 if(option_root)
    Free(option_root);

 for(i=0;i<cpp_command_num;i++)
    Free(cpp_command[i]);
 Free(cpp_command);

 for(i=0;i<option_nincdirs;i++)
    Free(option_incdirs[i]);
 Free(option_incdirs);

 for(i=0;i<option_nfiles;i++)
    Free(option_files[i]);
 Free(option_files);

 Free(run_command);
 Free(run_cpp_command);

 PrintMemoryStatistics();

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Print out the usage instructions.

  int verbose If true then output a long version of the information.
  ++++++++++++++++++++++++++++++++++++++*/

static void Usage(int verbose)
{
 fputs("\n"
       "              C Cross Referencing & Documenting tool - Version 1.5g\n"
       "              -----------------------------------------------------\n"
       "\n"
       "(c) Andrew M. Bishop 1995,96,97,98,99, [       amb@gedanken.demon.co.uk       ]\n"
       "                     2000,01,02,03,04  [http://www.gedanken.demon.co.uk/cxref/]\n"
       "\n"
       "Usage: cxref filename [ ... filename]\n"
       "             [-Odirname] [-Nbasename] [-Rdirname]\n"
       "             [-all-comments] [-no-comments]\n"
       "             [-verbatim-comments] [-block-comments]\n"
       "             [-xref[-all][-file][-func][-var][-type]]\n"
       "             [-warn[-all][-comment][-xref]]\n"
       "             [-index[-all][-file][-func][-var][-type]]\n"
       "             [-latex209|-latex2e] [-html20|-html32] [-rtf] [-sgml] [-raw]\n"
       "             [-Idirname] [-Ddefine] [-Udefine]\n"
       "             [-CPP cpp_program] [-- cpp_arg [ ... cpp_arg]]\n"
       "\n"
       "Usage: cxref filename [ ... filename] -delete\n"
       "             [-Odirname] [-Nbasename] [-Rdirname]\n"
       "\n",
       stderr);

 if(verbose)
    fputs("filename ...           : Files to document.\n"
          "-delete                : Delete all references to the named files.\n"
          "\n"
          "-Odirname              : The output directory for the documentation.\n"
          "-Nbasename             : The base filename for the output documentation.\n"
          "-Rdirname              : The root directory of the source tree.\n"
          "\n"
          "-all-comments          : Use all comments.\n"
          "-verbatim-comments     : Insert the comments verbatim in the output.\n"
          "-block-comments        : The comments are in block style.\n"
          "-no-comments           : Ignore all of the comments.\n"
          "\n"
          "-xref[-*]              : Do cross referencing (of specified types).\n"
          "-warn[-*]              : Produce warnings (of comments or cross references).\n"
          "\n"
          "-index[-*]             : Produce a cross reference index (of specified types).\n"
          "\n"
          "-latex209 | -latex2e   : Produce LaTeX output (version 2.09 or 2e - default=2e).\n"
          "-html20 | -html32      : Produce HTML output (version 2.0 or 3.2 - default=3.2).\n"
          "-rtf                   : Produce RTF output (version 1.x).\n"
          "-sgml                  : Produce SGML output (for SGML tools version 1.0.x).\n"
          "-raw                   : Produce raw output .\n"
          "\n"
          "-I*, -D*, -U*          : The usual compiler switches.\n"
          "-CPP cpp_program       : The cpp program to use.\n"
          "                       : (default '" CPP_COMMAND "')\n"
          "-- cpp_arg ...         : All arguments after the '--' are passed to cpp.\n"
          "\n"
          "The file .cxref in the current directory can also contain any of these arguments\n"
          "one per line, (except for filename and -delete).\n",
          stderr);
 else
    fputs("Run cxref with no arguments to get more verbose help\n",
          stderr);

 exit(1);
}


/*++++++++++++++++++++++++++++++++++++++
  Read in the options from the configuration file.

  int ParseConfigFile Returns the value returned by ParseOptions().
  ++++++++++++++++++++++++++++++++++++++*/

static int ParseConfigFile(void)
{
 FILE *file=fopen(CXREF_CONFIG_FILE,"r");
 char **lines=NULL;
 int nlines=0;
 char data[257];

 if(file)
   {
    while(fgets(data,256,file))
      {
       char *d=data+strlen(data)-1;

       if(*data=='#')
          continue;

       while(d>=data && (*d=='\r' || *d=='\n' || *d==' '))
          *d--=0;

       if(d<data)
          continue;

       if(!lines)
          lines=(char**)Malloc(8*sizeof(char*));
       else if((nlines%8)==7)
          lines=(char**)Realloc(lines,(nlines+9)*sizeof(char*));

       if((!strncmp(data,"-I",2) || !strncmp(data,"-D",2) || !strncmp(data,"-U",2) ||
           !strncmp(data,"-O",2) || !strncmp(data,"-N",2) || !strncmp(data,"-R",2)) &&
          (data[2]==' ' || data[2]=='\t'))
         {
          int i=2;
          while(data[i]==' ' || data[i]=='\t')
             data[i++]=0;
          lines[nlines++]=CopyString(data);
          lines[nlines++]=CopyString(data+i);
         }
       else if(!strncmp(data,"-CPP",4) &&
               (data[4]==' ' || data[4]=='\t'))
         {
          int i=4;
          while(data[i]==' ' || data[i]=='\t')
             data[i++]=0;
          lines[nlines++]=CopyString(data);
          lines[nlines++]=CopyString(data+i);
         }
       else
          if(*data)
             lines[nlines++]=CopyString(data);
      }

    if(nlines)
      {
       int n_files=option_nfiles;

       if(ParseOptions(nlines,lines,1))
         {
          fprintf(stderr,"cxref: Error parsing the .cxref file\n");
          return(1);
         }

       Free(lines);

       if(n_files!=option_nfiles)
         {
          for(;n_files<option_nfiles;n_files++)
             fprintf(stderr,"cxref: File names '%s' only allowed on command line.\n",option_files[n_files]);
          return(1);
         }
      }

    fclose(file);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Parse the options from the command line or from the .cxref file.

  int ParseOptions Return 1 if there is an error.

  int nargs The number of arguments.

  char **args The actual arguments

  int fromfile A flag indicating that they are read from the .cxref file.
  ++++++++++++++++++++++++++++++++++++++*/

static int ParseOptions(int nargs,char **args,int fromfile)
{
 int i,end_of_args=0;

 for(i=0;i<nargs;i++)
   {
    if(end_of_args)
      {
       if((cpp_command_num%8)==6)
          cpp_command=(char**)Realloc(cpp_command,(cpp_command_num+10)*sizeof(char*));
       cpp_command[cpp_command_num++]=MallocString(args[i]);
       run_command=ConcatStrings(3,run_command," ",args[i]);
       continue;
      }

    if(!strncmp(args[i],"-I",2) || !strncmp(args[i],"-D",2) || !strncmp(args[i],"-U",2))
      {
       char *incdir=NULL;
       if((cpp_command_num%8)==6)
          cpp_command=(char**)Realloc(cpp_command,(cpp_command_num+10)*sizeof(char*));
       cpp_command[cpp_command_num++]=MallocString(args[i]);
       if(args[i][2]==0)
         {
          if(args[i][1]=='I')
             incdir=args[i+1];
          if(i==nargs-1)
            {fprintf(stderr,"cxref: The -%c option requires a following argument.\n",args[i][1]);return(1);}
          if((cpp_command_num%8)==6)
             cpp_command=(char**)Realloc(cpp_command,(cpp_command_num+10)*sizeof(char*));
          run_command=ConcatStrings(3,run_command," ",args[i]);
          cpp_command[cpp_command_num++]=MallocString(args[++i]);
         }
       else
          if(args[i][1]=='I')
             incdir=&args[i][2];

       if(incdir)
         {
          if((option_nincdirs%8)==0)
             option_incdirs=(char**)Realloc(option_incdirs,(option_nincdirs+8)*sizeof(char*));
          option_incdirs[option_nincdirs++]=MallocString(incdir);
         }

       run_command=ConcatStrings(3,run_command," ",args[i]);
       continue;
      }

    if(!strcmp(args[i],"-CPP"))
      {
       char **old=cpp_command,*command;
       int j,old_com_num=cpp_command_num,old_arg_num=cpp_argument_num;

       if(i==nargs-1)
         {fprintf(stderr,"cxref: The -CPP option requires a following argument.\n");return(1);}
       command=args[++i];

       cpp_command_num=0;
       cpp_command=(char**)Malloc(8*sizeof(char*));
       cpp_command[cpp_command_num++]=MallocString(command);

       for(j=1;cpp_command[cpp_command_num-1][j];j++)
          if(cpp_command[cpp_command_num-1][j]==' ')
            {
             cpp_command[cpp_command_num-1][j]=0;
             if((cpp_command_num%8)==6)
                cpp_command=(char**)Realloc(cpp_command,(cpp_command_num+10)*sizeof(char*));
             cpp_command[cpp_command_num]=MallocString(&cpp_command[cpp_command_num-1][j+1]);
             cpp_command_num++;
             j=1;
            }

       cpp_argument_num=cpp_command_num;

       for(j=old_arg_num;j<old_com_num;j++)
         {
          if((cpp_command_num%8)==6)
             cpp_command=(char**)Realloc(cpp_command,(cpp_command_num+10)*sizeof(char*));
          cpp_command[cpp_command_num++]=old[j];
         }

       for(j=0;j<old_arg_num;j++)
          Free(old[j]);
       Free(old);

       run_command=ConcatStrings(4,run_command,"-CPP \"",args[i],"\"");
       continue;
      }

    if(!strncmp(args[i],"-O",2))
      {
       if(option_odir)
          Free(option_odir);
       if(args[i][2]==0)
         {
          if(i==nargs-1)
            {fprintf(stderr,"cxref: The -O option requires a following argument.\n");return(1);}
          run_command=ConcatStrings(3,run_command," ",args[i]);
          option_odir=MallocString(args[++i]);
         }
       else
          option_odir=MallocString(&args[i][2]);
       run_command=ConcatStrings(3,run_command," ",args[i]);
       continue;
      }

    if(!strncmp(args[i],"-N",2))
      {
       if(option_name)
          Free(option_name);
       if(args[i][2]==0)
         {
          if(i==nargs-1)
            {fprintf(stderr,"cxref: The -N option requires a following argument.\n");return(1);}
          run_command=ConcatStrings(3,run_command," ",args[i]);
          option_name=MallocString(args[++i]);
         }
       else
          option_name=MallocString(&args[i][2]);
       run_command=ConcatStrings(3,run_command," ",args[i]);
       continue;
      }

    if(!strncmp(args[i],"-R",2))
      {
       if(option_root)
          Free(option_root);
       if(args[i][2]==0)
         {
          if(i==nargs-1)
            {fprintf(stderr,"cxref: The -R option requires a following argument.\n");return(1);}
          run_command=ConcatStrings(3,run_command," ",args[i]);
          option_root=MallocString(args[++i]);
         }
       else
          option_root=MallocString(&args[i][2]);
       if(*option_root=='.' && !*(option_root+1))
          option_root=NULL;
       run_command=ConcatStrings(3,run_command," ",args[i]);
       continue;
      }

    if(!strcmp(args[i],"-delete"))
      {if(fromfile) {fprintf(stderr,"cxref: The -delete option cannot be used in the .cxref file.\n");return(1);}
       option_delete=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strcmp(args[i],"-all-comments"))
      {option_all_comments=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strcmp(args[i],"-verbatim-comments"))
      {option_verbatim_comments=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strcmp(args[i],"-block-comments"))
      {option_block_comments=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strcmp(args[i],"-no-comments"))
      {option_no_comments=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strncmp(args[i],"-xref",5))
      {
       char* p=&args[i][5];

       if(!*p)
          option_xref=XREF_ALL;
       else
          while(*p)
            {
             if(!strncmp(p,"-all" ,4)) {option_xref|=XREF_ALL ; p=&p[4]; continue;}
             if(!strncmp(p,"-file",5)) {option_xref|=XREF_FILE; p=&p[5]; continue;}
             if(!strncmp(p,"-func",5)) {option_xref|=XREF_FUNC; p=&p[5]; continue;}
             if(!strncmp(p,"-var" ,4)) {option_xref|=XREF_VAR ; p=&p[4]; continue;}
             if(!strncmp(p,"-type",5)) {option_xref|=XREF_TYPE; p=&p[5]; continue;}
             break;
            }

       run_command=ConcatStrings(3,run_command," ",args[i]);
       continue;
      }

    if(!strncmp(args[i],"-warn",5))
      {
       char* p=&args[i][5];

       if(!*p)
          option_warn=WARN_ALL;
       else
          while(*p)
            {
             if(!strncmp(p,"-all"    ,4)) {option_warn|=WARN_ALL    ; p=&p[4]; continue;}
             if(!strncmp(p,"-comment",8)) {option_warn|=WARN_COMMENT; p=&p[8]; continue;}
             if(!strncmp(p,"-xref"   ,5)) {option_warn|=WARN_XREF   ; p=&p[5]; continue;}
             break;
            }

       run_command=ConcatStrings(3,run_command," ",args[i]);
       continue;
      }

    if(!strncmp(args[i],"-index",6))
      {
       char* p=&args[i][6];

       if(!*p)
          option_index=INDEX_ALL;
       else
          while(*p)
            {
             if(!strncmp(p,"-all" ,4)) {option_index|=INDEX_ALL ; p=&p[4]; continue;}
             if(!strncmp(p,"-file",5)) {option_index|=INDEX_FILE; p=&p[5]; continue;}
             if(!strncmp(p,"-func",5)) {option_index|=INDEX_FUNC; p=&p[5]; continue;}
             if(!strncmp(p,"-var" ,4)) {option_index|=INDEX_VAR ; p=&p[4]; continue;}
             if(!strncmp(p,"-type",5)) {option_index|=INDEX_TYPE; p=&p[5]; continue;}
             break;
            }

       run_command=ConcatStrings(3,run_command," ",args[i]);
       continue;
      }

    if(!strcmp(args[i],"-raw"))
      {option_raw=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strcmp(args[i],"-latex209"))
      {option_latex=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}
    if(!strcmp(args[i],"-latex2e") || !strcmp(args[i],"-latex"))
      {option_latex=2; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strncmp(args[i],"-html20",7))
      {option_html=1; if(!strcmp(args[i]+7,"-src"))option_html+=16;
       run_command=ConcatStrings(3,run_command," ",args[i]); continue;}
    if(!strncmp(args[i],"-html32",7))
      {option_html=2; if(!strcmp(args[i]+7,"-src"))option_html+=16;
       run_command=ConcatStrings(3,run_command," ",args[i]); continue;}
    if(!strncmp(args[i],"-html",5))
      {option_html=2; if(!strcmp(args[i]+5,"-src"))option_html+=16;
       run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strcmp(args[i],"-rtf"))
      {option_rtf=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strcmp(args[i],"-sgml"))
      {option_sgml=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(!strcmp(args[i],"--"))
      {end_of_args=1; run_command=ConcatStrings(3,run_command," ",args[i]); continue;}

    if(args[i][0]=='-')
      {fprintf(stderr,"cxref: Unknown option '%s'.\n",args[i]);return(1);}

    if(fromfile)
      {fprintf(stderr,"cxref: File names '%s' only allowed on command line.\n",args[i]);return(1);}

    if(option_files && (option_nfiles%8)==0)
       option_files=(char**)Realloc(option_files,(option_nfiles+8)*sizeof(char*));
    option_files[option_nfiles++]=MallocString(args[i]);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Canonicalise a file name by removing '/../', '/./' and '//' references.

  char *CanonicaliseName Returns the argument modified.

  char *name The original name

  The same function is used in WWWOFFLE and cxref with changes for files or URLs.
  ++++++++++++++++++++++++++++++++++++++*/

char *CanonicaliseName(char *name)
{
 char *match,*name2;

 match=name;
 while((match=strstr(match,"/./")) || !strncmp(match=name,"./",2))
   {
    char *prev=match, *next=match+2;
    while((*prev++=*next++));
   }

 match=name;
 while((match=strstr(match,"//")))
   {
    char *prev=match, *next=match+1;
    while((*prev++=*next++));
   }

 match=name2=name;
 while((match=strstr(match,"/../")))
   {
    char *prev=match, *next=match+4;
    if((prev-name2)==2 && !strncmp(name2,"../",3))
      {name2+=3;match++;continue;}
    while(prev>name2 && *--prev!='/');
    match=prev;
    if(*prev=='/')prev++;
    while((*prev++=*next++));
   }

 match=&name[strlen(name)-2];
 if(match>=name && !strcmp(match,"/."))
    *match=0;

 match=&name[strlen(name)-3];
 if(match>=name && !strcmp(match,"/.."))
   {
    if(match==name)
       *++match=0;
    else
       while(match>name && *--match!='/')
          *match=0;
   }

#if 1 /* as used in cxref */

 match=&name[strlen(name)-1];
 if(match>name && !strcmp(match,"/"))
    *match=0;

 if(!*name)
    *name='.',*(name+1)=0;

#else /* as used in wwwoffle */

 if(!*name || !strncmp(name,"../",3))
    *name='/',*(name+1)=0;

#endif

 return(name);
}


/*++++++++++++++++++++++++++++++++++++++
  Calls CPP for the file to get all of the needed information.

  int DocumentTheFile Returns 1 in case of error, else 0.

  char* name The name of the file to document.

  The CPP is started as a sub-process, (using popen to return a FILE* for lex to use).
  ++++++++++++++++++++++++++++++++++++++*/

static int DocumentTheFile(char* name)
{
 struct stat stat_buf;
 int error1,error2;
 static int first=1;

 if(stat(name,&stat_buf)==-1)
   {fprintf(stderr,"cxref: Cannot access the file '%s'\n",name);return(1);}

 cpp_command[cpp_command_num  ]=name;
 cpp_command[cpp_command_num+1]=NULL;

 yyin=popen_execvp(cpp_command);

 if(!yyin)
   {fprintf(stderr,"cxref: Failed to start the cpp command '%s'\n",cpp_command[0]);exit(1);}

 if(!first)
    yyrestart(yyin);
 first=0;

#if YYDEBUG
 yydebug=(YYDEBUG==3);
#endif

 error1=yyparse();

 error2=pclose_execvp(yyin);

 if(error2)
    fprintf(stderr,"cxref: The preprocessor exited abnormally on '%s'\n",name);

 return(error1||error2);
}


/*+ The process id of the pre-processor. +*/
static pid_t popen_pid;

/*++++++++++++++++++++++++++++++++++++++
  A popen function that takes a list of arguments not a string.

  FILE* popen_execvp Returns a file descriptor.

  char** command The command arguments.
  ++++++++++++++++++++++++++++++++++++++*/

static FILE* popen_execvp(char** command)
{
 int fdr[2];

 if(pipe(fdr)==-1)
   {fprintf(stderr,"cxref: Can not pipe for the cpp command '%s'.\n",command[0]);exit(1);}

 if((popen_pid=fork())==-1)
   {fprintf(stderr,"cxref: Can not fork for the cpp command '%s.\n",command[0]);exit(1);}

 if(popen_pid)                   /* The parent */
   {
    close(fdr[1]);
   }
 else                            /* The child */
   {
    close(1);
    dup(fdr[1]);
    close(fdr[1]);

    close(fdr[0]);

    execvp(command[0],command);
    fprintf(stderr,"cxref: Can not execvp for the cpp command '%s', is it on the path?\n",command[0]);
    exit(1);
   }

 return(fdopen(fdr[0],"r"));
}


/*++++++++++++++++++++++++++++++++++++++
  Close the file to the to the preprocessor

  int pclose_execvp Return the error status.

  FILE* f The file to close.
  ++++++++++++++++++++++++++++++++++++++*/

static int pclose_execvp(FILE* f)
{
 int status,ret;

 waitpid(popen_pid,&status,0);
 fclose(f);

 if(WIFEXITED(status))
    ret=WEXITSTATUS(status);
 else
    ret=-1;

 return(ret);
}
