/*  $VER: vbcc (loop.c) V0.4     */
/*  schleifenorientierte Optimierungen  */

#include "opt.h"

static char FILE_[]=__FILE__;

#define MOVE_IC 1
#define MOVE_COMP 2

/*  Liste, in die ICs eingetragen werden, die aus Schleifen */
/*  gezogen werden sollen.                                  */
struct movlist{
    struct movlist *next;
    struct IC *IC;
    struct flowgraph *target_fg;
    int flags;
};

struct movlist *first_mov,*last_mov;

int report_weird_code,report_suspicious_loops;

/*  Bitvektoren fuer schleifeninvariante ICs    */
unsigned char *invariant,*inloop,*moved,*moved_completely;
unsigned char *fg_tmp;
unsigned char *not_movable;
size_t bsize;


/*  Liste, in die ICs fuer strength-reduction eingetragen   */
/*  werden.                                                 */
struct srlist{
    struct srlist *next;
    struct IC *ind_var;
    struct IC *IC;
    struct flowgraph *target_fg;
    /*  Hilfsvariable, falls eine aequivalente Operation schon reduziert    */
    /*  wurde.                                                              */
    struct Var *hv;
};

struct srlist *first_sr,*last_sr;

/*  Hier werden Induktionsvariablen vermerkt    */
struct IC **ind_vars;

static struct flowgraph *first_fg;

int loops(struct flowgraph *fg,int footers)
/*  kennzeichnet Schleifen im Flussgraph; wenn footers!=0 werden darf eine  */
/*  Schleife nur einen gemeinsamen Austrittspunkt haben                     */
{
    int i,start,end,c=0;struct flowlist *lp;struct flowgraph *g,*loopend;
    if(DEBUG&1024) printf("searching loops\n");
    g=fg;
    while(g){
        start=g->index;
        end=-1;
        for(lp=g->in;lp;lp=lp->next){
            if(!lp->graph) continue;
            if(lp->graph->branchout==g||!lp->graph->end||lp->graph->end->code!=BRA){
                i=lp->graph->index;
                if(i>=start&&i>end){ end=i;loopend=lp->graph; }
            }
        }
        if(end>=0){
        /*  Schleife oder etwas aehnliches  */
            struct flowgraph *p=g;
            if(DEBUG&1024) printf("found possible loop from blocks %d to %d\n",start,end);
            if(1/*goto_used*/){
                if(DEBUG&1024) printf("have to check...\n");
                do{
                    if(!p||p->index>end) break;

                    /*  testen, ob aus der Schleife gesprungen wird */
                    if(p->branchout&&footers){
                        i=p->branchout->index;
                        if(i<start){
                            end=-1;
                            break;
                        }
                        if(i>end&&(DEBUG&1024)){
                            puts("jump out of loop");
                            if(p->branchout!=loopend->normalout){
                                puts("no break");
                                if(p->branchout->start->typf!=return_label) puts("no return");
                            }
                        }
                        if(i>end&&p->branchout!=loopend->normalout&&p->branchout->start->typf!=return_label){
                        /*  Sprung zu anderem als dem normalen Austritt oder return */
                            end=-1;
                            break;
                        }
                    }
                    /*  testen, ob in die Schleife gesprungen wird  */
                    if(p!=g){
                        for(lp=p->in;lp;lp=lp->next){
                            if(!lp->graph) continue;
                            if(lp->graph->branchout==p){
                                i=lp->graph->index;
                                if(i<start){
                                    if(report_weird_code){error(175);report_weird_code=0;}
                                    end=-1;
                                    break;
                                }
                                if(i>end){
                                    end=-1;
                                    break;
                                }
                            }
                        }
                    }
                    if(p->index==end) break;
                    p=p->normalout;
                }while(end>=0);
            }else{
                if(DEBUG&1024) printf("must be a loop, because there was no goto\n");
            }
        }
        if(end>=0){
            if(DEBUG&1024) printf("confirmed that it is a loop\n");
            g->loopend=loopend;
            c++;
        }
        g=g->normalout;
    }
    return(c);
}

struct flowgraph *create_loop_headers(struct flowgraph *fg,int av)
/*  Fuegt vor jede Schleife einen Kopf-Block ein, wenn noetig.  */
/*  Wenn av!=0 werden aktive Variablen korrekt uebertragen und  */
/*  diverse Registerlisten uebernommen und index=-1 gesetzt.    */
/*  Kann einen Block mehrmals in der ->in Liste eintragen       */
{
    struct flowgraph *g,*last,*new,*rg=fg;
    struct IC *lic;
    if(DEBUG&1024) printf("creating loop-headers\n");
    g=fg;last=0;
    while(g){
        new=0;
        if(g->loopend){
            if(!last){
                struct flowlist *lp;
                new=mymalloc(sizeof(struct flowgraph));
                rg=new;
                new->in=0;
                new->start=new->end=0;
                lp=mymalloc(sizeof(struct flowlist));
                lp->graph=new;
                lp->next=g->in;
                g->in=lp;
            }else{
                struct flowlist *lp,*nl,**ls;
                new=mymalloc(sizeof(struct flowgraph));
                last->normalout=new;
                lic=mymalloc(ICS);
                lic->line=0;
                lic->file=0;
                new->start=new->end=lic;
                lic->code=LABEL;
                lic->typf=++label;
                lic->q1.flags=lic->q2.flags=lic->z.flags=0;
                lic->q1.am=lic->q2.am=lic->z.am=0;
                lic->use_cnt=lic->change_cnt=0;
                lic->use_list=lic->change_list=0;
                last->end->next=lic;    /*  kann nicht leer sein, da sonst branchout nicht 0 waere  */
                lic->prev=last->end;
                g->start->prev=lic;
                lic->next=g->start;
                lp=g->in;ls=&new->in;
                while(lp){
                    if(lp->graph&&lp->graph->index<g->index){
                    /*  Eintritt von oben soll in den Kopf  */
                        nl=mymalloc(sizeof(struct flowlist));
                        nl->graph=lp->graph;
                        nl->next=0;
                        (*ls)=nl;
                        ls=&nl->next;
                        if(lp->graph->branchout==g){
                            if(DEBUG&1024) printf("changing branch\n");
                            if(lp->graph->end->code<BEQ||lp->graph->end->code>BRA) ierror(0);
                            lp->graph->end->typf=lic->typf;
                            lp->graph->branchout=new;
                        }
                        lp->graph=new;
                    }
                    lp=lp->next;
                }
                if(!new->in) ierror(0);
            }
            if(new){
                if(DEBUG&1024) printf("must insert loop-header before block %d\n",g->index);
                basic_blocks++;
                new->branchout=0;
                new->loopend=0;
                if(av)
                    new->index=-1;
                else
                    new->index=basic_blocks;
                new->normalout=g;
                new->calls=0;
                new->loop_calls=0;
                new->rd_in=new->rd_out=new->rd_kill=new->rd_gen=0;
                new->ae_in=new->ae_out=new->ae_kill=new->ae_gen=0;
                new->cp_in=new->cp_out=new->cp_kill=new->cp_gen=0;
                if(!av){
                    new->av_in=new->av_out=new->av_kill=new->av_gen=0;
                }else{
                    new->av_in=mymalloc(vsize);
                    new->av_out=mymalloc(vsize);
                    new->av_gen=mymalloc(vsize);
                    new->av_kill=mymalloc(vsize);
                    memset(new->av_gen,0,vsize);
                    memset(new->av_kill,0,vsize);
                    memcpy(new->av_out,g->av_in,vsize);
                    memcpy(new->av_in,g->av_in,vsize);
                    memcpy(&new->regv,&g->regv,sizeof(new->regv));
                    memcpy(&new->regused,&g->regused,sizeof(new->regused));
                }
            }
        }
        last=g;
        g=g->normalout;
    }
    return(rg);
}
struct flowgraph *create_loop_footers(struct flowgraph *fg,int av)
/*  Fuegt hinter jede Schleife einen Fuss-Block ein, wenn noetig.   */
/*  Wenn av!=0 werden aktive Variablen korrekt uebertragen und      */
/*  diverse Registerlisten uebernommen und index auf -2 gesetzt.    */
{
    struct flowgraph *g,*loopend,*out,*new;
    struct IC *lic;
    if(DEBUG&1024) printf("creating loop-footers\n");
    g=fg;
    while(g){
        new=0;
        loopend=g->loopend;
        if(loopend){
            struct flowlist *lp,*nl,**ls;
            out=loopend->normalout;
            new=mymalloc(sizeof(struct flowgraph));
            new->normalout=out;
            loopend->normalout=new;
            lic=mymalloc(ICS);
            lic->line=0;
            lic->file=0;
            new->start=new->end=lic;
            lic->code=LABEL;
            lic->typf=++label;
            lic->q1.flags=lic->q2.flags=lic->z.flags=0;
            lic->q1.am=lic->q2.am=lic->z.am=0;
            lic->use_cnt=lic->change_cnt=0;
            lic->use_list=lic->change_list=0;
            if(out) lp=out->in; else {lp=0;new->in=0;}
            ls=&new->in;
            while(lp){
                if(lp->graph&&lp->graph->index<=loopend->index&&lp->graph->index>=g->index){
                /*  Austritt aus Schleife soll in den Fuss  */
                    nl=mymalloc(sizeof(struct flowlist));
                    nl->graph=lp->graph;
                    nl->next=0;
                    (*ls)=nl;
                    ls=&nl->next;
                    if(lp->graph->branchout==out){
                        if(DEBUG&1024) printf("changing branch\n");
                        if(lp->graph->end->code<BEQ||lp->graph->end->code>BRA) ierror(0);
                        lp->graph->end->typf=lic->typf;
                        lp->graph->branchout=new;
                    }
                    lp->graph=new;
                }
                lp=lp->next;
            }
            if(out&&!new->in) ierror(0);
            if(DEBUG&1024) printf("must insert loop-footer after block %d\n",loopend->index);
            basic_blocks++;
            new->branchout=0;
            new->loopend=0;
            if(av)
                new->index=-2;
            else
                new->index=basic_blocks;
            new->normalout=out;
            new->calls=0;
            new->loop_calls=0;
            new->rd_in=new->rd_out=new->rd_kill=new->rd_gen=0;
            new->ae_in=new->ae_out=new->ae_kill=new->ae_gen=0;
            new->cp_in=new->cp_out=new->cp_kill=new->cp_gen=0;
            if(!av){
                new->av_in=new->av_out=new->av_kill=new->av_gen=0;
            }else{
                new->av_in=mymalloc(vsize);
                new->av_out=mymalloc(vsize);
                new->av_kill=mymalloc(vsize);
                new->av_gen=mymalloc(vsize);
                memset(new->av_gen,0,vsize);
                memset(new->av_kill,0,vsize);
                if(out){
                    memcpy(new->av_out,out->av_in,vsize);
                    memcpy(new->av_in,out->av_in,vsize);
                }else{
                    memset(new->av_in,0,vsize);
                    memset(new->av_out,0,vsize);
                }
                memcpy(&new->regv,&g->regv,sizeof(new->regv));
                memcpy(&new->regused,&g->regused,sizeof(new->regused));
            }
            insert_IC_fg(new,loopend->end,lic);
        }
        g=g->normalout;
    }
    return(fg);
}
void add_movable(struct IC *p,struct flowgraph *fg,int flags)
/*  Fuegt IC p, das aus der Schleife in Block fg mit Flags flags    */
/*  verschoben werden darf in eine Liste.                           */
{
    struct movlist *new=mymalloc(sizeof(*new));
    new->IC=p;
    new->target_fg=fg;
    new->flags=flags;
    new->next=0;
    if(last_mov){
        last_mov->next=new;
        last_mov=new;
    }else{
        first_mov=last_mov=new;
    }
    BSET(moved,p->defindex);
    if(flags==MOVE_IC) BSET(moved_completely,p->defindex);
}
int move_to_head(void)
/*  Geht die Liste mit verschiebbaren ICs durch und schiebt die ICs */
/*  in den Vorkopf der Schleife. Ausserdem wird die Liste           */
/*  freigegeben.                                                    */
/*  Der Rueckgabewert hat Bit 1 gesetzt, wenn ICs ganz verschoben   */
/*  wurden und Bit 2, falls eine Berechnung mit Hilfsvariable vor   */
/*  die Schleife gezogen wurde.                                     */
{
    struct IC **fglist; /* Letztes IC vor jedem Block   */
    struct flowgraph *g;struct IC *p;struct movlist *m;
    int changed=0;

    if(!first_mov) return(0);

    if(DEBUG&1024) printf("moving the ICs out of the loop\n");

    fglist=mymalloc((basic_blocks+1)*sizeof(*fglist));
    p=0;
    for(g=first_fg;g;g=g->normalout){
        if(g->index>basic_blocks) ierror(0);
        if(g->end) p=g->end;
        fglist[g->index]=p;
    }
    while(first_mov){
        p=first_mov->IC;
        g=first_mov->target_fg;
        if(first_mov->flags==MOVE_IC){
            if(DEBUG&1024) {printf("moving IC out of loop:\n");pric2(stdout,p);}
            if(!p->prev||!p->next) ierror(0);
            p->next->prev=p->prev;
            p->prev->next=p->next;
            insert_IC_fg(g,fglist[g->index],p);
            fglist[g->index]=p;
            changed|=1;
        }else{
            struct Typ *t=mymalloc(TYPS);
            struct IC *new=mymalloc(ICS);
            struct Var *v;
            if(DEBUG&1024) {printf("moving computation out of loop:\n");pric2(stdout,p);}
            if(p->code==ADDRESS||p->code==ADDI2P||p->code==SUBIFP) t->flags=POINTER;
                else t->flags=p->typf;
            if(p->code==COMPARE||p->code==TEST) t->flags=0;
            if((t->flags&15)==POINTER){
                t->next=mymalloc(TYPS);
                t->next->flags=VOID;
                t->next->next=0;
            }else t->next=0;
            v=add_var(empty,t,AUTO,0);
            *new=*p;
            new->z.flags=VAR;
            new->z.v=v;
            new->z.val.vlong=l2zl(0L);
            /*  Die neue Operation benutzt maximal, was die andere benutzte */
            /*  und aendert nur die Hilfsvariable.                          */
            if(have_alias){
                new->use_cnt=p->use_cnt;
                new->use_list=mymalloc(new->use_cnt*VLS);
                memcpy(new->use_list,p->use_list,new->use_cnt*VLS);
                new->change_cnt=1;
                new->change_list=mymalloc(VLS);
                new->change_list[0].v=v;
                new->change_list[0].flags=0;
            }
            insert_IC_fg(g,fglist[g->index],new);
            fglist[g->index]=new;
            p->code=ASSIGN;
            p->typf=t->flags;
            p->q1.flags=VAR;
            p->q1.v=v;
            p->q1.val.vlong=l2zl(0L);
            p->q2.flags=0;
            p->q2.val.vlong=szof(t);
            /*  Die Operation in der Schleife benutzt nun zusaetzlich   */
            /*  noch die Hilfsvariable.                                 */
            if(have_alias){
                void *m=p->use_list;
                p->use_cnt++;
                p->use_list=mymalloc(p->use_cnt*VLS);
                memcpy(&p->use_list[1],m,(p->use_cnt-1)*VLS);
                free(m);
                p->use_list[p->use_cnt].v=v;
                p->use_list[p->use_cnt].flags=0;
            }
            changed|=2;
        }
        m=first_mov->next;
        free(first_mov);
        first_mov=m;
    }
    if(DEBUG&1024) print_flowgraph(first_fg);
    free(fglist);
    return(changed);
}
void calc_movable(struct flowgraph *start,struct flowgraph *end)
/*  Berechnet, welche Definitionen nicht aus der Schleife start-end     */
/*  verschoben werden duerfen. Eine Def. p von z darf nur verschoben    */
/*  werden, wenn keine andere Def. von p existiert und alle             */
/*  Verwendungen von z nur von p erreicht werden.                       */
/*  Benutzt rd_defs, rd_tmp, rd_vars.                                   */
{
    struct flowgraph *g;struct IC *p;
    int i,j,k,d;
    unsigned char *changed_vars;
    if(DEBUG&1024) printf("calculating not_movable for blocks %d to %d\n",start->index,end->index);
    if(!(c_flags_val[0].l&1024)){
        memset(not_movable,UCHAR_MAX,dsize);
        return;
    }
    memset(not_movable,0,dsize);
    changed_vars=mymalloc(vsize);
    memset(changed_vars,0,vsize);
    for(g=start;g;g=g->normalout){
        if(!g->rd_in) ierror(0);
        memcpy(rd_defs,g->rd_in,dsize);
        for(p=g->start;p;p=p->next){
            for(j=0;j<p->change_cnt;j++){
                i=p->change_list[j].v->index;
                if(p->change_list[j].flags&DREFOBJ) i+=vcount-rcount;
                if(i>=vcount) continue;
                if(BTST(changed_vars,i)){
                    bvunite(not_movable,defs[i],dsize);
                }else{
                    BSET(changed_vars,i);
                }
            }
            for(k=0;k<p->use_cnt;k++){
                i=p->use_list[k].v->index;
                if(p->use_list[k].flags&DREFOBJ) i+=vcount-rcount;
                if(i>=vcount) continue;
                if(BTST(rd_defs,i+dcount+1)){   /*  undefined->i ?  */
                    bvunite(not_movable,defs[i],dsize);
                }else{
                    memcpy(rd_tmp,rd_defs,dsize);
                    bvdiff(rd_tmp,defs[i],dsize);
                    for(d=-1,j=0;j<=dcount;j++){
                        if(BTST(rd_defs,j)){
                            if(d>=0){  /*  mehr als eine Def.  */
                                bvunite(not_movable,defs[i],dsize);
                                d=-1;break;
                            }else d=j;
                        }
                    }
                    if(d>=0){
                        memcpy(rd_tmp,defs[i],dsize);
                        BCLR(rd_tmp,j);
                        bvunite(not_movable,rd_tmp,dsize);
                    }
                }
            }

            /*  Das hier, um rd_defs zu aktualisieren.  */
            rd_change(p);
            if(p==g->end) break;
        }
        if(g==end) break;
    }
    free(changed_vars);
}
int used_in_loop_only(struct flowgraph *start,struct flowgraph *end,struct obj *o)
/*  Testet, ob Variable nur in der Schleife benutzt wird.               */
/*  Z.Z. wird nur auf Hilfsvariablen getestet.                          */
/*  Unbedingt aendern!                                                  */
{
    struct Var *v;struct flowgraph *g;struct IC *p;
    if((o->flags&(VAR|DREFOBJ))!=VAR) return(0);
    v=o->v;
    if((v->flags&USEDASADR)||v->nesting==0||v->storage_class==EXTERN||v->storage_class==STATIC)
        return(0);
    for(g=first_fg;g;g=g->normalout){
        if(g==start) g=end->normalout;
        if(!g) break;
        for(p=g->start;p;p=p->next){
            if((p->q1.flags&VAR)&&p->q1.v==v) return(0);
            if((p->q2.flags&VAR)&&p->q2.v==v) return(0);
            if((p->z.flags&VAR)&&p->z.v==v) return(0);
            if(p==g->end) break;
        }
        if(g==end) break;
    }
    return(1);
}

int always_reached(struct flowgraph *start,struct flowgraph *end,struct flowgraph *fg,struct IC *z)
/*  Testet, ob z immer ausgefuehrt wird, falls start in fg ausgefuehrt  */
/*  wird. fg_tmp ist ein Bitvektor, um zu merken, welche Bloecke sicher */
/*  zu z fuehren. Das ganze fuer die Schleife start-end.                */
{
    unsigned char *bmk=fg_tmp;
    struct IC *p;struct flowgraph *g;
    int changed;

    for(p=z;p;p=p->prev){
        if(p->code==CALL) return(0);
        if(p==fg->start) break;
    }

    if(fg==start) return(1);

    memset(bmk,0,bsize);
    BSET(bmk,fg->index);

    do{
        changed=0;
        for(g=start;g;g=g->normalout){
            if(!BTST(bmk,g->index)){
                struct flowgraph *n=g->normalout;
                struct flowgraph *b=g->branchout;
                if(n||b){
                    if((!b||BTST(bmk,b->index))&&
                       (!n||(g->end&&g->end->code==BRA)||BTST(bmk,n->index))){
                        for(p=g->end;p;p=p->prev){
                            if(p->code==CALL) break;
                            if(p==g->start){
                                if(g==start) return(1);
                                changed=1; BSET(bmk,g->index);
                                break;
                            }
                        }
                    }
                }
            }
            if(g==end) break;
        }
    }while(changed);
    return(0);
}

int def_invariant(int vindex,int ignore)
/*  Ermittelt, ob Variable vindex schleifeninvariant unter den Bedingungen  */
/*  rd_defs, inloop und invariant ist.                                      */
/*  Definition ignore wird nicht beachtet. Wenn ignore auf eine gueltige    */
/*  Definition gesetzt wird, kann man somit auf Induktionsvariablen testen  */
/*  (das Ergebnis sagt dann, ob das die einzige Definition in der Schleife  */
/*  ist).                                                                   */
{
    int i,k,j,d=0;
/*    printf("def_inv(%d)=%s(%ld)\n",vindex,vilist[vindex]->identifier,zl2l(vilist[vindex]->offset));*/
    if(!BTST(rd_defs,vindex+dcount+1)){
        memcpy(rd_tmp,rd_defs,dsize);
        bvintersect(rd_tmp,defs[vindex],dsize);
        for(j=1;j<=dcount;j++){
            if(j!=ignore&&BTST(rd_tmp,j)&&BTST(inloop,j)){
                /*  Mehr als eine moegliche Def. innerhalb der Schleife oder    */
                /*  eine invariante Def. in der Schleife => nicht invariant.    */
                if(d) return(0);
                if(!BTST(moved_completely,j)) return(0);
                d=1;
            }
        }
    }else{
        for(j=1;j<=dcount;j++){
            if(j!=ignore&&BTST(rd_defs,j)&&BTST(inloop,j)){
                struct IC *p=dlist[j];
                for(k=0;k<p->change_cnt;k++){
                    i=p->change_list[k].v->index;
                    if(p->change_list[k].flags&DREFOBJ) i+=vcount-rcount;
/*                    printf("modifies %d\n",i);*/
                    if(i==vindex) break;
                }
                if(k>=p->change_cnt) continue;
                /*  Mehr als eine moegliche Def. innerhalb der Schleife oder    */
                /*  eine invariante Def. in der Schleife => nicht invariant.    */
                if(d) return(0);
                if(!BTST(moved_completely,j)) return(0);
                d=1;
            }
        }
    }
    return(1);
}

void frequency_reduction(struct flowgraph *start,struct flowgraph *end,struct flowgraph *head)
/*  Schleifeninvariante ICs finden und in eine Liste eintragen, falls   */
/*  sie vor die Schleife gezogen werden koennen.                        */
{
    struct IC *p;struct flowgraph *g;


    int i,changed;

    if(head&&start->loopend){
        end=start->loopend;

        if(DEBUG&1024){
            printf("searching for loop-invariant code in loop from block %d to %d\n",start->index,end->index);
            printf("head_fg=%d\n",head->index);
        }
        calc_movable(start,end);
        /*  erstmal kein IC invariant   */
        memset(invariant,0,dsize);

        /*  kennzeichnen, welche ICs in der Schleife liegen */
        memset(inloop,0,dsize);
        for(g=start;g;g=g->normalout){
            for(p=g->start;p;p=p->next){
                if(p->defindex) BSET(inloop,p->defindex);
                if(p==g->end) break;
            }
            if(g==end) break;
        }

        do{
            changed=0;
            if(DEBUG&1024) printf("loop-invariant pass\n");

            /*  Schleifeninvariante ICs suchen  */

            for(g=start;g;g=g->normalout){
                memcpy(rd_defs,g->rd_in,dsize);
                for(p=g->start;p;p=p->next){
                    int k1,k2;
                    /*  testen, ob IC neu als invariant markiert werden kann    */
                    if(p->defindex&&p->code!=CALL&&p->code!=GETRETURN&&!BTST(invariant,p->defindex)){
                        if(!BTST(inloop,p->defindex)) ierror(0);
                        if(p->code==ADDRESS||!p->q1.flags||(p->q1.flags&KONST)||(p->q1.flags&VARADR)){
                            k1=1;
                        }else{
                            if(!(p->q1.flags&VAR)) ierror(0);
                            i=p->q1.v->index;
                            if(p->q1.flags&DREFOBJ) i+=vcount-rcount;
                            k1=def_invariant(i,-1);
                        }
                        if(k1){
                            if(!p->q2.flags||(p->q2.flags&KONST)||(p->q2.flags&VARADR)){
                                k2=1;
                            }else{
                                if(!(p->q2.flags&VAR)) ierror(0);
                                i=p->q2.v->index;
                                if(p->q2.flags&DREFOBJ) i+=vcount-rcount;
                                k2=def_invariant(i,-1);
                            }
                        }
                        if(k1&&k2){
/*                            if(DEBUG&1024){ printf("found loop-invariant IC:\n");pric2(stdout,p);}*/
                            if(!BTST(moved,p->defindex)&&(always_reached(start,end,g,p)||(!dangerous_IC(p)&&used_in_loop_only(start,end,&p->z)))){
                                if(p->z.flags&DREFOBJ)
                                    k1=def_invariant(p->z.v->index,-1);
                                else
                                    k1=1;
/*                                if(DEBUG&1024) printf("always reached or used only in loop\n");*/
                                if(k1&&!BTST(not_movable,p->defindex)){
/*                                    if(DEBUG&1024) printf("movable\n");*/
                                    add_movable(p,head,MOVE_IC);
                                }else{
                                    if(p->code==ADDRESS||((p->typf&15)<=POINTER&&(p->q2.flags||(p->q1.flags&DREFOBJ)))){
/*                                        if(DEBUG&1024) printf("move computation out of loop\n");*/
                                        add_movable(p,head,MOVE_COMP);
                                    }
                                }
                            }else{
                            /*  Wenn IC immer erreicht wird oder ungefaehrlich  */
                            /*  ist, kann man zumindest die Operation           */
                            /*  rausziehen, falls das lohnt.                    */
                                if(!BTST(moved,p->defindex)&&(!dangerous_IC(p)&&(p->typf&15)<=POINTER&&(p->q2.flags||(p->q1.flags&DREFOBJ)||p->code==ADDRESS))){
/*                                    if(DEBUG&1024) printf("move computation out of loop\n");*/
                                    add_movable(p,head,MOVE_COMP);
                                }
                            }
                            BSET(invariant,p->defindex);
                            changed=1;
                        }
                    }

                    /*  Das hier, um rd_defs zu aktualisieren.  */
                    rd_change(p);

                    if(p==g->end) break;
                }
                if(g==end) break;
            }
        }while(changed);

    }
    return;
}
void add_sr(struct IC *p,struct flowgraph *fg,int i_var)
/*  Fuegt IC p, das aus der Schleife in Block fg lineare Fkt. der   */
/*  Induktionsvariable i_var ist, in Liste ein.                     */
/*  Funktioniert als Stack. Da mit aeusseren Schleifen angefangen   */
/*  wird, werden ICs zuerst in inneren Schleifen reduziert. Da ein  */
/*  IC nur einmal reduziert wird, sollte dadurch das Problem eines  */
/*  ICs, das potentiell in mehreren Schleifen reduziert werden      */
/*  koennte, geloest werden.                                        */
{
    struct srlist *new=mymalloc(sizeof(*new));
    new->IC=p;
    new->target_fg=fg;
    new->ind_var=ind_vars[i_var];
    new->next=first_sr;
    new->hv=0;
    first_sr=new;
#if 0
    if(last_sr){
        last_sr->next=new;
        last_sr=new;
    }else{
        first_sr=last_sr=new;
    }
#endif
}
int do_sr(void)
/*  Durchlaufe die Liste aller strength-reduction-Kandidaten und    */
/*  ersetze sie durch neue Induktionsvariablen. Dabei aufpassen,    */
/*  falls ein IC schon von frequency-reduction bearbeitet wurde.    */
/*  Ausserdem wird die Liste freigegeben.                           */
{
    struct IC **fglist; /* Letztes IC vor jedem Block   */
    struct IC *p;
    struct flowgraph *g;
    struct srlist *mf;
    int changed=0;

    if(!first_sr) return(0);

    if(DEBUG&1024) printf("performing strength-reductions\n");

    fglist=mymalloc((basic_blocks+1)*sizeof(*fglist));
    p=0;
    for(g=first_fg;g;g=g->normalout){
        if(g->index>basic_blocks) ierror(0);
        if(g->end) p=g->end;
        fglist[g->index]=p;
    }

    while(first_sr){
        struct Var *niv,*nstep;
        struct Typ *t1,*t2;
        struct IC *iv_ic,*new,*m;
        int i,c;
        p=first_sr->IC;
        i=p->defindex;
        /*  Falls IC noch nicht verschoben und noch nicht reduziert wurde.  */
        if(!BTST(moved,i)&&p->code!=ASSIGN){
            if(first_sr->hv){
            /*  Es wurde schon eine aequivalente Operation reduziert, wir   */
            /*  koennen also dieselbe Hilfsvariable benutzen.               */
                p->code=ASSIGN;
                p->q1.flags=VAR;
                p->q1.v=first_sr->hv;
                p->q1.val.vlong=l2zl(0L);
                p->q2.flags=0;
                p->q2.val.vlong=szof(p->z.v->vtyp);
                /*  Hilfsvariable wird jetzt auch benutzt.  */
                if(have_alias){
                    void *m=p->use_list;
                    p->use_cnt++;
                    p->use_list=mymalloc(p->use_cnt*VLS);
                    memcpy(&p->use_list[1],m,(p->use_cnt-1)*VLS);
                    free(m);
                    p->use_list[0].v=first_sr->hv;
                    p->use_list[0].flags=0;
                }
            }else{
                int minus=0;
                if(DEBUG&1024){ printf("performing strength-reduction on IC:\n");pric2(stdout,p);}
                c=p->code;
                g=first_sr->target_fg;
                iv_ic=first_sr->ind_var;
                /*  Merken, wenn IC von der Form SUB x,ind_var->z   */
                if(c==SUB&&!compare_objs(&p->q2,&iv_ic->z,iv_ic->typf))
                    minus=1;
                t1=mymalloc(TYPS);
                t1->flags=p->typf;
                if(c==ADDI2P||c==SUBIFP){
                    t1->flags=POINTER;
                    t1->next=mymalloc(TYPS);
                    t1->next->flags=VOID;
                    t1->next->next=0;
                }else t1->next=0;
                niv=add_var(empty,t1,AUTO,0);
                /*  Suchen, ob es noch aequivalente Operationen gibt.   */
                /*  Noch sehr ineffizient...                            */
                for(mf=first_sr->next;mf;mf=mf->next){
                    if(mf->target_fg==g&&mf->ind_var==iv_ic){
                        m=mf->IC;
                        if(c==m->code&&p->typf==m->typf&&
                           !compare_objs(&p->q1,&m->q1,p->typf)&&
                           !compare_objs(&p->q2,&m->q2,p->typf)){
                            if(mf->hv) ierror(0);
                            mf->hv=niv;
                            if(DEBUG&1024){ printf("equivalent operation\n");pric2(stdout,m);}
                        }
                    }
                }
                /*  Initialisierung der Hilfsinduktionsvariablen    */
                new=mymalloc(ICS);
                *new=*p;
                new->z.flags=VAR;
                new->z.v=niv;
                new->z.val.vlong=l2zl(0L);
                /*  IC benutzt dasselbe wie p und aendert nur niv.  */
                if(have_alias){
                    new->change_cnt=1;
                    new->change_list=mymalloc(VLS);
                    new->change_list[0].v=niv;
                    new->change_list[0].flags=0;
                    new->use_cnt=p->use_cnt;
                    new->use_list=mymalloc(new->use_cnt*VLS);
                    memcpy(new->use_list,p->use_list,new->use_cnt*VLS);
                }
                insert_IC_fg(g,fglist[g->index],new);
                fglist[g->index]=m=new;
                /*  Ersetzen der Operation durch die Hilfsvariable  */
                p->code=ASSIGN;
                p->typf=t1->flags;
                p->q1=m->z;
                p->q2.flags=0;
                p->q2.val.vlong=szof(t1);
                /*  Benutzt jetzt auch Hilfsvariable.               */
                if(have_alias){
                    void *mr=p->use_list;
                    p->use_cnt++;
                    p->use_list=mymalloc(p->use_cnt*VLS);
                    memcpy(&p->use_list[1],mr,(p->use_cnt-1)*VLS);
                    free(mr);
                    new->use_list[0].v=niv;
                    new->use_list[0].flags=0;
                }
                /*  Berechnen der Schrittweite fuer Hilfsvariable   */
                if(c==MULT){
                    t2=mymalloc(TYPS);
                    t2->flags=iv_ic->typf;
                    t2->next=0;
                    nstep=add_var(empty,t2,AUTO,0);
                    new=mymalloc(ICS);
                    new->line=iv_ic->line;
                    new->file=iv_ic->file;
                    new->code=MULT;
                    new->typf=p->typf;
                    new->z.flags=VAR;
                    new->z.v=nstep;
                    new->z.val.vlong=l2zl(0L);
                    if(!compare_objs(&m->q1,&iv_ic->z,iv_ic->typf)) new->q1=m->q2;
                        else new->q1=m->q1;
                    if(!compare_objs(&iv_ic->q1,&iv_ic->z,iv_ic->typf)) new->q2=iv_ic->q2;
                        else new->q2=iv_ic->q1;
                    /*  Benutzt dasselbe wie iv_ic und m.   */
                    if(have_alias){
                        new->use_cnt=iv_ic->use_cnt+m->use_cnt;
                        new->use_list=mymalloc(new->use_cnt*VLS);
                        memcpy(new->use_list,iv_ic->use_list,iv_ic->use_cnt*VLS);
                        memcpy(&new->use_list[iv_ic->use_cnt],m->use_list,m->use_cnt*VLS);
                        new->change_cnt=1;
                        new->change_list=mymalloc(VLS);
                        new->change_list[0].v=nstep;
                        new->change_list[0].flags=0;
                    }
                    insert_IC_fg(g,fglist[g->index],new);
                    fglist[g->index]=m=new;
                }
                /*  Erhoehen der Hilfsvariable um Schrittweite      */
                new=mymalloc(ICS);
                new->line=iv_ic->line;
                new->file=iv_ic->file;

                new->code=iv_ic->code;
                if(minus){
                    switch(new->code){
                        case ADD:     new->code=SUB; break;
                        case SUB:     new->code=ADD; break;
                        case ADDI2P:  new->code=SUBIFP; break;
                        case SUBIFP:  new->code=ADDI2P; break;
                    }
                }
                if(t1->flags==POINTER){
                    if(new->code==ADD) new->code=ADDI2P;
                    if(new->code==SUB) new->code=SUBIFP;
                }
                new->typf=iv_ic->typf;
                new->q1.flags=VAR;
                new->q1.v=niv;
                new->q1.val.vlong=l2zl(0L);
                new->z=new->q1;
                if(c==MULT){
                    new->q2=m->z;
                }else{
                    if(!compare_objs(&iv_ic->q1,&iv_ic->z,iv_ic->typf)) new->q2=iv_ic->q2;
                        else new->q2=iv_ic->q1;
                }
                if(have_alias){
                    new->use_cnt=iv_ic->use_cnt+m->use_cnt;
                    new->use_list=mymalloc(new->use_cnt*VLS);
                    memcpy(new->use_list,iv_ic->use_list,iv_ic->use_cnt*VLS);
                    memcpy(&new->use_list[iv_ic->use_cnt],m->use_list,m->use_cnt*VLS);
                    new->change_cnt=1;
                    new->change_list=mymalloc(VLS);
                    new->change_list[0].v=niv;
                    new->change_list[0].flags=0;
                }
                /*  Flussgraph muss nur bei den Schleifenkoepfen ok sein.   */
                insert_IC(iv_ic,new);
                changed|=2;
            }
        }
        mf=first_sr->next;
        free(first_sr);
        first_sr=mf;
    }
    free(fglist);
    return(changed);
}
void strength_reduction(struct flowgraph *start,struct flowgraph *end,struct flowgraph *head)
/*  Ersetzen von Operationen mit einer Induktionsvariablen und einem    */
/*  schleifeninvarianten Operanden durch eine zusaetzliche              */
/*  Hilfsinduktionsvariable.                                            */
{
    struct flowgraph *g;struct IC *p;
    int i;
    if(DEBUG&1024) printf("performing strength_reduction on blocks %d to %d\n",start->index,end->index);
    for(i=0;i<vcount;i++) ind_vars[i]=0;
    /*  Nach Induktionsvariablen suchen.    */
    for(g=start;g;g=g->normalout){
        memcpy(rd_defs,g->rd_in,dsize);
        for(p=g->start;p;p=p->next){
            int c=p->code;
            if(c==ADD||c==ADDI2P||c==SUB||c==SUBIFP){
                if(!compare_objs(&p->q1,&p->z,p->typf)){
/*                    if(DEBUG&1024){printf("possible induction:\n");pric2(stdout,p);}*/
                    if(p->q2.flags&VAR){
                        i=p->q2.v->index;
                        if(p->q2.flags&DREFOBJ) i+=vcount-rcount;
                    }
                    if((p->q2.flags&(VAR|VARADR))!=VAR||def_invariant(i,-1)){
                        i=p->z.v->index;
                        if(p->z.flags&DREFOBJ) i+=vcount-rcount;
                        if(def_invariant(i,p->defindex)){
/*                            if(DEBUG&1024) {printf("found basic induction var:\n");pric2(stdout,p);}*/
                            ind_vars[i]=p;
                        }
                    }
                }
                if(USEQ2ASZ&&c!=SUB&&c!=SUBIFP&&!compare_objs(&p->q2,&p->z,p->typf)){
/*                    if(DEBUG&1024){printf("possible induction:\n");pric2(stdout,p);}*/
                    if(p->q1.flags&VAR){
                        i=p->q1.v->index;
                        if(p->q1.flags&DREFOBJ) i+=vcount-rcount;
                    }
                    if((p->q1.flags&(VAR|VARADR))!=VAR||def_invariant(i,-1)){
                        i=p->z.v->index;
                        if(p->z.flags&DREFOBJ) i+=vcount-rcount;
                        if(def_invariant(i,p->defindex)){
/*                            if(DEBUG&1024) {printf("found basic induction var:\n");pric2(stdout,p);}*/
                            ind_vars[i]=p;
                        }
                    }
                }
            }

            /*  Das hier, um rd_defs zu aktualisieren.  */
            rd_change(p);

            if(p==g->end) break;
        }
        if(g==end) break;
    }
    /*  Nach reduzierbaren Operationen suchen   */
    for(g=start;g;g=g->normalout){
        memcpy(rd_defs,g->rd_in,dsize);
        for(p=g->start;p;p=p->next){
            if((p->code==MULT||p->code==ADD||p->code==SUB||p->code==ADDI2P||p->code==SUBIFP)&&
               (((p->typf&15)!=FLOAT&&(p->typf&15)!=DOUBLE)||(c_flags[21]&USEDFLAG)) ){
                int k1,k2,iv;
                if((p->q1.flags&(VAR|VARADR))==VAR){
                    i=p->q1.v->index;
                    if(p->q1.flags&DREFOBJ) i+=vcount-rcount;
                    if(ind_vars[i]) {k1=1;iv=i;}
                    else if(def_invariant(i,-1)) k1=2;
                    else k1=0;
                }else k1=2;
                if((p->q2.flags&(VAR|VARADR))==VAR){
                    i=p->q2.v->index;
                    if(p->q2.flags&DREFOBJ) i+=vcount-rcount;
                    if(ind_vars[i]) {k2=1;iv=i;}
                    else if(def_invariant(i,-1)) k2=2;
                    else k2=0;
                }else k2=2;
                if(p->z.flags&VAR){
                /*  Aufpassen, dass eine Induktion nicht selbst reduziert   */
                /*  wird.                                                   */
                    i=p->z.v->index;
                    if(p->z.flags&DREFOBJ) i+=vcount-rcount;
                    if(ind_vars[i]) k1=0;
                }
                if(k1+k2==3){
/*                    if(DEBUG&1024) {printf("could perform strength-reduction on:\n");pric2(stdout,p);}*/
                    add_sr(p,head,iv);
                }
            }
            /*  Das hier, um rd_defs zu aktualisieren.  */
            rd_change(p);

            if(p==g->end) break;
        }
        if(g==end) break;
    }
}
void unroll(struct flowgraph *start,struct flowgraph *head)
{
    struct flowlist *lp;struct flowgraph *end;struct IC *p;
    struct obj *o;union atyps init_val,end_val,step_val;
    unsigned char *tmp;
    long dist,step;
    int bflag=0,t=0,i,flags=0; /* 1: sub, 2: init_val gefunden  */
    end=start->loopend;
    if(DEBUG&1024) printf("checking for possible unrolling from %d to %d\n",start->index,end->index);
    for(lp=start->in;lp;lp=lp->next)
        if(lp->graph->index>start->index&&lp->graph->index<=end->index&&lp->graph!=end) return;
    if(DEBUG&1024) printf("only one backward-branch\n");
    p=end->end;
    do{
        if(p->code>=BEQ&&p->code<BRA) bflag=p->code;
        if(p->code==TEST){
            o=&p->q1;t=p->typf;
            vlong=l2zl(0L);insert_const2(&end_val,LONG);
            break;
        }
        if(p->code==COMPARE){
            if(p->q2.flags&KONST){
                o=&p->q1;t=p->typf;
                end_val=p->q2.val;
                break;
            }
            if(p->q1.flags&KONST){
                o=&p->q2;t=p->typf;
                end_val=p->q2.val;
                if(bflag==BLT) bflag=BGT;
                if(bflag==BLE) bflag=BGE;
                if(bflag==BGT) bflag=BLT;
                if(bflag==BGE) bflag=BLE;
                break;
            }
            return;
        }
        if(p==end->start) return;
        p=p->prev;
    }while(p);
    if(!(o->flags&VAR)) return;
    p=ind_vars[o->v->index];
    if(!p) return;
    if(compare_objs(o,&p->z,t)) return;
    if(DEBUG&1024) printf("loop condition only dependant on induction var\n");
    if(!(p->q2.flags&KONST)) return;
    if(DEBUG&1024) printf("induction is constant\n");
    step_val=p->q2.val;
    if(p->code==SUB) flags|=1;
    i=p->z.v->index;
    if(p->z.flags&DREFOBJ) i+=vcount-rcount;
    tmp=mymalloc(dsize);
    memcpy(tmp,head->rd_out,dsize);
    if(BTST(tmp,i+dcount+1)) return; /*  keine eind. Def.    */
    bvintersect(tmp,defs[i],dsize);
    for(i=1;i<=dcount;i++){
        if(BTST(tmp,i)){
            if(DEBUG&1024){ printf("possible init:\n");pric2(stdout,dlist[i]);}
            if((flags&2)||dlist[i]->code!=ASSIGN||!(dlist[i]->q1.flags&KONST)){
                free(tmp);return;
            }
            init_val=dlist[i]->q1.val;
            flags|=2;
        }
    }
    free(tmp);
    if(!(flags&2)) return;
    if(DEBUG&1024){
        printf("loop number determinable\n");
        printf("init_val: ");printval(stdout,&init_val,t,1);
        printf("\nend_val: ");printval(stdout,&end_val,t,1);
        printf("\nstep_val: ");printval(stdout,&step_val,t,1);
        printf("\nflags=%d bflag=%d\n",flags,bflag);
    }
    /*  Nur integers als Induktionsvariablen.   */
    if((t&15)>LONG) return;
    /*  Distanz und Step werden als long behandelt, deshalb pruefen, ob */
    /*  alles im Bereich des garantierten Mindestwerte fuer long.       */
    /*  Wenn man hier die Arithmetik der Zielmaschine benutzen wuerde,  */
    /*  koennte man theoretisch mehr Faelle erkennen, aber das waere    */
    /*  recht popelig und man muss sehr aufpassen.                      */
    if(t&UNSIGNED){
        eval_const(&end_val,t);
        if(!zulleq(vulong,l2zl(2147483647))) return;
        dist=zul2ul(vulong);
        eval_const(&init_val,t);
        if(!zulleq(vulong,l2zl(2147483647))) return;
        dist-=zul2ul(vulong);
        eval_const(&step_val,t);
        if(!zulleq(vulong,l2zl(2147483647))) return;
        step=zul2ul(vulong);
    }else{
        eval_const(&end_val,t);
        if(!zlleq(vlong,l2zl(2147483647/2))) return;
        if(zlleq(vlong,l2zl(-2147483647/2))) return; /*  eins weniger als moeglich waere */
        dist=zl2l(vlong);
        eval_const(&init_val,t);
        if(!zlleq(vlong,l2zl(2147483647/2))) return;
        if(zlleq(vlong,l2zl(-2147483647/2))) return; /*  eins weniger als moeglich waere */
        dist-=zl2l(vlong);
        eval_const(&step_val,t);
        if(!zlleq(vlong,l2zl(2147483647))) return;
        if(zlleq(vlong,l2zl(-2147483647))) return; /*  eins weniger als moeglich waere */
        step=zl2l(vlong);
    }
    if(flags&1) step=-step;
    if(DEBUG&1024) printf("dist=%ld, step=%ld\n",dist,step);
    if(step==0) ierror(0);
    /*  Die Faelle kann man noch genauer untersuchen, ob die Schleife   */
    /*  evtl. nur einmal durchlaufen wird o.ae.                         */
    if(step<0&&dist>=0){
        if(report_suspicious_loops){ error(208);report_suspicious_loops=0;}
        return;
    }
    if(step>0&&dist<=0){
        if(report_suspicious_loops){ error(208);report_suspicious_loops=0;}
        return;
    }
    if(bflag==BEQ){
        if(report_suspicious_loops){ error(208);report_suspicious_loops=0;}
        return;
    }
    /*  Aufpassen, ob das Schleifenende bei BNE auch getroffen wird.    */
    if(bflag==BNE){
        if(dist%step){
            if(report_suspicious_loops){ error(208);report_suspicious_loops=0;}
            return;
        }
    }
    if(bflag==BLT||bflag==BGT) dist--;
    if(dist/step<0) ierror(0);
    if(DEBUG&1024) printf("loop is executed %ld times\n",dist/step+1);
}

int loop_optimizations(struct flowgraph *fg)
/*  steuert Optimierungen in Schleifen  */
{
    int changed=0,i;
    struct flowgraph *g,*last;
    if(DEBUG&1024) print_flowgraph(fg);
    if(loops(fg,0)==0) return(0);
    if(DEBUG&1024) print_flowgraph(fg);
    first_fg=fg=create_loop_headers(fg,0);
    if(DEBUG&1024) print_flowgraph(fg);
    num_defs();

    bsize=(basic_blocks+CHAR_BIT)/CHAR_BIT;
    fg_tmp=mymalloc(bsize);
    ind_vars=mymalloc(vcount*sizeof(*ind_vars));
    invariant=mymalloc(dsize);
    inloop=mymalloc(dsize);
    rd_defs=mymalloc(dsize);
    rd_tmp=mymalloc(dsize);
    rd_mode=1;
    reaching_definitions(fg);
    if(DEBUG&1024) print_flowgraph(fg);
    moved=mymalloc(dsize);
    memset(moved,0,dsize);
    moved_completely=mymalloc(dsize);
    memset(moved_completely,0,dsize);
    not_movable=mymalloc(dsize);

    first_mov=last_mov=0;
    first_sr=last_sr=0;

    for(last=0,g=fg;g;g=g->normalout){
        if(g->loopend){
            frequency_reduction(g,g->loopend,last);
            strength_reduction(g,g->loopend,last);
            unroll(g,last);
        }
        last=g;
    }

    for(i=0;i<vcount;i++) free(defs[i]);
    free(defs);
    free(dlist);
    free(rd_globals);
    free(rd_statics);
    free(rd_address);
    free(rd_drefs);
    free(rd_parms);
    free(rd_defs);
    free(rd_tmp);
    free(rd_vars);

    free(invariant);
    free(inloop);

    changed|=move_to_head();
    changed|=do_sr();

    free(moved);
    free(not_movable);
    free(moved_completely);

    if(changed&2){
        if(DEBUG&1024) printf("must repeat num_vars\n");
        free(vilist);
        free(av_globals);free(av_statics);
        free(av_drefs);free(av_address);
        num_vars();
    }

    free(ind_vars);
    free(fg_tmp);

    return(changed);
}

