/*  $VER: vbcc (opt.c) V0.4     */
/*  allgemeine Routinen fuer den Optimizer und Steuerung der einzelnen  */
/*  Laeufe                                                              */

#include "opt.h"

static char FILE_[]=__FILE__;

/*  die naechsten Funktionen sollten evtl. in ic.c                  */

/*  Sind use/change-Listen initialisiert?   */
int have_alias;

void insert_IC(struct IC *p,struct IC *new)
/*  fuegt new hinter p ein; p darf 0 sein                           */
{
    new->prev=p;
    if(p){ new->next=p->next; p->next=new; }
     else{ new->next=first_ic; first_ic=new; }
    if(new->next) new->next->prev=new; else last_ic=new;
    new->q1.am=new->q2.am=new->z.am=0;
}

#ifndef NO_OPTIMIZER

int gchanged;   /*  Merker, ob Optimierungslauf etwas geaendert hat */
int norek;      /*  diese Funktion wird nicht rekursiv auf          */
int nocall;     /*  diese Funktion kehrt nicht zum Caller zurueck   */

/*  temporary fuer verschiedene Bitvektoren */
unsigned char *tmp;

void recalc_offsets(struct flowgraph *fg)
/*  berechnet Offsets fuer auto-Variablen neu und versucht, fuer Variablen, */
/*  die nicht gleichzeitig aktiv sind, den gleichen Platz zu belegen        */
{
    int i,b,*eqto;size_t bsize;zlong *al,*sz;
    unsigned char **used,*tmp,*empty;
    struct IC *p;
    if(DEBUG&1024) printf("recalculating offsets\n");
    if(DEBUG&1024) printf("setting up arrays\n");
    bsize=(basic_blocks+CHAR_BIT-1)/CHAR_BIT;
    if(DEBUG&1024) printf("bsize=%lu\n",(unsigned long)bsize);
    tmp=mymalloc(bsize);
    al=mymalloc(sizeof(*al)*(vcount-rcount));
    eqto=mymalloc(sizeof(int)*(vcount-rcount));
    sz=mymalloc(sizeof(*sz)*(vcount-rcount));
    empty=mymalloc(bsize);
    memset(empty,0,bsize);
    used=mymalloc(sizeof(unsigned char *)*(vcount-rcount));
    /*  Tabelle, welche Variable in welchem Block belegt ist, aufbauen  */
    for(i=0;i<vcount-rcount;i++){
        if(zlleq(l2zl(0L),vilist[i]->offset)&&(vilist[i]->storage_class==AUTO||vilist[i]->storage_class==REGISTER)){
            if(DEBUG&2048) printf("setting up for %s,%ld\n",vilist[i]->identifier,zl2l(vilist[i]->offset));
            used[i]=mymalloc(bsize);
            memset(used[i],0,bsize);
        }else{
            used[i]=0;
        }
        sz[i]=szof(vilist[i]->vtyp);
        al[i]=falign(vilist[i]->vtyp);
        eqto[i]=-1;
    }
    b=0;
    while(fg){
        if(b>=basic_blocks) ierror(0);
        for(i=0;i<vcount-rcount;i++){
            if(used[i]&&(BTST(fg->av_in,i)||BTST(fg->av_out,i))){
                int r;
                BSET(used[i],b);
                for(r=1;r<=MAXR;r++)
                    if(fg->regv[r]&&fg->regv[r]->index==i) BCLR(used[i],b);
            }
        }
        for(p=fg->start;p;p=p->next){
            if((p->q1.flags&(VAR|REG))==VAR){
                i=p->q1.v->index;
                if(used[i]){
                    BSET(used[i],b);
                }
            }
            if((p->q2.flags&(VAR|REG))==VAR){
                i=p->q2.v->index;
                if(used[i]){
                    BSET(used[i],b);
                }
            }
            if((p->z.flags&(VAR|REG))==VAR){
                i=p->z.v->index;
                if(used[i]){
                    BSET(used[i],b);
                }
            }
            if(p==fg->end) break;
        }
        fg=fg->normalout;
        b++;
    }
    /*  schauen, ob Variablen in gleichen Speicher koennen  */
    if(DEBUG&1024) printf("looking for distinct variables\n");
    for(i=0;i<vcount-rcount;i++){
        if(!used[i]||eqto[i]>=0) continue;
        if(!memcmp(used[i],empty,bsize)){ free(used[i]);used[i]=0;continue;}
        for(b=i+1;b<vcount-rcount;b++){
            if(!used[b]||eqto[b]>=0) continue;
            if(DEBUG&2048) printf("comparing %s(%ld) and %s(%ld)\n",vilist[i]->identifier,zl2l(vilist[i]->offset),vilist[b]->identifier,zl2l(vilist[b]->offset));

            memcpy(tmp,used[i],bsize);
            bvintersect(tmp,used[b],bsize);
            if(!memcmp(tmp,empty,bsize)){
                if(DEBUG&1024) printf("memory for %s(%ld) and %s(%ld) equal\n",vilist[i]->identifier,zl2l(vilist[i]->offset),vilist[b]->identifier,zl2l(vilist[b]->offset));
                eqto[b]=i;
                if(!zlleq(al[b],al[i])) al[i]=al[b];
                if(!zlleq(sz[b],sz[i])) sz[i]=sz[b];
                bvunite(used[i],used[b],bsize);
            }
        }
    }
    if(DEBUG&1024) printf("final recalculating\n");
    max_offset=l2zl(0L);
    for(i=0;i<vcount-rcount;i++){
        if(!used[i]) continue;
        free(used[i]);
        if(DEBUG&2048) printf("adjusting offset for %s,%ld\n",vilist[i]->identifier,zl2l(vilist[i]->offset));
        if(eqto[i]>=0){
            vilist[i]->offset=vilist[eqto[i]]->offset;
            continue;
        }
        vilist[i]->offset=zlmult(zldiv(zladd(max_offset,zlsub(al[i],l2zl(1L))),al[i]),al[i]);
        max_offset=zladd(vilist[i]->offset,sz[i]);
    }
    free(used);
    free(sz);
    free(al);
    free(tmp);
    free(empty);
    free(eqto);
}
void remove_IC_fg(struct flowgraph *g,struct IC *p)
/*  Entfernt IC p und beachtet Flussgraph. Ausserdem werden             */
/*  use/change-Listen freigegeben.                                      */
{
    if(p->q1.am||p->q2.am||p->z.am) ierror(0);
    if(have_alias){
        free(p->use_list);
        free(p->change_list);
    }
    if(g->start==g->end){
        g->start=g->end=0;
    }else{
        if(p==g->end) g->end=p->prev;
        if(p==g->start) g->start=p->next;
    }
    remove_IC(p);
}

int peephole(void)
/*  macht alle moeglichen Vereinfachungen/Vereinheitlichungen   */
{
    struct IC *p;struct obj o;int t,c,null,eins,changed,done=0;
    do{
        if(DEBUG&1024) printf("searching for peephole optimizations\n");
        changed=0;ic_count=0;
        p=first_ic;
        while(p){
            c=p->code;
            t=p->typf;
            ic_count++;
	    if(c==LABEL&&report_suspicious_loops&&p->next&&p->next->code==BRA&&p->next->typf==t){
	      error(208);report_suspicious_loops=0;
	    }
            if(p->q1.flags&KONST){
                if((p->q2.flags&KONST)||!p->q2.flags){
                    struct IC *old=p->prev;
                    if(fold(p)){ changed=1; p=old;continue;}
                    p=p->next;continue;
                }else{
                    if(c==ADD||c==MULT||(c>=OR&&c<=AND)){ /*  const nach rechts   */
                        if(DEBUG&1024){ printf("swapped commutative op:\n");pric2(stdout,p);}
                        o=p->q1;p->q1=p->q2;p->q2=o;
                    }
                }
            }
            if(p->q2.flags&KONST){
            /*  algebraische Optimierungen  */
                eval_const(&p->q2.val,t);
                if(zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))&&zdeqto(vdouble,d2zd(0.0))) null=1; else null=0;
                if(zleqto(vlong,l2zl(1L))&&zuleqto(vulong,ul2zul(1UL))&&zdeqto(vdouble,d2zd(1.0))) eins=1; else eins=0;
                if(zleqto(vlong,l2zl(-1L))&&zdeqto(vdouble,d2zd(-1.0))) eins=-1;
                if(eins<0&&(c==MULT||c==DIV)){
                    if(DEBUG&1024){ printf("MULT/DIV with (-1) converted to MINUS:\n");pric2(stdout,p);}
                    p->code=c=MINUS;p->q2.flags=0;
                    changed=1;
                }
#if 0
                if(c==SUB){
                /*  VORSICHT: Das funktioniert bei bestimmten Werten nicht! */
                    if(DEBUG&1024){ printf("SUB converted to ADD:\n");pric2(stdout,p);}
                    p->code=c=ADD; calc(MINUS,t,&p->q2.val,0,&p->q2.val,p);
                    changed=1;
                }
#endif
                if((eins>0&&(c==MULT||c==DIV))||(null&&(c==ADD||c==SUB||c==ADDI2P||c==SUBIFP||c==LSHIFT||c==RSHIFT||c==OR||c==XOR))){
                    if(DEBUG&1024){ printf("operation converted to simple assignment:\n");pric2(stdout,p);}
                    if(c==ADDI2P||c==SUBIFP) p->typf=t=POINTER;
                    p->code=c=ASSIGN;p->q2.flags=0;p->q2.val.vlong=sizetab[t&NQ];
                    changed=1;
                }
                if(null&&(c==MULT||c==DIV||c==MOD||c==AND)){
                    if(c==DIV||c==MOD){ err_ic=p;error(210);err_ic=0;}
                    if(DEBUG&1024){ printf("operation converted to ASSIGN 0:\n");pric2(stdout,p);}
                    o.val.vlong=l2zl(0L);eval_const(&o.val,LONG);
                    insert_const2(&p->q1.val,t);p->q1.flags=KONST;
                    p->code=c=ASSIGN;p->q2.flags=0;p->q2.val.vlong=sizetab[t&NQ];
                    changed=1;
                }
                if(((t&NQ)<=LONG||fp_assoc)&&(c==ADD||c==ADDI2P||c==MULT||c==LSHIFT||c==RSHIFT||c==OR||c==AND)){
                /*  assoziative Operatoren  */
                    struct IC *n=p->next;
                    if(n&&n->code==c&&(n->q2.flags&KONST)&&n->typf==t&&n->q1.flags==p->z.flags&&n->q1.v==p->z.v&&zleqto(n->q1.val.vlong,p->z.val.vlong)){
                        if(DEBUG&1024){ printf("using associativity with:\n");pric2(stdout,p);pric2(stdout,p->next);}
                        n->q1=p->q1;
                        if(c==LSHIFT||c==RSHIFT||c==ADDI2P)
                            calc(ADD,t,&p->q2.val,&n->q2.val,&n->q2.val,0);
                        else
                            calc(c,t,&p->q2.val,&n->q2.val,&n->q2.val,0);
                        changed=1;
                        if(p->q1.flags==p->z.flags&&p->q1.v==p->z.v&&zleqto(p->q1.val.vlong,p->z.val.vlong)){
                            if(DEBUG&1024) printf("must remove first operation\n");
                            n=p;p=p->next;
                            if(have_alias){ free(n->use_list); free(n->change_list); }
                            remove_IC(n);continue;
                        }
                    }
                }
                if((c==ADDI2P||c==SUBIFP)&&(p->q1.flags&VARADR)){
                /*  add #var,#const -> move #var+const      */
                    union atyps val;
                    if(DEBUG&1024){printf("add/sub #var,#const changed to assign:\n");pric2(stdout,p);}
                    eval_const(&p->q2.val,t);
                    insert_const2(&val,LONG);
                    if(c==ADDI2P) calc(ADD,LONG,&p->q1.val,&val,&p->q1.val,0);
                        else      calc(SUB,LONG,&p->q1.val,&val,&p->q1.val,0);
                    p->code=c=ASSIGN;
                    p->q2.flags=0;
                    p->typf=t=POINTER;
                    p->q2.val.vlong=sizetab[t&NQ];
                    changed=1;
                }
                if((c==ADD||c==SUB)&&(t&NQ)<=LONG&&p->next&&p->next->next){
                    struct IC *p1=p->next,*p2=p1->next;
                    if(p1->code==MULT&&p2->code==ADDI2P&&
                       p1->typf==t&&p2->typf==t&&
                       (p1->q2.flags&KONST)&&(p->z.flags&(SCRATCH|DREFOBJ))==SCRATCH&&(p1->z.flags&(SCRATCH|DREFOBJ))==SCRATCH&&
                       !compare_objs(&p->z,&p1->q1,t)&&
                       !compare_objs(&p1->z,&p2->q2,t)){
                        if(DEBUG&1024){ printf("rearranging array-access:\n");pric2(stdout,p);pric2(stdout,p1);pric2(stdout,p2);}
                        p1->q1=p->q1;
                        p->q1=p2->q1;
                        p2->q1=p2->z;
                        p->z=p2->z;
                        calc(MULT,t,&p->q2.val,&p1->q2.val,&p->q2.val,0);
                        if(c==ADD) p->code=ADDI2P; else p->code=SUBIFP;
                        changed=1;continue;
                    }
                }
            }
            if(p->q1.flags&KONST){
            /*  algebraische Optimierungen  */
                eval_const(&p->q1.val,t);
                if(zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))&&zdeqto(vdouble,d2zd(0.0))) null=1; else null=0;
                if(null&&(c==DIV||c==MOD||c==LSHIFT||c==RSHIFT)){
                    if(DEBUG&1024){ printf("operation converted to ASSIGN 0:\n");pric2(stdout,p);}
                    o.val.vlong=l2zl(0L);eval_const(&o.val,LONG);
                    insert_const2(&p->q1.val,t);p->q1.flags=KONST;
                    p->code=c=ASSIGN;p->q2.flags=0;p->q2.val.vlong=sizetab[t&NQ];
                    changed=1;
                }
            }
            if(!USEQ2ASZ&&p->z.flags&&!compare_objs(&p->q2,&p->z,p->typf)){
                if(c==ADD||c==MULT||(c>=OR&&c<=AND)){
                    struct obj o;
                    if(DEBUG&1024){printf("swapping objs because USEQ2ASZ\n");pric2(stdout,p);}
                    o=p->q2;p->q2=p->q1;p->q1=o;
                    /*  kein changed hier!  */
                }else{pric2(stdout,p); ierror(0);}
            }
            if((c==ADD||c==SUB)&&p->next){
                struct IC *p1=p->next;
                if(p1->code==ADDI2P&&p1->typf==t&&(p->z.flags&(SCRATCH|DREFOBJ))==SCRATCH&&!compare_objs(&p->z,&p1->q2,t)){
                    if(DEBUG&1024){ printf("rearranging array-access:\n");pric2(stdout,p);pric2(stdout,p1);}
                    p1->q2=p->q1;
                    p->q1=p1->q1;
                    p->z=p1->z;
                    p1->q1=p1->z;
                    if(c==ADD) p->code=c=ADDI2P; else p->code=c=SUBIFP;
                    changed=1;continue;
                }
            }
            if((c==SUB||c==DIV||c==MOD)&&!compare_objs(&p->q1,&p->q2,p->typf)){
                /*  x-x=0, x/x=1, x%x=0 */
                if(DEBUG&1024){ printf("i-i, i/i, i%%i converted to ASSIGN 0/1:\n");pric2(stdout,p);}
                if(c==DIV) o.val.vlong=l2zl(1L); else o.val.vlong=l2zl(0L);
                eval_const(&o.val,LONG);insert_const2(&p->q1.val,t);p->q1.flags=KONST;
                p->code=c=ASSIGN;p->q2.flags=0;p->q2.val.vlong=sizetab[t&NQ];
                changed=1;
            }
            if(c==ASSIGN&&(p->z.flags&VAR)&&p->z.flags==p->q1.flags&&p->z.v==p->q1.v&&zleqto(p->z.val.vlong,p->q1.val.vlong)){
                struct IC *d;
                if(DEBUG&1024){ printf("removing redundant move:\n");pric2(stdout,p);}
                changed=1;
                d=p; p=p->next;
                if(have_alias){ free(d->use_list); free(d->change_list);}
                remove_IC(d); continue;
            }
            p=p->next;
        }
        if(changed) done|=changed;
        gchanged|=changed;
    }while(changed);
    return(done);
}

void insert_ccs(void)
/*  Fuegt Variablen fuer ccs ein.   */
{
    struct IC *p; struct Var *v; struct Typ *t;
    if(DEBUG&1024) printf("insert_ccs()\n");
    for(p=first_ic;p;p=p->next){
        if(p->code==COMPARE||p->code==TEST){
            p->z.flags=VAR;
            p->z.val.vlong=l2zl(0L);
            t=mymalloc(TYPS);
            t->flags=0;
            t->next=0;
            v=add_tmp_var(t);
            p->z.v=v;
            p=p->next;
            if(p->code<BEQ||p->code>BGT){
                p=p->prev;
                p->code=NOP;
                p->q1.flags=p->q2.flags=p->z.flags=0;
            }else{
                p->q1.flags=VAR;
                p->q1.val.vlong=l2zl(0L);
                p->q1.v=v;
            }
        }
    }
}

#endif
#define FREEAV free(av_globals);free(av_statics);free(av_drefs);free(av_address);
void optimize(long flags,struct Var *function)
/*  flags:   1=Register, 2=optimize, 4=cse/cp, 8=constant_propagation,  */
/*          16=dead_assignments, 32=global-optimizations                */
/*          64=blockweise Registervergabe, 128=loop_optimizations (nur  */
/*             in Verbindung mit 32), 256=recalc_offsets                */
{
#ifndef NO_OPTIMIZER
    struct flowgraph *fg=0;int r,pass=0;
    if(!function) ierror(0);
    norek=nocall=0;
    report_suspicious_loops=report_weird_code=1;
    if(!strcmp(function->identifier,"main")){norek=1;nocall=1;}
    /*  falls main() rekursiv aufgerufen werden kann, muss nomain==0 sein   */

#else

    flags&=1;

#endif
    if(flags&2){
#ifndef NO_OPTIMIZER
        /*  Variablen fuer ccs einsetzen.   */
        if(multiple_ccs) insert_ccs();
        /*  nur ein pass, wenn nur lokale Optimierungen */
        if(!(flags&32)) maxoptpasses=1;
        do{
            gchanged=0;pass++;
            av_globals=av_statics=av_address=av_drefs=0;
            rd_globals=rd_statics=rd_address=rd_drefs=0;
            ae_globals=ae_statics=ae_address=ae_drefs=0;
            cp_globals=cp_statics=cp_address=cp_drefs=0;
            dlist=0;vilist=0;elist=0;rd_parms=0;

            if(DEBUG&1024) printf("\noptimizer (function %s) pass %d\n",function->identifier,pass);
            num_vars();
            peephole();
            fg=jump_optimization();
            create_alias(fg);
            if(DEBUG&2048) print_vi();
            if(flags&8){
                do{
                    num_defs();
                    if(flags&32){
                        rd_mode=0;
                        reaching_definitions(fg);
                        if(DEBUG&1024) print_flowgraph(fg);
                    }
                    r=constant_propagation(fg,flags&32);
                    if(DEBUG&1024) {printf("constant_propagation returned %d\n",r);print_flowgraph(fg);}
                    if(r){
                        if(peephole()){free_flowgraph(fg);fg=jump_optimization();}
                    }
                }while(r);
            }
            if(flags&4){
                int repeat;
                do{
                    do{
                        num_exp();
                        if(DEBUG&1024) print_flowgraph(fg);
                        repeat=r=cse(fg,0);    /*  local cse   */
                        if(DEBUG&1024) printf("local cse returned %d\n",r);
                        gchanged|=r;
                        if(r){  /*  neue Variablen eingefuegt   */
                            if(DEBUG&1024) printf("must repeat num_vars\n");
                            free(vilist);
                            FREEAV;
                            num_vars();
                        }
                        do{
                            num_copies();
                            if(DEBUG&1024) print_flowgraph(fg);
                            r=copy_propagation(fg,0);   /*  copy propagation    */
                            if(DEBUG&1024) printf("local copy propagation returned %d\n",r);
                            if(r&2){
                                if(DEBUG&1024) printf("must repeat num_vars\n");
                                free(vilist);
                                FREEAV;
                                num_vars();
                            }
                            gchanged|=r;repeat|=r;
                        }while(r);
                    }while(repeat);
                    repeat=0;
                    if(flags&32){
                        num_exp();
                        if(DEBUG&1024) print_flowgraph(fg);
                        available_expressions(fg);
                        if(DEBUG&1024) print_flowgraph(fg);
                        r=cse(fg,1);gchanged|=r;repeat|=r;
                        if(DEBUG&1024) printf("global cse returned %d\n",r);
                        if(r){  /*  neue Variablen eingefuegt   */
                            if(DEBUG&1024) printf("must repeat num_vars\n");
                            free(vilist);
                            FREEAV;
                            num_vars();
                            gchanged|=r;repeat|=r;
                            do{
                                num_copies();
                                if(DEBUG&1024) print_flowgraph(fg);
                                r=copy_propagation(fg,0);   /*  copy propagation    */
                                if(DEBUG&1024) printf("local copy propagation returned %d\n",r);
                                if(r&2){
                                    if(DEBUG&1024) printf("must repeat num_vars\n");
                                    free(vilist);
                                    FREEAV;
                                    num_vars();
                                }
                                gchanged|=r;repeat|=r;
                            }while(r);
                        }
                        num_copies();
                        available_copies(fg);
                        if(DEBUG&1024) print_flowgraph(fg);
                        r=copy_propagation(fg,1);   /*  copy propagation    */
                        if(DEBUG&1024) printf("global copy propagation returned %d\n",r);
                        if(r&2){
                            if(DEBUG&1024) printf("must repeat num_vars\n");
                            free(vilist);
                            FREEAV;
                            num_vars();
                        }
                        gchanged|=r;repeat|=r;
                    }
                }while(0/*repeat*/);
            }
            if((flags&160)==160){
                r=loop_optimizations(fg);
                gchanged|=r;
                fg=jump_optimization();
            }
            if((flags&16)||((flags&1)&&pass>=maxoptpasses)){
/*                num_vars();*/
                free_alias(fg);
                create_alias(fg);
                active_vars(fg);
                if(DEBUG&1024) print_flowgraph(fg);
                if((flags&16)&&pass<=maxoptpasses){
                    r=dead_assignments(fg);
                    if(DEBUG&1024) printf("dead_assignments returned %d\n",r);
                    gchanged|=r;
                }
            }


            if((!gchanged||pass>=maxoptpasses)){
            /*  Funktion evtl. fuer inlining vorbereiten und    */
            /*  Registervergabe                                 */
                int varargs=0,c;
                if((c=function->vtyp->exact->count)!=0&&(*function->vtyp->exact->sl)[c-1].styp->flags!=VOID)
                    varargs=1;

                /*  default-Wert fuer inline-Entscheidung   */
                if(!varargs&&(flags&4096)&&(only_inline||ic_count<=inline_size)){
                /*  fuer function inlinig vorbereiten   */
                    struct IC *p,*new;
                    if(DEBUG&1024) printf("function <%s> prepared for inlining(ic_count=%d)\n",function->identifier,ic_count);
                    function->fi=new_fi();
                    function->fi->first_ic=first_ic;
                    function->fi->last_ic=last_ic;
                    first_ic=last_ic=0;
                    p=function->fi->first_ic;
                    while(p){
                        new=mymalloc(ICS);
                        memcpy(new,p,ICS);
                        if((p->code>=BEQ&&p->code<=BRA)||p->code==LABEL)
                            new->typf-=lastlabel;
                        add_IC(new);
                        p=p->next;
                    }
                    p=first_ic;first_ic=function->fi->first_ic;function->fi->first_ic=p;
                    p=last_ic;last_ic=function->fi->last_ic;function->fi->last_ic=p;
                    function->fi->vars=0;
                }
                if(flags&1){
                    local_regs(fg);
                    if(DEBUG&1024) print_flowgraph(fg);
                    loops(fg,1);
                    if(DEBUG&1024) print_flowgraph(fg);
                    fg=create_loop_headers(fg,1);
                    if(DEBUG&1024) print_flowgraph(fg);
                    fg=create_loop_footers(fg,1);
                    if(DEBUG&1024) print_flowgraph(fg);
                    loop_regs(fg);
                    if(DEBUG&1024) print_flowgraph(fg);
#if 0
                    if(flags&64){
                        block_regs(fg);
                        if(DEBUG&1024) print_flowgraph(fg);
                    }
#endif
                    insert_regs(fg);
                }
                if(flags&256) recalc_offsets(fg);
            }

            free_alias(fg);
            free_flowgraph(fg);
            free(vilist);
            FREEAV;

            if((flags&32)&&gchanged&&pass>=maxoptpasses) error(172,maxoptpasses);

        }while(gchanged&&pass<maxoptpasses);

        /*  nur, um nochmal ueberfluessige Labels zu entfernen  */
        fg=construct_flowgraph();
        free_flowgraph(fg);

        /*  Register bei Funktionsaufrufen sichern  */
        insert_saves();

#endif

    }else{
        /*  keine Optimierungen     */
        if(flags&1) simple_regs();
        load_simple_reg_parms();
    }
    lastlabel=label;
}

