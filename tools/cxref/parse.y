%{
/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5g.

  C parser.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,98,99,2000,01,02,03,04 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

#include <string.h>
#include "parse-yy.h"
#include "cxref.h"
#include "memory.h"

/*+ A structure to hold the information about an object. +*/
typedef struct _stack
{
 char *name;                    /*+ The name of the object. +*/
 char *type;                    /*+ The type of the object. +*/
 char *qual;                    /*+ The type qualifier of the object. +*/
}
stack;

#define yylex cxref_yylex

static int cxref_yylex(void);

static void yyerror(char *s);

/*+ When in a header file, some stuff can be skipped over quickly. +*/
extern int in_header;

/*+ A flag that is set to true when typedef is seen in a statement. +*/
int in_typedef=0;

/*+ The scope of the function / variable that is being examined. +*/
static int scope;

/*+ The variable must be LOCAL or EXTERNAL or GLOBAL, so this checks and sets that. +*/
#define SCOPE ( scope&(LOCAL|EXTERNAL|EXTERN_H|EXTERN_F) ? scope : scope|GLOBAL )

/*+ When in a function or a function definition, the behaviour is different. +*/
static int in_function=0,in_funcdef=0,in_funcbody=0;

/*+ The parsing stack +*/
static stack first={NULL,NULL,NULL},  /*+ first value. +*/
            *list=NULL,               /*+ list of all values. +*/
            *current=&first;          /*+ current values. +*/

/*+ The depth of the stack +*/
static int depth=0,             /*+ currently in use. +*/
           maxdepth=0;          /*+ total malloced. +*/

/*+ Declarations that are in the same statement share this comment. +*/
static char* common_comment=NULL;

/*+ When inside a struct / union / enum definition, this is the depth. +*/
static int in_structunion=0;

/*+ When inside a struct / union definition, this is the component type. +*/
static char *comp_type=NULL;

/*+ To solve the problem where a type name is used as an identifier. +*/
static int in_type_spec=0;


/*++++++++++++++++++++++++++++++++++++++
  Reset the current level on the stack.
  ++++++++++++++++++++++++++++++++++++++*/

static void reset(void)
{
 current->name=NULL;
 current->type=NULL;
 current->qual=NULL;
}


/*++++++++++++++++++++++++++++++++++++++
  Push a level onto the stack.
  ++++++++++++++++++++++++++++++++++++++*/

static void push(void)
{
 if(list==NULL)
   {
    list=(stack*)Malloc(8*sizeof(struct _stack));
    list[0]=first;
    maxdepth=8;
   }
 else if(depth==(maxdepth-1))
   {
    list=Realloc(list,(maxdepth+8)*sizeof(struct _stack));
    maxdepth+=8;
   }

 depth++;
 current=&list[depth];

 reset();
}


/*++++++++++++++++++++++++++++++++++++++
  Pop a level from the stack.
  ++++++++++++++++++++++++++++++++++++++*/

static void pop(void)
{
 reset();

 depth--;
 current=&list[depth];
}


/*++++++++++++++++++++++++++++++++++++++
  Reset the Parser, ready for the next file.
  ++++++++++++++++++++++++++++++++++++++*/

void ResetParser(void)
{
 in_typedef=0;
 scope=0;
 in_function=0;
 in_funcdef=0;
 in_funcbody=0;
 depth=0;
 maxdepth=0;
 if(list) Free(list);
 list=NULL;
 current=&first;
 reset();
 common_comment=NULL;
 in_structunion=0;
 comp_type=NULL;
 in_type_spec=0;
}

%}

/* Expected conflicts: 19 shift/reduce */

%token IDENTIFIER TYPE_NAME LITERAL STRING_LITERAL ELLIPSES
%token MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%token EQ_OP NE_OP PTR_OP AND_OP OR_OP DEC_OP INC_OP LE_OP GE_OP
%token LEFT_SHIFT RIGHT_SHIFT
%token SIZEOF
%token TYPEDEF EXTERN STATIC AUTO REGISTER CONST VOLATILE VOID INLINE
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE
%token STRUCT UNION ENUM
%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN
%token ASM

%start file

%%

/*-------------------- Top level --------------------*/

file
	: /* Empty */
	| program
	;

program
	: top_level_declaration
	| program top_level_declaration
	;

top_level_declaration
	: declaration
                { scope=0; reset(); common_comment=NULL; in_typedef=0; GetCurrentComment(); }
	| function_definition
                { scope=0; reset(); common_comment=NULL; in_typedef=0; GetCurrentComment(); }
	| asm_statement         /*+ GNU Extension +*/
	| null_statement
	;

/*-------------------- Declarations --------------------*/

declaration_list
	: declaration
                { scope=0; reset(); common_comment=NULL; in_typedef=0; }
	| declaration_list declaration
                { scope=0; reset(); common_comment=NULL; in_typedef=0;
                  $$=$2; }
	;

declaration
	: declaration_specifiers initialized_declarator_list ';'
                { in_type_spec=0; }
	| declaration_specifiers ';'
                { in_type_spec=0; }
	;

declaration_specifiers
    	: declaration_specifiers1
                { if(!in_structunion && !in_typedef && !in_function && !common_comment)
                  {common_comment=CopyString(GetCurrentComment()); SetCurrentComment(common_comment);} }
	;

declaration_specifiers1
	: storage_class_specifier
	| storage_class_specifier declaration_specifiers1
                { if($1) $$=ConcatStrings(3,$1," ",$2); else $$=$2; }
	| type_specifier
                { if(!current->type) current->type=$1; }
	| type_specifier declaration_specifiers1
                { if(!current->type) current->type=$1;
                  $$=ConcatStrings(3,$1," ",$2); }
	| type_qualifier
	| type_qualifier declaration_specifiers1
                { $$=ConcatStrings(3,$1," ",$2); }
	;

/* Initialised declarator list */

initialized_declarator_list
	: initialized_declarator
	| initialized_declarator_list ',' { in_type_spec=1; } initialized_declarator
	;

initialized_declarator
	: initialized_declarator1
                {
                 if((in_function==0 || in_function==3) && !in_funcdef && !in_structunion)
                   {
                    char* specific_comment=GetCurrentComment();
                    if(!common_comment)   SetCurrentComment(specific_comment); else
                    if(!specific_comment) SetCurrentComment(common_comment);   else
                    if(strcmp(common_comment,specific_comment)) SetCurrentComment(ConcatStrings(3,common_comment," ",specific_comment)); else
                                          SetCurrentComment(common_comment);
                   }

                 if(in_typedef)
                   {
                    char* vname=strstr($1,current->name);
                    SeenTypedefName(current->name,vname[strlen(current->name)]=='('?-1:1);
                    if(!in_header)
                       SeenTypedef(current->name,ConcatStrings(3,current->qual,current->type,$1));
                    if(in_function==3)
                       DownScope();
                   }
                 else if(in_function==2)
                    SeenFunctionArg(current->name,ConcatStrings(3,current->qual,current->type,$1));
                 else
                   {
                    char* vname=strstr($1,current->name);
                    if(vname[strlen(current->name)]!='(' && IsATypeName(current->type)!='f')
                      {
                       if((in_funcbody==0 || scope&EXTERN_F) && !in_structunion && !(in_header==GLOBAL && scope&EXTERN_H))
                          SeenVariableDefinition(current->name,ConcatStrings(3,current->qual,current->type,$1),SCOPE);
                       else
                          if(in_funcbody)
                             SeenScopeVariable(current->name);
                      }
                    else
                      {
                       SeenFunctionProto(current->name,in_funcbody);
                       if(in_function==3)
                          DownScope();
                      }
                   }

                 if(in_function==3 && !in_structunion) in_function=0;
                }
	;

initialized_declarator1
	: declarator
	| declarator asm_label  /*+ GNU Extension +*/
	| declarator initializer_part
	| declarator asm_label initializer_part /* GNU Extension */
	;

initializer_part
	: '=' initializer
	;

initializer
	: assignment_expression
	| '{' '}'
	| '{' initializer_list '}'
	| '{' initializer_list ',' '}'
	;

initializer_list
	: named_initializer
	| initializer_list ',' named_initializer
	;

named_initializer
	: initializer
	| component_name ':' initializer
	| '.' component_name '=' initializer
	| '[' named_initializer_index ']' initializer
	| '[' named_initializer_index ']' '=' initializer
	;

named_initializer_index
	: constant_expression
	| constant_expression ELLIPSES constant_expression
	;


/* Abstract declarator */

abstract_declarator
	: pointer
	| pointer direct_abstract_declarator
                { $$=ConcatStrings(2,$1,$2); }
	| direct_abstract_declarator
	;

direct_abstract_declarator
	: '(' abstract_declarator ')'
                { $$=ConcatStrings(3,$1,$2,$3);
                  { int i=0; while($2[i] && $2[i]=='*') i++; if(!$2[i]) in_type_spec=0; } }
	| '[' ']'
                { $$=ConcatStrings(2,$1,$2); }
	| direct_abstract_declarator '[' ']'
                { $$=ConcatStrings(3,$1,$2,$3); }
	| '[' constant_expression ']'
                { $$=ConcatStrings(3,$1,$2,$3); }
	| direct_abstract_declarator '[' constant_expression ']'
                { $$=ConcatStrings(4,$1,$2,$3,$4); }
	| '(' ')'
                { $$=ConcatStrings(2,$1,$2); }
	| direct_abstract_declarator '(' ')'
                { $$=ConcatStrings(3,$1,$2,$3); }
	| '(' parameter_type_list ')'
                { $$=ConcatStrings(3,$1,$2,$3); }
	| direct_abstract_declarator '(' parameter_type_list ')'
                { $$=ConcatStrings(4,$1,$2,$3,$4); }
	;

/* Declarator */

declarator
	: direct_declarator
                { in_type_spec=0; }
	| pointer direct_declarator
                { in_type_spec=0; $$=ConcatStrings(2,$1,$2); }
	;

pointer
	: '*'
	| '*' type_qualifier_list
                { $$=ConcatStrings(3,$1," ",$2); }
	| '*' pointer
                { $$=ConcatStrings(2,$1,$2); }
	| '*' type_qualifier_list pointer
                { $$=ConcatStrings(4,$1," ",$2,$3); }
	;

direct_declarator
	: simple_declarator
	| '(' declarator ')'
                { if($2[0]=='*' && $2[1]==' ') { $2=&$2[1]; $2[0]='*'; }
                  $$=ConcatStrings(4," ",$1,$2,$3);
                }
	| array_declarator
	| function_direct_declarator
	;

simple_declarator
	: IDENTIFIER
                { $$=ConcatStrings(2," ",$1); current->name=$1;
                  if(!current->type) current->type="int";
                  if(in_funcdef==1 && in_function!=3 && !in_structunion) SeenScopeVariable($1); }
	;

array_declarator
	: direct_declarator '[' ']'
                { $$=ConcatStrings(3,$1,$2,$3); }
        | direct_declarator '[' { in_type_spec=0; } constant_expression { in_type_spec=1; } ']'
                { $$=ConcatStrings(4,$1,$2,$4,$6); }
	;

/*-------------------- Storage class and types --------------------*/

name
	: IDENTIFIER
	;

storage_class_specifier
	: AUTO
                { $$=NULL; }
	| EXTERN
                { $$=NULL;
                  if(in_funcbody) scope|=EXTERN_F;
                  else if(in_header) scope|=EXTERN_H;
                  else scope|=EXTERNAL; }
	| REGISTER
                { $$=NULL; }
	| STATIC
                { $$=NULL; scope |= LOCAL; }
	| TYPEDEF
                { $$=NULL;
                  in_typedef=1; if(!in_header) SeenTypedef(NULL,NULL);
                  common_comment=CopyString(GetCurrentComment()); }
	| INLINE                /* GNU Extension */
                { $$=NULL; scope |= INLINED; }
	;

type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
                { $$=ConcatStrings(3,$1," ",$2); }
	;

type_qualifier
	: CONST
                { if(!current->type) current->qual=ConcatStrings(3,current->qual,$1," "); }
	| VOLATILE
                { if(!current->type) current->qual=ConcatStrings(3,current->qual,$1," "); }
	;

/* Types */

type_specifier
	: type_specifier1
                { in_type_spec=1; }
	;

type_specifier1
	: enumeration_type_specifier
	| floating_type_specifier
	| integer_type_specifier
	| structure_type_specifier
	| typedef_name
	| union_type_specifier
	| void_type_specifier
	;

floating_type_specifier
	: FLOAT
	| DOUBLE
	| DOUBLE LONG
                { $$=ConcatStrings(3,$1," ",$2); }
	| LONG DOUBLE
                { $$=ConcatStrings(3,$1," ",$2); }
	;

integer_type_specifier
	: integer_type_specifier_part
	| integer_type_specifier_part type_qualifier
                { $$=ConcatStrings(3,$1," ",$2); }
	| integer_type_specifier integer_type_specifier_part
                { $$=ConcatStrings(3,$1," ",$2); }
	;

integer_type_specifier_part
	: SIGNED
	| UNSIGNED
	| CHAR
	| SHORT
	| INT
	| LONG
	;

typedef_name
	: TYPE_NAME
	;

void_type_specifier
	: VOID
	;

type_name
	: declaration_specifiers
                { in_type_spec=0; }
	| declaration_specifiers abstract_declarator
                { in_type_spec=0; $$=ConcatStrings(2,$1,$2); }
	;

/* Enumerated types */

enumeration_type_specifier
	: enumeration_type_definition
	| enumeration_type_reference
	;

enumeration_type_definition
	: ENUM '{'
                { push();
                  if(!in_header)
                    {
                     if(in_structunion) SeenStructUnionComp($1,in_structunion);
                     else               SeenStructUnionStart($1);
                    }
                  in_structunion++; }
          enumeration_definition_list '}'
                { pop(); in_structunion--;
                  if(!in_structunion && !current->type) current->type=ConcatStrings(2,$1," {...}");
                  if(!in_header && !in_structunion && in_typedef) SeenStructUnionEnd();
                  $$=ConcatStrings(5,$1," ",$2,$4,$5); }
	| ENUM enumeration_tag '{'
                { push();
                  if(!in_header)
                    {
                     if(in_structunion) SeenStructUnionComp(ConcatStrings(3,$1," ",$2),in_structunion);
                     else               SeenStructUnionStart(ConcatStrings(3,$1," ",$2));
                    }
                  in_structunion++; }
          enumeration_definition_list '}'
                { pop(); in_structunion--;
                  if(!in_structunion && !current->type) current->type=ConcatStrings(3,$1," ",$2);
                  if(!in_header && !in_structunion) SeenStructUnionEnd();
                  $$=ConcatStrings(7,$1," ",$2," ",$3,$5,$6);}
	;

enumeration_definition_list
	: enumeration_definition_list1
	| enumeration_definition_list1 ',' /* Not ANSI, but common */
	;

enumeration_definition_list1
	: enumeration_constant_definition
	| enumeration_definition_list1 ',' enumeration_constant_definition
                { $$=ConcatStrings(3,$1,$2,$3); }
	;

enumeration_constant_definition
	: enumeration_constant
                { if(!in_header) SeenStructUnionComp($1,in_structunion); }
	| enumeration_constant '=' assignment_expression /* Should be constant expression */
                { $$=ConcatStrings(3,$1,$2,$3); if(!in_header) SeenStructUnionComp($1,in_structunion); }
	;

enumeration_constant
	: IDENTIFIER
	;

enumeration_type_reference
	: ENUM enumeration_tag
                { $$=ConcatStrings(3,$1," ",$2); }
	;

enumeration_tag
	: IDENTIFIER
	| TYPE_NAME
	;

/* Structures */

structure_type_specifier
	: structure_type_definition
	| structure_type_reference
	;

structure_type_definition
	: STRUCT '{'
                { push();
                  if(!in_header)
                    {
                     if(in_structunion) SeenStructUnionComp($1,in_structunion);
                     else               SeenStructUnionStart($1);
                    }
                  in_structunion++; }
	field_list '}'
                { pop(); in_structunion--;
                  if(!in_structunion && !current->type) current->type=ConcatStrings(2,$1," {...}");
                  if(!in_header && !in_structunion && in_typedef) SeenStructUnionEnd();
                  $$=ConcatStrings(5,$1," ",$2,$4,$5); }
	| STRUCT structure_tag '{'
                { push();
                  if(!in_header)
                    {
                     if(in_structunion) SeenStructUnionComp(ConcatStrings(3,$1," ",$2),in_structunion);
                     else               SeenStructUnionStart(ConcatStrings(3,$1," ",$2));
                    }
                  in_structunion++; }
	field_list '}'
                { pop(); in_structunion--;
                  if(!in_structunion && !current->type) current->type=ConcatStrings(3,$1," ",$2);
                  if(!in_header && !in_structunion) SeenStructUnionEnd();
                  $$=ConcatStrings(7,$1," ",$2," ",$3,$5,$6);}
	;

structure_type_reference
	: STRUCT structure_tag
                { $$=ConcatStrings(3,$1," ",$2); }
	;

structure_tag
	: IDENTIFIER
	| TYPE_NAME
	;

/* Unions */

union_type_specifier
	: union_type_definition
	| union_type_reference
	;

union_type_definition
	: UNION '{'
                { push();
                  if(!in_header)
                    {
                     if(in_structunion) SeenStructUnionComp($1,in_structunion);
                     else               SeenStructUnionStart($1);
                    }
                  in_structunion++; }
	field_list '}'
                { pop(); in_structunion--;
                  if(!in_structunion && !current->type) current->type=ConcatStrings(2,$1," {...}");
                  if(!in_header && !in_structunion && in_typedef) SeenStructUnionEnd();
                  $$=ConcatStrings(5,$1," ",$2,$4,$5); }
	| UNION union_tag '{'
                { push();
                  if(!in_header)
                    {
                     if(in_structunion) SeenStructUnionComp(ConcatStrings(3,$1," ",$2),in_structunion);
                     else               SeenStructUnionStart(ConcatStrings(3,$1," ",$2));
                    }
                  in_structunion++; }
	field_list '}'
                { pop(); in_structunion--;
                  if(!in_structunion && !current->type) current->type=ConcatStrings(3,$1," ",$2);
                  if(!in_header && !in_structunion) SeenStructUnionEnd();
                  $$=ConcatStrings(7,$1," ",$2," ",$3,$5,$6);}
	;

union_type_reference
	: UNION union_tag
                { $$=ConcatStrings(3,$1," ",$2); }
	;

union_tag
	: IDENTIFIER
	| TYPE_NAME
	;

/* Struct or Union */

field_list
	: /* empty */
	| field_list1
	;

field_list1
	: field_list2
	| field_list1 field_list2
                { $$=ConcatStrings(2,$1,$2); }
	;

field_list2
	: ';'
	| structure_type_definition ';'
                { $$ = ConcatStrings(3, $1, " ", $2);
                  if(!in_header) SeenStructUnionComp($1,in_structunion); }
	| union_type_definition ';'
                { $$ = ConcatStrings(3, $1, " ", $2);
                  if(!in_header) SeenStructUnionComp($1,in_structunion); }
	| component_declaration
	;

component_declaration
	: type_specifier
                { comp_type=$1; }
          component_declarator_list ';'
                { $$=ConcatStrings(3,$1,$3,$4); reset(); in_type_spec=0; }
	| type_qualifier_list type_specifier
                { comp_type=ConcatStrings(3,$1," ",$2); }
          component_declarator_list ';'
                { $$=ConcatStrings(4,$1,$2,$4,$5); reset(); in_type_spec=0; }
	| type_specifier type_qualifier_list
                { comp_type=ConcatStrings(3,$1," ",$2); }
          component_declarator_list ';'
                { $$=ConcatStrings(4,$1,$2,$4,$5); reset(); in_type_spec=0; }
	;

component_declarator_list
	: component_declarator
                { if(!in_header) SeenStructUnionComp(ConcatStrings(2,comp_type,$1),in_structunion); }
	| component_declarator_list ',' component_declarator
                { $$=ConcatStrings(3,$1,$2,$3);
                  if(!in_header) SeenStructUnionComp(ConcatStrings(2,comp_type,$3),in_structunion); }
	;

component_declarator
	: simple_component
	| bit_field
	;

simple_component
	: declarator
                { if(in_function==2 && !in_structunion) { DownScope(); pop(); in_function=0; } }
	;

bit_field
	: ':' width
                { $$=ConcatStrings(2,$1,$2); }
	| declarator ':' width
                { $$=ConcatStrings(3,$1,$2,$3); }
	;

width
	: assignment_expression /* Should be expression */
	;

component_name
	: IDENTIFIER
	| TYPE_NAME
	;

/*-------------------- Functions --------------------*/

/* Function Definition */

function_definition
	: function_specifier
                { pop(); in_funcbody=1; in_function=0; }
          compound_statement
                { in_funcbody=in_function=0; DownScope(); SeenFunctionDefinition(NULL); }
	;

function_specifier
	: function_specifier1
                { char *func_type,*fname=strstr($1,(current-1)->name),*parenth=strstr($1,"(");
                  if(parenth>fname)
                     {parenth[0]=0;func_type=ConcatStrings(3,(current-1)->qual,(current-1)->type,$1);}
                  else
                    {
                     int open=1;
                     char *argbeg=strstr(&parenth[1],"("),*argend;
                     argbeg[1]=0;
                     for(argend=argbeg+2;*argend;argend++)
                       {
                        if(*argend=='(') open++;
                        if(*argend==')') open--;
                        if(!open) break;
                       }
                     func_type=ConcatStrings(4,(current-1)->qual,(current-1)->type,$1,argend);
                    }
                  SeenFunctionDefinition(func_type);
                }
	;

function_specifier1
	: function_declarator
	| declaration_specifiers function_declarator
                { $$=ConcatStrings(3,current->qual,current->type,$2); }
	| function_declarator declaration_list
	| declaration_specifiers function_declarator declaration_list
                { $$=ConcatStrings(3,current->qual,current->type,$2); }
	;

/* Function Declaration */

function_declarator
	: function_declarator0
                { if(!in_structunion) { push(); in_function=2; } }
	;

function_declarator0
	: function_direct_declarator
	| '(' function_direct_declarator ')'
	| pointer function_direct_declarator
                { $$=ConcatStrings(2,$1,$2); }
	| pointer '(' function_direct_declarator ')'
                { $$=ConcatStrings(2,$1,$3); }
	;

function_direct_declarator
	: function_declarator1 '('
                { if(!in_structunion)
                  { push(); if(in_function==0) UpScope();
                    if(in_function==0 && !in_funcdef) in_function=1; if(in_function!=3) in_funcdef++; } }
          function_declarator2 ')'
                { if(!in_structunion)
                    { pop();  if(in_function!=3) in_funcdef--; if(in_funcdef==0) in_function=3; }
                  $$=ConcatStrings(4,$1,$2,$4,$5); }
	;

function_declarator1
	: direct_declarator
                {
                  if(!in_funcdef && !in_function && !in_funcbody && !in_structunion) SeenFunctionDeclaration(current->name,SCOPE);
                  in_type_spec=0;
                }
	;

function_declarator2
	: /* Empty */
                { if(in_function==1 && in_funcdef==1 && !in_structunion) SeenFunctionArg("void","void");
                  if(in_structunion) $$=NULL; else $$="void"; }
	| parameter_type_list
	| identifier_list
	;

identifier_list
	: IDENTIFIER
                { if(in_function==1 && in_funcdef==1 && in_funcbody==0 && !in_structunion) { SeenFunctionArg($1,NULL); SeenScopeVariable($1); } }
	| identifier_list ',' IDENTIFIER
                { if(in_function==1 && in_funcdef==1 && in_funcbody==0 && !in_structunion) { SeenFunctionArg($3,NULL); SeenScopeVariable($3); }
                  $$=ConcatStrings(3,$1,$2,$3); }
	;

parameter_type_list
	: parameter_list
	| parameter_list ',' ELLIPSES
                { if(in_function==1 && in_funcdef==1 && in_funcbody==0 && !in_structunion) SeenFunctionArg($3,$3);
                  $$=ConcatStrings(3,$1,$2,$3); }
	;

parameter_list
	: parameter_declaration
                { if(in_function==1 && in_funcdef==1 && in_funcbody==0 && !in_structunion) SeenFunctionArg(strcmp("void",$1)?current->name:"void",$1);
                  in_type_spec=0; }
	| parameter_list ',' parameter_declaration
                { if(in_function==1 && in_funcdef==1 && in_funcbody==0 && !in_structunion) SeenFunctionArg(current->name,$3);
                  in_type_spec=0; $$=ConcatStrings(3,$1,$2,$3); }
	;

parameter_declaration
	: declaration_specifiers declarator
                { in_type_spec=0; $$=ConcatStrings(2,$1,$2); }
	| declaration_specifiers
                { in_type_spec=0; }
	| declaration_specifiers abstract_declarator
                { in_type_spec=0; $$=ConcatStrings(2,$1,$2); }
	;

/*-------------------- Statements --------------------*/

statement
	: asm_statement         /*+ GNU Extension +*/
	| compound_statement
	| conditional_statement
	| iterative_statement
	| labeled_statement
	| switch_statement
	| break_statement
	| continue_statement
	| expression_statement
	| goto_statement
	| null_statement
	| return_statement
	;

/* Compound statement */

compound_statement
	: '{'
                { UpScope(); reset(); }
	  compound_statement_body
                { DownScope(); }
	  '}'
	;

compound_statement_body
	: /* Empty */
	| block_item_list
	;

block_item_list
	: block_item
	| block_item_list block_item
	;

block_item
	: statement
	| declaration
                { scope=0; reset(); common_comment=NULL; in_typedef=0; }
	;

/* Conditional statements */

conditional_statement
	: if_statement
	| if_else_statement
	;

if_else_statement
	: IF '(' expression ')' statement ELSE statement
	;

if_statement
	: IF '(' expression ')' statement
	;

/* Iterative statements */

iterative_statement
	: do_statement
	| for_statement
	| while_statement
	;

do_statement
	: DO statement WHILE '(' expression ')' ';'
	;

for_statement
	: FOR '(' for_expressions ')' statement
	;

for_expressions
	: ';' ';'
	| expression ';' ';'
	| ';' expression ';'
	| ';' ';' expression
	| ';' expression ';' expression
	| expression ';' ';' expression
	| expression ';' expression ';'
	| expression ';' expression ';' expression
	;

while_statement
	: WHILE '(' expression ')' statement
	;

/* Label Statements */

labeled_statement               /* Allows for no statement following a label. */
	: case_label ':'
	| named_label ':'
	| default_label ':'
	;

case_label
	: CASE constant_expression
	| CASE constant_expression ELLIPSES constant_expression
	;

default_label
	: DEFAULT
	;

named_label
	: IDENTIFIER
	;

/* Switch statement */

switch_statement
	: SWITCH '(' expression ')' statement
	;

/* Other Statements */

break_statement
	: BREAK ';'
	;

continue_statement
	: CONTINUE ';'
	;

expression_statement
	: expression ';'
	;

goto_statement
	: GOTO IDENTIFIER ';'
	;

null_statement
	: ';'
	;

return_statement
	: RETURN ';'
	| RETURN expression ';'
	;

/*-------------------- Expressions --------------------*/

expression
	: comma_expression
	;

/* Precedence 1 */

comma_expression
	: assignment_expression
	| comma_expression ',' assignment_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;

/* Precedence 2 */

assignment_expression
	: conditional_expression
	| named_label_address
	| unary_expression assignment_op assignment_expression
	| unary_expression assignment_op '{' assignment_expression_list '}'
	;
assignment_op
        : '='
        | MUL_ASSIGN
        | DIV_ASSIGN
        | MOD_ASSIGN
        | ADD_ASSIGN
        | SUB_ASSIGN
        | LEFT_ASSIGN
        | RIGHT_ASSIGN
        | AND_ASSIGN
        | XOR_ASSIGN
        | OR_ASSIGN
        ;

/* Precedence 3 */

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
                { $$=ConcatStrings(5,$1,$2,$3,$4,$5); }
	| logical_or_expression '?'            ':' conditional_expression /* GNU Extension */
                { $$=ConcatStrings(4,$1,$2,$3,$4); }
	;

/* Precedence 4 */

logical_or_expression
	: logical_and_expression
	| logical_or_expression OR_OP logical_and_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;

/* Precedence 5 */

logical_and_expression
	: bitwise_or_expression
	| logical_and_expression AND_OP bitwise_or_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;

/* Precedence 6 */

bitwise_or_expression
	: bitwise_xor_expression
	| bitwise_or_expression '|' bitwise_xor_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;

/* Precedence 7 */

bitwise_xor_expression
	: bitwise_and_expression
	| bitwise_xor_expression '^' bitwise_and_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;

/* Precedence 8 */

bitwise_and_expression
	: equality_expression
	| bitwise_and_expression '&' equality_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;

/* Precedence 9 */

equality_expression
	: relational_expression
	| equality_expression equality_op relational_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;
equality_op
	: EQ_OP
	| NE_OP
	;

/* Precedence 10 */

relational_expression
	: shift_expression
	| relational_expression relational_op shift_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;
relational_op
	: '<'
	| LE_OP
	| '>'
	| GE_OP
	;

/* Precedence 11 */

shift_expression
	: additive_expression
	| shift_expression shift_op additive_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;
shift_op
	: LEFT_SHIFT
	| RIGHT_SHIFT
	;

/* Precedence 12 */

additive_expression
	: multiplicative_expression
	| additive_expression add_op multiplicative_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;
add_op
	: '+'
	| '-'
	;

/* Precedence 13 */

multiplicative_expression
	: unary_expression
	| multiplicative_expression mult_op unary_expression
                { $$=ConcatStrings(3,$1,$2,$3); }
	;
mult_op
	: '*'
	| '/'
	| '%'
	;

/* Precedence 14 */

unary_expression
	: address_expression
	| bitwise_negation_expression
	| cast_expression
	| indirection_expression
	| logical_negation_expression
	| predecrement_expression
	| preincrement_expression
	| sizeof_expression
	| unary_minus_expression
	| unary_plus_expression
	| postfix_expression
	;

address_expression
	: '&' unary_expression
	;

bitwise_negation_expression
	: '~' unary_expression
                { $$=ConcatStrings(2,$1,$2); }
	;

cast_expression
	: '(' type_name ')' unary_expression
                { $$=ConcatStrings(4,$1,$2,$3,$4); }
	| '(' type_name ')' '{' assignment_expression_list '}' /* GNU Extension */
	| '(' type_name ')' '{' named_assignment_list '}' /* GNU Extension */
	;

indirection_expression
	: '*' unary_expression
	;

logical_negation_expression
	: '!' unary_expression
                { $$=ConcatStrings(2,$1,$2); }
	;

predecrement_expression
	: DEC_OP unary_expression
	;

preincrement_expression
	: INC_OP unary_expression
	;

sizeof_expression
	: SIZEOF '(' type_name ')'
                { $$=ConcatStrings(4,$1,$2,$3,$4); }
	| SIZEOF unary_expression
                { $$=ConcatStrings(2,$1,$2); }
	;

unary_minus_expression
	: '-' unary_expression
                { $$=ConcatStrings(2,$1,$2); }
	;

unary_plus_expression
	: '+' unary_expression
                { $$=ConcatStrings(2,$1,$2); }
	;

/* Precedence 15 */

postfix_expression
	: component_selection_expression
	| function_call
	| function_call_direct
                { if(!IsAScopeVariable($1)) SeenFunctionCall($1); }
	| postdecrement_expression
	| postincrement_expression
	| subscript_expression
	| primary_expression
	;

component_selection_expression
	: direct_component_selection
	| indirect_component_selection
	;

direct_component_selection
	: postfix_expression '.' component_name
	;

indirect_component_selection
	: postfix_expression PTR_OP component_name
	;

function_call
	: postfix_expression '(' ')'
	| postfix_expression '(' expression_list ')'
	;

function_call_direct
	: name '(' ')'
	| name '(' expression_list ')'
	;

postdecrement_expression
	: postfix_expression DEC_OP
	;

postincrement_expression
	: postfix_expression INC_OP
	;

subscript_expression
	: postfix_expression '[' expression ']'
	;

primary_expression
	: name
                { CheckFunctionVariableRef($1,in_funcbody); }
	| LITERAL
	| string_literal
	| parenthesized_expression
	;
string_literal
	: STRING_LITERAL
	| string_literal STRING_LITERAL
        ;

parenthesized_expression
	: '(' expression ')'
                { $$=ConcatStrings(3,$1,$2,$3); }
	| '(' { push(); } compound_statement { pop(); } ')' /* GNU Extension */
	;

/* Other expressions */

constant_expression
	: expression
	;

expression_list
	: assignment_expression
	| expression_list ',' assignment_expression
	;

/*-------------------- GNU Extensions --------------------*/

/* ASM Statements */

asm_statement
	: asm_type '(' string_literal ')' ';'
	| asm_type '(' string_literal ':' asm_inout_list ')' ';'
	| asm_type '(' string_literal ':' asm_inout_list ':' asm_inout_list ')' ';'
	| asm_type '(' string_literal ':' asm_inout_list ':' asm_inout_list ':' asm_clobber_list ')' ';'
	;

asm_type
	: ASM
	| ASM VOLATILE
	| VOLATILE ASM
	;

asm_inout_list
	: /* Empty */
	| asm_inout
	| asm_inout_list ',' asm_inout
	;

asm_inout
	: string_literal '(' expression ')'
	;

asm_clobber_list
	: /* Empty */
	| string_literal
	| asm_clobber_list ',' string_literal
	;

asm_label
	: ASM '(' string_literal ')'
	;

/* Named label address */

named_label_address
	: AND_OP named_label
	;

/* Assignment of structure / union */

assignment_expression_list
	: assignment_expression_list_item
	| assignment_expression_list ',' assignment_expression_list_item
	;

assignment_expression_list_item
	: /* Empty */
	| assignment_expression
	| '{' assignment_expression_list '}'
	;

named_assignment
	: component_name ':' assignment_expression
	| component_name ':' '{' assignment_expression_list '}'
	| '.' component_name '=' assignment_expression
	| '.' component_name '=' '{' assignment_expression_list '}'
	;

named_assignment_list
	: named_assignment
	| named_assignment_list ',' named_assignment
	;

%%

#if YYDEBUG

static int   last_yylex[11];
static char *last_yylval[11];
static int count=0,modcount=0;

#endif /* YYDEBUG */


 /*++++++++++++++++++++++++++++++++++++++
  Stop parsing the current file, due to an error.

  char *s The error message to print out.
  ++++++++++++++++++++++++++++++++++++++*/

static void yyerror( char *s )
{
#if YYDEBUG
 int i;
#endif

 fflush(stdout);
 fprintf(stderr,"%s:%d: cxref: %s\n\n",parse_file,parse_line,s);

#if YYDEBUG

 fprintf(stderr,"The previous 10, current and next 10 symbols are:\n");

 for(i=count>10?count-11:0,modcount=i%11;i<count-1;i++,modcount=i%11)
#ifdef YYBISON
    fprintf(stderr,"%3d | %3d : %16s : %s\n",i+1-count,last_yylex[modcount],yytname[YYTRANSLATE(last_yylex[modcount])],last_yylval[modcount]);
#else
    fprintf(stderr,"%3d | %3d : %s\n",i+1-count,last_yylex[modcount],last_yylval[modcount]);
#endif

#ifdef YYBISON
 fprintf(stderr,"  0 | %3d : %16s : %s\n",yychar,yytname[YYTRANSLATE(yychar)],yylval);
#else
 fprintf(stderr,"  0 | %3d : %s\n",yychar,yylval);
#endif

 for(i=0;i<10;i++)
   {
    yychar=yylex();
    if(!yychar)
      {fprintf(stderr,"END OF FILE\n");break;}
#ifdef YYBISON
    fprintf(stderr,"%3d | %3d : %16s : %s\n",i+1,yychar,yytname[YYTRANSLATE(yychar)],yylval);
#else
    fprintf(stderr,"%3d | %3d : %s\n",i+1,yychar,yylval);
#endif
   }

 fprintf(stderr,"\n");

#endif /* YYDEBUG */

 /* Finish off the input. */

#undef yylex

 if(yychar)
    while((yychar=yylex()));
}


 /*++++++++++++++++++++++++++++++++++++++
  Call the lexer, the feedback from the parser to the lexer is applied here.

  int cxref_yylex Returns the value from the lexer, modified due to parser feedback.
  ++++++++++++++++++++++++++++++++++++++*/

static int cxref_yylex(void)
{
 static int last_yyl=0;
 int yyl=yylex();

 if(yyl==TYPE_NAME)
    if(in_type_spec || (in_structunion && last_yyl=='}') || last_yyl==TYPE_NAME ||
       last_yyl==CHAR || last_yyl==SHORT || last_yyl==INT || last_yyl==LONG ||
       last_yyl==SIGNED || last_yyl==UNSIGNED ||
       last_yyl==FLOAT || last_yyl==DOUBLE)
       yyl=IDENTIFIER;

 last_yyl=yyl;

#if YYDEBUG

 last_yylex [modcount]=yyl;
 last_yylval[modcount]=yylval;

 if(yyl)
   {
    count++;
    modcount=count%11;
   }
 else
   {
    count=0;
    modcount=0;
   }

#if YYDEBUG == 2

 if(yyl)
#ifdef YYBISON
    printf("#parse.y# %6d | %16s:%4d | %3d : %16s : %s\n",count,parse_file,parse_line,yyl,yytname[YYTRANSLATE(yyl)],yylval);
#else
    printf("#parse.y# %6d | %16s:%4d | %3d : %s\n",count,parse_file,parse_line,yyl,yylval);
#endif /* YYBISON */
 else
    printf("#parse.y# %6d | %16s:%4d | END OF FILE\n",count,parse_file,parse_line);

 fflush(stdout);

#endif /* YYDEBUG==2 */

#endif /* YYDEBUG */

 return(yyl);
}
