/*  $VER: vbcc (cse.c) V0.4     */
/*  verfuegbare Ausdruecke und common subexpression elimination */

#include "opt.h"

static char FILE_[]=__FILE__;

/*  fuer verfuegbare Ausdruecke */
struct IC **elist;
unsigned int ecount;
size_t esize;
unsigned char *ae_globals,*ae_statics,*ae_address,*ae_drefs;
unsigned char **ae_kills;

void available_expressions(struct flowgraph *fg)
/*  berechnet die verfuegbaren Ausdruecke fuer jeden Block      */
{
    struct flowgraph *g;struct IC *p;unsigned char *tmp;
    int changed,pass,i,j;
    /*  ae_gen und ae_kill fuer jeden Block berechnen   */
    if(DEBUG&1024) printf("analysing available expressions\n");
    tmp=mymalloc(esize);
    g=fg;
    while(g){
        g->ae_in=mymalloc(esize);
        memset(g->ae_in,0,esize);
        g->ae_out=mymalloc(esize);
        memset(g->ae_out,0,esize);
        g->ae_gen=mymalloc(esize);
        memset(g->ae_gen,0,esize);
        g->ae_kill=mymalloc(esize);
        memset(g->ae_kill,0,esize);
        p=g->end;
        while(p){
            memset(tmp,0,esize);
            for(j=0;j<p->change_cnt;j++){
                i=p->change_list[j].v->index;
                if(p->change_list[j].flags&DREFOBJ) i+=vcount-rcount;
                if(i>=vcount) continue;
                bvunite(tmp,ae_kills[i],esize);
            }
            bvdiff(tmp,g->ae_gen,esize);
            bvunite(g->ae_kill,tmp,esize);
            i=p->expindex;
            if(i>=0&&!BTST(g->ae_kill,i)) BSET(g->ae_gen,i);

            if(p==g->start) break;
            p=p->prev;
        }
        if(g==fg){
            memset(g->ae_in,0,esize);
            memcpy(g->ae_out,g->ae_gen,esize);
        }else{
            memset(g->ae_out,UCHAR_MAX,esize);
            bvdiff(g->ae_out,g->ae_kill,esize);
        }
        g=g->normalout;
    }

    /*  ae_in und ae_out fuer jeden Block berechnen */
    /*  out(b)=U-gen(B) vorinitialisiert und        */
    /*  in(B0)=0, out(B0)=gen(B0)                   */
    if(DEBUG&1024) {printf("pass:");pass=0;}
    do{
        if(DEBUG&1024) {printf(" %d",++pass);fflush(stdout);}
        changed=0;
        g=fg->normalout;    /*  in B0 aendert sich nichts   */
        while(g){
            struct flowlist *lp;
            /*  in(B)=Schnitt out(P) mit P Vorgaenger von B */
            lp=g->in;
            i=0;    /*  Flag fuer ersten Vorgaenger */
            while(lp){
                if(!lp->graph) ierror(0);
                if(lp->graph->branchout==g||!lp->graph->end||lp->graph->end->code!=BRA){
                    if(i){
                        bvintersect(g->ae_in,lp->graph->ae_out,esize);
                    }else{
                        memcpy(g->ae_in,lp->graph->ae_out,esize);i=1;
                    }
                }
                lp=lp->next;
            }
            /*  out(b)=gen(B) U (in(B)-kill(B)  */
            memcpy(tmp,g->ae_in,esize);
            bvdiff(tmp,g->ae_kill,esize);
            bvunite(tmp,g->ae_gen,esize);
            if(!bvcmp(tmp,g->ae_out,esize)){changed=1;memcpy(g->ae_out,tmp,esize);}
            g=g->normalout;
        }
    }while(changed);
    if(DEBUG&1024) printf("\n");
    free(tmp);
}


int compare_objs(struct obj *o1,struct obj *o2,int t)
/*  Vergleicht die beiden Objekte; liefert 0, wenn sie gleich sind, sonst   */
/*  1 oder -1, um eine Ordnung darauf zu definieren                         */
{
    int i1,i2;
    i1=o1->flags;i2=o2->flags;
    if(i1<i2) return(-1);
    if(i1>i2) return(1);
    if(i1&KONST) return(compare_const(&o1->val,&o2->val,t));
    if(i1&VAR){
        i1=o1->v->index; i2=o2->v->index;
        if(i1<i2) return(-1);
        if(i1>i2) return(1);
        i1=zl2l(o1->val.vlong); i2=zl2l(o2->val.vlong);
        if(i1<i2) return(-1);
        if(i1>i2) return(1);
    }
    return(0);
}

int compare_exp(const void *a1,const void *a2)
/*  Stub fuer compare_objs, damit als Vergleichsfunktion fuer qsort geht */
{
    struct IC *p1,*p2;int i1,i2;
    p1=*((struct IC **)a1);p2=*((struct IC **)a2);
    if(!p1||!p2) ierror(0);
    i1=p1->code; i2=p2->code;
    if(i1<i2) return(-1);
    if(i1>i2) return(1);
    i1=p1->typf; i2=p2->typf;
    if(i1<i2) return(-1);
    if(i1>i2) return(1);
    i1=compare_objs(&p1->q1,&p2->q1,p1->typf);
    if(i1) return(i1);
    i1=compare_objs(&p1->q2,&p2->q2,p1->typf);
    return(i1);
}
void print_ae(unsigned char *exp)
{
    int i;
    if(!exp){ printf("available expressions not available\n"); return;}
    for(i=0;i<ecount;i++)
        if(BTST(exp,i))
            {printf("%3d,%3d: ",elist[i]->expindex,i);pric2(stdout,elist[i]);}
}
void num_exp(void)
/*  numeriert die Ausdruecke so, dass gleiche Ausdruecke die gleiche    */
/*  nummer erhalten                                                     */
{
    struct IC *p;int c,i;
    if(DEBUG&1024) printf("numerating expressions\n");
    ecount=0;
    if(DEBUG&1024){ printf("num_exp loop1\n");}
    for(p=first_ic;p;p=p->next){
        c=p->code;
        if(p->z.flags&&p->q1.flags&&c!=ASSIGN&&c!=MOVETOREG&&c!=MOVEFROMREG){
            p->expindex=ecount++;
            if(c==ADD||c==MULT||(c>=OR&&c<=AND)){
                if(p->q2.flags&&compare_objs(&p->q1,&p->q2,p->typf)<0&&(USEQ2ASZ||compare_objs(&p->q1,&p->z,p->typf))){
                    struct obj o;
                    o=p->q1;p->q1=p->q2;p->q2=o;
                }
            }
        }else p->expindex=-1;
    }
    elist=mymalloc(ecount*sizeof(struct IC *));
    if(DEBUG&1024){ printf("num_exp loop2\n");}
    for(p=first_ic;p;p=p->next){
        if(p->expindex>=0){
            elist[p->expindex]=p;
        }
    }
    esize=(ecount+CHAR_BIT-1)/CHAR_BIT;
    if(DEBUG&1024){ printf("%lu expressions, esize=%lu\nsorting expressions\n",(unsigned long)ecount,(unsigned long)esize);}
    if(ecount>1) qsort(elist,ecount,sizeof(struct IC *),compare_exp);
    if(DEBUG&1024){ printf("renumbering expressions\nnum_exp loop3\n");}
    if(ecount>0){   /*  Aufpassen, da ecount unsigned!  */
        for(c=0;c<ecount-1;c++){
            if(!compare_exp(&elist[c],&elist[c+1]))
                elist[c+1]->expindex=elist[c]->expindex;
        }
    }
    if(DEBUG&1024) printf("re-sorting expressions\n");
    /*  wieder in die richtige Reihenfolge bringen  */
    for(p=first_ic;p;p=p->next)
        if(p->expindex>=0) elist[p->expindex]=p;
    ae_globals=mymalloc(esize);
    memset(ae_globals,0,esize);
    ae_statics=mymalloc(esize);
    memset(ae_statics,0,esize);
    ae_address=mymalloc(esize);
    memset(ae_address,0,esize);
    ae_drefs=mymalloc(esize);
    memset(ae_drefs,0,esize);
    if(DEBUG&1024){ printf("num_exp loop4\n");}
    ae_kills=mymalloc(vcount*sizeof(unsigned char *));
    for(c=0;c<vcount;c++){
        ae_kills[c]=mymalloc(esize);
        memset(ae_kills[c],0,esize);
    }
    if(DEBUG&1024){ printf("num_exp loop5\n");}
    for(c=0;c<ecount;c++){
        struct Var *v;
/*        if(c<ecount-1&&elist[c]==elist[c+1]) continue;*/  /*  gleiche ueberspringen   */
        p=elist[c];
        if(p->code==ADDRESS) continue;
        if((p->q1.flags&(VAR|VARADR))==VAR){
            v=p->q1.v;
            i=v->index;
            BSET(ae_kills[i],c);
            if(p->q1.flags&DREFOBJ){ BSET(ae_kills[i+vcount-rcount],c);BSET(ae_drefs,c);}
            if(v->nesting==0||v->storage_class==EXTERN) BSET(ae_globals,c);
            if(v->storage_class==STATIC) BSET(ae_statics,c);
            if(v->flags&USEDASADR) BSET(ae_address,c);
        }
        if((p->q2.flags&(VAR|VARADR))==VAR){
            v=p->q2.v;
            i=v->index;
            BSET(ae_kills[i],c);
            if(p->q2.flags&DREFOBJ){ BSET(ae_kills[i+vcount-rcount],c);BSET(ae_drefs,c);}
            if(v->nesting==0||v->storage_class==EXTERN) BSET(ae_globals,c);
            if(v->storage_class==STATIC) BSET(ae_statics,c);
            if(v->flags&USEDASADR) BSET(ae_address,c);
        }
    }
}
void cse_replace(struct flowgraph *g,struct IC *p,struct IC *o,struct Var *v)
/*  ersetzt die cse bei o zu p mit Variable v   */
{
    struct IC *n;
    /*  Kopieranweisung erzeugen    */
    if(DEBUG&1024) printf("cse_replace\n");
    n=mymalloc(ICS);
    n->line=o->line;
    n->file=o->file;
    n->code=ASSIGN;
    n->typf=p->typf;
    n->expindex=n->defindex=-1;
    n->q1.flags=VAR;
    n->q1.v=v;
    n->q1.val.vlong=l2zl(0L);
    n->q2.flags=0;
    n->q2.val.vlong=szof(v->vtyp);
    n->z=o->z;
    /*  Die Kopieranweisung benutzt hoechstens, was die urspruengliche  */
    /*  Operation benutzt hat+die Hulfsvariable und aendert nur, was    */
    /*  die urspruengliche vorher geaendert hat.                        */
    if(have_alias){
        n->use_cnt=o->use_cnt+1;
        n->use_list=mymalloc(n->use_cnt*VLS);
        n->use_list[0].v=v;
        n->use_list[0].flags=0;
        memcpy(&n->use_list[1],o->use_list,o->use_cnt*VLS);
        n->change_cnt=o->change_cnt;
        n->change_list=o->change_list;
    }
    /*  evtl. FLussgraph korrigieren    */
    if(g->end==o) g->end=n;
    /*  einfuegen   */
    insert_IC(o,n);
    /*  Operation auf Hilfsvariable umlenken    */
    o->z=n->q1;
    /*  Operation aendert nun nur Hilfsvariable.   */
    if(have_alias){
        /*  Liste nicht freigeben, da sie umgebogen wird.   */
        o->change_cnt=1;
        o->change_list=mymalloc(VLS);
        o->change_list[0].v=v;
        o->change_list[0].flags=0;
    }
}
void cse_search(struct flowgraph *g,struct IC *p,struct IC *o,struct Var *v,int global,unsigned char *bmk)
/*  sucht die Quelle(n) fuer common subexpression und ersetzt sie   */
/*  bmk ist Buffer, um zu merken, welche Bloecke schon besucht sind */
{
    struct flowlist *lp;
    /*  Letzte Berechnung des Ausdrucks suchen, beginnend bei o */
    /*  bei global kann o auch 0 sein!                          */
/*    if(DEBUG&1024) printf("cse_search\n");*/
    if(global){
        if(BTST(bmk,g->index)) return;
    }
    while(o&&o->expindex!=p->expindex){
        if(o==g->start) break;
        o=o->prev;
    }
    if(!o&&!global) ierror(0);
    if(o&&o->expindex==p->expindex){
        if(!(o->z.flags&VAR)||o->z.v!=v)
            cse_replace(g,p,o,v);
        return;
    }
    if(!global) ierror(0);
    /*  Block als besucht markieren, wenn er durchsucht wurde. Der  */
    /*  erste Block bei globaler Suche muss beachtet werden und     */
    /*  Endlosschleifen vermieden werden.                           */
    if(o||!g->end) BSET(bmk,g->index);
    lp=g->in;
    while(lp){
        if(!lp->graph) ierror(0);
        if(lp->graph->branchout==g||!lp->graph->end||lp->graph->end->code!=BRA){
            cse_search(lp->graph,p,lp->graph->end,v,global,bmk);
        }
        lp=lp->next;
    }
}

int cse(struct flowgraph *fg,int global)
/*  common-subexpression-elimination; wenn global==0 werden nur einzelne    */
/*  Bloecke betrachtet                                                      */
{
    struct flowgraph *g;struct IC *p,*o;int changed,i,j;
    unsigned char *ae;struct Var *v;struct Typ *new;
    if(DEBUG&1024) printf("common-subexpression-elimination\n");
    changed=0;
    ae=mymalloc(esize);
    for(g=fg;g;g=g->normalout){
        if(!global) memset(ae,0,esize); else memcpy(ae,g->ae_in,esize);
        p=g->start;
        while(p){
            i=p->expindex;
            if(i>=0){
                if(i>=ecount) ierror(0);
                if(BTST(ae,i)){
                    if(DEBUG&1024){ printf("can eliminate common subexpression:\n");pric2(stdout,p);}
                    /*  Hilfsvariable erzeugen  */
                    new=mymalloc(TYPS);
                    if(p->code==ADDRESS||p->code==ADDI2P||p->code==SUBIFP) new->flags=POINTER;
                        else new->flags=p->typf;
                    if(p->code==COMPARE||p->code==TEST) new->flags=0;
                    if((new->flags&NQ)==POINTER){
                        new->next=mymalloc(TYPS);
                        new->next->flags=VOID;
                        new->next->next=0;
                    }else new->next=0;
                    v=add_tmp_var(new);
                    v->index=-1;
                    /*  Operation durch assign Hilfsvariable ersetzen   */
                    p->code=ASSIGN;
                    p->typf=new->flags;
                    p->q1.flags=VAR;
                    p->q1.v=v;
                    p->q1.val.vlong=l2zl(0L);
                    p->q2.flags=0;
                    p->q2.val.vlong=szof(new);
                    if(global){
                        unsigned char *bmk;size_t bsize;
                        bsize=(basic_blocks+CHAR_BIT-1)/CHAR_BIT;
                        bmk=mymalloc(bsize);
                        memset(bmk,0,bsize);
                        cse_search(g,p,0,v,global,bmk);
                        free(bmk);
                    }else cse_search(g,p,p->prev,v,global,0);
                    changed=1;
                    gchanged|=1;
                }else BSET(ae,i);
            }
            for(j=0;j<p->change_cnt;j++){
                i=p->change_list[j].v->index;
                if(p->change_list[j].flags&DREFOBJ) i+=vcount-rcount;
                if(i>=vcount) continue;
                bvdiff(ae,ae_kills[i],esize);
            }

            if(p==g->end) break;
            p=p->next;
        }
    }

    free(ae);
    for(i=0;i<vcount;i++) free(ae_kills[i]);
    free(ae_kills);
    free(ae_globals);
    free(ae_statics);
    free(ae_address);
    free(ae_drefs);
    free(elist);

    return(changed);
}
