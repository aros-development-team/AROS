/*  $VER: vbcc (alias.c) V0.4   */
/*  Listen benutzter/veraenderter Variablen und Behandlung von Decknamen.   */

#include "opt.h"

static char FILE_[]=__FILE__;

int p_typ(struct Var *v)
/*  Liefert den Typ, auf den Variable zeigen kann. Falls nicht eindeutig    */
/*  wird CHAR zurueckgegeben, da ein char * auf alles zeigen kann.          */
{
    struct Typ *t=v->vtyp;int f;
    /*  Kein Zeiger? Dann moeglicherweise Struktur, die verschiedene Zeiger */
    /*  enthalten koennte. Koennte man evtl. noch genauer pruefen.          */
    if((t->flags&NQ)!=POINTER||!t->next||(v->flags&DNOTTYPESAFE)) return(CHAR);
    f=t->next->flags&NQ;
    if(f==VOID) f=CHAR;
    return(f);
}

void ic_changes(struct IC *p,unsigned char *result)
/*  Initialisiert den Bitvektor result mit allen Variablen, die durch das   */
/*  IC p geaendert werden koennten.                                         */
{
    int i,j,t,t2;struct Var *v;
    memset(result,0,vsize);
    t=p->typf&NQ;
    if(p->z.flags&VAR){
        v=p->z.v;
        i=v->index;
        /*  Hilfsvariable, die waehrend diesem cse-Durchlauf eingefuehrt    */
        /*  wurde.                                                          */
        if(i<0) return;
        if(p->z.flags&DREFOBJ) i+=vcount-rcount;
        if(i>=vcount) ierror(0);
        BSET(result,i);
        /*  Wenn p geaendert wird, wird auch *p geaendert   */
        if(i<rcount) BSET(result,i+vcount-rcount);

        if(p->z.flags&DREFOBJ){
            if(noaliasopt){
                bvunite(result,av_drefs,vsize);
                bvunite(result,av_address,vsize);
                bvunite(result,av_globals,vsize);
            }else{
                for(j=0;j<vcount-rcount;j++){
                    v=vilist[j];
                    if(!v) ierror(0);
                    if(v->nesting==0||v->storage_class==EXTERN||(v->flags&USEDASADR)){
                        struct Typ *tp=v->vtyp;
                        if(!v->vtyp) ierror(0);
                        do{
                            t2=tp->flags&NQ;
                            tp=tp->next;
                        }while(t2==ARRAY);
                        if(t==t2||t==CHAR||t2>POINTER){
                            BSET(result,j);
                            if(j<rcount) BSET(result,j+vcount-rcount);
                            continue;
                        }
                    }
                    if(j<rcount){
                        t2=p_typ(v);
                        if(t==t2||t==CHAR||t2==CHAR||t2>POINTER) BSET(result,j+vcount-rcount);
                    }
                }
            }
        }else{
            if(v->nesting==0||v->storage_class==EXTERN||(v->flags&USEDASADR)){
                if(noaliasopt){
                    bvunite(result,av_drefs,vsize);
                }else{
                    for(j=0;j<rcount;j++){
                        t2=p_typ(vilist[j]);
                        if(t==t2||t2==CHAR||t>POINTER) BSET(result,j+vcount-rcount);
                    }
                }
            }
        }
    }
    if(p->code==CALL){
        bvunite(result,av_drefs,vsize);
        bvunite(result,av_address,vsize);
        bvunite(result,av_globals,vsize);
        bvunite(result,av_statics,vsize);
    }
}
void ic_uses(struct IC *p,unsigned char *result)
/*  Initialisiert den Bitvektor result mit allen Variablen, die durch das   */
/*  IC p benutzt werden koennten.                                           */
{
    int i,j,t,t2,c;struct Var *v;struct Typ *tp;
    memset(result,0,vsize);
    c=p->code;t=p->typf&NQ;
    if(c!=ADDRESS){
        if((p->q1.flags&(VAR|VARADR))==VAR){
            v=p->q1.v;
            i=v->index;
            if(c==ADDI2P||c==SUBIFP||c==SUBPFP) t=POINTER;
            if(c==CONVCHAR||c==CONVUCHAR) t=CHAR;
            if(c==CONVSHORT||c==CONVUSHORT) t=SHORT;
            if(c==CONVINT||c==CONVUINT) t=INT;
            if(c==CONVLONG||c==CONVULONG) t=LONG;
            if(c==CONVFLOAT) t=FLOAT;
            if(c==CONVDOUBLE) t=DOUBLE;
            if(c==CONVPOINTER) t=POINTER;
            if(i>=vcount) {pric2(stdout,p);ierror(0);}
            BSET(result,i);
            if(v->nesting==0||v->storage_class==EXTERN||(v->flags&USEDASADR)){
                if(noaliasopt){
                    bvunite(result,av_drefs,vsize);
                }else{
                    for(j=0;j<rcount;j++){
                        t2=p_typ(vilist[j]);
                        if(t==t2||t2==CHAR||t>POINTER) BSET(result,j+vcount-rcount);
                    }
                }
            }
            if(p->q1.flags&DREFOBJ){
                BSET(result,i+vcount-rcount);
                if(noaliasopt){
                    bvunite(result,av_drefs,vsize);
                    bvunite(result,av_address,vsize);
                    bvunite(result,av_globals,vsize);
                }else{
                    for(j=0;j<vcount-rcount;j++){
                        v=vilist[j];
                        if(v->nesting==0||v->storage_class==EXTERN||(v->flags&USEDASADR)){
                            tp=v->vtyp;
                            do{
                                t2=tp->flags&NQ;
                                tp=tp->next;
                            }while(t2==ARRAY);
                            if(t==t2||t==CHAR||t2>POINTER||t>POINTER) BSET(result,j);
                        }
                        if(j<rcount){
                            t2=p_typ(v);
                            if(t==t2||t==CHAR||t2==CHAR||t>POINTER) BSET(result,j+vcount-rcount);
                        }
                    }
                }
            }
        }
        if((p->q2.flags&(VAR|VARADR))==VAR){
            v=p->q2.v;
            i=v->index;
            if(c==SUBPFP) t=POINTER;
            if(i>=vcount) {pric2(stdout,p);ierror(0);}
            BSET(result,i);
            if(v->nesting==0||(v->flags&USEDASADR)){
                if(noaliasopt){
                    bvunite(result,av_drefs,vsize);
                }else{
                    for(j=0;j<rcount;j++){
                        t2=p_typ(vilist[j]);
                        if(t==t2||t2==CHAR||t>POINTER) BSET(result,j+vcount-rcount);
                    }
                }
            }
            if(p->q2.flags&DREFOBJ){
                BSET(result,i+vcount-rcount);
                if(noaliasopt){
                    bvunite(result,av_drefs,vsize);
                    bvunite(result,av_address,vsize);
                    bvunite(result,av_globals,vsize);
                }else{
                    for(j=0;j<vcount-rcount;j++){
                        v=vilist[j];
                        if(v->nesting==0||v->storage_class==EXTERN||(v->flags&USEDASADR)){
                            tp=v->vtyp;
                            do{
                                t2=tp->flags&NQ;
                                tp=tp->next;
                            }while(t2==ARRAY);
                            if(t==t2||t==CHAR||t2>POINTER) BSET(result,j);
                        }
                        if(j<rcount){
                            t2=p_typ(v);
                            if(t==t2||t==CHAR||t2==CHAR) BSET(result,j+vcount-rcount);
                        }
                    }
                }
            }
        }
    }
    if((p->z.flags&(VAR|VARADR|DREFOBJ))==(VAR|DREFOBJ)){
        v=p->z.v;
        i=v->index;
        if(i>=vcount) {pric2(stdout,p);ierror(0);}
        BSET(result,i);
        if(c==ADDI2P||c==SUBIFP) t=POINTER;
        if(v->nesting==0||v->storage_class==EXTERN||(v->flags&USEDASADR)){
            if(noaliasopt){
                bvunite(result,av_drefs,vsize);
            }else{
                for(j=0;j<rcount;j++){
                    t2=p_typ(vilist[j]);
                    if(t==t2||t2==CHAR||t>POINTER||t2>POINTER) BSET(result,j+vcount-rcount);
                }
            }
        }
    }
    if(p->code==CALL){
        bvunite(result,av_drefs,vsize);
        bvunite(result,av_address,vsize);
        bvunite(result,av_globals,vsize);
        bvunite(result,av_statics,vsize);
    }
}
void free_alias(struct flowgraph *fg)
/*  Gibt alle use/change-Listen der ICs im Flussgraphen frei.               */
{
    struct IC *p;struct flowgraph *g;
    if(DEBUG&1024) printf("freeing alias info\n");
    for(g=fg;g;g=g->normalout){
        for(p=g->start;p;p=p->next){
            if(p->code==LABEL&&(p->use_cnt>0||p->change_cnt>0)) ierror(0);
            if(p->use_cnt>0) free(p->use_list);
            if(p->change_cnt>0) free(p->change_list);
            if(p==g->end) break;
        }
    }
    have_alias=0;
}
void create_alias(struct flowgraph *fg)
/*  Initialisiert jedes IC mit einer Liste aller Variablen, die dadurch     */
/*  benutzt und veraendert werden koennten. Z.Z. wird bis auf Typ-basierte  */
/*  Optimierungen der worst-case angenommen.                                */
{
    unsigned char *vars=mymalloc(vsize);
    struct IC *p;struct flowgraph *g;
    int i,cnt;
    if(DEBUG&1024) printf("creating alias info\n");
    for(g=fg;g;g=g->normalout){
        for(p=g->start;p;p=p->next){
            ic_uses(p,vars);
            for(i=0,cnt=0;i<vcount;i++)
                if(BTST(vars,i)) cnt++;
            p->use_cnt=cnt;
            if(cnt==0){
                p->use_list=0;
            }else{
                p->use_list=mymalloc(cnt*VLS);
                for(cnt=0,i=0;i<vcount;i++){
                    if(BTST(vars,i)){
                        p->use_list[cnt].v=vilist[i];
                        if(i>=vcount-rcount) p->use_list[cnt].flags=DREFOBJ;
                                else         p->use_list[cnt].flags=0;
                        cnt++;
                    }
                }
            }
            ic_changes(p,vars);
            for(i=0,cnt=0;i<vcount;i++)
                if(BTST(vars,i)) cnt++;
            p->change_cnt=cnt;
            if(cnt==0){
                p->change_list=0;
            }else{
                p->change_list=mymalloc(cnt*VLS);
                for(cnt=0,i=0;i<vcount;i++){
                    if(BTST(vars,i)){
                        p->change_list[cnt].v=vilist[i];
                        if(i>=vcount-rcount) p->change_list[cnt].flags=DREFOBJ;
                                else         p->change_list[cnt].flags=0;
                        cnt++;
                    }
                }
            }
            if(p==g->end) break;
        }
    }
    free(vars);
    have_alias=1;
}
void update_alias(struct Var *old,struct Var *new)
/*  Aendert alle use/changes von (old) auf (new). Wird aufgerufen, wenn     */
/*  copy-propagation eine Variable neu zu einem DREFOBJ macht.              */
{
    struct IC *p;int i;
    void *m;
    if(DEBUG&1024) printf("update-alias\n");
    for(p=first_ic;p;p=p->next){
        for(i=0;i<p->use_cnt;i++){
            if(p->use_list[i].v==old&&(p->use_list[i].flags&DREFOBJ)){
                m=p->use_list;
                p->use_cnt++;
                p->use_list=mymalloc(p->use_cnt*VLS);
                p->use_list[0].v=new;
                p->use_list[0].flags=DREFOBJ;
                memcpy(&p->use_list[1],m,(p->use_cnt-1)*VLS);
                free(m);
                break;
            }
        }
        for(i=0;i<p->change_cnt;i++){
            if(p->change_list[i].v==old&&(p->change_list[i].flags&DREFOBJ)){
                m=p->change_list;
                p->change_cnt++;
                p->change_list=mymalloc(p->change_cnt*VLS);
                p->change_list[0].v=new;
                p->change_list[0].flags=DREFOBJ;
                memcpy(&p->change_list[1],m,(p->change_cnt-1)*VLS);
                free(m);
                break;
            }
        }
    }
}

