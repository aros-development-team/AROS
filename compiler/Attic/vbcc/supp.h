#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>

/* typenames */
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

#define NQ 15   /* f&NQ gives type without any qualifiers */
#define NU 31   /* f&NU gives type without any qualifiers but UNSIGNED */

/* type-qualifiers */
#define UNSIGNED 16
#define CONST 64
#define VOLATILE 128
#define UNCOMPLETE 256
#define STRINGCONST 512

/*  macro for internal errors */
#define ierror(a) error(-1,(a),__LINE__,FILE_)

#include "machine.h"

/*  additional information for functions, used by the optimizer,    */
struct function_info{
    struct IC *first_ic;    /* inline copy of function starts here */
    struct IC *last_ic;     /*  "       "       "      ends here   */
    struct Var *vars;       /* pointer to list of vars of that function */
};

/*  Structure for types.    */
struct Typ{
    int flags;  /*  see above   */
    struct Typ *next;
    struct struct_declaration *exact;   /* used for STRUCT/UNION/FUNKT  */
    zlong size;     /*  used for ARRAY  */
};
#define TYPS sizeof(struct Typ)

struct Var{
    int storage_class;  /* see below    */
    int reg;            /* Var is assigned to this hard-reg */
    int priority;       /* Priority to be used in simple_regs() */
    int flags;          /* see below */
    char *identifier;   /* name of the variable */
    int nesting;        /* can be freely used by the frontend */
    int index;          /* used by the optimizer */
    zlong offset;       /* offset relative to the stack frame */
    struct Typ *vtyp;   /* type of the variable */
    struct const_list *clist;   /* initialized? */
    struct Var *next;   /* pointer to next variable */
    struct function_info *fi;   /* used by the optimizer */
    struct Var *inline_copy;    /* used for function-inlining */
};

/* available storage-classes */
#define AUTO 1          /* var is allocated on the stack */
#define REGISTER 2      /* basically the same as AUTO (C-only) */
#define STATIC 3        /* var is static but has no external linkage */
#define EXTERN  4       /* var is static and has external linkage */
#define TYPEDEF 5       /* C-only */

/* available flags in struct Var */
#define USEDASSOURCE 1      /* the var has been read */
#define USEDASDEST 2        /* the var has been written */
#define DEFINED 4           /* the var has been defined (i.e. storage will
                               be allocated in the current file) */
#define USEDASADR 8         /* the address of the var has been taken */
#define GENERATED 16        /* code for static vars has been generated */
#define CONVPARAMETER 32
#define TENTATIVE 64        /* C-only */
#define USEDBEFORE 128      /* used by the optimizer */
#define INLINEV 256         /*  "               "    */
#define PRINTFLIKE 512      /* C-only */
#define SCANFLIKE 1024      /* C-only */
#define NOTTYPESAFE 2048    /* used by the optimizer */
#define DNOTTYPESAFE 4096   /*  "               "    */
#define REGPARM 8192        /* the var is a register parameter */

#define SLSIZE 32   /*  struct_lists in diesen Abstaenden realloc'en    */

/*  These structs are used to describe members of STRUCT/UNION or   */
/*  parameters of FUNKT. Some of the entries in struct_list are not */
/*  relevant for both alternatives.                                 */
struct struct_declaration{
    int count;  /* number of members/parameters */
    struct struct_declaration *next;
    struct struct_list (*sl)[];
};
struct struct_list{
    char *identifier;   /* name of the struct/union-tag */
    struct Typ *styp;   /* type of the member/parameter */
    int storage_class;  /* storage-class of function-parameter */
    int reg;            /* register to pass function-parameter */
};

/* This struct represents objects in the intermediate code. */
struct obj{
    int flags;      /* see below */
    int reg;        /* number of reg if flags&REG */
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


/*  Available flags in struct obj.  */
                    /*  KONST muss immer am kleinsten sein, um beim swappen */
                    /*  fuer available_expressions und Konstanten nach      */
                    /*  rechts nicht in eine Endlosschleife zu kommen       */

#define KONST 1     /*  The object is a constant. Its value is stored in    */
                    /*  val.                                                */
#define VAR 2       /*  The object is a variable (stored in v).             */
#define SCRATCH 8   /*  The object is a temporary.                          */
#define STACK 16    /*  obsolete                                            */
#define DREFOBJ 32  /*  The object must be dereferenced.                    */
#define REG 64      /*  The object is contained in a hardware register.     */
#define VARADR 128  /*  The object is the address of a static variable.     */
#define DONTREGISTERIZE 256 /*  Do not put this object into a register.     */


/*  The quads in the intermediate code. */
struct IC{
    struct IC *prev;    /* pointer to the next IC */
    struct IC *next;    /* pointer to the previous IC */
    int code;           /* see below */
    int typf;           /* usually type of the operands, see interface.doc */
    int defindex;       /* used by optimizer */
    int expindex;
    int copyindex;
    int change_cnt;
    int use_cnt;
    int line;           /* corresponding line in source file (or 0) */
    struct varlist *change_list;    /* used by optimizer */
    struct varlist *use_list;
    struct obj q1;      /* source 1 */
    struct obj q2;      /* source 2 */
    struct obj z;       /* target */
    char *file;         /* filename of the source file */
};
#define ICS sizeof(struct IC)


/*  Available codes for struct IC. See interface.doc. */
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

#define arith(c) ((c)>=CHAR&&(c)<=DOUBLE)

extern char *typname[];
extern zlong sizetab[16];
extern char *storage_class_name[];
extern char *ename[];

extern zlong align[16],maxalign;

/*  an empty string */
extern char *empty;

extern zchar vchar; extern zuchar vuchar;
extern zshort vshort; extern zushort vushort;
extern zint vint; extern zuint vuint;
extern zlong vlong; extern zulong vulong;
extern zfloat vfloat; extern zdouble vdouble;
extern zpointer vpointer;

#ifndef DEBUG
extern int DEBUG;
#endif

/*  used by the optimizer */
struct varlist{
    struct Var *v;
    int flags;
};
#define VLS sizeof(struct varlist)


extern struct IC *first_ic,*last_ic;
extern int regs[MAXR+1],regsa[MAXR+1],regused[MAXR+1],regscratch[MAXR+1];
extern zlong regsize[MAXR+1];
extern struct Var *regsv[MAXR+1];
extern char *regnames[];

extern int label,return_label;

/* The structures used for available options of the code generator. */
union ppi {char *p;long l;void (*f)(char *);};
#define USEDFLAG 1
#define STRINGFLAG 2
#define VALFLAG 4
#define FUNCFLAG 8
extern int g_flags[MAXGF];
extern char *g_flags_name[MAXGF];
extern union ppi g_flags_val[MAXGF];

extern zlong max_offset;

extern int function_calls;

/*  Das haette ich gern woanders    */
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

struct const_list{
    union atyps val;
    np tree;
    struct const_list *other,*next;
};
#define CLS sizeof(struct const_list)

extern zlong t_min[];
extern zulong t_max[];
extern zlong char_bit;

extern char cg_copyright[];

extern int goto_used;
extern int ic_count;
extern int only_inline;
extern int multiple_ccs;
extern int float_used;


extern struct IC *err_ic;


extern long maxoptpasses,optflags,inline_size,unroll_size;
extern long noaliasopt,fp_assoc;
extern struct Var *vl1,*vl2,*vl3;
extern int fline;
extern char errfname[FILENAME_MAX+1];

/* function which must be provided by the frontend */
extern void add_IC(struct IC *);
extern void error(int,...);
extern struct Var *add_tmp_var(struct Typ *);
extern void raus(void);

/* functions provided by supp.c */
extern void free_IC(struct IC *);
extern void insert_IC(struct IC *,struct IC *);
extern void pric(FILE *,struct IC *);
extern void pric2(FILE *,struct IC *);
extern void probj(FILE *,struct obj *,int);
extern void printzl(FILE *,zlong);
extern void printzul(FILE *,zulong);
extern void printzd(FILE *,zdouble);
extern void printval(FILE *,union atyps *,int,int);
extern void insert_const2(union atyps *,int);
extern void prd(FILE *,struct Typ *);
extern void freetyp(struct Typ *);
extern struct Typ *clone_typ(struct Typ *);
extern zlong szof(struct Typ *);
extern zlong falign(struct Typ *);
extern void eval_const(union atyps *,int);

extern void optimize(long, struct Var *);
extern void remove_IC(struct IC *);
extern void *mymalloc(size_t);
extern void simple_regs(void);


/*  functions provided by the code generator */
extern int regok(int,int,int);
extern int freturn(struct Typ *);
extern void gen_code(FILE *,struct IC *,struct Var *,zlong);
extern int init_cg(void);
extern void cleanup_cg(FILE *);
extern int dangerous_IC(struct IC *);
extern void gen_dc(FILE *,int,struct const_list *);
extern void gen_ds(FILE *,zlong,struct Typ *);
extern void gen_var_head(FILE *,struct Var *);
extern void gen_align(FILE *,zlong);
extern int shortcut(int, int);
extern int must_convert(np,int);
/*  Deklarationen fuer Registerparameterfunktionen. */
#ifdef HAVE_REGPARMS
extern struct reg_handle empty_reg_handle;
extern int reg_parm(struct reg_handle *, struct Typ *);
#endif

