W                       [ \t]
N                       (\r*\n)
D                       [0-9]
S                       [-+]
L                       [a-zA-Z_$]
H                       [a-fA-F0-9]
E                       [Ee]{S}?{D}+
FS                      (f|F|l|L)
IS                      ((u|U|l|L)+|([iI]8|[iI]16|[iI]32|[iI]64))

%x START_COMMENT     COMMENT     SPECIAL_COMMENT
%x START_COMMENT_CPP COMMENT_CPP SPECIAL_COMMENT_CPP
%x CPP CPP_START
%x CPP_INCLUDE CPP_INC_FILE CPP_INC_FLAGS
%x CPP_DEFINE CPP_DEFINE_ARGP CPP_DEFINE_BODY CPP_DEFINE_ARGS
%x GNU_LABEL GNU_VA_ARG GNU_ATTRIBUTE GNU_TYPEOF
%x CHAR_VARYING

%{
/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5f.

  C lexical analyser
  CPP processing, including GNU extensions, using yylval as a string.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,97,98,99,2000,01,02,03,04 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

#include <string.h>
#include <unistd.h>

#include "parse-yy.h"
#include "parse-yacc.h"
#include "cxref.h"
#include "memory.h"

/*+ The name of the current file. +*/
char* parse_file=NULL;

/*+ The current line number in the file. +*/
int parse_line=0;


/*+ If we are in a header file then ignore the comments. +*/
extern int in_header;

/*+ One of the options controlling how comments are processed, +*/
extern int option_all_comments,       /*+ use all comments not just the specially formattted ones. +*/
           option_block_comments,     /*+ remove the leading block comment marker. +*/
           option_no_comments;        /*+ ignore all comments. +*/

/*+ Flag that indicates if the comment warnings are to be issued. +*/
extern int option_warn;


/*+ The flags that come out of GCC when a file is included. +*/
static int inc_file_flags=0;

/*+ The name of a file seen in a CPP_INC_FILE state. +*/
static char *inc_file=NULL;

/*+ The value of the thing that is defined (but only if it is simple). +*/
static char* define_value=NULL;

/*+ The lex state at the time that a comment is seen. +*/
static int comment_init_state=0;

/*+ To get around the GCC __builtin_va_arg keyword, skip over matched () counted by this. +*/
static int gnu_va_arg_depth=0;

/*+ To get around the GCC __attribute__ keyword, skip over matched () counted by this. +*/
static int gnu_att_depth=0;

/*+ To get around the GCC __typeof__ keyword, skip over matched () counted by this. +*/
static int gnu_typ_depth=0;

/*+ If we see a comment immediately after a ',', ';', '};', '},' or ')' then push it before. +*/
static int push_past=0;


%}

%%

 /* Comments, could be embedded in a preprocessor directive. */

<INITIAL>"/*"                     { comment_init_state = INITIAL        ; BEGIN(START_COMMENT); }
<CPP_START>"/*"                   { comment_init_state = CPP_START      ; BEGIN(START_COMMENT); }
<CPP>"/*"                         { comment_init_state = CPP            ; BEGIN(START_COMMENT); }
<CPP_DEFINE>"/*"                  { comment_init_state = CPP_DEFINE     ; BEGIN(START_COMMENT); }
<CPP_INCLUDE>"/*"                 { comment_init_state = CPP_INCLUDE    ; BEGIN(START_COMMENT); }
<CPP_DEFINE_ARGP>"/*"             { comment_init_state = CPP_DEFINE_ARGP; BEGIN(START_COMMENT); }
<CPP_DEFINE_ARGS>"/*"             { comment_init_state = CPP_DEFINE_ARGS; BEGIN(START_COMMENT); }
<CPP_DEFINE_BODY>"/*"             { comment_init_state = CPP_DEFINE_BODY; BEGIN(START_COMMENT); }
<CPP_INC_FILE>"/*"                { comment_init_state = CPP_INC_FILE   ; BEGIN(START_COMMENT); }
<CPP_INC_FLAGS>"/*"               { comment_init_state = CPP_INC_FLAGS  ; BEGIN(START_COMMENT); }

<INITIAL>"//"                     { comment_init_state = INITIAL        ; BEGIN(START_COMMENT_CPP); }
<CPP_START>"//"                   { comment_init_state = CPP_START      ; BEGIN(START_COMMENT_CPP); }
<CPP>"//"                         { comment_init_state = CPP            ; BEGIN(START_COMMENT_CPP); }
<CPP_DEFINE>"//"                  { comment_init_state = CPP_DEFINE     ; BEGIN(START_COMMENT_CPP); }
<CPP_INCLUDE>"//"                 { comment_init_state = CPP_INCLUDE    ; BEGIN(START_COMMENT_CPP); }
<CPP_DEFINE_ARGP>"//"             { comment_init_state = CPP_DEFINE_ARGP; BEGIN(START_COMMENT_CPP); }
<CPP_DEFINE_ARGS>"//"             { comment_init_state = CPP_DEFINE_ARGS; BEGIN(START_COMMENT_CPP); }
<CPP_DEFINE_BODY>"//"             { comment_init_state = CPP_DEFINE_BODY; BEGIN(START_COMMENT_CPP); }
<CPP_INC_FILE>"//"                { comment_init_state = CPP_INC_FILE   ; BEGIN(START_COMMENT_CPP); }
<CPP_INC_FLAGS>"//"               { comment_init_state = CPP_INC_FLAGS  ; BEGIN(START_COMMENT_CPP); }

 /* Check for special or normal C style comments. */

<START_COMMENT>"*"+               { BEGIN(SPECIAL_COMMENT); }
<START_COMMENT>"+"+               { BEGIN(SPECIAL_COMMENT); }
<START_COMMENT>[*+ \t]*"*/"       { BEGIN(comment_init_state); while(push_past) {push_past--;unput(yylval[push_past]);} }
<START_COMMENT>{N}                { if(option_all_comments) SeenComment(yytext,0); parse_line++;
                                    if(option_all_comments) BEGIN(SPECIAL_COMMENT); else BEGIN(COMMENT); }
<START_COMMENT>[^*+]              { if(option_all_comments) SeenComment(yytext,0);
                                    if(option_all_comments) BEGIN(SPECIAL_COMMENT); else BEGIN(COMMENT); }

 /* Check for special or normal C++ style comments. */

<START_COMMENT_CPP>"*"+           { BEGIN(SPECIAL_COMMENT_CPP); }
<START_COMMENT_CPP>"+"+           { BEGIN(SPECIAL_COMMENT_CPP); }
<START_COMMENT_CPP>{N}            { BEGIN(comment_init_state); unput('\n'); while(push_past) {push_past--;unput(yylval[push_past]);} }
<START_COMMENT_CPP>[^*+]          { if(option_all_comments) BEGIN(SPECIAL_COMMENT_CPP); else BEGIN(COMMENT_CPP); }

<COMMENT>{N}                      { parse_line++; }
<COMMENT>.                        { }
<COMMENT>[+*]+/"*/"               { if(option_warn&WARN_COMMENT) fprintf(stderr,"%s:%d: Warning unbalanced cxref comment; starts simple, ends special.\n",parse_file,parse_line); }
<COMMENT>"*/"                     { BEGIN(comment_init_state); while(push_past) {push_past--;unput(yylval[push_past]);} }

<COMMENT_CPP>{N}                  { BEGIN(comment_init_state); unput('\n'); while(push_past) {push_past--;unput(yylval[push_past]);} }
<COMMENT_CPP>.*                   { /* Found EOF before newline! */ }

 /* Handle special C style comments. */

<SPECIAL_COMMENT>{N}                { if(!option_no_comments) SeenComment(yytext,0); parse_line++; }
 /* The following three lines are optimised for run-time speed.
    They replace the following pattern which has variable trailing context:
    <SPECIAL_COMMENT>{N}{W}*[+*|:]/({W}|{N})
 */
<SPECIAL_COMMENT>{N}{W}*[+*|:]/{W}  { parse_line++;
                                      if(option_block_comments) yytext[0]='\n',yytext[1]=0;
                                      if(!option_no_comments) SeenComment(yytext,0); }
<SPECIAL_COMMENT>{N}{W}*[+*|:]/\n   { parse_line++;
                                      if(option_block_comments) yytext[0]='\n',yytext[1]=0;
                                      if(!option_no_comments) SeenComment(yytext,0); }
<SPECIAL_COMMENT>{N}{W}*[+*|:]/\r\n { parse_line++;
                                      if(option_block_comments) yytext[0]='\n',yytext[1]=0;
                                      if(!option_no_comments) SeenComment(yytext,0); }
<SPECIAL_COMMENT>[^\r\n*+]+         { if(!option_no_comments) SeenComment(yytext,0); }
<SPECIAL_COMMENT>.                  { if(!option_no_comments) SeenComment(yytext,0); }

<SPECIAL_COMMENT>{W}*"*"+"*/"       { if(!option_no_comments) SeenComment(NULL,1);
                                      BEGIN(comment_init_state); while(push_past) {push_past--;unput(yylval[push_past]);} }
<SPECIAL_COMMENT>{W}*"+"+"*/"       { if(!option_no_comments) SeenComment(NULL,2);
                                      if(!in_header && comment_init_state==CPP_DEFINE_ARGS) SeenDefineFuncArgComment();
                                      if(!in_header && comment_init_state==CPP_DEFINE_BODY) SeenDefineComment();
                                      if(!in_header && comment_init_state==CPP_INCLUDE)     SeenIncludeComment();
                                      BEGIN(comment_init_state); while(push_past) {push_past--;unput(yylval[push_past]);} }
<SPECIAL_COMMENT>"*/"               { if(!option_all_comments && option_warn&WARN_COMMENT)
                                         fprintf(stderr,"%s:%d: Warning unbalanced cxref comment; starts special, ends simple.\n",parse_file,parse_line);
                                      if(option_all_comments) SeenComment(NULL,3); else if(!option_no_comments) SeenComment(NULL,2);
                                      if(!in_header && comment_init_state==CPP_DEFINE_ARGS) SeenDefineFuncArgComment();
                                      if(!in_header && comment_init_state==CPP_DEFINE_BODY) SeenDefineComment();
                                      if(!in_header && comment_init_state==CPP_INCLUDE)     SeenIncludeComment();
                                      BEGIN(comment_init_state); while(push_past) {push_past--;unput(yylval[push_past]);} }

 /* Handle special C++ style comments. */

<SPECIAL_COMMENT_CPP>[^\r\n]+     { if(!option_no_comments) SeenComment(yytext,0); }
<SPECIAL_COMMENT_CPP>.            { if(!option_no_comments) SeenComment(yytext,0); }
<SPECIAL_COMMENT_CPP>{W}*{N}      { if(!option_no_comments) SeenComment(NULL,2);
                                    if(!in_header && comment_init_state==CPP_DEFINE_ARGS) SeenDefineFuncArgComment();
                                    if(!in_header && comment_init_state==CPP_DEFINE_BODY) SeenDefineComment();
                                    if(!in_header && comment_init_state==CPP_INCLUDE)     SeenIncludeComment();
                                    BEGIN(comment_init_state); unput('\n'); while(push_past) {push_past--;unput(yylval[push_past]);} }

 /* Preprocessor directives, only valid at the top level. */

#{W}*                           { BEGIN(CPP_START); }

<CPP_START>{D}+{W}+             { parse_line=atoi(yytext); BEGIN(CPP_INC_FILE);}
<CPP_START>define{W}+           { BEGIN(CPP_DEFINE); }
<CPP_START>include{W}+          { BEGIN(CPP_INCLUDE); }
<CPP_START>[a-z]+               { BEGIN(CPP); }
<CPP_START>.|\n                 { BEGIN(INITIAL); /* Ignore bad pre-processor output */ }

 /* Preprocessor directives not #define or #include. */

<CPP>.                          { }
<CPP>\\{N}                      { parse_line++; }
<CPP>{N}                        { parse_line++; BEGIN(INITIAL); }

 /* Preprocessor directive #define. */

<CPP_DEFINE>{L}({L}|{D})*                   { if(!in_header) SeenDefine(yytext); BEGIN(CPP_DEFINE_ARGP); }
<CPP_DEFINE>\\{N}                           { parse_line++; }
<CPP_DEFINE>{W}+                            { }
<CPP_DEFINE>.|\n                            { BEGIN(INITIAL); /* Ignore bad pre-processor output */ }

<CPP_DEFINE_ARGP>"("                        { BEGIN(CPP_DEFINE_ARGS); }
<CPP_DEFINE_ARGP>{N}                        { parse_line++; BEGIN(INITIAL); }
<CPP_DEFINE_ARGP>[^\r\n(]                   { define_value=NULL; BEGIN(CPP_DEFINE_BODY); }
<CPP_DEFINE_ARGP>\\{N}                      { parse_line++; }
<CPP_DEFINE_ARGP>.                          { BEGIN(INITIAL); /* Ignore bad pre-processor output */ }

<CPP_DEFINE_ARGS>{L}({L}|{D})*              { if(!in_header) SeenDefineFunctionArg(yytext); }
<CPP_DEFINE_ARGS>{L}({L}|{D})*"..."         { if(!in_header) SeenDefineFunctionArg(yytext); }
<CPP_DEFINE_ARGS>"..."                      { if(!in_header) SeenDefineFunctionArg(yytext); }
<CPP_DEFINE_ARGS>","                        { }
<CPP_DEFINE_ARGS>{W}+                       { }
<CPP_DEFINE_ARGS>\\{N}                      { parse_line++; }
<CPP_DEFINE_ARGS>")"                        { define_value=(char*)1; BEGIN(CPP_DEFINE_BODY); }
<CPP_DEFINE_ARGS>.|\n                       { BEGIN(INITIAL); /* Ignore bad pre-processor output */ }

<CPP_DEFINE_BODY>{L}({L}|{D})*              { if(!in_header) {if(!define_value) define_value=CopyString(yytext); else define_value=(char*)1;} }
<CPP_DEFINE_BODY>0[xX]{H}+{IS}?             { if(!in_header) {if(!define_value) define_value=CopyString(yytext); else define_value=(char*)1;} }
<CPP_DEFINE_BODY>0{D}+{IS}?                 { if(!in_header) {if(!define_value) define_value=CopyString(yytext); else define_value=(char*)1;} }
<CPP_DEFINE_BODY>{S}?{D}+{IS}?              { if(!in_header) {if(!define_value) define_value=CopyString(yytext); else define_value=(char*)1;} }
<CPP_DEFINE_BODY>{S}?{D}+{E}{FS}?           { if(!in_header) {if(!define_value) define_value=CopyString(yytext); else define_value=(char*)1;} }
<CPP_DEFINE_BODY>{S}?{D}*"."{D}+({E})?{FS}? { if(!in_header) {if(!define_value) define_value=CopyString(yytext); else define_value=(char*)1;} }
<CPP_DEFINE_BODY>{S}?{D}+"."{D}*({E})?{FS}? { if(!in_header) {if(!define_value) define_value=CopyString(yytext); else define_value=(char*)1;} }
<CPP_DEFINE_BODY>\'(\\.|[^'\r\n])*\'        { if(!in_header) {if(!define_value) define_value=CopyString(yytext); else define_value=(char*)1;} }
<CPP_DEFINE_BODY>\"(\\.|[^"\r\n])*\"        { if(!in_header) {if(!define_value) define_value=CopyString(yytext); else define_value=(char*)1;} }
<CPP_DEFINE_BODY>.                          { }
<CPP_DEFINE_BODY>\\{N}                      { parse_line++; }
<CPP_DEFINE_BODY>{N}                        { parse_line++; if(define_value>(char*)1) SeenDefineValue(define_value); BEGIN(INITIAL); }

 /* Preprocessor directive #include. */

<CPP_INCLUDE>[<"][^>"]+[>"]     { SeenInclude(yytext); }
<CPP_INCLUDE>.                  { }
<CPP_INCLUDE>\\{N}              { parse_line++; }
<CPP_INCLUDE>{N}                { parse_line++; BEGIN(INITIAL); }

<CPP_INC_FILE>"\""[^"]+"\""     { inc_file=CopyString(yytext+1); inc_file[strlen(inc_file)-1]=0;
                                  inc_file_flags=0;
                                  if(!parse_file) inc_file_flags=-1;
                                  if(!parse_file) parse_file=inc_file;
                                  GetCurrentComment();
                                  BEGIN(CPP_INC_FLAGS); }
<CPP_INC_FILE>.|\n              { BEGIN(INITIAL); /* Ignore bad pre-processor output */ }

<CPP_INC_FLAGS>{W}+{D}+         { inc_file_flags+=1<<atoi(yytext); }
<CPP_INC_FLAGS>{W}*{N}          { if(inc_file_flags&6 || inc_file_flags==-1 || *inc_file=='<')
                                     parse_file=SeenFileChange(inc_file,inc_file_flags);
                                  BEGIN(INITIAL); }
<CPP_INC_FLAGS>.                { BEGIN(INITIAL); /* Ignore bad pre-processor output */ }

 /* GNU C strangeness. */

("__alignof"|"__alignof__")        { yylval="alignof"  ; return(SIZEOF);   }

("__signed"|"__signed__")          { yylval="signed"   ; return(SIGNED);   }
("__unsigned"|"__unsigned__")      { yylval="unsigned" ; return(UNSIGNED); }

("__volatile"|"__volatile__")      { yylval="volatile" ; return(VOLATILE); }
("__const"|"__const__")            { yylval="const"    ; return(CONST);    }

("__inline"|"__inline__")          { yylval="inline"   ; return(INLINE);   }

("asm"|"__asm"|"__asm__")          { yylval="asm"      ; return(ASM);      }

"__label__"                        { BEGIN(GNU_LABEL); }
<GNU_LABEL>[^;]+                   { }
<GNU_LABEL>";"                     { BEGIN(INITIAL); }

"__builtin_va_arg"                 { gnu_va_arg_depth=0; BEGIN(GNU_VA_ARG); yylval="__builtin_va_arg"; return(IDENTIFIER); }
<GNU_VA_ARG>"("                    { gnu_va_arg_depth++; if(gnu_va_arg_depth==1) { yylval="("; return('('); } }
<GNU_VA_ARG>[^()]+                 { }
<GNU_VA_ARG>")"                    { if(--gnu_va_arg_depth==0) { BEGIN(INITIAL); yylval=")"; return(')'); } }

("__attribute"|"__attribute__")    { gnu_att_depth=0; BEGIN(GNU_ATTRIBUTE); }
<GNU_ATTRIBUTE>"("                 { gnu_att_depth++; }
<GNU_ATTRIBUTE>[^()]+              { }
<GNU_ATTRIBUTE>")"                 { if(--gnu_att_depth==0) BEGIN(INITIAL); }

("typeof"|"__typeof"|"__typeof__") { gnu_typ_depth=0; BEGIN(GNU_TYPEOF); }
<GNU_TYPEOF>"("                    { gnu_typ_depth++; }
<GNU_TYPEOF>[^()]+                 { }
<GNU_TYPEOF>")"                    { if(--gnu_typ_depth==0) {BEGIN(INITIAL); yylval="typeof"; return(TYPE_NAME);} }

"__extension__"                    { }

("__restrict"|"__restrict__")      { }

"__FUNCTION__"                     { yylval="__FUNCTION__"; return(STRING_LITERAL); }

"__builtin_va_list"                { yylval="__builtin_va_list" ; return(TYPE_NAME); }

 /* VOS char_varying type */

"char_varying"{W}*"("{W}*{D}+{W}*")" { yylval=CopyString(yytext); BEGIN(CHAR_VARYING); return(CHAR); }
<CHAR_VARYING>.|\n                   { unput(*yytext); yylval=""; BEGIN(INITIAL); return('*'); }

 /* C language keywords. */

"auto"                  { yylval="auto"    ; return(AUTO);     }
"break"                 { yylval="break"   ; return(BREAK);    }
"case"                  { yylval="case"    ; return(CASE);     }
"char"                  { yylval="char"    ; return(CHAR);     }
"const"                 { yylval="const"   ; return(CONST);    }
"continue"              { yylval="continue"; return(CONTINUE); }
"default"               { yylval="default" ; return(DEFAULT);  }
"do"                    { yylval="do"      ; return(DO);       }
"double"                { yylval="double"  ; return(DOUBLE);   }
"else"                  { yylval="else"    ; return(ELSE);     }
"enum"                  { yylval="enum"    ; return(ENUM);     }
"extern"                { yylval="extern"  ; return(EXTERN);   }
"float"                 { yylval="float"   ; return(FLOAT);    }
"for"                   { yylval="for"     ; return(FOR);      }
"goto"                  { yylval="goto"    ; return(GOTO);     }
"if"                    { yylval="if"      ; return(IF);       }
"int"                   { yylval="int"     ; return(INT);      }
"inline"                { yylval="inline"  ; return(INLINE);   }
"long"                  { yylval="long"    ; return(LONG);     }
"register"              { yylval="register"; return(REGISTER); }
"return"                { yylval="return"  ; return(RETURN);   }
"short"                 { yylval="short"   ; return(SHORT);    }
"signed"                { yylval="signed"  ; return(SIGNED);   }
"sizeof"                { yylval="sizeof"  ; return(SIZEOF);   }
"static"                { yylval="static"  ; return(STATIC);   }
"struct"                { yylval="struct"  ; return(STRUCT);   }
"switch"                { yylval="switch"  ; return(SWITCH);   }
"typedef"               { yylval="typedef" ; return(TYPEDEF);  }
"union"                 { yylval="union"   ; return(UNION);    }
"unsigned"              { yylval="unsigned"; return(UNSIGNED); }
"void"                  { yylval="void"    ; return(VOID);     }
"volatile"              { yylval="volatile"; return(VOLATILE); }
"while"                 { yylval="while"   ; return(WHILE);    }

 /* C language operators. */

"..."                   { yylval="...";  return(ELLIPSES);     }
">>="                   { yylval=">>=";  return(RIGHT_ASSIGN); }
"<<="                   { yylval="<<=";  return(LEFT_ASSIGN);  }
"+="                    { yylval="+=";   return(ADD_ASSIGN); }
"-="                    { yylval="-=";   return(SUB_ASSIGN); }
"*="                    { yylval="*=";   return(MUL_ASSIGN); }
"/="                    { yylval="/=";   return(DIV_ASSIGN); }
"%="                    { yylval="%=";   return(MOD_ASSIGN); }
"&="                    { yylval="&=";   return(AND_ASSIGN); }
"^="                    { yylval="^=";   return(XOR_ASSIGN); }
"|="                    { yylval="|=";   return(OR_ASSIGN);  }
">>"                    { yylval=">>";   return(RIGHT_SHIFT);}
"<<"                    { yylval="<<";   return(LEFT_SHIFT); }
"++"                    { yylval="++";   return(INC_OP); }
"--"                    { yylval="--";   return(DEC_OP); }
"->"                    { yylval="->";   return(PTR_OP); }
"&&"                    { yylval="&&";   return(AND_OP); }
"||"                    { yylval="||";   return(OR_OP); }
"<="                    { yylval="<=";   return(LE_OP); }
">="                    { yylval=">=";   return(GE_OP); }
"=="                    { yylval="==";   return(EQ_OP); }
"!="                    { yylval="!=";   return(NE_OP); }
";"{W}*("/*"|"//")      { yylval="; ";   push_past=2;
                          comment_init_state=INITIAL; BEGIN(yytext[strlen(yytext)-1]=='*'?START_COMMENT:START_COMMENT_CPP); }
";"                     { yylval=";";    return(';'); }
"{"                     { yylval="{";    return('{'); }
"};"{W}*("/*"|"//")     { yylval="}; ";  push_past=3;
                          comment_init_state=INITIAL; BEGIN(yytext[strlen(yytext)-1]=='*'?START_COMMENT:START_COMMENT_CPP); }
"},"{W}*("/*"|"//")     { yylval="}, ";  push_past=3;
                          comment_init_state=INITIAL; BEGIN(yytext[strlen(yytext)-1]=='*'?START_COMMENT:START_COMMENT_CPP); }
"}"                     { yylval="}";    return('}'); }
","{W}*("/*"|"//")      { yylval=", ";   push_past=2;
                          comment_init_state=INITIAL; BEGIN(yytext[strlen(yytext)-1]=='*'?START_COMMENT:START_COMMENT_CPP); }
","                     { yylval=",";    return(','); }
":"                     { yylval=":";    return(':'); }
"="                     { yylval="=";    return('='); }
"("                     { yylval="(";    return('('); }
")"{W}*("/*"|"//")      { yylval=") ";   push_past=2;
                          comment_init_state=INITIAL; BEGIN(yytext[strlen(yytext)-1]=='*'?START_COMMENT:START_COMMENT_CPP); }
")"                     { yylval=")";    return(')'); }
"["                     { yylval="[";    return('['); }
"]"                     { yylval="]";    return(']'); }
"."                     { yylval=".";    return('.'); }
"&"                     { yylval="&";    return('&'); }
"!"                     { yylval="!";    return('!'); }
"~"                     { yylval="~";    return('~'); }
"-"                     { yylval="-";    return('-'); }
"+"                     { yylval="+";    return('+'); }
"*"                     { yylval="*";    return('*'); }
"/"                     { yylval="/";    return('/'); }
"%"                     { yylval="%";    return('%'); }
"<"                     { yylval="<";    return('<'); }
">"                     { yylval=">";    return('>'); }
"^"                     { yylval="^";    return('^'); }
"|"                     { yylval="|";    return('|'); }
"?"                     { yylval="?";    return('?'); }

 /* Variable / Function / Type names */

{L}({L}|{D})*           {
                          yylval=CopyString(yytext);
                          if(IsAScopeVariable(yytext))
                             return(IDENTIFIER);
                          else
                             if(IsATypeName(yytext))
                                return(TYPE_NAME);
                             else
                                return(IDENTIFIER);
                        }

 /* Literals */

0[xX]{H}+{IS}?                { yylval=CopyString(yytext); return(LITERAL); }
0[xX]{H}+"."{H}?[pP]{D}+{FS}? { yylval=CopyString(yytext); return(LITERAL); }
0{D}+{IS}?                    { yylval=CopyString(yytext); return(LITERAL); } 
{D}+{IS}?                     { yylval=CopyString(yytext); return(LITERAL); }
'(\\.|[^\\'])+'               { yylval=CopyString(yytext); return(LITERAL); }
"L"'(\\.|[^\\'])+'            { yylval=CopyString(yytext); return(LITERAL); }

{D}+{E}{FS}?                  { yylval=CopyString(yytext); return(LITERAL); }
{D}*"."{D}+({E})?{FS}?        { yylval=CopyString(yytext); return(LITERAL); }
{D}+"."{D}*({E})?{FS}?        { yylval=CopyString(yytext); return(LITERAL); }

\"(\\.|[^\\"])*\"             { yylval=CopyString(yytext); return(STRING_LITERAL); }
"L"\"(\\.|[^\\"])*\"          { yylval=CopyString(yytext); return(STRING_LITERAL); }

 /* Other text. */

{N}                     { parse_line++; }
.                       { /* Ignore bad characters */ }

%%

/*++++++++++++++++++++++++++++++++++++++
  Reset the Lexer, ready for the next file.
  ++++++++++++++++++++++++++++++++++++++*/

void ResetLexer(void)
{
 parse_file=NULL;
 parse_line=0;
 inc_file_flags=0;
 inc_file=NULL;
 define_value=NULL;
 comment_init_state=INITIAL;
 gnu_va_arg_depth=0;
 gnu_att_depth=0;
 gnu_typ_depth=0;
 push_past=0;
 BEGIN(INITIAL);
}
