/*  $VER: vbcc (vbc.h) V0.4     */

#include "supp.h"

#define eval_constn(a) eval_const(&a->val,a->ntyp->flags)

struct identifier_list{
    char *identifier;
    int length;
    struct identifier_list *next;
};
struct struct_identifier{
/*    int flags;*/
    char *identifier;
    struct struct_declaration *sd;
    struct struct_identifier *next;
};

#ifndef NODES
struct node{
    int flags,lvalue,sidefx;
    struct Typ *ntyp;
    struct node *left;
    struct node *right;
    struct argument_list *alist;
    char *identifier;
    union atyps val;
    struct obj o;
};

typedef struct node *np;

#define NODES sizeof(struct node)
#endif

struct argument_list{
    np  arg;
    struct argument_list *next;
};


#define MAXI 100 /* maximale Laenge von Identifiers in Bytes    */
#define MAXINPUT 2000    /* maximale Laenge einer Eingabezeile in Bytes  */
#define MAXN 64 /* maximale Verschachtelung von Bloecken */

extern void free_fi(struct function_info *);
extern struct Typ *arith_typ(struct Typ*,struct Typ *);
extern void insert_const(np);
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
extern void cpbez(char *m,int ckw),cpnum(char *m),killsp(void);
extern struct struct_declaration *add_sd(struct struct_declaration *);
extern void add_sl(struct struct_declaration *,struct struct_list (*)[]);
extern void free_sd(struct struct_declaration *);
extern void prl(FILE *,struct struct_declaration *);
extern char *add_identifier(char *,int);
extern struct Typ *declarator(struct Typ *),*direct_declarator(struct Typ *),
           *pointer(struct Typ *),*declaration_specifiers(void);
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
extern struct Var *first_var[MAXN],*last_var[MAXN],*merk_varf,*merk_varl;
extern void free_var(struct Var *);
extern void var_declaration(void);
extern int storage_class_specifiers(void);
extern void enter_block(void),leave_block(void);
extern struct Var *find_var(char *,int);
extern struct Var *add_var(char *,struct Typ *,int,struct const_list *);

extern int usz;

extern void gen_IC(np,int,int),convert(np,int),gen_label(int),savescratch(int,struct IC *,int);
struct regargs_list{
    struct regargs_list *next;
    int reg;
    struct Var *v;
};
#ifdef HAVE_REGPARMS
extern zlong push_args(struct argument_list *,struct struct_declaration *,int,struct regargs_list **,struct reg_handle *);
#else
extern zlong push_args(struct argument_list *,struct struct_declaration *,int,struct regargs_list **);
#endif
extern int allocreg(int,int);
extern void free_reg(int);

extern FILE *out,*ic1,*ic2,*ppout;

extern void statement(void),labeled_statement(void),if_statement(void);
extern void switch_statement(void),while_statement(void),for_statement(void);
extern void do_statement(void),goto_statement(void),continue_statement(void);
extern void break_statement(void),return_statement(void);
extern void expression_statement(void),compound_statement(void);
extern void translation_unit(void);
extern int main(int, char *[]);
extern int nocode,registerpri,looppri,currentpri;

extern np makepointer(np);

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

#define MAXCF 30
extern int c_flags[MAXCF];
extern char *c_flags_name[MAXCF];
extern union ppi c_flags_val[MAXCF];

extern FILE *open_out(char *,char *);

extern char *inname;

extern void gen_vars(struct Var *);

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

extern int afterlabel;

extern int errors;

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

extern int read_new_line;
extern int float_used;
extern char *cur_func;
extern int line;
extern void free_clist(struct const_list *);
extern struct const_list *first_clist,*last_clist;
extern struct Var *regsbuf[MAXR+1];
extern int regbnesting[MAXR+1];

