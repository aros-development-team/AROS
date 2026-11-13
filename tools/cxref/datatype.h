/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5.

  Definition of the different variables types that are used.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,99 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/


#ifndef DATA_TYPE_H
#define DATA_TYPE_H     /*+ To stop multiple inclusions. +*/

/*+ A pointer type to file information. +*/
typedef struct _File *File;

/*+ A pointer type to #include information. +*/
typedef struct _Include *Include;

/*+ A pointer type to #define information. +*/
typedef struct _Define *Define;

/*+ A pointer type to typedef information. +*/
typedef struct _Typedef *Typedef;

/*+ A pointer type to variable information. +*/
typedef struct _Variable *Variable;

/*+ A pointer type to function information. +*/
typedef struct _Function *Function;

/*+ A pointer type to struct and union information. +*/
typedef struct _StructUnion *StructUnion;

/*+ A data structure to contain lists of strings, eg functions that are called. +*/
typedef struct _StringList
{
 int    n;                              /*+ The number of strings in the list. +*/
 char** s;                              /*+ The strings. +*/
}
*StringList;

/*+ A data structure to contain two lists of strings, eg arguments and comments. +*/
typedef struct _StringList2
{
 int    n;                              /*+ The number of strings in the list. +*/
 char** s1;                             /*+ The first set of strings. +*/
 char** s2;                             /*+ The second set of strings. +*/
}
*StringList2;


/*+ A data structure to contain a complete file worth of information. +*/
struct _File
{
 char* comment;                         /*+ The file comment. +*/

 char* name;                            /*+ The name of the file. +*/

 Include includes;                      /*+ A linked list of include files. +*/

 Define defines;                        /*+ A linked list of #defines. +*/

 Typedef typedefs;                      /*+ A linked list of type definitions. +*/

 Variable variables;                    /*+ A linked list of variable definitions. +*/

 Function functions;                    /*+ A linked list of function prototypes. +*/

 StringList  inc_in;                    /*+ The files that this file is included in. +*/

 StringList2 f_refs;                    /*+ The functions that are referenced. +*/
 StringList2 v_refs;                    /*+ The variables that are referenced. +*/
};

/*+ A data structure to contain information about a #include. +*/
struct _Include
{
 char* comment;                         /*+ The comment for the include file. +*/

 char* name;                            /*+ The name of the included file. +*/

 int scope;                             /*+ The type of file, LOCAL or GLOBAL. +*/

 Include includes;                      /*+ The files that are include by this file. +*/

 Include next;                          /*+ A pointer to the next item. +*/
};

/*+ A data structure to contain information about a #define. +*/
struct _Define
{
 char* comment;                         /*+ The comment for the #define. +*/

 char* name;                            /*+ The name that is defined. +*/
 char* value;                           /*+ The value that is defined (if simple). +*/

 StringList2 args;                      /*+ The arguments to the #define function. +*/

 int lineno;                            /*+ The line number that this definition appears on. +*/

 Define next;                           /*+ A pointer to the next item. +*/
};

/*+ A data structure to contain the information for a typedef. +*/
struct _Typedef
{
 char* comment;                         /*+ The comment for the type definition. +*/

 char* name;                            /*+ The name of the defined type. +*/

 char* type;                            /*+ The type of the definition. +*/
 StructUnion sutype;                    /*+ The type of the definition if it is a locally declared struct / union. +*/
 Typedef typexref;                      /*+ The type of the definition if it is not locally declared or a repeat definition. +*/

 int lineno;                            /*+ The line number that this type definition appears on. +*/

 Typedef next;                          /*+ A pointer to the next item. +*/
};

/*+ A data structure to contain the information for a variable. +*/
struct _Variable
{
 char* comment;                         /*+ The comment for the variable. +*/

 char* name;                            /*+ The name of the variable. +*/

 char* type;                            /*+ The type of the variable. +*/

 int scope;                             /*+ The scope of the variable, STATIC, GLOBAL or EXTERNAL +*/

 char* defined;                         /*+ The name of the file that the variable is defined in as global if extern here. +*/

 char* incfrom;                         /*+ The name of the file that the variable is included from if any. +*/

 StringList2 visible;                   /*+ The names of the files that the variable is visible in. +*/
 StringList2 used;                      /*+ The names of the files that the variable is used in. +*/

 int lineno;                            /*+ The line number that this variable definition appears on. +*/

 Variable next;                         /*+ A pointer to the next item. +*/
};

/*+ A data structure to contain information for a function definition. +*/
struct _Function
{
 char* comment;                         /*+ The comment for the function. +*/

 char* name;                            /*+ The name of the function. +*/

 char* type;                            /*+ The return type of the function. +*/
 char* cret;                            /*+ A comment for the returned value. +*/

 char* protofile;                       /*+ The name of the file where the function is prototyped +*/

 char* incfrom;                         /*+ The name of the file that the function is included from if any. +*/

 StringList2 args;                      /*+ The arguments to the function. +*/

 int scope;                             /*+ The scope of the function, LOCAL or GLOBAL. +*/

 StringList  protos;                    /*+ The functions that are prototyped within this function. +*/

 StringList2 calls;                     /*+ The functions that are called from this function. +*/
 StringList2 called;                    /*+ The names of the functions that call this one. +*/
 StringList2 used;                      /*+ The places that the function is used, (references not direct calls). +*/

 StringList2 v_refs;                    /*+ The variables that are referenced from this function. +*/
 StringList2 f_refs;                    /*+ The functions that are referenced from this function. +*/

 int lineno;                            /*+ The line number that this function definition appears on. +*/

 Function next;                         /*+ A pointer to the next item. +*/
};

/*+ A data structure to contain a structure definition to allow structures to be matched to their typedefs (if any). +*/
struct _StructUnion
{
 char* comment;                         /*+ The comment for the struct or union or simple component. +*/

 char* name;                            /*+ The name of the struct or union or simple component. +*/

 int n_comp;                            /*+ The number of sub-components (if none then it is simple). +*/
 StructUnion* comps;                    /*+ The sub-components. +*/
};


/*++++++++++++++++++++++++++++++++++++++
  A function to add a pointer to the end of a linked list.

  dst The destination, where the pointer is to be added to.

  type The type of the pointer.

  src The pointer that is to be added to the end of the linked list.
  ++++++++++++++++++++++++++++++++++++++*/

#define AddToLinkedList(dst,type,src) {                               \
                                        if(dst)                       \
                                           {                          \
                                            type temp=dst;            \
                                            while(temp && temp->next) \
                                               temp=temp->next;       \
                                            temp->next=src;           \
                                           }                          \
                                        else                          \
                                           dst=src;                   \
                                      }
#endif /* DATA_TYPE_H */
