/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.4.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96 Andrew M. Bishop
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
int option_xref=0;

/*+ The command line switch for the output name, +*/
char *option_odir=".",          /*+ The directory to use. +*/
     *option_name="cxref";      /*+ The base part of the name. +*/

File *files=NULL;               /*+ The files that are queried. +*/
int n_files=0;                  /*+ The number of files referenced. +*/

Function *functions=NULL;       /*+ The functions that are queried. +*/
int n_functions=0;              /*+ The number of functions referenced. +*/

Variable *variables=NULL;       /*+ The variables that are queried. +*/
int n_variables=0;              /*+ The number of variables referenced. +*/

Typedef *typedefs=NULL;         /*+ The type definitions that are queried. +*/
int n_typedefs=0;               /*+ The number of typedefs referenced. +*/


/*++++++++++++++++++++++++++++++++++++++
  The main function that does it all.

  int main Returns the status, zero for normal termination, else an error.

  int argc The command line number of arguments.

  char** argv The actual command line arguments
  ++++++++++++++++++++++++++++++++++++++*/

int main(int argc,char** argv)
{
 int i,args=0;

 if(argc==1)
   {
    fputs("Usage: cxref-query [name [ ... name]]                       ; Names of objects to query.\n"
          "                   [-Odirname]                              ; Use dirname as the input directory\n"
          "                   [-Nbasename]                             ; Use basename.* as the input filenames\n"
          "                   [-xref[-all][-file][-func][-var][-type]] ; Use cross reference files (default -xref-all).\n"
          ,stderr);
    exit(1);
   }

 for(i=1;i<argc;i++)
   {
    if(!strncmp(argv[i],"-O",2))
      {
       if(argv[i][2]==0)
         {
          argv[i++]=NULL;
          if(i==argc)
            {fprintf(stderr,"cxref-query: The -O option requires a following argument.\n");return(1);}
          option_odir=argv[i];
         }
       else
          option_odir=&argv[i][2];
       argv[i]=NULL;
       continue;
      }

    if(!strncmp(argv[i],"-N",2))
      {
       if(argv[i][2]==0)
         {
          argv[i++]=NULL;
          if(i==argc)
            {fprintf(stderr,"cxref-query: The -N option requires a following argument.\n");return(1);}
          option_name=argv[i];
         }
       else
          option_name=&argv[i][2];
       argv[i]=NULL;
       continue;
      }

    if(!strncmp(argv[i],"-xref",5))
      {
       char* p=&argv[i][5];

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
       argv[i]=NULL;continue;
      }

    args++;
    if(!strncmp(argv[i],"./",2))
       argv[i]+=2;
   }

 LoadInCrossRefs();

 if(args)
   {
    for(i=1;i<argc;i++)
       if(argv[i])
         {
          printf("cxref-query> %s\n\n",argv[i]);
          OutputCrossRef(argv[i]);
         }
   }
 else
   {
    while(1)
      {
       char input[128];
       printf("cxref-query> ");
       if(!fgets(input,128,stdin))
         {printf("\n\n");break;}

       printf("\n");
       input[strlen(input)-1]=0;
       OutputCrossRef(input);
      }
   }

 PrintMemoryStatistics();

 return(0);
}
