/*  Code generator for a DEC Alpha 64bit RISC cpu with 32       */
/*  general purpose and 32 floating point registers.            */

#include "supp.h"

static char FILE_[]=__FILE__;

/*  Public data that MUST be there.                             */

/* Name and copyright. */
char cg_copyright[]="vbcc code-generator for DEC Alpha V0.3a (c) in 1997 by Volker Barthelmann";

/*  Commandline-flags the code-generator accepts                */
int g_flags[MAXGF]={VALFLAG,VALFLAG,0,0,0,0,0};
char *g_flags_name[MAXGF]={"cpu","fpu","const-in-data","sd","merge-constants","g","no-builtins"};
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
			"$0","$1","$2","$3","$4","$5","$6","$7",
			"$8","$9","$10","$11","$12","$13","$14","$15",
			"$16","$17","$18","$19","$20","$21","$22","$23",
			"$24","$25","$26","$27","$28","$29","$30","$31",
			"$f0","$f1","$f2","$f3","$f4","$f5","$f6","$f7",
			"$f8","$f9","$f10","$f11","$f12","$f13","$f14","$f15",
			"$f16","$f17","$f18","$f19","$f20","$f21","$f22","$f23",
			"$f24","$f25","$f26","$f27","$f28","$f29","$f30","$f31"};

/*  The Size of each register in bytes.                         */
zlong regsize[MAXR+1];

/*  A type which can store each register. */
struct Typ *regtype[MAXR+1];

/*  regsa[reg]!=0 if a certain register is allocated and should */
/*  not be used by the compiler pass.                           */
int regsa[MAXR+1];

/*  Specifies which registers may be scratched by functions.    */
int regscratch[MAXR+1]={0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,
			  1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,
                          1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,
                          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

struct reg_handle empty_reg_handle={0};


/****************************************/
/*  Private data and functions.         */
/****************************************/

static long malign[16]=  {1,1,2,4,8,4,8,1,8,1,1,1,4,1};
static long msizetab[16]={1,1,2,4,8,4,8,0,8,0,0,0,4,0};

static struct Typ ltyp={LONG};

static char x_t[16]={'?','b','w','l','q','s','t','?','q'};

static int sp=31;                  /*  Stackpointer                        */
static int gp=30;                  /*  Global pointer                      */
static int lr=27;                  /*  Link Register                       */
static int vp=28;                  /*  Virtual frame pointer               */
static int r31=32;                 /*  Read as zero                        */
static int f31=64;                 /*  Read as zero                        */
static int sd=14;                  /*  SmallDataPointer                    */
static int t1=2,t2=3,t3=4;         /*  Temporaries used by code generator  */
static int t4=5,t5=6;
static int f1=34,f2=35,f3=36;      /*  Temporaries used by code generator  */

#define DATA 0
#define BSS 1
#define CODE 2

static int section=-1,newobj,crsave,helpl,helps,stabs;
static char *codename="\t.text\n",*dataname="\t.data\n",*bssname="";
static int is_const(struct Typ *);
static int balign(struct obj *);
static char *labprefix="$C",*idprefix="";
static long framesize,frameoffset;
static void probj2(FILE *f,struct obj *p,int t);

#define ESGN 1
#define EUNS 2
static int st[MAXR+1];

static struct fpconstlist {
  struct fpconstlist *next;
  int label,typ;
  union atyps val;
} *firstfpc;

static void move_reg(FILE *f,int s,int t)
{
  if(t==r31||t==f31) ierror(0);  
  if(s<=32&&t>32) ierror(0);
  if(t<=32&&s>32) ierror(0);
  if(s==t) return;
  if(s<=32){
    fprintf(f,"\tbis\t%s,%s,%s\n",regnames[r31],regnames[s],regnames[t]);
  }else{
    fprintf(f,"\tcpys\t%s,%s,%s\n",regnames[s],regnames[s],regnames[t]);
  }
  st[t]=st[s];
}

static int addfpconst(struct obj *o,int t)
{
  struct fpconstlist *p=firstfpc;
  t&=NQ;
  if(g_flags[4]&USEDFLAG){
    for(p=firstfpc;p;p=p->next){
      if(t==p->typ){
	eval_const(&p->val,t);
	if(t==INT&&zleqto(vlong,zi2zl(o->val.vint))) return(p->label);
	if(t==LONG&&zleqto(vlong,o->val.vlong)) return(p->label);
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

static struct obj *cam(int flags,int base,int align,long offset)
/*  Initializes an addressing-mode structure and returns a pointer to   */
/*  that object. Will not survive a second call!                        */
{
  static struct obj obj;
  static struct AddressingMode am;
  obj.am=&am;
  am.flags=flags;
  am.base=base;
  am.offset=offset;
  am.align=align;
  return(&obj);
}

static char *dct[]={"??","byte","short","long","quad","long","long","quad","??","??","??","??"};

static void load_address(FILE *f,int r,struct obj *o,int typ)
/*  Generates code to load the address of a variable into register r.   */
{
  if((o->flags&VAR)&&(o->v->storage_class==AUTO||o->v->storage_class==REGISTER)){
    long of=zl2l(o->v->offset);
    if(of>=0)
      of+=frameoffset+zl2l(o->val.vlong);
    else
      of=-of-zl2l(maxalign)+framesize+zl2l(o->val.vlong);
    if(of>32767) ierror(0);
    fprintf(f,"\tlda\t%s,%ld(%s)\n",regnames[r],of,regnames[sp]);
  }else{
    fprintf(f,"\tlda\t%s,",regnames[r]);
    probj2(f,o,typ);fprintf(f,"\n");
  }
  st[r]=ESGN;
}
static void load_obj(FILE *f,int r,struct obj *o,int typ,int tmp)
{
  int adr,shift,al;  
  switch(typ&NQ){
  case POINTER:
  case DOUBLE:
  case FLOAT:
  case LONG:
  case INT:
    fprintf(f,"\tld%c\t%s,",x_t[typ&NQ],regnames[r]);
    probj2(f,o,typ);fprintf(f,"\n");
    st[r]=ESGN;return;
  case SHORT:
    al=balign(o);
    if(al>=0){
      fprintf(f,"\tldq_u\t%s,",regnames[r]);
      probj2(f,o,typ);fprintf(f,"\n");
      shift=64-16-al*8;
      if(shift) fprintf(f,"\tsll\t%s,%d,%s\n",regnames[r],shift,regnames[r]);
      if(typ&UNSIGNED){
	fprintf(f,"\tsrl\t%s,%d,%s\n",regnames[r],64-16,regnames[r]);
	st[r]=EUNS;
      }else{
	fprintf(f,"\tsra\t%s,%d,%s\n",regnames[r],64-16,regnames[r]);
	st[r]=ESGN;
      }
      return;
    }
    adr=0;
    if(o->am&&o->am->offset==0) 
      adr=o->am->base;
    else if(zleqto(o->val.vlong,l2zl(0L)))
      adr=o->reg;
    if(!adr){
      adr=tmp;load_address(f,adr,o,POINTER);
    }

    if(typ&UNSIGNED){
      fprintf(f,"\tbic\t%s,6,%s\n",regnames[adr],regnames[t4]); /* ldq_u ? */
      fprintf(f,"\tldq\t%s,0(%s)\n",regnames[t5],regnames[t4]);
      fprintf(f,"\tbic\t%s,1,%s\n",regnames[adr],regnames[t4]);
      fprintf(f,"\textwl\t%s,%s,%s\n",regnames[t5],regnames[t4],regnames[r]);
      /*
      fprintf(f,"\tbic\t%s,2,%s\n",regnames[adr],regnames[t4]);
      fprintf(f,"\tand\t%s,2,%s\n",regnames[adr],regnames[t5]);
      fprintf(f,"\tldq\t%s,0(%s)\n",regnames[r],regnames[t4]);
      fprintf(f,"\textwh\t%s,%s,%s\n",regnames[r],regnames[t5],regnames[r]);
      */
      st[r]=EUNS;return;
    }else{
      fprintf(f,"\tbic\t%s,6,%s\n",regnames[adr],regnames[t4]);
      fprintf(f,"\tand\t%s,6,%s\n",regnames[adr],regnames[t5]);
      fprintf(f,"\tldq\t%s,0(%s)\n",regnames[r],regnames[t4]);
      fprintf(f,"\tlda\t%s,2(%s)\n",regnames[t5],regnames[t5]);
      fprintf(f,"\textqh\t%s,%s,%s\n",regnames[r],regnames[t5],regnames[r]);
      fprintf(f,"\tsra\t%s,48,%s\n",regnames[r],regnames[r]);
      st[r]=ESGN;return;
    }
  case CHAR:
    al=balign(o);
    if(al>=0){
      fprintf(f,"\tldq_u\t%s,",regnames[r]);
      probj2(f,o,typ);fprintf(f,"\n");
      shift=64-8-al*8;
      if(shift&&al) fprintf(f,"\tsll\t%s,%d,%s\n",regnames[r],shift,regnames[r]);
      if(typ&UNSIGNED){
	if(al==0)
	  fprintf(f,"\tand\t%s,255,%s\n",regnames[r],regnames[r]);
	else
	  fprintf(f,"\tsrl\t%s,%d,%s\n",regnames[r],64-8,regnames[r]);
	st[r]=EUNS;
      }else{
	if(al==0)
	  fprintf(f,"\tsextb\t%s,%s\n",regnames[r],regnames[r]);
	else
	  fprintf(f,"\tsra\t%s,%d,%s\n",regnames[r],64-8,regnames[r]);
	st[r]=ESGN;
      }
      return;
    }
    if(typ&UNSIGNED){
      adr=0;
      if(o->am&&o->am->offset==0) 
	adr=o->am->base;
      else if(zleqto(o->val.vlong,l2zl(0L)))
	adr=o->reg;
      if(!adr){
	adr=tmp; load_address(f,adr,o,POINTER);
      }
      fprintf(f,"\tldq_u\t%s,0(%s)\n",regnames[r],regnames[adr]);
      fprintf(f,"\textbl\t%s,%s,%s\n",regnames[r],regnames[adr],regnames[r]);
      st[r]=EUNS;return;
    }else{
      fprintf(f,"\tldq_u\t%s,",regnames[t5]);
      probj2(f,o,typ);fprintf(f,"\n\tlda\t%s,",regnames[t4]);
      if(o->am) o->am->offset++; else o->val.vlong=zladd(o->val.vlong,l2zl(1L));
      probj2(f,o,typ);
      if(o->am) o->am->offset--; else o->val.vlong=zlsub(o->val.vlong,l2zl(1L));
      fprintf(f,"\n\textqh\t%s,%s,%s\n",regnames[t5],regnames[t4],regnames[t5]);
      fprintf(f,"\tsra\t%s,56,%s\n",regnames[t5],regnames[r]);
      st[r]=ESGN;return;
    }
  }
  ierror(0);
}
static void load_reg(FILE *f,int r,struct obj *o,int typ,int tmp)
/*  Generates code to load a memory object into register r. tmp is a    */
/*  general purpose register which may be used. tmp can be r.           */
{
  typ&=NU;
  if(o->am){
    load_obj(f,r,o,typ,tmp);
    return;
  }
  if(o->flags&KONST){
    long l;int lab;
    eval_const(&o->val,typ);
    if(typ==FLOAT||typ==DOUBLE){
      lab=addfpconst(o,typ);
      fprintf(f,"\tlda\t%s,%s%d\n",regnames[tmp],labprefix,lab);
      fprintf(f,"\tld%c\t%s,0(%s)\n",x_t[typ&NQ],regnames[r],regnames[tmp]);
      st[r]=ESGN;
      return;
    }
    if(zlleq(vlong,l2zl(32767))&&zlleq(l2zl(-32768),vlong)){
      fprintf(f,"\tlda\t%s,%ld(%s)\n",regnames[r],zl2l(vlong),regnames[r31]);
    }else{
/*       if((typ&NQ)<INT) ierror(0); */
      lab=addfpconst(o,typ);
      fprintf(f,"\tlda\t%s,%s%d\n",regnames[tmp],labprefix,lab);
      fprintf(f,"\tldq\t%s,0(%s)\n",regnames[r],regnames[tmp]);
    }
    st[r]=ESGN;
    return;
  }
  if((o->flags&VAR)&&(o->v->storage_class==EXTERN||o->v->storage_class==STATIC)){
    if(!(o->flags&VARADR)){
      load_address(f,tmp,o,POINTER);
      load_obj(f,r,cam(IMM_IND,tmp,balign(o),0L),typ,tmp);
    }else{
      load_address(f,r,o,POINTER);
    }
  }else{
    if(o->am||(o->flags&(DREFOBJ|REG))==REG){
      if(r!=o->reg)
	move_reg(f,o->reg,r);
    }else{
      if(o->flags&DREFOBJ) ierror(0);
      load_obj(f,r,o,typ,tmp);
    }
  }
}


static void store_reg(FILE *f,int r,struct obj *o,int typ)
/*  Generates code to store register r into memory object o.            */
{
  int adr,t6,t7;
  if((o->flags&(REG|DREFOBJ))==REG) ierror(0);
  if(r==t1){ t6=t2;t7=t3;}
  else if(r==t2) {t6=t1;t7=t3;}
  else {t6=t1;t7=t2;}
  if((typ&NQ)>SHORT){
    if((o->flags&VAR)&&(o->v->storage_class==EXTERN||o->v->storage_class==STATIC)){
      load_address(f,t6,o,POINTER);
      fprintf(f,"\tst%c\t%s,0(%s)\n",x_t[typ&NQ],regnames[r],regnames[t6]);
      return;
    }
    fprintf(f,"\tst%c\t%s,",x_t[typ&NQ],regnames[r]);
    probj2(f,o,typ);fprintf(f,"\n");
    return;
  }else{
    adr=0;
    if(o->am&&o->am->offset==0) 
      adr=o->am->base;
    else if(zleqto(o->val.vlong,l2zl(0L)))
      adr=o->reg;
    if(!adr){
      adr=t7; load_address(f,adr,o,POINTER);
    } 
    if((typ&NQ)==SHORT){
      fprintf(f,"\tbic\t%s,2,%s\n",regnames[adr],regnames[t4]);
      fprintf(f,"\tldl\t%s,0(%s)\n",regnames[t6],regnames[t4]);
      fprintf(f,"\tand\t%s,2,%s\n",regnames[adr],regnames[t5]);
      fprintf(f,"\tmskwl\t%s,%s,%s\n",regnames[t6],regnames[t5],regnames[t6]);
      fprintf(f,"\tinswl\t%s,%s,%s\n",regnames[r],regnames[t5],regnames[t7]);
      fprintf(f,"\tbis\t%s,%s,%s\n",regnames[t6],regnames[t7],regnames[t6]);
      fprintf(f,"\tstl\t%s,0(%s)\n",regnames[t6],regnames[t4]);
      return;
    }else{
      fprintf(f,"\tldq_u\t%s,0(%s)\n",regnames[t4],regnames[adr]);
      fprintf(f,"\tmskbl\t%s,%s,%s\n",regnames[t4],regnames[adr],regnames[t4]);
      fprintf(f,"\tinsbl\t%s,%s,%s\n",regnames[r],regnames[adr],regnames[t5]);
      fprintf(f,"\tbis\t%s,%s,%s\n",regnames[t4],regnames[t5],regnames[t4]);
      fprintf(f,"\tstq_u\t%s,0(%s)\n",regnames[t4],regnames[adr]);
      return;
    }
  }   
}

static void extend(FILE *f,int r)
{
  if(!r) return;
  if(st[r]==ESGN) return;
  fprintf(f,"\taddl\t%s,%s,%s\n",regnames[r],regnames[r31],regnames[r]);
  st[r]=ESGN;
}
static void uextend(FILE *f,int r)
{
  if(!r) return;
  if(st[r]==EUNS) return;
  fprintf(f,"\tzapnot\t%s,15,%s\n",regnames[r],regnames[r]);
  st[r]=EUNS;
}

static struct IC *do_refs(FILE *,struct IC *);
static void pr(FILE *,struct IC *);
static void function_top(FILE *,struct Var *,long,long);
static void function_bottom(FILE *f,struct Var *,long,long);

#define isreg(x) ((p->x.flags&(REG|DREFOBJ))==REG)

static int q1reg,q2reg,zreg;

static char *ccs[]={"eq","ne","lt","ge","le","gt",""};
static char *logicals[]={"bis","xor","and"};
static char *arithmetics[]={"sll","srl","add","sub","mul","div","rem"};

static struct IC *do_refs(FILE *f,struct IC *p)
/*  Does some pre-processing like fetching operands from memory to      */
/*  registers etc.                                                      */
{
  int typ=p->typf,typ1,reg,c=p->code;
  
  if(c==CONVCHAR) typ=CHAR;
  if(c==CONVUCHAR) typ=UNSIGNED|CHAR;
  if(c==CONVSHORT) typ=SHORT;
  if(c==CONVUSHORT) typ=UNSIGNED|SHORT;
  if(c==CONVINT) typ=INT;
  if(c==CONVUINT) typ=UNSIGNED|INT;
  if(c==CONVLONG) typ=LONG;
  if(c==CONVULONG) typ=UNSIGNED|LONG;
  if(c==CONVFLOAT) typ=FLOAT;
  if(c==CONVDOUBLE) typ=DOUBLE;
  if(c==CONVPOINTER) typ=UNSIGNED|LONG;
  
  q1reg=q2reg=zreg=0;
  if(p->q1.flags&REG) q1reg=p->q1.reg;
  if(p->q2.flags&REG) q2reg=p->q2.reg;
  if((p->z.flags&(REG|DREFOBJ))==REG) zreg=p->z.reg;
  
 
  if((p->q1.flags&(KONST|DREFOBJ))==KONST){
    eval_const(&p->q1.val,typ);
    if(zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))&&zdeqto(vdouble,d2zd(0.0))){
      if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) q1reg=f31; else q1reg=r31;
    }else{
      if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f1; else reg=t1;
      if(c==ASSIGN&&zreg) reg=zreg;
      if(c==SETRETURN&&p->z.reg) reg=p->z.reg;
      if((c==MOD||c==DIV)&&(typ&NQ)<=LONG&&!regs[25]) reg=25; /* Linux-div */
      if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE||c==DIV||c==SUB||c==ASSIGN||c==PUSH||c==SETRETURN||c==LSHIFT||c==RSHIFT||c==COMPARE){
	load_reg(f,reg,&p->q1,typ,t1);
	q1reg=reg;
      }else{
	if((typ&NQ)<=LONG){
	  if(!zulleq(vlong,l2zl(255L))||!zlleq(vlong,l2zl(255))||zlleq(vlong,l2zl(-1L))){
	    load_reg(f,reg,&p->q2,typ,t1);
	    q1reg=reg;
	  }	
	}
      }
    }
  }else if(c!=ADDRESS){
    if(p->q1.flags&&!q1reg){
      if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f1; else reg=t1;
      if((c==ASSIGN||(c>=CONVCHAR&&c<=CONVULONG))&&zreg&&(reg-r31)*(zreg-r31)>0) reg=zreg;
      if(c==SETRETURN&&p->z.reg) reg=p->z.reg;
      if((p->q1.flags&DREFOBJ)||c==ADDI2P||c==SUBIFP) {typ1=POINTER;reg=t1;} else typ1=typ;
      if(c==CALL) reg=vp;
      if((c==MOD||c==DIV)&&(typ1&NQ)<=LONG&&!regs[25]) reg=25; /* Linux-div */
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
      if((c==ASSIGN||(c>=CONVCHAR&&c<=CONVULONG))&&zreg&&regok(zreg,typ,0)) reg=zreg;
      if(c==SETRETURN&&p->z.reg) reg=p->z.reg;
      if((c==MOD||c==DIV)&&(typ&NQ)<=LONG&&!regs[25]) reg=25; /* Linux-div */
      if(c==ADDI2P||c==SUBIFP) 
	load_reg(f,reg,cam(IMM_IND,q1reg,-1,0),POINTER,t1);
      else
	load_reg(f,reg,cam(IMM_IND,q1reg,-1,0),typ,t1);
      q1reg=reg;
    }
  }
  if((p->q2.flags&(KONST|DREFOBJ))==KONST){
    eval_const(&p->q2.val,typ);
    if(zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))&&zdeqto(vdouble,d2zd(0.0))){
      if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) q2reg=f31; else q2reg=r31;
    }else{
      if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) reg=f2; else reg=t2;
      if((c==MOD||c==DIV)&&(typ&NQ)<=LONG&&!regs[26]) reg=26; /* Linux-div */
      if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE){
	load_reg(f,reg,&p->q2,typ,t2);
	q2reg=reg;
      }else{
	if((typ&NQ)<=LONG){	
	  if(!zulleq(vlong,l2zl(255L))||!zlleq(vlong,l2zl(255L))||zlleq(vlong,l2zl(-1L))){
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
      if((c==MOD||c==DIV)&&(typ1&NQ)<=LONG&&!regs[26]) reg=26; /* Linux-div */
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
      if((c==MOD||c==DIV)&&(typ&NQ)<=LONG&&!regs[26]) reg=26; /* Linux-div */
      load_reg(f,reg,cam(IMM_IND,q2reg,-1,0),typ,t2);
      q2reg=reg;
    }
  }
  if(p->z.flags&&!isreg(z)){
    typ=p->typf;
    if((typ&NQ)==FLOAT||(typ&NQ)==DOUBLE) zreg=f3; else zreg=t3;
    if((c==MOD||c==DIV)&&(typ&NQ)<=LONG&&!regs[28]) zreg=28; /* Linux-div */
  }
  if(q1reg){ p->q1.flags=REG; p->q1.reg=q1reg;}
  if(q2reg){ p->q2.flags=REG; p->q2.reg=q2reg;}
  return(p);
}
static long pof2(zulong x)
/*  Yields log2(x)+1 oder 0. */
{
    zulong p;int ln=1;
    p=ul2zul(1L);
    while(ln<=64&&zulleq(p,x)){
        if(zuleqto(x,p)) return(ln);
        ln++;p=zuladd(p,p);
    }
    return(0);
}
static void pr(FILE *f,struct IC *p)
/*  Writes the destination register to the real destination if necessary.   */
{
  int typ=p->typf;
  if(p->z.flags){
    if(p->code==ADDRESS||p->code==ADDI2P||p->code==SUBIFP) typ=POINTER;
    if(!isreg(z)){
      if(p->z.flags&DREFOBJ){
	if(p->z.flags&REG){
	  store_reg(f,zreg,cam(IMM_IND,p->z.reg,-1,0),typ);
	}else{
	  int r,m;
	  if(t1==zreg) r=t2; else r=t1;
	  m=p->z.flags;
	  p->z.flags&=~DREFOBJ;
	  load_reg(f,r,&p->z,POINTER,r);
	  p->z.flags=m;
	  store_reg(f,zreg,cam(IMM_IND,r,balign(&p->z),0),typ);
	}
      }else{
	store_reg(f,zreg,&p->z,typ);
      }
    }else{
      if(p->z.reg!=zreg)
	move_reg(f,zreg,p->z.reg);
    }
  }
}

static void probj2(FILE *f,struct obj *p,int t)
/*  Prints an object.                               */
{
  if(p->am){
    if(p->am->flags==REG_IND) ierror(0);
    if(p->am->flags==IMM_IND) fprintf(f,"%ld(%s)",p->am->offset,regnames[p->am->base]);
    return;
  }
  if(p->flags&DREFOBJ) fprintf(f,"(");
  if(p->flags&VAR) {
    if(p->v->storage_class==AUTO||p->v->storage_class==REGISTER){
      if(p->flags&REG){
	fprintf(f,"%s",regnames[p->reg]);
      }else{
	long of=zl2l(p->v->offset);
	if(of>=0)
	  of+=frameoffset+zl2l(p->val.vlong);
	else
	  of=-of-zl2l(maxalign)+framesize+zl2l(p->val.vlong);
	if(of>32767) ierror(0);
	fprintf(f,"%ld(%s)",of,regnames[sp]);
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
  if((p->flags&REG)&&!(p->flags&VAR)) fprintf(f,"%s",regnames[p->reg]);
  if(p->flags&KONST){
    printval(f,&p->val,t&NU,0);
  }
  if(p->flags&DREFOBJ) fprintf(f,")");
}
static void function_top(FILE *f,struct Var *v,long offset,long maxpushed)
/*  Generates function top.                             */
{
  int i;
   fprintf(f,"\t.set\tnoat\n"); 
  if(section!=CODE){fprintf(f,codename);section=CODE;}
  if(v->storage_class==EXTERN) fprintf(f,"\t.global\t%s%s\n",idprefix,v->identifier);
  fprintf(f,"\t.ent\t%s%s\n",idprefix,v->identifier);
  fprintf(f,"%s%s:\n",idprefix,v->identifier);
  if(function_calls) fprintf(f,"\tldgp\t%s,0(%s)\n",regnames[gp],regnames[vp]);
  fprintf(f,"%s%s..ng:\n",idprefix,v->identifier);
  framesize=offset+maxpushed;
  if(function_calls) framesize+=8; /* lr */
  for(i=1;i<=MAXR;i++)
    if(regused[i]&&!regscratch[i]&&!regsa[i]) 
      framesize+=8;
  framesize=((framesize+16-1)/16)*16;
  if(framesize>32767) ierror(0);
  if(framesize) fprintf(f,"\tlda\t%s,%ld(%s)\n",regnames[sp],-framesize,regnames[sp]);
  fprintf(f,"\t.frame\t%s,%ld,%s,0\n",regnames[sp],framesize,regnames[lr]);
  frameoffset=maxpushed;
  if(function_calls){
    fprintf(f,"\tstq\t%s,%ld(%s)\n",regnames[lr],frameoffset,regnames[sp]);
    frameoffset+=8;
  }
  for(i=1;i<=MAXR;i++){
    if(regused[i]&&!regscratch[i]&&!regsa[i]){
      if(i<=32){
	fprintf(f,"\tstq\t%s,%ld(%s)\n",regnames[i],frameoffset,regnames[sp]);
      }else{
	fprintf(f,"\tstt\t%s,%ld(%s)\n",regnames[i],frameoffset,regnames[sp]);
      }
      frameoffset+=8;
    }
  }
  fprintf(f,"\t.mask\t0x4000000,%ld\n",-framesize);
  fprintf(f,"\t.prologue\t%c\n",function_calls?'1':'0');
}
static void function_bottom(FILE *f,struct Var *v,long offset,long maxpushed)
/*  Generates function bottom.                          */
{
  int i;
  frameoffset=maxpushed;
  if(function_calls){
    fprintf(f,"\tldq\t%s,%ld(%s)\n",regnames[lr],frameoffset,regnames[sp]);
    frameoffset+=8;
  }
  for(i=1;i<=MAXR;i++){
    if(regused[i]&&!regscratch[i]&&!regsa[i]){
      if(i<=32){
	fprintf(f,"\tldq\t%s,%ld(%s)\n",regnames[i],frameoffset,regnames[sp]);
      }else{
	fprintf(f,"\tldt\t%s,%ld(%s)\n",regnames[i],frameoffset,regnames[sp]);
      }
      frameoffset+=8;
    }
  }  
  if(framesize) fprintf(f,"\tlda\t%s,%ld(%s)\n",regnames[sp],framesize,regnames[sp]);
  fprintf(f,"\tret\t%s,(%s),1\n",regnames[r31],regnames[lr]);
  fprintf(f,"\t.end\t%s%s\n",idprefix,v->identifier);
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
/*  Liefert die unteren 3 Bits des Objekts. -1 wenn unklar. */
{
  int sc;
  if(o->flags&DREFOBJ) return -1;
  if(o->am) return o->am->align;
  if(!(o->flags&VAR)) ierror(0);
  sc=o->v->storage_class;
  if(sc==EXTERN||sc==STATIC){
    /* Alle statischen Daten werden vom cg auf 64bit alignt. */
    return zl2l(zland(o->val.vlong,l2zl(7L)));
  }
  if(sc==AUTO||sc==REGISTER){
    zlong of=o->v->offset;
    if(!zlleq(l2zl(0L),of))
      of=zlsub(l2zl(0L),zladd(of,maxalign));
    return zl2l(zland(zladd(of,o->val.vlong),l2zl(7L)));
  }
  ierror(0);
}

/* Routinen fuer Debug-Informationen mit stabs. */

static int debug_blabel,debug_elabel;
static int debug_type(FILE *,struct Typ *);
static void debug_init(FILE *,struct Var *);
static void debug_exit(FILE *,struct Var *);
static void debug_var(FILE *,struct obj *);
static void debug_IC(FILE *,struct IC *);
static void debug_cleanup(FILE *);

static int debug_type(FILE *f,struct Typ *t)
     /* Liefert Typindex.  */
{
  return t->flags&NU;
}
static void debug_init(FILE *f,struct Var *v)
     /* Debug-Infos. Wird am Anfang von gen_code aufgerufen. */
{
  static int did_header;
  if(!did_header){
    fprintf(f,"\t.stabs \"\",100,0,0,%stext0\n",labprefix);
    fprintf(f,"\t.stabs \"%s\",100,0,0,%stext0\n",errfname,labprefix);
    fprintf(f,"\t.text\n%stext0:\n",labprefix);section=CODE;
    fprintf(f,"\t.stabs\t\"char:t%d=r1;-128;127;\",128,0,0,0\n",CHAR);
    fprintf(f,"\t.stabs\t\"short:t%d=r1;-32768;32767;\",128,0,0,0\n",SHORT);
    fprintf(f,"\t.stabs\t\"int:t%d=r1;-2147483648;2147483647;\",128,0,0,0\n",INT);
    fprintf(f,"\t.stabs\t\"long:t%d=r1;001000000000000000000000;000777777777777777777777;\",128,0,0,0\n",LONG);
    fprintf(f,"\t.stabs\t\"unsigned char:t%d=r1;0;255;\",128,0,0,0\n",UNSIGNED|CHAR);
    fprintf(f,"\t.stabs\t\"unsigned short:t%d=r1;0;65535;\",128,0,0,0\n",UNSIGNED|SHORT);
    fprintf(f,"\t.stabs\t\"unsigned int:t%d=r1;0;-1;\",128,0,0,0\n",UNSIGNED|INT);
    fprintf(f,"\t.stabs\t\"unsigned long:t%d=r1;0;-1;\",128,0,0,0\n",UNSIGNED|LONG);
    fprintf(f,"\t.stabs\t\"float:t%d=r1;4;0;\",128,0,0,0\n",FLOAT);
    fprintf(f,"\t.stabs\t\"double:t%d=r1;8;0;\",128,0,0,0\n",DOUBLE);
    fprintf(f,"\t.stabs\t\"void:t%d=%d;8;0;\",128,0,0,0\n",VOID,VOID);
    did_header=1;
  }
  fprintf(f,"\t.stabs\t\"%s:F%d\",36,0,0,%s%s\n",v->identifier,debug_type(f,v->vtyp->next),idprefix,v->identifier);
  debug_blabel=++label;debug_elabel=++label;
}
static void debug_exit(FILE *f,struct Var *v)
     /* Debug-Infos. Wird am Ende von gen_code aufgerufen. */
{
  struct IC *p;
  fprintf(f,"\t.stabn\t192,0,0,%s%d\n",labprefix,debug_blabel);
  fprintf(f,"\t.stabn\t224,0,0,%s%d\n",labprefix,debug_elabel);
  for(p=first_ic;p;p=p->next){
    if(p->q1.flags&VAR) debug_var(f,&p->q1);
    if(p->q2.flags&VAR) debug_var(f,&p->q2);
    if(p->z.flags&VAR) debug_var(f,&p->z);
  }
}
static void debug_var(FILE *f,struct obj *o)
     /* Debug-Infos fuer eine Variable ausgeben. */
{
  struct Var *v=o->v; int td;
  if(!*v->identifier) return;
  td=debug_type(f,v->vtyp);
  if(td==FUNKT) return;
  if(o->flags&REG){
    fprintf(f,"\t.stabs\t\"%s:r%d\",0x40,0,0,%d\n",v->identifier,td,o->reg-1);
    return;
  }
  if(v->storage_class==AUTO||v->storage_class==REGISTER){
    long of=zl2l(v->offset);
    if(!zlleq(l2zl(0L),v->offset)){
      of=-of-zl2l(maxalign)+framesize;
      fprintf(f,"#\toffset %ld:\n",of);
      fprintf(f,"\t.stabs\t\"%s:p%d\",0x80,0,0,%ld\n",v->identifier,td,of-framesize);
    }else{
      of+=frameoffset;
      fprintf(f,"#\toffset %ld:\n",of);
      fprintf(f,"\t.stabs\t\"%s:%d\",0x80,0,0,%ld\n",v->identifier,td,of-framesize);
    }
    return;
  }
  ierror(td);
}
static void debug_IC(FILE *f,struct IC *p)
     /* Debug-Infos. Wird fuer jedes IC aufgerufen. */
{
  static int lastline;int lab;
  printf("%d",p->line);pric2(stdout,p);
  if(!p->prev)
    fprintf(f,"%s%d:\n",labprefix,debug_blabel);
/*   if(p->q1.flags&VAR) debug_var(f,&p->q1); */
/*   if(p->q2.flags&VAR) debug_var(f,&p->q2); */
/*   if(p->z.flags&VAR) debug_var(f,&p->z); */
  if(p->line&&p->line!=lastline){
    lab=++label;lastline=p->line;
    fprintf(f,"%s%d:\n",labprefix,lab);
    fprintf(f,"\t.stabn\t68,0,%d,%s%d\n",lastline,labprefix,lab);
  }
  if(!p->next) fprintf(f,"%s%d:\n",labprefix,debug_elabel);
}
static void debug_cleanup(FILE *f)
     /* Debug-Infos. Wird in cleanup_cg aufgerufen. */
{
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
  maxalign=l2zl(8L);
  char_bit=l2zl(8L);
  for(i=0;i<16;i++){
    sizetab[i]=l2zl(msizetab[i]);
    align[i]=l2zl(malign[i]);
  }
  for(i= 1;i<=32;i++) {regsize[i]=l2zl(8L);regtype[i]=&ltyp;}
  for(i=33;i<=64;i++) {regsize[i]=l2zl(8L);regtype[i]=&ltyp;}
  
  /*  Use multiple ccs.   */
  multiple_ccs=0; /* not yet */
  
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
  t_min[LONG]=zllshift(l2zl(-1L),l2zl(63L));
  t_max[CHAR]=ul2zul(127UL);
  t_max[SHORT]=ul2zul(32767UL);
  t_max[INT]=ul2zul(2147483647UL);
  t_max[LONG]=zulsub(zulrshift(ul2zul(1UL),ul2zul(63UL)),ul2zul(1UL));
  t_max[UNSIGNED|CHAR]=ul2zul(255UL);
  t_max[UNSIGNED|SHORT]=ul2zul(65535UL);
  t_max[UNSIGNED|INT]=ul2zul(4294967295UL);
  t_max[UNSIGNED|LONG]=zulkompl(ul2zul(0UL));
  /*  Reserve a few registers for use by the code-generator.      */
  /*  This is not optimal but simple.                             */
  regsa[t1]=regsa[t2]=regsa[t3]=regsa[t4]=regsa[t5]=1;
  regsa[f1]=regsa[f2]=regsa[f3]=1;
  regsa[sp]=regsa[gp]=regsa[sd]=1;
  regsa[lr]=regsa[r31]=regsa[f31]=1;
  regscratch[t1]=regscratch[t2]=regscratch[t3]=regscratch[t4]=regscratch[t5]=0;
  regscratch[f1]=regscratch[f2]=regscratch[f3]=0;
  regscratch[sp]=regscratch[gp]=regscratch[sd]=0;
  regscratch[lr]=regscratch[r31]=regscratch[f31]=0;
  /* reserve at - noch aendern */
  /* regsa[29]=1;regscratch[29]=0; */
  /* Debug stabs? */
  stabs=(g_flags[5]&USEDFLAG);
  return(1);
}

int freturn(struct Typ *t)
/*  Returns the register in which variables of type t are returned. */
/*  If the value cannot be returned in a register returns 0.        */
/*  A pointer MUST be returned in a register. The code-generator    */
/*  has to simulate a pseudo register if necessary.                 */
{
  if((t->flags&NQ)==FLOAT||(t->flags&NQ)==DOUBLE) return(33);
  if((t->flags&NQ)==STRUCT||(t->flags&NQ)==UNION) return(0);
  if(zlleq(szof(t),l2zl(8L))) return(1); else return(0);
}

int regok(int r,int t,int mode)
/*  Returns 0 if register r cannot store variables of   */
/*  type t. If t==POINTER and mode!=0 then it returns   */
/*  non-zero only if the register can store a pointer   */
/*  and dereference a pointer to mode.                  */
{
  if(r==0) return(0);
  t&=NQ;
  if(t==0) return(0);
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
  if(zleqto(sizetab[tp],sizetab[op])&&op!=FLOAT&&tp!=FLOAT&&op!=DOUBLE&&tp!=DOUBLE) return(0);  
  return(1);
}

void gen_ds(FILE *f,zlong size,struct Typ *t)
/*  This function has to create <size> bytes of storage */
/*  initialized with zero.                              */
{
  if(newobj){
   size=zlmult(zldiv(zladd(size,l2zl(7L)),l2zl(8L)),l2zl(8L));
    printzl(f,size);
  }else{
    fprintf(f,"\t.zero\t");
    printzl(f,size);
  }
  fprintf(f,"\n");
  newobj=0;
}

void gen_align(FILE *f,zlong align)
/*  This function has to make sure the next data is     */
/*  aligned to multiples of <align> bytes.              */
{
  fprintf(f,"\t.align\t3\n");
}

void gen_var_head(FILE *f,struct Var *v)
/*  This function has to create the head of a variable  */
/*  definition, i.e. the label and information for      */
/*  linkage etc.                                        */
{
  int constflag;
  fprintf(f,"\t.align\t3\n");
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
      fprintf(f,"0x%02x%02x%02x%02x",ip[3],ip[2],ip[1],ip[0]);
      if((t&NQ)==DOUBLE){
	fprintf(f,",0x%02x%02x%02x%02x",ip[7],ip[6],ip[5],ip[4]);
      }
    }else{
      printval(f,&p->val,t&NU,0);
    }
  }else{
    if(p->tree->o.am) ierror(9);
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
  int addbuf,c,t,cmpreg,cmpflag,wasnoreg,i,varargs=0,fixargs;struct IC *m;
  long pushed,maxpushed;
  if(DEBUG&1) printf("gen_code()\n");
  if(stabs) debug_init(f,v);
  for(c=1;c<=MAXR;c++){regs[c]=regsa[c];st[c]=ESGN;}
  /* We do a pass over the code to retrieve some info and prepare certain optimizations */
  addbuf=0;maxpushed=0;
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
    if((t&NQ)<=LONG&&(m->q2.flags&KONST)&&(c==MULT||c==DIV||(c==MOD&&(t&UNSIGNED)))){ 
      eval_const(&m->q2.val,t); 
      i=pof2(vulong); 
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
    /* Need one stack slot for transfrring between integer and floating-point registers. */
    t&=NQ;
    if(c==CONVDOUBLE&&t<=LONG&&addbuf<8) addbuf=8;
    if(c==CONVFLOAT&&t<=LONG&&addbuf<8) addbuf=8;
/*     if(t==DOUBLE&&c>=CONVCHAR&&c<=CONVLONG&&addbuf<8) addbuf=8; */
/*     if(t==FLOAT&&c>=CONVCHAR&&c<=CONVLONG&&addbuf<8) addbuf=8;  */
    if(t==DOUBLE&&c!=CONVFLOAT&&addbuf<8) addbuf=8;
    if(t==FLOAT&&c!=CONVDOUBLE&&addbuf<8) addbuf=8;  
    /* May need one stack slot for inline memcpy. */
/*     if((c==ASSIGN||c==PUSH)&&t>=POINTER&&addbuf<8) addbuf=8; */
    /* Need additional stack slots for passing function arguments. */
    if(c==CALL&&maxpushed<zl2l(m->q2.val.vlong)) maxpushed=zl2l(m->q2.val.vlong);
    if(c==CALL&&(m->q1.flags&VAR)&&!strcmp(m->q1.v->identifier,"__va_start")) varargs=1;
    /* Need up to 4 stack slots for calling div/mod-functions. */
    if((c==DIV||c==MOD)&&(p->typf&NQ)<=LONG&&addbuf<32){
      if(regs[25]||regs[26]||regs[28]||regs[29]) addbuf=32;
    }
  }
  /* Noch nicht ok. */
  if(varargs){
    fixargs=0;
    for(i=0;i<v->vtyp->exact->count;i++){
      c=(*v->vtyp->exact->sl)[i].styp->flags&NQ;
      if(c<=POINTER) fixargs++;
    }
    if(fixargs<6) addbuf+=(6-fixargs)*16;
  }
  function_top(f,v,zl2l(offset+addbuf),maxpushed);
  if(varargs){
    for(i=fixargs+1;i<=6;i++){
      fprintf(f,"\tstq\t%s,%d(%s)\n",regnames[16+i],framesize-(7-i)*16,regnames[sp]);
      fprintf(f,"\tstt\t%s,%d(%s)\n",regnames[48+i],framesize-(7-i)*16+8,regnames[sp]);
    }
  }
  pushed=0;
  for(;p;pr(f,p),p=p->next){
    if(DEBUG) pric2(stdout,p);
    if(stabs) debug_IC(f,p);
    c=p->code;t=p->typf;
    if(c==NOP) continue;
    if(c==ALLOCREG) {regs[p->q1.reg]=1;continue;}
    if(c==FREEREG) {regs[p->q1.reg]=0;continue;}
    if(c==LABEL||(c>=BEQ&&c<=BRA)){
      int i;
      for(i=1;i<=32;i++) 
	if(regs[i]&&!regsa[i]) extend(f,i);
    }
    if(c==LABEL) {fprintf(f,"%s%d:\n",labprefix,t);continue;}
    if(c==BRA) {fprintf(f,"\tbr\t%s%d\n",labprefix,t);continue;}
    if(c>=BEQ&&c<BRA){
      if(cmpflag){
	fprintf(f,"\t%sb%s\t%s,%s%d\n",cmpreg<=32?"":"f",cmpflag<0?"eq":"ne",regnames[cmpreg],labprefix,t);
      }else{
	fprintf(f,"\t%sb%s\t%s,%s%d\n",cmpreg<=32?"":"f",ccs[c-BEQ],regnames[cmpreg],labprefix,t);
      }
      continue;
    }
    if(c==MOVETOREG){
      load_reg(f,p->z.reg,&p->q1,p->z.reg<=32?LONG:DOUBLE,0);
      p->z.flags=0;
      continue;
    }
    if(c==MOVEFROMREG){
      store_reg(f,p->q1.reg,&p->z,p->q1.reg<=32?LONG:DOUBLE);
      p->z.flags=0;
      continue;
    }
    if((t&NQ)==POINTER) t=(t-POINTER+LONG)|UNSIGNED;
    if((c==ASSIGN||c==PUSH)&&((t&NQ)>POINTER||((t&NQ)==CHAR&&zl2l(p->q2.val.vlong)!=1))){
      zlong size; struct obj loops;
      int salign,dalign,cl,a1,a2,mr,ms;
      size=p->q2.val.vlong;
      salign=balign(&p->q1);
      if(c==PUSH) dalign=0; else dalign=balign(&p->z);
      if(salign>=0&&dalign>=0&&(salign&3)==(dalign&3)){
	a1=t1; a2=t2;
      }else{
	if(!helpl) helpl=++label;
	cl=0;mr=0;ms=0;
	for(i=1;i<=32;i++){
	  if(i!=17&&i!=18&&i!=19&&regused[i]&&!regscratch[i]&&!regs[i]) mr=i;
	  if(regs[i]&&regscratch[i]) ms=1;
	}
	if(mr==0) mr=t3;
	if(regs[17]||regs[18]||regs[19]||function_calls==0) ms=1;
	if(ms) fprintf(f,"\tlda\t%s,%s%d\n",regnames[mr],labprefix,helpl);
	if(regs[17]) fprintf(f,"\tstq\t%s,%d(%s)\n",regnames[17],8*cl++,regnames[mr]);
	if(regs[18]) fprintf(f,"\tstq\t%s,%d(%s)\n",regnames[18],8*cl++,regnames[mr]);
	if(regs[19]) fprintf(f,"\tstq\t%s,%d(%s)\n",regnames[19],8*cl++,regnames[mr]);
	a1=18;a2=17;
	if(p->z.am&&p->z.am->base==18) ierror(0);
	if(!p->z.am&&(p->z.flags&REG)&&p->z.reg==18) ierror(0);
      }
      if(p->q1.flags&DREFOBJ){
	p->q1.flags&=~DREFOBJ;
	load_reg(f,a1,&p->q1,POINTER,a1);
	p->q1.flags|=DREFOBJ;
      }else{
	load_address(f,a1,&p->q1,POINTER);
      }
      if(p->z.flags&DREFOBJ){
	p->z.flags&=~DREFOBJ;
	load_reg(f,a2,&p->z,POINTER,a2);
	p->z.flags|=DREFOBJ;
      }else{
	if(c==PUSH){
	  fprintf(f,"\tlda\t%s,%ld(%s)\n",regnames[a2],pushed,regnames[sp]);
	  pushed+=zl2l(p->q2.val.vlong);
	}else{
	  load_address(f,a2,&p->z,POINTER);
	}
      }
      if(salign>=0&&dalign>=0&&(salign&3)==(dalign&3)){
	int do64,m,s;
	if((salign&7)==(dalign&7)) 
	  {do64=1;m=8;s='q';}
	else
	  {do64=0;m=4;s='l';salign&=3;}
	if(salign&7){
	  fprintf(f,"\tld%c\t%s,%d(%s)\n",s,regnames[t3],-salign,regnames[t1]);
	  fprintf(f,"\tld%c\t%s,%d(%s)\n",s,regnames[t4],-salign,regnames[t2]);
	  cl=(1<<salign)-1;
	  if(!zlleq(l2zl(m-salign),size)) cl+=((1<<(m-salign-zl2l(size)))-1)<<(salign+zl2l(size));
	  fprintf(f,"\tzap\t%s,%d,%s\n",regnames[t3],cl,regnames[t3]);
	  fprintf(f,"\tzapnot\t%s,%d,%s\n",regnames[t4],cl,regnames[t4]);
	  fprintf(f,"\tbis\t%s,%s,%s\n",regnames[t3],regnames[t4],regnames[t3]);
	  fprintf(f,"\tst%c\t%s,%d(%s)\n",s,regnames[t3],-salign,regnames[t2]);
	  size=zlsub(size,zl2l(m-salign));
	  salign=m-salign;
	}else 
	  salign=0;
	loops.val.vlong=zldiv(size,l2zl(8*m));
	if(zlleq(l2zl(2L),loops.val.vlong)){
	  loops.flags=KONST;
	  loops.am=0;
	  load_reg(f,t3,&loops,LONG,t3);
	  cl=++label;
	  fprintf(f,"\t.align\t4\n%s%d:\n",labprefix,cl);
	}
	if(!zleqto(loops.val.vlong,l2zl(0))){
	  for(i=0;i<8;i+=2){
	    fprintf(f,"\tld%c\t%s,%d(%s)\n",s,regnames[t4],salign+i*m,regnames[t1]);
	    fprintf(f,"\tld%c\t%s,%d(%s)\n",s,regnames[t5],salign+i*m+m,regnames[t1]);
	    fprintf(f,"\tst%c\t%s,%d(%s)\n",s,regnames[t4],salign+i*m,regnames[t2]);
	    fprintf(f,"\tst%c\t%s,%d(%s)\n",s,regnames[t5],salign+i*m+m,regnames[t2]);
	  }
	}
	if(zlleq(l2zl(2L),loops.val.vlong)){
	  fprintf(f,"\taddq\t%s,%d,%s\n",regnames[t1],8*m,regnames[t1]);
	  fprintf(f,"\taddq\t%s,%d,%s\n",regnames[t2],8*m,regnames[t2]);
	  fprintf(f,"\tsubq\t%s,1,%s\n",regnames[t3],regnames[t3]);
	  fprintf(f,"\tbne\t%s,%s%d\n",regnames[t3],labprefix,cl);
	}else{
	  if(!zleqto(loops.val.vlong,l2zl(0L)))
	    salign+=8*m;
	}
	size=zland(size,l2zl(8*m-1));
	for(i=0;i<(zl2l(size)/m/2)*2;i+=2){
	  fprintf(f,"\tld%c\t%s,%d(%s)\n",s,regnames[t4],salign+i*m,regnames[t1]);
	  fprintf(f,"\tld%c\t%s,%d(%s)\n",s,regnames[t5],salign+i*m+m,regnames[t1]);
	  fprintf(f,"\tst%c\t%s,%d(%s)\n",s,regnames[t4],salign+i*m,regnames[t2]);
	  fprintf(f,"\tst%c\t%s,%d(%s)\n",s,regnames[t5],salign+i*m+m,regnames[t2]);	  
	}
	size=zland(size,l2zl(2*m-1));
	if(zl2l(size)>=m){
	  fprintf(f,"\tld%c\t%s,%d(%s)\n",s,regnames[t4],salign+i*m,regnames[t1]);
	  fprintf(f,"\tst%c\t%s,%d(%s)\n",s,regnames[t4],salign+i*m,regnames[t2]);
	  size=zlsub(size,l2zl(m));i++;
	}
	if(zl2l(size)>0){
	  if(zl2l(size)==4){
	    fprintf(f,"\tldl\t%s,%d(%s)\n",regnames[t4],salign+i*m,regnames[t1]);
	    fprintf(f,"\tstl\t%s,%d(%s)\n",regnames[t4],salign+i*m,regnames[t2]);
	  }else{
	    fprintf(f,"\tld%c\t%s,%d(%s)\n",s,regnames[t4],salign+i*m,regnames[t1]);
	    fprintf(f,"\tld%c\t%s,%d(%s)\n",s,regnames[t5],salign+i*m,regnames[t2]);
	    cl=(1<<zl2l(size))-1;	  
	    fprintf(f,"\tzapnot\t%s,%d,%s\n",regnames[t4],cl,regnames[t4]);
	    fprintf(f,"\tzap\t%s,%d,%s\n",regnames[t5],cl,regnames[t5]);
	    fprintf(f,"\tbis\t%s,%s,%s\n",regnames[t4],regnames[t5],regnames[t4]);
	    fprintf(f,"\tst%c\t%s,%d(%s)\n",s,regnames[t4],salign+i*m,regnames[t2]);
	  }
	}
	p->z.flags=0; /* to prevent pr() from... */
	continue;
      } 
      for(i=1;i<=32;i++){
	if(i!=17&&i!=18&&i!=19&&regs[i]&&regscratch[i])
	  fprintf(f,"\tstq\t%s,%d(%s)\n",regnames[i],8*cl++,regnames[mr]);
      }
      if(function_calls==0) fprintf(f,"\tstq\t%s,%d(%s)\n",regnames[lr],8*cl++,regnames[mr]);
      if(cl>helps) helps=cl;
      loops.val.vlong=size;
      loops.flags=KONST;
      loops.am=0;
      load_reg(f,19,&loops,LONG,19);
      fprintf(f,"\t.global\t%smemcpy\n",idprefix);
      fprintf(f,"\tjsr\t%s,%smemcpy\n",regnames[lr],idprefix);
      fprintf(f,"\tldgp\t%s,0(%s)\n",regnames[gp],regnames[lr]);
      cl=0;
      if(ms&&mr==t3) fprintf(f,"\tlda\t%s,%s%d\n",regnames[mr],labprefix,helpl);
      if(regs[17]) fprintf(f,"\tldq\t%s,%d(%s)\n",regnames[17],8*cl++,regnames[mr]);
      if(regs[18]) fprintf(f,"\tldq\t%s,%d(%s)\n",regnames[18],8*cl++,regnames[mr]);
      if(regs[19]) fprintf(f,"\tldq\t%s,%d(%s)\n",regnames[19],8*cl++,regnames[mr]);
      for(i=1;i<=32;i++){
	if(i!=17&&i!=18&&i!=19&&regs[i]&&regscratch[i])
	  fprintf(f,"\tldq\t%s,%d(%s)\n",regnames[i],8*cl++,regnames[mr]);
      }
      if(function_calls==0) fprintf(f,"\tldq\t%s,%d(%s)\n",regnames[lr],8*cl++,regnames[mr]);     
      p->z.flags=0;
      continue;
    }
    if(isreg(q1)) wasnoreg=1; else wasnoreg=0;
    p=do_refs(f,p);
    c=p->code;
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
      if((to&NQ)<=LONG&&(t&NQ)<=LONG){
	if((to&NQ)>=(t&NQ)){
	  if((t&NQ)<INT){
	    if(to&UNSIGNED){
	      fprintf(f,"\tzapnot\t%s,%d,%s\n",regnames[q1reg],(t&NQ)==CHAR?1:3,regnames[zreg]);
	      st[zreg]=EUNS;continue;
	    }else{
	      fprintf(f,"\tsext%c\t%s,%s\n",(t&NQ)==CHAR?'b':'w',regnames[q1reg],regnames[zreg]);
	      st[zreg]=ESGN;continue;
	    }
	  }
	  zreg=q1reg;
	}else if((t&NQ)==LONG){
	  if(to&UNSIGNED)
	    uextend(f,q1reg);
	  else
	    extend(f,q1reg);
	  st[q1reg]=ESGN;
	  zreg=q1reg;
	}else
	  zreg=q1reg;
	continue;
      }
      if(to==FLOAT||to==DOUBLE){
	st[zreg]=ESGN;
	if((t&NQ)==FLOAT||(t&NQ)==DOUBLE){
	  fprintf(f,"\tcvt%c%c\t%s,%s\n",x_t[to&NQ],x_t[t&NQ],regnames[q1reg],regnames[zreg]);
	  continue;
	}
/* 	if(t&UNSIGNED) ierror(0); */
	fprintf(f,"\tcvttqc\t%s,%s\n",regnames[q1reg],regnames[f3]);
/* 	fprintf(f,"\tftoit\t%s,%s\n",regnames[q1reg],regnames[zreg]);  */
/* 	fprintf(f,"\t.long\t%ld\n",(0x1cl<<26)+(0x70l<<5)+(31l<<16)+((long)(q1reg-33)<<21)+zreg-1); */
	fprintf(f,"\tstt\t%s,%ld(%s)\n",regnames[f3],framesize-addbuf,regnames[sp]);
	fprintf(f,"\tldq\t%s,%ld(%s)\n",regnames[zreg],framesize-addbuf,regnames[sp]);
	st[zreg]=ESGN;
	continue;
      }
      if((t&NQ)==FLOAT||(t&NQ)==DOUBLE){
/* 	if(to&UNSIGNED) ierror(0); */
/* 	fprintf(f,"\titoft\t%s,%s\n",regnames[q1reg],regnames[zreg]);  */
	fprintf(f,"\tstq\t%s,%ld(%s)\n",regnames[q1reg],framesize-addbuf,regnames[sp]);
	fprintf(f,"\tldt\t%s,%ld(%s)\n",regnames[zreg],framesize-addbuf,regnames[sp]);
	fprintf(f,"\tcvtq%c\t%s,%s\n",x_t[t&15],regnames[zreg],regnames[zreg]);

	continue;
      }
    }
    if(c==KOMPLEMENT){      
      fprintf(f,"\tornot\t%s,%s,%s\n",regnames[r31],regnames[q1reg],regnames[zreg]);
      if((t&NQ)==INT) st[zreg]=0; else st[zreg]=ESGN;
      continue;
    }
    if(c==SETRETURN){
      if(p->z.reg){
	if(zreg==0) load_reg(f,p->z.reg,&p->q1,t,t3);
	extend(f,p->z.reg);
      }else
	ierror(0);
      continue;
    }
    if(c==GETRETURN){
      if(p->q1.reg){
	zreg=p->q1.reg;
	st[zreg]=ESGN;
      }else
	p->z.flags=0;
      continue;
    }
    if(c==CALL){
      int reg;
      if((p->q1.flags&VAR)&&p->q1.v->fi&&p->q1.v->fi->inline_asm){
	fprintf(f,"%s\n",p->q1.v->fi->inline_asm);
	pushed-=zl2l(p->q2.val.vlong);
	continue;
      }
      if((p->q1.flags&VAR)&&p->q1.v->storage_class==EXTERN&&!(g_flags[6]&USEDFLAG)){
	char *s=p->q1.v->identifier;
	if(!strcmp("abs",s)||!strcmp("labs",s)){
	  fprintf(f,"\tsubq\t%s,%s,%s\n",regnames[r31],regnames[17],regnames[1]);
	  fprintf(f,"\tcmovge\t%s,%s,%s\n",regnames[17],regnames[17],regnames[1]);
	  continue;
	}
	if(!strcmp("fabs",s)){
	  fprintf(f,"\tfabs\t%s,%s\n",regnames[17],regnames[1]);
	  continue;
	}
	if(!strcmp("__va_fixargs",s)){
	  fprintf(f,"\tlda\t%s,%d(%s)\n",regnames[1],fixargs,regnames[r31]);
	  continue;
	}
	if(!strcmp("__va_start",s)){
	  fprintf(f,"\tlda\t%s,%d(%s)\n",regnames[1],framesize-(6-fixargs)*16,regnames[sp]);
	  continue;
	}
      }
      for(reg=17;reg<=22;reg++)
	extend(f,reg);
      if(q1reg){
	if(q1reg!=vp) move_reg(f,q1reg,vp);
	fprintf(f,"\tjsr\t%s,(%s),0\n",regnames[lr],regnames[vp]);
      }else{
	fprintf(f,"\tjsr\t%s,",regnames[lr]);
	probj2(f,&p->q1,t);fprintf(f,"\n");
      }
      fprintf(f,"\tldgp\t%s,0(%s)\n",regnames[gp],regnames[lr]);
      pushed-=zl2l(p->q2.val.vlong);
      continue;
    }
    if(c==ASSIGN||c==PUSH){
      if(t==0) ierror(0);
      if(q1reg){
	if(c==PUSH){
	  extend(f,q1reg);
	  if((t&NQ)==FLOAT){
	    fprintf(f,"\tcvtst\t%s,%s\n",regnames[q1reg],regnames[f1]);
	    q1reg=f1;
	  }
	  fprintf(f,"\tst%c\t%s,%ld(%s)\n",q1reg<=32?'q':'t',regnames[q1reg],pushed,regnames[sp]);
	  pushed+=8;
	  continue;
	}
	if(c==ASSIGN) zreg=q1reg;
	continue;
      }else ierror(0);
    }
    if(c==ADDRESS){
      load_address(f,zreg,&p->q1,POINTER);
      continue;
    }
    if(c==MINUS){
      if((t&NQ)<=LONG)
	fprintf(f,"\tsub%c\t%s,%s,%s\n",x_t[t&NQ],regnames[r31],regnames[q1reg],regnames[zreg]);
      else
	fprintf(f,"\tsub%c\t%s,%s,%s\n",x_t[t&NQ],regnames[f31],regnames[q1reg],regnames[zreg]);
      st[zreg]=ESGN;
      continue;
    }
    if(c==TEST){
      if(st[q1reg]==0) extend(f,q1reg);
      cmpreg=q1reg;
      cmpflag=0;
      continue;
    }
    if(c==COMPARE){
      struct IC *br=p->next;
      while(1){
	if(br->code>=BEQ&&br->code<BRA) break;
	if(br->code!=FREEREG) ierror(0);
	br=br->next;
      }      
      if((t&NQ)==FLOAT||(t&NQ)==DOUBLE) cmpreg=f3; else cmpreg=t3;
      if(br->code==BEQ||br->code==BNE){
	if((t&NU)==(UNSIGNED|INT)){
	  if(st[q1reg]==ESGN) extend(f,q2reg);
	  else if(st[q2reg]==ESGN) extend(f,q1reg);
	  else if(st[q1reg]==EUNS) uextend(f,q2reg);
	  else if(st[q2reg]==EUNS) uextend(f,q1reg);
	  else {extend(f,q1reg);extend(f,q2reg);}
	}
	if((t&NU)==INT){extend(f,q1reg);extend(f,q2reg);}	  
	if((t&NQ)==FLOAT||(t&NQ)==DOUBLE) fprintf(f,"\tsub%c\t%s,",x_t[t&NQ],regnames[q1reg]);
	  else fprintf(f,"\tsub%c\t%s,",x_t[t&NQ],regnames[q1reg]);
	probj2(f,&p->q2,t);fprintf(f,",%s\n",regnames[cmpreg]);
	cmpflag=0;st[cmpreg]=ESGN;
      }else{
	char *s="";
	if(t&UNSIGNED) s="u";
	if((t&NU)==(UNSIGNED|INT)){uextend(f,q1reg);uextend(f,q2reg);}
	if((t&NU)==INT){extend(f,q1reg);extend(f,q2reg);}
	if((t&NQ)==FLOAT||(t&NQ)==DOUBLE) s="t";
	if(br->code==BLT||br->code==BGE){
	  fprintf(f,"\tcmp%slt\t%s,",s,regnames[q1reg]);
	  probj2(f,&p->q2,t);fprintf(f,",%s\n",regnames[cmpreg]);
	  if(br->code==BGE) cmpflag=-1; else cmpflag=1;
	}else{
	  fprintf(f,"\tcmp%sle\t%s,",s,regnames[q1reg]);
	  probj2(f,&p->q2,t);fprintf(f,",%s\n",regnames[cmpreg]);
	  if(br->code==BGT) cmpflag=-1; else cmpflag=1;
	}
      }
      continue;
    }
    if(c>=OR&&c<=AND){
      fprintf(f,"\t%s\t%s,",logicals[c-OR],regnames[q1reg]);
      probj2(f,&p->q2,t);fprintf(f,",%s\n",regnames[zreg]);
      /* hier ist mehr moeglich */
      if((t&NQ)==INT) st[zreg]=0; else st[zreg]=ESGN;
      continue;
    }
    if(c>=LSHIFT&&c<=MOD){
      int xt;
      if(c==LSHIFT&&(p->q2.flags&KONST)){
	eval_const(&p->q2.val,t);
	if(zuleqto(vulong,ul2zul(1UL))){
	  fprintf(f,"\tadd%c\t%s,%s,%s\n",x_t[t&NQ],regnames[q1reg],regnames[q1reg],regnames[zreg]);
	  st[zreg]=ESGN;continue;
	}
	if(zuleqto(vulong,ul2zul(2UL))){
	  fprintf(f,"\ts4add%c\t%s,0,%s\n",x_t[t&NQ],regnames[q1reg],regnames[zreg]);
	  st[zreg]=ESGN;continue;
	}
	if(zuleqto(vulong,ul2zul(3UL))){
	  fprintf(f,"\ts8add%c\t%s,0,%s\n",x_t[t&NQ],regnames[q1reg],regnames[zreg]);
	  st[zreg]=ESGN;continue;
	}
      }
      if(c==RSHIFT||c==LSHIFT){
	if(c==RSHIFT){
	  if(t&UNSIGNED){
	    if((t&NQ)<LONG) uextend(f,q1reg);
	    fprintf(f,"\tsrl\t");
	  }else{
	    extend(f,q1reg);
	    fprintf(f,"\tsra\t");
	  }
	  st[zreg]=st[q1reg];
	}else{
	  fprintf(f,"\tsll\t");
	  if((t&NQ)<=INT) st[zreg]=0; else st[zreg]=ESGN;
	}
	fprintf(f,"%s,",regnames[q1reg]);
	probj2(f,&p->q2,t);fprintf(f,",%s\n",regnames[zreg]);
	continue;
      }
      if((c==DIV||c==MOD)&&(t&NQ)<=LONG){
	/* Linux-Routinen aufrufen. q1=$24 q2=$25 z=$27 $28 scratch */
	if(!q1reg) ierror(0);
	if(q1reg!=25){
	  if(regs[25]) fprintf(f,"\tstq\t%s,%ld(%s)\n",regnames[25],framesize-addbuf,regnames[sp]);
	  move_reg(f,q1reg,25);
	}
	if(q2reg!=26&&regs[26]) fprintf(f,"\tstq\t%s,%ld(%s)\n",regnames[26],framesize-addbuf+8,regnames[sp]);
	if(q2reg==25) 
	  fprintf(f,"\tldq\t%s,%ld(%s)\n",regnames[26],framesize-addbuf,regnames[sp]);
	else
	  load_reg(f,26,&p->q2,t,26);
	if(zreg!=28&&regs[28]) fprintf(f,"\tstq\t%s,%ld(%s)\n",regnames[28],framesize-addbuf+16,regnames[sp]);
	if(regs[29]) fprintf(f,"\tstq\t%s,%ld(%s)\n",regnames[29],framesize-addbuf+24,regnames[sp]);
	fprintf(f,"\t%sq%s\t%s,%s,%s\n",arithmetics[c-LSHIFT],(t&UNSIGNED)?"u":"",regnames[25],regnames[26],regnames[28]);
	if(zreg!=28) move_reg(f,28,zreg);
	if(q1reg!=25&&regs[25]) fprintf(f,"\tldq\t%s,%ld(%s)\n",regnames[25],framesize-addbuf,regnames[sp]);
	if(q2reg!=26&&regs[26]) fprintf(f,"\tldq\t%s,%ld(%s)\n",regnames[26],framesize-addbuf+8,regnames[sp]);
	if(zreg!=28&&regs[28]) fprintf(f,"\tldq\t%s,%ld(%s)\n",regnames[28],framesize-addbuf+16,regnames[sp]);
	if(regs[29]) fprintf(f,"\tldq\t%s,%ld(%s)\n",regnames[29],framesize-addbuf+24,regnames[sp]);
	/* Was fuer st[zreg]? */
	continue;
      }
      xt=x_t[t&NQ];
      if((t&NQ)<INT) xt='l';
      fprintf(f,"\t%s%c\t%s,",arithmetics[c-LSHIFT],xt,regnames[q1reg]);
      probj2(f,&p->q2,t);fprintf(f,",%s\n",regnames[zreg]);
      st[zreg]=ESGN;
      continue;
    }
    if(c==SUBPFP){
      fprintf(f,"\tsubq\t%s,%s,%s\n",regnames[q1reg],regnames[q2reg],regnames[zreg]);
      st[zreg]=ESGN;continue;
    }
    if(c==ADDI2P||c==SUBIFP){
      if(t&UNSIGNED){
	if((t&NQ)<LONG) uextend(f,q2reg); 
      }else
	extend(f,q2reg);
      if(c==ADDI2P) fprintf(f,"\taddq\t%s,",regnames[q1reg]); 
      else fprintf(f,"\tsubq\t%s,",regnames[q1reg]);
      probj2(f,&p->q2,t);
      fprintf(f,",%s\n",regnames[zreg]);
      st[zreg]=ESGN;continue;
    }
    ierror(0);
  }  
  function_bottom(f,v,zl2l(offset+addbuf),maxpushed);
  if(stabs) debug_exit(f,v);
}

int shortcut(int code,int typ)
{
  return(0);
}

int reg_parm(struct reg_handle *m, struct Typ *t)
{
  int f;
  f=t->flags&NQ;
  if(f<=LONG||f==POINTER){
    if(m->nextr>=6) return(0);
    return(17+m->nextr++);
  }
  if(f==FLOAT||f==DOUBLE){
    if(m->nextr>=6) return(0);
    return(49+m->nextr++);
  }
  return(0);
}
void cleanup_cg(FILE *f)
{
  struct fpconstlist *p;
  unsigned char *ip;
  if(f&&stabs) debug_cleanup(f);
  while(p=firstfpc){
    if(f){
      int t=p->typ&NQ;
      if(section!=CODE){fprintf(f,codename);section=CODE;}
      fprintf(f,"\t.align\t3\n%s%d:\n\t",labprefix,p->label);
      if(t==FLOAT||t==DOUBLE){
	ip=(unsigned char *)&p->val.vdouble;
	fprintf(f,"\t.long\t0x%02x%02x%02x%02x",ip[3],ip[2],ip[1],ip[0]);
	if((p->typ&NQ)==DOUBLE){
	  fprintf(f,",0x%02x%02x%02x%02x",ip[7],ip[6],ip[5],ip[4]);
	}
      }else{
	fprintf(f,"\t.quad\t",dct[t]);
	printval(f,&p->val,p->typ,0);
      }
      fprintf(f,"\n");
    }
    firstfpc=p->next;
    free(p);
  }
  if(f&&helps) fprintf(f,"\t.lcomm\t%s%d,%d\n",labprefix,helpl,helps*8);
}


