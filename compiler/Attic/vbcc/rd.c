/*  $VER: vbcc (rd.c) V0.4     */
/*  verfuegbare Definitionen und constant propagation   */

#include "opt.h"

static char FILE_[]=__FILE__;

/*  fuer verfuegbare Definitionen   */
unsigned int dcount;
size_t dsize;
struct IC **dlist;
unsigned char **defs;       /*  gibt an, welche Definitionen, welche    */
                            /*  Variablen definieren                    */
/*  alle Definitionen, globaler oder Adr. fuer propagation etc.         */
unsigned char *rd_globals,*rd_address,*rd_statics,*rd_drefs;
/*  dasseble, aber hier nur die undefs  */
unsigned char *rd_defs,*rd_tmp,*rd_parms;

/*  Bitvektor fuer geaenderte Variablen fuer ic_changes */
unsigned char *rd_vars;

/*  rd_mode==0 => reaching definitions fuer constant-propagatione,      */
/*  sonst fuer loop-invariant.                                          */
int rd_mode;

int compare_const(union atyps *q1,union atyps *q2,int t)
/*  vergleiht zwei Konstanten; -1, wenn q1<q2; 0, wenn q1==q1; 1 sonst  */
{
    zdouble d1,d2;zlong l1,l2;zulong u1,u2;zpointer p1,p2;
    t&=NU;
    eval_const(q1,t);p1=vpointer;d1=vdouble;l1=vlong;u1=vulong;
    eval_const(q2,t);p2=vpointer;d2=vdouble;l2=vlong;u2=vulong;
    if(t==POINTER) return(zpleq(p2,p1)?!zpeqto(p1,p2):-1);
    if(t==DOUBLE||t==FLOAT) return(zdleq(d2,d1)?!zdeqto(d1,d2):-1);
    if(t&UNSIGNED) return(zulleq(u2,u1)?!zuleqto(u1,u2):-1);
    return(zlleq(l2,l1)?!zleqto(l1,l2):-1);
    ierror(0);
}

void print_rd(unsigned char *bitvector)
/*  druckt Definitionen in einem Bitvektor */
{
    unsigned int i;
    if(!bitvector) {printf("reaching definitions not available\n");return;}
    for(i=1;i<=dcount;i++)
        if(BTST(bitvector,i)) {printf("%3u:",i);pric2(stdout,dlist[i]);}
    for(i=0;i<vcount-rcount;i++)
        if(BTST(bitvector,i+dcount+1)) printf("%3u:\t\tundefined\t->%s\n",i+dcount+1,vilist[i]->identifier);
    for(i=vcount-rcount;i<vcount;i++)
        if(BTST(bitvector,i+dcount+1)) printf("%3u:\t\tundefined\t->(%s)\n",i+dcount+1,vilist[i]->identifier);
}

void num_defs(void)
/*  Numeriert alle Variablendefinitionen (nur elementare Typen noetig)  */
/*  und erzeugt diverse Bitvektoren.                                    */
{
    int i=0;struct IC *p;
    if(DEBUG&1024) printf("numerating definitions\n");
    for(p=first_ic;p;p=p->next){
        if(!(p->z.flags&VAR)&&p->code!=CALL){p->defindex=0;continue;}
/*        if((p->z.v->vtyp->flags&NQ)>POINTER){p->defindex=0;continue;}*/
        p->defindex=++i;
    }
    dcount=i;dsize=(dcount+CHAR_BIT+vcount)/CHAR_BIT;    /* +1, da bei 1 anfaengt */
    if(DEBUG&1024) printf("%lu definitions, dsize=%lu\n",(unsigned long)dcount,(unsigned long)dsize);
    /*  fuer jede Variable wird eine kuenstliche unbestimmte Def. erzeugt   */
    /*  Feld erzeugen, dass zu jeder Variable alle Definitionen erhaelt    */
    defs=mymalloc(sizeof(unsigned char *)*vcount);
    for(i=0;i<vcount;i++){
        defs[i]=mymalloc(dsize);
        memset(defs[i],0,dsize);
        BSET(defs[i],i+dcount+1);
    }
    dlist=mymalloc((dcount+1)*sizeof(struct IC *));
    rd_globals=mymalloc(dsize);
    memset(rd_globals,0,dsize);
    rd_statics=mymalloc(dsize);
    memset(rd_statics,0,dsize);
    rd_address=mymalloc(dsize);
    memset(rd_address,0,dsize);
    rd_drefs=mymalloc(dsize);
    memset(rd_drefs,0,dsize);

    rd_vars=mymalloc(vsize);

    for(p=first_ic;p;p=p->next){
        if(p->defindex){
            if(p->z.flags&VAR){
                i=p->z.v->index;
                if(p->z.flags&DREFOBJ) i+=vcount-rcount;
                BSET(defs[i],p->defindex);
            }
            dlist[p->defindex]=p;
        }
    }
    if(DEBUG&2048){
        for(i=1;i<=dcount;i++){ printf("Def%3d: ",i);pric2(stdout,dlist[i]);}
        for(i=0;i<vcount;i++){
            printf("Definitionen fuer <%s>:\n",vilist[i]->identifier);
            print_rd(defs[i]);
        }
    }
    for(i=0;i<vcount-rcount;i++){   /*  rd_globals berechnen    */
        if(vilist[i]->vtyp->flags&CONST) continue;
        if(vilist[i]->nesting==0||vilist[i]->storage_class==EXTERN) bvunite(rd_globals,defs[i],dsize);
        if(vilist[i]->flags&USEDASADR) bvunite(rd_address,defs[i],dsize);
        if(vilist[i]->storage_class==STATIC) bvunite(rd_statics,defs[i],dsize);
        if(i<rcount){
            bvunite(rd_address,defs[i+vcount-rcount],dsize);
            bvunite(rd_globals,defs[i+vcount-rcount],dsize);
            bvunite(rd_drefs,defs[i+vcount-rcount],dsize);
        }
    }
    rd_parms=mymalloc(dsize);
    memset(rd_parms,0,dsize);
    for(i=0;i<vcount;i++){
        struct Var *v=vilist[i];
        if(i>=vcount-rcount||zl2l(v->offset)<0||(v->flags&REGPARM)||v->nesting==0||v->storage_class==EXTERN||v->storage_class==STATIC){
            BSET(rd_parms,i+dcount+1);
        }
    }
/*    if(DEBUG&1024){printf("rd_globals:\n");print_rd(rd_globals);}*/
}
int complete_def(struct IC *p)
/*  Testet, ob eine Definition die Zielvariable komplett setzt oder nur */
/*  teilweise.                                                          */
{
    struct Typ *t=p->z.v->vtyp;
    zlong s1,s2;
    if(p->z.flags&DREFOBJ) t=t->next;
    if(!t) return(0);
    s1=szof(t);
    if(p->code==ASSIGN||p->code==GETRETURN) s2=p->q2.val.vlong;
        else s2=sizetab[p->typf&NQ];
/*    if(s1<s2) ierror(0);*/
    if(zleqto(s1,s2)) return(1); else return(0);
}
void reaching_definitions(struct flowgraph *fg)
/*  Berechnet die verfuegbaren Definitionen fuer jeden Block.   */
{
    struct flowgraph *g;struct IC *p;unsigned char *tmp;
    int changed,pass,i,j;
    /*  rd_gen und rd_kill fuer jeden Block berechnen   */
    if(DEBUG&1024) printf("analysing reaching definitions\n");
    tmp=mymalloc(dsize);
    g=fg;
    while(g){
        g->rd_in=mymalloc(dsize);
        memset(g->rd_in,0,dsize);
        g->rd_out=mymalloc(dsize);
        memset(g->rd_out,0,dsize);
        g->rd_gen=mymalloc(dsize);
        memset(g->rd_gen,0,dsize);
        g->rd_kill=mymalloc(dsize);
        memset(g->rd_kill,0,dsize);
        p=g->end;
        while(p){
            if(p->defindex){
                int zi=-1;
                if(!BTST(g->rd_kill,p->defindex)) BSET(g->rd_gen,p->defindex);
                if(p->z.flags&VAR){
                    if(!BTST(g->rd_kill,p->defindex)&&complete_def(p)){
                        zi=p->z.v->index;
                        if(p->z.flags&DREFOBJ) zi+=vcount-rcount;
                        memcpy(tmp,defs[zi],dsize);
                        bvdiff(tmp,g->rd_gen,dsize);
                        bvunite(g->rd_kill,tmp,dsize);
                    }
                }
                for(j=0;j<p->change_cnt;j++){
                    i=p->change_list[j].v->index;
                    if(p->change_list[j].flags&DREFOBJ) i+=vcount-rcount;
                    if(i>=vcount) continue;
                    if((i!=zi||(p->typf&NQ)>POINTER)&&!BTST(g->rd_kill,i+dcount+1)){
                        BSET(g->rd_gen,i+dcount+1);
                        if(i<rcount&&!BTST(g->rd_kill,dcount+1+i+vcount-rcount))
                            BSET(g->rd_gen,i+vcount-rcount+dcount+1);
                    }
                }
            }

            if(p==g->start) break;
            p=p->prev;
        }
        memcpy(g->rd_out,g->rd_gen,dsize);
        g=g->normalout;
    }

    /*  rd_in und rd_out fuer jeden Block berechnen */
    /*  out(b)=gen(B) vorinitialisiert              */
    if(DEBUG&1024) {printf("pass:");pass=0;}
    do{
        if(DEBUG&1024) {printf(" %d",++pass);fflush(stdout);}
        changed=0;
        g=fg;
        while(g){
            struct flowlist *lp;
            /*  in(B)=U out(C) : C Vorgaenger von B */
            if(g==fg) memcpy(g->rd_in,rd_parms,dsize);
                else  memset(g->rd_in,0,dsize);
            lp=g->in;
            while(lp){
                if(!lp->graph) ierror(0);
                if(lp->graph->branchout==g||!lp->graph->end||lp->graph->end->code!=BRA)
                    bvunite(g->rd_in,lp->graph->rd_out,dsize);
                lp=lp->next;
            }
            /*  out(b)=gen(B) U (in(B)-kill(B)  */
            memcpy(tmp,g->rd_in,dsize);
            bvdiff(tmp,g->rd_kill,dsize);
            bvunite(tmp,g->rd_gen,dsize);
            if(!bvcmp(tmp,g->rd_out,dsize)){changed=1;memcpy(g->rd_out,tmp,dsize);}
            g=g->normalout;
        }
    }while(changed);
    if(DEBUG&1024) printf("\n");
    free(tmp);
}
void calc(int c,int t,union atyps *q1,union atyps *q2,union atyps *z,struct IC *p)
/*  berechnet z:=q1 op q2; mit Konstanten                           */
{
    zdouble d1,d2;zlong l1,l2;zulong u1,u2;
    eval_const(q1,t);
    d1=vdouble;l1=vlong;u1=vulong;
    if(c!=MINUS&&c!=KOMPLEMENT){
        eval_const(q2,t);
        d2=vdouble;l2=vlong;u2=vulong;
    }
    if(c==ADD){ vdouble=zdadd(d1,d2);vlong=zladd(l1,l2);vulong=zuladd(u1,u2);}
    if(c==SUB){ vdouble=zdsub(d1,d2);vlong=zlsub(l1,l2);vulong=zulsub(u1,u2);}
    if(c==MULT){ vdouble=zdmult(d1,d2);vlong=zlmult(l1,l2);vulong=zulmult(u1,u2);}
    if(c==DIV||c==MOD){
        if(zdeqto(d2,d2zd(0.0))&&zleqto(l2,l2zl(0L))&&zuleqto(u2,ul2zul(0UL))){
            err_ic=p;error(210);err_ic=0;
            vlong=l2zl(0L);vulong=ul2zul(0L);vdouble=zl2zd(l1);
        }else{
            if(c==DIV){vdouble=zddiv(d1,d2);if(!zleqto(l2,l2zl(0L))) vlong=zldiv(l1,l2);if(!zuleqto(u2,ul2zul(0UL))) vulong=zuldiv(u1,u2);}
             else     {if(!zleqto(l1,l2zl(0L))) vlong=zlmod(l1,l2);if(!zuleqto(u2,ul2zul(0UL))) vulong=zulmod(u1,u2);}
        }
    }
    if(c==AND){ vlong=zland(l1,l2);vulong=zuland(u1,u2);}
    if(c==OR){ vlong=zlor(l1,l2);vulong=zulor(u1,u2);}
    if(c==XOR){ vlong=zlxor(l1,l2);vulong=zulxor(u1,u2);}
    if(c==LSHIFT){ vlong=zllshift(l1,l2);vulong=zullshift(u1,u2);}
    if(c==RSHIFT){ vlong=zlrshift(l1,l2);vulong=zulrshift(u1,u2);}
    if(c==MINUS){ vdouble=zdsub(d2zd(0.0),d1);vlong=zlsub(l2zl(0L),l1);vulong=zulsub(ul2zul(0UL),u1);}
    if(c==KOMPLEMENT){ vlong=zlkompl(l1);vulong=zulkompl(u1);}

    vint=zl2zi(vlong);vshort=zl2zs(vlong);vchar=zl2zc(vlong);
    vuint=zul2zui(vulong);vushort=zul2zus(vulong);vuchar=zul2zuc(vulong);
    vfloat=zd2zf(vdouble);
    insert_const2(z,t);
}

int fold(struct IC *p)
/*  wertet konstante ICs aus    */
{
    int c;
    if(!p) ierror(0);
    c=p->code;
    if(c==ADDI2P||c==SUBIFP||c==SUBPFP||c==ASSIGN||c==PUSH||c==SETRETURN) return(0);
    if(DEBUG&1024) {printf("folding IC:\n");pric2(stdout,p);}
    if(c==TEST||c==COMPARE){
        union atyps val;int cc; /*  condition codes */
        struct IC *bp;
        if(c==TEST){
            eval_const(&p->q1.val,p->typf);
            if(zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))&&zdeqto(vdouble,d2zd(0.0))&&zpeqto(vpointer,zul2zp(ul2zul(0UL)))) cc=0; else cc=1;
        } else cc=compare_const(&p->q1.val,&p->q2.val,p->typf);
        bp=p->next;
        if(bp->code>=BEQ&&bp->code<=BGT&&(!p->z.flags||p->z.v==bp->q1.v)){
            if(DEBUG&1024) printf("(cc=%d; comparison eliminated)\n",cc);
            if(have_alias){ free(p->use_list);free(p->change_list);}
            remove_IC(p);
            while(1){   /*  zugehoerigen Branch suchen  */
                if(!bp||bp->code==LABEL||bp->code==BRA) ierror(0);
                c=bp->code;
                if(c>=BEQ&&c<=BGT) break;
                bp=bp->next;
            }
            if((c==BEQ&&cc==0)||(c==BNE&&cc!=0)||(c==BLT&&cc<0)||(c==BGT&&cc>0)||(c==BLE&&cc<=0)||(c==BGE&&cc>=0)){
                if(DEBUG&1024){ printf("changed following branch to BRA:\n");pric2(stdout,bp);}
                bp->code=BRA;
            }else{
                if(DEBUG&1024){ printf("removed following branch:\n");pric2(stdout,bp);}
                if(have_alias){ free(bp->use_list);free(bp->change_list);}
                remove_IC(bp);
            }
            return(1);
        }
    }
    if(c>=CONVCHAR&&c<=CONVULONG){
        int t;
        if(c==CONVCHAR) t=CHAR;
        if(c==CONVUCHAR) t=UNSIGNED|CHAR;
        if(c==CONVSHORT) t=SHORT;
        if(c==CONVUSHORT) t=UNSIGNED|SHORT;
        if(c==CONVINT) t=INT;
        if(c==CONVUINT) t=UNSIGNED|INT;
        if(c==CONVLONG) t=LONG;
        if(c==CONVULONG) t=UNSIGNED|LONG;
        if(c==CONVFLOAT) t=FLOAT;
        if(c==CONVDOUBLE) t=DOUBLE;
        if(c==CONVPOINTER) t=POINTER;
        eval_const(&p->q1.val,t);
        insert_const2(&p->q1.val,p->typf);
    }else calc(c,p->typf,&p->q1.val,&p->q2.val,&p->q1.val,p);
    p->q2.flags=0;
    p->q2.val.vlong=sizetab[p->typf&NQ];
    p->code=ASSIGN;
    if(DEBUG&1024){printf("becomes\n");pric2(stdout,p);}
    return(1);
}
int propagate(struct obj *o,int replace)
/*  versucht, Objekte durch Konstanten zu ersetzen und erkennt manche   */
/*  nicht initialisierten Variablen; ist replace==0, wird nicht ersetzt */
{
    unsigned int i,j,t;union atyps *val=0;
    struct Var *v;struct IC *p;
    if(!o||!o->v) ierror(0);
    v=o->v;
    i=v->index;
    if(o->flags&DREFOBJ) i+=vcount-rcount;
    if(DEBUG&2048){
        printf("propagate <%s>\n",o->v->identifier);
        if(o->flags&DREFOBJ) printf("(DREFOBJ)");
        printf("\nall reaching definitions:\n");print_rd(rd_defs);
        printf("definitions of object:\n");print_rd(defs[i]);
    }
    if(v->nesting==0||v->storage_class==STATIC||v->storage_class==EXTERN){
    /*  Wenn moeglich bei statischen Variablen den Wert bei der         */
    /*  Initialisierung ermitteln.                                      */
        if(replace&&(v->vtyp->flags&NQ)<=DOUBLE&&((v->vtyp->flags&CONST)||(v->nesting>0&&!(v->flags&(USEDASADR|USEDASDEST))))){
            /*  Variable hat noch den Wert der Initialisierung.         */
            if(v->clist){
                /*  Der Wert der Initialisierung ist noch gespeichert.  */
                if(DEBUG&1024) printf("using static initializer\n");
                o->val=v->clist->val;
                o->flags=KONST;
                return(1);
            }else{
                /*  Hier evtl. eine implizite 0 erkennen.               */
            }
        }
    }
    if(BTST(rd_defs,i+dcount+1)) return(0);
    memcpy(rd_tmp,rd_defs,dsize);
    bvintersect(rd_tmp,defs[i],dsize);
    i=0;
    for(j=1;j<=dcount;j++){
        if(BTST(rd_tmp,j)){
            p=dlist[j];
            /*  Die Def. muss mit dem Objekt uebereinstimmen (koennte   */
            /*  auch ein anderes Struktur- oder Arrayy-Element sein).   */
            /*  Dann uebergehen, aber merken, dass eine Def. existiert, */
            /*  um keine falsche "used before defined"-Meldung zu       */
            /*  erzeugen.                                               */
            if(compare_objs(&p->z,o,p->typf)){ i=1; continue;}
            /*  Wenn die Def. kein einfacher Typ ist, muss sie von      */
            /*  einer evtl. spaeteren Def. ueberschrieben worden sein,  */
            /*  da oben schon auf undefined geprueft wurde.             */
            if((p->typf&NQ)>POINTER){ i=1; continue; }
            /*  Wenn es keine Zuweisung einer Konstanten ist, ist keine */
            /*  Weitergabe von Konstanten moeglich.                     */
            if(p->code!=ASSIGN) return(0);
            if(!(p->q1.flags&KONST)) return(0);
            if(val){
                /*  Wenn mehr als eine Konstante, muessen alle gleich sein  */
                /*  und den gleichen Typ haben.                             */
                if((p->typf&NU)!=t) return(0);
                if(compare_const(&p->q1.val,val,t)) return(0);
            }else{
                val=&p->q1.val;t=p->typf&NU;
            }
        }
    }
    /*  kann Konstante einsetzen    */
    if(val){
        if(!replace) return(0);
        if(DEBUG&1024){ printf("can <%s> replace by constant\n",o->v->identifier);}
        o->val=*val;
        o->flags=KONST;
        return(1);
    }
    /*  gar keine Definition gefunden   */
    if(!i&&!(v->flags&USEDBEFORE)){
        error(171,v->identifier);v->flags|=USEDBEFORE;
        if(!*v->identifier) {probj(stdout,o,0);prd(stdout,v->vtyp);printf("; offset=%ld\n",zl2l(v->offset));}
    }
    return(0);

}
int constant_propagation(struct flowgraph *fg,int global)
/*  sucht nach konstanten Ausdruecken und nicht definierten Variablen   */
/*  wenn global!=0, dann werden reaching definitions benutzt und global */
/*  optimiert, ansonsten nur innerhalb von basic blocks                 */
{
    struct IC *p;int changed=0,i;struct flowgraph *g;
    if(DEBUG&1024){printf("trying constant propagation\n");}
    rd_defs=mymalloc(dsize);
    rd_tmp=mymalloc(dsize);
    g=fg;
    while(g){
        if(global) memcpy(rd_defs,g->rd_in,dsize);
            else   memset(rd_defs,~0,dsize);
        p=g->start;
        while(p){
/*            if(DEBUG&1024){print_rd(rd_defs);pric2(stdout,p);}*/
            if(p->code!=ADDRESS&&p->code!=NOP&&(p->typf&NQ)<=POINTER&&(p->code<LABEL||p->code>BRA)){
                int i;
                if((p->q1.flags&(VAR|VARADR))==VAR){
                    i=p->q1.v->index;
                    if(p->q1.flags&DREFOBJ) i+=vcount-rcount;
                    changed|=propagate(&p->q1,1);
                }
                if((p->q2.flags&(VAR|VARADR))==VAR){
                    i=p->q2.v->index;
                    if(p->q2.flags&DREFOBJ) i+=vcount-rcount;
                    changed|=propagate(&p->q2,1);
                }
            }
            /*  sollte eigentlich nichts bringen, ausser evtl. Meldung  */
            /*  bei unitiialisierter Variablen                          */
            if(((p->z.flags&(VAR|DREFOBJ))==(VAR|DREFOBJ))){
                i=p->z.v->index+vcount-rcount;
                if(!BTST(rd_defs,i+dcount+1))
                    changed|=propagate(&p->z,0);
            }
            rd_change(p);

            if(p==g->end) break;
            p=p->next;
        }
        g=g->normalout;
    }

    gchanged|=changed;
    for(i=0;i<vcount;i++) free(defs[i]);
    free(defs);
    free(dlist);
    free(rd_globals);
    free(rd_statics);
    free(rd_address);
    free(rd_drefs);
    free(rd_tmp);
    free(rd_defs);
    free(rd_parms);
    free(rd_vars);
    return(changed);
}
void rd_change(struct IC *p)
/*  Fuehrt die Aenderungen in rd_defs durch, die bei IC p auftreten.    */
/*  Benutzt eine ganze Reihe globaler Bitvektoren.                      */
{
    int i,j,zi=-1;
/*    print_rd(rd_defs);*/
    if(p->defindex){
        if(p->z.flags&VAR){
            zi=p->z.v->index;
            if(p->z.flags&DREFOBJ) zi+=vcount-rcount;
        }
        for(j=0;j<p->change_cnt;j++){
            i=p->change_list[j].v->index;
            if(p->change_list[j].flags&DREFOBJ) i+=vcount-rcount;
            if(i>=vcount) continue;
            if(i!=zi||(p->typf&NQ)>POINTER){
                BSET(rd_defs,i+dcount+1);
                if(i<rcount) BSET(rd_defs,i+vcount-rcount+dcount+1);
            }
        }
        if(zi>=0&&complete_def(p)){
            bvdiff(rd_defs,defs[zi],dsize);
        }
        BSET(rd_defs,p->defindex);
    }
/*    pric2(stdout,p);*/
}

