/*  $VER: vbcc (vars.c) V0.4    */

#include "vbc.h"

char *s,*ident;
char string[MAXINPUT+2],number[MAXI],buff[MAXI];
struct struct_declaration *first_sd[MAXN],*last_sd[MAXN],*merk_sdf,*merk_sdl;
struct struct_identifier *first_si[MAXN],*last_si[MAXN],*merk_sif,*merk_sil;
struct identifier_list *first_ilist[MAXN],*last_ilist[MAXN],*merk_ilistf,*merk_ilistl;
struct llist *first_llist,*last_llist;
int nesting;
struct Var *first_var[MAXN],*last_var[MAXN],*merk_varf,*merk_varl;


FILE *out,*ic1,*ic2,*ppout;

int nocode;
int registerpri=200,currentpri,looppri=10;
int return_value,break_label,switch_typ,switch_count,switch_act;
struct Typ *return_typ;
struct Var *return_var;
zlong local_offset[MAXN];

int c_flags[MAXCF]={
    VALFLAG,STRINGFLAG,0,0,
    VALFLAG,0,0,0,
    VALFLAG,FUNCFLAG,FUNCFLAG,VALFLAG,
    VALFLAG,0,0,0,
    0,0,0,0,
    0,0,0,0,0,
    VALFLAG
};
char *c_flags_name[MAXCF]={
    "O","o","ic1","ic2",
    "debug","noasm","quiet","ansi",
    "maxerrors","dontwarn","warn","maxoptpasses",
    "inline-size","nested-comments","cpp-comments","macro-redefinition",
    "no-trigraphs","no-preprocessor","E","dontkeep-initialized-data",
    "strip-path","fp-associative","iso","no-alias-opt","no-multiple-ccs",
    "unroll-size"
};
union ppi c_flags_val[MAXCF];

char *inname;

struct Var *regsbuf[MAXR+1];
int regbnesting[MAXR+1];

struct const_list *first_clist,*last_clist;

int afterlabel;

struct err_out err_out[]={
#include "errors.h"
"",0
};
int err_num=sizeof(err_out)/sizeof(struct err_out)-1;

char *cur_func="shouldn't happen!";
int read_new_line;


char *copyright="vbcc V0.5g (c) in 1995-97 by Volker Barthelmann";

