/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5e.

  Collects the comments from the parser.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,98 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

/*+ Turn on the debugging in this file. +*/
#define DEBUG 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "datatype.h"
#include "cxref.h"

static void TidyCommentString(char **string,int spaces);

/*+ The option to insert the comments verbatim into the output. +*/
extern int option_verbatim_comments;

/*+ The file that is currently being processed. +*/
extern File CurFile;

/*+ The name of the current file. +*/
extern char* parse_file;

/*+ The current (latest) comment. +*/
static char* current_comment=NULL;

/*+ The malloced string for the current comment. +*/
static char* malloc_comment=NULL;

/*+ The status of the current comment. +*/
static int comment_ended=0;


/*++++++++++++++++++++++++++++++++++++++
  Function that is called when a comment or part of one is seen. The comment is built up until an end of comment is signaled.

  char* c The comment text.

  int flag A flag to indicate the type of comment that it is.
           if flag==0 then it is a comment of some sort.
           If flag==1 then it is the end of a file (/ * * comment * * /) comment
           if flag==2 then it is the end of the other special comment (/ * + comment + * /).
           if flag==3 then it is the end of a normal comment (/ * comment * /).
  ++++++++++++++++++++++++++++++++++++++*/

void SeenComment(char* c,int flag)
{
 switch(flag)
   {
   case 1:
#if DEBUG
    printf("#Comment.c# Seen comment /**\n%s\n**/\n",current_comment);
#endif
    TidyCommentString(&current_comment,0);
    if(!CurFile->comment && !strcmp(CurFile->name,parse_file))
       SeenFileComment(current_comment);
    current_comment=NULL;
    if(malloc_comment) *malloc_comment=0;
    comment_ended=1;
    break;

   case 2:
#if DEBUG
    printf("#Comment.c# Seen comment /*+\n%s\n+*/\n",current_comment);
#endif
    TidyCommentString(&current_comment,0);
    if(SeenFuncIntComment(current_comment))
      {
       current_comment=NULL;
       if(malloc_comment) *malloc_comment=0;
      }
    comment_ended=1;
    break;

   case 3:
#if DEBUG
    printf("#Comment.c# Seen comment /*\n%s\n*/\n",current_comment);
#endif
    TidyCommentString(&current_comment,!option_verbatim_comments);
    if(!CurFile->comment && !strcmp(CurFile->name,parse_file))
      {
       SeenFileComment(current_comment);
       current_comment=NULL;
       if(malloc_comment) *malloc_comment=0;
      }
    comment_ended=1;
    break;

   default:
    if(comment_ended)
      {
       comment_ended=0;
       current_comment=NULL;
       if(malloc_comment) *malloc_comment=0;
      }

    if(malloc_comment==NULL)
      {
       malloc_comment=Malloc(strlen(c)+1);
       strcpy(malloc_comment,c);
      }
    else
      {
       malloc_comment=Realloc(malloc_comment,strlen(c)+strlen(malloc_comment)+1);
       strcat(malloc_comment,c);
      }

    current_comment=malloc_comment;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Provide the current (latest) comment.

  char* GetCurrentComment Returns the current (latest) comment.
  ++++++++++++++++++++++++++++++++++++++*/

char* GetCurrentComment(void)
{
 char* comment=current_comment;

#if DEBUG
 printf("#Comment.c# GetCurrentComment returns <<<%s>>>\n",comment);
#endif

 current_comment=NULL;

 return(comment);
}


/*++++++++++++++++++++++++++++++++++++++
  Set the current (latest) comment.

  char* comment The comment.
  ++++++++++++++++++++++++++++++++++++++*/

void SetCurrentComment(char* comment)
{
#if DEBUG
 printf("#Comment.c# SetCurrentComment set to <<<%s>>>\n",comment);
#endif

 if(comment)
   {
    if(malloc_comment!=comment)
      {
       malloc_comment=Realloc(malloc_comment,strlen(comment)+1);
       strcpy(malloc_comment,comment);
      }
    current_comment=malloc_comment;
   }
 else
   {
    current_comment=NULL;
    if(malloc_comment) *malloc_comment=0;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  A function to split out the arguments etc from a comment,
  for example the function argument comments are separated using this.

  char* SplitComment Returns the required comment.

  char** original A pointer to the original comment, this is altered in the process.

  char* name The name that is to be cut out from the comment.

  A most clever function that ignores spaces so that 'char* b' and 'char *b' match.
  ++++++++++++++++++++++++++++++++++++++*/

char* SplitComment(char** original,char* name)
{
 char* c=NULL;

 if(*original)
   {
    int l=strlen(name);
    c=*original;

    do{
       int i,j,failed=0;
       char* start=c;

       while(c[0]=='\n')
          c++;

       for(i=j=0;i<l;i++,j++)
         {
          while(name[i]==' ') i++;
          while(c[j]==' ') j++;

          if(!c[j] || name[i]!=c[j])
            {failed=1;break;}
         }

       if(!failed)
         {
          char* old=*original;
          char* end=strstr(c,"\n\n");
          *start=0;
          if(end)
             *original=MallocString(ConcatStrings(2,*original,end));
          else
             if(start==*original)
                *original=NULL;
             else
                *original=MallocString(*original);
          if(end)
             *end=0;

          if(end && &c[j+1]>=end)
             c=NULL;
          else
            {
             c=CopyString(&c[j+1]);
             TidyCommentString(&c,1);
             if(!*c)
                c=NULL;
            }

          Free(old);
          break;
         }
      }
    while((c=strstr(c,"\n\n")));
   }

 return(c);
}


/*++++++++++++++++++++++++++++++++++++++
  Tidy up the current comment string by snipping off trailing and leading junk.

  char **string The string that is to be tidied.

  int spaces Indicates that leading and trailing whitespace are to be removed as well.
  ++++++++++++++++++++++++++++++++++++++*/

static void TidyCommentString(char **string,int spaces)
{
 int whitespace;
 char *to=*string,*from=*string,*str;

 if(!*string)
    return;

 /* Remove CR characters. */

 while(*from)
   {
    if(*from=='\r')
       from++;
    else
       *to++=*from++;
   }
 *to=0;

 /* Remove leading blank lines. */

 whitespace=1;
 str=*string;
 do
   {
    if(*str!='\n')
       do
         {
          if(*str!=' ' && *str!='\t')
             whitespace=0;
         }
       while(whitespace && *str && *++str!='\n');

    if(whitespace)
       *string=++str;
    else if(spaces)
       *string=str;
   }
 while(whitespace);

 /* Remove trailing blank lines. */

 whitespace=1;
 str=*string+strlen(*string)-1;
 do
   {
    if(*str!='\n')
       do
         {
          if(*str!=' ' && *str!='\t')
             whitespace=0;
         }
       while(whitespace && str>*string && *--str!='\n');

    if(whitespace)
       *str--=0;
    else if(spaces)
       *(str+1)=0;
   }
 while(whitespace);

 /* Replace lines containing just whitespace with empty lines. */

 str=*string;
 do
   {
    char *start;

    whitespace=1;

    while(*str=='\n')
       str++;

    start=str;

    while(*str && *++str!='\n')
       {
        if(*str!=' ' && *str!='\t')
           whitespace=0;
       }

    if(whitespace)
      {
       char *copy=start;

       while((*start++=*str++));

       str=copy;
      }
   }
 while(*str);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the malloced string for the comment
  ++++++++++++++++++++++++++++++++++++++*/

void DeleteComment(void)
{
 current_comment=NULL;
 if(malloc_comment)
    Free(malloc_comment);
 malloc_comment=NULL;
 comment_ended=0;
}
