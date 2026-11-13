/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5c.

  Prototypes for general functions.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,98,99 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/


#ifndef CXREF_H
#define CXREF_H  /*+ To stop multiple inclusions. +*/

#include "datatype.h"

/* Definitions for variable / function / file types */

#define LOCAL     1      /*+ Signifies a LOCAL function / variable / include file. +*/
#define GLOBAL    2      /*+ Signifies a GLOBAL fuction / variable / include file. +*/
#define EXTERNAL  4      /*+ Signifies an EXTERNAL variable. +*/
#define EXTERN_H  8      /*+ Signifies an EXTERNAL variable seen in a header file. +*/
#define EXTERN_F 16      /*+ Signifies an EXTERNAL variable seen in a function file. +*/
#define INLINED  32      /*+ Signifies an INLINED function. +*/

/* Definitions for -xref options */

#define XREF_FILE   1    /*+ Signifies that file cross references are required. +*/
#define XREF_FUNC   2    /*+ Signifies that function cross references are required. +*/
#define XREF_VAR    4    /*+ Signifies that variable cross references are required. +*/
#define XREF_TYPE   8    /*+ Signifies that type definition cross references are required. +*/
#define XREF_ALL   15    /*+ Signifies that all the above cross references are required. +*/

/* Definitions for -warn options */

#define WARN_COMMENT  1  /*+ Signifies that warnings for commetns are required. +*/
#define WARN_XREF     2  /*+ Signifies that warnings for cross references are required. +*/
#define WARN_ALL      3  /*+ Signifies that all of the above warnings are required. +*/

/* Definitions for -index options */

#define INDEX_FILE   1   /*+ Signifies that an index of files is needed. +*/
#define INDEX_FUNC   2   /*+ Signifies that an index of global functions is needed. +*/
#define INDEX_VAR    3   /*+ Signifies that an index of global variables is needed. +*/
#define INDEX_TYPE   4   /*+ Signifies that an index of type definitions is needed. +*/
#define INDEX_ALL   15   /*+ Signifies that a complete index of all of the above is needed. +*/

/* in cxref.c */

char *CanonicaliseName(char *name);

/* In parse.l */

void ResetLexer(void);

/* In parse.y */

void ResetParser(void);

/* in file.c */

File NewFile(char* name);
void DeleteFile(File file);

void SeenFileComment(char* comment);

/* in comment.c */

void SeenComment(char* c,int flag);
char* GetCurrentComment(void);
void SetCurrentComment(char* comment);
char* SplitComment(char** original,char* name);
void DeleteComment(void);

/* in preproc.c */

void SeenInclude(char *name);
void SeenIncludeComment(void);
char *SeenFileChange(char *name,int flag);

void SeenDefine(char* name);
void SeenDefineComment(void);
void SeenDefineValue(char* value);
void SeenDefineFunctionArg(char* name);
void SeenDefineFuncArgComment(void);

void ResetPreProcAnalyser(void);
void DeleteIncludeType(Include inc);
void DeleteDefineType(Define inc);

/* in type.c */

void SeenTypedefName(char* name,int what_type);
int IsATypeName(char* name);
void SeenTypedef(char* name,char* type);

void SeenStructUnionStart(char* name);
void SeenStructUnionComp(char* name,int depth);
void SeenStructUnionEnd(void);

void ResetTypeAnalyser(void);
void DeleteTypedefType(Typedef type);

/* in var.c */

void SeenVariableDefinition(char* name,char* type,int scope);

void UpScope(void);
void DownScope(void);
void SeenScopeVariable(char* name);
int IsAScopeVariable(char* name);

void ResetVariableAnalyser(void);
void DeleteVariableType(Variable var);

/* in func.c */

void SeenFunctionProto(char* name,int in_a_function);

void SeenFunctionDeclaration(char* name,int scope);
void SeenFunctionDefinition(char* type);
void SeenFunctionArg(char* name,char* type);

void SeenFunctionCall(char* name);
void CheckFunctionVariableRef(char* name,int in_a_function);

int SeenFuncIntComment(char* comment);

void ResetFunctionAnalyser(void);
void DeleteFunctionType(Function func);

/* In slist.c */

StringList NewStringList(void);
void AddToStringList(StringList sl,char* str,int alphalist,int uniqlist);
void DeleteStringList(StringList sl);

StringList2 NewStringList2(void);
void AddToStringList2(StringList2 sl,char* str1,char* str2,int alphalist,int uniqlist);
void DeleteStringList2(StringList2 sl);

/* In xref.c */

void CrossReference(File file,int outputs);
void CreateAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types);

void CrossReferenceDelete(char *name);

/* In warn-raw.c */

void WriteWarnRawFile(File file);
void WriteWarnRawAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types);

int CopyOrSkip(char *string,char *type,int *copy,int *skip);

/* In latex.c */

void WriteLatexFile(File file);
void WriteLatexAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types);

void WriteLatexFileDelete(char *name);

/* In html.c */

void WriteHTMLFile(File file);
void WriteHTMLAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types);

void WriteHTMLFileDelete(char *name);

/* In rtf.c */

void WriteRTFFile(File file);
void WriteRTFAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types);

void WriteRTFFileDelete(char *name);

/* In sgml.c */

void WriteSGMLFile(File file);
void WriteSGMLAppendix(StringList files,StringList2 funcs,StringList2 vars,StringList2 types);

void WriteSGMLFileDelete(char *name);


/* Not defined on Suns */

#if defined(__sun__) && !defined(__svr4__)
#include <stdio.h>

int fputs(const char *s, FILE *stream);
int fprintf(FILE*, const char*,...);
int pclose( FILE *stream);
int fscanf( FILE *stream, const char *format, ...);
int fclose(FILE*);
int rename(const char *oldpath, const char *newpath);
int printf(const char*,...);
int fwrite(const void*,unsigned int,unsigned int, FILE*);
int fread(void*,unsigned int,unsigned int, FILE*);
int isatty(int desc);
int fflush( FILE *stream);
#endif /* Suns */

#endif /* CXREF_H */
