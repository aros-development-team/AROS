/*  $VER: vbcc (vbc.h) V0.4     */

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>

#define CHAR 1
#define SHORT 2
#define INT 3
#define LONG 4
#define FLOAT 5
#define DOUBLE 6
#define VOID 7
#define POINTER 8
#define ARRAY 9
#define STRUCT 10
#define UNION 11
#define ENUM 12
#define FUNKT 13
#define UNSIGNED 16
#define CONST 64
#define VOLATILE 128
#define UNCOMPLETE 256
#define STRINGCONST 512

#include "machine.h"

#define eval_constn(a) eval_const(&a->val,a->ntyp->flags)

/*  Zusaetzliche Informationen ueber Funktionen (z.Z. Rumpf fuer Inlining)  */
struct function_info{
    struct IC *first_ic,*last_ic;
    struct Var *vars;
};

struct Typ{
    int flags;
    struct Typ *next;
    struct struct_declaration *exact;
    zlong size;
};
#define TYPS sizeof(struct Typ)

struct identifier_list{
    char *identifier;
    int length;
    struct identifier_list *next;
};
struct Var{
    int storage_class,reg,priority,flags;
    char *identifier;
    int nesting,index;
    zlong offset;
    struct Typ *vtyp;
    struct const_list *clist;
    struct Var *next;
    struct function_info *fi;
    struct Var *inline_copy;
};
#define USEDASSOURCE 1
#define USEDASDEST 2
#define DEFINED 4
#define USEDASADR 8
#define GENERATED 16
#define CONVPARAMETER 32
#define TENTATIVE 64
#define USEDBEFORE 128
#define INLINEV 256
#define PRINTFLIKE 512
#define SCANFLIKE 1024
#define NOTTYPESAFE 2048
#define DNOTTYPESAFE 4096
#define REGPARM 8192

#define SLSIZE 32   /*  struct_lists in diesen Abstaenden realloc'en    */

struct struct_list{
    char *identifier;
    struct Typ *styp;
    int storage_class,reg;
};
struct struct_declaration{
    int count;
    struct struct_declaration *next;
    struct struct_list (*sl)[];
};

struct struct_identifier{
/*    int flags;*/
    char *identifier;
    struct struct_declaration *sd;
    struct struct_identifier *next;
};

struct obj{
    int flags,reg;
    struct Var *v;
    struct AddressingMode *am;
    union atyps{
        zchar vchar;
        zuchar vuchar;
        zshort vshort;
        zushort vushort;
        zint vint;
        zuint vuint;
        zlong vlong;
        zulong vulong;
        zfloat vfloat;
        zdouble vdouble;
        zpointer vpointer;
    }val;
};

struct node{
    int flags,lvalue,sidefx;
    struct Typ *ntyp;
    struct node *left;
    struct node *right;
    struct argument_list *alist;
    char *identifier;
    union atyps val;
    struct obj o;
/*  es muss noch sowas wie struct internal_object * dazu    */
};

typedef struct node *np;

#define NODES sizeof(struct node)

#define KOMMA 1
#define ASSIGN 2
#define ASSIGNADD 3
#define ASSIGNSUB 4
#define ASSIGNMULT 5
#define ASSIGNDIV 6
#define ASSIGNMOD 7
#define ASSIGNAND 8
#define ASSIGNXOR 9
#define ASSIGNOR 10
#define ASSIGNLSHIFT 11
#define ASSIGNRSHIFT 12
#define COND 13
#define LOR 14
#define LAND 15
#define OR 16
#define XOR 17
#define AND 18
#define EQUAL 19
#define INEQUAL 20
#define LESS 21
#define LESSEQ 22
#define GREATER 23
#define GREATEREQ 24
#define LSHIFT 25
#define RSHIFT 26
#define ADD 27
#define SUB 28
#define MULT 29
#define DIV 30
#define MOD 31
#define NEGATION 32
#define KOMPLEMENT 33
#define PREINC 34
#define POSTINC 35
#define PREDEC 36
#define POSTDEC 37
#define MINUS 38
#define CONTENT 39
#define ADDRESS 40
#define CAST 41
#define CALL 42
#define INDEX 43
#define DPSTRUCT 44
#define DSTRUCT 45
#define IDENTIFIER 46
#define CEXPR 47
#define STRING 48
#define MEMBER 49
#define CONVCHAR 50
#define CONVSHORT 51
#define CONVINT 52
#define CONVLONG 53
#define CONVFLOAT 54
#define CONVDOUBLE 55
#define CONVVOID 56
#define CONVPOINTER 57
#define CONVUCHAR 58
#define CONVUSHORT 59
#define CONVUINT 60
#define CONVULONG 61
#define ADDRESSA 62
#define FIRSTELEMENT 63
#define PMULT 64
#define ALLOCREG 65
#define FREEREG 66
#define PCEXPR 67
#define TEST 68
#define LABEL 69
#define BEQ 70
#define BNE 71
#define BLT 72
#define BGE 73
#define BLE 74
#define BGT 75
#define BRA 76
#define COMPARE 77
#define PUSH 78
#define POP 79
#define ADDRESSS 80
#define ADDI2P 81
#define SUBIFP 82
#define SUBPFP 83
#define PUSHREG 84
#define POPREG 85
#define POPARGS 86
#define SAVEREGS 87
#define RESTOREREGS 88
#define ILABEL 89
#define DC 90
#define ALIGN 91
#define COLON 92
#define GETRETURN 93
#define SETRETURN 94
#define MOVEFROMREG 95
#define MOVETOREG 96
#define NOP 97

struct argument_list{
    np  arg;
    struct argument_list *next;
};

#define AUTO 1
#define REGISTER 2
#define STATIC 3
#define EXTERN  4
#define TYPEDEF 5

#define MAXI 100 /* maximale Laenge von Identifiers in Bytes    */
#define MAXINPUT 2000    /* maximale Laenge einer Eingabezeile in Bytes  */
#define MAXN 30 /* maximale Verschachtelung von Bloecken */
#define MAXM 100 /* maximale Anzahl an Bloecken pro Funktion (grob) */

#define arith(c) ((c)>=CHAR&&(c)<=DOUBLE)

extern char *typname[];
extern zlong sizetab[16];
extern char *storage_class_name[];
extern char *ename[];

/* Tabelle fuer alignment requirements, maschinenabhaengig */
extern zlong align[16],maxalign;

extern void error(int,...);

#define ierror(a) error(158,(a),__LINE__,FILE_)

extern void free_fi(struct function_info *);
extern struct Typ *arith_typ(struct Typ*,struct Typ *);
extern void insert_const(np);
extern void insert_const2(union atyps *,int);
extern int int_erw(int);
extern int type_expression(np),compare_pointers(struct Typ *,struct Typ *,int),
    compare_sd(struct struct_declaration *,struct struct_declaration *);
extern np identifier_expression(void),constant_expression(void),string_expression(void),
   postfix_expression(void),unary_expression(void),cast_expression(void),
   multiplicative_expression(void),additive_expression(void),
   shift_expression(void),relational_expression(void),equality_expression(void),
   and_expression(void),exclusive_or_expression(void),
   inclusive_or_expression(void),logical_and_expression(void),
   logical_or_expression(void),conditional_expression(void),
   assignment_expression(void),expression(void),primary_expression(void);
/* puh  */
extern void pre(FILE *,np),pra(FILE *,struct argument_list *);
extern void free_expression(np),free_alist(struct argument_list *);
extern void prd(FILE *,struct Typ *),freetyp(struct Typ *);
extern void cpbez(char *m,int ckw),cpnum(char *m),killsp(void);
extern struct struct_declaration *add_sd(struct struct_declaration *);
extern void add_sl(struct struct_declaration *,struct struct_list (*)[]);
extern void free_sd(struct struct_declaration *);
extern void prl(FILE *,struct struct_declaration *);
extern char *add_identifier(char *,int);
extern struct Typ *declarator(struct Typ *),*direct_declarator(struct Typ *),
           *pointer(struct Typ *),*declaration_specifiers(void),
           *clone_typ(struct Typ *);
extern int declaration(int),type_uncomplete(struct Typ *);
extern struct struct_declaration *find_struct(char *,int);
extern void add_struct_identifier(char *,struct struct_declaration *);
extern void free_si(struct struct_identifier *);
extern char *s,*ident;
extern char string[MAXINPUT+2],number[MAXI],buff[MAXI];
extern struct struct_declaration *first_sd[MAXN],*last_sd[MAXN],*merk_sdf,*merk_sdl;
extern struct struct_identifier *first_si[MAXN],*last_si[MAXN],*merk_sif,*merk_sil;
extern struct identifier_list *first_ilist[MAXN],*last_ilist[MAXN],*merk_ilistf,*merk_ilistl;
extern void free_ilist(struct identifier_list *);
extern int nesting;
extern char *empty;
extern struct Var *first_var[MAXN],*last_var[MAXN],*merk_varf,*merk_varl;
extern struct Var *add_var(char *,struct Typ *,int,struct const_list *);
extern void free_var(struct Var *);
extern void var_declaration(void);
extern int storage_class_specifiers(void);
extern void enter_block(void),leave_block(void);
extern struct Var *find_var(char *,int);
extern zlong szof(struct Typ *);

extern void eval_const(union atyps *,int);
extern zchar vchar; extern zuchar vuchar;
extern zshort vshort; extern zushort vushort;
extern zint vint; extern zuint vuint;
extern zlong vlong; extern zulong vulong;
extern zfloat vfloat; extern zdouble vdouble;
extern zpointer vpointer;

extern int usz;

#ifndef DEBUG
extern int DEBUG;
#endif

/*  Liste fuer Variablen, die benutzt/veraendert werden.    */
struct varlist{
    struct Var *v;
    int flags;
};

#define VLS sizeof(struct varlist)

struct IC{
    struct IC *prev,*next;
    int code,typf,defindex,expindex,copyindex;
    int change_cnt,use_cnt,line;
    struct varlist *change_list,*use_list;
    struct obj q1,q2,z;
    char *file;
};

#define ICS sizeof(struct IC)
#define KONST 1     /*  KONST muss immer am kleinsten sein, um beim swappen */
                    /*  fuer available_expressions und Konstanten nach      */
                    /*  rechts nicht in eine Endlosschleife zu kommen       */
#define VAR 2
#define SCRATCH 8
#define STACK 16
#define DREFOBJ 32
#define REG 64
#define VARADR 128
#define DONTREGISTERIZE 256

extern struct IC *first_ic,*last_ic;
extern int regs[MAXR+1],regsa[MAXR+1],regused[MAXR+1],regscratch[MAXR+1];
extern zlong regsize[MAXR+1];
extern struct Var *regsv[MAXR+1],*regsbuf[MAXR+1];
extern int regbnesting[MAXR+1];

extern void add_IC(struct IC *),free_IC(struct IC *),insert_IC(struct IC *,struct IC *);
extern void gen_IC(np,int,int),convert(np,int),gen_label(int),savescratch(int,struct IC *,int);
struct regargs_list{
    struct regargs_list *next;
    int reg;
    struct Var *v;
};
extern zlong push_args(struct argument_list *,struct struct_declaration *,int,struct regargs_list **);
extern int regok(int,int,int),allocreg(int,int),freturn(struct Typ *);
extern int icok(struct IC *);
extern void free_reg(int);
extern void pric(FILE *,struct IC *),pric2(FILE *,struct IC *);
extern char *regnames[];
extern void probj(FILE *,struct obj *,int);

extern void printzl(FILE *,zlong),printzul(FILE *,zulong),printzd(FILE *,zdouble);
extern void printval(FILE *,union atyps *,int,int);

extern int label;

extern FILE *out,*ic1,*ic2,*ppout;

extern void statement(void),labeled_statement(void),if_statement(void);
extern void switch_statement(void),while_statement(void),for_statement(void);
extern void do_statement(void),goto_statement(void),continue_statement(void);
extern void break_statement(void),return_statement(void);
extern void expression_statement(void),compound_statement(void),raus(void);
extern void translation_unit(void);
extern int main(int, char *[]);
extern int nocode,registerpri,looppri,currentpri;

extern void *mymalloc(size_t);

extern np makepointer(np);

extern int must_convert(np,int);

extern int switch_typ,switch_count,switch_act;
struct llist{
    char *identifier;
    int label,flags,switch_count;
    struct llist *next;
    union atyps val;
};
#define LABELDEFINED 1
#define LABELUSED 2
#define LABELDEFAULT 4
#define LSIZE sizeof(struct llist)
extern struct llist *first_llist,*last_llist;
extern struct llist *find_label(char *),*add_label(char *);
extern void free_llist(struct llist *);

extern int endok,return_label,return_value,break_label;
extern struct Var *return_var;
extern struct Typ *return_typ;
extern zlong local_offset[MAXN];

extern void scratch_var(struct obj *,int),get_scratch(struct obj *,int,int);
extern struct obj gen_cond(int,int,int);

extern void simple_regs(void);

union ppi {char *p;long l;void (*f)(char *);};

#define USEDFLAG 1
#define STRINGFLAG 2
#define VALFLAG 4
#define FUNCFLAG 8

#define MAXCF 30
extern int c_flags[MAXCF];
extern char *c_flags_name[MAXCF];
extern union ppi c_flags_val[MAXCF];

extern int g_flags[MAXGF];
extern char *g_flags_name[MAXGF];
extern union ppi g_flags_val[MAXGF];


extern FILE *open_out(char *,char *);

extern char *inname;

extern void gen_code(FILE *,struct IC *,struct Var *,zlong);

extern int init_cg(void);
extern void cleanup_cg(FILE *);

extern void gen_vars(struct Var *);

extern int dangerous_IC(struct IC *);

extern zlong max_offset;

extern int function_calls;

struct const_list{
    union atyps val;
    np tree;
    struct const_list *other,*next;
};
extern struct const_list *first_clist,*last_clist;
#define CLS sizeof(struct const_list)

/*  Format der Tabelle fuer Fehlermeldungen */
struct err_out{
    char *text;
    int  flags;
};
/*  Flags fuer err_out.flags    */
#define ERROR       1
#define WARNING     2
#define ANSIV       4
#define INTERNAL    8
#define FATAL      16
#define MESSAGE    32
#define DONTWARN   64
#define PREPROC   128
#define NOLINE    256
#define INFUNC    512
#define INIC     1024

extern struct err_out err_out[];
extern int err_num;

extern void gen_dc(FILE *,int,struct const_list *);
extern void gen_ds(FILE *,zlong,struct Typ *),gen_var_head(FILE *,struct Var *);
extern void gen_align(FILE *,zlong);
extern void free_clist(struct const_list *);

extern void remove_IC(struct IC *);

extern zlong t_min[];
extern zulong t_max[];
extern zlong char_bit;

extern int afterlabel;

extern int goto_used;

extern int errors;
extern int ic_count;


/*  fuer den Praeprozessor  */

#define MAXPPINPUT 2000     /*  maximale Laenge einer Eingabezeile  */
#define MAXINCNESTING 50    /*  maximale Verschachtelung von Includes   */

extern FILE *in[MAXINCNESTING];    /*  Sourcefiles     */
extern int zn[MAXINCNESTING];      /*  Zeilennummern   */
extern char *filename[MAXINCNESTING];   /*  Filenamen   */
extern int incnesting;             /*  aktuelle Verschachtelungstiefe  */
extern unsigned long linenr;                 /*  Zeilennummer */

#define MAXINCPATHS 20      /*  maximale Anzahl der Includepfade    */

extern char *incpath[MAXINCPATHS];   /*  Includepfade    */
                                            /*  Rest ist NULL   */

extern int incpathc;     /*  Anzahl der Includepfade     */

extern int pp_init(void);
extern void pp_free(void);
extern int pp_include(char *filename);
extern int pp_nextline(void);
extern int pp_define(char *text);

extern int only_inline;

extern int read_new_line;
extern int float_used;
extern char *cur_func,errfname[FILENAME_MAX+1];
extern int line,fline;
extern struct IC *err_ic;

extern int multiple_ccs;

extern int shortcut(int, int);

