/*  $VER: vbcc (declaration.c) V0.4     */

#include "vbc.h"

static char FILE_[]=__FILE__;

#define PARAMETER 8
#define OLDSTYLE 16

struct const_list *initialization(struct Typ *,int);
int test_assignment(struct Typ *,np);
int return_sc,has_return;

extern int float_used;
extern void optimize(long,struct Var *);

int settyp(int typnew, int typold)
/* Unterroutine fuer declaration_specifiers()               */
{
    static int warned_long_double;
    if(DEBUG&2) printf("settyp: new=%d old=%d\n",typnew,typold);
    if(typold==LONG&&typnew==FLOAT){ error(203); return(DOUBLE);}
    if(typold==LONG&&typnew==DOUBLE){
        if(!warned_long_double){error(204);warned_long_double=1;}
        return(DOUBLE);
    }
    if(typold!=0&&typnew!=INT){error(47);return(typnew);}
    if(typold==0&&typnew==INT) return(INT);
    if(typold==0) return(typnew);
    if(typold==SHORT||typold==LONG) return(typold);
    error(48);
    return(typnew);
}

#define dsc if(storage_class) error(49); if(typ||type_qualifiers) error(50)
#define XSIGNED 16384

struct Typ *declaration_specifiers(void)
/* Erzeugt neuen Typ und gibt Zeiger darauf zurueck,      */
/* parst z.B. unsigned int, struct bla etc.               */
/* evtl. muessen noch storage-classes uns strengere       */
/* Pruefungen etc. eingebaut werden                       */
{
    int typ=0,type_qualifiers=0,notdone,storage_class;
    char *merk,*imerk,sident[MAXI],sbuff[MAXI];
    struct Typ *new=mymalloc(TYPS),*t,*ts;
    struct struct_declaration *ssd;
    struct struct_list (*sl)[];
    size_t slsz;
    struct Var *v;
    storage_class=0;
    new->next=0; new->exact=0;
    do{
        merk=s;killsp();cpbez(buff,0);notdone=0;
        if(DEBUG&2) printf("ts: %s\n",buff);
        if(!strcmp("struct",buff)) notdone=STRUCT;
        if(!strcmp("union",buff)) notdone=UNION;
        if(notdone!=0){
            killsp();
            if(*s!='{'){
                cpbez(sident,1);
                killsp();
                ssd=find_struct(sident,0);
                if(ssd&&*s=='{'&&find_struct(sident,nesting)&&ssd->count>0) error(13,sident);
                if(!ssd||((*s=='{'||*s==';')&&!find_struct(sident,nesting))){
                    typ=settyp(notdone,typ);
                    ssd=mymalloc(sizeof(*ssd));
                    ssd->count=0;
                    new->exact=ssd=add_sd(ssd);
                    add_struct_identifier(sident,ssd);
                }else{
                    new->exact=ssd;
                    typ=settyp(new->flags=notdone,typ);
                }
            }else{
                *sident=0;
                typ=settyp(notdone,typ);
                ssd=mymalloc(sizeof(*ssd));
                ssd->count=0;
                new->exact=ssd=add_sd(ssd);
            }
            if(*s=='{'){
                s++;
                killsp();
                slsz=SLSIZE;
                sl=mymalloc(slsz*sizeof(struct struct_list));
                ssd->count=0;
                imerk=ident;
                ts=declaration_specifiers();
                while(*s!='}'&&ts){
                    ident=sbuff;
                    t=declarator(clone_typ(ts));
                    killsp();
                    if(*s==':'){
                    /*  bitfields werden hier noch ignoriert    */
                        np tree;
                        if((ts->flags&15)!=INT) error(51);
                        s++;killsp();tree=assignment_expression();
                        if(type_expression(tree)){
                            if(tree->flags!=CEXPR) error(52);
                            if((tree->ntyp->flags&15)<CHAR||(tree->ntyp->flags&15)>LONG) error(52);
                        }
                        if(tree) free_expression(tree);
                    }else{
                        if(*ident==0) error(53);
                    }
                    if(type_uncomplete(t)){
                        error(14,sbuff);
                        freetyp(t);
                        break;
                    }
                    if((t->flags&15)==FUNKT)
                        error(15,sbuff);

                    if(*ident!=0){
                        int i=ssd->count;
                        while(--i>=0)
                            if(!strcmp((*sl)[i].identifier,ident))
                                error(16,ident);
                    }
                    (*sl)[ssd->count].styp=t;
                    (*sl)[ssd->count].identifier=add_identifier(ident,strlen(ident));
                    ssd->count++;
                    if(ssd->count>=slsz-1){
                        slsz+=SLSIZE;
                        sl=realloc(sl,slsz*sizeof(struct struct_list));
                        if(!sl){error(12);raus();}
                    }
                    killsp();
                    if(*s==',') {s++;killsp();continue;}
                    if(*s!=';') error(54); else s++;
                    killsp();
                    if(*s!='}'){
                        if(ts) freetyp(ts);
                        ts=declaration_specifiers();killsp();
                    }
                }
                if(ts) freetyp(ts);
                if(ssd->count==0) error(55);
                ident=imerk;
                add_sl(ssd,sl);
                free(sl);
                if(*s!='}') error(56); else s++;
                new->flags=notdone|type_qualifiers;
            }
            notdone=1;
        }
        if(!strcmp("enum",buff)){
        /*  enumerations; die Namen werden leider noch ignoriert    */
            killsp();notdone=1;
            if(*s!='{'){cpbez(buff,1);killsp();}
            if(*s=='{'){
                zlong val; struct Var *v; struct Typ *t;
                val=l2zl(0L);
                s++;killsp();
                while(*s!='}'){
                    cpbez(sident,1);killsp();
                    t=mymalloc(TYPS);
                    t->flags=CONST|INT;
                    t->next=0;
                    if(find_var(sident,nesting)) error(17,sident);
                    v=add_var(sident,t,AUTO,0); /*  AUTO hier klug? */
                    if(*s=='='){
                        s++;killsp();
                        v->clist=initialization(v->vtyp,0);
                        val=zi2zl(v->clist->val.vint);killsp();
                    }else{
                        v->clist=mymalloc(CLS);
                        v->clist->val.vint=val;
                        v->clist->next=v->clist->other=0;
                        v->clist->tree=0;
                    }
                    vlong=l2zl(1L);val=zladd(val,vlong);
                    v->vtyp->flags=CONST|ENUM;
                    if(*s=='}') break;
                    if(*s==',') s++; else error(57);
                    killsp();
                }
                s++;
            }
            killsp();
            typ=settyp(INT,typ);*buff=0;
        }
        if(!strcmp("void",buff)) {typ=settyp(VOID,typ);notdone=1;}
        if(!strcmp("char",buff)) {typ=settyp(CHAR,typ);notdone=1;}
        if(!strcmp("short",buff)) {typ=settyp(SHORT,typ);notdone=1;}
        if(!strcmp("int",buff)) {typ=settyp(INT,typ);notdone=1;}
        if(!strcmp("long",buff)) {typ=settyp(LONG,typ);notdone=1;}
        if(!strcmp("float",buff)) {typ=settyp(FLOAT,typ);notdone=1;}
        if(!strcmp("double",buff)) {typ=settyp(DOUBLE,typ);notdone=1;}
        if(!strcmp("const",buff)){
            if(type_qualifiers&CONST) error(58);
            type_qualifiers|=CONST;notdone=1;
        }
        if(!strcmp("volatile",buff)){
            if(type_qualifiers&VOLATILE) error(58);
            type_qualifiers|=VOLATILE;notdone=1;
        }
        if(!strcmp("unsigned",buff)){
            if(type_qualifiers&(XSIGNED|UNSIGNED)) error(58);
            notdone=1;type_qualifiers|=UNSIGNED;
        }
        if(!strcmp("signed",buff)){
            if(type_qualifiers&(XSIGNED|UNSIGNED)) error(58);
            notdone=1;type_qualifiers|=XSIGNED;
        }
        if(!strcmp("auto",buff)) {dsc;storage_class=AUTO;notdone=1;}
        if(!strcmp("register",buff)){dsc;storage_class=REGISTER;notdone=1;}
        if(!strcmp("static",buff)) {dsc;storage_class=STATIC;notdone=1;}
        if(!strcmp("extern",buff)) {dsc;storage_class=EXTERN;notdone=1;}
        if(!strcmp("typedef",buff)) {dsc;storage_class=TYPEDEF;notdone=1;}

        if(!notdone&&*buff&&typ==0&&!(type_qualifiers&(XSIGNED|UNSIGNED))){
            v=find_var(buff,0);
            if(v&&v->storage_class==TYPEDEF){
                free(new);
                new=clone_typ(v->vtyp);
                typ=settyp(new->flags,typ);
                notdone=1;
            }
        }
        if(DEBUG&2) printf("typ:%d\n",typ);
        killsp();
    }while(notdone);
    s=merk;
    return_sc=storage_class;
    if(typ==0){
        if(storage_class==0&&type_qualifiers==0) {free(new);return(0);}
        typ=INT;
    }
    if(type_qualifiers&(XSIGNED|UNSIGNED))
        if(typ!=INT&&typ!=CHAR&&typ!=LONG&&typ!=SHORT)
            error(58);
    if(DEBUG&2) printf("ts finish:%s\n",s);
    new->flags=typ|type_qualifiers;
    return(new);
}


struct Typ *declarator(struct Typ *a)
/* Erzeugt einen neuen Typ, auf Basis des Typs a            */
/* a wird hiermit verkettet                                 */
{
    struct Typ *t;
    killsp();*ident=0;
    t=direct_declarator(pointer(a));
    if(!a) {if(t) freetyp(t);return(0);} else return(t);
}
struct Typ *pointer(struct Typ *a)
/* Unterroutine fuer declaration(), behandelt Zeiger auf Typ    */
/* Kann sein, dass die Assoziativitaet nicht stimmt */
{
    struct Typ *t;char *merk;int notdone;
    if(!a) return(0);
    killsp();
    while(*s=='*'){
        s++;
        t=mymalloc(TYPS);
        t->flags=POINTER;
        t->next=a;
        a=t;
        do{
            killsp();
            merk=s;cpbez(buff,0);
            notdone=0;
            if(!strcmp("const",buff)) {a->flags|=CONST;notdone=1;}
            if(!strcmp("volatile",buff)) {a->flags|=VOLATILE;notdone=1;}
        }while(notdone);
        s=merk;
    }
    return(a);
}

struct Typ *direct_declarator(struct Typ *a)
/*  Unterroutine zu declarator()                    */
/* behandelt [],(funkt),(dekl)                      */
/* Funktionesufrufe und Arrays noch unvollstaendig  */
/* implementiert, auch Assoziativitaet zweifelhaft  */
{
    struct Typ *rek=0,*merk,*p,*t,*first,*last=0;
    struct struct_declaration *fsd;
    struct struct_list (*sl)[];
    size_t slsz;
    char *imerk,fbuff[MAXI];
    killsp();
    if(!isalpha((unsigned char)*s)&&*s!='_'&&*s!='('&&*s!='[') return(a);
    if(isalpha((unsigned char)*s)||*s=='_'){
        cpbez(ident,1);
        if(!a) return(0);
    }else if(*s=='('&&a){
        /* Rekursion */
        imerk=s; s++; killsp();
        if(*s!=')'&&*ident==0&&!declaration(0)){
            merk=a;
            rek=declarator(a);
            if(*s!=')') error(59); else s++;
        }else s=imerk;
    }
    if(!a)return(0);
    killsp();
    while(*s=='['||*s=='('){
        if(*s=='['){
            s++;
            killsp();
            p=mymalloc(TYPS);
            p->flags=ARRAY;
            p->next=0;
            if(*s==']'){
                p->size=l2zl(0L);
            }else{
                np tree;
                tree=expression();
                if(!type_expression(tree)){
/*                    error("incorrect constant expression");*/
                }else{
                    if(tree->sidefx) error(60);
                    if(tree->flags!=CEXPR||(tree->ntyp->flags&15)<CHAR||(tree->ntyp->flags&15)>LONG){
                        error(19);
                    }else{
                        eval_constn(tree);
                        p->size=vlong;
                        if(zleqto(p->size,l2zl(0L))) {error(61);p->size=l2zl(1L);}
                    }
                }
                free_expression(tree);
            }
            if(*s!=']') error(62); else s++;
            if(last){
                last->next=p;
                last=p;
            }else{
                first=last=p;
            }
        }
        if(*s=='('){
            int komma;
            /* Identifier- oder Parameter-list noch nicht komplett */
            /* z.B. ... oder ohne Parameter                        */
            s++;
            killsp();
            fsd=mymalloc(sizeof(*fsd));
            slsz=SLSIZE;
            sl=mymalloc(sizeof(struct struct_list)*slsz);
            fsd->count=0;
            imerk=ident;komma=0;
            enter_block();
            while(*s!=')'&&*s!='.'){
                ident=fbuff;*fbuff=0;komma=0;
                t=declarator(declaration_specifiers());
                if(!t&&*ident==0) {error(20);
                                   break;}
                if(fsd->count){
                    if((t&&!(*sl)[fsd->count-1].styp)||
                       (!t&&(*sl)[fsd->count-1].styp))
                        error(63);
                }
                if(!return_sc) return_sc=AUTO;
                if(return_sc!=AUTO&&return_sc!=REGISTER)
                    {error(21);return_sc=AUTO;}
                (*sl)[fsd->count].styp=t;
                (*sl)[fsd->count].storage_class=return_sc;
                (*sl)[fsd->count].identifier=add_identifier(ident,strlen(ident));
                if(t){
                    if(((*sl)[fsd->count].styp->flags&15)==VOID&&fsd->count!=0)
                        error(22);
                    /*  Arrays in Zeiger umwandeln  */
                    if(((*sl)[fsd->count].styp->flags&15)==ARRAY)
                        (*sl)[fsd->count].styp->flags=POINTER;
                    /*  Funktionen in Zeiger auf Funktionen umwandeln   */
                    if(((*sl)[fsd->count].styp->flags&15)==FUNKT){
                        struct Typ *new;
                        new=mymalloc(TYPS);
                        new->flags=POINTER;
                        new->next=(*sl)[fsd->count].styp;
                        (*sl)[fsd->count].styp=new;
                    }

                }
                fsd->count++;
                if(fsd->count>=slsz-2){     /*  eins Reserve fuer VOID  */
                    slsz+=SLSIZE;
                    sl=realloc(sl,slsz*sizeof(struct struct_list));
                    if(!sl){error(12);raus();}
                }
                killsp(); /* Hier Syntaxpruefung strenger machen */
                if(*s==',') {s++;komma=1; killsp();}
            }
            ident=imerk;
            if((*s!='.'||*(s+1)!='.'||*(s+2)!='.')||!komma){
                if(fsd->count>0&&(!(*sl)[fsd->count-1].styp||((*sl)[fsd->count-1].styp->flags&15)!=VOID)){
                    (*sl)[fsd->count].styp=mymalloc(TYPS);
                    (*sl)[fsd->count].styp->flags=VOID;
                    (*sl)[fsd->count].styp->next=0;
                    (*sl)[fsd->count].identifier=empty;
                    fsd->count++;
                }
            }else if(komma) {s+=3;komma=0;}
            p=mymalloc(TYPS);
            p->flags=FUNKT;
            p->next=0;
            {
                int m=nesting;
                nesting=0;
                p->exact=add_sd(fsd);
                add_sl(fsd,sl);
                free(sl);
                nesting=m;
            }
            killsp();
            if(*s!=')'||komma) error(59); else s++;
            killsp();
            if(*s==','||*s==';'||*s==')'||*s=='=') leave_block();
            if(last){
                last->next=p;
                last=p;
            }else{
                first=last=p;
            }
        }
        killsp();
    }
    if(last){last->next=a;last=a;a=first;}
    if(rek!=0&&rek!=merk){
        /* Zweite Liste anhaengen */
        p=rek;
        while(p->next!=merk) p=p->next;
        if(p) p->next=a; else ierror(0);
        return(rek);
    }
    return(a);
}
int declaration(int offset)
/*  Testet, ob eine Typangabe kommt. Wenn offset!=0 ist,    */
/*  muss s auf '(' zeigen und es wird getestet, ob nach der */
/*  Klammer eine Typangabe kommt.                           */
/*  In jedem Fall zeigt s danach wieder auf dieselbe Stelle */
/*  im Source.                                              */
{
    char *merk=s,buff[MAXI];
    struct Var *v;
    if(offset){
        s++;
        read_new_line=0;
        if(DEBUG&1) printf("cleared read_new_line\n");
        killsp();
        if(read_new_line){  /*  es kam eine neue Zeile  */
            memmove(s+1,s,MAXINPUT);
            *s='(';
            if(DEBUG&1) printf("look-ahead: %s|\n",s);
            merk=s;
            s++;
            cpbez(buff,0);
        }else{
            if(DEBUG&1) printf("read_new_line unchanged\n");
            cpbez(buff,0);
        }
    }else{
        cpbez(buff,0);
    }
    s=merk;
    if(!strcmp("auto",buff)) return(1);
    if(!strcmp("char",buff)) return(1);
    if(!strcmp("const",buff)) return(1);
    if(!strcmp("double",buff)) return(1);
    if(!strcmp("enum",buff)) return(1);
    if(!strcmp("extern",buff)) return(1);
    if(!strcmp("float",buff)) return(1);
    if(!strcmp("int",buff)) return(1);
    if(!strcmp("long",buff)) return(1);
    if(!strcmp("register",buff)) return(1);
    if(!strcmp("short",buff)) return(1);
    if(!strcmp("signed",buff)) return(1);
    if(!strcmp("static",buff)) return(1);
    if(!strcmp("struct",buff)) return(1);
    if(!strcmp("typedef",buff)) return(1);
    if(!strcmp("union",buff)) return(1);
    if(!strcmp("unsigned",buff)) return(1);
    if(!strcmp("void",buff)) return(1);
    if(!strcmp("volatile",buff)) return(1);
    v=find_var(buff,0);
    if(v&&v->storage_class==TYPEDEF) return(1);
    return(0);
}
void add_sl(struct struct_declaration *sd,struct struct_list (*sl)[])
/*  Fuegt ein struct_list-Array in eine struct_declaration ein.     */
/*  Das Array muss mind. sd->count Elements haben und wird kopiert. */
{
    size_t sz=sizeof(struct struct_list)*sd->count;
    sd->sl=mymalloc(sz);
    memcpy(sd->sl,sl,sz);
}
struct struct_declaration *add_sd(struct struct_declaration *new)
/*  Fuegt eine struct Declaration in Liste ein      */
{
    new->next=0;
    if(first_sd[nesting]==0){
        first_sd[nesting]=last_sd[nesting]=new;
    }else{
        last_sd[nesting]->next=new;
        last_sd[nesting]=new;
    }
    return(new);
}
void free_sd(struct struct_declaration *p)
/*  Gibt eine struct_declaration-List inkl. struct_lists und    */
/*  allen Typen jeder struct_list frei, nicht aber identifier   */
{
    int i;struct struct_declaration *merk;
    while(p){
        merk=p->next;
        for(i=0;i<p->count;i++) if((*p->sl)[i].styp) freetyp((*p->sl)[i].styp);
        if(p->count>0) free(p->sl);
        free(p);
        p=merk;
    }
}
char *add_identifier(char *identifier,int length)
/*  Kopiert identifier an sicheren Ort, der spaeter zentral     */
/*  freigegeben werden kann.                                    */
/*  Sollte noch einbauen, dass ueberprueft wird, ob schon       */
/*  vorhanden und dann nicht zweimal speichern                  */
{
    struct identifier_list *new;
    if((*identifier==0&&length==0)||identifier==empty) return(empty);
    new=mymalloc(sizeof(struct identifier_list));
    new->identifier=mymalloc(length+1);
    memcpy(new->identifier,identifier,length+1);
    new->next=0;new->length=length;
    if(last_ilist[nesting]){
        last_ilist[nesting]->next=new;
        last_ilist[nesting]=new;
    }else{
        last_ilist[nesting]=first_ilist[nesting]=new;
    }
    return(new->identifier);
}
void free_ilist(struct identifier_list *p)
/*  Gibt eine verkettete identifier_liste und saemtliche darin  */
/*  gespeicherten Identifier frei.                              */
{
    struct identifier_list *merk;
    while(p){
        merk=p->next;
        if(p->identifier) free(p->identifier);
        free(p);
        p=merk;
    }
}
int type_uncomplete(struct Typ *p)
/*  Testet, ob Typ unvollstaendig ist. Momentan gelten nur      */
/*  unvollstaendige Strukturen und Arrays von solchen als       */
/*  unvollstaendig, aber keine Zeiger oder Funktionen darauf    */
{
    struct struct_declaration *sd;
    if(!p){ierror(0);return(0);}
    if((p->flags&15)==STRUCT||(p->flags&15)==UNION)
        if(p->exact->count<=0) return(1);
    if((p->flags&15)==ARRAY){
        if(zlleq(p->size,l2zl(0L))) return(1);
        if(type_uncomplete(p->next)) return(1);
    }
    return(0);
}
void add_struct_identifier(char *identifier,struct struct_declaration *sd)
/*  Erzeugt neuen struct_identifier, fuegt ihn in Liste an und  */
/*  vervollstaendigt unvollstaendige Typen dieser Struktur.     */
{
    struct struct_identifier *new;
/*    struct Typ *t;*/
    if(DEBUG&1) printf("add_si %s (nesting=%d)->%p\n",identifier,nesting,(void *)sd);
    new=mymalloc(sizeof(struct struct_identifier));
    new->identifier=add_identifier(identifier,strlen(identifier));
    new->sd=sd; new->next=0;
    if(first_si[nesting]==0){
        first_si[nesting]=new;last_si[nesting]=new;
    }else{
        last_si[nesting]->next=new;last_si[nesting]=new;
    }
}
void free_si(struct struct_identifier *p)
/*  Gibt eine struct_identifier-Liste frei, aber nicht die      */
/*  identifiers und struct_declarations                         */
{
    struct struct_identifier *merk;
    while(p){
        merk=p->next;
        free(p);
        p=merk;
    }
}
struct struct_declaration *find_struct(char *identifier,int endnesting)
/*  Sucht angegebene Strukturdefinition und liefert             */
/*  entsprechende struct_declaration                            */
{
    struct struct_identifier *si; int i;
    for(i=nesting;i>=endnesting;i--){
        si=first_si[i];
        while(si){
            if(!strcmp(si->identifier,identifier)){
                if(DEBUG&1) printf("found struct tag <%s> at nesting %d->%p\n",identifier,i,(void *)si->sd);
                return(si->sd);
            }
            si=si->next;
        }
    }
    if(DEBUG&1) printf("didn't find struct tag <%s>\n",identifier);
    return(0);
}
struct Var *add_var(char *identifier, struct Typ *t, int storage_class,struct const_list *clist)
/*  Fuegt eine Variable mit Typ in die var_list ein             */
/*  maschinenspezifisches und Codegeneration fehlen noch        */
/*  Alignment maschinenabhaengig                                */
/*  In der storage_class werden die Flags PARAMETER und evtl.   */
/*  OLDSTYLE erkannt.                                           */
{
    struct Var *new;int f;
    struct struct_declaration *sd;
    /*if(*identifier==0) return;*/ /* sollte woanders bemaekelt werden */
    if(DEBUG&2) printf("add_var(): %s\n",identifier);
    if((t->flags&15)==FUNKT&&((t->next->flags&15)==ARRAY||(t->next->flags&15)==FUNKT))
        error(25);
    new=mymalloc(sizeof(struct Var));
    new->identifier=add_identifier(identifier,strlen(identifier));
    new->clist=clist;
    new->vtyp=t;
    new->storage_class=storage_class&7;
    new->next=0;
    new->flags=0;
    new->fi=0;
    new->nesting=nesting;
/*    if((storage_class&7)==STATIC||(storage_class&7)==EXTERN) new->flags=USEDASSOURCE|USEDASDEST;*/
    if(storage_class&PARAMETER) new->flags=USEDASDEST;
    if((storage_class&7)==REGISTER) new->priority=registerpri; else new->priority=0;
    if(last_var[nesting]){
        new->offset=zladd(last_var[nesting]->offset,szof(last_var[nesting]->vtyp));
        last_var[nesting]->next=new;
        last_var[nesting]=new;
    }else{
        new->offset=l2zl(0L);
        first_var[nesting]=last_var[nesting]=new;
    }
    f=t->flags&15;
    if((storage_class&7)==AUTO||(storage_class&7)==REGISTER){
        if(type_uncomplete(t)&&(t->flags&15)!=ARRAY) error(202,identifier);
        /*  das noch ueberpruefen   */
        if((c_flags_val[0].l&2)&&nesting==1&&!(storage_class&PARAMETER)){
            new->offset=max_offset;
        }else{
            new->offset=local_offset[nesting];
        }
        new->offset=zlmult(zldiv(zladd(new->offset,zlsub(align[f],l2zl(1L))),align[f]),align[f]);
        if((storage_class&PARAMETER)&&f>=CHAR&&f<=SHORT){
        /*  Integer-Erweiterungen fuer alle Funktionsparameter  */
            local_offset[nesting]=zladd(new->offset,sizetab[INT]);
        }else{
            local_offset[nesting]=zladd(new->offset,szof(new->vtyp));
        }
        /*  Bei alten Funktionen werden FLOAT als DOUBLE uebergeben */
        if((storage_class&(PARAMETER|OLDSTYLE))==(PARAMETER|OLDSTYLE)&&f==FLOAT)
            local_offset[nesting]=zladd(local_offset[nesting],zlsub(sizetab[DOUBLE],sizetab[FLOAT]));

        if(zlleq(max_offset,local_offset[nesting])) max_offset=local_offset[nesting];
    }
    if((storage_class&7)==STATIC) new->offset=l2zl((long)++label);
    if(storage_class&PARAMETER){
/*        new->storage_class&=~PARAMETER;*/
        /* ob das hier so funktioniert ? Bei BIGENDIAN nimmt man den    */
        /* hinteren Teil des INTs, bei LOWENDIAN muesste man eigentlich */
        /* nichts tun, oder? Datenformate, bei denen wirklich           */
        /* umgewandelt werden muss, werden (noch?) nicht unterstuetzt.  */
        /* Ob sowas aber ueberhaupt zulaessig waere, weiss ich nicht.   */
        if(f>=CHAR&&f<=SHORT&&!zlleq(sizetab[INT],sizetab[f])){
            if(BIGENDIAN){
                new->offset=zladd(new->offset,zlsub(sizetab[INT],sizetab[f]));
            }else{
                if(!LITTLEENDIAN)
                    ierror(0);
            }
        }
        if((storage_class&OLDSTYLE)&&f==FLOAT){
        /*  Bei alten Funktionen werden DOUBLE nach FLOAT konvertiert   */
            struct IC *conv=mymalloc(ICS);
            conv->code=CONVDOUBLE;
            conv->typf=FLOAT;
            conv->q1.flags=VAR|DONTREGISTERIZE;
            conv->z.flags=VAR;
            conv->q2.flags=0;
            conv->q1.v=conv->z.v=new;
            conv->q1.val.vlong=conv->z.val.vlong=l2zl(0);
            add_IC(conv);
            new->flags|=CONVPARAMETER;
        }
        new->offset=zlsub(l2zl(0L),new->offset);
    }
    if((storage_class&7)==EXTERN){
        if(!strcmp("fprintf",identifier)) new->flags|=PRINTFLIKE;
        if(!strcmp("printf",identifier))  new->flags|=PRINTFLIKE;
        if(!strcmp("sprintf",identifier)) new->flags|=PRINTFLIKE;
        if(!strcmp("fscanf",identifier))  new->flags|=SCANFLIKE;
        if(!strcmp("scanf",identifier))   new->flags|=SCANFLIKE;
        if(!strcmp("sscanf",identifier))  new->flags|=SCANFLIKE;
    }
    return(new);
}
void free_fi(struct function_info *p)
/*  Gibt ein function_info mit Inhalt frei  */
{
    if(p->first_ic) free_IC(p->first_ic);
    if(p->vars) free_var(p->vars);
    free(p);
}
void free_var(struct Var *p)
/*  Gibt Variablenliste inkl. Typ, aber ohne Identifier frei    */
{
    struct Var *merk;
    while(p){
        merk=p->next;
        if(!(p->flags&USEDASADR)&&(p->storage_class==AUTO||p->storage_class==REGISTER)){
            if(*p->identifier&&!(p->flags&USEDASDEST)&&(p->vtyp->flags&15)<=POINTER) error(64,p->identifier);
            if(*p->identifier&&!(p->flags&USEDASSOURCE)&&(p->vtyp->flags&15)<=POINTER) error(65,p->identifier);
        }
        if(DEBUG&2) printf("free_var %s, pri=%d\n",p->identifier,p->priority);
        if(p->vtyp) freetyp(p->vtyp);
        if(p->clist) free_clist(p->clist);
        if(p->fi){
            if(DEBUG&2) printf("free_fi of function %s\n",p->identifier);
            free_fi(p->fi);
            if(DEBUG&2) printf("end free_fi of function %s\n",p->identifier);
        }
        free(p);
        p=merk;
    }
}
struct Var *find_var(char *identifier,int endnesting)
/*  sucht Variable mit Bezeichner und liefert Zeiger zurueck    */
/*  es werden nur Variablen der Bloecke endnesting-nesting      */
/*  durchsucht                                                  */
{
    int i;struct Var *v;
    if(*identifier==0||identifier==0) return(0);
    for(i=nesting;i>=endnesting;i--){
        v=first_var[i];
        while(v){
            if(!strcmp(v->identifier,identifier)) return(v);
            v=v->next;
        }
    }
    return(0);
}
void var_declaration(void)
/*  Bearbeitet eine Variablendeklaration und erzeugt alle       */
/*  noetigen Strukturen                                         */
{
    struct Typ *ts,*t,*old=0,*om=0;char *imerk,vident[MAXI];
    int mdef=0,makeint=0,notdone,storage_class,msc,extern_flag,isfunc,had_decl;
    struct Var *v;
    ts=declaration_specifiers();notdone=1;
    storage_class=return_sc;
    if(storage_class==EXTERN) extern_flag=1; else extern_flag=0;
    killsp();
    if(*s==';'){
        if(storage_class||((ts->flags&15)!=STRUCT&&(ts->flags&15)!=UNION&&(ts->flags&15)!=INT))
            error(36);
        freetyp(ts);s++;killsp();
        return;
    }
    if(nesting==0&&(storage_class==AUTO||storage_class==REGISTER))
        {error(66);storage_class=EXTERN;}
    if(!ts){
        if(nesting<=1){
            ts=mymalloc(TYPS);
            ts->flags=INT;ts->next=0;
            makeint=1;
            if(!storage_class) storage_class=EXTERN;
            error(67);
        }else{
            ierror(0);return;
        }
    }
    if(storage_class==0){
        if(nesting==0) storage_class=EXTERN; else storage_class=AUTO;
    }
    msc=storage_class;
    while(notdone){
        int oldnesting=nesting;
        imerk=ident;ident=vident;*vident=0;  /* merken von ident hier vermutlich */
        storage_class=msc;
        if(old) {freetyp(old);old=0;}
        t=declarator(clone_typ(ts));
        if((t->flags&15)!=FUNKT) isfunc=0;
            else {isfunc=1;if(storage_class!=STATIC) storage_class=EXTERN;}
        ident=imerk;                    /* nicht unbedingt noetig ?         */
        v=find_var(vident,oldnesting);
        if(v){
            had_decl=1;
            if(nesting>0&&(v->flags&DEFINED)&&!extern_flag&&!isfunc){
                error(27,vident);
            }else{
                if(t&&v->vtyp&&!compare_pointers(v->vtyp,t,255)){
                    error(68,vident);
                }
                if(storage_class!=v->storage_class&&!extern_flag)
                    error(28,v->identifier);
                if(!isfunc&&!extern_flag) v->flags|=TENTATIVE;
            }
            if(!isfunc){
                v->vtyp=t;
            }else{
                om=v->vtyp;
                if(t->exact->count>0) {old=v->vtyp;v->vtyp=t;}
            }
        }else{
            had_decl=0;
            if(isfunc&&*s!=','&&*s!=';'&&*s!=')'&&*s!='='&&nesting>0) nesting--;
            v=add_var(vident,t,storage_class,0);
            if(isfunc&&*s!=','&&*s!=';'&&*s!=')'&&*s!='='&&nesting>=0) nesting++;
            if(!v) ierror(0);
            else{
                if(!isfunc&&!extern_flag){
                    v->flags|=TENTATIVE;
                    if(nesting>0) v->flags|=DEFINED;
                }
            }
            om=0;
        }
        killsp();
        /*  Initialisierung von Variablen bei Deklaration   */
        if(*s=='='){
            s++;killsp();
            if(!had_decl&&v->nesting==0&&v->storage_class==EXTERN)
                error(168,v->identifier);
            if(v->flags&DEFINED) {if(nesting==0) error(30,v->identifier);}
                else v->flags|=DEFINED;
            if(v->storage_class==TYPEDEF) error(114,v->identifier);
            if(extern_flag){
                if(nesting==0)
                    error(118,v->identifier);
                else
                    error(207,v->identifier);
                if(v->storage_class!=EXTERN){ error(77);v->storage_class=EXTERN;}
            }
            v->clist=initialization(v->vtyp,v->storage_class==AUTO||v->storage_class==REGISTER);
            if(v->clist){
                if((v->vtyp->flags&15)==ARRAY&&zleqto(v->vtyp->size,l2zl(0L))){
                    struct const_list *p=v->clist;
                    while(p){v->vtyp->size=zladd(v->vtyp->size,l2zl(1L));p=p->next;}
                    local_offset[nesting]=zladd(local_offset[nesting],szof(v->vtyp));
                    if(zlleq(max_offset,local_offset[nesting])) max_offset=local_offset[nesting];
                }
                if(v->storage_class==AUTO||v->storage_class==REGISTER){
                    struct IC *new;
                /*  Initialisierung von auto-Variablen  */
                    new=mymalloc(ICS);
                    new->code=ASSIGN;
                    new->typf=v->vtyp->flags;
                    new->q2.flags=0;
                    new->q2.val.vlong=szof(v->vtyp);
                    new->z.flags=VAR;
                    new->z.v=v;
                    new->z.val.vlong=l2zl(0L);
                    if(v->clist->tree){
                    /*  einzelner Ausdruck  */
                        gen_IC(v->clist->tree,0,0);
                        convert(v->clist->tree,v->vtyp->flags&31);
                        new->q1=v->clist->tree->o;
/*                        v->clist=0;*/
                    }else{
                    /*  Array etc.  */
                        struct Var *nv;
                        nv=add_var(empty,clone_typ(v->vtyp),STATIC,v->clist);
                        nv->flags|=DEFINED;
                        nv->vtyp->flags|=CONST;
/*                        v->clist=0;*/
                        new->q1.flags=VAR;
                        new->q1.v=nv;
                        new->q1.val.vlong=l2zl(0L);
                    }
                    add_IC(new);
/*                    if(v->clist&&v->clist->tree){free_expression(v->clist->tree);v->clist->tree=0;}*/
                }else if(c_flags[19]&USEDFLAG){
                    /*  Ohne Optimierung gleich erzeugen; das ist noch  */
                    /*  etwas von der genauen Implementierung der Liste */
                    /*  der Variablen abhaengig.                        */
                    struct Var *merk=v->next;
                    v->next=0;
                    gen_vars(v);
                    v->next=merk;
                    v->clist=0;
                }
            }
        }else{
            if((v->flags&DEFINED)&&type_uncomplete(v->vtyp)) error(202,v->identifier);
            if((v->vtyp->flags&CONST)&&(v->storage_class==AUTO||v->storage_class==REGISTER))
                error(119,v->identifier);
        }
        if(*s==',') {s++;killsp();mdef=1;} else notdone=0;
    }
    freetyp(ts);
    if(!mdef&&t&&(t->flags&15)==FUNKT&&*s!=';'){
    /*  Funktionsdefinition                                     */
        int i,oldstyle=0;
        fline=line;
        if(DEBUG&1) printf("Funktionsdefinition!\n");
        cur_func=v->identifier;
        if(only_inline==2) only_inline=0;
        if(nesting<1) ierror(0);
        if(nesting>1) error(32);
        if(v->flags&DEFINED) error(33,v->identifier);
            else v->flags|=DEFINED;
        if(storage_class!=EXTERN&&storage_class!=STATIC) error(34);
        if(extern_flag) error(120);
        if(!strcmp(v->identifier,"main")&&(!t->next||t->next->flags!=INT)) error(121);
        if(!had_decl&&v->nesting==0&&v->storage_class==EXTERN)
            error(168,v->identifier);
        while(*s!='{'){
        /*  alter Stil  */
            struct Typ *nt=declaration_specifiers();notdone=1;oldstyle=OLDSTYLE;
            if(!ts) {error(35);}
            while(notdone){
                int found=0;
                imerk=ident;ident=vident;*vident=0;
                ts=declarator(clone_typ(nt));
                ident=imerk;
                if(!ts) {error(36);}
                else{
                    for(i=0;i<t->exact->count;i++){
                        if(!strcmp((*t->exact->sl)[i].identifier,vident)){
                            found=1;
                            if((*t->exact->sl)[i].styp){
                                error(69,vident);
                                freetyp((*t->exact->sl)[i].styp);
                            }
                            /*  typ[] in *typ   */
                            if((ts->flags&15)==ARRAY) ts->flags=POINTER;
                            /*  typ() in *typ() */
                            if((ts->flags&15)==FUNKT){
                                struct Typ *new=mymalloc(TYPS);
                                new->flags=POINTER;
                                new->next=ts;
                                ts=new;
                            }
                            if(!return_sc) return_sc=AUTO;
                            if(return_sc!=AUTO&&return_sc!=REGISTER)
                                {error(122);return_sc=AUTO;}
                            (*t->exact->sl)[i].storage_class=return_sc;
                            (*t->exact->sl)[i].styp=ts;
                        }
                    }
                }
                if(!found) {error(37,vident);}
                killsp();
                if(*s==',') {s++;killsp();} else notdone=0;
            }
            if(nt) freetyp(nt);
            if(*s==';'){s++;killsp();
            }else{
                error(54);
                while(*s!='{'&&*s!=';'){s++;killsp();}
            }
        }
        if(t->exact->count==0){
            struct struct_list sl[1];
            if(DEBUG&1) printf("prototype converted to (void)\n");
            t->exact->count=1;
            sl[0].identifier=empty;
            sl[0].storage_class=AUTO;
            sl[0].styp=mymalloc(TYPS);
            sl[0].styp->flags=VOID;
            sl[0].styp->next=0;
            nesting--;
            add_sl(t->exact,&sl);
            nesting++;
        }
        if(om&&!compare_sd(om->exact,t->exact))
            error(123);
        nocode=0;currentpri=1;
/*        enter_block();*/
        local_offset[1]=maxalign;
        return_var=0;
        if(!v->vtyp) ierror(0);
        if(v->vtyp->next->flags==VOID) return_typ=0;
        else{
            return_typ=v->vtyp->next;
            if(!freturn(return_typ)){
#ifdef OLDPARMS
                return_var=add_var(empty,clone_typ(return_typ),STATIC,0);
                return_var->flags|=DEFINED;
#else
                /*  Parameter fuer die Rueckgabe von Werten, die nich in einem  */
                /*  Register sind.                                              */
                struct Typ *rt=mymalloc(TYPS);
                rt->flags=POINTER;rt->next=return_typ;
                return_var=add_var(empty,clone_typ(rt),AUTO|PARAMETER|oldstyle,0);
                return_var->flags|=DEFINED;
                free(rt);
#endif
            }
        }
        first_ic=last_ic=0;ic_count=0;
        for(i=0;i<t->exact->count;i++){
            if(!(*t->exact->sl)[i].styp&&*(*t->exact->sl)[i].identifier){
                struct Typ *nt;
                nt=mymalloc(TYPS);
                nt->flags=INT; nt->next=0;
                (*t->exact->sl)[i].styp=nt;
                (*t->exact->sl)[i].storage_class=AUTO;
                error(124);
            }
            if(*(*t->exact->sl)[i].identifier){
                struct Var *tmp;
                tmp=add_var((*t->exact->sl)[i].identifier,clone_typ((*t->exact->sl)[i].styp),(*t->exact->sl)[i].storage_class|PARAMETER|oldstyle,0);
                tmp->flags|=DEFINED;
                if(oldstyle){
                    freetyp((*t->exact->sl)[i].styp);
                    (*t->exact->sl)[i].styp=0; /*  Prototype entfernen */
                }
            }
        }
        if(oldstyle) t->exact->count=0; /*  Prototype entfernen */
        local_offset[1]=l2zl(0L);
        return_label=++label;
        v->flags|=GENERATED;
        {int i;
            for(i=1;i<=MAXR;i++) {regs[i]=regused[i]=regsa[i];regsbuf[i]=0;}
        }
        max_offset=l2zl(0L);function_calls=0;float_used=0;has_return=0;goto_used=0;
        compound_statement();
        if((v->vtyp->next->flags&15)!=VOID&&!has_return){
            if(strcmp(v->identifier,"main")) error(173,v->identifier);
                else error(174,v->identifier);
        }
        {int i;
            for(i=1;i<=MAXR;i++) if(regs[i]!=regsa[i]) {printf("Register %s:\n",regnames[i]);ierror(0);}
        }
        gen_label(return_label);
#ifdef OLDPARMS
        if(return_var){
        /*  Zeiger in Returnregister schreiben, bei Funktionen, deren   */
        /*  Rueckgabewert nicht in Registern uebergeben wird            */
            int rreg;
            struct Typ *pointer=mymalloc(TYPS);
            struct IC *new=mymalloc(ICS);
            pointer->flags=POINTER;pointer->next=0;
            rreg=freturn(pointer);
            free(pointer);
            if(regs[rreg]) ierror(0);
            new->code=ALLOCREG;
            new->q1.flags=REG;
            new->q2.flags=new->z.flags=0;
            new->typf=0;
            new->q1.reg=rreg;
            add_IC(new);
            regs[rreg]=1;
            new=mymalloc(ICS);
            new->code=ASSIGN;
            new->typf=POINTER;
            new->q1.flags=SCRATCH|VAR|VARADR;
            new->q1.reg=0;
            new->q1.v=return_var;
            new->q1.val.vlong=l2zl(0L);
            new->q2.flags=0;
            new->q2.val.vlong=sizetab[POINTER];
            new->z.flags=REG;
            new->z.reg=rreg;
            add_IC(new);
            free_reg(rreg);
        }
#endif
        if(first_ic&&errors==0){
            if((c_flags[2]&USEDFLAG)&&ic1){fprintf(ic1,"function %s\n",v->identifier); pric(ic1,first_ic);}
/*            if((c_flags[0]&USEDFLAG)&&(c_flags_val[0].l&1)) simple_regs();*/
            if(c_flags[0]&USEDFLAG) optimize(c_flags_val[0].l,v);
            if((c_flags[3]&USEDFLAG)&&ic2){fprintf(ic2,"function %s\n",v->identifier); pric(ic2,first_ic);}
            if(out&&!only_inline&&!(c_flags[5]&USEDFLAG)){
                gen_code(out,first_ic,v,max_offset);
            }
/*            if(DEBUG&8192){fprintf(ic2,"function %s, after gen_code\n",v->identifier); pric(ic2,first_ic);}*/
            free_IC(first_ic);
            first_ic=last_ic=0;
        }
        if(v->fi&&v->fi->first_ic){
            struct Var *vp;
            if(DEBUG&1) printf("leave block %d (inline-version)\n",nesting);
            if(nesting!=1) ierror(0);
            if(merk_varl) merk_varl->next=first_var[nesting]; else merk_varf=first_var[nesting];
            if(last_var[nesting]) merk_varl=last_var[nesting];
            if(merk_sil) merk_sil->next=first_si[nesting]; else merk_sif=first_si[nesting];
            if(last_si[nesting]) merk_sil=last_si[nesting];
            if(merk_sdl) merk_sdl->next=first_sd[nesting]; else merk_sdf=first_sd[nesting];
            if(last_sd[nesting]) merk_sdl=last_sd[nesting];
            if(merk_ilistl) merk_ilistl->next=first_ilist[nesting]; else merk_ilistf=first_ilist[nesting];
            if(last_ilist[nesting]) merk_ilistl=last_ilist[nesting];

            if(merk_varf&&!only_inline) gen_vars(merk_varf);
            if(first_llist) free_llist(first_llist);
            if(first_clist) free_clist(first_clist);
            if(merk_sif) free_si(merk_sif);
/*  struct-declarations erst ganz am Schluss loeschen. Um zu vermeiden,     */
/*  dass struct-declarations in Prototypen frei werden und dann eine        */
/*  spaetere struct, dieselbe Adresse bekommt und dadurch gleich wird.      */
/*  Nicht sehr schoen - wenn moeglich noch mal aendern.                     */
/*            if(merk_sdf) free_sd(merk_sdf);*/
            /*  hier noch was ueberlegen    */
/*            if(merk_ilistf) free_ilist(merk_ilistf);*/
            nesting--;
            v->fi->vars=merk_varf;
/*            v->fi->vars=first_var[1];*/
            /*  keine echten Parameter=>keine negativen Offsets */
/*            vp=first_var[1];*/
            vp=merk_varf;
            while(vp){
                if(vp->storage_class==AUTO||vp->storage_class==REGISTER){
                    if(!zlleq(l2zl(0L),vp->offset)){
                        vp->offset=l2zl(0L);
                        if(DEBUG&1024) printf("converted parameter <%s>(%ld) for inlining\n",vp->identifier,(long)zl2l(vp->offset));
                    }else vp->offset=l2zl(4L);  /*  Dummy, da recalc_offsets?   */
                }
                vp=vp->next;
            }
        }else{
            leave_block();
        }
        if(only_inline==2) only_inline=0;
        cur_func="oops, I forgot it";
    }else{
        if(makeint) error(125);
        if(*s==';') s++; else error(54);
        if((t->flags&15)==FUNKT&&t->exact){
            struct struct_declaration *sd=t->exact;int i,f;
            for(f=0,i=0;i<sd->count;i++)
                if(!(*sd->sl)[i].styp){error(126);f=1;}
            if(f){
                for(i=0;i<sd->count;i++) if((*sd->sl)[i].styp) freetyp((*sd->sl)[i].styp);
                sd->count=0;
            }
        }
    }
    if(old) freetyp(old);
}
int storage_class_specifiers(void)
/*  Gibt angegebene storage_class zurueck                   */
/*  muss nach ANSI wohl in declaration_specifiers           */
/*  integriert werden                                       */
{
    char *merk=s,buff[MAXI];
    killsp();cpbez(buff,0);
    if(!strcmp("auto",buff)) return(AUTO);
    if(!strcmp("register",buff)) return(REGISTER);
    if(!strcmp("static",buff)) return(STATIC);
    if(!strcmp("extern",buff)) return(EXTERN);
    if(!strcmp("typedef",buff)) return(TYPEDEF);
    s=merk;return(0);
}
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
int compare_pointers(struct Typ *a,struct Typ *b,int qual)
/*  vergleicht, ob Typ beider Typen gleich ist, const/volatile      */
/*  werden laut ANSI nicht beruecksichtigt                          */
{
    struct struct_declaration *sd;
    int af=a->flags&qual,bf=b->flags&qual;
    if(af!=bf) return(0);
    af&=15;bf&=15;
    if(af==FUNKT){
        if(a->exact->count&&!compare_sd(a->exact,b->exact)) return(0);
    }
    if(af==STRUCT||af==UNION){
        if(a->exact!=b->exact) return(0);
    }
    if(af==ARRAY){
        if(!zleqto(a->size,l2zl(0L))&&!zleqto(b->size,l2zl(0L))&&!zleqto(a->size,b->size)) return(0);
    }
    if(a->next==0&&b->next!=0) return(0);
    if(a->next!=0&&b->next==0) return(0);
    if(a->next==0&&b->next==0) return(1);
    return(compare_pointers(a->next,b->next,qual));
}
int compare_sd(struct struct_declaration *a,struct struct_declaration *b)
/*  Vergleicht, ob zwei struct_declarations identisch sind          */
/*  Wird nur nur fuer Prototypen benutzt, leere Liste immer gleich  */
{
    int i;
    if(!a->count||!b->count) return(1);
    if(a->count!=b->count) return(0);
    for(i=0;i<a->count;i++)
        if((*a->sl)[i].styp&&(*b->sl)[i].styp&&!compare_pointers((*a->sl)[i].styp,(*b->sl)[i].styp,255)) return(0);
    return(1);
}
void free_clist(struct const_list *p)
/*  gibt clist frei                                         */
{
    struct const_list *merk;
    return;
    while(p){
        merk=p->next;
        if(p->other) free_clist(p->other);
        if(p->tree) free_expression(p->tree);
        free(p);
        p=merk;
    }
}
void gen_clist(FILE *,struct Typ *,struct const_list *);

void gen_vars(struct Var *v)
/*  generiert Variablen                                     */
{
    int mode,al;struct Var *p;
    if(errors!=0||(c_flags[5]&USEDFLAG)) return;
    for(mode=0;mode<3;mode++){
        for(al=maxalign;al>=1;al--){
            int i,flag;
            for(i=1,flag=0;i<15;i++) if(align[i]==al) flag=1;
            if(!flag) continue;
            gen_align(out,al);
            for(p=v;p;p=p->next){
                if(DEBUG&2) printf("gen_var(): %s\n",p->identifier);
                if(p->storage_class==STATIC||p->storage_class==EXTERN){
                    if(!(p->flags&GENERATED)){
                        if(p->storage_class==EXTERN&&!(p->flags&(USEDASSOURCE|USEDASDEST))&&!(p->flags&(TENTATIVE|DEFINED))) continue;
                        if((p->vtyp->flags&15)!=ARRAY){
                            if(align[p->vtyp->flags&15]!=al) continue;
                        }else{
                            if(align[p->vtyp->next->flags&15]!=al) continue;
                        }
                        /*  erst konstante initialisierte Daten */
                        if(mode==0){
                            if(!p->clist) continue;
                            if(!(p->vtyp->flags&(CONST|STRINGCONST))){
                                struct Typ *t=p->vtyp;int f=0;
                                do{
                                    if(t->flags&(CONST|STRINGCONST)) break;
                                    if((t->flags&15)!=ARRAY){f=1;break;}
                                    t=t->next;
                                }while(1);
                                if(f) continue;
                            }
                        }
                        /*  dann initiolisierte */
                        if(mode==1&&!p->clist) continue;
                        /*  und dann der Rest   */
                        if(mode==2&&p->clist) continue;
                        gen_var_head(out,p);
                        if(!(p->flags&(TENTATIVE|DEFINED))){
                            if(p->storage_class==STATIC) error(127,p->identifier);
                            continue;
                        }
                        if(!p->clist){
                            if(type_uncomplete(p->vtyp)) error(202,p->identifier);
                            gen_ds(out,szof(p->vtyp),p->vtyp);
                        }else{
                            gen_clist(out,p->vtyp,p->clist);
                        }
                        p->flags|=GENERATED;
                    }
                }
            }
        }
    }
}
void gen_clist(FILE *f,struct Typ *t,struct const_list *cl)
/*  generiert dc fuer const_list                            */
/*  hier ist noch einiges zu tun                            */
{
    int i;zlong sz;
    if((t->flags&15)==ARRAY){
        for(sz=l2zl(0L);!zlleq(t->size,sz)&&cl;sz=zladd(sz,l2zl(1L)),cl=cl->next){
            if(!cl->other){ierror(0);return;}
            gen_clist(f,t->next,cl->other);
        }
        if(!zlleq(t->size,sz)) gen_ds(f,zlmult(zlsub(t->size,sz),szof(t->next)),t->next);
        return;
    }
    if((t->flags&15)==UNION){
        gen_clist(f,(*t->exact->sl)[0].styp,cl);
        sz=zlsub(szof(t),szof((*t->exact->sl)[0].styp));
        if(!zleqto(sz,l2zl(0L))) gen_ds(f,sz,0);
        return;
    }
    if((t->flags&15)==STRUCT){
        zlong al;int fl;struct Typ *st,*h;
        sz=l2zl(0L);
        for(i=0;i<t->exact->count&&cl;i++){
            if(!cl->other){ierror(0);return;}
            st=(*t->exact->sl)[i].styp;
            h=st;
            do{
                fl=h->flags&15;
                h=h->next;
            }while(fl==ARRAY);
            al=align[fl];
            if(!zleqto(zlmod(sz,al),l2zl(0L))){
                gen_ds(f,zlsub(al,zlmod(sz,al)),0);
                sz+=zladd(sz,zlsub(al,zlmod(sz,al)));
            }
            if(!(*t->exact->sl)[i].identifier) ierror(0);
            if((*t->exact->sl)[i].identifier[0]){
                gen_clist(f,st,cl->other);
                cl=cl->next;
            }else{
                gen_ds(f,szof(st),0); /* sollte unnamed bitfield sein */
            }
            sz=zladd(sz,szof(st));
        }
        for(;i<t->exact->count;i++){
            st=(*t->exact->sl)[i].styp;
            h=st;
            do{
                fl=h->flags&15;
                h=h->next;
            }while(fl==ARRAY);
            al=align[fl];
            if(!zleqto(zlmod(sz,al),l2zl(0L))){
                gen_ds(f,zlsub(al,zlmod(sz,al)),0);
                sz+=zladd(sz,zlsub(al,zlmod(sz,al)));
            }
            gen_ds(f,szof((*t->exact->sl)[i].styp),(*t->exact->sl)[i].styp);
            sz=zladd(sz,szof(st));
        }
        al=align[STRUCT];
        if(!zleqto(zlmod(sz,al),l2zl(0L)))
            gen_ds(f,zlsub(al,zlmod(sz,al)),0);
        return;
    }
    gen_dc(f,t->flags&31,cl);
}
struct const_list *initialization(struct Typ *t,int noconst)
/*  traegt eine Initialisierung in eine const_list ein          */
{
    struct const_list *first,*cl,**prev;np tree,tree2;int bracket;zlong i;
    int f=t->flags&15;
    if(f==FUNKT){error(42);return(0);}
    if(*s=='{'){s++;killsp();bracket=1;} else bracket=0;
    if(f==ARRAY){
        if(*s=='\"'&&t->next&&(t->next->flags&15)==CHAR){
            killsp();
            tree=string_expression();
            first=(struct const_list *)tree->identifier;
            free_expression(tree);
        }else{
            prev=0;
            for(i=l2zl(0L);(zleqto(t->size,l2zl(0L))||!zlleq(t->size,i))&&*s!='}';i=zladd(i,l2zl(1L))){
                if(!zlleq(i,0)){
                    if(*s==','){s++;killsp();} else break;
                    if(*s=='}') break;
                }
                cl=mymalloc(CLS);
                cl->next=0;cl->tree=0;
                cl->other=initialization(t->next,0);
                killsp();
                if(prev) *prev=cl; else first=cl;
                prev=&cl->next;
            }
        }
    }else if(f==STRUCT&&(bracket||!noconst)){
        if(t->exact->count<=0)
            {error(43);return(0);}
        prev=0;
        for(i=l2zl(0L);!zlleq(t->exact->count,i)&&*s!='}';i=zladd(i,l2zl(1L))){
            if((*t->exact->sl)[zl2l(i)].identifier[0]==0) {continue;} /* unnamed bitfield */
            if(!zlleq(i,0)){
                if(*s==','){s++;killsp();} else break;
                if(*s=='}') break;
            }
            cl=mymalloc(CLS);
            cl->next=0;cl->tree=0;
            cl->other=initialization((*t->exact->sl)[zl2l(i)].styp,0);
            if(prev) *prev=cl; else first=cl;
            prev=&cl->next;
        }
    }else if(f==UNION&&(bracket||!noconst)){
        if(t->exact->count<=0)
            {error(44);return(0);}
        first=initialization((*t->exact->sl)[0].styp,0);
    }else{
        tree2=tree=assignment_expression();
        if(!tree){error(45);return(0);}
        if(!type_expression(tree)){free_expression(tree); return(0);}
        tree=makepointer(tree);
        test_assignment(t,tree);
        if(!noconst){
        /*  nur Konstanten erlaubt (bei Arrays/Strukturen etc. oder static) */
            if(tree->flags!=CEXPR){
                while(tree->flags==CAST) tree=tree->left;
                if(tree->flags==ADDRESS||tree->flags==ADDRESSS||tree->flags==ADDRESSA){
                    gen_IC(tree,0,0);
                    if(!(tree->o.flags&VARADR)){
                    /*  hier fehlen noch viele Pruefungen   */
                        free_expression(tree);error(46);
                        return(0);
                    }
                    first=mymalloc(CLS);
                    first->next=first->other=0;
                    first->tree=tree;
                    killsp();
                }else{
                    free_expression(tree);error(46);
                    return(0);
                }
            }else{
                first=mymalloc(CLS);
                first->next=first->other=0;
                first->tree=0;
                eval_constn(tree);
                tree->ntyp->flags=t->flags;
                insert_const(tree);
                first->val=tree->val;
                free_expression(tree2);
                killsp();
            }
        }else{
        /*  auch anderes erlaubt    */
            first=mymalloc(CLS);
            first->next=first->other=0;
            first->tree=tree;
            killsp();
        }
    }
    if(bracket){
        if(*s==','){s++;killsp();}
        if(*s=='}'){s++;killsp();} else error(128);
    }
    return(first);
}



