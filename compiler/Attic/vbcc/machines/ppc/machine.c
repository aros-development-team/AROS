/*  Code generator for a PPC RISC cpu with 32 general purpose,  */
/*  32 floating point and 8 condition code registers.           */

#include "supp.h"

static char FILE_[]=__FILE__;

/*  Public data that MUST be there.                             */

/* Name and copyright. */
char cg_copyright[]="vbcc code-generator for PPC V0.2g (c) in 1997 by Volker Barthelmann";

/*  Commandline-flags the code-generator accepts                */
int g_flags[MAXGF]={VALFLAG,VALFLAG,0,0,0,0,
                    0,0,0,0,0};
char *g_flags_name[MAXGF]={"cpu","fpu","const-in-data","sd","merge-constants","fsub-zero",
                           "elf","amiga-align","no-regnames","peephole","setccs"};
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

/*  Type which can store each register. */
struct Typ *regtype[MAXR+1];

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

static char *mregnames[MAXR+1];

static long malign[16]=  {1,1,2,4,4,4,8,1,4,1,1,1,4,1};
static long msizetab[16]={1,1,2,4,4,4,8,0,4,0,0,0,4,0};

static struct Typ ltyp={LONG},ldbl={DOUBLE},lchar={CHAR};

static int r2=3;                   /*  reserved or toc                     */
static int r3=11;                  /*  return value                        */
static int sp=2;                   /*  Stackpointer                        */
static int fp=2;                   /*  Framepointer                        */
static int sd=14;                  /*  SmallDataPointer                    */
static int t1=12,t2=13,t3=1;       /*  Temporaries used by code generator  */
static int f1=33,f2=34,f3=35;      /*  Temporaries used by code generator  */

#define DATA 0
#define BSS 1
#define CODE 2
#define RODATA 3

static int section=-1,newobj,crsave;
static char *codename="\t.text\n",*dataname="\t.data\n",*bssname="",*rodataname="\t.section\t.rodata\n";
static int is_const(struct Typ *);
static char *labprefix="l",*idprefix="_";
static long frameoffset,pushed,maxpushed,framesize,localoffset;
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
      fprintf(f,"\taddi\t%s,%s,%ld\n",mregnames[r],mregnames[sp],(long)(zl2l(o->v->offset)+frameoffset+zl2l(o->val.vlong)));
    }else{
      fprintf(f,"\taddi\t%s,%s,%ld\n",mregnames[r],mregnames[sp],(long)(framesize+8-zl2l(o->v->offset)-zl2l(maxalign)+zl2l(o->val.vlong)));
    }
  }else{
    fprintf(f,"\tlis\t%s,",mregnames[r]);
    probj2(f,o,typ);fprintf(f,"@ha\n");
    fprintf(f,"\taddi\t%s,%s,",mregnames[r],mregnames[r]);
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
      if((g_flags[5]&USEDFLAG)&&zdeqto(vdouble,d2zd(0.0))){
        fprintf(f,"\tfsub\t%s,%s,%s\n",mregnames[r],mregnames[r],mregnames[r]);
        return;
      }
      lab=addfpconst(o,typ);
      fprintf(f,"\tlis\t%s,%s%d@ha\n",mregnames[tmp],labprefix,lab);
      fprintf(f,"\tl%s\t%s,%s%d@l(%s)\n",ldt[typ],mregnames[r],labprefix,lab,mregnames[tmp]);
      return;
    }
    if(zlleq(vlong,l2zl(32767))&&zlleq(l2zl(-32768),vlong)){
      fprintf(f,"\tli\t%s,%ld\n",mregnames[r],zl2l(vlong));
    }else{
      l=zl2l(zs2zl(zl2zs(zland(zlrshift(vlong,l2zl(16L)),l2zl(65535L)))));
      fprintf(f,"\tlis\t%s,%ld\n",mregnames[r],l);
      l=zl2l(zland(vlong,l2zl(65535L)));
      fprintf(f,"\tori\t%s,%s,%ld\n",mregnames[r],mregnames[r],l);
    }
    return;
  }
  if((o->flags&VAR)&&(o->v->storage_class==EXTERN||o->v->storage_class==STATIC)){
    if(o->flags&VARADR){
      load_address(f,r,o,POINTER);
    }else{
      fprintf(f,"\tlis\t%s,",mregnames[tmp]);
      probj2(f,o,typ);fprintf(f,"@ha\n");
      fprintf(f,"\tl%s\t%s,",ldt[typ],mregnames[r]);
      probj2(f,o,typ);fprintf(f,"@l(%s)\n",mregnames[tmp]);
    }
  }else{
    if((o->flags&(DREFOBJ|REG))==REG){
      if(r!=o->reg)
        fprintf(f,"\t%smr\t%s,%s\n",r>=33?"f":"",mregnames[r],mregnames[o->reg]);
    }else{
      fprintf(f,"\tl%s\t%s,",ldt[typ],mregnames[r]);
      probj2(f,o,typ);fprintf(f,"\n");
    }
  }
  if(typ==CHAR) fprintf(f,"\textsb\t%s,%s\n",mregnames[r],mregnames[r]);
}


static void store_reg(FILE *f,int r,struct obj *o,int typ)
/*  Generates code to store register r into memory object o.            */
{
  if((o->flags&VAR)&&(o->v->storage_class==EXTERN||o->v->storage_class==STATIC)){
    int tmp=t1;
    if(tmp==r) tmp=t2;
    fprintf(f,"\tlis\t%s,",mregnames[tmp]);
    probj2(f,o,typ);fprintf(f,"@ha\n");
    fprintf(f,"\tst%s\t%s,",sdt[typ],mregnames[r]);
    probj2(f,o,typ);fprintf(f,"@l(%s)\n",mregnames[tmp]);
    return;
  }
  fprintf(f,"\tst%s\t%s,",sdt[typ&NU],mregnames[r]);
  probj2(f,o,typ);fprintf(f,"\n");
}

static long pof2(zulong x)
/*  Yields log2(x)+1 oder 0. */
{
  zulong p;int ln=1;
  p=ul2zul(1L);
  while(ln<=32&&zulleq(p,x)){
    if(zuleqto(x,p)) return(ln);
    ln++;p=zuladd(p,p);
  }
  return(0);
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
      union atyps val;
      p->code=c=ADD;
      val.vlong=zlsub(l2zl(0L),vlong);
      eval_const(&val,LONG);
      insert_const2(&p->q2.val,typ);
    }
  }

  q1reg=q2reg=zreg=0;
  if(p->q1.flags&REG) q1reg=p->q1.reg;
  if(p->q2.flags&REG) q2reg=p->q2.reg;
  if((p->z.flags&(REG|DREFOBJ))==REG) zreg=p->z.reg;

  if((p->q1.flags&(KONST|DREFOBJ))==KONST){
    eval_const(&p->q1.val,typ);
    if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f1; else reg=t1;
    if(c==ASSIGN&&zreg) reg=zreg;
    if(c==SETRETURN&&p->z.reg) reg=p->z.reg;
    if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE||c==DIV||c==ASSIGN||c==PUSH||c==SETRETURN||c==LSHIFT||c==RSHIFT||c==COMPARE){
      load_reg(f,reg,&p->q1,typ,t1);
      q1reg=reg;
    }else{
      if((typ&NQ)<=LONG){
        if((c>=OR&&c<=AND)||(c==COMPARE&&(typ&UNSIGNED))){
          if(!zulleq(vulong,ul2zul(65535UL))){
            load_reg(f,reg,&p->q1,typ,t1);
            q1reg=reg;
          }
        }else{
          if(!zlleq(vlong,l2zl(32767L))||!zlleq(l2zl(-32768L),vlong)){
            load_reg(f,reg,&p->q2,typ,t1);
            q1reg=reg;
          }
        }
      }
    }
  }else if(c!=ADDRESS){
    if(p->q1.flags&&!q1reg){
      if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f1; else reg=t1;
      if((c==ASSIGN||(c>=CONVCHAR&&c<=CONVULONG&&c!=CONVFLOAT&&c!=CONVDOUBLE))&&zreg>=1&&zreg<=32) reg=zreg;
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
      if((c==ASSIGN||(c>=CONVCHAR&&c<=CONVULONG&&c!=CONVFLOAT&&c!=CONVDOUBLE))&&zreg>=1&&zreg<=32) reg=zreg;
      if(c==SETRETURN&&p->z.reg) reg=p->z.reg;
      if(p->q1.am)
        load_reg(f,reg,&p->q1,typ,t1);
      else
        load_reg(f,reg,cam(IMM_IND,q1reg,0),typ,t1);
      q1reg=reg;
    }
  }
  typ=p->typf;
  if((p->q2.flags&(KONST|DREFOBJ))==KONST){
    eval_const(&p->q2.val,typ);
    if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f2; else reg=t2;
    if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE||c==DIV||c==SUB||c==MOD){
      load_reg(f,reg,&p->q2,typ,t2);
      q2reg=reg;
    }else{
      if((typ&NQ)<=LONG){
        if((c>=OR&&c<=AND)||(c==COMPARE&&(typ&UNSIGNED))){
          if(!zulleq(vulong,ul2zul(65535UL))){
            load_reg(f,reg,&p->q2,typ,t2);
            q2reg=reg;
          }
        }else{
          if(!zlleq(vlong,l2zl(32767L))||!zlleq(l2zl(-32768L),vlong)){
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
      if(p->q2.am)
        load_reg(f,reg,&p->q2,typ,t2);
      else
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
  if(p->code==ADDRESS) typ=POINTER;
  if(p->z.flags){
    if(!isreg(z)){
      if(p->z.flags&DREFOBJ){
        if(p->z.flags&REG){
          if(p->z.am)
            store_reg(f,zreg,&p->z,typ);
          else
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
        fprintf(f,"\t%smr\t%s,%s\n",zreg>=33?"f":"",mregnames[p->z.reg],mregnames[zreg]);
    }
  }
}

static void probj2(FILE *f,struct obj *p,int t)
/*  Prints an object.                               */
{
  if(p->am){
    if(p->am->flags==REG_IND) fprintf(f,"%s(%s)",mregnames[p->am->offset],mregnames[p->am->base]);
    if(p->am->flags==IMM_IND) fprintf(f,"%ld(%s)",p->am->offset,mregnames[p->am->base]);
    return;
  }
  /*  if(p->flags&DREFOBJ) fprintf(f,"(");*/
  if(p->flags&VAR) {
    if(p->v->storage_class==AUTO||p->v->storage_class==REGISTER){
      if(p->flags&REG){
        fprintf(f,"%s",mregnames[p->reg]);
      }else{
        if(zl2l(p->v->offset)>=0){
          fprintf(f,"%ld(%s)",(long)(zl2l(p->v->offset)+zl2l(p->val.vlong)+frameoffset),mregnames[sp]);
        }else{
          fprintf(f,"%ld(%s)",(long)(framesize+8-zl2l(p->v->offset)-zl2l(maxalign)+zl2l(p->val.vlong)),mregnames[sp]);
        }
      }
    }else{
      if(!zleqto(l2zl(0L),p->val.vlong)){printval(f,&p->val,LONG,0);fprintf(f,"+");}
      if(p->v->storage_class==STATIC&&(p->v->vtyp->flags&NQ)!=FUNKT){
        fprintf(f,"%s%ld",labprefix,zl2l(p->v->offset));
      }else{
        fprintf(f,"%s%s",idprefix,p->v->identifier);
      }
    }
  }
  if((p->flags&REG)&&!(p->flags&VAR)) fprintf(f,"%s",mregnames[p->reg]);
  if(p->flags&KONST){
    printval(f,&p->val,t&NU,0);
  }
  /*  if(p->flags&DREFOBJ) fprintf(f,")");*/
}
static void peephole(struct IC *p)
/* Try to use addressing modes */
{
  int c,r;long of;
  for(;p;p=p->next){
    c=p->code;
    if((c==ADDI2P||c==SUBIFP)&&(p->q2.flags&KONST)&&isreg(q1)&&isreg(z)&&p->next){
      struct IC *p2=p->next;int c2=p2->code;
      eval_const(&p->q2.val,p->typf);
      of=zl2l(vlong);
      r=p->z.reg;
      if(of>=-32768&&of<=32767&&c2!=CALL&&(c2<LABEL||c2>BRA)&&p2->next&&p2->next->code==FREEREG&&p2->next->q1.reg==r){
        if(((p2->q1.flags&(REG|DREFOBJ))!=REG||p2->q1.reg!=r)&&
           ((p2->q2.flags&(REG|DREFOBJ))!=REG||p2->q2.reg!=r)&&
           ((p2->z.flags&(REG|DREFOBJ))!=REG||p2->z.reg!=r)){
          if((p2->q1.flags&(REG|DREFOBJ))==(REG|DREFOBJ)&&p2->q1.reg==r){
            p2->q1.am=mymalloc(sizeof(struct AddressingMode));
            p2->q1.am->flags=IMM_IND;
            p2->q1.am->base=p->q1.reg;
            p2->q1.am->offset=of;
          }
          if((p2->q2.flags&(REG|DREFOBJ))==(REG|DREFOBJ)&&p2->q2.reg==r){
            p2->q2.am=mymalloc(sizeof(struct AddressingMode));
            p2->q2.am->flags=IMM_IND;
            p2->q2.am->base=p->q1.reg;
            p2->q2.am->offset=of;
          }
          if((p2->z.flags&(REG|DREFOBJ))==(REG|DREFOBJ)&&p2->z.reg==r){
            p2->z.am=mymalloc(sizeof(struct AddressingMode));
            p2->z.am->flags=IMM_IND;
            p2->z.am->base=p->q1.reg;
            p2->z.am->offset=of;
          }
          p->code=NOP;p->q1.flags=p->q2.flags=p->z.flags=0;
        }
      }
    }
  }
}
static void function_top(FILE *f,struct Var *v,long offset)
/*  Generates function top.                             */
{
  int i;long of;
  if(section!=CODE){fprintf(f,codename);section=CODE;}
  if(v->storage_class==EXTERN) fprintf(f,"\t.global\t%s%s\n",idprefix,v->identifier);
  fprintf(f,"\t.align\t4\n%s%s:\n",idprefix,v->identifier);
  frameoffset=8+maxpushed;
  framesize=frameoffset+offset;
  for(i=1;i<=64;i++){
    if(regused[i]&&!regscratch[i]&&!regsa[i]){
      if(i<=32) framesize+=4; else framesize+=8;
    }
  }
  for(crsave=0,i=65;i<=72;i++)
    if(regused[i]&&!regscratch[i]&&!regsa[i]) crsave=1;
  if(crsave) framesize+=4;
  if(framesize==8&&function_calls==0) framesize=frameoffset=0;
  framesize=(framesize+15)/16*16;
  if(framesize>32767) ierror(0);
  if(framesize!=0)
    fprintf(f,"\tstwu\t%s,-%ld(%s)\n",mregnames[sp],framesize,mregnames[sp]);
  if(function_calls)
    fprintf(f,"\tmflr\t%s\n\tst%s\t%s,%ld(%s)\n",mregnames[t1],sdt[LONG],mregnames[t1],framesize+4,mregnames[sp]);
  of=8+maxpushed+offset;
  if(crsave){
    fprintf(f,"\tmfcr\t%s\n\tst%s\t%s,%ld(%s)\n",mregnames[t1],sdt[LONG],mregnames[t1],of,mregnames[sp]);
    of+=4;
  }
  for(i=1;i<=64;i++){
    if(regused[i]&&!regscratch[i]&&!regsa[i]){
      if(i<=32){
        fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[LONG],mregnames[i],of,mregnames[sp]);
        of+=4;
      }else{
        fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[DOUBLE],mregnames[i],of,mregnames[sp]);
        of+=8;
      }
    }
  }
}
static void function_bottom(FILE *f,struct Var *v,long offset)
/*  Generates function bottom.                          */
{
  int i;long of;
  of=8+maxpushed+offset;
  if(crsave){
    fprintf(f,"\tl%s\t%s,%ld(%s)\n\tmtcr\t%s\n",ldt[LONG],mregnames[t1],of,mregnames[sp],mregnames[t1]);
    of+=4;
  }
  for(i=1;i<=64;i++){
    if(regused[i]&&!regscratch[i]&&!regsa[i]){
      if(i<=32){
        fprintf(f,"\tl%s\t%s,%ld(%s)\n",ldt[LONG],mregnames[i],of,mregnames[sp]);
        of+=4;
      }else{
        fprintf(f,"\tl%s\t%s,%ld(%s)\n",ldt[DOUBLE],mregnames[i],of,mregnames[sp]);
        of+=8;
      }
    }
  }
  if(function_calls)
    fprintf(f,"\tl%s\t%s,%ld(%s)\n\tmtlr\t%s\n",ldt[LONG],mregnames[t1],framesize+4,mregnames[sp],mregnames[t1]);
  if(framesize) fprintf(f,"\taddi\t%s,%s,%ld\n",mregnames[sp],mregnames[sp],framesize);
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
static int balign(struct obj *o)
/*  Liefert die unteren 2 Bits des Objekts. -1 wenn unklar. */
{
  int sc;
  if(o->flags&DREFOBJ) return -1;
  if(o->am) ierror(0);
  if(!(o->flags&VAR)) ierror(0);
  sc=o->v->storage_class;
  if(sc==EXTERN||sc==STATIC){
    /* Alle statischen Daten werden vom cg auf 32bit alignt. */
    return zl2l(zland(o->val.vlong,l2zl(3L)));
  }
  if(sc==AUTO||sc==REGISTER){
    zlong of=o->v->offset;
    if(!zlleq(l2zl(0L),of))
      of=zlsub(l2zl(0L),zladd(of,maxalign));
    return zl2l(zland(zladd(of,o->val.vlong),l2zl(3L)));
  }
  ierror(0);
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
  if(g_flags[7]&USEDFLAG){
    malign[INT]=malign[LONG]=malign[POINTER]=malign[FLOAT]=malign[DOUBLE]=2;
  }
  for(i=0;i<16;i++){
    sizetab[i]=l2zl(msizetab[i]);
    align[i]=l2zl(malign[i]);
  }
  for(i=0;i<=MAXR;i++) mregnames[i]=regnames[i];
  for(i= 1;i<=32;i++){
    regsize[i]=l2zl(4L);
    regtype[i]=&ltyp;
    if(g_flags[8]&USEDFLAG) mregnames[i]++;
  }
  for(i=33;i<=64;i++){
    regsize[i]=l2zl(8L);
    regtype[i]=&ldbl;
    if(g_flags[8]&USEDFLAG) mregnames[i]++;
  }
  for(i=65;i<=72;i++){
    regsize[i]=l2zl(1L);
    regtype[i]=&lchar;
    if(g_flags[8]&USEDFLAG) mregnames[i]+=2;
  }

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
  regsa[sp]=regsa[fp]=regsa[sd]=regsa[r2]=1;
  regscratch[t1]=regscratch[t2]=regscratch[t3]=0;
  regscratch[f1]=regscratch[f2]=regscratch[f3]=0;
  regscratch[sp]=regscratch[fp]=regscratch[sd]=regscratch[r2]=0;

  if(g_flags[6]&USEDFLAG) {labprefix=".l";idprefix="";}

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
    fprintf(f,"\t.align\t2\n");
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
        if(v->clist&&constflag&&!(g_flags[2]&USEDFLAG)&&section!=RODATA){fprintf(f,rodataname);section=RODATA;}
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
            if(v->clist&&constflag&&!(g_flags[2]&USEDFLAG)&&section!=RODATA){fprintf(f,rodataname);section=RODATA;}
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
  int c,t,i,addbuf,varargs=0,fixedgpr,fixedfpr;
  char *fpp;struct IC *m;
  long of=(zl2l(offset)+3)/4*4,regbase,tmpoff;
  if(DEBUG&1) printf("gen_code()\n");
  for(c=1;c<=MAXR;c++) regs[c]=regsa[c];
  maxpushed=0;addbuf=0;
  for(m=p;m;m=m->next){
    c=m->code;t=m->typf&NU;
    if(c==ALLOCREG) {regs[m->q1.reg]=1;continue;}
    if(c==FREEREG) {regs[m->q1.reg]=0;continue;}
    if(c==COMPARE&&(m->q2.flags&KONST)){
      eval_const(&m->q2.val,t);
      if(zleqto(vlong,l2zl(0L))&&zdeqto(vdouble,d2zd(0.0))){
        m->q2.flags=0;m->code=c=TEST;
      }
    }
    if((t&NQ)<=LONG&&(m->q2.flags&KONST)&&(t&NQ)<=LONG&&(c==MULT||c==DIV||(c==MOD&&(t&UNSIGNED)))){
      eval_const(&m->q2.val,t);
      i=pof2(vlong);
      if(i){
        if(c==MOD){
          vlong=zlsub(vlong,l2zl(1L));
          m->code=AND;
        }else{
          vlong=l2zl(i-1);
          if(c==DIV) m->code=RSHIFT; else m->code=LSHIFT;
        }
        c=m->code;
        if((t&NU)==CHAR) m->q2.val.vchar=zl2zc(vlong);
        if((t&NU)==SHORT) m->q2.val.vshort=zl2zs(vlong);
        if((t&NU)==INT) m->q2.val.vint=zl2zi(vlong);
        if((t&NU)==LONG) m->q2.val.vlong=vlong;
        vulong=zl2zul(vlong);
        if((t&NU)==(UNSIGNED|CHAR)) m->q2.val.vuchar=zul2zuc(vulong);
        if((t&NU)==(UNSIGNED|SHORT)) m->q2.val.vushort=zul2zus(vulong);
        if((t&NU)==(UNSIGNED|INT))  m->q2.val.vuint=zul2zui(vulong);
        if((t&NU)==(UNSIGNED|LONG)) m->q2.val.vulong=vulong;
      }
    }
    if((c==CONVFLOAT||c==CONVDOUBLE)&&t!=FLOAT&&t!=DOUBLE&&addbuf<8) addbuf=8;
    if((t==FLOAT||t==DOUBLE)&&c>=CONVCHAR&&c<=CONVULONG&&addbuf<8) addbuf=8;
    if(c==CALL&&maxpushed<zl2l(m->q2.val.vlong)) maxpushed=zl2l(m->q2.val.vlong);
    if(c==CALL&&(m->q1.flags&VAR)&&!strcmp(m->q1.v->identifier,"__va_start")) varargs=1;
  }
  if(g_flags[9]&USEDFLAG) peephole(p);
  if(varargs){
    fixedgpr=fixedfpr=0;
    for(i=0;i<v->vtyp->exact->count;i++){
      c=(*v->vtyp->exact->sl)[i].styp->flags&NQ;
      if(c==POINTER||c<=LONG) fixedgpr++;
      if(c==FLOAT||c==DOUBLE) fixedfpr++;
    }
    regbase=of; /* merken */
    addbuf+=96;
  }
  of+=addbuf;tmpoff=8+maxpushed+of;
  function_top(f,v,of);
  if(varargs){
    regbase=frameoffset+regbase;
    fpp="";
    if(!(g_flags[8]&USEDFLAG)) fpp="r";
    for(i=fixedgpr;i<8;i++)
      fprintf(f,"\tstw\t%s%d,%ld(%s)\n",fpp,i+3,regbase+i*4,mregnames[sp]);
    if(!(g_flags[8]&USEDFLAG)) fpp="f";
    for(i=fixedfpr;i<8;i++)
      fprintf(f,"\tstfd\t%s%d,%ld(%s)\n",fpp,i+1,regbase+32+i*8,mregnames[sp]);
  }
  pushed=0;
  for(;p;pr(f,p),p=p->next){
    c=p->code;t=p->typf;
    if(c==NOP) continue;
    if(c==ALLOCREG) {regs[p->q1.reg]=1;continue;}
    if(c==FREEREG) {regs[p->q1.reg]=0;continue;}
    if(c==LABEL) {fprintf(f,"%s%d:\n",labprefix,t);continue;}
    if(c==BRA) {fprintf(f,"\tb\t%s%d\n",labprefix,t);continue;}
    if(c>=BEQ&&c<BRA){
      if(!(p->q1.flags&REG)) p->q1.reg=65;
      fprintf(f,"\tb%s\t%s,%s%d\n",ccs[c-BEQ],mregnames[p->q1.reg],labprefix,t);
      continue;
    }
    if(c==MOVETOREG){
      if(p->z.reg<=32){
        load_reg(f,p->z.reg,&p->q1,INT,0);
      }else{
        if(p->z.reg>64) ierror(0);
        load_reg(f,p->z.reg,&p->q1,DOUBLE,0);
      }
      p->z.flags=0;
      continue;
    }
    if(c==MOVEFROMREG){
      if(p->q1.reg<=32){
        store_reg(f,p->q1.reg,&p->z,INT);
      }else{
        if(p->q1.reg>64) ierror(0);
        store_reg(f,p->q1.reg,&p->z,DOUBLE);
      }
      p->z.flags=0;
      continue;
    }
    if((c==ASSIGN||c==PUSH)&&((t&NQ)>POINTER||((t&NQ)==CHAR&&zl2l(p->q2.val.vlong)!=1))){
      unsigned long size,l;
      int a1,a2,b;char *ld,*st;
      size=zl2l(p->q2.val.vlong);
      a1=balign(&p->q1);
      if(c==ASSIGN) a2=balign(&p->z); else a2=0;
      b=1;ld=ldt[CHAR];st=sdt[CHAR];
      if(a1>=0&&a2>=0){
        if(a1==0&&a2==0){
          b=4;ld=ldt[INT];st=sdt[INT];
        }else if((a1&1)==0&&(a2&1)==0){
          b=2;ld=ldt[SHORT];st=sdt[SHORT];
        }
      }
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
          if(!(p->q1.flags&VAR)) ierror(0);
          if(zl2l(falign(p->q1.v->vtyp))==8) pushed=(pushed+7)/8*8;
          fprintf(f,"\taddi\t%s,%s,%ld\n",mregnames[t2],mregnames[sp],pushed+8-b);
          pushed+=size;
        }else{
          load_address(f,t2,&p->z,POINTER);
        }
      }
      fprintf(f,"\taddi\t%s,%s,-%d\n",mregnames[t1],mregnames[t1],b);
      if(c==ASSIGN) fprintf(f,"\taddi\t%s,%s,-%d\n",mregnames[t2],mregnames[t2],b);
      l=size/(8*b);
      if(l>1){
        if((l>>16)&65535) fprintf(f,"\tlis\t%s,%lu\n",mregnames[t3],(l>>16)&65535);
        fprintf(f,"\tli\t%s,%lu\n",mregnames[t3],l&65535);
        fprintf(f,"\tmtctr\t%s\n",mregnames[t3]);
        fprintf(f,"%s%d:\n",labprefix,++label);
      }
      if(l>0){
        for(i=b;i<=7*b;i+=b){
          fprintf(f,"\tl%s\t%s,%d(%s)\n",ld,mregnames[t3],i,mregnames[t1]);
          fprintf(f,"\tst%s\t%s,%d(%s)\n",st,mregnames[t3],i,mregnames[t2]);
        }
        fprintf(f,"\tl%su\t%s,%d(%s)\n",ld,mregnames[t3],i,mregnames[t1]);
        fprintf(f,"\tst%su\t%s,%d(%s)\n",st,mregnames[t3],i,mregnames[t2]);
      }
      if(l>1){
        fprintf(f,"\tbdnz\t%s%d\n",labprefix,label);
      }
      size=size%(8*b);
      for(i=0;i<size/b;i++){
        fprintf(f,"\tl%su\t%s,%d(%s)\n",ld,mregnames[t3],b,mregnames[t1]);
        fprintf(f,"\tst%su\t%s,%d(%s)\n",st,mregnames[t3],b,mregnames[t2]);
      }
      size=size%b;i=b;
      if(size&2){
        fprintf(f,"\tl%su\t%s,%d(%s)\n",ldt[SHORT],mregnames[t3],b,mregnames[t1]);
        fprintf(f,"\tst%su\t%s,%d(%s)\n",sdt[SHORT],mregnames[t3],b,mregnames[t2]);
        i=2;
      }
      if(size&1){
        fprintf(f,"\tl%su\t%s,%d(%s)\n",ldt[CHAR],mregnames[t3],i,mregnames[t1]);
        fprintf(f,"\tst%su\t%s,%d(%s)\n",sdt[CHAR],mregnames[t3],i,mregnames[t2]);
      }
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
      static struct obj o;char *ip;
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
          zreg=q1reg;
          continue;
        }
        if((t&NU)==(UNSIGNED|INT)||(t&NU)==(UNSIGNED|LONG)){
          o.flags=KONST;
          ip=(char *)&o.val.vdouble;
          ip[0]=0x41;
          ip[1]=0xe0;
          ip[2]=0x00;
          ip[3]=0x00;
          ip[4]=0x00;
          ip[5]=0x00;
          ip[6]=0x00;
          ip[7]=0x00;
          load_reg(f,f2,&o,DOUBLE,t2);
          fprintf(f,"\tfcmpu\t%s,%s,%s\n",mregnames[65],mregnames[q1reg],mregnames[f2]);
          fprintf(f,"\tcror\t3,2,1\n");
          fprintf(f,"\tbso\t%s,%s%d\n",mregnames[65],labprefix,++label);
          fprintf(f,"\tfctiwz\t%s,%s\n",mregnames[f2],mregnames[q1reg]);
          fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[DOUBLE],mregnames[f2],tmpoff-8,mregnames[sp]);
          fprintf(f,"\tl%s\t%s,%ld(%s)\n",ldt[t&NQ],mregnames[zreg],tmpoff-zl2l(sizetab[t&NQ]),mregnames[sp]);
          fprintf(f,"\tb\t%s%d\n",labprefix,++label);
          fprintf(f,"%s%d:\n",labprefix,label-1);
          fprintf(f,"\tfsub\t%s,%s,%s\n",mregnames[f2],mregnames[q1reg],mregnames[f2]);
          fprintf(f,"\tfctiwz\t%s,%s\n",mregnames[f2],mregnames[f2]);
          fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[DOUBLE],mregnames[f2],tmpoff-8,mregnames[sp]);
          fprintf(f,"\tl%s\t%s,%ld(%s)\n",ldt[INT],mregnames[zreg],tmpoff-zl2l(sizetab[t&NQ]),mregnames[sp]);
          fprintf(f,"\txoris\t%s,%s,32768\n",mregnames[zreg],mregnames[zreg]);
          fprintf(f,"%s%d:\n",labprefix,label);
        }else{
          fprintf(f,"\tfctiwz\t%s,%s\n",mregnames[f3],mregnames[q1reg]);
          fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[DOUBLE],mregnames[f3],tmpoff-8,mregnames[sp]);
          fprintf(f,"\tl%s\t%s,%ld(%s)\n",ldt[t&NQ],mregnames[zreg],tmpoff-zl2l(sizetab[t&NQ]),mregnames[sp]);
        }
        if(t==CHAR) fprintf(f,"\textsb\t%s,%s\n",mregnames[zreg],mregnames[zreg]);
        continue;
      }
      if((t&NQ)==FLOAT||(t&NQ)==DOUBLE){
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
        if((to&NU)==(UNSIGNED|INT)||(to&NU)==(UNSIGNED|LONG)){
          ip[4]=0x00;
          load_reg(f,f2,&o,DOUBLE,t2);
          fprintf(f,"\tlis\t%s,17200\n",mregnames[t2]);
          fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[INT],mregnames[q1reg],tmpoff-4,mregnames[sp]);
          fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[INT],mregnames[t2],tmpoff-8,mregnames[sp]);
          fprintf(f,"\tl%s\t%s,%ld(%s)\n",ldt[DOUBLE],mregnames[zreg],tmpoff-8,mregnames[sp]);
          fprintf(f,"\tfsub\t%s,%s,%s\n",mregnames[zreg],mregnames[zreg],mregnames[f2]);
        }else{
          fprintf(f,"\tlis\t%s,17200\n",mregnames[t2]);
          fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[INT],mregnames[t2],tmpoff-8,mregnames[sp]);
          fprintf(f,"\txoris\t%s,%s,32768\n",mregnames[t2],mregnames[q1reg]);
          fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[INT],mregnames[t2],tmpoff-4,mregnames[sp]);
          fprintf(f,"\tl%s\t%s,%ld(%s)\n",ldt[DOUBLE],mregnames[zreg],tmpoff-8,mregnames[sp]);
          load_reg(f,f2,&o,DOUBLE,t2);
          fprintf(f,"\tfsub\t%s,%s,%s\n",mregnames[zreg],mregnames[zreg],mregnames[f2]);
        }
        continue;
      }
      if((t&NQ)>=(to&NQ)){
        zreg=q1reg;
        continue;
      }else{
        if((t&NU)==CHAR) fprintf(f,"\textsb\t%s,%s\n",mregnames[zreg],mregnames[q1reg]);
        if((t&NU)==SHORT) fprintf(f,"\textsh\t%s,%s\n",mregnames[zreg],mregnames[q1reg]);
        if((t&NU)==(UNSIGNED|CHAR)) fprintf(f,"\tandi.\t%s,%s,255\n",mregnames[zreg],mregnames[q1reg]);
        if((t&NU)==(UNSIGNED|SHORT)) fprintf(f,"\tandi.\t%s,%s,65535\n",mregnames[zreg],mregnames[q1reg]);
        continue;
      }
    }
    if(c==KOMPLEMENT){
      fprintf(f,"\tnor\t%s,%s,%s\n",mregnames[zreg],mregnames[q1reg],mregnames[q1reg]);
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
        p->z.flags=0;
      continue;
    }
    if(c==CALL){
      int reg;
      if((p->q1.flags&VAR)&&p->q1.v->fi&&p->q1.v->fi->inline_asm){
        fprintf(f,"%s\n",p->q1.v->fi->inline_asm);
      }else{
        if(p->q1.flags&VAR){
          if(!strcmp("__va_start",p->q1.v->identifier)){
            fprintf(f,"\taddi\t%s,%s,%ld\n",mregnames[r3],mregnames[sp],framesize+8);
            continue;
          }
          if(!strcmp("__va_regbase",p->q1.v->identifier)){
            fprintf(f,"\taddi\t%s,%s,%ld\n",mregnames[r3],mregnames[sp],regbase);
            continue;
          }
          if(!strcmp("__va_fixedgpr",p->q1.v->identifier)){
            fprintf(f,"\tli\t%s,%d\n",mregnames[r3],fixedgpr);
            continue;
          }
          if(!strcmp("__va_fixedfpr",p->q1.v->identifier)){
            fprintf(f,"\tli\t%s,%d\n",mregnames[r3],fixedfpr);
            continue;
          }
        }
        if(g_flags[10]&USEDFLAG) fprintf(f,"\tcreqv\t6,6,6\n");
        if(q1reg){
          fprintf(f,"\tmtlr\t%s\n",mregnames[q1reg]);
          fprintf(f,"\tblrl\n");
        }else{
          fprintf(f,"\tbl\t");probj2(f,&p->q1,t);
          fprintf(f,"\n");
        }
      }
      pushed-=zl2l(p->q2.val.vlong);
      continue;
    }
    if(c==ASSIGN||c==PUSH){
      if(t==0) ierror(0);
      if(q1reg){
        if(c==PUSH){
          if((t&NQ)==DOUBLE) pushed=(pushed+7)/8*8;
          fprintf(f,"\tst%s\t%s,%ld(%s)\n",sdt[t&NQ],mregnames[q1reg],pushed+8,mregnames[sp]);
          pushed+=zl2l(p->q2.val.vlong);
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
      fprintf(f,"\t%sneg\t%s,%s\n",fpp,mregnames[zreg],mregnames[q1reg]);
      continue;
    }
    if(c==TEST){
      if(!(p->z.flags&REG))
        p->z.reg=65;
      if((t&NQ)==FLOAT||(t&NQ)==DOUBLE)
        ierror(0);
      else
        fprintf(f,"\tcmp%swi\t%s,%s,0\n",(t&UNSIGNED)?"l":"",mregnames[p->z.reg],mregnames[q1reg]);
      continue;
    }
    if(c==COMPARE){
      if(!(p->z.flags&REG))
        p->z.reg=65;
      if((t&NQ)==FLOAT||(t&NQ)==DOUBLE)
        fprintf(f,"\tfcmpu\t%s,%s,",mregnames[p->z.reg],mregnames[q1reg]);
      else
        fprintf(f,"\tcmp%sw%s\t%s,%s,",(t&UNSIGNED)?"l":"",isimm[q2reg==0],mregnames[p->z.reg],mregnames[q1reg]);
      probj2(f,&p->q2,t);fprintf(f,"\n");
      continue;
    }
    if(c>=OR&&c<=AND){
      fprintf(f,"\t%s%s%s\t%s,%s,",logicals[c-OR],isimm[q2reg==0],(q2reg==0&&c==AND)?".":"",mregnames[zreg],mregnames[q1reg]);
      probj2(f,&p->q2,t|UNSIGNED);fprintf(f,"\n");
      continue;
    }
    if(c==SUB&&(p->q1.flags&KONST)){
      fprintf(f,"\tsubfic\t%s,%s,",mregnames[zreg],mregnames[q2reg]);
      probj2(f,&p->q1,t&NQ);fprintf(f,"\n");
      continue;
    }
    if(c>=LSHIFT&&c<=MOD){
      if(c==MOD){
        i=0;
        if(zreg==q1reg||zreg==q2reg){
          if(t1!=q1reg&&t1!=q2reg) i=t1;
          if(t2!=q1reg&&t2!=q2reg) i=t2;
        }else i=zreg;
        if(i==0||i==q1reg||i==q2reg) ierror(0);
        fprintf(f,"\tdivw%s\t%s,%s,%s\n",(t&UNSIGNED)?"u":"",mregnames[i],mregnames[q1reg],mregnames[q2reg]);
        fprintf(f,"\tmullw\t%s,%s,%s\n",mregnames[i],mregnames[i],mregnames[q2reg]);
        fprintf(f,"\tsubf\t%s,%s,%s\n",mregnames[zreg],mregnames[i],mregnames[q1reg]);
        continue;
      }
      if(c==DIV&&(t&UNSIGNED))
        fprintf(f,"\tdivwu%s\t%s,%s,",isimm[q2reg==0],mregnames[zreg],mregnames[q1reg]);
      else if(c==MULT&&((t&NQ)==FLOAT||(t&NQ)==DOUBLE))
        fprintf(f,"\tfmul\t%s,%s,",mregnames[zreg],mregnames[q1reg]);
      else if(c==DIV&&((t&NQ)==FLOAT||(t&NQ)==DOUBLE))
        fprintf(f,"\tfdiv\t%s,%s,",mregnames[zreg],mregnames[q1reg]);
      else if(c==MULT&&q2reg==0)
        fprintf(f,"\tmulli\t%s,%s,",mregnames[zreg],mregnames[q1reg]);
      else
        fprintf(f,"\t%s%s%s\t%s,%s,",fpp,arithmetics[c-LSHIFT],isimm[q2reg==0],mregnames[zreg],mregnames[q1reg]);
      probj2(f,&p->q2,t&NQ);fprintf(f,"\n");
      continue;
    }
    ierror(0);
  }
  function_bottom(f,v,of);
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
      if(section!=RODATA){fprintf(f,rodataname);section=RODATA;}
      if((p->typ&NQ)==DOUBLE)
        fprintf(f,"\t.align\t3\n");
      else
        fprintf(f,"\t.align\t2\n");
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


