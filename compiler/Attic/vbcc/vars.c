/*  $VER: vbcc (vars.c) V0.4    */

#include "vbc.h"

char *typname[]={"strange","char","short","int","long","float","double","void",
                 "pointer","array","struct","union","enum","function"};
char *storage_class_name[]={"strange","auto","register","static","extern","typedef"};

char *ename[]={"strange","sequence","move","set+","set-","set*","set/","set%",
               "set&","set^","set|","set<<","set>>","?:","lor","land","or",
               "eor","and","equal","unequal","lt","le","gt","ge","lsl",
               "lsr","add","sub","mul","div","mod","negate",
               "not","preinc","postinc","predec","postdec","neg",
               "dref-pointer","address-of","cast","call","index",
               "dref-struct-pointer","dref-struct","identifier","constant",
               "string","member",
                "convert-char","convert-short","convert-int","convert-long",
                "convert-float","convert-double","convert-void","convert-pointer",
                "convert-uchar","convert-ushort","convert-uint","convert-ulong",
                "address-of-array","first-element-of-array","pmult",
                "allocreg","freereg","pconstant","test","label","beq","bne",
                "blt","bge","ble","bgt","bra","compare","push","pop",
                "address-of-struct","add-int-to-pointer","sub-int-from-pointer",
                "sub-pointer-from-pointer","push-reg","pop-reg","pop-args",
                "save-regs","restore-regs","identifier-label","dc","align",
                "colon","get-return","set-return","move-from-reg","move-to-reg",
                "nop"};

char *s,*ident;
char string[MAXINPUT+2],number[MAXI],buff[MAXI];
struct struct_declaration *first_sd[MAXN],*last_sd[MAXN],*merk_sdf,*merk_sdl;
struct struct_identifier *first_si[MAXN],*last_si[MAXN],*merk_sif,*merk_sil;
struct identifier_list *first_ilist[MAXN],*last_ilist[MAXN],*merk_ilistf,*merk_ilistl;
struct llist *first_llist,*last_llist;
int nesting;
char *empty="";
struct Var *first_var[MAXN],*last_var[MAXN],*merk_varf,*merk_varl;

zchar vchar; zuchar vuchar;
zshort vshort; zushort vushort;
zint vint; zuint vuint;
zlong vlong; zulong vulong;
zfloat vfloat; zdouble vdouble;
zpointer vpointer;

#ifndef DEBUG
int DEBUG;
#endif

int label;

FILE *out,*ic1,*ic2,*ppout;

int nocode;
int registerpri=200,currentpri,looppri=10;
int return_label,return_value,break_label,switch_typ,switch_count,switch_act;
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

int regs[MAXR+1],regused[MAXR+1];
struct Var *regsv[MAXR+1],*regsbuf[MAXR+1];
int regbnesting[MAXR+1];

char *inname;

zlong max_offset;
int function_calls;

struct const_list *first_clist,*last_clist;

int afterlabel;

int goto_used;

struct err_out err_out[]={
#include "errors.h"
"",0
};
int err_num=sizeof(err_out)/sizeof(struct err_out)-1;

char *cur_func="shouldn't happen!";
int ic_count;
int lastlabel,fline;
int read_new_line;
int only_inline;
struct IC *err_ic;

int multiple_ccs;

char *copyright="vbcc V0.5a (c) in 1995-97 by Volker Barthelmann\n";

