/*  Code generator for Intel 80386 or higher.			*/

#include "vbc.h"

static char FILE_[]=__FILE__;

/*  Public data that MUST be there.				*/

/*  Commandline-flags the code-generator accepts		*/
#define FLAG_CPU	    g_flags[0]
#define FLAG_FPU	    g_flags[1]
#define FLAG_NODELPOP	    g_flags[2]
#define FLAG_CONSTINDATA    g_flags[3]
#define FLAG_MERGECONST     g_flags[4]
#define FLAG_ELF	    g_flags[5]
int g_flags[MAXGF]={VALFLAG,VALFLAG,0,0,
		    0,0};
char *g_flags_name[MAXGF]={"cpu","fpu","no-delayed-popping","const-in-data",
			   "merge-constants","elf"};
union ppi g_flags_val[MAXGF];

/*  Alignment-requirements for all types in bytes.		*/
zlong align[16];

/*  Alignment that is sufficient for every object.		*/
zlong maxalign;

/*  CHAR_BIT of the target machine.				*/
zlong char_bit;

/*  Sizes of all elementary types in bytes.			*/
zlong sizetab[16];

/*  Minimum and Maximum values each type can have.		*/
/*  Must be initialized in init_cg().                           */
zlong t_min[32];
zulong t_max[32];

/*  Names of all registers.					*/
char *regnames[MAXR+1]={"noreg","%eax","%ecx","%edx","%ebx",
				"%esi","%edi","%ebp","%esp",
				"%st(0)","%st(1)","%st(2)","%st(3)",
				"%st(4)","%st(5)","%st(6)","%st(7)"};

/*  The Size of each register in bytes. 			*/
zlong regsize[MAXR+1];

/*  regsa[reg]!=0 if a certain register is allocated and should */
/*  not be used by the compiler pass.				*/
int regsa[MAXR+1];

/*  Specifies which registers may be scratched by functions.	*/
int regscratch[MAXR+1]={0,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1};


/****************************************/
/*  Some private data and functions.	*/
/****************************************/

static long malign[16]={1,1,2,2,2,2,2,2,2,2,2,2,2,2,2};
static long msizetab[16]={0,1,2,4,4,4,8,0,4,0,0,0,4,0};


#define DATA 0
#define BSS 1
#define CODE 2

static int section=-1,newobj;
static char *codename="\t.text\n",*dataname="\t.data\n",*bssname="";
static int is_const(struct Typ *);
static const int ax=1,cx=2,dx=3,bx=4,si=5,di=6,bp=7,sp=8;
static char x_t[]={'?','b','w','l','l','s','l','v','l','a','s','u','e','f','?','?'};
static int is_const(struct Typ *);
static void pr(FILE *,struct IC *);
static void function_top(FILE *,struct Var *,long);
static void function_bottom(FILE *f,struct Var *,long);

#define isreg(x) ((p->x.flags&(REG|DREFOBJ))==REG)

static long loff,stackoffset,notpopped;

static char *ccs[]={"z","nz","l","ge","le","g","mp"};
static char *ccu[]={"z","nz","b","ae","be","a","mp"};
static char *logicals[]={"or","xor","and"};
static char *arithmetics[]={"shl","shr","add","sub","imul","div","mod"};
static char *farithmetics[]={"f?","f?","fadd","fsub","fmul","fdiv","fsubr","fdivr"};
static char *dct[]={"","byte","short","long","long","long","long","long","long"};
static pushedsize,pushorder=2;
static int fst[8];
static int cxl,dil,sil;

static struct fpconstlist {
    struct fpconstlist *next;
    int label,typ;
    union atyps val;
} *firstfpc;

static int addfpconst(struct obj *o,int t)
{
    struct fpconstlist *p=firstfpc;
    t&=15;
    if(FLAG_MERGECONST&USEDFLAG){
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
static void probj2(FILE *f,struct obj *p,int t)
/*  Gibt Objekt auf Bildschirm aus			*/
{
    if((p->flags&(DREFOBJ|REG))==(DREFOBJ|REG)) fprintf(f,"(");
    if(p->flags&VARADR) fprintf(f,"$");
    if((p->flags&VAR)&&!(p->flags&REG)) {
	if(p->v->storage_class==AUTO||p->v->storage_class==REGISTER){
	    if(p->v->offset<0) fprintf(f,"%ld(%%esp)",(long)(loff-zl2l(p->v->offset)+zl2l(p->val.vlong))-stackoffset+pushedsize);
		else	       fprintf(f,"%ld(%%esp)",(long)(zl2l(p->v->offset)+zl2l(p->val.vlong)-stackoffset));
	}else{
	    if(!zleqto(l2zl(0L),p->val.vlong)){printval(f,&p->val,LONG,0);fprintf(f,"+");}
	    if(p->v->identifier==empty||(p->v->nesting>0&&p->v->storage_class!=EXTERN)){
		fprintf(f,"l%ld",zl2l(p->v->offset));
	    }else{
		fprintf(f,"%s%s",FLAG_ELF?"":"_",p->v->identifier);
	    }
	}
    }
    if(p->flags&REG){
	if(p->reg>8){
	    int i;
	    for(i=0;i<8;i++){
		if(fst[i]==p->reg)
		    fprintf(f,"%s",regnames[i+9]);
	    }
	}else{
	    fprintf(f,"%s",regnames[p->reg]);
	}
    }
    if(p->flags&KONST){
	if((t&15)==FLOAT||(t&15)==DOUBLE){
	    fprintf(f,"l%d",addfpconst(p,t));
	}else{
	    fprintf(f,"$");printval(f,&p->val,t&31,0);
	}
    }
    if((p->flags&(DREFOBJ|REG))==(DREFOBJ|REG)) fprintf(f,")");
}
static void fxch(FILE *f,int i)
{
    int m;
    fprintf(f,"\tfxch\t%s\n",regnames[i+9]);
    m=fst[0];fst[0]=fst[i];fst[i]=m;
}
static int freest(void)
{
    int i;
    for(i=0;i<8;i++){
	if(fst[i]<0) return(i);
    }
    ierror(0);
}
static void fpush(FILE *f)
{
    int i;
    if(fst[7]>0){
	i=freest();
	fxch(f,i);fxch(f,7);
    }
    for(i=7;i>0;i--)
	fst[i]=fst[i-1];
    fst[0]=-1;
}
static void fpop(void)
{
    int i;
/*    if(fst[0]>0&&regs[fst[0]]) ierror(0);*/
    for(i=0;i<7;i++)
	fst[i]=fst[i+1];
    fst[7]=-1;
}
static void fload(FILE *f,struct obj *o,int t)
{
    fprintf(f,"\tfld");
    if((o->flags&(REG|DREFOBJ))==REG) fprintf(f,"\t");
	else fprintf(f,"%c\t",x_t[t&15]);
    probj2(f,o,t);fprintf(f,"\n");
    fpush(f);
}
static void fstore(FILE *f,struct obj *o,int t)
{
    int i;
    if((o->flags&(REG|DREFOBJ))==REG){
	for(i=0;i<8;i++)
	    if(fst[i]==o->reg) fst[i]=-1;
	fst[0]=o->reg;
    }else{
	fprintf(f,"\tfstp%c\t",x_t[t&15]);probj2(f,o,t);
	fpop();fprintf(f,"\n");
    }
}
static void prfst(FILE *f,char *s)
{
    int i;
    if(DEBUG==0) return;
    fprintf(f,"*\t%s\t",s);
    for(i=0;i<8;i++){
	if(fst[i]>=0)
	    fprintf(f,"%s ",regnames[fst[i]]+3);
	else
	    fprintf(f,"--- ");
    }
    fprintf(f,"\n");
}
static void finit(void)
{
    int i;
    for(i=0;i<8;i++){
	if(regs[i+9])
	    fst[i]=i+9;
	else
	    fst[i]=-1;
    }
}
static void forder(FILE *f)
{
    int i,m,unordered;
    prfst(f,"forder");
oloop:
    unordered=0;
    for(i=0;i<8;i++){
	if(fst[i]>=0&&fst[i]!=i+9&&regs[fst[i]]){unordered=1;break;}
    }
    if(!unordered) return;
    if(fst[0]>=0&&regs[fst[0]]){
	if(fst[0]!=9){
	    fxch(f,fst[0]-9);
	    goto oloop;
	}else{
	    fxch(f,freest());
	}
    }
    for(i=1;i<8;i++){
	if(fst[i]>=0&&fst[i]!=i+9&&regs[fst[i]]&&fst[i]!=9){
	    fxch(f,i);
	    goto oloop;
	}
    }
    if(regs[9]){
	for(i=1;i<8;i++){
	    if(fst[i]==9){ fxch(f,i);return;}
	}
    }
}
static void pr(FILE *f,struct IC *p)
{
    int i;
    for(;pushorder>2;pushorder>>=1){
	for(i=1;i<=8;i++){
	    if(regs[i]&pushorder){
		fprintf(f,"\tpopl\t%s\n",regnames[i]);
		stackoffset+=4;regs[i]&=~pushorder;
	    }
	}
    }
    for(i=1;i<=8;i++)
	if(regs[i]&2) regs[i]&=~2;
}
static void function_top(FILE *f,struct Var *v,long offset)
/*  erzeugt Funktionskopf			*/
{
    int i;
    if(section!=CODE){fprintf(f,codename);section=CODE;}
    if(v->storage_class==EXTERN) fprintf(f,"\t.global\t%s%s\n",FLAG_ELF?"":"_",v->identifier);
    fprintf(f,"%s%s:\n",FLAG_ELF?"":"_",v->identifier);
    for(pushedsize=0,i=1;i<sp;i++){
	if(regused[i]&&!regscratch[i]){
	    fprintf(f,"\tpushl\t%s\n",regnames[i]);
	    pushedsize+=4;
	}
    }
    if(offset) fprintf(f,"\tsubl\t$%ld,%%esp\n",offset);
}
static void function_bottom(FILE *f,struct Var *v,long offset)
/*  erzeugt Funktionsende			*/
{
    int i;
    if(offset) fprintf(f,"\taddl\t$%ld,%%esp\n",offset);
    for(i=sp-1;i>0;i--){
	if(regused[i]&&!regscratch[i]){
	    fprintf(f,"\tpopl\t%s\n",regnames[i]);
	}
    }
    fprintf(f,"\tret\n");
}
static int is_const(struct Typ *t)
/*  Tests if a type can be placed in the code-section.	*/
{
    if(!(t->flags&(CONST|STRINGCONST))){
	do{
	    if(t->flags&(CONST|STRINGCONST)) return(1);
	    if((t->flags&15)!=ARRAY) return(0);
	    t=t->next;
	}while(1);
    }else return(1);
}
static int compare_objects(struct obj *o1,struct obj *o2)
{
    if(o1->flags==o2->flags&&o1->am==o2->am){
	if(!(o1->flags&VAR)||(o1->v==o2->v&&zleqto(o1->val.vlong,o2->val.vlong))){
	    if(!(o1->flags&REG)||o1->reg==o2->reg){
		return(1);
	    }
	}
    }
    return(0);
}
static int get_reg(FILE *f,struct IC *p)
{
    int i;
    /*	If we can use a register which was already used by the compiler */
    /*	or it is a sratch register then we can use it without problems. */
    for(i=1;i<=8;i++){
	if(!regs[i]&&(regused[i]||regscratch[i])){
	    regs[i]=2;
	    return(i);
	}
    }
    /*	Otherwise we have to save this register.			*/
    /*	We may not use a register which is used in this IC.		*/
    for(i=1;i<=8;i++){
	if(regs[i]<2
	    &&(!(p->q1.flags&REG)||p->q1.reg!=i)
	    &&(!(p->q2.flags&REG)||p->q2.reg!=i)
	    &&(!(p->z.flags&REG)||p->z.reg!=i) ){

	    fprintf(f,"\tpushl\t%s\n",regnames[i]);
	    /*	Mark register as pushed (taking care of the order). */
	    pushorder<<=1; regs[i]|=pushorder;
	    stackoffset-=4;
	    return(i);
	}
    }
    ierror(0);
}
static void move(FILE *f,struct obj *q,int qr,struct obj *z,int zr,int t)
/*  Generates code to move object q (or register qr) into object z (or  */
/*  register zr).							*/
{
    if(q&&(q->flags&(REG|DREFOBJ))==REG) qr=q->reg;
    if(z&&(z->flags&(REG|DREFOBJ))==REG) zr=z->reg;
    if(qr&&zr){
	if(qr!=zr)
	    fprintf(f,"\tmovl\t%s,%s\n",regnames[qr],regnames[zr]);
	return;
    }
    fprintf(f,"\tmov%c\t",x_t[t&15]);
    if(qr) fprintf(f,"%s",regnames[qr]); else probj2(f,q,t);
    fprintf(f,",");
    if(zr) fprintf(f,"%s",regnames[zr]); else probj2(f,z,t);
    fprintf(f,"\n");
}

/****************************************/
/*  End of private fata and functions.	*/
/****************************************/


int init_cg(void)
/*  Does necessary initializations for the code-generator. Gets called	*/
/*  once at the beginning and should return 0 in case of problems.	*/
{
    int i;
    /*	Initialize some values which cannot be statically initialized	*/
    /*	because they are stored in the target's arithmetic.             */
    maxalign=l2zl(4L);
    char_bit=l2zl(8L);
    for(i=0;i<16;i++){
	sizetab[i]=l2zl(msizetab[i]);
	align[i]=l2zl(malign[i]);
    }
    for(i=1;i<= 8;i++) regsize[i]=l2zl(4L);
    for(i=9;i<=16;i++) regsize[i]=l2zl(8L);

    /*	Initialize the min/max-settings. Note that the types of the	*/
    /*	host system may be different from the target system and you may */
    /*	only use the smallest maximum values ANSI guarantees if you	*/
    /*	want to be portable.						*/
    /*	That's the reason for the subtraction in t_min[INT]. Long could */
    /*	be unable to represent -2147483648 on the host system.		*/
    t_min[UNSIGNED|CHAR]=t_min[UNSIGNED|SHORT]=t_min[UNSIGNED|INT]=t_min[UNSIGNED|LONG]=l2zl(0L);
    t_min[CHAR]=l2zl(-128L);
    t_min[SHORT]=l2zl(-32768L);
    t_min[LONG]=zlsub(l2zl(-2147483647L),l2zl(1L));
    t_min[INT]=t_min[LONG];
    t_max[CHAR]=ul2zul(128UL);
    t_max[SHORT]=ul2zul(32767UL);
    t_max[LONG]=ul2zul(2147483647UL);
    t_max[INT]=t_max[LONG];
    t_max[UNSIGNED|CHAR]=ul2zul(255UL);
    t_max[UNSIGNED|SHORT]=ul2zul(65535UL);
    t_max[UNSIGNED|LONG]=ul2zul(4294967295UL);
    t_max[UNSIGNED|INT]=t_max[UNSIGNED|LONG];
    /*	Reserve a few registers for use by the code-generator.	    */
    /*	We only reserve the stack-pointer here. 		    */
    regsa[sp]=1;
    /*	We need at least one free slot in the flaoting point stack  */
    regsa[16]=1;regscratch[16]=0;
    return(1);
}

int freturn(struct Typ *t)
/*  Returns the register in which variables of type t are returned. */
/*  If the value cannot be returned in a register returns 0.	    */
{
    if((t->flags&15)==FLOAT||(t->flags&15)==DOUBLE) return 9;
    if((t->flags&15)<=POINTER) return 1;
    return 0;
}

int regok(int r,int t,int mode)
/*  Returns 0 if register r cannot store variables of	*/
/*  type t. If t==POINTER and mode!=0 then it returns	*/
/*  non-zero only if the register can store a pointer	*/
/*  and dereference a pointer to mode.			*/
{
    if(r==0) return(0);
    t&=15;
    if(r>8){
	if(t==FLOAT||t==DOUBLE) return(1);
	    else		return(0);
    }
    if(t==CHAR&&(r==si||r==di||r==bp)) return(0);
    if(t<=LONG) return(1);
    if(t==POINTER) return(1);
    return(0);
}

int dangerous_IC(struct IC *p)
/*  Returns zero if the IC p can be safely executed	*/
/*  without danger of exceptions or similar things.	*/
/*  vbcc may generate code in which non-dangerous ICs	*/
/*  are sometimes executed although control-flow may	*/
/*  never reach them (mainly when moving computations   */
/*  out of loops).					*/
/*  Typical ICs that generate exceptions on some	*/
/*  machines are:					*/
/*	- accesses via pointers 			*/
/*	- division/modulo				*/
/*	- overflow on signed integer/floats		*/
{
    int c=p->code;
    if((p->q1.flags&DREFOBJ)||(p->q2.flags&DREFOBJ)||(p->z.flags&DREFOBJ))
	return(0);
    if((c==DIV||c==MOD)&&!(p->q2.flags&KONST))
	return(1);
    return(0);
}

int must_convert(np p,int t)
/*  Returns zero if code for converting np to type t	*/
/*  can be omitted.					*/
/*  In this generic 32bit RISC cpu pointers and 32bit	*/
/*  integers have the same representation and can use	*/
/*  the same registers. 				*/
{
    int o=p->ntyp->flags,op=o&15,tp=t&15;
    if(tp==POINTER&&op==POINTER) return(0);
    if((t&UNSIGNED)&&(o&UNSIGNED)&&zleqto(sizetab[tp],sizetab[op])) return(0);
    if((tp==INT&&op==LONG)||(tp==LONG&&op==INT)) return(0);

    return(1);
}

void gen_ds(FILE *f,zlong size,struct Typ *t)
/*  This function has to create <size> bytes of storage */
/*  initialized with zero.				*/
{
    if(newobj) fprintf(f,"%ld\n",zl2l(size));
	else   fprintf(f,"\t.space\t%ld\n",zl2l(size));
    newobj=0;
}

void gen_align(FILE *f,zlong align)
/*  This function has to make sure the next data is	*/
/*  aligned to multiples of <align> bytes.		*/
{
    if(!zlleq(align,l2zl(1L))) fprintf(f,"\t.align\t2\n");
}

void gen_var_head(FILE *f,struct Var *v)
/*  This function has to create the head of a variable	*/
/*  definition, i.e. the label and information for	*/
/*  linkage etc.					*/
{
    int constflag;
    if(v->clist) constflag=is_const(v->vtyp);
    if(v->storage_class==STATIC){
	if((v->vtyp->flags&15)==FUNKT) return;
	if(v->clist&&(!constflag||(FLAG_CONSTINDATA&USEDFLAG))&&section!=DATA){fprintf(f,dataname);section=DATA;}
	if(v->clist&&constflag&&!(FLAG_CONSTINDATA&USEDFLAG)&&section!=CODE){fprintf(f,codename);section=CODE;}
	if(!v->clist&&section!=BSS){fprintf(f,bssname);section=BSS;}
	if(section!=BSS) fprintf(f,"\t.align\t2\nl%ld:\n",zl2l(v->offset));
	    else fprintf(f,"\t.lcomm\tl%ld,",zl2l(v->offset));
	newobj=1;
    }
    if(v->storage_class==EXTERN){
	fprintf(f,"\t.global\t%s%s\n",FLAG_ELF?"":"_",v->identifier);
	if(v->flags&(DEFINED|TENTATIVE)){
	    if(v->clist&&(!constflag||(FLAG_CONSTINDATA&USEDFLAG))&&section!=DATA){fprintf(f,dataname);section=DATA;}
	    if(v->clist&&constflag&&!(FLAG_CONSTINDATA&USEDFLAG)&&section!=CODE){fprintf(f,codename);section=CODE;}
	    if(!v->clist&&section!=BSS){fprintf(f,bssname);section=BSS;}
	    if(section!=BSS) fprintf(f,"\t.align\t2\n%s%s:\n",FLAG_ELF?"":"_",v->identifier);
		else fprintf(f,"\t.comm\t%s%s,",FLAG_ELF?"":"_",v->identifier);
	    newobj=1;
	}
    }
}

void gen_dc(FILE *f,int t,struct const_list *p)
/*  This function has to create static storage		*/
/*  initialized with const-list p.			*/
{
    fprintf(f,"\t.%s\t",dct[t&15]);
    if(!p->tree){
	if((t&15)==FLOAT||(t&15)==DOUBLE){
	/*  auch wieder nicht sehr schoen und IEEE noetig   */
	    unsigned char *ip;
	    ip=(unsigned char *)&p->val.vdouble;
	    fprintf(f,"0x%02x%02x%02x%02x",ip[3],ip[2],ip[1],ip[0]);
	    if((t&15)==DOUBLE){
		fprintf(f,",0x%02x%02x%02x%02x",ip[7],ip[6],ip[5],ip[4]);
	    }
	}else{
	    printval(f,&p->val,t&31,0);
	}
    }else{
	int m=p->tree->o.flags;
	p->tree->o.flags&=~VARADR;
	probj2(f,&p->tree->o,t&31);
	p->tree->o.flags=m;
    }
    fprintf(f,"\n");newobj=0;
}

/*  The main code-generation routine.			*/
/*  f is the stream the code should be written to.	*/
/*  p is a pointer to a doubly linked list of ICs	*/
/*  containing the function body to generate code for.	*/
/*  v is a pointer to the function.			*/
/*  offset is the size of the stackframe the function	*/
/*  needs for local variables.				*/
void gen_code(FILE *f,struct IC *p,struct Var *v,zlong offset)
{
    int c,t,lastcomp=0,reg;
    if(DEBUG&1) printf("gen_code()\n");
    for(c=1;c<=15;c++) regs[c]=regsa[c];
    regs[16]=0;
    loff=((zl2l(offset)+1)/2)*2;
    function_top(f,v,loff);
    stackoffset=notpopped=0;
    finit();
    for(;p;pr(f,p),p=p->next){
	c=p->code;t=p->typf;
	if(c==NOP) continue;
	if(c==SUBPFP) c=SUB;
	if(c==SUBIFP) c=SUB;
	if(c==ADDI2P) c=ADD;
	if(c==ALLOCREG){
	    regs[p->q1.reg]=1;
	    continue;
	}
	if(c==FREEREG){
	    if(p->q1.reg>9){
		for(c=0;c<8;c++)
		    if(fst[c]==p->q1.reg) fst[c]=-1;
	    }
	    regs[p->q1.reg]=0;
	    continue;
	}
	if(notpopped){
	    int flag=0;
	    if(c==LABEL||c==COMPARE||c==TEST||c==BRA) flag=1;
	    if(!flag&&c!=CALL&&c!=GETRETURN){
		struct IC *np=p->next;
		while(np&&np->code==FREEREG) np=np->next;
		if(np&&np->code==TEST) flag=1;
	    }
	    if(flag){
		fprintf(f,"\taddl\t$%ld,%%esp\n",notpopped);
		stackoffset+=notpopped;notpopped=0;
	    }
	}
	if(c==LABEL) {forder(f);fprintf(f,"l%d:\n",t);continue;}
	if(c>=BEQ&&c<=BRA){
	    forder(f);
	    if(lastcomp&UNSIGNED) fprintf(f,"\tj%s\tl%d\n",ccu[c-BEQ],t);
		else		  fprintf(f,"\tj%s\tl%d\n",ccs[c-BEQ],t);
	    continue;
	}
	if(c==MOVETOREG){
	    if(p->z.reg>8){
		for(c=0;c<8;c++){
		    if(fst[c]==p->z.reg) fst[c]=0;
		}
		fload(f,&p->q1,DOUBLE);
		fst[0]=p->z.reg;
		continue;
	    }
	    move(f,&p->q1,0,0,p->z.reg,LONG);
	    continue;
	}
	if(c==MOVEFROMREG){
	    if(p->q1.reg>8){
		if(fst[0]!=p->q1.reg){
		    for(c=0,reg=-1;c<8;c++){
			if(fst[c]==p->q1.reg) reg=c;
		    }
		    if(reg<0) ierror(0);
		    fxch(f,reg);
		}
		fprintf(f,"\tfstp\t");probj2(f,&p->z,DOUBLE);
		fprintf(f,"\n");fpop();
		continue;
	    }
	    move(f,0,p->q1.reg,&p->z,0,LONG);
	    continue;
	}
	if((p->q1.flags&(DREFOBJ|REG))==DREFOBJ){
	    reg=get_reg(f,p);
	    move(f,&p->q1,0,0,reg,LONG);
	    p->q1.flags|=REG;p->q1.reg=reg;
	}
	if((p->q2.flags&(DREFOBJ|REG))==DREFOBJ){
	    reg=get_reg(f,p);
	    move(f,&p->q2,0,0,reg,LONG);
	    p->q2.flags|=REG;p->q2.reg=reg;
	}
	if((p->z.flags&(DREFOBJ|REG))==DREFOBJ){
	    reg=get_reg(f,p);
	    move(f,&p->z,0,0,reg,LONG);
	    p->z.flags|=REG;p->z.reg=reg;
	}
	if(c>=CONVCHAR&&c<=CONVULONG){
	    int to;
	    if(c==CONVCHAR) to=CHAR;
	    if(c==CONVUCHAR) to=(UNSIGNED|CHAR);
	    if(c==CONVSHORT) to=SHORT;
	    if(c==CONVUSHORT) to=(UNSIGNED|SHORT);
	    if(c==CONVINT) to=INT;
	    if(c==CONVUINT) to=(UNSIGNED|INT);
	    if(c==CONVLONG) to=INT;
	    if(c==CONVULONG) to=(UNSIGNED|INT);
	    if(c==CONVFLOAT) to=FLOAT;
	    if(c==CONVDOUBLE) to=DOUBLE;
	    if(c==CONVPOINTER) to=(UNSIGNED|INT);
	    if((t&31)==LONG) t=INT;
	    if((t&31)==(UNSIGNED|LONG)||(t&31)==POINTER) t=(UNSIGNED|INT);
	    if((to&15)<=INT&&(t&15)<=INT){
		if(isreg(q1)){
		    move(f,0,p->q1.reg,&p->z,0,t);
		    continue;
		}
		if(isreg(z)) reg=p->z.reg; else reg=get_reg(f,p);
		if((to&15)<=SHORT){
		    fprintf(f,"\tmov%c%cl\t",(to&UNSIGNED)?'z':'s',x_t[to&15]);
		    probj2(f,&p->q1,to);fprintf(f,",%s\n",regnames[reg]);
		}else{
		    move(f,&p->q1,0,0,reg,to);
		}
		move(f,0,reg,&p->z,0,t);
		continue;
	    }
	    if((t&15)==FLOAT||(t&15)==DOUBLE){
		if((to&15)==FLOAT||(to&15)==DOUBLE){
		    if(isreg(q1)&&fst[0]==p->q1.reg){
			if(!isreg(z)||p->z.reg!=fst[0]){
			    fprintf(f,"\tfst%c\t",x_t[t&15]);
			    probj2(f,&p->z,t);fprintf(f,"\n");
			}
			continue;
		    }
		    fload(f,&p->q1,to);
		    fstore(f,&p->z,t);
		    continue;
		}
		if((to&15)<=SHORT){
		    if(isreg(q1)){
			reg=p->q1.reg;
			if(to&UNSIGNED){
			    fprintf(f,"\tandl\t$%ld,%s\n",(to&15)==CHAR?255L:65535L,regnames[reg]);
			}else{
			    fprintf(f,"\tc%ctl\t%s\n",x_t[to&15],regnames[reg]);
			}
		    }else{
			reg=get_reg(f,p);
			if(to&UNSIGNED){
			    fprintf(f,"\tmovz%cl\t",x_t[to&15]);
			}else{
			    fprintf(f,"\tmovs%cl\t",x_t[to&15]);
			}
			probj2(f,&p->q1,to);fprintf(f,",%s\n",regnames[reg]);
		    }
		    fprintf(f,"\tmovl\t%s,-4(%s)\n",regnames[reg],regnames[sp]);
		    fprintf(f,"\tfildl\t-4(%s)\n",regnames[sp]);
		}else{
		    if(to&UNSIGNED){
			fprintf(f,"\tpushl\t$0\n\tpushl\t");stackoffset-=4;
			probj2(f,&p->q1,to);
			fprintf(f,"\n\tfildq\t(%s)\n\taddl\t$8,%s\n",regnames[sp],regnames[sp]);
			stackoffset+=4;
		    }else{
			if(isreg(q1)){
			    fprintf(f,"\tmovl\t%s,-4(%s)\n\tfildl\t-4(%s)\n",regnames[p->q1.reg],regnames[sp],regnames[sp]);
			}else{
			    fprintf(f,"\tfildl\t");probj2(f,&p->q1,t);
			    fprintf(f,"\n");
			}
		    }
		}
		fpush(f);
		fstore(f,&p->z,t);
		continue;
	    }
	    if((to&15)==FLOAT||(to&15)==DOUBLE){
		if(isreg(q1)&&fst[0]==p->q1.reg){
		    if((t&15)==CHAR){
			if(isreg(z)) reg=p->z.reg; else reg=get_reg(f,p);
			fprintf(f,"\tfistw\t-2(%s)\n\tmovsbl\t-2(%s),%s\n",regnames[sp],regnames[sp],regnames[reg]);
			move(f,0,reg,&p->z,0,t);
		    }else{
			if(isreg(z))
			    fprintf(f,"\tfistl\t-4(%s)\n\tmov%c\t-4(%s),",regnames[sp],x_t[t&15],regnames[sp]);
			else
			    fprintf(f,"\tfist%c\t",x_t[t&15]);
			probj2(f,&p->z,t);fprintf(f,"\n");
		    }
		}else{
		    fload(f,&p->q1,to);
		    if((t&15)==CHAR){
			if(isreg(z)) reg=p->z.reg; else reg=get_reg(f,p);
			fprintf(f,"\tfistpw\t-2(%s)\n\tmovsbl\t-2(%s),%s\n",regnames[sp],regnames[sp],regnames[reg]);
			fpop(); move(f,0,reg,&p->z,0,t);
		    }else{
			if(isreg(z))
			    fprintf(f,"\tfistpl\t-4(%s)\n\tmov%c\t-4(%s),",regnames[sp],x_t[t&15],regnames[sp]);
			else
			    fprintf(f,"\tfistp%c\t",x_t[t&15]);
			probj2(f,&p->z,t);fprintf(f,"\n");fpop();
		    }
		}
		continue;
	    }
	    ierror(0);
	}
	if(c==MINUS||c==KOMPLEMENT){
	    char *s;
	    if((t&15)==FLOAT||(t&15)==DOUBLE){
		if(isreg(z)&&p->z.reg==9&&isreg(q1)&&p->q1.reg==9){
		    fprintf(f,"\tfchs\n");
		    continue;
		}
		fload(f,&p->q1,t);
		fprintf(f,"\tfchs\n");
		fprintf(f,"\tfstp%c\t",x_t[t&15]);
		probj2(f,&p->z,t);fprintf(f,"\n");
		fpop();
		continue;
	    }
	    if(c==MINUS) s="neg"; else s="not";
	    if(compare_objects(&p->q1,&p->q2)){
		fprintf(f,"\t%s%c\t",s,x_t[t&15]);
		probj2(f,&p->z,t);fprintf(f,"\n");
		continue;
	    }
	    if(isreg(z)) reg=p->z.reg; else reg=get_reg(f,p);
	    move(f,&p->q1,0,0,reg,t);
	    fprintf(f,"\t%s%c\t%s\n",s,x_t[t&15],regnames[reg]);
	    move(f,0,reg,&p->z,0,t);
	    continue;
	}
	if(c==SETRETURN){
	    if(p->z.reg){
		if(p->z.reg==9){
		    if(!isreg(q1)||fst[0]!=p->q1.reg)
			fload(f,&p->q1,t);
		}else{
		    move(f,&p->q1,0,0,p->z.reg,t);
		}
	    }
	    continue;
	}
	if(c==GETRETURN){
	    if(p->q1.reg){
		if(p->q1.reg==9){
		    if(!isreg(z)||fst[0]!=p->z.reg)
			fstore(f,&p->z,t);
		}else{
		    move(f,0,p->q1.reg,&p->z,0,t);
		}
	    }
	    continue;
	}
	if(c==CALL){
	    int reg;
	    fprintf(f,"\tcall\t");probj2(f,&p->q1,t);
	    fprintf(f,"\n");
	    if(!zleqto(l2zl(0L),p->q2.val.vlong)){
		notpopped+=zl2l(p->q2.val.vlong);
		if(!(FLAG_NODELPOP&USEDFLAG)&&stackoffset==-notpopped){
		/*  Entfernen der Parameter verzoegern	*/
		}else{
		    fprintf(f,"\taddl\t$%ld,%%esp\n",zl2l(p->q2.val.vlong));
		    stackoffset+=zl2l(p->q2.val.vlong);
		    notpopped-=zl2l(p->q2.val.vlong);
		}
	    }
	    continue;
	}
	if(c==ASSIGN||c==PUSH){
	    if((t&15)==FLOAT||(t&15)==DOUBLE){
		if(c==ASSIGN){
		    prfst(f,"fassign");
		    fload(f,&p->q1,t);
		    fstore(f,&p->z,t);
		    continue;
		}else if(isreg(q1)){
		    prfst(f,"fpush");
		    fprintf(f,"\tsubl\t$%ld,%s\n",zl2l(sizetab[t&15]),regnames[sp]);
		    stackoffset-=zl2l(sizetab[t&15]);
		    if(fst[0]==p->q1.reg){
			fprintf(f,"\tfst%c\t(%s)\n",x_t[t&15],regnames[sp]);
		    }else{
			fload(f,&p->q1,t);
			fprintf(f,"\tfstp%c\t(%s)\n",x_t[t&15],regnames[sp]);
			fpop();
		    }
		    continue;
		}
	    }
	    if((t&15)>POINTER||!zleqto(p->q2.val.vlong,sizetab[t&15])||!zlleq(p->q2.val.vlong,l2zl(4L))){
		int mdi=di,msi=si,m=0;long l;
		l=zl2l(p->q2.val.vlong);
		if(regs[cx]){m|=1;if(!cxl)cxl=++label;fprintf(f,"\tmovl\t%s,l%d\n",regnames[cx],cxl);}
		if(regs[msi]||!regused[msi]){m|=2;if(!sil)sil=++label;fprintf(f,"\tmovl\t%s,l%d\n",regnames[msi],sil);}
		if(regs[mdi]||!regused[mdi]){m|=4;if(!dil)dil=++label;fprintf(f,"\tmovl\t%s,l%d\n",regnames[mdi],dil);}
		if((p->z.flags&REG)&&p->z.reg==msi&&(p->q1.flags&REG)&&p->q1.reg==mdi){
		    msi=di;mdi=si;
		    m|=8;
		}
		if(!(p->z.flags&REG)||p->z.reg!=msi){
		    fprintf(f,"\tleal\t");probj2(f,&p->q1,t);
		    fprintf(f,",%s\n",regnames[msi]);
		}
		if(c==PUSH){
		    fprintf(f,"\tsubl\t$%ld,%s\n\tmovl\t%s,%s\n",l,regnames[sp],regnames[sp],regnames[mdi]);
		    stackoffset-=l;
		}else{
		    fprintf(f,"\tleal\t");probj2(f,&p->z,t);
		    fprintf(f,",%s\n",regnames[mdi]);
		}
		if((p->z.flags&REG)&&p->z.reg==msi){
		    fprintf(f,"\tleal\t");probj2(f,&p->q1,t);
		    fprintf(f,",%s\n",regnames[msi]);
		}
		if(m&8){
		    msi=si;mdi=di;
		    fprintf(f,"\txch\t%s,%s\n",regnames[msi],regnames[mdi]);
		}
		if((t&15)==ARRAY||(t&15)==CHAR||l<4){
		    fprintf(f,"\tmovl\t$%ld,%s\n\trep\n\tmovsb\n",l,regnames[cx]);
		}else{
		    if(l>=8)
			fprintf(f,"\tmovl\t$%ld,%s\n\trep\n",l/4,regnames[cx]);
		    fprintf(f,"\tmovsl\n");
		    if(l%2) fprintf(f,"\tmovsw\n");
		    if(l%1) fprintf(f,"\tmovsb\n");
		}
		if(m&4) fprintf(f,"\tmovl\tl%d,%s\n",dil,regnames[mdi]);
		if(m&2) fprintf(f,"\tmovl\tl%d,%s\n",sil,regnames[msi]);
		if(m&1) fprintf(f,"\tmovl\tl%d,%s\n",cxl,regnames[cx]);
		continue;
	    }
	    if(t==FLOAT) t=LONG;
	    if(c==PUSH){
		fprintf(f,"\tpush%c\t",x_t[t&15]);
		probj2(f,&p->q1,t);fprintf(f,"\n");
		stackoffset-=zl2l(p->q2.val.vlong);
		continue;
	    }
	    if(c==ASSIGN){
		if(p->q1.flags&KONST){
		    move(f,&p->q1,0,&p->z,0,t);
		    continue;
		}
		if(isreg(z)) reg=p->z.reg; else reg=get_reg(f,p);
		move(f,&p->q1,0,0,reg,t);
		move(f,0,reg,&p->z,0,t);
		continue;
	    }
	    ierror(0);
	}
	if(c==ADDRESS){
	    if(isreg(z)) reg=p->z.reg; else reg=get_reg(f,p);
	    fprintf(f,"\tleal\t");probj2(f,&p->q1,t);
	    fprintf(f,",%s\n",regnames[reg]);
	    move(f,0,reg,&p->z,0,POINTER);
	    continue;
	}
	if(c==TEST){
	    lastcomp=t;
	    if((t&15)==FLOAT||(t&15)==DOUBLE){
		if(isreg(q1)&&fst[0]==p->q1.reg){
		    fprintf(f,"\tftst\n");lastcomp|=UNSIGNED;
		    continue;
		}else{
		    p->code=c=COMPARE;
		    p->q2.flags=KONST;
		    p->q2.val.vdouble=d2zd(0.0);
		    if((t&15)==FLOAT) p->q2.val.vfloat=zd2zf(p->q2.val.vdouble);
		    /*	fall through to COMPARE */
		}
	    }else{
		fprintf(f,"\tcmp%c\t$0,",x_t[t&15]);
		probj2(f,&p->q1,t);fprintf(f,"\n");
		continue;
	    }
	}
	if(c==COMPARE){
	    lastcomp=t;
	    if(isreg(q2)||(p->q1.flags&KONST)){
		struct IC *b=p->next;
		struct obj o;
		o=p->q1;p->q1=p->q2;p->q2=o;
		while(b&&b->code==FREEREG) b=b->next;
		if(!b) ierror(0);
		if(b->code==BLT) b->code=BGT;
		if(b->code==BLE) b->code=BGE;
		if(b->code==BGT) b->code=BLT;
		if(b->code==BGE) b->code=BLE;
	    }
	    if((t&15)==FLOAT||(t&15)==DOUBLE){
		prfst(f,"fcomp");
		if(isreg(q1)&&p->q1.reg==fst[0]){
		    fprintf(f,"\tfcom%c\t",x_t[t&15]);
		    probj2(f,&p->q2,t);fprintf(f,"\n");
		}else{
		    fload(f,&p->q1,t);
		    fprintf(f,"\tfcomp%c\t",x_t[t&15]);
		    probj2(f,&p->q2,t);fprintf(f,"\n");
		    fpop();
		}
		fprintf(f,"\tfstsw\n\tsahf\n");
		lastcomp|=UNSIGNED;
		continue;
	    }
	    if(!isreg(q1)){
		if(!isreg(q2)){
		    reg=get_reg(f,p);
		    move(f,&p->q1,0,0,reg,t);
		    p->q1.flags=REG;
		    p->q1.reg=reg;
		}
	    }
	    fprintf(f,"\tcmp%c\t",x_t[t&15]);
	    probj2(f,&p->q2,t);fprintf(f,",");
	    probj2(f,&p->q1,t);fprintf(f,"\n");
	    continue;
	}
	if(c>=OR&&c<=AND){
	    if(isreg(z)) reg=p->z.reg; else reg=get_reg(f,p);
	    move(f,&p->q1,0,0,reg,t);
	    fprintf(f,"\t%s%c\t",logicals[c-OR],x_t[t&15]);
	    probj2(f,&p->q2,t);fprintf(f,",%s\n",regnames[reg]);
	    move(f,0,reg,&p->z,0,t);
	    continue;
	}
	if((t&15)==FLOAT||(t&15)==DOUBLE){
	    char s[2];
	    prfst(f,"fmath");
	    if(isreg(q2)) s[0]=0; else {s[0]=x_t[t&15];s[1]=0;}
	    if(isreg(z)&&isreg(q1)&&p->q1.reg==fst[0]&&p->z.reg==fst[0]){
		fprintf(f,"\t%s%s\t",farithmetics[c-LSHIFT],s);
		probj2(f,&p->q2,t); fprintf(f,"\n");continue;
	    }
	    fload(f,&p->q1,t);
	    fprintf(f,"\t%s%s\t",farithmetics[c-LSHIFT],s);
	    probj2(f,&p->q2,t); fprintf(f,"\n");
	    fstore(f,&p->z,t); continue;
	}
	if(c==MOD||c==DIV){
	    int m=0;
	    if(regs[ax]&&(!isreg(z)||p->z.reg!=ax)){
		fprintf(f,"\tpushl\t%s\n",regnames[ax]);
		stackoffset-=4;m|=1;
	    }
	    if(regs[dx]&&(!isreg(z)||p->z.reg!=dx)){
		fprintf(f,"\tpushl\t%s\n",regnames[dx]);
		stackoffset-=4;m|=2;
	    }
	    move(f,&p->q1,0,0,ax,t);
	    if(p->q2.flags&KONST){
		fprintf(f,"\tpush%c\t",x_t[t&15]);probj2(f,&p->q2,t);
		fprintf(f,"\n");m|=4;stackoffset-=4;
	    }
	    if(t&UNSIGNED) fprintf(f,"\tmovl\t$0,%s\n\tdivl\t",regnames[dx]);
		else	   fprintf(f,"\tcltd\n\tidivl\t");
	    if((m&4)||(isreg(q2)&&p->q2.reg==dx)){
		fprintf(f,"(%s)",regnames[sp]);
	    }else{
		probj2(f,&p->q2,t);
	    }
	    fprintf(f,"\n");
	    if(c==DIV) move(f,0,ax,&p->z,0,t);
		else   move(f,0,dx,&p->z,0,t);
	    if(m&4){ fprintf(f,"\taddl\t$%ld,%s\n",zl2l(sizetab[t&15]),regnames[sp]);stackoffset+=4;}
	    if(m&2){ fprintf(f,"\tpopl\t%s\n",regnames[dx]);stackoffset+=4;}
	    if(m&1){ fprintf(f,"\tpopl\t%s\n",regnames[ax]);stackoffset+=4;}
	    continue;
	}
	if(c>=LSHIFT&&c<=MOD){
	    if(isreg(z)) reg=p->z.reg; else reg=get_reg(f,p);
	    move(f,&p->q1,0,0,reg,t);
	    fprintf(f,"\t%s%c\t",arithmetics[c-LSHIFT],x_t[t&15]);
	    probj2(f,&p->q2,t);fprintf(f,",%s\n",regnames[reg]);
	    move(f,0,reg,&p->z,0,t);
	    continue;
	}
	ierror(0);
    }
    if(notpopped){
	fprintf(f,"\taddl\t$%ld,%%esp\n",notpopped);
	stackoffset+=notpopped;notpopped=0;
    }
    function_bottom(f,v,loff);
}

int shortcut(int code,int typ)
{
    return(0);
}

void cleanup_cg(FILE *f)
{
    struct fpconstlist *p;
    unsigned char *ip;
    while(p=firstfpc){
	if(f){
	    if(section!=CODE){fprintf(f,codename);section=CODE;}
	    fprintf(f,"l%d:\n\t.long\t",p->label);
	    ip=(unsigned char *)&p->val.vdouble;
	    fprintf(f,"0x%02x%02x%02x%02x",ip[3],ip[2],ip[1],ip[0]);
	    if((p->typ&15)==DOUBLE){
		fprintf(f,",0x%02x%02x%02x%02x",ip[7],ip[6],ip[5],ip[4]);
	    }
	    fprintf(f,"\n");
	}
	firstfpc=p->next;
	free(p);
    }
    if(f){
	if(section!=BSS){fprintf(f,bssname);section=BSS;}
	if(cxl) fprintf(f,"\t.lcomm\tl%d,4\n",cxl);
	if(sil) fprintf(f,"\t.lcomm\tl%d,4\n",sil);
	if(dil) fprintf(f,"\t.lcomm\tl%d,4\n",dil);
    }
}

