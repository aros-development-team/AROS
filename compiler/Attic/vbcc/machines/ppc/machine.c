/*  Code generator for a PPC RISC cpu with 32 general purpose,  */
/*  32 floating point and 8 condition code registers.           */

#include "supp.h"

static char FILE_[]=__FILE__;

/*  Public data that MUST be there.                             */

/*  Commandline-flags the code-generator accepts                */
int g_flags[MAXGF]={VALFLAG,VALFLAG,0,0,0};
char *g_flags_name[MAXGF]={"cpu","fpu","const-in-data","sd","merge-constants"};
union ppi g_flags_val[MAXGF];

/*  Alignment-requirements for all types in bytes.              */
zlong align[16];

/*  Alignment that is sufficient for every object.              */
zlong maxalign;

/*  CHAR_BIT for the target machine.                            */
zlong char_bit;

/*  Tabelle fuer die Groesse der einzelnen Typen                */
zlong sizetab[16];

/*  Minimum and Maximum values each type can have.              */
/*  Must be initialized in init_cg().                           */
zlong t_min[32];
zulong t_max[32];

/*  Names of all registers.                                     */
char *regnames[MAXR+1]={"noreg",
                        "r0","r1","r2","r10","r9","r8","r7","r6",
                        "r5","r4","r3","r11","r12","r13","r14","r15",
                        "r16","r17","r18","r19","r20","r21","r22","r23",
                        "r24","r25","r26","r27","r28","r29","r30","r31",
                        "f9","f10","f0","f11","f12","f13","f8","f7",
                        "f6","f5","f4","f3","f2","f1","f14","f15",
                        "f16","f17","f18","f19","f20","f21","f22","f23",
                        "f24","f25","f26","f27","f28","f29","f30","f31",
                        "cr0","cr1","cr2","cr3","cr4","cr5","cr6","cr7",
                        "cnt"};

/*  The Size of each register in bytes.                         */
zlong regsize[MAXR+1];

/*  regsa[reg]!=0 if a certain register is allocated and should */
/*  not be used by the compiler pass.                           */
int regsa[MAXR+1];

/*  Specifies which registers may be scratched by functions.    */
int regscratch[MAXR+1]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          1,1,0,0,0,1,1,1,1};

struct reg_handle empty_reg_handle={0,0};


/****************************************/
/*  Private data and functions.         */
/****************************************/

static long malign[16]={1,1,2,4,4,4,4,4,4,4,4,4,4,4,4};
static long msizetab[16]={1,1,2,4,4,4,8,0,4,0,0,0,4,0};

static int sp=2;                   /*  Stackpointer                        */
static int fp=2;                   /*  Framepointer                        */
static int sd=14;                  /*  SmallDataPointer                    */
static int t1=12,t2=13,t3=1;       /*  Temporaries used by code generator  */
static int f1=33,f2=34,f3=35;      /*  Temporaries used by code generator  */

#define DATA 0
#define BSS 1
#define CODE 2

static int section=-1,newobj,crsave;
static char *codename="\t.text\n",*dataname="\t.data\n",*bssname="";
static int is_const(struct Typ *);
static char *labprefix="l",*idprefix="_";
static long frameoffset,stackoffset,localoffset;
static void probj2(FILE *f,struct obj *p,int t);

static struct fpconstlist {
    struct fpconstlist *next;
    int label,typ;
    union atyps val;
} *firstfpc;

static int addfpconst(struct obj *o,int t)
{
    struct fpconstlist *p=firstfpc;
    t&=NQ;
    if(g_flags[4]&USEDFLAG){
        for(p=firstfpc;p;p=p->next){
            if(t==p->typ){
                eval_const(&p->val,t);
                if(t==FLOAT&&zdeqto(vdouble,zf2zd(o->val.vfloat))) return(p->label);
                if(t==DOUBLE&&zdeqto(vdouble,o->val.vdouble)) return(p->label);
            }
        }
    }
    p=mymalloc(sizeof(struct fpconstlist));
    p->next=firstfpc;
    p->label=++label;
    p->typ=t;
    p->val=o->val;
    firstfpc=p;
    return(p->label);
}

#define REG_IND 1
#define IMM_IND 2

static struct obj *cam(int flags,int base,long offset)
/*  Initializes an addressing-mode structure and returns a pointer to   */
/*  that object. Will not survive a second call!                        */
{
    static struct obj obj;
    static struct AddressingMode am;
    obj.am=&am;
    am.flags=flags;
    am.base=base;
    am.offset=offset;
    return(&obj);
}

static char *ldt[]={"","bz","ha","wz","wz","fs","fd","","wz","","","","","","","",
                    "","bz","hz","wz","wz"};
static char *sdt[]={"","b","h","w","w","fs","fd","","w","","","","","","","",
                    "","b","h","w","w"};

static void load_address(FILE *f,int r,struct obj *o,int typ)
/*  Generates code to load the address of a variable into register r.   */
{
    if(!(o->flags&VAR)) ierror(0);
    if(o->v->storage_class==AUTO||o->v->storage_class==REGISTER){
        if(zl2l(o->v->offset)>=0){
            fprintf(f,"\taddi\t%s,%s,%ld\n",regnames[r],regnames[sp],(long)(zl2l(o->v->offset)+localoffset-stackoffset));
        }else{
            fprintf(f,"\taddi\t%s,%s,%ld\n",regnames[r],regnames[sp],(long)(-zl2l(o->v->offset)-zl2l(maxalign)-frameoffset-stackoffset));
        }
    }else{
        fprintf(f,"\tlis\t%s,",regnames[r]);
        probj2(f,o,typ);fprintf(f,"@ha\n");
        fprintf(f,"\taddi\t%s,%s,",regnames[r],regnames[r]);
        probj2(f,o,typ);fprintf(f,"@l\n");
    }
}
static void load_reg(FILE *f,int r,struct obj *o,int typ,int tmp)
/*  Generates code to load a memory object into register r. tmp is a    */
/*  general purpose register which may be used. tmp can be r.           */
{
    typ&=NU;
    if(o->flags&KONST){
        long l;
        eval_const(&o->val,typ);
        if(typ==FLOAT||typ==DOUBLE){
            int lab;
            if(zdeqto(vdouble,d2zd(0.0))){
                fprintf(f,"\tfsub\t%s,%s,%s\n",regnames[r],regnames[r],regnames[r]);
                return;
            }
            lab=addfpconst(o,typ);
            fprintf(f,"\tlis\t%s,%s%d@ha\n",regnames[tmp],labprefix,lab);
            fprintf(f,"\tl%s\t%s,%s%d@l(%s)\n",ldt[typ],regnames[r],labprefix,lab,regnames[tmp]);
            return;
        }
        if(zlleq(vlong,l2zl(32767))&&zlleq(l2zl(-32768),vlong)){
            fprintf(f,"\tli\t%s,%ld\n",regnames[r],zl2l(vlong));
        }else{
            l=zl2l(zland(zlrshift(vlong,l2zl(16L)),l2zl(65535L)));
            fprintf(f,"\tlis\t%s,%ld\n",regnames[r],l);
            l=zl2l(zland(vlong,l2zl(65535L)));
            fprintf(f,"\tori\t%s,%s,%ld\n",regnames[r],regnames[r],l);
        }
        return;
    }
    if((o->flags&VAR)&&(o->v->storage_class==EXTERN||o->v->storage_class==STATIC)){
        if(o->flags&VARADR){
            load_address(f,r,o,POINTER);
        }else{
            fprintf(f,"\tlis\t%s,",regnames[tmp]);
            probj2(f,o,typ);fprintf(f,"@ha\n");
            fprintf(f,"\tl%s\t%s,",ldt[typ],regnames[r]);
            probj2(f,o,typ);fprintf(f,"@l(%s)\n",regnames[tmp]);
        }
    }else{
        if((o->flags&(DREFOBJ|REG))==REG){
            if(r!=o->reg)
                fprintf(f,"\t%smr\t%s,%s\n",r>=33?"f":"",regnames[r],regnames[o->reg]);
        }else{
            fprintf(f,"\tl%s\t%s,",ldt[typ],regnames[r]);
            probj2(f,o,typ);fprintf(f,"\n");
        }
    }
}


static void store_reg(FILE *f,int r,struct obj *o,int typ)
/*  Generates code to store register r into memory object o.            */
{
    if((o->flags&VAR)&&(o->v->storage_class==EXTERN||o->v->storage_class==STATIC)){
        int tmp=t1;
        if(tmp==r) tmp=t2;
        fprintf(f,"\tlis\t%s,",regnames[tmp]);
        probj2(f,o,typ);fprintf(f,"@ha\n");
        fprintf(f,"\tst%s\t%s,",sdt[typ],regnames[r]);
        probj2(f,o,typ);fprintf(f,"@l(%s)\n",regnames[tmp]);
        return;
    }
    fprintf(f,"\tst%s\t%s,",sdt[typ&NU],regnames[r]);
    probj2(f,o,typ);fprintf(f,"\n");
}
static char *dct[]={"","byte","uahalf","uaword","uaword","uaword","uaword"};
static struct IC *do_refs(FILE *,struct IC *);
static void pr(FILE *,struct IC *);
static void function_top(FILE *,struct Var *,long);
static void function_bottom(FILE *f,struct Var *,long);

#define isreg(x) ((p->x.flags&(REG|DREFOBJ))==REG)

static int q1reg,q2reg,zreg;

static char *ccs[]={"eq","ne","lt","ge","le","gt",""};
static char *logicals[]={"or","xor","and"};
static char *arithmetics[]={"slw","srw","add","sub","mullw","divw","mod"};
static char *isimm[]={"","i"};

static struct IC *do_refs(FILE *f,struct IC *p)
/*  Does some pre-processing like fetching operands from memory to      */
/*  registers etc.                                                      */
{
    int typ=p->typf,typ1,reg,c=p->code;

    if(c==CONVCHAR) typ=CHAR;
    if(c==CONVUCHAR) typ=UNSIGNED|CHAR;
    if(c==CONVSHORT) typ=SHORT;
    if(c==CONVUSHORT) typ=UNSIGNED|SHORT;
    if(c==CONVINT) typ=LONG;
    if(c==CONVUINT) typ=UNSIGNED|LONG;
    if(c==CONVLONG) typ=LONG;
    if(c==CONVULONG) typ=UNSIGNED|LONG;
    if(c==CONVFLOAT) typ=FLOAT;
    if(c==CONVDOUBLE) typ=DOUBLE;
    if(c==CONVPOINTER) typ=UNSIGNED|LONG;

    if(c==SUB&&(p->q2.flags&KONST)&&(typ&NQ)<=LONG){
        eval_const(&p->q2.val,typ);
        if(zlleq(vlong,l2zl(32768L))&&zlleq(l2zl(-32767L),vlong)){
            p->code=c=ADD;
            p->q2.val.vlong=zlsub(l2zl(0L),vlong);
        }
    }

    q1reg=q2reg=zreg=0;
    if(p->q1.flags&REG) q1reg=p->q1.reg;
    if(p->q2.flags&REG) q2reg=p->q2.reg;
    if((p->z.flags&(REG|DREFOBJ))==REG) zreg=p->z.reg;

    if(p->q1.flags&KONST){
        eval_const(&p->q1.val,typ);
        if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f1; else reg=t1;
        if(c==ASSIGN&&zreg) reg=zreg;
        if(c==SETRETURN&&p->z.reg) reg=p->z.reg;
        if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE||c==DIV||c==ASSIGN||c==PUSH||c==SETRETURN||c==LSHIFT||c==RSHIFT||c==COMPARE){
            load_reg(f,reg,&p->q1,typ,t1);
            q1reg=reg;
        }else{
            if((typ&NQ)<=LONG){
                if(c>=OR&&c<=AND){
                    if(!zulleq(vulong,ul2zul(65535UL))){
                        load_reg(f,reg,&p->q1,typ,t1);
                        q1reg=reg;
                    }
                }else{
                    if(!zulleq(vlong,l2zl(32767L))||!zlleq(l2zl(-32768L),vlong)){
                        load_reg(f,reg,&p->q2,typ,t1);
                        q1reg=reg;
                    }
                }
            }
        }
    }else if(c!=ADDRESS){
        if(p->q1.flags&&!q1reg){
            if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f1; else reg=t1;
            if((c==ASSIGN||(c>=CONVCHAR&&c<=CONVULONG))&&zreg) reg=zreg;
            if(c==SETRETURN&&p->z.reg) reg=p->z.reg;
            if(p->q1.flags&DREFOBJ) {typ1=POINTER;reg=t1;} else typ1=typ;
            if((typ1&NQ)<=POINTER){
                int m=p->q1.flags;
                p->q1.flags&=~DREFOBJ;
                load_reg(f,reg,&p->q1,typ1,t1);
                p->q1.flags=m;
                q1reg=reg;
            }
        }
        if((p->q1.flags&DREFOBJ)&&(typ&NQ)<=POINTER){
            if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f1; else reg=t1;
            if((c==ASSIGN||(c>=CONVCHAR&&c<=CONVULONG))&&zreg) reg=zreg;
            if(c==SETRETURN&&p->z.reg) reg=p->z.reg;
            load_reg(f,reg,cam(IMM_IND,q1reg,0),typ,t1);
            q1reg=reg;
        }
    }
    if(p->q2.flags&KONST){
        eval_const(&p->q2.val,typ);
        if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f2; else reg=t2;
        if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE||c==DIV||c==SUB){
            load_reg(f,reg,&p->q2,typ,t2);
            q2reg=reg;
        }else{
            if((typ&NQ)<=LONG){
                if(c>=OR&&c<=AND){
                    if(!zulleq(vulong,ul2zul(65535UL))){
                        load_reg(f,reg,&p->q2,typ,t2);
                        q2reg=reg;
                    }
                }else{
                    if(!zulleq(vlong,l2zl(32767L))||!zlleq(l2zl(-32768L),vlong)){
                        load_reg(f,reg,&p->q2,typ,t2);
                        q2reg=reg;
                    }
                }
            }
        }
    }else{
        if(p->q2.flags&&!q2reg){
            if(p->q2.flags&DREFOBJ) typ1=POINTER; else typ1=typ;
            if((typ1&NQ)==FLOAT||(typ1&NQ)==DOUBLE) reg=f2; else reg=t2;
            if((typ1&NQ)<=POINTER){
                int m=p->q2.flags;
                p->q2.flags&=~DREFOBJ;
                load_reg(f,reg,&p->q2,typ1,t2);
                p->q2.flags=m;
                q2reg=reg;
            }
        }
        if((p->q2.flags&DREFOBJ)&&(typ&NQ)<=POINTER){
            if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f2; else reg=t2;
            load_reg(f,reg,cam(IMM_IND,q2reg,0),typ,t2);
            q2reg=reg;
        }
    }
    if(p->z.flags&&!isreg(z)){
        typ=p->typf;
        if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) zreg=f3; else zreg=t3;
    }
    if(q1reg){ p->q1.flags=REG; p->q1.reg=q1reg;}
    if(q2reg){ p->q2.flags=REG; p->q2.reg=q2reg;}
    return(p);
}
static void pr(FILE *f,struct IC *p)
/*  Writes the destination register to the real destination if necessary.   */
{
    int typ=p->typf;
    if(p->z.flags){
        if(!isreg(z)){
            if(p->z.flags&DREFOBJ){
                if(p->z.flags&REG){
                    store_reg(f,zreg,cam(IMM_IND,p->z.reg,0),typ);
                }else{
                    int r;
                    if(t1==zreg) r=t2; else r=t1;
                    load_reg(f,r,&p->z,POINTER,r);
                    store_reg(f,zreg,cam(IMM_IND,r,0),typ);
                }
            }else{
                store_reg(f,zreg,&p->z,typ);
            }
        }else{
            if(p->z.reg!=zreg)
                fprintf(f,"\t%smr\t%s,%s\n",zreg>=33?"f":"",regnames[p->z.reg],regnames[zreg]);
        }
    }
}

static void probj2(FILE *f,struct obj *p,int t)
/*  Prints an object.                               */
{
    if(p->am){
        if(p->am->flags==REG_IND) fprintf(f,"%s(%s)",regnames[p->am->offset],regnames[p->am->base]);
        if(p->am->flags==IMM_IND) fprintf(f,"%ld(%s)",p->am->offset,regnames[p->am->base]);
        return;
    }
    if(p->flags&DREFOBJ) fprintf(f,"(");
    if(p->flags&VAR) {
        if(p->v->storage_class==AUTO||p->v->storage_class==REGISTER){
            if(p->flags&REG){
                fprintf(f,"%s",regnames[p->reg]);
            }else{
                if(zl2l(p->v->offset)>=0){
                    fprintf(f,"%ld(%s)",(long)(zl2l(p->v->offset)+zl2l(p->val.vlong)+localoffset-stackoffset),regnames[sp]);
                }else{
                    fprintf(f,"%ld(%s)",(long)(-zl2l(p->v->offset)-zl2l(maxalign)+zl2l(p->val.vlong)-frameoffset-stackoffset),regnames[sp]);
                }
            }
        }else{
            if(!zleqto(l2zl(0L),p->val.vlong)){printval(f,&p->val,LONG,0);fprintf(f,"+");}
            if(p->v->storage_class==STATIC&&(p->v->vtyp->flags&NQ)!=FUNKT){
                fprintf(f,"l%ld",zl2l(p->v->offset));
            }else{
                fprintf(f,"%s%s",idprefix,p->v->identifier);
            }
        }
    }
    if((p->flags&REG)&&!(p->flags&VAR)) fprintf(f,"%s",regnames[p->reg]);
    if(p->flags&KONST){
        printval(f,&p->val,t&NU,0);
    }
    if(p->flags&DREFOBJ) fprintf(f,")");
}
static void function_top(FILE *f,struct Var *v,long offset)
/*  Generates function top.                             */
{
    int i,smoved=0;long sz;
    if(section!=CODE){fprintf(f,codename);section=CODE;}
    if(v->storage_class==EXTERN) fprintf(f,"\t.global\t%s%s\n",idprefix,v->identifier);
    fprintf(f,"%s%s:\n",idprefix,v->identifier);
    offset=((offset+zl2l(maxalign)-1)/zl2l(maxalign))*zl2l(maxalign);
    frameoffset=-offset;
    if(function_calls){
        frameoffset-=4;smoved=1;
        fprintf(f,"\tmflr\t%s\n\tst%su\t%s,%ld(%s)\n",regnames[t1],sdt[LONG],regnames[t1],frameoffset,regnames[sp]);
    }
    if(crsave){
        frameoffset-=4;
        if(!smoved) {sz=frameoffset;smoved=1;} else sz=-4;
        fprintf(f,"\tmfcr\t%s\n\tst%su\t%s,%ld(%s)\n",regnames[t1],sdt[LONG],regnames[t1],sz,regnames[sp]);
    }
    for(i=1;i<=64;i++){
        if(regused[i]&&!regscratch[i]&&!regsa[i]){
            if(i<=32){
                frameoffset-=4;
                if(!smoved) {sz=frameoffset;smoved=1;} else sz=-4;
                fprintf(f,"\tst%su\t%s,%ld(%s)\n",sdt[LONG],regnames[i],sz,regnames[sp]);
            }else{
                frameoffset-=8;
                if(!smoved) {sz=frameoffset;smoved=1;} else sz=-8;
                fprintf(f,"\tst%su\t%s,%ld(%s)\n",sdt[DOUBLE],regnames[i],sz,regnames[sp]);
            }
        }
    }
    if(!smoved&&frameoffset)
        fprintf(f,"\taddi\t%s,%s,%ld\n",regnames[sp],regnames[sp],frameoffset);
    localoffset=-frameoffset-offset;
}
static void function_bottom(FILE *f,struct Var *v,long offset)
/*  Generates function bottom.                          */
{
    int i;long o=0;
    for(i=64;i>=1;i--){
        if(regused[i]&&!regscratch[i]&&!regsa[i]){
            if(i<=32){
                fprintf(f,"\tl%s\t%s,%ld(%s)\n",ldt[LONG],regnames[i],o,regnames[sp]);
                o+=4;
            }else{
                fprintf(f,"\tl%s\t%s,%ld(%s)\n",ldt[DOUBLE],regnames[i],o,regnames[sp]);
                o+=8;
            }
        }
    }
    if(crsave){ fprintf(f,"\tlwz\t%s,%ld(%s)\n\tmtcr\t%s\n",regnames[t1],o,regnames[sp],regnames[t1]);o+=4;}
    if(function_calls) fprintf(f,"\tlwz\t%s,%ld(%s)\n\tmtlr\t%s\n",regnames[t1],o,regnames[sp],regnames[t1]);
    if(frameoffset) fprintf(f,"\taddi\t%s,%s,%ld\n",regnames[sp],regnames[sp],-frameoffset);
    fprintf(f,"\tblr\n");
    fprintf(f,"\t.type\t%s%s,@function\n",idprefix,v->identifier);
    fprintf(f,"\t.size\t%s%s,$-%s%s\n",idprefix,v->identifier,idprefix,v->identifier);
}
static int is_const(struct Typ *t)
/*  Tests if a type can be placed in the code-section.  */
{
    if(!(t->flags&(CONST|STRINGCONST))){
        do{
            if(t->flags&(CONST|STRINGCONST)) return(1);
            if((t->flags&NQ)!=ARRAY) return(0);
            t=t->next;
        }while(1);
    }else return(1);
}

/****************************************/
/*  End of private data and functions.  */
/****************************************/


int init_cg(void)
/*  Does necessary initializations for the code-generator. Gets called  */
/*  once at the beginning and should return 0 in case of problems.      */
{
    int i;
    /*  Initialize some values which cannot be statically initialized   */
    /*  because they are stored in the target's arithmetic.             */
    maxalign=l2zl(4L);
    char_bit=l2zl(8L);
    for(i=0;i<16;i++){
        sizetab[i]=l2zl(msizetab[i]);
        align[i]=l2zl(malign[i]);
    }
    for(i= 1;i<=32;i++) regsize[i]=l2zl(4L);
    for(i=33;i<=64;i++) regsize[i]=l2zl(8L);
    for(i=65;i<=72;i++) regsize[i]=l2zl(1L);

    /*  Use multiple ccs.   */
    multiple_ccs=1;

    /*  Initialize the min/max-settings. Note that the types of the     */
    /*  host system may be different from the target system and you may */
    /*  only use the smallest maximum values ANSI guarantees if you     */
    /*  want to be portable.                                            */
    /*  That's the reason for the subtraction in t_min[INT]. Long could */
    /*  be unable to represent -2147483648 on the host system.          */
    t_min[UNSIGNED|CHAR]=t_min[UNSIGNED|SHORT]=t_min[UNSIGNED|INT]=t_min[UNSIGNED|LONG]=l2zl(0L);
    t_min[CHAR]=l2zl(-128L);
    t_min[SHORT]=l2zl(-32768L);
    t_min[INT]=zlsub(l2zl(-2147483647L),l2zl(1L));
    t_min[LONG]=t_min[INT];
    t_max[CHAR]=ul2zul(127UL);
    t_max[SHORT]=ul2zul(32767UL);
    t_max[INT]=ul2zul(2147483647UL);
    t_max[LONG]=t_max[INT];
    t_max[UNSIGNED|CHAR]=ul2zul(255UL);
    t_max[UNSIGNED|SHORT]=ul2zul(65535UL);
    t_max[UNSIGNED|INT]=ul2zul(4294967295UL);
    t_max[UNSIGNED|LONG]=t_max[UNSIGNED|INT];
    /*  Reserve a few registers for use by the code-generator.      */
    /*  This is not optimal but simple.                             */
    regsa[t1]=regsa[t2]=regsa[t3]=1;
    regsa[f1]=regsa[f2]=regsa[f3]=1;
    regsa[sp]=regsa[fp]=regsa[sd]=1;
    regscratch[t1]=regscratch[t2]=regscratch[t3]=0;
    regscratch[f1]=regscratch[f2]=regscratch[f3]=0;
    regscratch[sp]=regscratch[fp]=regscratch[sd]=0;

    return(1);
}

int freturn(struct Typ *t)
/*  Returns the register in which variables of type t are returned. */
/*  If the value cannot be returned in a register returns 0.        */
/*  A pointer MUST be returned in a register. The code-generator    */
/*  has to simulate a pseudo register if necessary.                 */
{
    if((t->flags&NQ)==FLOAT||(t->flags&NQ)==DOUBLE) return(46);
    if((t->flags&NQ)==STRUCT||(t->flags&NQ)==UNION) return(0);
    if(zlleq(szof(t),l2zl(4L))) return(11); else return(0);
}

int regok(int r,int t,int mode)
/*  Returns 0 if register r cannot store variables of   */
/*  type t. If t==POINTER and mode!=0 then it returns   */
/*  non-zero only if the register can store a pointer   */
/*  and dereference a pointer to mode.                  */
{
    if(r==0) return(0);
    t&=NQ;
    if(t==0){
        if(r>=65&&r<=72) return(1); else return(0);
    }
    if((t==FLOAT||t==DOUBLE)&&r>=33&&r<=64) return(1);
    if(t==POINTER&&r>=1&&r<=32) return(1);
    if(t>=CHAR&&t<=LONG&&r>=1&&r<=32) return(1);
    return(0);
}

int dangerous_IC(struct IC *p)
/*  Returns zero if the IC p can be safely executed     */
/*  without danger of exceptions or similar things.     */
/*  vbcc may generate code in which non-dangerous ICs   */
/*  are sometimes executed although control-flow may    */
/*  never reach them (mainly when moving computations   */
/*  out of loops).                                      */
/*  Typical ICs that generate exceptions on some        */
/*  machines are:                                       */
/*      - accesses via pointers                         */
/*      - division/modulo                               */
/*      - overflow on signed integer/floats             */
{
    int c=p->code;
    if((p->q1.flags&DREFOBJ)||(p->q2.flags&DREFOBJ)||(p->z.flags&DREFOBJ))
        return(0);
    if((c==DIV||c==MOD)&&!(p->q2.flags&KONST))
        return(1);
    return(0);
}

int must_convert(np p,int t)
/*  Returns zero if code for converting np to type t    */
/*  can be omitted.                                     */
/*  On the PowerPC cpu pointers and 32bit               */
/*  integers have the same representation and can use   */
/*  the same registers.                                 */
{
    int o=p->ntyp->flags,op=o&NQ,tp=t&NQ;
    if(zleqto(sizetab[tp],sizetab[op])&&op!=FLOAT&&tp!=FLOAT) return(0);

    return(1);
}

void gen_ds(FILE *f,zlong size,struct Typ *t)
/*  This function has to create <size> bytes of storage */
/*  initialized with zero.                              */
{
    if(newobj) fprintf(f,"%ld\n",zl2l(size));
        else   fprintf(f,"\t.space\t%ld\n",zl2l(size));
    newobj=0;
}

void gen_align(FILE *f,zlong align)
/*  This function has to make sure the next data is     */
/*  aligned to multiples of <align> bytes.              */
{
    if(!zlleq(align,l2zl(1L))) fprintf(f,"\t.align\t2\n");
}

void gen_var_head(FILE *f,struct Var *v)
/*  This function has to create the head of a variable  */
/*  definition, i.e. the label and information for      */
/*  linkage etc.                                        */
{
    int constflag;
    if(v->clist) constflag=is_const(v->vtyp);
    if(v->storage_class==STATIC){
        if((v->vtyp->flags&NQ)==FUNKT) return;
        if(v->clist&&(!constflag||(g_flags[2]&USEDFLAG))&&section!=DATA){fprintf(f,dataname);section=DATA;}
        if(v->clist&&constflag&&!(g_flags[2]&USEDFLAG)&&section!=CODE){fprintf(f,codename);section=CODE;}
        if(!v->clist&&section!=BSS){fprintf(f,bssname);section=BSS;}
        fprintf(f,"\t.type\t%s%ld,@object\n",labprefix,zl2l(v->offset));
        fprintf(f,"\t.size\t%s%ld,%ld\n",labprefix,zl2l(v->offset),zl2l(szof(v->vtyp)));
        if(section!=BSS) fprintf(f,"%s%ld:\n",labprefix,zl2l(v->offset));
            else fprintf(f,"\t.lcomm\t%s%ld,",labprefix,zl2l(v->offset));
        newobj=1;
    }
    if(v->storage_class==EXTERN){
        fprintf(f,"\t.global\t%s%s\n",idprefix,v->identifier);
        if(v->flags&(DEFINED|TENTATIVE)){
            if(v->clist&&(!constflag||(g_flags[2]&USEDFLAG))&&section!=DATA){fprintf(f,dataname);section=DATA;}
            if(v->clist&&constflag&&!(g_flags[2]&USEDFLAG)&&section!=CODE){fprintf(f,codename);section=CODE;}
            if(!v->clist&&section!=BSS){fprintf(f,bssname);section=BSS;}
            fprintf(f,"\t.type\t%s%s,@object\n",idprefix,v->identifier);
            fprintf(f,"\t.size\t%s%s,%ld\n",idprefix,v->identifier,zl2l(szof(v->vtyp)));
            if(section!=BSS) fprintf(f,"%s%s:\n",idprefix,v->identifier);
                else fprintf(f,"\t.comm\t%s%s,",idprefix,v->identifier);
            newobj=1;
        }
    }
}

void gen_dc(FILE *f,int t,struct const_list *p)
/*  This function has to create static storage          */
/*  initialized with const-list p.                      */
{
    if((t&NQ)==POINTER) t=UNSIGNED|LONG;
    fprintf(f,"\t.%s\t",dct[t&NQ]);
    if(!p->tree){
        if((t&NQ)==FLOAT||(t&NQ)==DOUBLE){
        /*  auch wieder nicht sehr schoen und IEEE noetig   */
            unsigned char *ip;
            ip=(unsigned char *)&p->val.vdouble;
            fprintf(f,"0x%02x%02x%02x%02x",ip[0],ip[1],ip[2],ip[3]);
            if((t&NQ)==DOUBLE){
                fprintf(f,",0x%02x%02x%02x%02x",ip[4],ip[5],ip[6],ip[7]);
            }
        }else{
            printval(f,&p->val,t&NU,0);
        }
    }else{
        probj2(f,&p->tree->o,t&NU);
    }
    fprintf(f,"\n");newobj=0;
}


/*  The main code-generation routine.                   */
/*  f is the stream the code should be written to.      */
/*  p is a pointer to a doubly linked list of ICs       */
/*  containing the function body to generate code for.  */
/*  v is a pointer to the function.                     */
/*  offset is the size of the stackframe the function   */
/*  needs for local variables.                          */

void gen_code(FILE *f,struct IC *p,struct Var *v,zlong offset)
/*  The main code-generation.                                           */
{
    int c,t;char *fpp;
    if(DEBUG&1) printf("gen_code()\n");
    for(c=1;c<=MAXR;c++) regs[c]=regsa[c];
    function_top(f,v,zl2l(offset));
    stackoffset=0;
    for(;p;pr(f,p),p=p->next){
        c=p->code;t=p->typf;
        if(c==NOP) continue;
        if(c==ALLOCREG) {regs[p->q1.reg]=1;continue;}
        if(c==FREEREG) {regs[p->q1.reg]=0;continue;}
        if(c==LABEL) {fprintf(f,"%s%d:\n",labprefix,t);continue;}
        if(c==BRA) {fprintf(f,"\tb\t%s%d\n",labprefix,t);continue;}
        if(c>=BEQ&&c<BRA){
            if(!(p->q1.flags&REG)) p->q1.reg=65;
            fprintf(f,"\tb%s\t%s,%s%d\n",ccs[c-BEQ],regnames[p->q1.reg],labprefix,t);
            continue;
        }
        if(c==MOVETOREG){
            load_reg(f,p->z.reg,&p->q1,INT,0);
            p->z.flags=0;
            continue;
        }
        if(c==MOVEFROMREG){
            store_reg(f,p->q1.reg,&p->z,INT);
            p->z.flags=0;
            continue;
        }
        if((c==ASSIGN||c==PUSH)&&(t&NQ)>POINTER){
            unsigned long size,l;
            size=zl2l(p->q2.val.vlong);
            l=((size>>16)&65535);
            if(l) fprintf(f,"\tlis\t%s,%lu\n",regnames[t3],l);
            l=(size&65535);
            fprintf(f,"\tli\t%s,%lu\n",regnames[t3],l);
            fprintf(f,"\tmtctr\t%s\n",regnames[t3]);

            if(p->q1.flags&DREFOBJ){
                p->q1.flags&=~DREFOBJ;
                load_reg(f,t1,&p->q1,POINTER,t1);
                p->q1.flags|=DREFOBJ;
            }else{
                load_address(f,t1,&p->q1,POINTER);
            }
            if(p->z.flags&DREFOBJ){
                p->z.flags&=~DREFOBJ;
                load_reg(f,t2,&p->z,POINTER,t2);
                p->z.flags|=DREFOBJ;
            }else{
                if(c==PUSH){
                    t2=sp;
                }else{
                    load_address(f,t2,&p->z,POINTER);
                }
            }
            fprintf(f,"\tadd\t%s,%s,%s\n",regnames[t1],regnames[t1],regnames[t3]);
            if(c==ASSIGN)
                fprintf(f,"\tadd\t%s,%s,%s\n",regnames[t2],regnames[t2],regnames[t3]);
            fprintf(f,"%s%d:\n",labprefix,++label);
            fprintf(f,"\tlbzu\t%s,-1(%s)\n",regnames[t3],regnames[t1]);
            fprintf(f,"\tstbu\t%s,-1(%s)\n",regnames[t3],regnames[t2]);
            fprintf(f,"\tbdnz\t%s%d\n",labprefix,label);
            stackoffset-=size;
            p->z.flags=0;
            continue;
        }
        if(c==TEST&&((t&NQ)==FLOAT||(t&NQ)==DOUBLE)){
            p->code=c=COMPARE;
            p->q2.flags=KONST;
            p->q2.val.vdouble=d2zd(0.0);
            if((t&NQ)==FLOAT) p->q2.val.vfloat=zd2zf(p->q2.val.vdouble);
        }
        p=do_refs(f,p);
        c=p->code;
        if(c==SUBPFP) c=SUB;
        if(c==ADDI2P) c=ADD;
        if(c==SUBIFP) c=SUB;
        if(c>=CONVCHAR&&c<=CONVULONG){
            int to;
            if(c==CONVCHAR) to=CHAR;
            if(c==CONVUCHAR) to=UNSIGNED|CHAR;
            if(c==CONVSHORT) to=SHORT;
            if(c==CONVUSHORT) to=UNSIGNED|SHORT;
            if(c==CONVINT) to=LONG;
            if(c==CONVUINT) to=UNSIGNED|LONG;
            if(c==CONVLONG) to=LONG;
            if(c==CONVULONG) to=UNSIGNED|LONG;
            if(c==CONVFLOAT) to=FLOAT;
            if(c==CONVDOUBLE) to=DOUBLE;
            if(c==CONVPOINTER) to=UNSIGNED|LONG;
            if(to==FLOAT||to==DOUBLE){
                if((t&NQ)==FLOAT||(t&NQ)==DOUBLE){
                    if(q1reg!=zreg) fprintf(f,"\tfmr\t%s,%s\n",regnames[zreg],regnames[q1reg]);
                    continue;
                }
                if(t&UNSIGNED) ierror(0);
                fprintf(f,"\tfctiwz\t%s,%s\n",regnames[f3],regnames[q1reg]);
                fprintf(f,"\tst%su\t%s,-8(%s)\n",sdt[DOUBLE],regnames[f3],regnames[sp]);
                stackoffset-=8;
                fprintf(f,"\tl%s\t%s,4(%s)\n",ldt[t&NU],regnames[zreg],regnames[sp]);
                fprintf(f,"\taddi\t%s,%s,8\n",regnames[sp],regnames[sp]);
                continue;
            }
            if((t&NQ)==FLOAT||(t&NQ)==DOUBLE){
                static struct obj o;char *ip;
                if(to&UNSIGNED) ierror(0);
                o.flags=KONST;
                ip=(char *)&o.val.vdouble;
                ip[0]=0x43;
                ip[1]=0x30;
                ip[2]=0x00;
                ip[3]=0x00;
                ip[4]=0x80;
                ip[5]=0x00;
                ip[6]=0x00;
                ip[7]=0x00;
                fprintf(f,"\tlis\t%s,17200\n",regnames[t2]);
                fprintf(f,"\tst%su\t%s,-8(%s)\n",sdt[INT],regnames[t2],regnames[sp]);
                stackoffset-=8;
                fprintf(f,"\txoris\t%s,%s,32768\n",regnames[t2],regnames[q1reg]);
                fprintf(f,"\tst%s\t%s,4(%s)\n",sdt[INT],regnames[t2],regnames[sp]);
                fprintf(f,"\tl%s\t%s,0(%s)\n",ldt[t&NQ],regnames[zreg],regnames[sp]);
                load_reg(f,f2,&o,DOUBLE,t2);
                fprintf(f,"\tfsub\t%s,%s,%s\n",regnames[zreg],regnames[zreg],regnames[f2]);
                fprintf(f,"\taddi\t%s,%s,8\n",regnames[sp],regnames[sp]);
                continue;
            }
/*            if(q1reg==zreg){
                if(to==CHAR) fprintf(f,"\textsb\t%s,%s\n",regnames[zreg],regnames[q1reg]);
                if(to==SHORT) fprintf(f,"\textsh\t%s,%s\n",regnames[zreg],regnames[q1reg]);
                if(to==(UNSIGNED|CHAR)) fprintf(f,"\tandi\t%s,%s,255\n",regnames[zreg],regnames[q1reg]);
                if(to==(UNSIGNED|SHORT)) fprintf(f,"\tandi\t%s,%s,65535\n",regnames[zreg],regnames[q1reg]);
            }else*/  zreg=q1reg;
            continue;
        }
        if(c==KOMPLEMENT){
            fprintf(f,"\tnor\t%s,%s,%s\n",regnames[zreg],regnames[q1reg],regnames[q1reg]);
            continue;
        }
        if(c==SETRETURN){
            if(p->z.reg){
                if(zreg==0) load_reg(f,p->z.reg,&p->q1,t,t3);
            }else
                ierror(0);
            continue;
        }
        if(c==GETRETURN){
            if(p->q1.reg)
                zreg=p->q1.reg;
            else
                ierror(0);
            continue;
        }
        if(c==CALL){
            int reg;
            if(q1reg){
                fprintf(f,"\tmtlr\t%s\n",regnames[q1reg]);
                fprintf(f,"\tblrl\n");
            }else{
                fprintf(f,"\tbl\t");probj2(f,&p->q1,t);
                fprintf(f,"\n");
            }
            if(!zleqto(l2zl(0L),p->q2.val.vlong)){
                fprintf(f,"\taddi\t%s,%s,%ld\n",regnames[sp],regnames[sp],zl2l(p->q2.val.vlong));
                stackoffset+=zl2l(p->q2.val.vlong);
            }
            continue;
        }
        if(c==ASSIGN||c==PUSH){
            if(t==0) ierror(0);
            if(q1reg){
                if(c==PUSH){
                    fprintf(f,"\tst%su\t%s,-%ld(%s)\n",sdt[t&NQ],regnames[q1reg],zl2l(p->q2.val.vlong),regnames[sp]);
                    stackoffset-=zl2l(p->q2.val.vlong);
                    continue;
                }
                if(c==ASSIGN) zreg=q1reg;
                continue;
            }
        }
        if(c==ADDRESS){
            load_address(f,zreg,&p->q1,POINTER);
            continue;
        }
        if((t&NQ)==FLOAT||(t&NQ)==DOUBLE) fpp="f"; else fpp="";
        if(c==MINUS){
            fprintf(f,"\t%sneg\t%s,%s\n",fpp,regnames[zreg],regnames[q1reg]);
            continue;
        }
        if(c==TEST){
            if(!(p->z.flags&REG))
                p->z.reg=65;
            if((t&NQ)==FLOAT||(t&NQ)==DOUBLE)
                fprintf(f,"\tfcmpu\t%s,%s\n",regnames[p->z.reg],regnames[q1reg]);
            else
                fprintf(f,"\tcmp%swi\t%s,%s,0\n",(t&UNSIGNED)?"l":"",regnames[p->z.reg],regnames[q1reg]);
            continue;
        }
        if(c==COMPARE){
            if(!(p->z.flags&REG))
                p->z.reg=65;
            if((t&NQ)==FLOAT||(t&NQ)==DOUBLE)
                fprintf(f,"\tfcmpu\t%s,%s,",regnames[p->z.reg],regnames[q1reg]);
            else
                fprintf(f,"\tcmp%sw%s\t%s,%s,",(t&UNSIGNED)?"l":"",isimm[q2reg==0],regnames[p->z.reg],regnames[q1reg]);
            probj2(f,&p->q2,t);fprintf(f,"\n");
            continue;
        }
        if(c>=OR&&c<=AND){
            fprintf(f,"\t%s%s%s\t%s,%s,",logicals[c-OR],isimm[q2reg==0],(q2reg==0&&c==AND)?".":"",regnames[zreg],regnames[q1reg]);
            probj2(f,&p->q2,t);fprintf(f,"\n");
            continue;
        }
        if(c==SUB&&(p->q1.flags&KONST)){
            fprintf(f,"\tsubfic\t%s,%s,",regnames[zreg],regnames[q2reg]);
            probj2(f,&p->q1,t);fprintf(f,"\n");
            continue;
        }
        if(c>=LSHIFT&&c<=MOD){
            if(c==MOD) ierror(0);
            if(c==DIV&&(t1&UNSIGNED))
                fprintf(f,"\tdivwu%s\t%s,%s,",isimm[q2reg==0],regnames[zreg],regnames[q1reg]);
            else if(c==MULT&&((t&NQ)==FLOAT||(t&NQ)==DOUBLE))
                fprintf(f,"\tfmul%s\t%s,%s,",isimm[q2reg==0],regnames[zreg],regnames[q1reg]);
            else if(c==MULT&&q2reg==0)
                fprintf(f,"\tmulli\t%s,%s,",regnames[zreg],regnames[q1reg]);
            else
                fprintf(f,"\t%s%s%s\t%s,%s,",fpp,arithmetics[c-LSHIFT],isimm[q2reg==0],regnames[zreg],regnames[q1reg]);
            probj2(f,&p->q2,t);fprintf(f,"\n");
            continue;
        }
        ierror(0);
    }
    function_bottom(f,v,zl2l(offset));
}

int shortcut(int code,int typ)
{
    return(0);
}

int reg_parm(struct reg_handle *m, struct Typ *t)
{
    int f;
    if(!m) ierror(0);
    if(!t) ierror(0);
    f=t->flags&NQ;
    if(f<=LONG||f==POINTER){
        if(m->gregs>=8) return(0);
        return(11-m->gregs++);
    }
    if(f==FLOAT||f==DOUBLE){
        if(m->fregs>=8) return(0);
        return(46-m->fregs++);
    }
    return(0);
}
void cleanup_cg(FILE *f)
{
    struct fpconstlist *p;
    unsigned char *ip;
    while(p=firstfpc){
        if(f){
            if(section!=CODE){fprintf(f,codename);section=CODE;}
            fprintf(f,"%s%d:\n\t.long\t",labprefix,p->label);
            ip=(unsigned char *)&p->val.vdouble;
            fprintf(f,"0x%02x%02x%02x%02x",ip[0],ip[1],ip[2],ip[3]);
            if((p->typ&NQ)==DOUBLE){
                fprintf(f,",0x%02x%02x%02x%02x",ip[4],ip[5],ip[6],ip[7]);
            }
            fprintf(f,"\n");
        }
        firstfpc=p->next;
        free(p);
    }
}


