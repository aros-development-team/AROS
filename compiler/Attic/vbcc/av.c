/*  $VER: vbcc (av.c) V0.4     */
/*  aktive Variablen und Elimination unnoetiger Anweisungen */

#include "opt.h"

static char FILE_[]=__FILE__;

/*  fuer aktive Variablen   */
struct Var **vilist;
unsigned int vcount;    /*  0..vcount-rcount-1: vars, vcount-rcount..vcount: DREFOBJs */
unsigned int rcount;
size_t vsize;
unsigned char *av_globals,*av_address,*av_statics,*av_drefs;
int report_dead_statements;

void print_av(unsigned char *bitvector)
/*  druckt Variablen in einem Bitvektor */
{
    int i;
    if(!bitvector) {printf("active variables not available\n");return;}
    for(i=0;i<vcount-rcount;i++)
        if(BTST(bitvector,i)) printf("%3d: %s,%ld\n",i,vilist[i]->identifier,zl2l(vilist[i]->offset));
    for(i=vcount-rcount;i<vcount;i++)
        if(BTST(bitvector,i)) printf("%3d: (%s),%ld\n",i,vilist[i]->identifier,zl2l(vilist[i]->offset));
}
void num_vars(void)
/*  Numeriert Variablen und erzeugt Indexliste  */
{
    unsigned int i,j;struct IC *p;struct Var *v,*a[3],*vp;
    if(DEBUG&1024) printf("numerating variables loop1\n");
    /*  alle Indizes auf -1 */
    a[0]=first_var[0];
    a[1]=first_var[1];
    a[2]=merk_varf;
    for(j=0;j<3;j++){
        v=a[j];
        while(v){
            v->index=-1;
            /*  Variablen von inline-Funktionen */
            if(j==0&&v->fi&&v->fi->first_ic){
                for(vp=v->fi->vars;vp;vp=vp->next) vp->index=-1;
            }
            v=v->next;
        }
    }
    /*  erst alle Variablen, die als DREFOBJ benutzt werden */
    if(DEBUG&1024) printf("numerating variables loop2\n");
    i=0;
    for(p=first_ic;p;p=p->next){
        if(p->code<LABEL||p->code>BRA){
            int c=p->code;
            j=p->typf&15;
            if(c==SUBPFP) j=POINTER;
            if(c==ADDI2P||c==SUBIFP) j=POINTER;
            if(c==CONVCHAR||c==CONVUCHAR) j=CHAR;
            if(c==CONVSHORT||c==CONVUSHORT) j=SHORT;
            if(c==CONVINT||c==CONVUINT) j=INT;
            if(c==CONVLONG||c==CONVULONG) j=LONG;
            if(c==CONVPOINTER) j=POINTER;
            if(c==CONVFLOAT) j=FLOAT;
            if(c==CONVDOUBLE) j=DOUBLE;
            if((p->q1.flags&(VAR|DREFOBJ))==(VAR|DREFOBJ)){
                v=p->q1.v;
                if(!v->vtyp->next||(v->vtyp->next->flags&15)!=j) v->flags|=DNOTTYPESAFE;
                if(v->index<0) v->index=i++;
            }
            j=p->typf&15;
            if(c==SUBPFP) j=POINTER;
            if((p->q2.flags&(VAR|DREFOBJ))==(VAR|DREFOBJ)){
                v=p->q2.v;
                if(!v->vtyp->next||(v->vtyp->next->flags&15)!=j) v->flags|=DNOTTYPESAFE;
                if(v->index<0) v->index=i++;
            }
            j=p->typf&15;
            if(c==ADDI2P||c==SUBIFP) j=POINTER;
            if((p->z.flags&(VAR|DREFOBJ))==(VAR|DREFOBJ)){
                v=p->z.v;
                if(!v->vtyp->next||(v->vtyp->next->flags&15)!=j) v->flags|=DNOTTYPESAFE;
                if(v->index<0) v->index=i++;
            }
        }
    }
    if(DEBUG&1024) printf("numerating variables loop3\n");
    rcount=i;    /*  Anzahl der DREFOBJ-Variablen    */
    /*  jetzt den Rest  */
    for(p=first_ic;p;p=p->next){
        if(p->code<LABEL||p->code>BRA){
            j=p->typf&15;
            if((p->q1.flags&(VAR|DREFOBJ))==VAR){
                v=p->q1.v;
                if((v->vtyp->flags&15)!=j) v->flags|=NOTTYPESAFE;
                if(v->index<0) v->index=i++;
            }
            if((p->q2.flags&(VAR|DREFOBJ))==VAR){
                v=p->q2.v;
                if((v->vtyp->flags&15)!=j) v->flags|=NOTTYPESAFE;
                if(v->index<0) v->index=i++;
            }
            if((p->z.flags&(VAR|DREFOBJ))==VAR){
                v=p->z.v;
                if((v->vtyp->flags&15)!=j) v->flags|=NOTTYPESAFE;
                if(v->index<0) v->index=i++;
            }
        }
    }
    if(DEBUG&1024) printf("numerating variables loop4\n");
    vcount=i+rcount; /*  alle benutzten Variablen+Anzahl der DREFOBJs    */
    vilist=mymalloc(vcount*sizeof(struct Var *));
    for(j=0;j<3;j++){
        int i;
        v=a[j];
        while(v){
            i=v->index;
/*            printf("%s has index %d\n",v->identifier,i);*/
            if(i>=0){
                if(i>=vcount-rcount) ierror(0);
                vilist[i]=v;
                if(i<rcount) vilist[i+vcount-rcount]=v;
            }
            /*  Variablen von inline-Funktionen */
            if(j==0&&v->fi&&v->fi->first_ic){
                for(vp=v->fi->vars;vp;vp=vp->next){
                    i=vp->index;
                    if(i>=0){
                        if(i>=vcount-rcount) ierror(0);
                        vilist[i]=vp;
                        if(i<rcount) vilist[i+vcount-rcount]=vp;
                    }
                }
            }
            v=v->next;
        }
    }

    vsize=(vcount+CHAR_BIT-1)/CHAR_BIT;
    if(DEBUG&1024) printf("%lu variables (%lu DREFOBJs), vsize=%lu\n",(unsigned long)vcount,(unsigned long)rcount,(unsigned long)vsize);

    av_drefs=mymalloc(vsize);
    memset(av_drefs,0,vsize);
    /*  alle DREFOBJs   */
    for(i=vcount-rcount;i<vcount;i++) BSET(av_drefs,i);

    /*  av_globals enthaelt alle globalen Variablen und av_address      */
    /*  zusaetzlich noch alle Variablen, deren Adressen genommen wurden */
    av_globals=mymalloc(vsize);
    memset(av_globals,0,vsize);
    av_statics=mymalloc(vsize);
    memset(av_statics,0,vsize);
    av_address=mymalloc(vsize);
    memcpy(av_address,av_globals,vsize);
    for(i=0;i<vcount-rcount;i++){
        if(vilist[i]->nesting==0||vilist[i]->storage_class==EXTERN) BSET(av_globals,i);
        if(vilist[i]->flags&USEDASADR) BSET(av_address,i);
        if(vilist[i]->storage_class==STATIC) BSET(av_statics,i);
        if(i<rcount){
/*            if((vilist[i]->vtyp->flags&15)!=POINTER){ printf("%s(%ld)\n",vilist[i]->identifier,zl2l(vilist[i]->offset));ierror(0);}*/
            BSET(av_address,i+vcount-rcount);
            BSET(av_globals,i+vcount-rcount);
        }
    }
}
void print_vi(void)
/*  Druckt vilist und testet Konsistenz */
{
    int i;
    printf("\nprint_vi()\n");
    for(i=0;i<vcount;i++){
        if(!vilist[i]||(i<rcount&&vilist[i]->index!=i)) ierror(0);
        printf("%3d: %s\n",i,vilist[i]->identifier);
    }
}
void av_change(struct IC *p,unsigned char *use,unsigned char *def)
/*  Berechnet die Aenderungen, die sich durch IC p an use und def ergeben.  */
{
    int i,j,n=-1;
    int g1,g2;

    /*  Wenn eine Quelle==Ziel, dann wird dadurch kein neuer use erzeugt,   */
    /*  um z.B. unbenutzte Induktionsvariablen in Schleifen zu eliminieren. */
    g1=compare_objs(&p->q1,&p->z,p->typf);
    g2=compare_objs(&p->q2,&p->z,p->typf);
    if(!g1&&(p->q1.flags&(VAR|DREFOBJ))==VAR) n=p->q1.v->index;
    if(!g2&&(p->q2.flags&(VAR|DREFOBJ))==VAR) n=p->q2.v->index;

    for(j=0;j<p->use_cnt;j++){
        i=p->use_list[j].v->index;
        if(p->use_list[j].flags&DREFOBJ) i+=vcount-rcount;
        if(i>=vcount) continue;
        if(i!=n&&!BTST(def,i)) BSET(use,i);
    }

    /*  Ein Wert wird nicht zerstoert, wenn es kein elementarer Typ ist und */
    /*  die Groesse kleiner als die Variable (steht in alle solchen ICs in  */
    /*  q2.val.vlong.                                                       */
    if((p->z.flags&(VAR|DREFOBJ))==VAR&&g1&&g2&&((p->z.v->vtyp->flags&15)<=POINTER||zleqto(p->q2.val.vlong,szof(p->z.v->vtyp)))){
        i=p->z.v->index;
        if(i>=vcount) ierror(0);
        if(!BTST(use,i)) BSET(def,i);
        /*  Wenn p geaendert wird, wird auch *p geaendert   */
        if(i<rcount&&!BTST(def,i+vcount-rcount)) BSET(use,i+vcount-rcount);
    }
    if((p->z.flags&(VAR|DREFOBJ))==(VAR|DREFOBJ)&&g1&&g2){
        i=p->z.v->index+vcount-rcount;
        if(i>=vcount) ierror(0);
        if(!BTST(use,i)) BSET(def,i);
    }
}
void active_vars(struct flowgraph *fg)
/*  analysiert aktive Variablen im Flussgraphen, nomain==0, wenn zu     */
/*  optimierende Funktion main() ist                                    */
{
    struct IC *p;
    int changed,pass;struct flowgraph *g;

    if(DEBUG&1024){printf("analysing active variables\n");/*scanf("%d",&i);*/}
    tmp=mymalloc(vsize);
    /*  av_gen und av_kill fuer jeden Basic Block berechnen */
    if(DEBUG&1024){printf("active_vars(): loop1\n");/*scanf("%d",&i);*/}
    g=fg;
    while(g){
        g->av_gen=mymalloc(vsize);
        memset(g->av_gen,0,vsize);
        g->av_kill=mymalloc(vsize);
        memset(g->av_kill,0,vsize);
        g->av_in=mymalloc(vsize);
        memset(g->av_in,0,vsize);
        g->av_out=mymalloc(vsize);
        memset(g->av_out,0,vsize);
        for(p=g->start;p;p=p->next){
            av_change(p,g->av_gen,g->av_kill);
            if(p==g->end) break;
        }
        g=g->normalout;
    }

    /*  av_in und av_out fuer alle Bloecke berechnen    */
    if(DEBUG&1024){printf("active_vars(): loop2\npass: ");/*scanf("%d",&i);*/}
    pass=0;
    do{
        if(DEBUG&1024) {printf(" %d",++pass);fflush(stdout);}
        changed=0;
        g=fg;
        while(g){
            /* out(B)=U in(C) ueber alle Nachfolger C von B */
            memset(g->av_out,0,vsize);  /*  noetig? */
            if(g->branchout) bvunite(g->av_out,g->branchout->av_in,vsize);
            if((!g->end||g->end->code!=BRA)&&g->normalout) bvunite(g->av_out,g->normalout->av_in,vsize);
            /*  Am Ende muessen alle globalen Variablen bekannt sein    */
            if(!g->normalout){
                bvunite(g->av_out,av_globals,vsize);
                /*if(!nocall)*/ bvunite(g->av_out,av_statics,vsize);
            }
            /* in(B)=use(B)U(out(B)-def(B)) */
            memcpy(tmp,g->av_out,vsize);
            bvdiff(tmp,g->av_kill,vsize);
            bvunite(tmp,g->av_gen,vsize);

            if(!bvcmp(tmp,g->av_in,vsize)){changed=1;memcpy(g->av_in,tmp,vsize);}
            g=g->normalout;
        }
    }while(changed);
    if(DEBUG&1024) printf("\n");
    free(tmp);
}
int dead_assignments(struct flowgraph *fg)
/*  Findet Zuweisungen, die unnoetig sind, da die Variable nie mehr     */
/*  benutzt werden kann.                                                */
{
    int changed=0;struct IC *p;unsigned char *isused;
    int i,j;
    if(DEBUG&1024) printf("searching for dead assignments\n");
    isused=mymalloc(vsize);
    while(fg){
        memcpy(isused,fg->av_out,vsize);
        p=fg->end;
        while(p){
            if(p->z.flags&VAR){
                i=p->z.v->index;
                if(p->z.flags&DREFOBJ) i+=vcount-rcount;
                if(!BTST(isused,i)&&!(p->typf&VOLATILE)){
                    if(DEBUG&1024){printf("dead assignment deleted:\n");pric2(stdout,p);}
                    if(*p->z.v->identifier&&p->code!=ASSIGN){ err_ic=p;error(170,i>=vcount-rcount?"*":"",p->z.v->identifier);err_ic=0;}
                    if(p->code!=GETRETURN) changed=1;
                    if(p==fg->start){remove_IC_fg(fg,p);break;}
                    p=p->prev;remove_IC_fg(fg,p->next);
                    continue;
                }
                if((p->z.v->vtyp->flags&15)<=POINTER||zleqto(p->q2.val.vlong,szof(p->z.v->vtyp)))
                    BCLR(isused,i);
                /*  bei Zuweisung an p wird *p aktiv    */
                if(i<rcount) BSET(isused,i+vcount-rcount);
            }
            for(j=0;j<p->use_cnt;j++){
                i=p->use_list[j].v->index;
                if(p->use_list[j].flags&DREFOBJ) i+=vcount-rcount;
                if(i>=vcount) continue;
                BSET(isused,i);
            }

            if(p==fg->start) break;
            p=p->prev;
        }
        fg=fg->normalout;
    }
    free(isused);
    return(changed);
}

