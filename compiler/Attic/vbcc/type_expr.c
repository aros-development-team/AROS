/*  $VER: vbcc (type_expr.c) V0.4   */

#include "vbc.h"

static char FILE_[]=__FILE__;

int alg_opt(np),type_expression2(np);
int test_assignment(struct Typ *,np);
void make_cexpr(np);

int dontopt;

void insert_const2(union atyps *p,int t)
/*  Traegt Konstante in entprechendes Feld ein        */
{
    if(!p) ierror(0);
    t&=31;
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
void insert_const(np p)
/*  Spezialfall fuer np */
{
    if(!p||!p->ntyp) ierror(0);
    insert_const2(&p->val,p->ntyp->flags);
}
int const_typ(struct Typ *p)
/*  Testet, ob Typ konstant ist oder konstante Elemente enthaelt    */
{
    int i;struct struct_declaration *sd;
    if(p->flags&CONST) return(1);
    if(p->flags==STRUCT||p->flags==UNION)
        for(i=0;i<p->exact->count;i++)
            if(const_typ((*p->exact->sl)[i].styp)) return(1);
    return(0);
}
void eval_const(union atyps *p,int t)
/*  weist bestimmten globalen Variablen Wert einer CEXPR zu         */
{
    int f=t&15;
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
struct Typ *arith_typ(struct Typ *a,struct Typ *b)
/*  Erzeugt Typ fuer arithmetische Umwandlung von zwei Operanden    */
{
    int ta,tb;struct Typ *new;
    new=mymalloc(TYPS);
    new->next=0;
    ta=a->flags&31;tb=b->flags&31;
    if(ta==DOUBLE||tb==DOUBLE){new->flags=DOUBLE;return(new);}
    if(ta==FLOAT||tb==FLOAT){new->flags=FLOAT;return(new);}
    ta=int_erw(ta);tb=int_erw(tb);
    if(ta==(UNSIGNED|LONG)||tb==(UNSIGNED|LONG)){new->flags=UNSIGNED|LONG;return(new);}
    if((ta==LONG&&tb==(UNSIGNED|INT))||(ta==(UNSIGNED|INT)&&tb==LONG)){
        if(UINT_MAX<=LONG_MAX) new->flags=LONG; else new->flags=UNSIGNED|LONG;
        return(new);
    }
    if(ta==LONG||tb==LONG){new->flags=LONG;return(new);}
    if(ta==(UNSIGNED|INT)||tb==(UNSIGNED|INT)){new->flags=UNSIGNED|INT;return(new);}
    new->flags=INT;
    return(new);
}
int int_erw(int t)
/*  Fuehrt Integer_Erweiterung eines Typen durch                */
{
    if((t&15)>SHORT) return(t);
    if((t&31)==CHAR&&SCHAR_MAX<=INT_MAX) return(INT);
    if((t&31)==SHORT&&SHRT_MAX<=INT_MAX) return(INT);
    if((t&31)==(UNSIGNED|CHAR)&&UCHAR_MAX<=INT_MAX) return(INT);
    if((t&31)==(UNSIGNED|SHORT)&&USHRT_MAX<=INT_MAX) return(INT);
    return(UNSIGNED|INT);
}
int type_expression(np p)
/*  Art Frontend fuer type_expression2(). Setzt dontopt auf 0   */
{
    dontopt=0;
    return(type_expression2(p));
}
int type_expression2(np p)
/*  Erzeugt Typ-Strukturen fuer jeden Knoten des Baumes und     */
/*  liefert eins zurueck, wenn der Baum ok ist, sonst 0         */
/*  Die Berechnung von Konstanten und andere Vereinfachungen    */
/*  sollten vielleicht in eigene Funktion kommen                */
{
    int ok,f=p->flags,mopt=dontopt;
    if(!p){ierror(0);return(1);}
/*    if(p->ntyp) printf("Warnung: ntyp!=0\n");*/
    p->lvalue=0;
    p->sidefx=0;
    ok=1;
    if(f==CALL&&p->left->flags==IDENTIFIER&&!find_var(p->left->identifier,0)){
    /*  implizite Deklaration bei Aufruf einer Funktion     */
        struct struct_declaration *sd;struct Typ *t;
        error(161,p->left->identifier);
        sd=mymalloc(sizeof(*sd));
        sd->count=0;
        t=mymalloc(TYPS);
        t->flags=FUNKT;
        t->exact=add_sd(sd);
        t->next=mymalloc(TYPS);
        t->next->next=0;
        t->next->flags=INT;
        add_var(p->left->identifier,t,EXTERN,0);
    }
    dontopt=0;
    if(f==ADDRESS&&p->left->flags==IDENTIFIER) {p->left->flags|=256;/*puts("&const");*/}
    if(p->left&&p->flags!=ASSIGNADD){ struct struct_declaration *sd;
    /*  bei ASSIGNADD wird der linke Zweig durch den Link bewertet  */
        ok&=type_expression2(p->left); p->sidefx|=p->left->sidefx;
        if(!ok) return(0);
    }
    if(p->right&&p->right->flags!=MEMBER){ struct struct_declaration *sd;
        if(p->flags==ASSIGNADD) dontopt=1; else dontopt=0;
        ok&=type_expression2(p->right); p->sidefx|=p->right->sidefx;
        if(!ok) return(0);
    }

/*    printf("bearbeite %s\n",ename[p->flags]);*/
/*  Erzeugung von Zeigern aus Arrays                            */
/*  Hier muss noch einiges genauer werden (wie gehoert das?)    */
    if(p->left&&((p->left->ntyp->flags&15)==ARRAY||(p->left->ntyp->flags&15)==FUNKT)){
        if(f!=ADDRESS&&f!=ADDRESSA&&f!=ADDRESSS&&f!=FIRSTELEMENT&&f!=DSTRUCT&&(f<PREINC||f>POSTDEC)&&(f<ASSIGN||f>ASSIGNRSHIFT)){
            np new=mymalloc(NODES);
            if((p->left->ntyp->flags&15)==ARRAY) new->flags=ADDRESSA;
             else new->flags=ADDRESS;
            new->ntyp=0;
            new->left=p->left;
            new->right=0;new->lvalue=0;new->sidefx=0; /* sind sidefx immer 0? */
            p->left=new;
            ok&=type_expression2(p->left);
        }
    }
    if(p->right&&f!=FIRSTELEMENT&&f!=DSTRUCT&&f!=ADDRESSS&&((p->right->ntyp->flags&15)==ARRAY||(p->right->ntyp->flags&15)==FUNKT)){
            np new=mymalloc(NODES);
            if((p->right->ntyp->flags&15)==ARRAY) new->flags=ADDRESSA;
             else new->flags=ADDRESS;
            new->ntyp=0;
            new->left=p->right;
            new->right=0;new->lvalue=0;new->sidefx=0; /* sind sidefx immer 0? */
            p->right=new;
            ok&=type_expression2(p->right);
    }

    if(f==IDENTIFIER||f==(IDENTIFIER|256)){
        int ff;struct Var *v;
        v=find_var(p->identifier,0);
        if(v==0){error(82,p->identifier);return(0);}
        ff=v->vtyp->flags&15;
        if((ff>=CHAR&&ff<=DOUBLE)||ff==POINTER||ff==STRUCT||ff==UNION/*||ff==ARRAY*/) p->lvalue=1;
        p->ntyp=clone_typ(v->vtyp);
        /*  arithmetischen const Typ als Konstante behandeln, das muss noch
            deutlich anders werden, bevor man es wirklich so machen kann
        if((p->ntyp->flags&CONST)&&arith(p->ntyp->flags&15)&&v->clist&&!(f&256)){
            p->flags=CEXPR;
            p->val=v->clist->val;
            v->flags|=USEDASSOURCE;
        }*/
        p->flags&=~256;
        if((p->ntyp->flags&15)==ENUM){
        /*  enumerations auch als Konstante (int) behandeln */
            p->flags=CEXPR;
            if(!v->clist) ierror(0);
            p->val=v->clist->val;
            p->ntyp->flags=CONST|INT;
        }
        p->o.v=v;
        return(1);
    }

    if(f==CEXPR||f==PCEXPR||f==STRING) return(1);

    if(f==KOMMA){
        p->ntyp=clone_typ(p->right->ntyp);
        if(f==CEXPR) p->val=p->right->val;
        return(ok);
    }
    if(f==ASSIGN||f==ASSIGNADD){
        if(!p){ierror(0);return(0);}
        if(!p->left){ierror(0);return(0);}
        if(p->left->lvalue==0) {error(86);/*prd(p->left->ntyp);*/return(0);}
        if(const_typ(p->left->ntyp)) {error(87);return(0);}
        if(type_uncomplete(p->left->ntyp)) {error(88);return(0);}
        if(type_uncomplete(p->right->ntyp)) {error(88);return(0);}
        p->ntyp=clone_typ(p->left->ntyp);
        p->sidefx=1;
        return(test_assignment(p->left->ntyp,p->right));
    }
    if(f==LOR||f==LAND){
        int a1=-1,a2=-1,m;
        if(f==LAND) m=1; else m=0;
        p->ntyp=mymalloc(TYPS);
        p->ntyp->flags=INT;p->ntyp->next=0;
        if(!arith(p->left->ntyp->flags&15)&&(p->left->ntyp->flags&15)!=POINTER)
            {error(89);ok=0;}
        if(!arith(p->right->ntyp->flags&15)&&(p->right->ntyp->flags&15)!=POINTER)
            {error(89);ok=0;}
        if(p->left->flags==CEXPR){
            eval_constn(p->left);
            if(!zdeqto(vdouble,d2zd(0.0))||!zuleqto(vulong,ul2zul(0UL))||!zleqto(vlong,l2zl(0L))) a1=1; else a1=0;
        }
        if(p->right->flags==CEXPR){
            eval_constn(p->right);
            if(!zdeqto(vdouble,d2zd(0.0))||!zuleqto(vulong,ul2zul(0UL))||!zleqto(vlong,l2zl(0L))) a2=1; else a2=0;
        }
        if(a1==1-m||a2==1-m||(a1==m&&a2==m)){
            p->flags=CEXPR;p->sidefx=0;
            if(!p->left->sidefx) {free_expression(p->left);p->left=0;} else p->sidefx=1;
            if(!p->right->sidefx||a1==1-m) {free_expression(p->right);p->right=0;} else p->sidefx=0;
            if(a1==1-m||a2==1-m) {p->val.vint=zl2zi(l2zl((long)(1-m)));}
             else {p->val.vint=zl2zi(l2zl((long)m));}
        }
        return(ok);
    }
    if(f==OR||f==AND||f==XOR){
        if(((p->left->ntyp->flags&15)<CHAR)||((p->left->ntyp->flags&15)>LONG)){error(90);return(0);}
        if(((p->right->ntyp->flags&15)<CHAR)||((p->right->ntyp->flags&15)>LONG)){error(90);return(0);}
        p->ntyp=arith_typ(p->left->ntyp,p->right->ntyp);
        if(!mopt){
            if(!alg_opt(p)) ierror(0);
        }
        return(ok);
    }
    if(f==LESS||f==LESSEQ||f==GREATER||f==GREATEREQ||f==EQUAL||f==INEQUAL){
    /*  hier noch einige Abfragen fuer sichere Entscheidungen einbauen  */
    /*  z.B. unigned/signed-Vergleiche etc.                             */
    /*  die val.vint=0/1-Zuweisungen muessen noch an zint angepasst     */
    /*  werden                                                          */
        zlong s1,s2;zulong u1,u2;zdouble d1,d2;int c=0;
        struct Typ *t;
        if(!arith(p->left->ntyp->flags&15)||!arith(p->right->ntyp->flags&15)){
            if((p->left->ntyp->flags&15)!=POINTER||(p->right->ntyp->flags&15)!=POINTER){
                if(f!=EQUAL&&f!=INEQUAL){
                    error(92);return(0);
                }else{
                    if(((p->left->ntyp->flags&15)!=POINTER||p->right->flags!=CEXPR)&&
                       ((p->right->ntyp->flags&15)!=POINTER||p->left->flags!=CEXPR)){
                        error(93);return(0);
                    }else{
                        if(p->left->flags==CEXPR) eval_constn(p->left);
                         else                     eval_constn(p->right);
                        if(vdouble!=0||vlong!=0||vulong!=0)
                            {error(40);return(0);}
                    }
                }
            }else{
                if(compare_pointers(p->left->ntyp->next,p->right->ntyp->next,15)){
                }else{
                    if(f!=EQUAL&&f!=INEQUAL){error(41);}
                    if((p->left->ntyp->next->flags&15)!=VOID&&(p->right->ntyp->next->flags&15)!=VOID)
                        {error(41);}
                }
            }
        }
        if(p->left->flags==CEXPR){
            eval_constn(p->left);
            d1=vdouble;u1=vulong;s1=vlong;c|=1;
            if(p->right->ntyp->flags&UNSIGNED&&!(p->left->ntyp->flags&UNSIGNED)){
                if(zdleq(d1,zl2zd(l2zl(0)))&&zlleq(s1,l2zl(0))){
                    if(!zdeqto(d1,d2zd(0.0))||!zleqto(s1,l2zl(0L))){
                        error(165);
                    }else{
                        if(f==GREATER||f==LESSEQ) error(165);
                    }
                }
            }
        }
        if(p->right->flags==CEXPR){
            eval_constn(p->right);
            d2=vdouble;u2=vulong;s2=vlong;c|=2;
            if(p->left->ntyp->flags&UNSIGNED&&!(p->right->ntyp->flags&UNSIGNED)){
                if(zdleq(d2,zl2zd(l2zl(0)))&&zlleq(s2,l2zl(0))){
                    if(!zdeqto(d2,d2zd(0.0))||!zleqto(s2,l2zl(0L))){
                        error(165);
                    }else{
                        if(f==LESS||f==GREATEREQ) error(165);
                    }
                }
            }
        }
        p->ntyp=mymalloc(TYPS);
        p->ntyp->flags=INT;
        p->ntyp->next=0;
        if(c==3){
            p->flags=CEXPR;
            t=arith_typ(p->left->ntyp,p->right->ntyp);
            if(!p->left->sidefx) {free_expression(p->left);p->left=0;}
            if(!p->right->sidefx) {free_expression(p->right);p->right=0;}
            if((t->flags&15)==FLOAT||(t->flags&15)==DOUBLE){
                if(f==EQUAL) p->val.vint=zdeqto(d1,d2);
                if(f==INEQUAL) p->val.vint=!zdeqto(d1,d2);
                if(f==LESSEQ) p->val.vint=zdleq(d1,d2);
                if(f==GREATER) p->val.vint=!zdleq(d1,d2);
                if(f==LESS){
                    if(zdleq(d1,d2)&&!zdeqto(d1,d2)) p->val.vint=1;
                     else p->val.vint=0;
                }
                if(f==GREATEREQ){
                    if(!zdleq(d1,d2)||zdeqto(d1,d2)) p->val.vint=1;
                     else p->val.vint=0;
                }
            }else{
                if(t->flags&UNSIGNED){
                    if(f==EQUAL) p->val.vint=zuleqto(u1,u2);
                    if(f==INEQUAL) p->val.vint=!zuleqto(u1,u2);
                    if(f==LESSEQ) p->val.vint=zulleq(u1,u2);
                    if(f==GREATER) p->val.vint=!zulleq(u1,u2);
                    if(f==LESS){
                        if(zulleq(u1,u2)&&!zuleqto(u1,u2)) p->val.vint=1;
                         else p->val.vint=0;
                    }
                    if(f==GREATEREQ){
                        if(!zulleq(u1,u2)||zuleqto(u1,u2)) p->val.vint=1;
                         else p->val.vint=0;
                    }
                }else{
                    if(f==EQUAL) p->val.vint=zleqto(s1,s2);
                    if(f==INEQUAL) p->val.vint=!zleqto(s1,s2);
                    if(f==LESSEQ) p->val.vint=zlleq(s1,s2);
                    if(f==GREATER) p->val.vint=!zlleq(s1,s2);
                    if(f==LESS){
                        if(zlleq(s1,s2)&&!zleqto(s1,s2)) p->val.vint=1;
                         else p->val.vint=0;
                    }
                    if(f==GREATEREQ){
                        if(!zlleq(s1,s2)||zleqto(s1,s2)) p->val.vint=1;
                         else p->val.vint=0;
                    }
                }
            }
            freetyp(t);
        }
        return(ok);
    }
    if(f==ADD||f==SUB||f==MULT||f==DIV||f==MOD||f==LSHIFT||f==RSHIFT||f==PMULT){
        if(!arith(p->left->ntyp->flags&15)||!arith(p->right->ntyp->flags&15)){
            np new;zlong sz; int type=0;
            if(f!=ADD&&f!=SUB){error(94);return(0);}
            if((p->left->ntyp->flags&15)==POINTER){
                if((p->left->ntyp->next->flags&15)==VOID)
                    {error(95);return(0);}
                if((p->right->ntyp->flags&15)==POINTER){
                    if((p->right->ntyp->next->flags&15)==VOID)
                        {error(95);return(0);}
                    if(!compare_pointers(p->left->ntyp->next,p->right->ntyp->next,15))
                        {error(41);}
                    if(f!=SUB){error(96);return(0);}
                     else {type=3;}
                }else{
                    if((p->right->ntyp->flags&15)>LONG)
                        {error(97,ename[f]);return(0);}
                    if((p->right->ntyp->flags&15)<CHAR)
                        {error(97,ename[f]);return(0);}
                    if(p->right->flags!=PMULT&&p->right->flags!=PCEXPR){
                        new=mymalloc(NODES);
                        new->flags=PMULT;
                        new->ntyp=0;
                        new->left=p->right;
                        new->right=mymalloc(NODES);
                        new->right->flags=PCEXPR;
                        new->right->left=new->right->right=0;
                        new->right->ntyp=mymalloc(TYPS);
                        new->right->ntyp->flags=INT;
                        new->right->ntyp->next=0;
                        sz=szof(p->left->ntyp->next);
                        if(zleqto(l2zl(0L),sz)) error(78);
                        new->right->val.vint=zl2zi(sz);
                        p->right=new;
                        ok&=type_expression2(p->right);
                    }
                    type=1;
                }
            }else{
                np merk;
                if((p->right->ntyp->flags&15)!=POINTER)
                    {error(98);return(0);}
                if((p->right->ntyp->next->flags&15)==VOID)
                    {error(95);return(0);}
                if((p->left->ntyp->flags&15)>LONG)
                    {error(98);return(0);}
                if((p->left->ntyp->flags&15)<CHAR)
                    {error(98);return(0);}
                if(p->flags==SUB){error(99);return(0);}
                if(p->left->flags!=PMULT&&p->left->flags!=PCEXPR){
                    new=mymalloc(NODES);
                    new->flags=PMULT;
                    new->ntyp=0;
                    new->left=p->left;
                    new->right=mymalloc(NODES);
                    new->right->flags=PCEXPR;
                    new->right->left=new->right->right=0;
                    new->right->ntyp=mymalloc(TYPS);
                    new->right->ntyp->flags=INT;
                    new->right->ntyp->next=0;
                    sz=szof(p->right->ntyp->next);
                    if(zleqto(l2zl(0L),sz)) error(78);
                    new->right->val.vint=zl2zi(sz);
                    p->left=new;
                    ok&=type_expression2(p->left);
                }
                type=2;
                merk=p->left;p->left=p->right;p->right=merk;
            }
            if(type==0){ierror(0);return(0);}
            else{
                if(type==3){
                    p->ntyp=mymalloc(TYPS);
                    p->ntyp->next=0;p->ntyp->flags=INT;
                }else{
                    /*if(type==1)*/ p->ntyp=clone_typ(p->left->ntyp);
                    /* else       p->ntyp=clone_typ(p->right->ntyp);*/
                    /*  Abfrage wegen Vertauschen der Knoten unnoetig   */
                }
            }
        }else{
            p->ntyp=arith_typ(p->left->ntyp,p->right->ntyp);
            if((p->ntyp->flags&15)>LONG&&(f==MOD||f==LSHIFT||f==RSHIFT))
                {error(101);ok=0;}
            /*  Typerweiterungen fuer SHIFTS korrigieren    */
            if((f==LSHIFT||f==RSHIFT)&&(p->ntyp->flags&15)==LONG&&(p->left->ntyp->flags&15)<LONG)
                {p->ntyp->flags&=~15;p->ntyp->flags|=INT;}
        }
        /*  fuegt &a+x zusammen, noch sub und left<->right machen   */
        /*  Bei CEXPR statt PCEXPR auch machen?                     */
        if((p->flags==ADD||p->flags==SUB)){
            np m,c=0,a=0;
            if(p->left->flags==PCEXPR&&p->flags==ADD) c=p->left;
            if(p->right->flags==PCEXPR) c=p->right;
            if(p->left->flags==ADDRESS||p->left->flags==ADDRESSA||p->left->flags==ADDRESSS) a=p->left;
            if(p->right->flags==ADDRESS||p->right->flags==ADDRESSA||p->right->flags==ADDRESSS) a=p->right;
            if(c&&a){
                m=a->left;
                /*  kann man das hier so machen oder muss man da mehr testen ?  */
                while(m->flags==FIRSTELEMENT||m->flags==ADDRESS||m->flags==ADDRESSA||m->flags==ADDRESSS) m=m->left;
                if(m->flags==IDENTIFIER){
                    if(DEBUG&1) printf("&a+x with %s combined\n",ename[p->left->flags]);
                    eval_const(&c->val,c->ntyp->flags);
                    if(p->flags==ADD) m->val.vlong=zuladd(m->val.vlong,vlong);
                     else m->val.vlong=zlsub(m->val.vlong,vlong);
                    vlong=szof(m->ntyp);
                    if(!zleqto(vlong,l2zl(0L))&&zulleq(vlong,m->val.vlong)){
                        if(zuleqto(vlong,m->val.vlong))
                            error(79);
                        else
                            error(80);
                    }
                    vlong=l2zl(0L);
                    if(!zleqto(m->val.vlong,l2zl(0L))&&zulleq(m->val.vlong,vlong)) error(80);
                    free_expression(c);
                    if(p->ntyp) freetyp(p->ntyp);
                    *p=*a;
                    free(a);
                    return(type_expression2(p));
                }
            }
        }
        if(!mopt){
            if(!alg_opt(p)) ierror(0);
        }
        return(ok);
    }
    if(f==CAST){
        int from=p->left->ntyp->flags&15,to=p->ntyp->flags&15;
        if(to==VOID) return(ok);
        if(from==VOID)
            {error(102);return(0);}
        if((!arith(to)||!arith(from))&&
           (to!=POINTER||from!=POINTER)){
            if(to==POINTER){
                if(from<=LONG){
                    if(!zlleq(sizetab[from],sizetab[POINTER])){
                        error(103);return(0);
                    }
                }else{
                    error(104);return(0);
                }
            }else{
                if(from!=POINTER)
                    {error(105);return(0);}
                if(to<=LONG){
                    if(!zlleq(sizetab[POINTER],sizetab[to])){
                        error(106);return(0);
                    }
                }else{
                    error(104);return(0);
                }
            }
        }
        if(from<=LONG&&to<=LONG&&!zlleq(sizetab[from],sizetab[to])&&p->left->flags!=CEXPR) error(166);
        if(to==POINTER&&from==POINTER&&!zlleq(align[p->ntyp->next->flags&15],align[p->left->ntyp->next->flags&15]))
            error(167);
        if(p->left->flags==CEXPR){
            eval_constn(p->left);
            if((p->ntyp->flags&15)==POINTER)
                if(!zuleqto(vulong,ul2zul(0UL))||!zleqto(vlong,l2zl(0L))||!zdeqto(vdouble,d2zd(0.0)))
                    error(81);
            insert_const(p);
            p->flags=CEXPR;
            if(!p->left->sidefx) {free_expression(p->left);p->left=0;}
        }
        return(ok);
    }
    if(f==MINUS||f==KOMPLEMENT||f==NEGATION){
        if(!arith(p->left->ntyp->flags&15)){
            if(f!=NEGATION){error(107);return(0);
            }else{
                if((p->left->ntyp->flags&15)!=POINTER)
                    {error(108);return(0);}
            }
        }
        if(f==KOMPLEMENT&&(p->left->ntyp->flags&15)>LONG)
            {error(109);return(0);}
        if(f==NEGATION){
            p->ntyp=mymalloc(TYPS);
            p->ntyp->next=0;
            p->ntyp->flags=INT;
        }else{
            if(!p->left->ntyp) ierror(0);
            p->ntyp=clone_typ(p->left->ntyp);
            if((p->ntyp->flags&15)<=LONG) p->ntyp->flags=int_erw(p->ntyp->flags);
        }
        if(p->left->flags==CEXPR){
            eval_constn(p->left);
            if(f==KOMPLEMENT){vlong=zlkompl(vlong);vulong=zlkompl(vulong);
                              vint=zl2zi(vlong);vuint=zul2zui(vulong);}
            if(f==MINUS){vdouble=zdsub(d2zd(0.0),vdouble);vfloat=zd2zf(vdouble);
                         vulong=zulsub(ul2zul(0UL),vulong);vuint=zul2zui(vulong);
                         vlong=zlsub(l2zl(0L),vlong);vint=zl2zi(vlong);}
            if(f==NEGATION){if(zdeqto(vdouble,d2zd(0.0))&&zuleqto(vulong,ul2zul(0UL))&&zleqto(vlong,l2zl(0L)))
                vlong=l2zl(1L); else vlong=l2zl(0L);
                vint=zl2zi(vlong);
            }
            insert_const(p);
            p->flags=CEXPR;
            if(!p->left->sidefx&&p->left) {free_expression(p->left);p->left=0;}
        }
        return(ok);
    }
    if(f==CONTENT){
        if((p->left->ntyp->flags&15)!=POINTER)
            {error(111);return(0);}
        if((p->left->ntyp->next->flags&15)!=ARRAY&&type_uncomplete(p->left->ntyp->next))
            {error(112);return(0);}
        p->ntyp=clone_typ(p->left->ntyp->next);
        if((p->ntyp->flags&15)!=ARRAY) p->lvalue=1;
        if(p->left->flags==ADDRESS&&zuleqto(p->left->val.vulong,ul2zul(0UL))){
        /*  *&x durch x ersetzen                                */
            np merk;
            merk=p->left;
            if(p->ntyp) freetyp(p->ntyp);
            if(p->left->ntyp) freetyp(p->left->ntyp);
            *p=*p->left->left;
            free(merk->left);
            free(merk);
            return(ok);
        }
        /*  *&ax durch firstelement-of(x) ersetzen  */
        if(p->left->flags==ADDRESSA||p->left->flags==ADDRESSS){
            np merk;
            if(DEBUG&1) printf("substitutet * and %s with FIRSTELEMENT\n",ename[p->left->flags]);
            p->flags=FIRSTELEMENT;
            p->lvalue=1;    /*  evtl. hier erst Abfrage ?   */
            merk=p->left;
            p->left=merk->left;
            p->right=merk->right;
            if(merk->ntyp) freetyp(merk->ntyp);
            free(merk);
        }
        return(ok);
    }
    if(f==FIRSTELEMENT){
/*        if((p->left->ntyp->flags&15)!=ARRAY)
            {ierror(0);return(0);}*/
        if((p->left->ntyp->flags&15)==ARRAY) p->ntyp=clone_typ(p->left->ntyp->next);
         else{
            int i,n=-1;
            for(i=0;i<p->left->ntyp->exact->count;i++)
                if(!strcmp((*p->left->ntyp->exact->sl)[i].identifier,p->right->identifier)) n=i;
            if(n<0){ierror(0);return(0);}
            p->ntyp=clone_typ((*p->left->ntyp->exact->sl)[n].styp);
        }
        p->lvalue=1;    /*  hier noch genauer testen ?  */
        return(ok);
    }
    if(f==ADDRESS){
        if((p->left->ntyp->flags&15)!=FUNKT&&(p->left->ntyp->flags&15)!=ARRAY){
            if(!p->left->lvalue){error(115);return(0);}
            if(p->left->flags==IDENTIFIER){
                struct Var *v;
                v=find_var(p->left->identifier,0);
                if(!v){error(116,p->left->identifier);return(0);}
                if(v->storage_class==REGISTER)
                    {error(117);return(0);}
            }
        }
        p->ntyp=mymalloc(TYPS);
        p->ntyp->flags=POINTER;
        p->ntyp->next=clone_typ(p->left->ntyp);
        return(ok);
    }
    if(f==ADDRESSA){
        p->ntyp=clone_typ(p->left->ntyp);
        p->ntyp->flags=POINTER;
        return(ok);
    }
    if(f==ADDRESSS){
        int i,n=-1;
        for(i=0;i<p->left->ntyp->exact->count;i++)
            if(!strcmp((*p->left->ntyp->exact->sl)[i].identifier,p->right->identifier)) n=i;
        if(n<0){ierror(0);return(0);}
        p->ntyp=mymalloc(TYPS);
        p->ntyp->flags=POINTER;
        if(!p->left->ntyp) ierror(0);
        if(!p->left->ntyp->exact) ierror(0);
        if(!(*p->left->ntyp->exact->sl)[n].styp) ierror(0);
        p->ntyp->next=clone_typ((*p->left->ntyp->exact->sl)[n].styp);
        return(ok);
    }
    if(f==DSTRUCT){
    /*  hier kann man bei unions einiges schneller/einfacher machen */
        int i=0,f;struct Typ *t,*h;np new;zlong offset=l2zl(0L),al;
        if((p->left->ntyp->flags&15)!=STRUCT&&(p->left->ntyp->flags&15)!=UNION)
            {error(8);return(0);}
        if(type_uncomplete(p->left->ntyp)){error(11);return(0);}
        if(p->right->flags!=MEMBER)
            {ierror(0);return(0);}
        while(i<p->left->ntyp->exact->count&&strcmp((*p->left->ntyp->exact->sl)[i].identifier,p->right->identifier)){
            t=(*p->left->ntyp->exact->sl)[i].styp;
            h=t;
            do{
                f=h->flags&15;
                h=h->next;
            }while(f==ARRAY);
            al=align[f];
            offset=zlmult(zldiv(zladd(offset,zlsub(al,l2zl(1L))),al),al);
            offset=zladd(offset,szof(t));
            i++;
        }
        if(i>=p->left->ntyp->exact->count) {error(23,p->right->identifier);return(0);}

        t=(*p->left->ntyp->exact->sl)[i].styp;
        h=t;
        do{
            f=h->flags&15;
            h=h->next;
        }while(f==ARRAY);
        al=align[f];
        offset=zlmult(zldiv(zladd(offset,zlsub(al,l2zl(1L))),al),al);
        if((p->left->ntyp->flags&15)==UNION) offset=l2zl(0L);
        p->flags=CONTENT;if(p->ntyp) {freetyp(p->ntyp);p->ntyp=0;}
        new=mymalloc(NODES);
        new->flags=ADD;
        new->ntyp=0;
        new->right=mymalloc(NODES);
        new->right->left=new->right->right=0;
        new->right->flags=PCEXPR;
        new->right->ntyp=mymalloc(TYPS);
        new->right->ntyp->flags=LONG;
        new->right->ntyp->next=0;
        new->right->val.vlong=offset;
        new->left=mymalloc(NODES);
        new->left->flags=ADDRESSS;
        new->left->left=p->left;
        new->left->right=p->right;
        new->left->ntyp=0;
        p->left=new;p->right=0;

        return(type_expression2(p));
    }
    if(f==PREINC||f==POSTINC||f==PREDEC||f==POSTDEC){
        if(!p->left->lvalue){error(86);return(0);}
        if(p->left->ntyp->flags&CONST){error(87);return(0);}
        if(!arith(p->left->ntyp->flags&15)){
            if((p->left->ntyp->flags&15)!=POINTER){
                error(24);return(0);
            }else{
                if((p->left->ntyp->next->flags&15)==VOID)
                    {error(95);return(0);}
            }
        }
        p->ntyp=clone_typ(p->left->ntyp);
        p->sidefx=1;
        return(ok);
    }
    if(f==CALL){
        struct argument_list *al;int i,flags=0;char *s=0;
        struct struct_declaration *sd;
        al=p->alist;
        if((p->left->ntyp->flags&15)!=POINTER||(p->left->ntyp->next->flags&15)!=FUNKT)
            {error(26);return(0);}
        if(ok&&p->left->left->flags==IDENTIFIER&&p->left->left->o.v->storage_class==EXTERN){
            s=p->left->left->o.v->identifier;
            flags=p->left->left->o.v->flags;
        }
        sd=p->left->ntyp->next->exact;
        if(!sd){ierror(0);return(0);}
        if(sd->count==0){
            error(162);
            if(s){
                if(!strcmp(s,"printf")||!strcmp(s,"fprintf")||!strcmp(s,"sprintf")||
                   !strcmp(s,"scanf")|| !strcmp(s,"fscanf")|| !strcmp(s,"sscanf"))
                    error(213);
            }
        }
        while(al){
            if(!al->arg) ierror(0);
            if(!type_expression2(al->arg)) return(0);
            al->arg=makepointer(al->arg);
            if(type_uncomplete(al->arg->ntyp)) error(39);
            al=al->next;
        }
        p->sidefx=1;
        p->ntyp=clone_typ(p->left->ntyp->next->next);
        i=0;al=p->alist;
        while(al){
            if(i>=sd->count) return(ok);
            if(!(*sd->sl)[i].styp) return(ok); /* nur Bezeichner, aber kein Typ im Prototype */
            if(!test_assignment((*sd->sl)[i].styp,al->arg)) return(0);
            if(i==sd->count-1&&(flags&(PRINTFLIKE|SCANFLIKE))){
                if(al->arg->left&&al->arg->left->flags==STRING){
                /*  Argumente anhand des Formatstrings ueberpruefen */
                    struct const_list *cl=(struct const_list *)al->arg->left->identifier;
                    int fused=0;
                    al=al->next;
                    while(cl&&cl->other){
                        int c,fflags=' ',at;
                        struct Typ *t;
                        c=(int)zul2zl(zuc2zul(cl->other->val.vchar));
                        cl=cl->next;
                        if(c==0){
                            if(cl) error(215);
                            break;
                        }
                        if(c!='%') continue;
                        if(!cl){error(214);return(ok);}
                        c=(int)zul2zl(zuc2zul(cl->other->val.vchar));
                        cl=cl->next;
                        while(isdigit((unsigned char)c)||
                              c=='-'||c=='+'||c==' '||c=='#'||c=='.'||
                              c=='h'||c=='l'||c=='L'||c=='*'){
                            fused|=3;
                            if(fflags!='*'&&(c=='h'||c=='l'||c=='L'||c=='*'))
                                fflags=c;
                            c=(int)zul2zl(zuc2zul(cl->other->val.vchar));
                            cl=cl->next;
                            if(!cl){error(214);return(ok);}
                        }
                        if(DEBUG&1) printf("format=%c%c\n",fflags,c);
                        if(c=='%') continue;
                        if(fflags=='*') continue;
                        if(!al){error(214);return(ok);}
                        t=al->arg->ntyp;
                        if(DEBUG&1){ prd(stdout,t);printf("\n");}
                        if((flags&SCANFLIKE)){
                            if((t->flags&15)!=POINTER){error(214);return(ok);}
                            t=t->next;
                        }
                        at=t->flags&31;
                        if(flags&PRINTFLIKE){
                            switch(c){
                            case 'o':
                            case 'x':
                            case 'X':
                            case 'c':
                                at&=15;     /*  fall through    */
                            case 'i':
                            case 'd':
                                fused|=1;
                                if(at==LONG&&fflags!='l'){error(214);return(ok);}
                                if(fflags=='l'&&at!=LONG){error(214);return(ok);}
                                if(at<CHAR||at>LONG){error(214);return(ok);}
                                break;
                            case 'u':
                                fused=1;
                                if(al->arg->flags==CEXPR) at|=UNSIGNED;
                                if(at==(UNSIGNED|LONG)&&fflags!='l'){error(214);return(ok);}
                                if(fflags=='l'&&at!=(UNSIGNED|LONG)){error(214);return(ok);}
                                if(at<(UNSIGNED|CHAR)||at>(UNSIGNED|LONG)){error(214);return(ok);}
                                break;
                            case 's':
                                fused|=1;
                                if(at!=POINTER||(t->next->flags&15)!=CHAR){error(214);return(ok);}
                                break;
                            case 'f':
                            case 'e':
                            case 'E':
                            case 'g':
                            case 'G':
                                fused|=7;
                                if(at!=FLOAT&&at!=DOUBLE){error(214);return(ok);}
                                break;
                            case 'p':
                                fused|=3;
                                if(at!=POINTER||(t->next->flags)!=VOID){error(214);return(ok);}
                                break;
                            case 'n':
                                fused|=3;
                                if(at!=POINTER){error(214);return(ok);}
                                at=t->next->flags&31;
                                if(fflags=='h'&&at!=SHORT){error(214);return(ok);}
                                if(fflags==' '&&at!=INT){error(214);return(ok);}
                                if(fflags=='l'&&at!=LONG){error(214);return(ok);}
                                break;
                            default:
                                error(214);return(ok);
                            }
                        }else{
                            switch(c){
                            case '[':
                                fused|=3;
                                do{
                                    c=(int)zul2zl(zuc2zul(cl->other->val.vchar));
                                    cl=cl->next;
                                    if(!cl){error(214);return(ok);}
                                }while(c!=']');     /*  fall through    */
                            case 's':
                            case 'c':
                                fused|=1;
                                if((at&15)!=CHAR){error(214);return(ok);}
                                break;
                            case 'n':
                                fused|=3;       /*  fall through    */
                            case 'd':
                            case 'i':
                            case 'o':
                            case 'x':
                                fused|=1;
                                if(fflags=='h'&&at!=SHORT){error(214);return(ok);}
                                if(fflags==' '&&at!=INT){error(214);return(ok);}
                                if(fflags=='l'&&at!=LONG){error(214);return(ok);}
                                break;
                            case 'u':
                                fused|=1;
                                if(fflags=='h'&&at!=(UNSIGNED|SHORT)){error(214);return(ok);}
                                if(fflags==' '&&at!=(UNSIGNED|INT)){error(214);return(ok);}
                                if(fflags=='l'&&at!=(UNSIGNED|LONG)){error(214);return(ok);}
                                break;
                            case 'e':
                            case 'f':
                            case 'g':
                                fused|=7;
                                if(fflags==' '&&at!=FLOAT){error(214);return(ok);}
                                if(fflags=='l'&&at!=DOUBLE){error(214);return(ok);}
                                if(fflags=='L'&&at!=DOUBLE){error(214);return(ok);}
                                break;
                            case 'p':
                                fused|=3;
                                if(at!=VOID){error(214);return(ok);}
                                break;
                            default:
                                error(214);return(ok);
                            }
                        }
                        al=al->next;
                    }
                    if(al){ error(214);return(ok);} /* zu viele */
                    if(DEBUG&1) printf("fused=%d\n",fused);
                    if(fused!=7&&s){
                    /*  Wenn kein Format benutzt wird, kann man printf, */
                    /*  scanf etc. durch aehnliches ersetzen.           */
                        struct Var *v;char repl[MAXI+6]="__v";
                        if(fused==3) fused=2;
                        repl[3]=fused+'0';repl[4]=0;
                        strcat(repl,s);
                        if(DEBUG&1) printf("repl=%s\n",repl);
                        while(fused<=2){
                            v=find_var(repl,0);
                            if(v&&v->storage_class==EXTERN){
                                p->left->left->o.v=v;
                                break;
                            }
                            fused++;repl[3]++;
                        }
                    }
                    return(ok);
                }
            }
            i++;al=al->next;
        }
        if(i>=sd->count) return(ok);
        if((*sd->sl)[i].styp&&((*sd->sl)[i].styp->flags&15)!=VOID){error(83);/*printf("sd->count=%d\n",sd->count);*/}
        return(ok);
    }
    if(f==COND){
        if(!arith(p->left->ntyp->flags&15)&&(p->left->ntyp->flags&15)!=POINTER){
            error(29);
            return(0);
        }
        if(p->left->flags==CEXPR&&!p->left->sidefx){
            int null;np merk;
            if(DEBUG&1) printf("constant conditional-expression simplified\n");
            eval_constn(p->left);
            if(zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))&&zdeqto(vdouble,d2zd(0.0))) null=1; else null=0;
            free_expression(p->left);
            merk=p->right;
            if(null){
                free_expression(p->right->left);
                *p=*p->right->right;
            }else{
                free_expression(p->right->right);
                *p=*p->right->left;
            }
            if(merk->ntyp) freetyp(merk->ntyp);
            free(merk);
            return(1);
        }
        p->ntyp=clone_typ(p->right->ntyp);
        return(1);
    }
    if(f==COLON){
        /*  Hier fehlt noch korrekte Behandlung der Typattribute    */
        if(arith(p->left->ntyp->flags&15)&&arith(p->right->ntyp->flags&15)){
            p->ntyp=arith_typ(p->left->ntyp,p->right->ntyp);
            return(1);
        }
        if(compare_pointers(p->left->ntyp,p->right->ntyp,15)){
            p->ntyp=clone_typ(p->left->ntyp);
            return(1);
        }
        if((p->left->ntyp->flags&15)==POINTER&&(p->right->ntyp->flags&15)==POINTER){
            if((p->left->ntyp->next->flags&15)==VOID){
                p->ntyp=clone_typ(p->left->ntyp);
                return(1);
            }
            if((p->right->ntyp->next->flags&15)==VOID){
                p->ntyp=clone_typ(p->right->ntyp);
                return(1);
            }
        }
        if((p->left->ntyp->flags&15)==POINTER&&p->right->flags==CEXPR){
            eval_constn(p->right);
            if(zleqto(vlong,l2zl(0L))&&zuleqto(ul2zul(0UL),vulong)&&zdeqto(d2zd(0.0),vdouble)){
                p->ntyp=clone_typ(p->left->ntyp);
                return(1);
            }
        }
        if((p->right->ntyp->flags&15)==POINTER&&p->left->flags==CEXPR){
            eval_constn(p->left);
            if(zleqto(l2zl(0L),vlong)&&zuleqto(ul2zul(0UL),vulong)&&zdeqto(d2zd(0.0),vdouble)){
                p->ntyp=clone_typ(p->right->ntyp);
                return(1);
            }
        }
        error(31);
        return(0);
    }
    if(f) printf("type_testing fuer diesen Operator (%d) noch nicht implementiert\n",f);
    return(0);
}

np makepointer(np p)
/*  Fuehrt automatische Zeigererzeugung fuer Baumwurzel durch   */
/*  Durch mehrmaligen Aufruf von type_expression() ineffizient  */
{
    struct struct_declaration *sd;
    if((p->ntyp->flags&15)==ARRAY||(p->ntyp->flags&15)==FUNKT){
        np new=mymalloc(NODES);
        if((p->ntyp->flags&15)==ARRAY){
            new->flags=ADDRESSA;
            new->ntyp=clone_typ(p->ntyp);
            new->ntyp->flags=POINTER;
/*            new->ntyp=0;*/
        }else{
            new->flags=ADDRESS;
            new->ntyp=mymalloc(TYPS);
            new->ntyp->flags=POINTER;
            new->ntyp->next=clone_typ(p->ntyp);
/*            new->ntyp=0;*/
        }
        new->left=p;
        new->right=0;
        new->lvalue=0;  /* immer korrekt ?  */
        new->sidefx=p->sidefx;
/*        type_expression(new);*/
        return(new);
    }else return(p);
}
int alg_opt(np p)
/*  fuehrt algebraische Vereinfachungen durch                   */
/*  hier noch genau testen, was ANSI-gemaess erlaubt ist etc.   */
/*  v.a. Floating-Point ist evtl. kritisch                      */
{
    int c=0,f,komm,null1,null2,eins1,eins2;np merk;
    zdouble d1,d2;zulong u1,u2;zlong s1,s2;
    f=p->flags;
    /*  kommutativ? */
    if(f==ADD||f==MULT||f==PMULT||(f>=OR&&f<=AND)) komm=1; else komm=0;
    /*  Berechnet Wert, wenn beides Konstanten sind     */
    if(p->left->flags==CEXPR||p->left->flags==PCEXPR){
        eval_constn(p->left);
        d1=vdouble;u1=vulong;s1=vlong;c|=1;
    }
    if(p->right->flags==CEXPR||p->right->flags==PCEXPR){
        eval_constn(p->right);
        d2=vdouble;u2=vulong;s2=vlong;c|=2;
    }
    if(c==3){
        p->flags=CEXPR;
        if(DEBUG&1) printf("did simple constant folding\n");
        if(!p->left->sidefx) {free_expression(p->left);p->left=0;}
        if(!p->right->sidefx) {free_expression(p->right);p->right=0;}
        if(f==AND){
            vulong=zuland(u1,u2);
            vlong=zland(s1,s2);
        }
        if(f==OR){
            vulong=zulor(u1,u2);
            vlong=zlor(s1,s2);
        }
        if(f==XOR){
            vulong=zulxor(u1,u2);
            vlong=zlxor(s1,s2);
        }
        if(f==ADD){
            vulong=zuladd(u1,u2);
            vlong=zladd(s1,s2);
            vdouble=zdadd(d1,d2);
        }
        if(f==SUB){
            vulong=zulsub(u1,u2);
            vlong=zlsub(s1,s2);
            vdouble=zdsub(d1,d2);
        }
        if(f==MULT||f==PMULT){
            vulong=zulmult(u1,u2);
            vlong=zlmult(s1,s2);
            vdouble=zdmult(d1,d2);
            if(f==PMULT) p->flags=PCEXPR;
        }
        if(f==DIV){
            if(zleqto(l2zl(0L),s2)&&zuleqto(ul2zul(0UL),u2)&&zdeqto(d2zd(0.0),d2)){
                error(84);
                vlong=l2zl(0L);vulong=zl2zul(vlong);vdouble=zl2zd(vlong);
            }else{
                if(!zuleqto(ul2zul(0UL),u2)) vulong=zuldiv(u1,u2);
                if(!zleqto(l2zl(0L),s2)) vlong=zldiv(s1,s2);
                if(!zdeqto(d2zd(0.0),d2)) vdouble=zddiv(d1,d2);
            }
        }
        if(f==MOD){
            if(zleqto(l2zl(0L),s2)&&zuleqto(ul2zul(0UL),u2)){
                error(84);
                vlong=l2zl(0L);vulong=zl2zul(vlong);
            }else{
                if(!zuleqto(ul2zul(0UL),u2)) vulong=zulmod(u1,u2);
                if(!zleqto(l2zl(0L),s2)) vlong=zlmod(s1,s2);
            }
        }
        if(f==LSHIFT){
            vulong=zullshift(u1,u2);
            vlong=zllshift(s1,s2);
        }
        if(f==RSHIFT){
            vulong=zulrshift(u1,u2);
            vlong=zlrshift(s1,s2);
        }
        vuint=zul2zui(vulong);vint=zl2zi(vlong);vfloat=zd2zf(vdouble);
        insert_const(p);
        return(1);
    }
    /*  Konstanten nach rechts, wenn moeglich       */
    if(c==1&&komm){
        if(DEBUG&1) printf("exchanged commutative constant operand\n");
        merk=p->left;p->left=p->right;p->right=merk;
        c=2;
        d2=d1;u2=u1;s2=s1;
    }
    /*  Vertauscht die Knoten, um Konstanten                */
    /*  besser zusammenzufassen (bei allen Type erlaubt?)   */
    /*  Hier muss noch einiges kontrolliert werden          */
    if(komm&&c==2&&p->flags==p->left->flags){
        if(p->left->right->flags==CEXPR||p->left->right->flags==PCEXPR){
            np merk;
            merk=p->right;p->right=p->left->left;p->left->left=merk;
            if(DEBUG&1) printf("Vertausche Add-Nodes\n");
            return(type_expression(p));
        }
    }
    if(zdeqto(d2zd(0.0),d1)&&zuleqto(ul2zul(0UL),u1)&&zleqto(l2zl(0L),s1)) null1=1; else null1=0;
    if(zdeqto(d2zd(0.0),d2)&&zuleqto(ul2zul(0UL),u2)&&zleqto(l2zl(0L),s2)) null2=1; else null2=0;
    eins1=eins2=0;
    vlong=l2zl(1L);vulong=zl2zul(vlong);vdouble=zl2zd(vlong);
    if(zdeqto(d1,vdouble)&&zuleqto(u1,vulong)&&zleqto(s1,vlong)) eins1=1;
    if(zdeqto(d2,vdouble)&&zuleqto(u2,vulong)&&zleqto(s2,vlong)) eins2=1;
    if(!(p->ntyp->flags&UNSIGNED)){
        vlong=l2zl(-1L);vdouble=zl2zd(vlong);
        if(zdeqto(d1,vdouble)&&zleqto(s1,vlong)) eins1=-1;
        if(zdeqto(d2,vdouble)&&zleqto(s2,vlong)) eins2=-1;
    }
    if(c==2){
        /*  a+0=a-0=a^0=a>>0=a<<0=a*1=a/1=a   */
        if(((eins2==1&&(f==MULT||f==PMULT||f==DIV))||(null2&&(f==ADD||f==SUB||f==OR||f==XOR||f==LSHIFT||f==RSHIFT)))&&!p->right->sidefx){
            if(DEBUG&1){if(f==MULT||f==PMULT||f==DIV) printf("a*/1->a\n"); else printf("a+-^0->a\n");}
            free_expression(p->right);
            merk=p->left;
            *p=*p->left;
/*            freetyp(merk->ntyp);  das war Fehler  */
            free(merk);
            return(type_expression(p));
        }
        /*  a*0=0   */
        if(null2&&(f==MULT||f==PMULT||f==AND||f==DIV||f==MOD)){
            if(DEBUG&1) printf("a*&/%%0->0\n");
            if(null2&&(f==DIV||f==MOD)) error(84);
            if(p->flags==PMULT) p->flags=PCEXPR; else p->flags=CEXPR;
            /*  hier nur int,long,float,double moeglich, hoffe ich  */
            vlong=l2zl(0L);vulong=zl2zul(vlong);
            vint=zl2zi(vlong);vuint=zul2zui(vulong);
            vdouble=zl2zd(vlong);vfloat=zd2zf(vdouble);
            insert_const(p);
            if(!p->left->sidefx){free_expression(p->left);p->left=0;} else make_cexpr(p->left);
            if(!p->right->sidefx){free_expression(p->right);p->right=0;} else make_cexpr(p->right);
/*            return(type_expression(p));   */
            return(1);
        }
        if(eins2==-1&&(f==MULT||f==PMULT||f==DIV)&&!p->right->sidefx){
            if(DEBUG&1) printf("a*/(-1)->-a\n");
            free_expression(p->right);
            p->right=0;
            p->flags=MINUS;
            return(type_expression(p));
        }
    }
    if(c==1){
        /*  0-a=-a  */
        if(((f==DIV&&eins1==-1)||(f==SUB&&null1))&&!p->left->sidefx){
            if(DEBUG&1){if(f==DIV) printf("-1/a->-a\n"); else printf("0-a->-a\n");}
            free_expression(p->left);
            p->flags=MINUS;
            p->left=p->right;
            p->right=0;
            return(type_expression(p));
        }
        /*  0/a=0   */
        if(null1&&(f==DIV||f==MOD||f==LSHIFT||f==RSHIFT)){
            if(DEBUG&1) printf("0/%%<<>>a->0\n");
            p->flags=CEXPR;
            /*  fier nur int,long,float,double moeglich, hoffe ich  */
            vlong=l2zl(0L);vulong=zl2zul(vlong);
            vint=zl2zi(vlong);vuint=zul2zui(vulong);
            vdouble=zl2zd(vlong);vfloat=zd2zf(vdouble);
            insert_const(p);
            if(!p->left->sidefx){free_expression(p->left);p->left=0;}else make_cexpr(p->left);
            if(!p->right->sidefx){free_expression(p->right);p->right=0;} else make_cexpr(p->right);
            return(type_expression(p));
        }
    }
    return(1);
}
void make_cexpr(np p)
/*  Macht aus einem Knoten, der durch constant-folding ueberfluessig    */
/*  wurde, eine PCEXPR, sofern er keine Nebenwirkungen von sich aus     */
/*  erzeugt. Hier noch ueberpruefen, ob CEXPR besser waere.             */
/*  Fuehrt rekursiven Abstieg durch. Ist das so korrekt?                */
{
    int f=p->flags;
    if(f!=ASSIGN&&f!=ASSIGNADD&&f!=CALL&&f!=POSTINC&&f!=POSTDEC&&f!=PREINC&&f!=PREDEC){
        p->flags=PCEXPR;
        if(p->left) make_cexpr(p->left);
        if(p->right) make_cexpr(p->right);
    }
}
int test_assignment(struct Typ *zt,np q)
/*  testet, ob q an Typ z zugewiesen werden darf    */
{
    struct Typ *qt=q->ntyp;
    if(arith(zt->flags&15)&&arith(qt->flags&15)){
        if((zt->flags&15)<=LONG&&(qt->flags&15)<=LONG&&
           !zlleq(sizetab[qt->flags&15],sizetab[zt->flags&15])&&q->flags!=CEXPR)
            error(166);
        return(1);
    }
    if(((zt->flags&15)==STRUCT&&(qt->flags&15)==STRUCT)||
       ((zt->flags&15)==UNION &&(qt->flags&15)==UNION )){
        if(!compare_pointers(zt,qt,255)){
            error(38);return(0);}
        else return(1);
    }
    if((zt->flags&15)==POINTER&&(qt->flags&15)==POINTER){
        if((zt->next->flags&15)==VOID&&(qt->next->flags&15)!=FUNKT) return(1);
        if((qt->next->flags&15)==VOID&&(qt->next->flags&15)!=FUNKT) return(1);
        if(!compare_pointers(zt->next,qt->next,(c_flags[7]&USEDFLAG)?31:15)){
            error(85);
        }else{
            if((qt->next->flags&CONST)&&!(zt->next->flags&CONST))
                error(91);
            if((qt->next->flags&VOLATILE)&&!(zt->next->flags&VOLATILE))
                error(100);
            if(qt->next->next&&zt->next->next&&!compare_pointers(zt->next->next,qt->next->next,255))
                error(110);
        }
        return(1);
    }
    if((zt->flags&15)==POINTER&&q->flags==CEXPR){
        eval_constn(q);
        if(!(zdeqto(d2zd(0.0),vdouble)&&zleqto(l2zl(0L),vlong)&&zuleqto(ul2zul(0UL),vulong)))
            error(113);
        return(1);
    }
    error(39);return(0);
}

