/*  $VER: vbcc (opt.h) V0.4     */

#include "vbc.h"

#define BSET(array,bit) (array)[(bit)/CHAR_BIT]|=1<<((bit)%CHAR_BIT)
#define BCLR(array,bit) (array)[(bit)/CHAR_BIT]&=~(1<<((bit)%CHAR_BIT))
#define BTST(array,bit) ((array)[(bit)/CHAR_BIT]&(1<<((bit)%CHAR_BIT)))

extern int gchanged;   /*  Merker, ob Optimierungslauf etwas geaendert hat */
extern int norek;      /*  diese Funktion wird nicht rekursiv auf          */
extern int nocall;     /*  diese Funktion kehrt nicht zum Caller zurueck   */

/*  temporary fuer verschiedene Bitvektoren */
extern unsigned char *tmp;

/*  fuer aktive Variablen   */
extern struct Var **vilist;
extern unsigned int vcount;    /*  0..vcount-rcount-1: vars, vcount-rcount..vcount: DREFOBJs */
extern unsigned int rcount;
extern size_t vsize;
extern unsigned char *av_globals,*av_address,*av_statics,*av_drefs;
extern int report_dead_statements;

/*  fuer verfuegbare Definitionen   */
extern unsigned int dcount;
extern size_t dsize;
extern struct IC **dlist;
extern unsigned char **defs;       /*  gibt an, welche Definitionen, welche    */
                            /*  Variablen definieren                    */
/*  alle Definitionen, globaler oder Adr. fuer propagation etc.         */
extern unsigned char *rd_globals,*rd_address,*rd_statics,*rd_drefs;
/*  dasselbe fuer die undefs    */
extern unsigned char *rd_defs,*rd_tmp,*rd_parms,*rd_vars;
/*  Modus fuer reaching_definitions */
extern int rd_mode;

/*  fuer verfuegbare Ausdruecke */
extern struct IC **elist;
extern unsigned int ecount;
extern size_t esize;
extern unsigned char *ae_globals,*ae_address,*ae_statics,*ae_drefs;

/*  fuer verfuegbare Kopien */
extern unsigned int ccount;
extern size_t csize;
extern struct IC **clist;

/*  fuer frequency-reduction    */
extern unsigned char *inloop,*invariant;

/*  alle Assignments, globaler oder Adr. fuer propagation etc.         */
extern unsigned char *cp_globals,*cp_address,*cp_statics,*cp_drefs,*cp_act,*cp_dest;
/*  alle Kopieranweisungen, die eine best. Variable als Quelle haben    */
extern unsigned char **copies;

extern int have_alias;

struct flowgraph{
    struct IC *start,*end;
    struct flowgraph *normalout,*branchout;
    struct flowlist *in;
    int index;
    /*  Letzter Block der Schleife, falls Block Start einer Schleife ist    */
    struct flowgraph *loopend;
    /*  Anzahl Funktionsaufrufe im Block/der Schleife   */
    int calls,loop_calls;
    /*  Bitvektoren fuer aktive Variablen   */
    unsigned char *av_in,*av_out,*av_gen,*av_kill;
    unsigned char *rd_in,*rd_out,*rd_gen,*rd_kill;
    unsigned char *ae_in,*ae_out,*ae_gen,*ae_kill;
    unsigned char *cp_in,*cp_out,*cp_gen,*cp_kill;
    /*  Registervariablen   */
    struct Var *regv[MAXR+1];
    /*  Merker, ob Register gebraucht wurde; MACR+1 Bits    */
    unsigned char regused[(MAXR+CHAR_BIT)/CHAR_BIT];
};

extern unsigned int basic_blocks;

struct flowlist{
    struct flowgraph *graph;
    struct flowlist *next;
};

int bvcmp(unsigned char *dest,unsigned char *src,size_t len);
void bvunite(unsigned char *dest,unsigned char *src,size_t len);
void bvintersect(unsigned char *dest,unsigned char *src,size_t len);
void bvdiff(unsigned char *dest,unsigned char *src,size_t len);
int compare_const(union atyps *q1,union atyps *q2,int t);
int compare_objs(struct obj *o1,struct obj *o2,int t);
void simple_regs(void);
void remove_IC_fg(struct flowgraph *g,struct IC *p);

extern int lastlabel;

struct flowgraph *construct_flowgraph(void);
void print_av(unsigned char *bitvector);
void print_rd(unsigned char *bitvector);
void print_ae(unsigned char *bitvector);
void print_cp(unsigned char *bitvector);
void print_flowgraph(struct flowgraph *g);
void free_flowgraph(struct flowgraph *g);
void num_vars(void);
void print_vi(void);
void av_change(struct IC *p,unsigned char *use,unsigned char *def);
void active_vars(struct flowgraph *fg);
int dead_assignments(struct flowgraph *fg);
void insert_IC(struct IC *p,struct IC *new);
void insert_IC_fg(struct flowgraph *fg,struct IC *p,struct IC *new);
void insert_allocreg(struct flowgraph *fg,struct IC *p,int code,int reg);

extern struct Var *lregv[MAXR+1];
extern struct flowgraph *lfg;

extern int report_weird_code,report_suspicious_loops;

int replace_local_reg(struct obj *o);
void local_regs(struct flowgraph *fg);
void loop_regs(struct flowgraph *fg);
void block_regs(struct flowgraph *fg);
void insert_saves(void);
struct flowgraph *jump_optimization(void);
void num_defs(void);
void reaching_definitions(struct flowgraph *fg);
void rd_change(struct IC *p);
void calc(int c,int t,union atyps *q1,union atyps *q2,union atyps *z,struct IC *p);
int fold(struct IC *p);
int peephole(void);
int propagate(struct obj *o,int replace);
int constant_propagation(struct flowgraph *fg,int global);
int compare_exp(const void *a1,const void *a2);
void num_exp(void);
void available_expressions(struct flowgraph *fg);
void available_copies(struct flowgraph *fg);
int cse(struct flowgraph *fg,int global);
void num_copies(void);
int copy_propagation(struct flowgraph *fg,int global);
int loops(struct flowgraph *fg,int mode);
struct flowgraph *create_loop_headers(struct flowgraph *fg,int av);
struct flowgraph *create_loop_footers(struct flowgraph *fg,int av);
void insert_regs(struct flowgraph *fg);
void recalc_offsets(struct flowgraph *fg);
void optimize(long flags,struct Var *function);
int loop_optimizations(struct flowgraph *fg);
void ic_uses(struct IC *p,unsigned char *result);
void ic_changes(struct IC *p,unsigned char *result);
void create_alias(struct flowgraph *fg);
void free_alias(struct flowgraph *fg);
void update_alias(struct Var *old,struct Var *new);
