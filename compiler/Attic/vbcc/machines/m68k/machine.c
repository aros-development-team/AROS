/*  $VER: vbcc (machine.c amiga68k) V0.4    */

/*  Code generator for Motorola 680x0 CPUs. Supports 68000-68060+68881/2.   */
/*  PhxAss and the GNU assembler is supported.                              */

#include "supp.h"

static char FILE_[]=__FILE__;

/*  Public data that MUST be there.                             */

/*  Commandline-flags the code-generator accepts                */
int g_flags[MAXGF]={VALFLAG,VALFLAG,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
char *g_flags_name[MAXGF]={
    "cpu","fpu","d2scratch","noa4","sc","sd","prof","const-in-data",
    "use-framepointer","no-addressing-modes","no-delayed-popping",
    "gas","branch-opt","no-fp-return","no-mreg-return","g"};

union ppi g_flags_val[MAXGF];

/*  Alignment-requirements for all types in bytes.              */
zlong align[16];

/*  Alignment that is sufficient for every object.              */
zlong maxalign=2;

/*  CHAR_BIT of the target machine.                             */
zlong char_bit;

/*  Sizes of all elementary types in bytes.                     */
zlong sizetab[16];

/*  Minimum and Maximum values each type can have.              */
/*  Must be initialized in init_cg().                           */
zlong t_min[32];
zulong t_max[32];

/*  Names of all registers.                                     */
char *regnames[MAXR+1]={"noreg","a0","a1","a2","a3","a4","a5","a6","a7",
                               "d0","d1","d2","d3","d4","d5","d6","d7",
                        "fp0","fp1","fp2","fp3","fp4","fp5","fp6","fp7"};

/*  The Size of each register in bytes.                         */
zlong regsize[MAXR+1];

/*  regsa[reg]!=0 if a certain register is allocated and should */
/*  not be used by the compiler pass.                           */
int regsa[MAXR+1]={0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*  Specifies which registers may be scratched by functions.    */
int regscratch[MAXR+1]={0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0};


/****************************************/
/*  Some private data and functions.    */
/****************************************/

static long malign[16]={1,1,2,2,2,2,2,2,2,2,2,2,2,2,2};
static long msizetab[16]={0,1,2,4,4,4,8,0,4,0,0,0,4,0};

static char reglist[200];

#define DATA 0
#define BSS 1
#define CODE 2

static int reglabel,freglabel,section=-1;
static char *codename,*bssname,*dataname;

static void probj2(FILE *,struct obj *,int);
static struct IC *do_refs(FILE *,struct IC *);
static void pr(FILE *,struct IC *);
static int get_reg(FILE *,int,struct IC *);
static long pof2(zulong);
static void function_top(FILE *,struct Var *,long);
static void function_bottom(FILE *f,struct Var *,long);

static void saveregs(FILE *,struct IC *),restoreregsa(FILE *,struct IC *);
static void restoreregsd(FILE *,struct IC *);

static void assign(FILE *,struct IC *,struct obj *,struct obj *,int,long,int);
static int compare_objects(struct obj *o1,struct obj *o2);

static int is_const(struct Typ *t);

static char x_s[]={'0','b','w','3','l'};
static char x_t[]={'?','b','w','l','l','s','d','v','l','a','s','u','e','f','?','?'};

static char *quick[2]={"","q"};
static char *strshort[2]={"l","w"};

static char *ubranch[]={"eq","ne","cs","cc","ls","hi"};

static int pushedreg,stored_cc; /* pushedreg&2: aregsaved; 4: dreg; 8: freg */
                                /* 16: durch RESTOREREGS gepushed           */
static int pushlabel,pushflag;
static int geta4;

#define D16OFF 1024

static int newobj=0;   /*  um zu erkennen, ob neue section erlaubt ist */

static int gas,dbout;

static int isquickkonst(union atyps *,int),isquickkonst2(union atyps *,int),regavailable(int);
static void move(FILE *,struct obj *,int,struct obj *,int,int);
static void add(FILE *,struct obj *,int,struct obj *,int,int);
static void sub(FILE *,struct obj *,int,struct obj *,int,int);
static void mult(FILE *,struct obj *,int,struct obj *,int,int,int,struct IC *);

static long pof2(zulong x)
/*  Yields log2(x)+1 oder 0. */
{
    zulong p;int ln=1;
    p=ul2zul(1L);
    while(zulleq(p,x)){
        if(zuleqto(x,p)) return(ln);
        ln++;p=zuladd(p,p);
    }
    return(0);
}

#define isreg(x) ((p->x.flags&(REG|DREFOBJ))==REG)

#define PEA 1000

static int addressing(void);
static long notpopped,dontpop,stackoffset,loff;
static int offlabel,regoffset;
/*  For keeping track of condition codes.   */
static struct obj *cc_set,*cc_set_tst;
static int cc_typ,cc_typ_tst;

static int pget_reg(FILE *f,int flag,struct IC *p)
{
    int i;
    flag=1+flag*8;
    for(i=flag;i<flag+8;i++){
        if(regs[i]==1&&(!p||(i!=p->q1.reg&&i!=p->q2.reg&&i!=p->z.reg))){
            if(p){
                if((p->q1.am&&(p->q1.am->dreg==i||p->q1.am->basereg==i))
                 ||(p->q2.am&&(p->q2.am->dreg==i||p->q2.am->basereg==i))
                 ||(p->z.am&&(p->z.am->dreg==i||p->z.am->basereg==i))){
                    continue;
                }
            }
            regs[i]+=8;if(!pushlabel) {pushlabel=++label;pushflag=1;}
            fprintf(f,"\tmove.l\t%s,l%d\n",regnames[i],pushlabel);
            if(i<9) pushedreg|=2;
            else if (i<17) pushedreg|=4;
            else pushedreg|=8;
            return(i);
        }
    }
    ierror(0);
}
static int get_reg(FILE *f,int flag,struct IC *p)
/*  Gets a register: flag=0=areg, 1=dreg, 2=fpreg           */
{
    int i;
    flag=1+flag*8;
    for(i=flag;i<flag+8;i++){
        if(regs[i]==0){
            if(p){
                if((p->q1.am&&(p->q1.am->dreg==i||p->q1.am->basereg==i))
                 ||(p->q2.am&&(p->q2.am->dreg==i||p->q2.am->basereg==i))
                 ||(p->z.am&&(p->z.am->dreg==i||p->z.am->basereg==i))){
/*                    iwarning("%s used in get_reg(1)",regnames[i]);*/
                    continue;
                }
            }
            regs[i]=2;pushedreg|=1;
            if(!regused[i]&&!regscratch[i]){regused[i]=1; }
            return(i);
        }
    }
    for(i=flag;i<flag+8;i++){
        if(regs[i]==1&&(!p||(i!=p->q1.reg&&i!=p->q2.reg&&i!=p->z.reg))){
            if(p){
                if((p->q1.am&&(p->q1.am->dreg==i||p->q1.am->basereg==i))
                 ||(p->q2.am&&(p->q2.am->dreg==i||p->q2.am->basereg==i))
                 ||(p->z.am&&(p->z.am->dreg==i||p->z.am->basereg==i))){
/*                    iwarning("%s used in get_reg(2)",regnames[i]);*/
                    continue;
                }
            }
            regs[i]+=4;
            if(i<17) {fprintf(f,"\tmove.l\t%s,-(a7)\n",regnames[i]);stackoffset-=4;}
             else {fprintf(f,"\tfmove.x\t%s,-(a7)\n",regnames[i]);stackoffset-=12;}
/*            if(p->code==COMPARE) ierror("corrupt code for compare generated - sorry");*/
            if(i<9) pushedreg|=2;
            else if (i<17) pushedreg|=4;
            else pushedreg|=8;
            return(i);
        }
    }
    ierror(0);
}
static int isquickkonst(union atyps *p,int t)
/*  Returns 1 if constant is between -128 and 127.   */
{
    zlong zl;zulong zul;
    eval_const(p,t);
    if(t&UNSIGNED){
        zul=ul2zul(127UL);
        return(zulleq(vulong,zul));
    }else{
        zl=l2zl(-129L);
        if(zlleq(vlong,zl)) return(0);
        zl=l2zl(127L);
        return(zlleq(vlong,zl));
    }
}
static int isquickkonst2(union atyps *p,int t)
/*  Returns 1 if constant is between 1 and 8.   */
{
    zlong zl;zulong zul;
    eval_const(p,t);
    if(t&UNSIGNED){
        if(zuleqto(ul2zul(0UL),vulong)) return(0);
        zul=ul2zul(8UL);
        return(zulleq(vulong,zul));
    }else{
        if(zleqto(l2zl(0L),vlong)) return(0);
        zl=l2zl(-1L);
        if(zlleq(vlong,zl)) return(0);
        zl=l2zl(8L);
        return(zlleq(vlong,zl));
    }
}
static int regavailable(int art)
/*  Returns true if matching register is available.     */
{
    int i;
    art=1+art*8;
    for(i=art+1;i<art+8;i++)
        if(regs[i]==0) return(1);
    return(0);
}
static int compare_objects(struct obj *o1,struct obj *o2)
/*  Tests if two objects are equal.     */
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
static struct IC *do_refs(FILE *f,struct IC *p)
/*  Loads DREFOBJs into address registers, if necessary.    */
/*  Small constants are loaded into data registers if this  */
/*  improves code.                                          */
{
    int reg,c=p->code,t=p->typf,equal;
    if((p->q1.flags&DREFOBJ)&&(!(p->q1.flags&REG)||p->q1.reg<1||p->q1.reg>8)){
        equal=0;
        if(compare_objects(&p->q1,&p->q2)) equal|=1;
        if(compare_objects(&p->q1,&p->z)) equal|=2;
        reg=get_reg(f,0,p);
        p->q1.flags&=~DREFOBJ;
        fprintf(f,"\tmove.l\t");probj2(f,&p->q1,t);
        p->q1.flags=REG|DREFOBJ;
        p->q1.reg=reg;
        fprintf(f,",%s\n",regnames[p->q1.reg]);
        if(equal&1) p->q2=p->q1;
        if(equal&2) p->z=p->q1;
    }
    if((p->q2.flags&DREFOBJ)&&(!(p->q2.flags&REG)||p->q2.reg<1||p->q2.reg>8)){
        if(compare_objects(&p->q2,&p->z)) equal=1; else equal=0;
        reg=get_reg(f,0,p);
        p->q2.flags&=~DREFOBJ;
        fprintf(f,"\tmove.l\t");probj2(f,&p->q2,t);
        p->q2.flags=REG|DREFOBJ;
        p->q2.reg=reg;
        fprintf(f,",%s\n",regnames[p->q2.reg]);
        if(equal) p->z=p->q2;
    }
    if((p->z.flags&DREFOBJ)&&(!(p->z.flags&REG)||p->z.reg<1||p->z.reg>8)){
        reg=get_reg(f,0,p);
        p->z.flags&=~DREFOBJ;
        fprintf(f,"\tmove.l\t");probj2(f,&p->z,t);
        p->z.flags=REG|DREFOBJ;
        p->z.reg=reg;
        fprintf(f,",%s\n",regnames[p->z.reg]);
    }
    if(g_flags_val[0].l!=68040){
    /*  Don't do it on 040 because it's slower. */
        if(x_t[t&NQ]=='l'&&(t&NQ)!=FLOAT&&(c!=ASSIGN||!isreg(z))&&
           c!=MULT&&c!=DIV&&c!=MOD&&c!=LSHIFT&&c!=RSHIFT&&c!=SETRETURN&&c!=PUSH&&c!=ADDI2P&&c!=SUBIFP&&
           (!(p->z.flags&REG)||p->z.reg<9||p->z.reg>16)){
            /*  Constants into registers.    */
            if((p->q1.flags&KONST)&&isquickkonst(&p->q1.val,t)&&((c!=ADD&&c!=SUB&&c!=ADDI2P&&c!=SUBIFP)||!isquickkonst2(&p->q1.val,t))){
                eval_const(&p->q1.val,t);
                if((!zdeqto(d2zd(0.0),vdouble)||!zleqto(l2zl(0L),vlong)||!zuleqto(ul2zul(0UL),vulong))&&regavailable(1)){
                    reg=get_reg(f,1,p);
                    move(f,&p->q1,0,0,reg,t);
                    p->q1.flags=REG;p->q1.reg=reg;
                    p->q1.val.vlong=l2zl(0L);
                }
            }
            if((p->q2.flags&KONST)&&isquickkonst(&p->q2.val,t)&&((c!=ADD&&c!=SUB&&c!=ADDI2P&&c!=SUBIFP)||!isquickkonst2(&p->q2.val,t))){
                eval_const(&p->q2.val,t);
                if((!zdeqto(d2zd(0.0),vdouble)||!zleqto(l2zl(0L),vlong)||!zuleqto(ul2zul(0UL),vulong))&&regavailable(1)){
                    reg=get_reg(f,1,p);
                    move(f,&p->q2,0,0,reg,t);
                    p->q2.flags=REG;p->q2.reg=reg;
                    p->q2.val.vlong=l2zl(0L);
                }
            }
        }
    }
    return(p);
}
static void pr(FILE *f,struct IC *p)
/*  Release registers and pop them from stack if necessary. */
{
    int i,size=0;
    /*  To keep track of condition codes.   */
    if((pushedreg&12)&&(p->code==TEST||p->code==COMPARE)){
        char *fp;struct IC *branch;
        if(g_flags_val[1].l>=68000&&((p->typf&NQ)==FLOAT||(p->typf&NQ)==DOUBLE)) fp="f"; else fp="";
        branch=p;
        while(branch->code<BEQ||branch->code>=BRA) branch=branch->next;
        if((p->typf&UNSIGNED)||(p->typf&NQ)==POINTER){
            fprintf(f,"\ts%s\t-2(a7)\n",ubranch[branch->code-BEQ]);
        }else{
            fprintf(f,"\t%ss%s\t-2(a7)\n",fp,ename[branch->code]+1);
        }
        stored_cc=1;
    }
    for(i=MAXR;i>0;i--){
        if(regs[i]==2) regs[i]=0;
        if(regs[i]&8){
            regs[i]&=~8;
            fprintf(f,"\tmove.l\tl%d,%s\n",pushlabel,regnames[i]);
            if(i>=9) cc_set=0;
        }
        if(regs[i]&4){
            regs[i]&=~4;
            if(i<17) {fprintf(f,"\tmove.l\t(a7)+,%s\n",regnames[i]);stackoffset+=4;size+=4;}
             else {fprintf(f,"\tfmove.x\t(a7)+,%s\n",regnames[i]);stackoffset+=12;size+=12;}
            if(i>=9) cc_set=0;
        }
    }
    if((pushedreg&12)&&(p->code==TEST||p->code==COMPARE))
        fprintf(f,"\ttst.b\t-%d(a7)\n",size+2);
}
static void probj2(FILE *f,struct obj *p,int t)
/*  Write object.   */
{
    if(p->am){
    /*  Addressing modes.   */
        if(g_flags[9]&USEDFLAG) {ierror(0);p->am=0;return;}
        if(p->am->skal>=0){
            long l=0;
            if(p->flags&D16OFF) l=zl2l(p->val.vlong);
            fprintf(f,"(%ld,%s",p->am->dist+l,regnames[p->am->basereg]);
            if(p->am->dreg){
                fprintf(f,",%s",regnames[p->am->dreg&127]);
                if(p->am->dreg&128) fprintf(f,".w"); else fprintf(f,".l");
                if(p->am->skal) fprintf(f,"*%d",p->am->skal);
            }
            fprintf(f,")");
            return;
        }
        if((p->flags&D16OFF)&&!zleqto(l2zl(0L),p->val.vlong)) ierror(0);
        if(p->am->skal==-1){
            fprintf(f,"(%s)+",regnames[p->am->basereg]);
            return;
        }
        if(p->am->skal==-2){    /*  Noch nicht implementiert    */
            fprintf(f,"-(%s)",regnames[p->am->basereg]);
            return;
        }
    }
    if(p->flags&DREFOBJ){
        fprintf(f,"(");
        if((p->flags&D16OFF)&&!zleqto(l2zl(0L),p->val.vlong))
            {printval(f,&p->val,LONG,0);fprintf(f,",");}
    }
    if(p->flags&VARADR) fprintf(f,"#");
    if(p->flags&VAR) {
        if(p->v->storage_class==AUTO||p->v->storage_class==REGISTER){
            if(p->flags&REG){
                fprintf(f,"%s",regnames[p->reg]);
            }else{
                long os;
                os=zl2l(p->val.vlong);
                if(!(g_flags[8]&USEDFLAG)){
                    if(!zlleq(l2zl(0L),p->v->offset)) os=os+loff-zl2l(p->v->offset);
                     else              os=os+zl2l(p->v->offset);
                    fprintf(f,"(%ld+l%d,a7)",os-stackoffset,offlabel);
                }else{
                    if(!zlleq(l2zl(0L),p->v->offset)) os=os-zl2l(p->v->offset)+4;
                     else              os=os-(zl2l(p->v->offset)+zl2l(szof(p->v->vtyp)));
                    fprintf(f,"(%ld,a5)",os);
                }
            }
        }else{
            if(!zleqto(l2zl(0L),p->val.vlong)){printval(f,&p->val,LONG,0);fprintf(f,"+");}
            if(p->v->storage_class==STATIC&&(p->v->vtyp->flags&NQ)!=FUNKT){
                fprintf(f,"l%ld",zl2l(p->v->offset));
            }else{
                fprintf(f,"_%s",p->v->identifier);
            }
            if((g_flags[5]&USEDFLAG)&&!(p->flags&VARADR)&&(p->v->vtyp->flags&NQ)!=FUNKT&&((g_flags[7]&USEDFLAG)||!is_const(p->v->vtyp)) )
                fprintf(f,"(a4)");
        }
    }
    if((p->flags&REG)&&!(p->flags&VAR)) fprintf(f,"%s",regnames[p->reg]);
    if(p->flags&KONST){
        /*  This requires IEEE floats/doubles on the host compiler. */
        if(t==FLOAT||t==DOUBLE){
            unsigned char *ip=(unsigned char *)&p->val.vfloat;
            char *s;
            if(gas) s="0x"; else s="$";
            fprintf(f,"#%s%02x%02x%02x%02x",s,ip[0],ip[1],ip[2],ip[3]);
            if(t==DOUBLE){
                if(DEBUG&1) printf("doubleconst=%f\n",p->val.vdouble);
                fprintf(f,"%02x%02x%02x%02x",ip[4],ip[5],ip[6],ip[7]);
            }
        }else {fprintf(f,"#");printval(f,&p->val,t&NU,0);}
    }
    if(p->flags&DREFOBJ) fprintf(f,")");
}
static char tsh[]={'w','l'};
static int proflabel;
static void function_top(FILE *f,struct Var *v,long offset)
/*  Writes function header. */
{
    geta4=0;
    if(gas){
    }else{
        if(g_flags[NQ]&USEDFLAG) fprintf(f,"\tsymdebug\n");
        if(g_flags_val[0].l!=68000) fprintf(f,"\tmachine\t%ld\n",g_flags_val[0].l);
        if(g_flags_val[1].l>68000) fprintf(f,"\tfpu\t1\n");
        if(g_flags[4]&USEDFLAG) fprintf(f,"\tnear\tcode\n");
        if(g_flags[5]&USEDFLAG) fprintf(f,"\tnear\ta4,-2\n");
        fprintf(f,"\topt\t0\n\topt\tNQLPSM");
        if(g_flags_val[0].l!=68040) fprintf(f,"R");
        if((g_flags[12]&USEDFLAG)||(optflags&2)) fprintf(f,"BT");
        fprintf(f,"\n");
    }
    if(section!=CODE){fprintf(f,codename);section=CODE;}
    if(g_flags[6]&USEDFLAG){
        proflabel=++label;
        if(gas){
            fprintf(f,"l%d:\n\t.byte\t\"%s\",0\n",proflabel,v->identifier);
        }else{
            fprintf(f,"l%d\n\tdc.b\t\"%s\",0\n",proflabel,v->identifier);
        }
    }
    if(v->storage_class==EXTERN){
        if(gas){
            fprintf(f,"\t.global\t_%s\n",v->identifier);
        }else{
            fprintf(f,"\tpublic\t_%s\n",v->identifier);
        }
    }
    if(gas){
        fprintf(f,"\t.align\t2\n_%s:\n",v->identifier);
    }else{
        fprintf(f,"\tcnop\t0,4\n_%s\n",v->identifier);
    }
    if(g_flags[6]&USEDFLAG){
        if(gas){
            fprintf(f,"\tpea\tl%d\n\t.global\t__startprof\n\tjbsr\t__startprof\n\taddq.w\t#4,a7\n",proflabel);
        }else{
            fprintf(f,"\tpea\tl%d\n\tpublic\t__startprof\n\tjsr\t__startprof\n\taddq.w\t#4,a7\n",proflabel);
        }
    }
    offset=-((offset+4-1)/4)*4;
    loff=-offset;offlabel=++label;
    if(!(g_flags[8]&USEDFLAG)){
        if(offset<0) fprintf(f,"\tsub%s.%s\t#%ld,a7\n",quick[offset>=-8],strshort[offset>=-32768],-offset);
    }else{
        if(offset>=-32768||g_flags_val[0].l>=68020){
            fprintf(f,"\tlink.%c\ta5,#%ld\n",tsh[offset<-32768],offset);
        }else{
            fprintf(f,"\tlink.w\ta5,#-32768\n");offset+=32768;
            fprintf(f,"\tsub.%c\t#%ld,a5\n",tsh[offset<-32768],offset);
        }
    }
    if(g_flags_val[1].l>68000&&float_used){
        if(gas){
            fprintf(f,"\t.word\t0xf227,l%d\n",freglabel);
        }else{
            fprintf(f,"\tfmovem.x\tl%d,-(a7)\n",freglabel);
        }
    }
    if(gas){
        fprintf(f,"\tmovem.l\t#l%d,-(a7)\n",reglabel);
    }else{
        fprintf(f,"\tmovem.l\tl%d,-(a7)\n",reglabel);
    }
}
static void function_bottom(FILE *f,struct Var *v,long offset)
/*  Writes function footer. */
{
    int i,size=0;unsigned int pushval,popval;
    *reglist=0;
    pushval=popval=0;
    for(i=1;i<=16;i++){
        if((regused[i]&&!regscratch[i]&&!regsa[i])||(i==5&&geta4)){
            if(*reglist) strcat(reglist,"/");
            strcat(reglist,regnames[i]);
            if(i<9){  pushval|=(256>>i);popval|=(128<<i);}
              else {  pushval|=(0x8000>>(i-9));popval|=(1<<(i-9));}
            size+=4;
        }
    }
    if(gas){
        if(popval) fprintf(f,"\t.equ\tl%d,%u\n\tmovem.l\t(a7)+,#%u\n",reglabel,pushval,popval);
            else fprintf(f,"\t.equ\tl%d,0\n",reglabel);
    }else{
        if(*reglist) fprintf(f,"l%d\treg\t%s\n\tmovem.l\t(a7)+,l%d\n",reglabel,reglist,reglabel);
            else fprintf(f,"l%d\treg\n",reglabel);
    }
    *reglist=0;pushval=0xe000;popval=0xd000;
    for(i=17;i<=MAXR;i++){
        if(regused[i]&&!regscratch[i]&&!regsa[i]){
            if(*reglist) strcat(reglist,"/");
            strcat(reglist,regnames[i]);
            pushval|=(1<<(i-17));popval|=(0x80>>(i-17));
            size+=12;
        }
    }
    if(g_flags_val[1].l>68000&&float_used){
        if(gas){
            if(popval!=0xd000) fprintf(f,"\t.equ\tl%d,0x%x\n\t.word\t0xf21f,0x%x\n",freglabel,(int)pushval,(int)popval);
                else fprintf(f,"\t.equ\tl%d,0xe000\n",freglabel);
        }else{
            if(*reglist) fprintf(f,"l%d\tfreg\t%s\n\tfmovem.x\t(a7)+,l%d\n",freglabel,reglist,freglabel);
                else fprintf(f,"l%d\tfreg\n",freglabel);
        }
    }
    if(!(g_flags[8]&USEDFLAG)){
        if(loff) fprintf(f,"\tadd%s.%s\t#%ld,a7\n",quick[loff<=8],strshort[loff<32768],loff);
        if(gas){
            fprintf(f,"\t.equ\tl%d,%d\n",offlabel,size);
        }else{
            fprintf(f,"l%d\tEQU\t%d\n",offlabel,size);
        }
    }else fprintf(f,"\tunlk\ta5\n");
    if(g_flags[6]&USEDFLAG){
        if(gas){
            fprintf(f,"\tpea\tl%d\n\t.global\t__endprof\n\tjbsr\t__endprof\n\taddq.w\t#4,a7\n",proflabel);
        }else{
            fprintf(f,"\tpea\tl%d\n\tpublic\t__endprof\n\tjsr\t__endprof\n\taddq.w\t#4,a7\n",proflabel);
        }
    }
    fprintf(f,"\trts\n");
}
static void move(FILE *f,struct obj *q,int qreg,struct obj *z,int zreg,int t)
/*  erzeugt eine move Anweisung...Da sollen mal Optimierungen rein  */
{
    if(!zreg&&(z->flags&(REG|DREFOBJ))==REG) zreg=z->reg;
    if(!qreg&&(q->flags&(REG|DREFOBJ))==REG) qreg=q->reg;
    if(zreg==qreg&&zreg) return;
    if(q&&(q->flags&VARADR)&&zreg>=1&&zreg<=8){
        fprintf(f,"\tlea\t");
        q->flags&=~VARADR;probj2(f,q,t);q->flags|=VARADR;
        fprintf(f,",%s\n",regnames[zreg]);
        return;
    }
    if(zreg>=9&&zreg<=16&&q&&(q->flags&KONST)&&isquickkonst(&q->val,t)){
        fprintf(f,"\tmoveq\t");
    }else{
        if(zreg>=17||qreg>=17){
            if(qreg>=17&&zreg>=17) fprintf(f,"\tfmove.x\t");
             else fprintf(f,"\tfmove.%c\t",x_t[t&NQ]);
        }else{
            fprintf(f,"\tmove.%c\t",x_s[msizetab[t&NQ]]);
        }
    }
    if(qreg) fprintf(f,"%s",regnames[qreg]); else probj2(f,q,t);
    fprintf(f,",");
    if(zreg) fprintf(f,"%s",regnames[zreg]); else probj2(f,z,t);
    fprintf(f,"\n");
}
static void add(FILE *f,struct obj *q,int qreg,struct obj *z,int zreg,int t)
/*  erzeugt eine add Anweisung...Da sollen mal Optimierungen rein   */
{
    if(!qreg&&!q) ierror(0);
    if(!zreg&&!z) ierror(0);
    if(!zreg&&(z->flags&(REG|DREFOBJ))==REG) zreg=z->reg;
    if(!qreg&&(q->flags&KONST)&&isquickkonst2(&q->val,t)){
        fprintf(f,"\taddq.%c\t",x_t[t&NQ]);
    }else{
        /*  hier noch Abfrage, ob #c.w,ax   */
        fprintf(f,"\tadd.%c\t",x_t[t&NQ]);
    }
    if(qreg) fprintf(f,"%s",regnames[qreg]); else probj2(f,q,t);
    fprintf(f,",");
    if(zreg) fprintf(f,"%s",regnames[zreg]); else probj2(f,z,t);
    fprintf(f,"\n");
}
static void sub(FILE *f,struct obj *q,int qreg,struct obj *z,int zreg,int t)
/*  erzeugt eine sub Anweisung...Da sollen mal Optimierungen rein   */
{
    if(!zreg&&(z->flags&(REG|DREFOBJ))==REG) zreg=z->reg;
    if(q&&(q->flags&KONST)&&isquickkonst2(&q->val,t)){
        fprintf(f,"\tsubq.%c\t",x_t[t&NQ]);
    }else{
        /*  hier noch Abfrage, ob #c.w,ax   */
        fprintf(f,"\tsub.%c\t",x_t[t&NQ]);
    }
    if(qreg) fprintf(f,"%s",regnames[qreg]); else probj2(f,q,t);
    fprintf(f,",");
    if(zreg) fprintf(f,"%s",regnames[zreg]); else probj2(f,z,t);
    fprintf(f,"\n");
}
static void mult(FILE *f,struct obj *q,int qreg,struct obj *z,int zreg, int t,int c,struct IC *p)
/*  erzeugt eine mult Anweisung...Da sollen mal Optimierungen rein  */
/*  erzeugt auch div/mod etc.                                       */
{
    int modreg;
    if(!qreg&&(q->flags&(REG|DREFOBJ))==REG) qreg=q->reg;
    if(!zreg&&(z->flags&(REG|DREFOBJ))==REG) zreg=z->reg;
    if((c==MULT||c==DIV||c==MOD)&&g_flags_val[0].l<68020&&msizetab[t&NQ]==4){
        if(c==MULT){
        /*  ist das mit get_reg(.,.,0) ok? nochmal ueberdenken...   */
        /*  ...die ganze Routine am besten...                       */
        /*  ...es war nicht, deshalb ist es jetzt geaendert         */
            int dx,dy,t1,t2;
            if(zreg>=9&&zreg<=16){
                dx=zreg;
            }else{
                dx=get_reg(f,1,p);
                move(f,z,0,0,dx,t);
            }
            if(qreg>=9&&qreg<=16&&qreg!=dx){
                dy=qreg;
            }else{
                dy=get_reg(f,1,p);
                move(f,q,0,0,dy,t);
            }
            t1=get_reg(f,1,p);t2=get_reg(f,1,p);
            if(t1==dx||t2==dx||t1==dy||t2==dy) ierror(0);
            fprintf(f,"\tmove.l\t%s,%s\n",regnames[dx],regnames[t1]);
            fprintf(f,"\tmove.l\t%s,%s\n",regnames[dy],regnames[t2]);
            fprintf(f,"\tswap\t%s\n",regnames[t1]);
            fprintf(f,"\tswap\t%s\n",regnames[t2]);
            fprintf(f,"\tmulu.w\t%s,%s\n",regnames[dy],regnames[t1]);
            fprintf(f,"\tmulu.w\t%s,%s\n",regnames[dx],regnames[t2]);
            fprintf(f,"\tmulu.w\t%s,%s\n",regnames[dy],regnames[dx]);
            fprintf(f,"\tadd.w\t%s,%s\n",regnames[t2],regnames[t1]);
            fprintf(f,"\tswap\t%s\n",regnames[t1]);
            fprintf(f,"\tclr.w\t%s\n",regnames[t1]);
            fprintf(f,"\tadd.l\t%s,%s\n",regnames[t1],regnames[dx]);
            if(zreg!=dx) move(f,0,t1,z,0,t);
        }else ierror(0);
        return;
    }
    if(c==MULT){
        /*  das duerfte nur der Aesthetik dienen... */
        if(t&UNSIGNED) fprintf(f,"\tmulu.%c\t",x_t[t&NQ]); else fprintf(f,"\tmuls.%c\t",x_t[t&NQ]);
    }
    if(c==DIV){
        if(t&UNSIGNED) fprintf(f,"\tdivu.%c\t",x_t[t&NQ]); else fprintf(f,"\tdivs.%c\t",x_t[t&NQ]);
    }
    if(qreg) fprintf(f,"%s",regnames[qreg]); else probj2(f,q,t);
    fprintf(f,",");
    /*  eigentlich muss zreg!=0 sein...     */
    if(zreg) fprintf(f,"%s",regnames[zreg]); else probj2(f,z,t);
    fprintf(f,"\n");
}
static struct IC *am_freedreg[9],*am_shiftdreg[9];
static struct IC *am_dist_ic[9],*am_dreg_ic[9],*am_use[9];
/*  am_dist_ic und am_dreg_ic werden auch fuer (ax)+ benutzt    */
static long am_dist[9],am_dreg[9],am_base[9],am_inc[9],am_skal[9],am_dbase[9];
#define AMS sizeof(struct AddressingMode)

static void clear_am(int reg)
/*  loescht Werte fuer erweiterte Adressierungsarten fuer Register reg  */
{
    if(reg<0||reg>16) ierror(0);
    if(DEBUG&32) printf("clear_am(%s)\n",regnames[reg]);
    if(reg<=8){
        am_dist_ic[reg]=am_dreg_ic[reg]=am_use[reg]=0;
        am_dist[reg]=am_dreg[reg]=am_base[reg]=am_inc[reg]=0;
    }else{
        reg-=8;
        am_freedreg[reg]=am_shiftdreg[reg]=0;
        am_skal[reg]=am_dbase[reg]=0;
    }
}
static int addressing(void)
/*  Untersucht ICs auf erweiterte Addresierungsarten    */
{
    struct IC *p;int count,localused=0;
    if(DEBUG&32) printf("addressing() started\n");
    for(count=1;count<=16;count++) clear_am(count);
    for(count=0,p=first_ic;p;p=p->next){
        int c=p->code,q1reg,q2reg,zreg;
        if(p->q1.flags&REG) q1reg=p->q1.reg; else q1reg=0;
        if(p->q2.flags&REG) q2reg=p->q2.reg; else q2reg=0;
        if(p->z.flags&REG) zreg=p->z.reg; else zreg=0;
        if(c==ADDI2P) c=ADD;
        if(c==SUBIFP) c=SUB;
        if(DEBUG&32) pric2(stdout,p);
        if(!localused){
            if((p->q1.flags&(VAR|REG))==VAR&&(p->q1.v->storage_class==AUTO||p->q1.v->storage_class==REGISTER)&&p->q1.v->offset>=0)
                localused=1;
            if((p->q2.flags&(VAR|REG))==VAR&&(p->q2.v->storage_class==AUTO||p->q2.v->storage_class==REGISTER)&&p->q2.v->offset>=0)
                localused=1;
            if((p->z.flags&(VAR|REG))==VAR&&(p->z.v->storage_class==AUTO||p->z.v->storage_class==REGISTER)&&p->z.v->offset>=0)
                localused=1;
            if(DEBUG&32&&localused==1) printf("localused=1\n");
        }
        if(c==ASSIGN&&isreg(q1)&&isreg(z)&&q1reg>=1&&q1reg<=8&&zreg>=1&&zreg<=8){
        /*  fuer (ax)+  */
            int i;
            clear_am(q1reg);
            for(i=1;i<=8;i++)
                if(am_base[i]==zreg||am_base[i]==q1reg) clear_am(i);
            clear_am(zreg);am_base[zreg]=q1reg;am_dreg_ic[zreg]=p;
            if(DEBUG&32) printf("move %s,%s found\n",regnames[q1reg],regnames[zreg]);
            continue;
        }
        if(c==MULT&&g_flags_val[0].l>=68020&&(p->q2.flags&KONST)&&isreg(z)&&zreg>=9&&zreg<=16){
        /*  dx=a*const, fuer Skalierung    */
            int dreg=zreg-8;
            if(dreg<1||dreg>8) ierror(0);
            if(q1reg>=1&&q1reg<=16){
                if(isreg(q1)&&(q1reg>8||am_use[q1reg]!=p)) clear_am(q1reg);
                if((p->q1.flags&DREFOBJ)&&q1reg<=8&&am_use[q1reg]) clear_am(q1reg);
            }
            if(DEBUG&32) printf("mult x,const->dreg found\n");
            if(am_skal[dreg]) {clear_am(zreg);continue;}
            eval_const(&p->q2.val,p->typf);
            am_skal[dreg]=zl2l(vlong);
            if(am_skal[dreg]!=2&&am_skal[dreg]!=4&&am_skal[dreg]!=8)
                {clear_am(zreg);continue;}
            am_shiftdreg[dreg]=p;
            if(isreg(q1)&&q1reg>=9&&q1reg<=16) am_dbase[dreg]=q1reg; else am_dbase[dreg]=zreg;
            if(DEBUG&32) printf("is usable\n");
            continue;
        }
        if((c==ADD||c==SUB)&&(p->q2.flags&KONST)&&zreg>=1&&zreg<=8&&isreg(z)){
        /*  add ax,#const,ax->az Test auf d8/16 fehlt noch (nicht mehr) */
            long l;
            if(zreg<1||zreg>8) ierror(0);
            eval_const(&p->q2.val,p->typf);
            l=zl2l(vlong);
            if(c==SUB) l=-l;
            if(q1reg==zreg&&am_use[zreg]&&(l==1||l==2||l==4)){
                if(l==msizetab[am_use[zreg]->typf&NQ]){
                    struct IC *op=am_use[zreg];
                    struct obj *o=0;
                    if(DEBUG&32){ printf("found postincrement:\n");pric2(stdout,op);pric2(stdout,p);}
                    if((op->q1.flags&(REG|DREFOBJ))==(REG|DREFOBJ)&&op->q1.reg==zreg){
                        if(DEBUG&32) printf("q1\n");
                        o=&op->q1;
                    }
                    if((op->q2.flags&(REG|DREFOBJ))==(REG|DREFOBJ)&&op->q2.reg==zreg){
                        if(DEBUG&32) printf("q2\n");
                        if(o) continue; else o=&op->q2;
                    }
                    if((op->z.flags&(REG|DREFOBJ))==(REG|DREFOBJ)&&op->z.reg==zreg){
                        if(DEBUG&32) printf("z\n");
                        if(o) continue; else o=&op->z;
                    }
                    o->am=mymalloc(AMS);
                    o->am->basereg=zreg;
                    o->am->skal=-1;
                    o->am->dist=0;
                    o->am->dreg=0;
                    p=p->prev;
                    remove_IC(p->next);
                    clear_am(zreg);continue;
                }
            }
            clear_am(q1reg);
            if(am_dist[zreg]||am_inc[zreg]||am_use[zreg]) {clear_am(zreg);continue;} /* nur ein Offset */
            if(isreg(q1)&&q1reg==zreg&&(l==1||l==2||l==4)){
            /*  ax+=const, fuer (ax)+   */
                int i,f;
                for(f=0,i=1;i<=8;i++){
                    if(am_base[i]==zreg&&!am_dreg[i]&&!am_dist[i]){
                        if(f) ierror(0);
                        am_inc[i]=l;am_dist_ic[i]=p;f=i;
                        if(DEBUG&32) printf("inc %s found\n",regnames[i]);
                    }
                }
                if(f) continue;
            }
            am_dist[zreg]=l;
            if(DEBUG&32) printf("dist=%ld\n",am_dist[zreg]);
            if(g_flags_val[0].l<68020){
            /*  bei <68020 darf der Offset nur 16bit oder 8bit bei dreg sein */
                if((am_dreg[zreg]&&(am_dist[zreg]<-128||am_dist[zreg]>127))||am_dist[zreg]<-32768||am_dist[zreg]>32767)
                    {clear_am(zreg);continue;}
            }
            am_dist_ic[zreg]=p;
            if(am_base[zreg]){
                if(q1reg!=zreg||!isreg(q1)) {clear_am(zreg);continue;}
            }else{
                if(q1reg>=1&&q1reg<=8&&isreg(q1)) am_base[zreg]=q1reg; else am_base[zreg]=zreg;
                if(DEBUG&32) printf("%s potential base for %s\n",regnames[am_base[zreg]],regnames[zreg]);
            }
            if(DEBUG&32) printf("add #const,%s found\n",regnames[zreg]);
            continue;
        }
        if(c==ADD&&q2reg>=9&&q2reg<=16&&isreg(q2)&&zreg>=1&&zreg<=8&&isreg(z)&&(p->q1.flags&(REG|DREFOBJ))!=(REG|DREFOBJ)){
        /*  add ax,dy->az   */
            int i;
            if(zreg<1||zreg>8) ierror(0);
            for(i=1;i<=8;i++)
                if(am_dreg[i]==q2reg){ clear_am(q2reg);clear_am(i);}
            clear_am(q1reg);
            if(am_dreg[zreg]||am_inc[zreg]||am_use[zreg]) {clear_am(zreg);continue;} /* nur ein Regoffset */
            if(g_flags_val[0].l<68020&&(am_dist[zreg]<-128||am_dist[zreg]>127))
                {clear_am(zreg);continue;} /* bei <68020 nur 8bit Offset */
            am_dreg[zreg]=q2reg;
            if((p->typf&NQ)==SHORT) am_dreg[zreg]|=128; /* dx.w statt dx.l */
            am_dreg_ic[zreg]=p;
            if(am_base[zreg]){
                if(q1reg!=zreg||!isreg(q1)) {clear_am(zreg);continue;}
            }else{
                if(q1reg>=1&&q1reg<=8&&isreg(q1)) am_base[zreg]=q1reg; else am_base[zreg]=zreg;
            }
            if(DEBUG&32) printf("add %s,%s found\n",regnames[q2reg],regnames[zreg]);
            continue;
        }
        if(c==FREEREG){
        /*  wir koennen den Modus tatsaechlich benutzen */
            struct AddressingMode *am;struct IC *p1,*p2;int dreg,i;
            if(DEBUG&32) printf("freereg %s found\n",regnames[p->q1.reg]);
            if(q1reg>=9&&q1reg<=16) {am_freedreg[q1reg-8]=p;if(DEBUG&32) printf("freedreg[%d]=%lx\n",q1reg-8,(long)p);}
            if(q1reg>8) continue;
            if(DEBUG&32) printf("use=%p,base=%p,dist=%p,dreg=%p\n",(void*)am_use[q1reg],(void*)am_base[q1reg],(void*)am_dist[q1reg],(void*)am_dreg[q1reg]);
            for(i=1;i<=8;i++) if(am_base[i]==q1reg) clear_am(i);
            if(!am_use[q1reg]||!am_base[q1reg]) continue;
            if(!am_dist[q1reg]&&!am_dreg[q1reg]&&!am_inc[q1reg]) continue;
            p1=am_dist_ic[q1reg];p2=am_dreg_ic[q1reg];
            if(DEBUG&32){
                printf("could really use %s\n",regnames[q1reg]);
                if(p1) pric2(stdout,p1);
                if(p2) pric2(stdout,p2);
            }
            if(am_base[q1reg]==q1reg){
                if(p1) {p1->q2.flags=0;p1->code=ASSIGN;p1->q2.val.vlong=l2zl(4L);p1->typf=POINTER;}
                if(p2) {p2->q2.flags=0;p2->code=ASSIGN;p2->q2.val.vlong=l2zl(4L);p2->typf=POINTER;}
            }else{
                if(p1) remove_IC(p1);
                if(p2) remove_IC(p2);
            }
            dreg=(am_dreg[q1reg]&127)-8;
            am=mymalloc(AMS);
            am->skal=0;
            am->basereg=am_base[q1reg];
            am->dist=am_dist[q1reg];
            am->dreg=am_dreg[q1reg];
            if(am_inc[q1reg]) am->skal=-1;
            if(dreg>0){
                /*  bei (d,ax,dy) das freereg dy nach hinten verschieben    */
                if(dreg<1||dreg>8) ierror(0);
                if(p1=am_freedreg[dreg]){
                    if(DEBUG&32){
                        printf("freereg %s moved from %p to %p\n",regnames[dreg+8],(void*)p1,(void*)p);
                        pric2(stdout,p1);
                    }
                    if(p1->code!=FREEREG){ierror(0);printf("freereg[%d]=%p\n",dreg,(void*)p1);continue;}
                    if(!p1->next) {ierror(0);continue;}
                    if(!p1->prev) {ierror(0);continue;}
                    p1->prev->next=p1->next;
                    p1->next->prev=p1->prev;
                    p1->next=p->next;
                    p1->prev=p;
                    if(p->next) p->next->prev=p1;
                    p->next=p1;
                }
                if(am_skal[dreg]){
                /*  Skalierung bearbeiten   */
                    if(p1){
                        am->skal=am_skal[dreg];
                        am->dreg=am_dbase[dreg];
                        p1=am_shiftdreg[dreg];
                        if(DEBUG&32) pric2(stdout,p1);
                        if(am_dbase[dreg]==dreg+8){
                            p1->code=ASSIGN;p1->q2.flags=0;p1->q2.val.vlong=sizetab[p1->typf&NQ];
                        }else remove_IC(p1);
                    }
                    clear_am(dreg+8);
                }
            }
            /*  das hier duerfte unnoetig sein, da die Adressierungsart in  */
            /*  einem IC eigentlich hoechstens einmal vorkommen darf        */
            if(q1reg<0||q1reg>8) ierror(0);
            p1=am_use[q1reg];
            if(DEBUG&32) pric2(stdout,p1);
            if(p1->code==PUSH&&p1->q1.reg==q1reg&&((p1->q1.flags&(DREFOBJ|REG))==(REG))){
                p1->q1.am=mymalloc(AMS);
                memcpy(p1->q1.am,am,AMS);
                p->q1.val.vlong=l2zl(0L);
                p1->code=PEA;
                if(DEBUG&32) printf("q1 patched\n");
            }
            if(p1->q1.reg==q1reg&&((p1->q1.flags&(DREFOBJ|REG))==(DREFOBJ|REG))){
                p1->q1.am=mymalloc(AMS);
                memcpy(p1->q1.am,am,AMS);
                p1->q1.val.vlong=l2zl(0L);
                if(DEBUG&32) printf("q1 patched\n");
            }
            if(p1->q2.reg==q1reg&&((p1->q2.flags&(DREFOBJ|REG))==(DREFOBJ|REG))){
                p1->q2.am=mymalloc(AMS);
                memcpy(p1->q2.am,am,AMS);
                p1->q2.val.vlong=l2zl(0L);
                if(DEBUG&32) printf("q2 patched\n");
            }
            if(p1->z.reg==q1reg&&((p1->z.flags&(DREFOBJ|REG))==(DREFOBJ|REG))){
                p1->z.am=mymalloc(AMS);
                memcpy(p1->z.am,am,AMS);
                p1->z.val.vlong=l2zl(0L);
                if(DEBUG&32) printf("z patched\n");
            }
            free(am);count++;
            clear_am(q1reg);
            continue;
        }
        if(c>=LABEL&&c<=BRA){
            int i;      /*  ueber basic blocks hinweg unsicher  */
            for(i=1;i<=16;i++) clear_am(i);
            continue;
        }
        /*  Wenn Libraryaufrufe noetig sind (floating point ohne FPU oder   */
        /*  32bit mul/div/mod ohne 020+) keine Addressierungsarten nutzen   */
        if(g_flags_val[1].l<68000&&(p->typf==FLOAT||p->typf==DOUBLE||c==CONVFLOAT||c==CONVDOUBLE)){
            int i;
            for(i=1;i<=16;i++) clear_am(i);
            continue;
        }
        if(g_flags_val[0].l<68020&&(c==DIV||c==MOD)){
            int i;
            for(i=1;i<=16;i++) clear_am(i);
            continue;
        }
        if(c==PUSH&&((p->q1.flags&(DREFOBJ|REG))==REG&&q1reg<=8&&(am_inc[q1reg]||am_dist[q1reg]||am_dreg[q1reg]))){
            if(q1reg<1||q1reg>8) ierror(0);
            if(am_inc[q1reg]&&am_inc[q1reg]!=msizetab[p->typf&NQ]) clear_am(q1reg); else am_use[q1reg]=p;
            if(DEBUG&32) printf("use of %s found\n",regnames[q1reg]);
            continue;
        }
        if(((p->q1.flags&(DREFOBJ|REG))==(DREFOBJ|REG)&&q1reg<=8)){
            if(q1reg<1||q1reg>8) ierror(0);
            if(am_use[q1reg]&&(am_use[q1reg]!=p||am_inc[q1reg])) clear_am(q1reg); else am_use[q1reg]=p;
            if(am_inc[q1reg]&&am_inc[q1reg]!=sizetab[p->typf&NQ]) clear_am(q1reg);
            if(DEBUG&32) printf("use of %s found\n",regnames[q1reg]);
        }
        if(((p->q2.flags&(DREFOBJ|REG))==(DREFOBJ|REG)&&q2reg<=8)){
            if(q2reg<1||q2reg>8) ierror(0);
            if(am_use[q2reg]&&(am_use[q2reg]!=p||am_inc[q2reg])) clear_am(q2reg); else am_use[q2reg]=p;
            if(am_inc[q2reg]&&am_inc[q2reg]!=sizetab[p->typf&NQ]) clear_am(q2reg);
            if(DEBUG&32) printf("use of %s found\n",regnames[q2reg]);
        }
        if(((p->z.flags&(DREFOBJ|REG))==(DREFOBJ|REG)&&zreg<=8)){
            if(zreg<1||zreg>8) ierror(0);
            if(am_use[zreg]&&(am_use[zreg]!=p||am_inc[zreg])) clear_am(zreg); else am_use[zreg]=p;
            if(am_inc[zreg]&&am_inc[zreg]!=sizetab[p->typf&NQ]) clear_am(zreg);
            if(DEBUG&32) printf("use of %s found\n",regnames[zreg]);
        }
        if(c==ALLOCREG){
        /*  allocreg zaehlt als zerstoerung von reg */
            p->z.flags=REG;
            p->z.reg=zreg=q1reg;
        }
        if(q1reg>=1&&q1reg<=16&&isreg(q1)&&(q1reg>8||am_use[q1reg]!=p)) clear_am(q1reg);
        if(q2reg>=1&&q2reg<=16&&isreg(q2)&&(q2reg>8||am_use[q2reg]!=p)) clear_am(q2reg);
        if(zreg>=1&&zreg<=16&&isreg(z)) clear_am(zreg);
        if(isreg(z)&&zreg<=16){
        /*  schauen, ob eines der Register ueberschrieben wird  */
        /*  wohl noch sehr langsam                              */
            int i;
            for(i=1;i<=8;i++)
                if(!am_use[i]&&(am_base[i]==zreg||(am_dreg[i]&127)==zreg)) clear_am(i);
        }
        if(c==ALLOCREG) p->z.flags=0;
    }
    if(DEBUG&1) printf("%d addressingmodes used, localused=%d\n",count,localused);
    return(localused);
}
static int alignment(struct obj *o)
/*  versucht rauszufinden, wie ein Objekt alignet ist   */
{
    /*  wenn es keine Variable ist, kann man nichts aussagen    */
    long os;
    if((o->flags&(DREFOBJ|VAR))!=VAR||o->am) return(0);
    if(!o->v) ierror(0);
    os=zl2l(o->val.vlong);
    if(o->v->storage_class==AUTO||o->v->storage_class==REGISTER){
        if(!(g_flags[8]&USEDFLAG)){
            if(!zlleq(l2zl(0L),o->v->offset)) os=os+loff-zl2l(o->v->offset);
             else              os=os+zl2l(o->v->offset);
        }else{
            if(!zlleq(l2zl(0L),o->v->offset)) os=os-zl2l(o->v->offset);
             else              os=os-(zl2l(o->v->offset)+zl2l(szof(o->v->vtyp)));
        }
    }
    return(os&3);
}
static void assign(FILE *f,struct IC *p,struct obj *q,struct obj *z,int c,long size,int t)
/*  Generiert Code fuer Zuweisungen und PUSH.   */
{
    /*  auch noch sehr fpu-spezifisch   */
    if((t&NQ)==FLOAT||(t&NQ)==DOUBLE){
        if(q&&(q->flags&KONST)){
            if(z&&(z->flags&(DREFOBJ|REG))==REG){
            /*  FP-Konstante->Register (muss immer reinpassen)  */
                if(z->reg>=17) fprintf(f,"\tfmove"); else fprintf(f,"\tmove");
                fprintf(f,".%c\t",x_t[t&NQ]);probj2(f,q,t);
                fprintf(f,",%s\n",regnames[z->reg]);
            }else{
            /*  FP-Konstante->Speicher (evtl. auf zweimal)  */
                int m;unsigned char *ip=(unsigned char *)&q->val.vfloat; /* nicht sehr schoen  */
                char *s;
                if(gas) s="0x"; else s="$";
                if(c==PUSH&&t==DOUBLE){
                    fprintf(f,"\tmove.l\t#%s%02x%02x%02x%02x,-(a7)\n",s,ip[4],ip[5],ip[6],ip[7]);
                    stackoffset-=4;
                }
                fprintf(f,"\tmove.l\t#%s%02x%02x%02x%02x,",s,ip[0],ip[1],ip[2],ip[3]);
                if(c==ASSIGN) probj2(f,z,t); else {fprintf(f,"-(a7)");stackoffset-=4;}
                fprintf(f,"\n");
                if(t!=DOUBLE||c==PUSH) return;
                m=0;
                if(z&&z->flags&REG){
                    m=1;z->flags|=D16OFF;
                    z->val.vlong=l2zl(0L);
                }
                vlong=l2zl(4L);
                z->val.vlong=zladd(z->val.vlong,vlong);
                fprintf(f,"\tmove.l\t#%s%02x%02x%02x%02x,",s,ip[4],ip[5],ip[6],ip[7]);
                probj2(f,z,t);
                fprintf(f,"\n");
                if(m){
                    z->flags&=~D16OFF;vlong=l2zl(4L);
                    z->val.vlong=zlsub(z->val.vlong,vlong);
                }
            }
            return;
        }
        if((q&&(q->flags&REG)&&q->reg>=17)||(z&&(z->flags&REG)&&z->reg>=17)){
            if(c==ASSIGN&&q->reg==z->reg) return;
            if(c==ASSIGN){ move(f,q,0,z,0,t);return;}
            fprintf(f,"\tfmove.%c\t",x_t[t&NQ]);
            probj2(f,q,t);
            fprintf(f,",");
            if(c==PUSH) {fprintf(f,"-(a7)");stackoffset-=size;} else probj2(f,z,t);
            fprintf(f,"\n");return;
        }
    }
    if(size<=4&&(t&NQ)!=ARRAY&&((t&NQ)!=CHAR||size==1)){
        if((t&NQ)==STRUCT||(t&NQ)==UNION){
            if(size==2) t=SHORT; else t=LONG;
        }
        if(c==ASSIGN){move(f,q,0,z,0,t);return;}
        /*  Sonderfall pea  */
        if((q->flags&VARADR)&&c==PUSH){
            fprintf(f,"\tpea\t");
            q->flags&=~VARADR; probj2(f,q,t); q->flags|=VARADR;
            fprintf(f,"\n"); stackoffset-=4;return;
        }
        fprintf(f,"\tmove.%c\t",x_s[size]);
        probj2(f,q,t);
        fprintf(f,",");
        if(c==PUSH) {fprintf(f,"-(a7)");stackoffset-=size;} else probj2(f,z,t);
        fprintf(f,"\n");return;
    }else{
        int a1,a2,qreg,zreg,dreg,s=size,loops,scratch=0;char *cpstr;
        struct IC *m;
        for(m=p->next;m&&m->code==FREEREG;m=m->next){
            if(q&&m->q1.reg==q->reg) scratch|=1;
            if(z&&m->q1.reg==z->reg) scratch|=2;
        }
        if(c==PUSH) cpstr="\tmove.%c\t-(%s),-(%s)\n"; else cpstr="\tmove.%c\t(%s)+,(%s)+\n";
        if((c==PUSH||(scratch&1))&&(q->flags&(REG|DREFOBJ))==(REG|DREFOBJ)&&q->reg>=1&&q->reg<=8&&!q->am){
            qreg=q->reg;
            if(c==PUSH)
                fprintf(f,"\tadd%s.%s\t#%ld,%s\n",quick[s<=8],strshort[s<=32767],(long)s,regnames[q->reg]);
        }else{
            if(c!=ASSIGN&&!regavailable(0)) qreg=pget_reg(f,0,p);
                else qreg=get_reg(f,0,p);
            if(c==PUSH){q->flags|=D16OFF; q->val.vlong=zladd(q->val.vlong,l2zl((long)s));}
            fprintf(f,"\tlea\t");probj2(f,q,POINTER);
            if(c==PUSH) q->val.vlong=zlsub(q->val.vlong,l2zl((long)s));
            fprintf(f,",%s\n",regnames[qreg]);
        }
        if(c==PUSH){
            zreg=8;
/*            fprintf(f,"\tadd%s.%s\t#%ld,%s\n",quick[s<=8],strshort[s<=32767],(long)s,regnames[qreg]);*/
        }else{
            if((scratch&2)&&(z->flags&(REG|DREFOBJ))==(REG|DREFOBJ)&&z->reg>=1&&z->reg<=8&&!z->am){
                zreg=z->reg;
            }else{
                zreg=get_reg(f,0,p);
                fprintf(f,"\tlea\t");probj2(f,z,POINTER);
                fprintf(f,",%s\n",regnames[zreg]);
            }
        }
        a1=alignment(q);
        if(c!=PUSH)  a2=alignment(z); else a2=0;
        /*  wenn Typ==CHAR, dann ist das ein inline_memcpy und wir nehmen   */
        /*  das unguenstigste Alignment an                                  */
        if((t&NQ)==CHAR){ a1=1;a2=2;}

        if((a1&1)&&(a2&1)){fprintf(f,cpstr,'b',regnames[qreg],regnames[zreg]);s--;a1&=~1;a2&=~1;}
        if((a1&2)&&(a2&2)){fprintf(f,cpstr,'w',regnames[qreg],regnames[zreg]);s-=2;a1&=~2;a2&=~2;}
        if(!(a1&1)&&!(a2&1)) loops=s/16-1; else loops=s/4-1;
        if(loops>0){
            if(c!=ASSIGN&&!regavailable(1)) dreg=pget_reg(f,0,p);
                else dreg=get_reg(f,1,p);
            fprintf(f,"\tmove%s.l\t#%d,%s\nl%d:\n",quick[loops>=-128&&loops<=127],loops,regnames[dreg],++label);
        }
        if(loops>=0){
            int t;
            if(!(a1&1)&&!(a2&1)) t='l'; else t='b';
            fprintf(f,cpstr,t,regnames[qreg],regnames[zreg]);
            fprintf(f,cpstr,t,regnames[qreg],regnames[zreg]);
            fprintf(f,cpstr,t,regnames[qreg],regnames[zreg]);
            fprintf(f,cpstr,t,regnames[qreg],regnames[zreg]);
        }
        if(loops>0){
            if(loops<=32767&&loops>=-32768){
                fprintf(f,"\tdbra\t%s,l%d\n",regnames[dreg],label);
            }else{
                fprintf(f,"\tsubq.l\t#1,%s\n\tbge\tl%d\n",regnames[dreg],label);
            }
        }
        if(!(a1&1)&&!(a2&1)){
            if(s&8){
                fprintf(f,cpstr,'l',regnames[qreg],regnames[zreg]);
                fprintf(f,cpstr,'l',regnames[qreg],regnames[zreg]);
            }
            if(s&4) fprintf(f,cpstr,'l',regnames[qreg],regnames[zreg]);
            if(s&2) fprintf(f,cpstr,'w',regnames[qreg],regnames[zreg]);
            if(s&1) fprintf(f,cpstr,'b',regnames[qreg],regnames[zreg]);
        }else{
            s&=3;
            while(s){fprintf(f,cpstr,'b',regnames[qreg],regnames[zreg]);s--;}
        }
        if(c==PUSH) stackoffset-=size;
    }
    return;
}
static int store_saveregs;
static void saveregs(FILE *f,struct IC *p)
{
    int dontsave;
    store_saveregs=0;
    if((p->z.flags&(REG|DREFOBJ))==REG) dontsave=p->z.reg; else dontsave=0;
    if(dontsave!= 9&&regs[ 9]) {fprintf(f,"\tmove.l\td0,-(a7)\n");stackoffset-=4;store_saveregs|=1;}
    if(dontsave!=10&&regs[10]) {fprintf(f,"\tmove.l\td1,-(a7)\n");stackoffset-=4;store_saveregs|=2;}
    if(dontsave!= 1&&regs[ 1]) {fprintf(f,"\tmove.l\ta0,-(a7)\n");stackoffset-=4;store_saveregs|=4;}
    if(dontsave!= 2&&regs[ 2]) {fprintf(f,"\tmove.l\ta1,-(a7)\n");stackoffset-=4;store_saveregs|=8;}
}
static void restoreregsa(FILE *f,struct IC *p)
{
    if(store_saveregs&8) {fprintf(f,"\tmove.l\t(a7)+,a1\n");stackoffset+=4;}
    if(store_saveregs&4) {fprintf(f,"\tmove.l\t(a7)+,a0\n");stackoffset+=4;}
}
static void restoreregsd(FILE *f,struct IC *p)
{
    int dontsave;
    if((p->z.flags&(REG|DREFOBJ))==REG) dontsave=p->z.reg; else dontsave=0;
    if(dontsave!=10&&(store_saveregs&2)) {fprintf(f,"\tmovem.l\t(a7)+,d1\n");stackoffset+=4;}
    if(dontsave!=9 &&(store_saveregs&1)) {fprintf(f,"\tmovem.l\t(a7)+,d0\n");stackoffset+=4;}
}
static int is_const(struct Typ *t)
/*  tested, ob ein Typ konstant (und damit evtl. in der Code-Section) ist   */
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
/*  End of private fata and functions.  */
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
    for(i= 1;i<=16;i++) regsize[i]=l2zl( 4L);
    for(i=17;i<=24;i++) regsize[i]=l2zl(12L);

    /*  default CPU is 68000    */
    if(!(g_flags[0]&USEDFLAG)) g_flags_val[0].l=68000;
    /*  no FPU by default       */
    if(!(g_flags[1]&USEDFLAG)) g_flags_val[1].l=0;
    if(g_flags_val[1].l<68000) {x_t[FLOAT]='l';}
    if(g_flags[2]&USEDFLAG) regscratch[11]=1;
    if(g_flags[3]&USEDFLAG) regsa[5]=1;
    if(g_flags[15]&USEDFLAG) dbout=1;
    if(g_flags[11]&USEDFLAG){
        gas=1;
        codename="\t.text\n";
        bssname="";
        dataname="\t.data\n";
        if(g_flags[5]&USEDFLAG) regsa[5]=1;
    }else{
        codename="\tsection\t\"CODE\",code\n";
        if(g_flags[5]&USEDFLAG){
            /*  preparing small data    */
            regsa[5]=1;
            bssname= "\tsection\t\"__MERGED\",bss\n";
            dataname="\tsection\t\"__MERGED\",data\n";
        }else{
            bssname= "\tsection\t\"BSS\",bss\n";
            dataname="\tsection\t\"DATA\",data\n";
        }
    }
    /*  a5 can be used if no framepointer is used.  */
    if(!(g_flags[8]&USEDFLAG)) regsa[6]=0;
    if(DEBUG&1) printf("CPU=%ld FPU=%ld\n",g_flags_val[0].l,g_flags_val[1].l);

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
    t_max[CHAR]=ul2zul(127L);
    t_max[SHORT]=ul2zul(32767UL);
    t_max[INT]=ul2zul(2147483647UL);
    t_max[LONG]=t_max[INT];
    t_max[UNSIGNED|CHAR]=ul2zul(255UL);
    t_max[UNSIGNED|SHORT]=ul2zul(65535UL);
    t_max[UNSIGNED|INT]=ul2zul(4294967295UL);
    t_max[UNSIGNED|LONG]=t_max[UNSIGNED|INT];

    return(1);
}

int freturn(struct Typ *t)
/*  Returns the register in which variables of type t are returned. */
/*  If the value cannot be returned in a register returns 0.        */
{
    long l;int tu=t->flags&NQ;
    if(tu==FLOAT){
        if(g_flags_val[1].l>=68000&&!(g_flags[13]&USEDFLAG))
            return(17);
        else
            return(9);
    }
    if(tu==DOUBLE){
        if(g_flags_val[1].l>=68000&&!(g_flags[13]&USEDFLAG)){
            return(17);
        }else{
            if(g_flags[14]&USEDFLAG) return(0);
            return(9);
        }
    }
    if(tu==STRUCT||tu==UNION){
        if(!(g_flags[14]&USEDFLAG)){
            l=zl2l(szof(t));
            if(l==4||l==8||l==12||l==16) return(9);
        }
        return(0);
    }
    if(zlleq(szof(t),l2zl(4L))) return(9); else return(0);
}

int regok(int r,int t,int mode)
/*  Returns 0 if register r cannot store variables of   */
/*  type t. If t==POINTER and mode!=0 then it returns   */
/*  non-zero only if the register can store a pointer   */
/*  and dereference a pointer to mode.                  */
{
    if(r==0) return(0);
    t&=NQ;
    if(t==FLOAT||t==DOUBLE){
        if(g_flags_val[1].l>=68000){
            if(r>=17&&r<=24) return(1); else return(0);
        }else{
            if(t==FLOAT&&r>=9&&r<=16) return(1); else return(0);
        }
    }
    if(t==POINTER&&mode==0&&r>=9&&r<=16) return(1);
    if(t==POINTER&&r>=1&&r<=8) return(1);
    if(t>=CHAR&&t<=LONG&&r>=9&&r<=16) return(1);
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
    if((c==DIV||c==MOD)&&!(p->q2.flags&KONST)) return(1);
    return(0);
}

int must_convert(np p,int t)
/*  Returns zero if code for converting np to type t    */
/*  can be omitted.                                     */
{
    int o=p->ntyp->flags,op=o&NQ,tp=t&NQ;
    /*  All pointers have the same representation.  */
    if(tp==POINTER&&op==POINTER) return(0);
    /*  Pointer and int/long as well   */
/*    if(tp==POINTER&&(op==INT||op==LONG)) return(0);
    if(op==POINTER&&(tp==INT||tp==LONG)) return(0);*/
    /*  Signed und Unsigned integers with the same size, too.  */
    if((t&UNSIGNED)&&(o&UNSIGNED)&&zleqto(sizetab[tp],sizetab[op])) return(0);
    /*  int==long   */
    if((tp==INT&&op==LONG)||(tp==LONG&&op==INT)) return(0);

    if((tp==FLOAT||tp==DOUBLE)&&(op==FLOAT||op==DOUBLE)&&(p->o.flags&REG)&&p->o.reg>=17&&p->o.reg<=24)
        return(0);

    return(1);
}

void gen_ds(FILE *f,zlong size,struct Typ *t)
/*  This function has to create <size> bytes of storage */
/*  initialized with zero.                              */
{
    if(gas){
        if(newobj) fprintf(f,"%ld\n",zl2l(size));
            else   fprintf(f,"\t.space\t%ld\n",zl2l(size));
        newobj=0;
    }else{
        if(section!=BSS&&newobj){fprintf(f,bssname);section=BSS;}
        fprintf(f,"\tds.b\t%ld\n",zl2l(size));newobj=0;
    }
}

void gen_align(FILE *f,zlong align)
/*  This function has to make sure the next data is     */
/*  aligned to multiples of <align> bytes.              */
{
    if(align>1){
        if(gas){
            fprintf(f,"\t.align\t2\n");
        }else{
            fprintf(f,"\tcnop\t0,4\n");
        }
    }
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
        if(v->clist&&(!constflag||(g_flags[7]&USEDFLAG))&&section!=DATA){fprintf(f,dataname);section=DATA;}
        if(v->clist&&constflag&&!(g_flags[7]&USEDFLAG)&&section!=CODE){fprintf(f,codename);section=CODE;}
        if(!v->clist&&section!=BSS){fprintf(f,bssname);section=BSS;}
        if(gas){
            if(section!=BSS) fprintf(f,"\t.align\t2\nl%ld:\n",zl2l(v->offset));
                else fprintf(f,"\t.lcomm\tl%ld,",zl2l(v->offset));
        }else{
            fprintf(f,"\tcnop\t0,4\nl%ld\n",zl2l(v->offset));
        }
        newobj=1;
    }
    if(v->storage_class==EXTERN){
        if(gas){
            fprintf(f,"\t.global\t_%s\n",v->identifier);
        }else{
            fprintf(f,"\tpublic\t_%s\n",v->identifier);
        }
        if(v->flags&(DEFINED|TENTATIVE)){
            if(v->clist&&(!constflag||(g_flags[7]&USEDFLAG))&&section!=DATA){fprintf(f,dataname);section=DATA;}
            if(v->clist&&constflag&&!(g_flags[7]&USEDFLAG)&&section!=CODE){fprintf(f,codename);section=CODE;}
            if(!v->clist&&section!=BSS){fprintf(f,bssname);section=BSS;}
            if(gas){
                if(section!=BSS) fprintf(f,"\t.align\t2\n_%s:\n",v->identifier);
                    else fprintf(f,"\t.comm\t_%s,",v->identifier);
            }else{
                fprintf(f,"\tcnop\t0,4\n_%s\n",v->identifier);
            }
            newobj=1;
        }
    }
}
void gen_dc(FILE *f,int t,struct const_list *p)
/*  This function has to create static storage          */
/*  initialized with const-list p.                      */
{
    char s;
    if(!p){ierror(0);return;}
/*    if(section!=DATA){fprintf(f,dataname);section=DATA;}*/
    if((t&NQ)==FLOAT||(t&NQ)==DOUBLE) s='l'; else s=x_t[t&NQ];
    if(gas){
        char *str;
        if(s=='b') str="\t.byte\t";
        else if(s=='w') str="\t.short\t";
        else if(s=='l') str="\t.long\t";
        else ierror(0);
        fprintf(f,"%s",str);
    }else{
        fprintf(f,"\tdc.%c\t",s);
    }
    if(!p->tree){
        if((t&NQ)==FLOAT||(t&NQ)==DOUBLE){
        /*  auch wieder nicht sehr schoen und IEEE noetig   */
            unsigned char *ip;char *s;
            ip=(unsigned char *)&p->val.vdouble;
            if(gas) s="0x"; else s="$";
            fprintf(f,"%s%02x%02x%02x%02x",s,ip[0],ip[1],ip[2],ip[3]);
            if((t&NQ)==DOUBLE){
                fprintf(f,",%s%02x%02x%02x%02x",s,ip[4],ip[5],ip[6],ip[7]);
            }
        }else{
            printval(f,&p->val,t&NU,0);
        }
    }else{
        int m,m2;
        p->tree->o.am=0;
        m=p->tree->o.flags;
        p->tree->o.flags&=~VARADR;
        m2=g_flags[5];
        g_flags[5]&=~USEDFLAG;
        probj2(f,&p->tree->o,t&NU);
        p->tree->o.flags=m;
        g_flags[5]=m2;
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
{
    int c,t,comptyp;char fp[2]="\0\0";
    int act_line=0;char *act_file=0;
    if(DEBUG&1) printf("gen_code()\n");
    for(c=1;c<=MAXR;c++) regs[c]=regsa[c];
    if(!(g_flags[9]&USEDFLAG)){
    /*  Adressierungsarten benutzen */
        if(!addressing()) offset=l2zl(0L);
    }
    if(dbout){
        if(!gas){
            act_line=fline;
            fprintf(f,"\tdsource\t\"%s\"\n",errfname);
            fprintf(f,"\tdebug\t%d\n",act_line);
        }
    }
    reglabel=++label;freglabel=++label;
    function_top(f,v,zl2l(offset));
    if(p!=first_ic) ierror(0);
    cc_set=cc_set_tst=0;
    stackoffset=notpopped=dontpop=0;
    for(;p;pr(f,p),p=p->next){
        if(dbout){
        /*  Hier soll spaeter mal das Sourcefile angegeben werden.  */
            if(p->file&&p->file!=act_file) ierror(0);
            if(p->line&&p->line!=act_line){
                act_line=p->line;
                if(!gas) fprintf(f,"\tdebug\t%d\n",act_line);
            }
        }
        c=p->code;t=p->typf;
        if(c==NOP) continue;
        cc_set_tst=cc_set;
        cc_typ_tst=cc_typ;
        if(cc_set_tst&&(DEBUG&512)){fprintf(f,"; cc_set_tst=");probj2(f,cc_set_tst,t);fprintf(f,"\n");}
        if(cc_set&&(DEBUG&512)){fprintf(f,"; cc_set=");probj2(f,cc_set,t);fprintf(f,"\n");}
        pushedreg&=16;if(c==RESTOREREGS) pushedreg=0;
        if(DEBUG&256){fprintf(f,"; "); pric2(f,p);}
        if(DEBUG&512) fprintf(f,"; stackoffset=%ld, notpopped=%ld, pushedreg=%d, dontpop=%ld\n",stackoffset,notpopped,pushedreg,dontpop);
        /*  muessen wir Argumente poppen?   */
        if(notpopped&&!dontpop){
            int flag=0;
            if(c==LABEL||c==COMPARE||c==TEST||c==BRA){
                fprintf(f,"\tadd%s.%s\t#%ld,a7\n",quick[notpopped<=8],strshort[notpopped<32768],notpopped);
                stackoffset+=notpopped;notpopped=0;/*cc_set_tst=cc_set=0;*/
            }
        }
        /*  na, ob das hier ok ist..?   */
        if(c==SUBPFP) c=SUB;
        if(c==PMULT) c=MULT;
        if(c==ALLOCREG) {regs[p->q1.reg]=1;continue;}
        if(c==FREEREG) {regs[p->q1.reg]=0;continue;}
        if(c==LABEL){
            if(dbout) act_line=0;
            if(gas){
                fprintf(f,"l%d:\n",t);
            }else{
                fprintf(f,"l%d\n",t);
            }
            cc_set=0;continue;
        }
        if(c==BRA){fprintf(f,"\t%sbra\tl%d\n",(gas?"j":""),t);continue;}
        if(c>=BEQ&&c<BRA){
            if(gas){
                if(stored_cc){fprintf(f,"\tjne\tl%d\n",t);stored_cc=0;continue;}
                if((comptyp&UNSIGNED)||(comptyp&NQ)==POINTER){
                    fprintf(f,"\tj%s\tl%d\n",ubranch[c-BEQ],t);
                }else{
                    fprintf(f,"\t%sj%s\tl%d\n",fp,ename[c]+1,t);
                }
            }else{
                if(stored_cc){fprintf(f,"\tbne\tl%d\n",t);stored_cc=0;continue;}
                if((comptyp&UNSIGNED)||(comptyp&NQ)==POINTER){
                    fprintf(f,"\tb%s\tl%d\n",ubranch[c-BEQ],t);
                }else{
                    fprintf(f,"\t%s%s\tl%d\n",fp,ename[c],t);
                }
            }
            continue;
        }
        if(p->q1.am){
            if(!regs[p->q1.am->basereg]){pric2(stdout,p);printf("%s\n",regnames[p->q1.am->basereg]); ierror(0);}
            if(p->q1.am->dreg&&!regs[p->q1.am->dreg&127]) {printf("Register %s:\n",regnames[p->q1.am->dreg&127]);ierror(0);}
        }
        if(p->q2.am){
            if(!regs[p->q2.am->basereg]) ierror(0);
            if(p->q2.am->dreg&&!regs[p->q2.am->dreg&127]) {printf("Register %s:\n",regnames[p->q2.am->dreg&127]);ierror(0);}
        }
        if(p->z.am){
            if(!regs[p->z.am->basereg]) ierror(0);
            if(p->z.am->dreg&&!regs[p->z.am->dreg&127]) {printf("Register %s:\n",regnames[p->z.am->dreg&127]);ierror(0);}
        }
        if((p->q1.flags&REG)&&!regs[p->q1.reg]){printf("Register %s:\n",regnames[p->q1.reg]);ierror(0);}
        if((p->q2.flags&REG)&&!regs[p->q2.reg]){printf("Register %s:\n",regnames[p->q2.reg]);ierror(0);}
        if((p->z.flags&REG)&&!regs[p->z.reg]){printf("Register %s:\n",regnames[p->z.reg]);ierror(0);}
        if((p->q2.flags&REG)&&(p->z.flags&REG)&&p->q2.reg==p->z.reg){pric2(stdout,p);ierror(0);}
        if((p->q2.flags&VAR)&&(p->z.flags&VAR)&&p->q2.v==p->z.v){pric2(stdout,p);ierror(0);}
        /*  COMPARE #0 durch TEST ersetzen (erlaubt, da tst alle Flags setzt)   */
        if(c==COMPARE&&(p->q2.flags&KONST)){
            eval_const(&p->q2.val,t);
            if(zleqto(l2zl(0L),vlong)&&zuleqto(ul2zul(0UL),vulong)&&zdeqto(d2zd(0.0),vdouble)){
                c=p->code=TEST;p->q2.flags=0;
            }
        }
        if(c==COMPARE&&(p->q1.flags&KONST)){
            eval_const(&p->q1.val,t);
            if(zleqto(l2zl(0L),vlong)&&zuleqto(ul2zul(0UL),vulong)&&zdeqto(d2zd(0.0),vdouble)){
                struct IC *bp=p->next;int bc;
                c=p->code=TEST;p->q1=p->q2;p->q2.flags=0;p->q2.am=0;
                /*  Nachfolgenden Branch umdrehen   */
                while(bp&&bp->code==FREEREG) bp=bp->next;
                bc=bp->code;
                if(!bp||bc<BEQ||bc>BGT) ierror(0);
                if(bc==BLT) bp->code=BGT;
                if(bc==BGT) bp->code=BLT;
                if(bc==BLE) bp->code=BGE;
                if(bc==BGE) bp->code=BLE;
            }
        }
        /*  gesetzte ConditionCodes merken  */
        if(p->z.flags&&(!isreg(z)||p->z.reg>=9)&&c!=CONVFLOAT&&c!=CONVDOUBLE&&(((t&NQ)!=FLOAT&&(t&NQ)!=DOUBLE)||g_flags_val[1].l>68000)){
             cc_set=&p->z;cc_typ=p->typf;
        }else{
            cc_set=0;
        }
        if(c==PEA){
            fprintf(f,"\tpea\t");probj2(f,&p->q1,t);fprintf(f,"\n");
            stackoffset-=zl2l(p->q2.val.vlong);
            dontpop+=zl2l(p->q2.val.vlong);
            continue;
        }
        if(c==MOVEFROMREG){
            if(p->q1.reg<17) fprintf(f,"\tmove.l\t%s,",regnames[p->q1.reg]);
                else         fprintf(f,"\tfmove.x\t%s,",regnames[p->q1.reg]);
            probj2(f,&p->z,t);fprintf(f,"\n");
            continue;
        }
        if(c==MOVETOREG){
            if(p->z.reg<17) fprintf(f,"\tmove.l\t");
                else        fprintf(f,"\tfmove.x\t");
            probj2(f,&p->q1,t);fprintf(f,",%s\n",regnames[p->z.reg]);
            continue;
        }
        if(g_flags[9]&USEDFLAG)
            if(p->q1.am||p->q2.am||p->z.am){
                ierror(0);
                p->q1.am=p->q2.am=p->z.am=0;
            }
        p=do_refs(f,p);
        if(g_flags[9]&USEDFLAG)
            if(p->q1.am||p->q2.am||p->z.am){
                ierror(0);
                p->q1.am=p->q2.am=p->z.am=0;
            }
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
            if(c==CONVVOID){ierror(0);continue;}
            if(t==FLOAT||t==DOUBLE||to==FLOAT||to==DOUBLE){
                if(g_flags_val[1].l>=68000){
                    int zreg=0;
                    if((t==FLOAT||t==DOUBLE)&&(to==FLOAT||to==DOUBLE)){
                        if(isreg(q1)&&isreg(z)){
                            if(p->q1.reg!=p->z.reg)
                                fprintf(f,"\tfmove.x\t%s,%s\n",regnames[p->q1.reg],regnames[p->z.reg]);
                            continue;
                        }
                    }
                    if(isreg(z)&&p->z.reg>=17)
                        zreg=p->z.reg;
                    if(isreg(q1)&&p->q1.reg>=17){
                        if(!zreg) zreg=p->q1.reg; else zreg=get_reg(f,2,p);}
                    if(!zreg) zreg=get_reg(f,2,p);
                    if((to&UNSIGNED)&&x_t[to&NQ]!='l'){
                        int dreg=get_reg(f,1,p);
                        fprintf(f,"\tmoveq\t#0,%s\n",regnames[dreg]);
                        move(f,&p->q1,0,0,dreg,to);
                        move(f,0,dreg,0,zreg,LONG);
                    }else{
                        if(!isreg(q1)||p->q1.reg!=zreg)
                            move(f,&p->q1,0,0,zreg,to);
                    }
                    if(t!=FLOAT&&t!=DOUBLE){
                    /*  nach integer, d.h. Kommastellen abschneiden */
                        if(g_flags_val[1].l==68040/*||g_flags_val[1].l==68060*/){
                        /*  bei 040 emuliert    */
                            int dreg1=get_reg(f,1,p),dreg2=get_reg(f,1,p);
                            fprintf(f,"\tfmove.l\tfpcr,%s\n",regnames[dreg2]);
                            fprintf(f,"\tmoveq\t#16,%s\n",regnames[dreg1]);
                            fprintf(f,"\tor.l\t%s,%s\n",regnames[dreg2],regnames[dreg1]);
                            fprintf(f,"\tand.w\t#-33,%s\n",regnames[dreg1]);
                            fprintf(f,"\tfmove.l\t%s,fpcr\n",regnames[dreg1]);
                            if((t&UNSIGNED)&&(t&NQ)<INT){
                                fprintf(f,"\tfmove.l\t%s,%s\n",regnames[zreg],regnames[dreg1]);
                                fprintf(f,"\tmove.%c\t%s,",x_t[t&NQ],regnames[dreg1]);
                            }else{
                                fprintf(f,"\tfmove.%c\t%s,",x_t[t&NQ],regnames[zreg]);
                            }
                            probj2(f,&p->z,t);fprintf(f,"\n");
                            fprintf(f,"\tfmove.l\t%s,fpcr\n",regnames[dreg2]);
                            continue;
                        }else{
                            if(!isreg(q1)||p->q1.reg!=zreg){
                                fprintf(f,"\tfintrz.x\t%s\n",regnames[zreg]);
                            }else{
                                int nreg=get_reg(f,2,p);
                                fprintf(f,"\tfintrz.x\t%s,%s\n",regnames[zreg],regnames[nreg]);
                                zreg=nreg;
                            }
                            if((t&UNSIGNED)&&(t&NQ)<INT){
                                int r;
                                if(p->z.flags&REG) r=p->z.reg; else r=get_reg(f,1,p);
                                move(f,0,zreg,0,r,LONG);
                                move(f,0,r,&p->z,0,t);
                            }else{
                                move(f,0,zreg,&p->z,0,t);
                            }
                            continue;
                        }
                    }
                    if(to&UNSIGNED&&x_t[to&NQ]=='l'){
                        int nlabel;
                        fprintf(f,"\ttst.%c\t",x_t[to&NQ]);
                        probj2(f,&p->q1,to);fprintf(f,"\n");
                        nlabel=++label;
                        fprintf(f,"\tbge.s\tl%d\n",nlabel);
                        fprintf(f,"\tfadd.d\t#4294967296,%s\n",regnames[zreg]);
                        fprintf(f,"l%d:\n",nlabel);
                    }
                    if(!(p->z.reg)||p->z.reg!=zreg){
                        move(f,0,zreg,&p->z,0,t);
                    }
                }else{
                    cc_set=0;
                    if(to==t){
                        assign(f,p,&p->q1,&p->z,ASSIGN,zl2l(p->q2.val.vlong),t);
                        continue;
                    }
                    if(to==FLOAT&&t==DOUBLE){
                        saveregs(f,p);
                        assign(f,p,&p->q1,0,PUSH,msizetab[FLOAT],FLOAT);
                        if(gas){
                            fprintf(f,"\t.global\t__ieees2d\n\tjbsr\t__ieees2d\n\taddq.w\t#4,a7\n");
                        }else{
                            fprintf(f,"\tpublic\t__ieees2d\n\tjsr\t__ieees2d\n\taddq.w\t#4,a7\n");
                        }
                        stackoffset+=4;
                        restoreregsa(f,p);
                        fprintf(f,"\tmovem.l\td0/d1,");
                        probj2(f,&p->z,t);fprintf(f,"\n");
                        restoreregsd(f,p);
                        continue;
                    }
                    if(to==DOUBLE&&t==FLOAT){
                        saveregs(f,p);
                        assign(f,p,&p->q1,0,PUSH,msizetab[DOUBLE],DOUBLE);
                        if(gas){
                            fprintf(f,"\t.global\t__ieeed2s\n\tjbsr\t__ieeed2s\n\taddq.w\t#8,a7\n");
                        }else{
                            fprintf(f,"\tpublic\t__ieeed2s\n\tjsr\t__ieeed2s\n\taddq.w\t#8,a7\n");
                        }
                        stackoffset+=8;
                        restoreregsa(f,p);
                        move(f,0,9,&p->z,0,t);
                        restoreregsd(f,p);
                        continue;
                    }
                    if(to==FLOAT||to==DOUBLE){
                        int uns;
                        saveregs(f,p);
                        if(t&UNSIGNED) uns='u'; else uns='s';
                        assign(f,p,&p->q1,0,PUSH,sizetab[to&NQ],to);
                        if(gas){
                            fprintf(f,"\t.global\t__ieeefix%c%c\n\tjbsr\t__ieeefix%c%c\n\taddq.w\t#%ld,a7\n",x_t[to&NQ],uns,x_t[to&NQ],uns,zl2l(sizetab[to&NQ]));
                        }else{
                            fprintf(f,"\tpublic\t__ieeefix%c%c\n\tjsr\t__ieeefix%c%c\n\taddq.w\t#%ld,a7\n",x_t[to&NQ],uns,x_t[to&NQ],uns,zl2l(sizetab[to&NQ]));
                        }
                        stackoffset+=sizetab[to&NQ];
                        restoreregsa(f,p);
                        move(f,0,9,&p->z,0,t);
                        restoreregsd(f,p);
                        continue;
                    }else{
                        int uns,xt=x_t[to&NQ];
                        saveregs(f,p);
                        if(to&UNSIGNED) uns='u'; else uns='s';
                        if(xt!='l') {fprintf(f,"\tsubq.w\t#4,a7\n");stackoffset-=4;}
                        fprintf(f,"\tmove.%c\t",xt);
                        probj2(f,&p->q1,to);
                        if(xt!='l') fprintf(f,",(a7)\n"); else {fprintf(f,",-(a7)\n");stackoffset-=4;}
                        if(gas){
                            fprintf(f,"\t.global\t__ieeeflt%c%c%c\n\tjbsr\t__ieeeflt%c%c%c\n\taddq.w\t#4,a7\n",uns,xt,x_t[t&NQ],uns,xt,x_t[t&NQ]);
                        }else{
                            fprintf(f,"\tpublic\t__ieeeflt%c%c%c\n\tjsr\t__ieeeflt%c%c%c\n\taddq.w\t#4,a7\n",uns,xt,x_t[t&NQ],uns,xt,x_t[t&NQ]);
                        }
                        stackoffset+=4;
                        restoreregsa(f,p);
                        if(t==DOUBLE){
                            fprintf(f,"\tmovem.l\td0/d1,");
                            probj2(f,&p->z,t);fprintf(f,"\n");
                        }else move(f,0,9,&p->z,0,t);
                        restoreregsd(f,p);
                        continue;
                    }
                }
                continue;
            }
            if((to&NQ)<(t&NQ)){
                int zreg;
                if(isreg(z)&&p->z.reg>=9&&p->z.reg<=16)
                    zreg=p->z.reg; else zreg=get_reg(f,1,p);
                /*  aufpassen, falls unsigned und Quelle==Ziel  */
                if((to&UNSIGNED)&&isreg(q1)&&zreg==p->q1.reg){
                    unsigned long l;
                    if((to&NQ)==CHAR) l=0xff; else l=0xffff;
                    fprintf(f,"\tand.%c\t#%lu,%s\n",x_t[t&NQ],l,regnames[zreg]);
                    continue;
                }
                if(to&UNSIGNED) fprintf(f,"\tmoveq\t#0,%s\n",regnames[zreg]);
                move(f,&p->q1,0,0,zreg,to);
                if(!(to&UNSIGNED)){
                    if((to&NQ)==CHAR&&(t&NQ)==SHORT) fprintf(f,"\text.w\t%s\n",regnames[zreg]);
                    if((to&NQ)==SHORT&&msizetab[t&NQ]==4) fprintf(f,"\text.l\t%s\n",regnames[zreg]);
                    if((to&NQ)==CHAR&&msizetab[t&NQ]==4){
                        if(g_flags_val[0].l>=68020)
                            fprintf(f,"\textb.l\t%s\n",regnames[zreg]);
                        else
                            fprintf(f,"\text.w\t%s\n\text.l\t%s\n",regnames[zreg],regnames[zreg]);
                    }
                }
                if(!isreg(z)||p->z.reg!=zreg){
                    move(f,0,zreg,&p->z,0,t);
                }
            }else{
                long diff;int m;
                m=0;
                if(p->q1.flags&REG){
                    p->q1.val.vlong=l2zl(0L);
                    p->q1.flags|=D16OFF;m=1;
                }
                diff=msizetab[to&NQ]-msizetab[t&NQ];
                vlong=l2zl(diff);
                p->q1.val.vlong=zladd(p->q1.val.vlong,vlong);
                move(f,&p->q1,0,&p->z,0,t);
                vlong=l2zl(diff);
                p->q1.val.vlong=zlsub(p->q1.val.vlong,vlong);
                if(m) p->q1.flags&=~D16OFF;
            }
            continue;
        }
        if((t==FLOAT||t==DOUBLE)&&g_flags_val[1].l>=68000) *fp='f'; else *fp=0;
        if(c==MINUS||c==KOMPLEMENT){
            int zreg;
            if(t==FLOAT||t==DOUBLE){
                if(g_flags_val[1].l>=68000){
                    if(isreg(z)) zreg=p->z.reg; else zreg=get_reg(f,2,p);
                    fprintf(f,"\tfneg.");
                    if(isreg(q1)) fprintf(f,"x\t%s",regnames[p->q1.reg]);
                        else    {fprintf(f,"%c\t",x_t[t&NQ]);probj2(f,&p->q1,t);}
                    fprintf(f,",%s\n",regnames[zreg]);
                    if(!isreg(z)||p->z.reg!=zreg){
                        move(f,0,zreg,&p->z,0,t);
                    }
                    continue;
                }else{
                    saveregs(f,p);
                    assign(f,p,&p->q1,0,PUSH,msizetab[t&NQ],t);
                    if(gas){
                        fprintf(f,"\t.global\t__ieeeneg%c\n\tjbsr\t__ieeeneg%c\n\taddq.w\t#%ld,a7\n",x_t[t&NQ],x_t[t&NQ],msizetab[t&NQ]);
                    }else{
                        fprintf(f,"\tpublic\t__ieeeneg%c\n\tjsr\t__ieeeneg%c\n\taddq.w\t#%ld,a7\n",x_t[t&NQ],x_t[t&NQ],msizetab[t&NQ]);
                    }
                    stackoffset+=msizetab[t&NQ];
                    restoreregsa(f,p);
                    if(t==DOUBLE){
                        fprintf(f,"\tmovem.l\td0/d1,");
                        probj2(f,&p->z,t);fprintf(f,"\n");
                    }else move(f,0,9,&p->z,0,t);
                    restoreregsd(f,p);
                    continue;
                }
            }
            if(compare_objects(&p->q1,&p->z)){
                fprintf(f,"\t%s.%c\t",ename[c],x_t[t&NQ]);
                probj2(f,&p->q1,t);fprintf(f,"\n");
                continue;
            }
            if(isreg(z)&&p->z.reg>=9/*&&p->z.reg<=16*/)
                zreg=p->z.reg; else zreg=get_reg(f,1,p);
            if(!isreg(q1)||p->q1.reg!=zreg){
                move(f,&p->q1,0,0,zreg,t);
            }
            fprintf(f,"\t%s.%c\t%s\n",ename[c],x_t[t&NQ],regnames[zreg]);
            if(!isreg(z)||p->z.reg!=zreg){
                move(f,0,zreg,&p->z,0,t);
            }
            continue;
        }
        if(c==SETRETURN){
        /*  Returnwert setzen - q2.val.vlong==size, z.reg==Returnregister */
            if((t&NQ)==DOUBLE&&p->z.reg==9){
                if(p->q1.flags&KONST){
                    unsigned char *ip=(unsigned char *)&p->q1.val.vdouble;
                    char *s;
                    if(gas) s="0x"; else s="$";
                    fprintf(f,"\tmove.l\t#%s%02x%02x%02x%02x,d0\n",s,ip[0],ip[1],ip[2],ip[3]);
                    fprintf(f,"\tmove.l\t#%s%02x%02x%02x%02x,d1\n",s,ip[4],ip[5],ip[6],ip[7]);
                    continue;
                }
                if(isreg(q1)){
                    fprintf(f,"\tfmove.d\t%s,-(a7)\n\tmovem.l\t(a7)+",regnames[p->q1.reg]);
                }else{
                    fprintf(f,"\tmovem.l\t");probj2(f,&p->q1,t);
                }
                fprintf(f,",d0/d1\n");
                continue;
            }
            if(((t&NQ)==STRUCT||(t&NQ)==UNION)&&p->z.reg==9){
                long l=zl2l(p->q2.val.vlong);
                fprintf(f,"\tmovem.l\t");probj2(f,&p->q1,t);
                fprintf(f,",d0");
                if(l>=8) fprintf(f,"/d1");
                if(l>=12) fprintf(f,"/a0");
                if(l>=16) fprintf(f,"/a1");
                fprintf(f,"\n");
                continue;
            }
        /*  Wenn Returnwert ueber Zeiger gesetzt wird, nichts noetig    */
            if(p->z.reg) move(f,&p->q1,0,0,p->z.reg,p->typf);
            continue;
        }
        if(c==GETRETURN){
        /*  Returnwert holen - q2.val.vlong==size, q1.reg==Returnregister     */
            if((t&NQ)==DOUBLE&&p->q1.reg==9){
                fprintf(f,"\tmovem.l\td0/d1");
                if(isreg(z)){
                    fprintf(f,",-(a7)\n\tfmove.d\t(a7)+,%s\n",regnames[p->z.reg]);
                }else{
                    fprintf(f,",");probj2(f,&p->z,t);fprintf(f,"\n");
                }
                continue;
            }
            if(((t&NQ)==STRUCT||(t&NQ)==UNION)&&p->q1.reg==9){
                long l=zl2l(p->q2.val.vlong);
                fprintf(f,"\tmovem.l\t");
                fprintf(f,"d0");
                if(l>=8) fprintf(f,"/d1");
                if(l>=12) fprintf(f,"/a0");
                if(l>=16) fprintf(f,"/a1");
                fprintf(f,",");probj2(f,&p->z,t);fprintf(f,"\n");
                continue;
            }

        /*  Wenn Returnwert ueber Zeiger gesetzt wird, nichts noetig    */
            cc_set=0;
            if(p->q1.reg){
                move(f,0,p->q1.reg,&p->z,0,p->typf);
                if(!(p->z.flags&REG)||(p->z.reg!=p->q1.reg&&p->z.reg>=9)){ cc_set=&p->z;cc_typ=p->typf;}
            }
            continue;
        }
        if(c==CALL){
            if(gas){
                fprintf(f,"\tjbsr\t");
            }else{
                fprintf(f,"\tjsr\t");
            }
            /*  Wenn geta4() aufgerufen wurde, merken.  */
            if((p->q1.flags&(VAR|DREFOBJ))==VAR&&!strcmp(p->q1.v->identifier,"geta4")&&p->q1.v->storage_class==EXTERN)
                geta4=1;
            if((p->q1.flags&(DREFOBJ|REG))==DREFOBJ) ierror(0);
            probj2(f,&p->q1,t);
            fprintf(f,"\n");
            if(dbout) act_line=0;
            if(zl2l(p->q2.val.vlong)){
                notpopped+=zl2l(p->q2.val.vlong);
                dontpop-=zl2l(p->q2.val.vlong);
                if(!(g_flags[10]&USEDFLAG)&&!(pushedreg&30)&&stackoffset==-notpopped){
                /*  Entfernen der Parameter verzoegern  */
                }else{
                    if(dbout&&!gas){ act_line=p->line; fprintf(f,"\tdebug\t%d\n",act_line);}
                    fprintf(f,"\tadd%s.%s\t#%ld,a7\n",quick[zl2l(p->q2.val.vlong)<=8],strshort[zl2l(p->q2.val.vlong)<32768],zl2l(p->q2.val.vlong));
                    stackoffset+=zl2l(p->q2.val.vlong);
                    notpopped-=zl2l(p->q2.val.vlong);
                }
            }
            continue;
        }
        if(c==TEST){
            /*  ConditionCodes schon gesetzt?   */
            cc_set=&p->q1;cc_typ=t;
            comptyp=t;
            if(cc_set_tst&&t==cc_typ_tst){
                struct IC *branch;
                if(t&UNSIGNED){
                    branch=p->next;
                    while(branch&&(branch->code<BEQ||branch->code>BGT))
                        branch=branch->next;
                    if(!branch) ierror(0);
                    if(branch->code==BLE) branch->code=BEQ;
                    if(branch->code==BGT) branch->code=BNE;
                    if(branch->code==BGE) {branch->code=BRA;continue;}
                    if(branch->code==BLT) {branch->code=NOP;continue;}
                }
                if(compare_objects(&p->q1,cc_set_tst)&&p->q1.am==cc_set_tst->am&&zleqto(p->q1.val.vlong,cc_set_tst->val.vlong)){
                    if(DEBUG&512){fprintf(f,"; tst eliminated: cc=");probj2(f,cc_set_tst,t);
                                  fprintf(f,", q1=");probj2(f,&p->q1,t);fprintf(f,"\n");}
                    continue;
                }
            }
            if(g_flags_val[0].l<68020&&isreg(q1)&&p->q1.reg>=1&&p->q1.reg<=8){
            /*  tst ax gibt es nicht bei <68000 :-( */
                if(regavailable(1)){
                    fprintf(f,"\tmove.%c\t%s,%s\n",x_t[t&NQ],regnames[p->q1.reg],regnames[get_reg(f,1,p)]);
                }else{
                    fprintf(f,"\tcmp.w\t#0,%s\n",regnames[p->q1.reg]);
                }
                continue;
            }
            if((t==DOUBLE||t==FLOAT)&&g_flags_val[1].l<68000){
            /*  nicht sehr schoen   */
                int result=get_reg(f,1,p);
                saveregs(f,p);
                assign(f,p,&p->q1,0,PUSH,msizetab[t&NQ],t);
                if(gas){
                    fprintf(f,"\t.global\t__ieeetst%c\n\tjbsr\t__ieeetst%c\n\taddq.w\t#%ld,a7\n",x_t[t&NQ],x_t[t&NQ],msizetab[t&NQ]);
                }else{
                    fprintf(f,"\tpublic\t__ieeetst%c\n\tjsr\t__ieeetst%c\n\taddq.w\t#%ld,a7\n",x_t[t&NQ],x_t[t&NQ],msizetab[t&NQ]);
                }
                stackoffset+=msizetab[t&NQ];
                restoreregsa(f,p);
                if(result!=9) fprintf(f,"\tmove.l\td0,%s\n",regnames[result]);
                fprintf(f,"\ttst.l\t%s\n",regnames[result]);
                restoreregsd(f,p);
                continue;
            }
            if(isreg(q1)&&p->q1.reg>=17){fprintf(f,"\tftst.x\t%s\n",regnames[p->q1.reg]);continue;}
            fprintf(f,"\t%stst.%c\t",fp,x_t[t&NQ]);probj2(f,&p->q1,t);
            fprintf(f,"\n");
            continue;
        }
        if(c==ASSIGN||c==PUSH){
            if(c==ASSIGN&&compare_objects(&p->q1,&p->z)) cc_set=0;
            if(c==PUSH) dontpop+=zl2l(p->q2.val.vlong);
            assign(f,p,&p->q1,&p->z,c,zl2l(p->q2.val.vlong),t);
            continue;
        }
        if(c==ADDRESS){
            int zreg;
            if(isreg(z)&&p->z.reg>=1&&p->z.reg<=8)
                zreg=p->z.reg; else zreg=get_reg(f,0,p);
            fprintf(f,"\tlea\t");probj2(f,&p->q1,t);
            fprintf(f,",%s\n",regnames[zreg]);
            if(!isreg(z)||p->z.reg!=zreg){
                move(f,0,zreg,&p->z,0,POINTER);
            }
            continue;
        }
        if(c==COMPARE){
            int zreg;
            comptyp=t;
            if((p->q1.flags&KONST)||isreg(q2)){
            /*  evtl. Argumente von cmp und nachfolgendes bcc umdrehen  */
                struct IC *n;struct obj m;
                n=p->next;
                while(n){
                    if(n->code>=BEQ&&n->code<BRA){
                        if(!p->z.flags){
                            if(DEBUG&1) printf("arguments of cmp exchanged\n");
                            m=p->q1;p->q1=p->q2;p->q2=m;
                            p->z.flags=1;
                        }
                        /*  nachfolgenden Branch umdrehen   */
                        switch(n->code){
                            case BGT: n->code=BLT;break;
                            case BLT: n->code=BGT;break;
                            case BGE: n->code=BLE;break;
                            case BLE: n->code=BGE;break;
                        }
                        break;
                    }
                    if(n->code==FREEREG) n=n->next; else break; /*  compare ohne branch => leerer Block o.ae.   */
                }
            }
            if(t==FLOAT||t==DOUBLE){
                if(g_flags_val[1].l>=68000){
                    if(isreg(q1)&&p->q1.reg>=17){
                        zreg=p->q1.reg;
                    }else{
                        zreg=get_reg(f,2,p);
                        move(f,&p->q1,0,0,zreg,t);
                    }
                    if(isreg(q2)){fprintf(f,"\tfcmp.x\t%s,%s\n",regnames[p->q2.reg],regnames[zreg]);continue;}
                    fprintf(f,"\tfcmp.%c\t",x_t[t&NQ]);probj2(f,&p->q2,t);
                    fprintf(f,",%s\n",regnames[zreg]);
                    continue;
                }else{
                /*  nicht sehr schoen   */
                    int result=get_reg(f,1,p);
                    saveregs(f,p);
                    assign(f,p,&p->q2,0,PUSH,msizetab[t&NQ],t);
                    assign(f,p,&p->q1,0,PUSH,msizetab[t&NQ],t);
                    if(gas){
                        fprintf(f,"\t.global\t__ieeecmp%c\n\tjbsr\t__ieeecmp%c\n\tadd.w\t#%ld,a7\n",x_t[t&NQ],x_t[t&NQ],2*msizetab[t&NQ]);
                    }else{
                        fprintf(f,"\tpublic\t__ieeecmp%c\n\tjsr\t__ieeecmp%c\n\tadd.w\t#%ld,a7\n",x_t[t&NQ],x_t[t&NQ],2*msizetab[t&NQ]);
                    }
                    stackoffset+=2*msizetab[t&NQ];
                    restoreregsa(f,p);
                    if(result!=9) fprintf(f,"\tmove.l\td0,%s\n",regnames[result]);
                    fprintf(f,"\ttst.l\t%s\n",regnames[result]);
                    restoreregsd(f,p);
                    continue;
                }
            }
            if(p->q2.flags&KONST){
                fprintf(f,"\tcmp.%c\t",x_t[t&NQ]);probj2(f,&p->q2,t);
                fprintf(f,",");probj2(f,&p->q1,t);fprintf(f,"\n");
                continue;
            }
            if(isreg(q1)){
                zreg=p->q1.reg;
            }else{
                zreg=get_reg(f,1,p);    /* hier evtl. auch Adressregister nehmen */
                move(f,&p->q1,0,0,zreg,t);
            }
            fprintf(f,"\tcmp.%c\t",x_t[t&NQ]);probj2(f,&p->q2,t);
            fprintf(f,",%s\n",regnames[zreg]);
            continue;
        }
        if(c==ADDI2P||c==SUBIFP){
            int zreg,r;
            if(isreg(q1)&&p->q1.reg<=8&&isreg(z)&&p->z.reg<=8&&p->q1.reg!=p->z.reg){
            /*  q1 und z Adressregister => lea nehmen   */
                if(p->q2.flags&KONST){
                    eval_const(&p->q2.val,t);
                    if(c==SUBIFP) vlong=zlsub(l2zl(0L),vlong);
                    if(g_flags_val[0].l>=68020||(zlleq(vlong,l2zl(32767))&&zlleq(l2zl(-32768),vlong))){
                        fprintf(f,"\tlea\t(%ld,%s),%s\n",zl2l(vlong),regnames[p->q1.reg],regnames[p->z.reg]);
                        continue;
                    }
                }else if(c==ADDI2P&&isreg(q2)){
                    fprintf(f,"\tlea\t(%s,%s.%c),%s\n",regnames[p->q1.reg],regnames[p->q2.reg],x_t[t&NQ],regnames[p->z.reg]);
                    continue;
                }
            }
            if(compare_objects(&p->q1,&p->z)){
                if(p->q2.flags&KONST){
                    if(c==ADDI2P)
                        fprintf(f,"\tadd%s.l\t",quick[isquickkonst2(&p->q2.val,t)]);
                    else
                        fprintf(f,"\tsub%s.l\t",quick[isquickkonst2(&p->q2.val,t)]);
                    probj2(f,&p->q2,t);fprintf(f,",");
                    probj2(f,&p->z,POINTER);fprintf(f,"\n");
                    continue;
                }
                if(isreg(q1)&&(x_t[t&NQ]=='l'||p->q1.reg<=8)){
                    if(c==ADDI2P)
                        fprintf(f,"\tadd.%c\t",x_t[t&NQ]);
                    else
                        fprintf(f,"\tsub.%c\t",x_t[t&NQ]);
                    probj2(f,&p->q2,t);fprintf(f,",%s\n",regnames[p->z.reg]);
                    continue;
                }
                if(isreg(q2)&&p->q2.reg>=1){
                    r=p->q2.reg;
                }else{
                    r=get_reg(f,1,p);
                    move(f,&p->q2,0,0,r,t);
                }
                if(x_t[t&NQ]!='l'&&(!isreg(z)||p->z.reg<1||p->z.reg>8)){
                /*  wenn Ziel kein Adressregister, muss short erst auf long */
                /*  char darf hier nicht auftreteten und long passt schon   */
                    if(t&UNSIGNED) fprintf(f,"\tswap\t%s\n\tclr.w\t%s\n\tswap\t%s\n",regnames[r],regnames[r],regnames[r]);
                     else          fprintf(f,"\text.l\t%s\n",regnames[r]);
                    t=POINTER;
                }
/*                if(c==ADDI2P)
                    fprintf(f,"\tadd.%c\t%s,",x_t[t&NQ],regnames[r]);
                else
                    fprintf(f,"\tsub.%c\t%s,",x_t[t&NQ],regnames[r]);
                probj2(f,&p->z,t);fprintf(f,"\n");*/
                if(c==ADDI2P) add(f,0,r,&p->z,0,t);
                 else         sub(f,0,r,&p->z,0,t);
                continue;
            }
            if(isreg(z)&&p->z.reg>=1&&p->z.reg<=16)
                zreg=p->z.reg; else zreg=get_reg(f,0,p);
            /*  Spezialfall, falls Ziel Datenregister und short */
            /*  nicht schoen, aber auf die Schnelle...          */
            if(x_t[t&NQ]!='l'&&zreg>8){
                move(f,&p->q2,0,0,zreg,t);
                if(t&UNSIGNED) fprintf(f,"\tswap\t%s\n\tclr.w\t%s\n\tswap\t%s\n",regnames[zreg],regnames[zreg],regnames[zreg]);
                 else          fprintf(f,"\text.l\t%s\n",regnames[zreg]);
                if(c==SUBIFP) fprintf(f,"\tneg.l\t%s\n",regnames[zreg]);
                add(f,&p->q1,0,0,zreg,POINTER);
                if(!isreg(z)||p->z.reg!=zreg)
                    move(f,0,zreg,&p->z,0,POINTER);
                continue;
            }
            if(!isreg(q1)||p->q1.reg!=zreg){
                move(f,&p->q1,0,0,zreg,POINTER);
            }
            if(c==ADDI2P) add(f,&p->q2,0,0,zreg,t);
             else         sub(f,&p->q2,0,0,zreg,t);
            if(!isreg(z)||p->z.reg!=zreg){
                move(f,0,zreg,&p->z,0,POINTER);
            }
            continue;
        }
        if((c>=OR&&c<=AND)||(c>=LSHIFT&&c<=MOD)){
            int zreg,q1reg,q2reg;
            if((p->q2.flags&KONST)&&
               (!(p->q1.flags&REG)||!(p->z.flags&REG)||p->q1.reg!=p->z.reg)&&
               (!(p->q1.flags&VAR)||!(p->z.flags&VAR)||p->q1.v!=p->z.v)&&
               ((c>=OR&&c<=AND)||c==ADD||c==MULT)){
                struct obj o;
                if(c==MULT){
                    eval_const(&p->q2.val,t);
                    if(zlleq(l2zl(0L),vlong)&&zulleq(ul2zul(0UL),vulong)&&!pof2(vulong)){
                        o=p->q1;p->q1=p->q2;p->q2=o;
                    }
                }else{
                    o=p->q1;p->q1=p->q2;p->q2=o;
                }
            }
            if(t==FLOAT||t==DOUBLE){
                if(g_flags_val[1].l>=68000){
                    if(isreg(z)&&p->z.reg>=17) zreg=p->z.reg;
                        else zreg=get_reg(f,2,p);
                    if(!isreg(q1)||p->q1.reg!=p->z.reg)
                        move(f,&p->q1,0,0,zreg,t);
                    fprintf(f,"\tf%s.",ename[c]);
                    if(isreg(q2)) fprintf(f,"x\t");
                        else      fprintf(f,"%c\t",x_t[t&NQ]);
                    probj2(f,&p->q2,t);
                    fprintf(f,",%s\n",regnames[zreg]);
                    if(!isreg(z)||p->z.reg!=zreg){
                        move(f,0,zreg,&p->z,0,t);
                    }
                    continue;
                }else{
                    saveregs(f,p);
                    assign(f,p,&p->q2,0,PUSH,msizetab[t&NQ],t);
                    assign(f,p,&p->q1,0,PUSH,msizetab[t&NQ],t);
                    if(gas){
                        fprintf(f,"\t.global\t__ieee%s%c\n\tjbsr\t__ieee%s%c\n\tadd.w\t#%ld,a7\n",ename[c],x_t[t&NQ],ename[c],x_t[t&NQ],2*msizetab[t&NQ]);
                    }else{
                        fprintf(f,"\tpublic\t__ieee%s%c\n\tjsr\t__ieee%s%c\n\tadd.w\t#%ld,a7\n",ename[c],x_t[t&NQ],ename[c],x_t[t&NQ],2*msizetab[t&NQ]);
                    }
                    stackoffset+=2*msizetab[t&NQ];
                    restoreregsa(f,p);
                    if(t==DOUBLE){
                        fprintf(f,"\tmovem.l\td0/d1,");
                        probj2(f,&p->z,t);fprintf(f,"\n");
                    }else move(f,0,9,&p->z,0,t);
                    restoreregsd(f,p);
                    continue;
                }
            }
            if((c==MULT||c==DIV||(c==MOD&&(p->typf&UNSIGNED)))&&(p->q2.flags&KONST)){
            /*  ersetzt mul etc. mit Zweierpotenzen     */
            /*  hier evtl. noch Fehler                  */
                long ln;
                eval_const(&p->q2.val,t);
                if(zlleq(l2zl(0L),vlong)&&zulleq(ul2zul(0UL),vulong)){
                    if(ln=pof2(vulong)){
                        if(c==MOD){
                            vlong=zlsub(vlong,l2zl(1L));
                            p->code=AND;
                        }else{
                            vlong=l2zl(ln-1);
                            if(c==DIV) p->code=RSHIFT; else p->code=LSHIFT;
                        }
                        c=p->code;
                        if((t&NU)==CHAR) p->q2.val.vchar=zl2zc(vlong);
                        if((t&NU)==SHORT) p->q2.val.vshort=zl2zs(vlong);
                        if((t&NU)==INT) p->q2.val.vint=zl2zi(vlong);
                        if((t&NU)==LONG) p->q2.val.vlong=vlong;
                        vulong=zl2zul(vlong);
                        if((t&NU)==(UNSIGNED|CHAR)) p->q2.val.vuchar=zul2zuc(vulong);
                        if((t&NU)==(UNSIGNED|SHORT)) p->q2.val.vushort=zul2zus(vulong);
                        if((t&NU)==(UNSIGNED|INT))  p->q2.val.vuint=zul2zui(vulong);
                        if((t&NU)==(UNSIGNED|LONG)) p->q2.val.vulong=vulong;
                    }
                }
            }
            if(c==DIV||c==MOD){
                if(x_t[t&NQ]=='l'&&g_flags_val[0].l<68020){
                /*  das hier ist auch nicht allzu schoen  */
                    char *fname;
                    cc_set=0;   /*  Library-Funktionen setzen cc nicht immer */
                    saveregs(f,p);
                    fprintf(f,"\tmove.l\t"); probj2(f,&p->q2,t);
                    fprintf(f,",-(a7)\n");
                    stackoffset-=4;
                    fprintf(f,"\tmove.l\t"); probj2(f,&p->q1,t);
                    fprintf(f,",-(a7)\n");
                    stackoffset-=4;
                    if(c==DIV){
                        if(t&UNSIGNED) fname="divu"; else fname="divs";
                    }else{
                        if(t&UNSIGNED) fname="modu"; else fname="mods";
                    }
                    if(gas){
                        fprintf(f,"\t.global\t__l%s\n\tjbsr\t__l%s\n",fname,fname);
                    }else{
                        fprintf(f,"\tpublic\t__l%s\n\tjsr\t__l%s\n",fname,fname);
                    }
                    fprintf(f,"\taddq.w\t#8,a7\n");
                    stackoffset+=8;
                    restoreregsa(f,p);
                    move(f,0,9,&p->z,0,t);
                    restoreregsd(f,p);
                    continue;
                }

            }
            /*  hier die zweite Alternative mit isreg() schreiben?  */
            if(compare_objects(&p->q2,&p->z)){
                struct obj m;
                if((c>=OR&&c<=AND)||c==ADD||c==SUB){
                    if(c!=SUB){
                        m=p->q1;p->q1=p->q2;p->q2=m;
                    }else{
                        if(isreg(q2)&&p->q2.reg>=9&&p->q2.reg<=16){
                            m=p->q1;p->q1=p->q2;p->q2=m;
                            c=ADD;
                            fprintf(f,"\tneg.%c\t",x_t[t&NQ]);
                            probj2(f,&p->q1,t);fprintf(f,"\n");
                        }
                    }
                }
            }
            if(compare_objects(&p->q1,&p->z)){
                if((c>=OR&&c<=AND)||c==ADD||c==SUB){
                    int r;
                    if(p->q2.flags&KONST){
                        if(c==ADD) {add(f,&p->q2,0,&p->z,0,t);continue;}
                        if(c==SUB) {sub(f,&p->q2,0,&p->z,0,t);continue;}
                        fprintf(f,"\t%s.%c\t",ename[c],x_t[t&NQ]);
                        probj2(f,&p->q2,t);fprintf(f,",");
                        probj2(f,&p->z,t);fprintf(f,"\n");
                        continue;
                    }
                    if(!isreg(z)){
                        if(isreg(q2)&&p->q2.reg>=9&&p->q2.reg<=16)
                            r=p->q2.reg; else r=get_reg(f,1,p);
                        if(!isreg(q2)||p->q2.reg!=r){
                            move(f,&p->q2,0,0,r,t);
                        }
                        fprintf(f,"\t%s.%c\t%s,",ename[c],x_t[t&NQ],regnames[r]);
                        probj2(f,&p->z,t);fprintf(f,"\n");
                        continue;
                    }
                }
            }
            /*  bei xor oder asl (ausser 0<=const<=8) muss q2 in Register   */
            if(isreg(q2)&&p->q2.reg>=9&&p->q2.reg<=16){
                q2reg=p->q2.reg;
            }else{
                if(c==LSHIFT||c==RSHIFT||c==XOR){
                    eval_const(&p->q2.val,t);
                    if(c==XOR||!(p->q2.flags&KONST)||!isquickkonst2(&p->q2.val,t)){
                        q2reg=get_reg(f,1,p);
                        move(f,&p->q2,0,0,q2reg,t);
                    }else q2reg=0;
                }else{
                    q2reg=0;
                }
            }
            if(c==MOD){
                int modreg;
                modreg=get_reg(f,1,p);
                if(isreg(z)&&p->z.reg>=9&&p->z.reg<=16&&p->z.reg!=q2reg)
                    zreg=p->z.reg; else zreg=get_reg(f,1,p);
                move(f,&p->q1,0,0,modreg,t);
                if(0 /*g_flags_val[0].l==68060*/){
                /*  div?l.l wird da emuliert?   */
                    fprintf(f,"\tsmi\t%s\n\textb.l\t%s\n",regnames[zreg],regnames[zreg]);
                    if(t&UNSIGNED) fprintf(f,"\tdivu.%c\t",x_t[t&NQ]); else fprintf(f,"\tdivs.%c\t",x_t[t&NQ]);
                }else{
                    if(t&UNSIGNED) fprintf(f,"\tdivul.%c\t",x_t[t&NQ]); else fprintf(f,"\tdivsl.%c\t",x_t[t&NQ]);
                }
                probj2(f,&p->q2,t);
                fprintf(f,",%s:%s\n",regnames[zreg],regnames[modreg]);
                move(f,0,zreg,&p->z,0,t);
                cc_set=0;
                continue;
            }
            if(isreg(z)&&p->z.reg>=9&&p->z.reg<=16&&p->z.reg!=q2reg)
                zreg=p->z.reg; else zreg=get_reg(f,1,p);
            if(isreg(q1)&&p->q1.reg>=9&&p->q1.reg<=16)
                q1reg=p->q1.reg; else q1reg=0;
            if(q1reg!=zreg){
                move(f,&p->q1,0,0,zreg,t);
            }
            if(c!=MULT&&c!=DIV&&c!=MOD&&c!=ADD&&c!=SUB){
                if(c==RSHIFT&&!(t&UNSIGNED)) fprintf(f,"\tasr.%c\t",x_t[t&NQ]);
                 else fprintf(f,"\t%s.%c\t",ename[c],x_t[t&NQ]);
                if(q2reg) fprintf(f,"%s",regnames[q2reg]); else probj2(f,&p->q2,t);
                fprintf(f,",%s\n",regnames[zreg]);
            }else{
                if(c==ADD) add(f,&p->q2,q2reg,0,zreg,t);
                if(c==SUB) sub(f,&p->q2,q2reg,0,zreg,t);
                if(c==MULT||c==DIV||c==MOD) mult(f,&p->q2,q2reg,0,zreg,t,c,p);
            }
            if((!isreg(z)||p->z.reg!=zreg)){
                move(f,0,zreg,&p->z,0,t);
            }
            continue;
        }
        ierror(0);
    }
    if(notpopped){
        fprintf(f,"\tadd%s.%s\t#%ld,a7\n",quick[notpopped<=8],strshort[notpopped<32768],notpopped);
        stackoffset+=notpopped;notpopped=0;
    }
    function_bottom(f,v,zl2l(offset));
    if(pushflag){   /*  Speicher fuer pushlabel generieren - leider im cseg */
        if(gas){
            fprintf(f,"\t.lcomm\tl%d,4\n",pushlabel);
        }else{
            fprintf(f,"\tcnop\t0,4\nl%d\n\tds.b\t4\n",pushlabel);
        }
        pushflag=0;
    }
}

int shortcut(int code,int typ)
{
    if(code==COMPARE) return(1);
    return(0);
}

void cleanup_cg(FILE *f)
{
    return;
}

