#include "supp.h"

static char FILE_[]=__FILE__;

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

char *empty="";
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

int regs[MAXR+1],regused[MAXR+1];
struct Var *regsv[MAXR+1];
int goto_used;
int ic_count;
zlong max_offset;
int function_calls;
int multiple_ccs;
int lastlabel,return_label;
int only_inline;
struct IC *err_ic;
long maxoptpasses=10;
long optflags;
long inline_size=100;
long unroll_size=200;
long fp_assoc,noaliasopt;
int fline;
char errfname[FILENAME_MAX+1];
struct IC *first_ic,*last_ic;
int float_used;
/*  Das haette ich gern woanders    */
struct Var *vl1,*vl2,*vl3;


struct Typ *clone_typ(struct Typ *old)
/*  Erzeugt Kopie eines Typs und liefert Zeiger auf Kopie   */
{
    struct Typ *new;
    if(!old) return(0);
    new=mymalloc(TYPS);
    *new=*old;
    if(new->next) new->next=clone_typ(new->next);
    return(new);
}
void free_IC(struct IC *p)
/*  Gibt IC-Liste inkl. Typen frei                  */
{
    struct IC *merk;
    if(DEBUG&1) printf("free_IC()\n");
    while(p){
        if(p->q1.am) free(p->q1.am);
        if(p->q2.am) free(p->q2.am);
        if(p->z.am) free(p->z.am);
        merk=p->next;
        free(p);
        p=merk;
    }
}
void remove_IC(struct IC *p)
/*  entfernt IC p aus Liste */
{
    if(p->prev) p->prev->next=p->next; else first_ic=p->next;
    if(p->next) p->next->prev=p->prev; else last_ic=p->prev;
    if(p->q1.am) free(p->q1.am);
    if(p->q2.am) free(p->q2.am);
    if(p->z.am) free(p->z.am);
    free(p);
}
void freetyp(struct Typ *p)
/* Gibt eine Typ-Liste frei, aber keine struct_declaration oder so */
{
    int f;struct Typ *merk;
    if(DEBUG&8){printf("freetyp: ");prd(stdout,p);printf("\n");}
    while(p){
        merk=p->next;
        f=p->flags&NQ;
        if(merk&&f!=ARRAY&&f!=POINTER&&f!=FUNKT){ierror(0);return;}
        free(p);
        p=merk;
    }
}
zlong falign(struct Typ *t)
/*  Liefert Alignment eines Typs. Funktioniert im Gegensatz zum  */
/*  align[]-Array auch mit zusammengesetzten Typen.              */
{
  int i,f; zlong al,alt;
  f=t->flags&NQ;
  al=align[f];
  if(f<=POINTER) return al;
  if(f==ARRAY){
    do{ 
      t=t->next; 
      f=t->flags&NQ;
    }while(f==ARRAY);
    alt=falign(t);
    if(zlleq(al,alt)) return alt; else return al;
  }
  if(f==UNION||f==STRUCT){
    for(i=0;i<t->exact->count;i++){
      alt=falign((*t->exact->sl)[i].styp);
      if(!zlleq(alt,al)) al=alt;
    }
    return al;
  }
  return al;
}
zlong szof(struct Typ *t)
/*  Liefert die benoetigte Groesse eines Typs in Bytes      */
{
    int i=t->flags&NQ,j;zlong size,m;
    if(i<=POINTER) return sizetab[i];
    if(i==ARRAY) return(zlmult((t->size),szof(t->next)));
    if(i==UNION){
        for(j=0,size=l2zl(0L);j<t->exact->count;j++){
            m=szof((*t->exact->sl)[j].styp);
            if(zleqto(m,l2zl(0L))) return(l2zl(0L));
            if(!zlleq(m,size)) size=m;
        }
	m=falign(t);
        return zlmult(zldiv(zladd(size,zlsub(m,l2zl(1L))),m),m); /* align */
    }
    if(i==STRUCT){
        for(j=0,size=0;j<t->exact->count;j++){
            struct Typ *h=(*t->exact->sl)[j].styp;
	    m=falign(h);
            size=zlmult(zldiv(zladd(size,zlsub(m,l2zl(1L))),m),m);
	    m=szof(h);
            if(zleqto(m,l2zl(0L))) return(l2zl(0L));
            size=zladd(size,m);
        }
	m=falign(t);
        return zlmult(zldiv(zladd(size,zlsub(m,l2zl(1L))),m),m); /* align */
    }
    return sizetab[i];
}
void printval(FILE *f,union atyps *p,int t,int verbose)
/*  Gibt atyps aus                                      */
{
    if(t==CHAR){if(verbose)fprintf(f,"C");vlong=zc2zl(p->vchar);printzl(f,vlong);}
    if(t==(UNSIGNED|CHAR)){if(verbose)fprintf(f,"UC");vulong=zuc2zul(p->vuchar);printzul(f,vulong);}
    if(t==SHORT){if(verbose)fprintf(f,"S");vlong=zs2zl(p->vshort);printzl(f,vlong);}
    if(t==(UNSIGNED|SHORT)){if(verbose) fprintf(f,"US");vulong=zus2zul(p->vushort);printzul(f,vulong);}
    if(t==FLOAT){if(verbose)fprintf(f,"F");vdouble=zf2zd(p->vfloat);printzd(f,vdouble);}
    if(t==DOUBLE){if(verbose)fprintf(f,"D");printzd(f,p->vdouble);}
    if(t==INT){if(verbose)fprintf(f,"I");vlong=zi2zl(p->vint);printzl(f,vlong);}
    if(t==LONG){if(verbose)fprintf(f,"L");printzl(f,p->vlong);}
    if(t==(UNSIGNED|INT)){if(verbose)fprintf(f,"UI");vulong=zui2zul(p->vuint);printzul(f,vulong);}
    if(t==(UNSIGNED|LONG)){if(verbose)fprintf(f,"UL");printzul(f,p->vulong);}
    /*  das hier ist nicht wirklich portabel    */
    if(t==POINTER){if(verbose)fprintf(f,"P");vulong=zp2zul(p->vpointer);printzul(f,vulong);}
}
void pric2(FILE *f,struct IC *p)
/*  Gibt ein IC aus */
{
    if(p->next&&p->next->prev!=p) ierror(0);
    if(p->code>=LABEL&&p->code<=BRA){
        if(p->code==LABEL)
            fprintf(f,"L%d",p->typf);
        else{
            fprintf(f,"\t%s L%d",ename[p->code],p->typf);
            if(p->q1.flags){ fprintf(f,",");probj(f,&p->q1,0);}
        }
    }else{
        fprintf(f,"\t%s ",ename[p->code]);
        if(p->typf&UNSIGNED) fprintf(f,"unsigned ");
        if(p->typf) fprintf(f,"%s ",typname[p->typf&NQ]);
        probj(f,&p->q1,p->typf);
        if(p->q2.flags){fprintf(f,",");probj(f,&p->q2,p->typf);}
        if(p->z.flags){fprintf(f,"->");probj(f,&p->z,p->typf);}
        if(p->code==ASSIGN||p->code==PUSH||p->code==POP) fprintf(f," size=%ld",zl2l(p->q2.val.vlong));
        if((p->code==SAVEREGS||p->code==RESTOREREGS)&&p->q1.reg) fprintf(f," except %s",regnames[p->q1.reg]);
    }
    fprintf(f,"\n");
}
void pric(FILE *f,struct IC *p)
/*  Gibt IC-Liste auf dem Bildschirm aus                */
{
    while(p){
        pric2(f,p);
/*        if(p->q1.am||p->q2.am||p->z.am) ierror(0);*/
        p=p->next;
    }
}
void printzl(FILE *f,zlong x)
/*  Konvertiert zlong nach ASCII                        */
/*  basiert noch einigermassen auf                      */
/*  Zweierkomplementdarstellung (d.h. -MIN>MAX)         */
/*  Ausserdem muss max(abs(long))<=max(unsigned long)   */
{
    zlong zl;zulong zul;
    zl=l2zl(0L);
    if(zlleq(x,zl)&&!zleqto(x,l2zl(0L))){
        fprintf(f,"-");zl=zul2zl(t_max[LONG]);
        if(zlleq(x,zlsub(l2zl(0L),zl))&&!zleqto(x,zlsub(l2zl(0L),zl))){
        /*  aufpassen, da -x evtl. >LONG_MAX    */
            zul=t_max[LONG];
            x=zladd(x,zl);
        } else zul=ul2zul(0UL);
        x=zlsub(l2zl(0L),x);
        vulong=zl2zul(x);
        zul=zuladd(zul,vulong);
    }else zul=zl2zul(x);
    printzul(f,zul);
}
void printzul(FILE *f,zulong x)
/*  Konvertiert zulong nach ASCII                       */
{
    zulong zul;unsigned long l;
    if(DEBUG&64) printf("printzul:%lu\n",zul2ul(x));
    zul=ul2zul(10UL);
    if(!zuleqto(zuldiv(x,zul),ul2zul(0UL))) printzul(f,zuldiv(x,zul));
    zul=zulmod(x,zul);l=zul2ul(zul);
    fprintf(f,"%c",(int)(l+'0'));
}
void printzd(FILE *f,zdouble x)
/*  Konvertiert zdouble nach ASCII, noch nicht fertig   */
{
    fprintf(f,"fp-constant");
}
void *mymalloc(size_t size)
/*  Belegt Speicher mit Abfrage     */
{
    void *p;static int safe;
    /*  Um ein Fehlschlagen bei size==0 zu vermeiden; nicht sehr schoen,    */
    /*  aber das einfachste...                                              */
    if(size==0) size=1;
    if(!(p=malloc(size))){
        error(12);
        raus();
    }
    return(p);
}
void probj(FILE *f,struct obj *p,int t)
/*  Gibt Objekt auf Bildschirm aus                      */
{
    if(p->am) ierror(0);
    if(p->flags&DREFOBJ) fprintf(f,"(");
    if(p->flags&VARADR) fprintf(f,"#");
    if(p->flags&VAR) {
        printval(f,&p->val,LONG,1);
        if(p->v->storage_class==AUTO||p->v->storage_class==REGISTER){
            if(p->flags&REG)
                fprintf(f,"+%s",regnames[p->reg]);
            else
                fprintf(f,"+%ld(FP)", zl2l(p->v->offset));
        }else{
            if(p->v->storage_class==STATIC){
                fprintf(f,"+L%ld",zl2l(p->v->offset));
            }else{
                fprintf(f,"+_%s",p->v->identifier);
            }
        }
        fprintf(f,"(%s)",p->v->identifier);
        if(p->v->reg) fprintf(f,":%s",regnames[p->v->reg]);
    }
    if((p->flags&REG)&&!(p->flags&VAR)) fprintf(f,"%s",regnames[p->reg]);
    if(p->flags&KONST){
        fprintf(f,"#");printval(f,&p->val,t&NU,1);
    }
    if(p->flags&DREFOBJ) fprintf(f,")");
}
void prl(FILE *o,struct struct_declaration *p)
/* Gibt eine struct_declaration auf dem Bildschirm aus */
{
    int i;
    for(i=0;i<p->count;i++) {fprintf(o," %d.:",i); prd(o,(*p->sl)[i].styp);}
}
void prd(FILE *o,struct Typ *p)
/* Gibt einen Typ auf dem Bildschirm aus    */
{
    int f;
    if(!p) {fprintf(o,"empty type ");return;}
    f=p->flags;
/*    fprintf(o,"(Sizeof=%ld,flags=%d)",zl2l(szof(p)),f);*/
/*    if(type_uncomplete(p)) {fprintf(o,"incomplete ");}*/
    if(f&CONST) {fprintf(o,"const ");f&=~CONST;}
    if(f&STRINGCONST) {fprintf(o,"string-const ");f&=~STRINGCONST;}
    if(f&VOLATILE) {fprintf(o,"volatile ");f&=~VOLATILE;}
    if(f&UNSIGNED) {fprintf(o,"unsigned ");f&=~UNSIGNED;}
    if(f==FUNKT) {fprintf(o,"function with parameters (");
                  prl(o,p->exact);
                  fprintf(o,") returning ");prd(o,p->next);return;}
    if(f==STRUCT){fprintf(o,"struct with components {");
                  prl(o,p->exact);fprintf(o,"} ");
                  return;
    }
    if(f==UNION) {fprintf(o,"union with components {");
                  prl(o,p->exact);fprintf(o,"} ");
                  return;
    }
    if(f==POINTER) {fprintf(o,"pointer to ");prd(o,p->next);return;}
    if(f==ARRAY) {fprintf(o,"array [size %ld] of ",zl2l(p->size));prd(o,p->next);return;}
    fprintf(o,"%s",typname[f]);
}


void insert_const2(union atyps *p,int t)
/*  Traegt Konstante in entprechendes Feld ein        */
{
    if(!p) ierror(0);
    t&=NU;
    if(t==CHAR) {p->vchar=vchar;return;}
    if(t==SHORT) {p->vshort=vshort;return;}
    if(t==INT) {p->vint=vint;return;}
    if(t==LONG) {p->vlong=vlong;return;}
    if(t==(UNSIGNED|CHAR)) {p->vuchar=vuchar;return;}
    if(t==(UNSIGNED|SHORT)) {p->vushort=vushort;return;}
    if(t==(UNSIGNED|INT)) {p->vuint=vuint;return;}
    if(t==(UNSIGNED|LONG)) {p->vulong=vulong;return;}
    if(t==FLOAT) {p->vfloat=vfloat;return;}
    if(t==DOUBLE) {p->vdouble=vdouble;return;}
    if(t==POINTER) {p->vpointer=vpointer;return;}
}
void eval_const(union atyps *p,int t)
/*  weist bestimmten globalen Variablen Wert einer CEXPR zu         */
{
    int f=t&NQ;
    if(!p) ierror(0);
    if(f>=CHAR&&f<=LONG){
        if(!(t&UNSIGNED)){
            if(f==CHAR) vlong=zc2zl(p->vchar);
            if(f==SHORT)vlong=zs2zl(p->vshort);
            if(f==INT)  vlong=zi2zl(p->vint);
            if(f==LONG) vlong=p->vlong;
            vulong=zl2zul(vlong);
            vdouble=zl2zd(vlong);
        }else{
            if(f==CHAR) vulong=zuc2zul(p->vuchar);
            if(f==SHORT)vulong=zus2zul(p->vushort);
            if(f==INT)  vulong=zui2zul(p->vuint);
            if(f==LONG) vulong=p->vulong;
            vlong=zul2zl(vulong);
            vdouble=zul2zd(vulong);
        }
        vpointer=zul2zp(vulong);
    }else{
        if(f==POINTER){
            vulong=zp2zul(p->vpointer);
            vlong=zul2zl(vulong);vdouble=zul2zd(vulong);
        }else{
            if(f==FLOAT) vdouble=zf2zd(p->vfloat); else vdouble=p->vdouble;
            vlong=zd2zl(vdouble);
            vulong=zl2zul(vlong);
        }
    }
    vfloat=zd2zf(vdouble);
    vuchar=zul2zuc(vulong);
    vushort=zul2zus(vulong);
    vuint=zul2zui(vulong);
    vchar=zl2zc(vlong);
    vshort=zl2zs(vlong);
    vint=zl2zi(vlong);
}


