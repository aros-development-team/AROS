/*  $VER: vbcc (regs.c) V0.4    */
/*  Registerzuteilung           */

#include "opt.h"

static char FILE_[]=__FILE__;

#ifndef NO_OPTIMIZER

int (*savings)[MAXR+1],regu[MAXR+1];
int *rvlist;

int cmp_savings(const void *v1,const void *v2)
/*  Vergleichsfkt, um rvlist nach savings zu sortieren  */
{
    return savings[*(int *)v2][0]-savings[*(int *)v1][0];
}
int entry_load(struct flowgraph *fg,int i)
/*  Testet, ob die Variable in Register i am Anfang von Block fg geladen    */
/*  werden muss, d.h. ein Vorgaenger sie nicht im selben Register hat.      */
{
    struct flowlist *lp;
    lp=fg->in;
    /*  Parameter am Anfang laden?  */
/*    if(fg==fg1&&!zlleq(l2zl(0L),fg->regv[i]->offset)) return(1);*/
    if(!lp) return(1);
    while(lp){
        if(lp->graph&&lp->graph->regv[i]!=fg->regv[i]&&BTST(lp->graph->av_out,fg->regv[i]->index)) return(1);
        lp=lp->next;
    }
    return(0);
}

int exit_save(struct flowgraph *fg,int i)
/*  Testet, ob die Variable in Register i am Ende von Block fg gespeichert  */
/*  werden muss, d.h. der Vorgaenger eines Nachfolgers nicht dieselbe       */
/*  Variable im selben Register hat.                                        */
{
    struct flowlist *lp;
    if((fg->normalout&&(!fg->end||fg->end->code!=BRA))&&BTST(fg->normalout->av_in,fg->regv[i]->index)){
        if(fg->normalout->regv[i]!=fg->regv[i]) return(1);
        lp=fg->normalout->in;
        while(lp){
            if(lp->graph&&lp->graph->regv[i]!=fg->regv[i]) return(1);
            lp=lp->next;
        }
    }
    if(fg->branchout&&BTST(fg->branchout->av_in,fg->regv[i]->index)){
        if(fg->branchout->regv[i]!=fg->regv[i]) return(1);
        lp=fg->branchout->in;
        while(lp){
            if(lp->graph&&lp->graph->regv[i]!=fg->regv[i]) return(1);
            lp=lp->next;
        }
    }
    return(0);
}
void load_reg_parms(struct flowgraph *fg)
/*  Laedt Registerparameter, falls noetig.                              */
{
    int i; struct Var *v;
    for(i=0;i<vcount-rcount;i++){
        v=vilist[i];
        if((v->flags&REGPARM)&&fg->regv[v->reg]!=v&&(BTST(fg->av_in,i)||(v->flags&USEDASADR))){
            struct IC *new; int j;
            insert_allocreg(fg,0,FREEREG,v->reg);
            new=mymalloc(ICS);
            new->line=0;
            new->file=0;
            new->code=ASSIGN;
            new->typf=v->vtyp->flags;
            new->q1.flags=REG;
            new->q1.reg=v->reg;
            new->q2.flags=0;
            new->q2.val.vlong=szof(v->vtyp);
            new->z.flags=VAR;
            new->z.val.vlong=l2zl(0L);
            new->z.v=v;
            for(j=1;j<=MAXR;j++)
                if(fg->regv[j]==v){ new->z.flags|=REG;new->z.reg=j;break; }
            new->q1.am=new->q2.am=new->z.am=0;
            new->use_cnt=new->change_cnt=0;
            new->use_list=new->change_list=0;
            insert_IC_fg(fg,0,new);
            insert_allocreg(fg,0,ALLOCREG,v->reg);
            if(new->z.flags&REG){
                /*  ALLOCREG verschieben    */
                struct IC *p;
                insert_allocreg(fg,0,ALLOCREG,new->z.reg);
                for(p=new->next;p;p=p->next){
                    if(p->code==ALLOCREG&&p->q1.reg==new->z.reg){
                        remove_IC_fg(fg,p);
                        break;
                    }
                }
                if(!p) ierror(0);
            }
            v->offset=l2zl(0);
        }
    }
}
void insert_regs(struct flowgraph *fg1)
/*  Fuegt Registervariablen in die ICs ein.                             */
{
    int i;struct IC *p,*lic=0,*new;struct flowgraph *lfg=0,*fg;
    if(DEBUG&9216) printf("inserting register variables\n");
    fg=fg1;
    while(fg){
        if(DEBUG&8192) printf("block %d:\n",fg->index);
        p=fg->start;
        while(p){
            for(i=1;i<=MAXR;i++){
                if(!fg->regv[i]) continue;
                if(p->code==ALLOCREG&&p->q1.reg==i) ierror(0);
                if((p->q1.flags&(VAR|DONTREGISTERIZE))==VAR&&p->q1.v==fg->regv[i]){
                    p->q1.flags|=REG;
                    p->q1.reg=i;
                }
                if((p->q2.flags&(VAR|DONTREGISTERIZE))==VAR&&p->q2.v==fg->regv[i]){
                    p->q2.flags|=REG;
                    p->q2.reg=i;
                }
                if((p->z.flags&(VAR|DONTREGISTERIZE))==VAR&&p->z.v==fg->regv[i]){
                    p->z.flags|=REG;
                    p->z.reg=i;
                }
            }
            if(DEBUG&8192) pric2(stdout,p);
            if(p==fg->end) break;
            p=p->next;
        }
        if(fg->start&&fg->start->code==LABEL) lic=fg->start;
        for(i=1;i<=MAXR;i++){
            if(fg->regv[i]){
                if(DEBUG&8192){
                    printf("(%s),%ld assigned to %s\n",fg->regv[i]->identifier,zl2l(fg->regv[i]->offset),regnames[i]);
                    if(BTST(fg->av_in,fg->regv[i]->index)) printf("active at the start of block\n");
                    if(BTST(fg->av_out,fg->regv[i]->index)) printf("active at the end of block\n");
                }

                if(BTST(fg->av_out,fg->regv[i]->index)){
                /*  Variable beim Austritt aktiv?   */
                    if(exit_save(fg,i)){
                        struct IC *tp;
                        if(DEBUG&8192) printf("\thave to save it at end of block\n");
                        new=mymalloc(ICS);
                        new->line=0;
                        new->file=0;
                        new->code=ASSIGN;
                        new->typf=fg->regv[i]->vtyp->flags;
                        /*  cc  */
                        if(new->typf==0) ierror(0);
                        new->q1.flags=VAR|REG;
                        new->q1.val.vlong=l2zl(0L);
                        new->q1.v=fg->regv[i];
                        new->q1.reg=i;
                        new->q2.flags=0;
                        new->q2.val.vlong=szof(fg->regv[i]->vtyp);
                        new->z.flags=VAR|DONTREGISTERIZE;
                        new->z.val.vlong=l2zl(0L);
                        new->z.v=fg->regv[i];
                        new->q1.am=new->q2.am=new->z.am=0;
                        new->use_cnt=new->change_cnt=0;
                        new->use_list=new->change_list=0;
                        /*  Vor FREEREGs und evtl. Branch+COMPARE/TEST setzen   */
                        if(fg->end){
                            tp=fg->end;
                            while(tp!=fg->start&&tp->code==FREEREG)
                                tp=tp->prev;
                            if(tp&&tp->code>=BEQ&&tp->code<=BRA){
                                if(tp->code<BRA){
                                    int c;
                                    do{
                                        tp=tp->prev;
                                        c=tp->code;
                                        if(c!=FREEREG&&c!=COMPARE&&c!=TEST) ierror(0);
                                    }while(c!=COMPARE&&c!=TEST);
                                }
                                tp=tp->prev;
                            }
                        }else tp=lic;
                        insert_IC_fg(fg,tp,new);
                    }
                }
                if(BTST(fg->av_in,fg->regv[i]->index)){
                    if(entry_load(fg,i)&&(fg!=fg1||!(fg->regv[i]->flags&REGPARM))){
                        if(DEBUG&8192) printf("\thave to load it at start of block\n");

                        new=mymalloc(ICS);
                        new->line=0;
                        new->file=0;
                        new->code=ASSIGN;
                        new->typf=fg->regv[i]->vtyp->flags;
                        /*  cc  */
                        if(new->typf==0) ierror(0);
                        new->q1.flags=VAR|DONTREGISTERIZE;
                        new->q1.val.vlong=l2zl(0L);
                        new->q1.v=fg->regv[i];
                        new->q2.flags=0;
                        new->q2.val.vlong=szof(fg->regv[i]->vtyp);
                        new->z.flags=VAR|REG;
                        new->z.val.vlong=l2zl(0L);
                        new->z.v=fg->regv[i];
                        new->z.reg=i;
                        new->q1.am=new->q2.am=new->z.am=0;
                        new->use_cnt=new->change_cnt=0;
                        new->use_list=new->change_list=0;
                        insert_IC_fg(fg,lic,new);
                    }
                }
                if(!lfg||!lfg->regv[i]) insert_allocreg(fg,lic,ALLOCREG,i);
                if(!fg->normalout||!fg->normalout->regv[i])
                    insert_allocreg(fg,fg->end?fg->end:lic,FREEREG,i);
            }
        }
        if(fg->end) lic=fg->end;
        lfg=fg;
        fg=fg->normalout;
    }
    load_reg_parms(fg1);
}

void do_loop_regs(struct flowgraph *start,struct flowgraph *end)
/*  Macht die Variablenzuweisung in Schleife start-end.                 */
/*  Wenn end==0 Registerzuweisung fuer die ganze Funktion, ansonsten    */
/*  fuer die Schleife, die zum Header start gehoert.                    */
{
    struct flowgraph *g;
    int i,r;
    struct Var *lregs[MAXR+1]={0};
    unsigned char rused[(MAXR+CHAR_BIT)/CHAR_BIT];
    /*  Berechnen, wieviel ungefaehr eingespart wird, wenn eine Variable    */
    /*  fuer diese Schleife in einem best. Register gehalten wird.          */
    /*  Die savings in einer Schleife werden multipliziert, um das          */
    /*  Laden/Speichern ausserhalb der Schleife geringer zu wichten.        */
/*    if(end&&(!g->normalout||!g->normalout->loopend||g->normalout->loopend->normalout->index!=-2)) ierror(0);*/
    /*  alle auf 0  */
    for(i=0;i<vcount-rcount;i++){
        for(r=1;r<=MAXR;r++){
            savings[i][r]=0;
        }
    }
    if(end){
        struct Var *v;
        /*  Evtl. Kosten fuers Laden/Speichern beim Ein-/Austritt in die    */
        /*  Schleife.                                                       */
        end=start->normalout->loopend;
        g=end->normalout;
        if(DEBUG&9216) printf("assigning regs to blocks %d to %d\n",start->normalout->index,end->index);
        /*  Werte modifizieren, falls Variable am Anfang/Ende der Schleife  */
        /*  geladen/gespeichert werden muss.                                */
        for(i=0;i<vcount-rcount;i++){
            v=vilist[i];
            if(BTST(start->av_in,i)){
                for(r=1;r<=MAXR;r++)
                    if(start->regv[r]!=v) savings[i][r]--;
            }
            if(BTST(g->av_out,i)){
                for(r=1;r<=MAXR;r++)
                    if(g->regv[r]!=v) savings[i][r]--;
            }
        }
        /*  Werte modifizieren, falls eine andere Variable gespeichert oder */
        /*  geladen werden muss. Hmm..stimmt das so?                        */
        for(r=1;r<=MAXR;r++){
            v=start->regv[r];
            if(v&&BTST(start->av_in,v->index)){
                for(i=0;i<vcount-rcount;i++)
                    if(v->index!=i) savings[i][r]--;
            }
            if(v&&BTST(g->av_out,v->index)){
                for(i=0;i<vcount-rcount;i++)
                    if(v->index!=i) savings[i][r]--;
            }
        }
        g=start->normalout;
    }else{
        /*  Bei Registervergabe fuer die ganze Funktion muessen alle beim   */
        /*  Eintritt der Funktion aktiven Variablen geladen werden.         */
        if(DEBUG&9216) printf("assigning regs to whole function\n");
        for(i=0;i<vcount-rcount;i++){
            if(BTST(start->av_in,i)){
                int pr=vilist[i]->reg;
                for(r=1;r<=MAXR;r++){
                    if(pr==0||!regok(r,vilist[i]->vtyp->flags,0)||regsa[r]){
                        savings[i][r]-=8;
                    }else{
                        if(r==pr) savings[i][r]+=8; else savings[i][r]+=4;
                    }
                }
            }
        }
        g=start;
    }
    if(DEBUG&9216) printf("calculating approximate savings\n");

    for(;g;g=g->normalout){
        struct IC *p;struct Var *v;
        int t,vt;
        if(g->calls>0){
        /*  bei Funktionsaufrufen muessen Scratchregister gespeichert werden */
            for(r=1;r<=MAXR;r++)
                if(regscratch[r])
                    for(i=0;i<vcount-rcount;i++) savings[i][r]-=g->calls*16;
        }
        /*  Wenn das Register in dem Block benutzt wird, muss man es retten */
        for(r=1;r<=MAXR;r++){
            if(BTST(g->regused,r)){
                int vi;
                if(g->regv[r]) vi=g->regv[r]->index; else vi=-1;
                for(i=0;i<vcount-rcount;i++)
                    if(vi!=i) savings[i][r]-=16;
            }
        }
        for(p=g->start;p;p=p->next){
            if((p->q1.flags&(VAR|VARADR|REG))==VAR){
                v=p->q1.v;
                if((v->storage_class==AUTO||v->storage_class==REGISTER)&&!(v->flags&USEDASADR)){
                    vt=v->vtyp->flags&31;
                    i=v->index;
                    if(p->q1.flags&DREFOBJ) t=p->typf&31; else t=0;
                    for(r=1;r<=MAXR;r++){
                        if(!regsa[r]&&!BTST(g->regused,r)){
                            /*  extra saving, falls passendes Reg fuer DREF */
                            if(t&&regok(r,vt,t)) savings[i][r]+=8;
                            if(regok(r,vt,0)) savings[i][r]+=8;
                        }
                    }
                }
            }
            if((p->q2.flags&(VAR|VARADR|REG))==VAR){
                v=p->q2.v;
                if((v->storage_class==AUTO||v->storage_class==REGISTER)&&!(v->flags&USEDASADR)){
                    vt=v->vtyp->flags&31;
                    i=v->index;
                    if(p->q2.flags&DREFOBJ) t=p->typf&31; else t=0;
                    for(r=1;r<=MAXR;r++){
                        if(!regsa[r]&&!BTST(g->regused,r)){
                            /*  extra saving, falls passendes Reg fuer DREF */
                            if(t&&regok(r,vt,t)) savings[i][r]+=8;
                            if(regok(r,vt,0)) savings[i][r]+=8;
                        }
                    }
                }
            }
            if((p->z.flags&(VAR|VARADR|REG))==VAR){
                v=p->z.v;
                if((v->storage_class==AUTO||v->storage_class==REGISTER)&&!(v->flags&USEDASADR)){
                    vt=v->vtyp->flags&31;
                    i=v->index;
                    if(p->z.flags&DREFOBJ) t=p->typf&31; else t=0;
                    for(r=1;r<=MAXR;r++){
                        if(!regsa[r]&&!BTST(g->regused,r)){
                            /*  extra saving, falls passendes Reg fuer DREF */
                            if(t&&regok(r,vt,t)) savings[i][r]+=8;
                            if(regok(r,vt,0)) savings[i][r]+=8;
                        }
                    }
                }
            }
            if(p==g->end) break;
        }
        if(g==end) break;
    }
    /*  Maximum ermitteln   */
    for(i=0;i<vcount-rcount;i++){
        int m=0;
        for(r=1;r<=MAXR;r++){
            /*  Falls Variable in best. Register muss.  */
            if(r==vilist[i]->reg&&!(vilist[i]->flags&REGPARM)) savings[i][r]*=4;
            if(savings[i][r]>m) m=savings[i][r];
        }
        savings[i][0]=m;
    }

    if(DEBUG&8192){
        for(i=0;i<vcount-rcount;i++){
            printf("(%s),%ld(best=%d):\n",vilist[i]->identifier,zl2l(vilist[i]->offset),savings[i][0]);
            for(r=1;r<=MAXR;r++)
                printf("%s=%d ",regnames[r],savings[i][r]);
            printf("\n");
        }
    }
    /*  Suchen, welche Variablen/Registerkombination das beste Ergebnis */
    /*  liefert. Nur angenaehert, da sonst wohl zu aufwendig. Simplex?  */
    memset(rused,0,(MAXR+CHAR_BIT)/CHAR_BIT);
    for(i=0;i<vcount-rcount;i++) rvlist[i]=i;
    qsort(rvlist,vcount-rcount,sizeof(*rvlist),cmp_savings);
    for(i=0;i<vcount-rcount;i++){
        int use,m=0,vi;
        vi=rvlist[i];
        if(vilist[vi]->flags&USEDASADR) continue;
        if(DEBUG&8192) printf("%d: (%s),%ld(best=%d)\n",i,vilist[vi]->identifier,zl2l(vilist[vi]->offset),savings[vi][0]);
        for(r=1;r<=MAXR;r++){
            if(!lregs[r]&&savings[vi][r]>m){
                m=savings[vi][r];
                use=r;
                if(m==savings[vi][0]) break;
            }
        }
        if(m>0){
            if(DEBUG&9216) printf("assigned (%s),%ld to %s, saving=%d\n",vilist[vi]->identifier,zl2l(vilist[vi]->offset),regnames[use],m);
            lregs[use]=vilist[vi];
            BSET(rused,use);
        }
    }
    /*  Registervariablen in alle Bloecke der Schleife eintragen    */
    /*  dabei beruecksichtigen, dass sie in manchen Bloecken nicht  */
    /*  in Register kommen koennen, wenn das Register da schon von  */
    /*  local_regs benutzt wird                                     */
    /*  Gegebenenfalls auch in Header/Footer einer Schleife         */
    /*  eintragen.                                                  */
    if(DEBUG&9216) printf("propagate register vars\n");
    for(g=start;g;g=g->normalout){
        for(r=1;r<=MAXR;r++){
            if(lregs[r]&&!BTST(g->regused,r)){
                /*  Falls Variable schon in anderem Register, loeschen  */
                for(i=1;i<=MAXR;i++){
                    if(g->regv[i]==lregs[r]) g->regv[i]=0;
                }
                g->regv[r]=lregs[r];
            }
        }
        if(end&&g==end->normalout) break;
    }
}
void block_regs(struct flowgraph *fg)
/*  macht die Variablenzuweisung fuer einzelne Bloecke  */
{
    struct flowgraph *g,**fgp;
    int i,r,changed,fgz;
    if(DEBUG&9216) printf("block_regs\n");

    savings=mymalloc((vcount-rcount)*sizeof(*savings));
    rvlist=mymalloc((vcount-rcount)*sizeof(*rvlist));

    /*  Array auf Bloecke im Flussgraphen mangels doppelter Verkettung  */
    fgp=mymalloc(basic_blocks*sizeof(*fgp));
    g=fg;fgz=0;
    while(g){
        fgp[fgz]=g;fgz++;
        g=g->normalout;
    }
    if(fgz>basic_blocks) ierror(0); else basic_blocks=fgz;
    /*  alle auf 0  */
    do{
        changed=0;
        if(DEBUG&9216) printf("block_regs pass\n");
        for(fgz=basic_blocks-1;fgz>=0;fgz--){
            struct IC *p;struct Var *v;struct flowlist *lp;
            int t,vt;
            g=fgp[fgz];
            if(DEBUG&8192) printf("assigning regs to block %d\n",g->index);
            /*  berechnen, wieviel ungefaehr eingespart wird, wenn eine Variable    */
            /*  fuer diesen Block in einem best. Register gehalten wird             */
            if(DEBUG&8192) printf("calculating approximate savings\n");

            for(i=0;i<vcount-rcount;i++){
                for(r=1;r<=MAXR;r++){
                    if(!g->regv[r]||g->regv[r]->index!=i){
                        int w=0;
                        /*  Variable muss evtl. geladen/gespeichert werden  */
                        if(BTST(g->av_in,i)) w--;
                        if(BTST(g->av_out,i)) w--;
                        savings[i][r]=w;
                    }
                }
            }
            if(g->calls>0){
            /*  bei Funktionsaufrufen muessen Scratchregister gespeichert werden */
                for(r=1;r<=MAXR;r++)
                    if(regscratch[r])
                        for(i=0;i<vcount-rcount;i++) savings[i][r]-=g->calls*2;
            }
            /*  Wenn Vorgaenger/Nachfolger selbe Variable im selben */
            /*  Register hat, entfaellt Laden/Speichern in diesem   */
            /*  Block und vermutlich auch im Vorgaenger/Nachfolger  */
            /*  nicht immer, aber naeherungsweise...                */
            lp=g->in;
            while(lp){
                if(lp->graph){
                    for(r=1;r<=MAXR;r++){
                        if(lp->graph->regv[r]&&BTST(g->av_in,lp->graph->regv[r]->index)) savings[lp->graph->regv[r]->index][r]+=2;
                    }
                }
                lp=lp->next;
            }
            if(g->branchout){
                for(r=1;r<=MAXR;r++){
                    if(g->branchout->regv[r]&&BTST(g->av_out,g->branchout->regv[r]->index)) savings[g->branchout->regv[r]->index][r]+=2;
                }
            }
            if(g->normalout&&(!g->normalout->end||g->normalout->end->code!=BRA)){
                for(r=1;r<=MAXR;r++){
                    if(g->normalout->regv[r]&&BTST(g->av_out,g->normalout->regv[r]->index)) savings[g->normalout->regv[r]->index][r]+=2;
                }
            }

            p=g->start;
            while(p){
                if((p->q1.flags&(VAR|VARADR|REG))==VAR){
                    v=p->q1.v;
                    if((v->storage_class==AUTO||v->storage_class==REGISTER)&&!(v->flags&USEDASADR)){
                        vt=v->vtyp->flags&31;
                        i=v->index;
                        if(p->q1.flags&DREFOBJ) t=p->typf&31; else t=0;
                        for(r=1;r<=MAXR;r++){
                            if(!regsa[r]&&!BTST(g->regused,r)){
                                /*  extra saving, falls passendes Reg fuer DREF */
                                if(t&&regok(r,vt,t)) savings[i][r]++;
                                if(regok(r,vt,0)) savings[i][r]++;
                            }
                        }
                    }
                }
                if((p->q2.flags&(VAR|VARADR|REG))==VAR){
                    v=p->q2.v;
                    if((v->storage_class==AUTO||v->storage_class==REGISTER)&&!(v->flags&USEDASADR)){
                        vt=v->vtyp->flags&31;
                        i=v->index;
                        if(p->q2.flags&DREFOBJ) t=p->typf&31; else t=0;
                        for(r=1;r<=MAXR;r++){
                            if(!regsa[r]&&!BTST(g->regused,r)){
                                /*  extra saving, falls passendes Reg fuer DREF */
                                if(t&&regok(r,vt,t)) savings[i][r]++;
                                if(regok(r,vt,0)) savings[i][r]++;
                            }
                        }
                    }
                }
                if((p->z.flags&(VAR|VARADR|REG))==VAR){
                    v=p->z.v;
                    if((v->storage_class==AUTO||v->storage_class==REGISTER)&&!(v->flags&USEDASADR)){
                        vt=v->vtyp->flags&31;
                        i=v->index;
                        if(p->z.flags&DREFOBJ) t=p->typf&31; else t=0;
                        for(r=1;r<=MAXR;r++){
                            if(!regsa[r]&&!BTST(g->regused,r)){
                                /*  extra saving, falls passendes Reg fuer DREF */
                                if(t&&regok(r,vt,t)) savings[i][r]++;
                                if(regok(r,vt,0)) savings[i][r]++;
                            }
                        }
                    }
                }
                if(p==g->end) break;
                p=p->next;
            }
            /*  moegliche Kandidaten suchen; muss nicht immer die beste */
            /*  Kombination finden, sollte aber bei lokaler Vergabe     */
            /*  selten einen Unterschied machen                         */
            for(r=1;r<=MAXR;r++){
                if(g->regv[r]||BTST(g->regused,r)) continue;
                for(i=0;i<vcount-rcount;i++){
                    if(savings[i][r]>0){
                        int flag;struct Var *v=vilist[i];
                        /*  Variable schon in anderem Register? */
                        for(flag=1;flag<=MAXR;flag++)
                            if(g->regv[flag]==v){flag=-1;break;}
                        if(flag>0){
                            if(DEBUG&9216) printf("assigned (%s),%ld to %s; saving=%d\n",vilist[i]->identifier,zl2l(vilist[i]->offset),regnames[r],savings[i][r]);
                            g->regv[r]=vilist[i];
                            changed=1;
                            break;
                        }
                    }
                }
            }
        }
    }while(changed);
    /*  jetzt nochmal globale Register vergeben */
/*    do_loop_regs(fgp[0],fgp[basic_blocks-1]);*/

    free(fgp);
    free(rvlist);
    free(savings);
}

void loop_regs(struct flowgraph *fg)
/*  weist Variablen in Schleifen Register zu    */
{
    struct flowgraph *g;
    if(DEBUG&9216) printf("assigning regs to function\n");
    savings=mymalloc((vcount-rcount)*sizeof(*savings));
    rvlist=mymalloc((vcount-rcount)*sizeof(*rvlist));
    do_loop_regs(fg,0);
    if(DEBUG&9216) printf("assigning regs in loops\n");
    for(g=fg;g;g=g->normalout){
        if(g->index==-1) do_loop_regs(g,g);
    }
    free(rvlist);
    free(savings);
}
void insert_allocreg(struct flowgraph *fg,struct IC *p,int code,int reg)
/*  Fuegt ein ALLOCREG/FREEREG (in code) hinter p ein - bei p==0 in */
/*  first_ic.                                                       */
{
    struct IC *new=mymalloc(ICS);
    new->line=0;
    new->file=0;
    BSET(fg->regused,reg);
    regused[reg]=1;
    new->code=code;
    new->typf=0;
    new->q1.am=new->q2.am=new->z.am=0;
    new->q1.flags=REG;
    new->q1.reg=reg;
    new->q2.flags=new->z.flags=0;
    new->use_cnt=new->change_cnt=0;
    new->use_list=new->change_list=0;
    insert_IC_fg(fg,p,new);
}

struct Var *lregv[MAXR+1];
struct flowgraph *lfg;

void free_hreg(struct flowgraph *fg,struct IC *p,int reg,int mustr)
/*  Macht das Register reg frei, damit es als lokale Variable im IC p   */
/*  zur Verfuegung steht. Wenn mustr!=0, muss das Register unbedingt    */
/*  freigemacht werden, ansonsten kann davon abgesehen werden.          */
{
    struct IC *m,*first;struct Var *v;
    int preg[MAXR+1]={0},calls=0,rreg,i;
    first=0; v=lregv[reg];
    if(!v) ierror(0);
    if(DEBUG&8192) printf("free_hreg %s,%s,%d\n",regnames[reg],v->identifier,mustr);
    if(v->reg) error(218,regnames[reg]);
    for(m=p;m;m=m->next){
        if(m->code==CALL) calls++;
        if(m->code==ALLOCREG){
            preg[m->q1.reg]=1;
            if(m->q1.reg==reg) ierror(0);
        }
        if(m->code==FREEREG){
            preg[m->q1.reg]=1;
            if(m->q1.reg==reg) break;
        }
        if(!USEQ2ASZ){
            if((m->q2.flags&VAR)&&m->q2.v==v&&(m->z.flags&(REG|DREFOBJ))==REG&&
               (!(m->z.flags&VAR)||m->z.v!=v))
                preg[m->z.reg]=1;
        }
        if(((m->q1.flags&VAR)&&m->q1.v==v)||
           ((m->q2.flags&VAR)&&m->q2.v==v)||
           ((m->z.flags&(VAR|DREFOBJ))==(VAR|DREFOBJ)&&m->z.v==v))
            first=m;
/*        if((m->z.flags&(REG|DREFOBJ))==REG&&m->z.reg==reg) break;*/
    }
    if(!first) {pric(stdout,p);ierror(0);}
    for(rreg=0,i=1;i<=MAXR;i++){
        if(preg[i]||regu[i]||regsa[i]||!regok(i,v->vtyp->flags,0)) continue;
        if(calls==0&&regscratch[i]){rreg=i;break;}
        if(calls>0&&!regscratch[i]){rreg=i;break;}
        if(calls==0&&mustr) rreg=i;
    }
    if(!rreg&&!mustr) return;
    for(m=p;m!=first->next;m=m->next){
        if((m->q1.flags&VAR)&&m->q1.v==v)
            {if(!rreg) m->q1.flags&=~REG; else m->q1.reg=rreg;}
        if((m->q2.flags&VAR)&&m->q2.v==v)
            {if(!rreg) m->q2.flags&=~REG; else m->q2.reg=rreg;}
        if((m->z.flags&VAR)&&m->z.v==v)
            {if(!rreg) m->z.flags&=~REG; else m->z.reg=rreg;}
    }
    if(rreg){lregv[rreg]=lregv[reg];regused[rreg]=1;regu[rreg]=1;BSET(fg->regused,rreg);}
    lregv[reg]=0;regu[reg]=0;

    for(m=first->next;m&&m->code==FREEREG;m=m->next){
        if(m->q1.reg==reg){
            if(!rreg) remove_IC_fg(fg,m); else m->q1.reg=rreg;
            if(rreg) insert_allocreg(fg,first,FREEREG,rreg);
            return;
        }
    }
    insert_allocreg(fg,first->prev,ALLOCREG,reg);
    if(rreg) insert_allocreg(fg,first,FREEREG,rreg);
}
int replace_local_reg(struct obj *o)
/*  tested, ob o eine Scratch-Variable ist und ersetzt sie gegebenenfalls   */
{
    int i;struct Var *v;
    if((o->flags&(VAR|REG|VARADR))==VAR){
        v=o->v;i=v->index;
        if(BTST(lfg->av_kill,i)&&!BTST(lfg->av_out,i)){
            for(i=1;i<=MAXR;i++){
                if(lregv[i]==v){
                    o->flags|=(REG|SCRATCH);
/*                    o->flags&=~VAR;*/
                    o->reg=i;
                    return(i);
                }
            }
        }
    }
    return(0);
}
void local_regs(struct flowgraph *fg)
/*  versucht Variablen, die nur innerhalb eines Basic Blocks benutzt    */
/*  werden (kill==true und out==false), Register zuzuweisen.            */
{
    struct IC *p;
    int i,t,r,nr,mustalloc;
    if(DEBUG&9216) printf("assigning temporary variables to registers\n");
    lfg=fg;
    while(lfg){
        if(DEBUG&1024) printf("block %d\n",lfg->index);
        for(i=1;i<=MAXR;i++){lregv[i]=0; regu[i]=regsa[i]; lfg->regv[i]=0;}
        memset(&lfg->regused,0,(MAXR+CHAR_BIT)/CHAR_BIT);
        lfg->calls=0;
        p=lfg->end;
        while(p){
            i=replace_local_reg(&p->z);
            if(i&&!(p->z.flags&DREFOBJ)){
                lregv[i]=0;regu[i]--;
                nr=i;mustalloc=1;
                if(DEBUG&8192) printf("regu[%s] decremented to %d\n",regnames[i],regu[i]);
            }else nr=0;
            if(p->code!=ADDRESS){
                if(replace_local_reg(&p->q1)==nr) mustalloc=0;
                if(replace_local_reg(&p->q2)==nr) mustalloc=0;
            }
            /*  hier wegen USEQ2ASZ aufpassen; kommutative ICs sollten so   */
            /*  angeordnet werden, dass ein evtl. Register rechts steht     */
            if((p->q2.flags&(VAR|REG|VARADR))==VAR&&!(p->q2.v->flags&USEDASADR)&&!(p->q2.v->vtyp->flags&VOLATILE)&&(p->q2.v->storage_class==AUTO||p->q2.v->storage_class==REGISTER)){
                i=p->q2.v->index;
                if(BTST(lfg->av_kill,i)&&!BTST(lfg->av_out,i)){
                    t=p->q2.v->vtyp->flags;
                    if(USEQ2ASZ&&nr&&regok(nr,t,0)&&(!(p->q2.flags&DREFOBJ)||regok(nr,t,p->typf))) r=nr; else r=0;
                    if(p->q2.v->reg){ r=p->q2.v->reg;if(regu[r]) free_hreg(lfg,p,r,1);}
                    for(i=0;r==0&&i<=MAXR;i++){
                        if(!regu[i]&&!regsa[i]&&regok(i,t,0)&&(USEQ2ASZ||i!=nr)) {r=i;break;}
                    }
                    if(r){
                        if(r!=nr) insert_allocreg(lfg,p,FREEREG,r);
                            else mustalloc=0;
                        lregv[r]=p->q2.v;regused[r]=regu[r]=1;
                        if(replace_local_reg(&p->q2)!=r) ierror(0);
                        replace_local_reg(&p->q1);
                        replace_local_reg(&p->z);
                        if((DEBUG&9216)&&*p->q2.v->identifier) printf("temporary <%s> assigned to %s\n",p->q2.v->identifier,regnames[r]);
                        if(DEBUG&8192) printf("temporary <%s> assigned to %s\n",p->q2.v->identifier,regnames[r]);
                    }
                }
            }
            if((p->z.flags&(VAR|REG|DREFOBJ))==(VAR|DREFOBJ)&&!(p->z.v->flags&USEDASADR)&&!(p->z.v->vtyp->flags&VOLATILE)&&(p->z.v->storage_class==AUTO||p->z.v->storage_class==REGISTER)){
                i=p->z.v->index;
                if(BTST(lfg->av_kill,i)&&!BTST(lfg->av_out,i)){
                    r=0;
                    if(p->z.v->reg){ r=p->z.v->reg;if(regu[r]) free_hreg(lfg,p,r,1);}
                    for(i=0,t=p->z.v->vtyp->flags;i<=MAXR;i++){
                        if(!regu[i]&&!regsa[i]&&regok(i,t,0)) {r=i;break;}
                    }
                    if(r){
                        insert_allocreg(lfg,p,FREEREG,r);
                        lregv[r]=p->z.v;regused[r]=regu[r]=1;
                        if(replace_local_reg(&p->z)!=r){
                            for(i=1;i<=MAXR;i++) if(lregv[i]) printf("%d:%s=%s(%p)\n",i,regnames[i],lregv[i]->identifier,(void*)lregv[i]);
                            ierror(r);}
                        replace_local_reg(&p->q1);
                        if((DEBUG&9216)&&*p->z.v->identifier) printf("temporary <%s> assigned to %s\n",p->z.v->identifier,regnames[r]);
                        if(DEBUG&8192) printf("temporary <%s> assigned to %s\n",p->z.v->identifier,regnames[r]);
                    }
                }
            }
            if((p->q1.flags&(VAR|REG|VARADR))==VAR&&!(p->q1.v->flags&USEDASADR)&&!(p->q1.v->vtyp->flags&VOLATILE)&&(p->q1.v->storage_class==AUTO||p->q1.v->storage_class==REGISTER)){
                i=p->q1.v->index;
                if(BTST(lfg->av_kill,i)&&!BTST(lfg->av_out,i)){
                    t=p->q1.v->vtyp->flags;
                    if(nr&&regok(nr,t,0)&&(!(p->q1.flags&DREFOBJ)||regok(nr,t,p->typf))) r=nr; else r=0;
                    if(p->q1.v->reg){ r=p->q1.v->reg;if(regu[r]) free_hreg(lfg,p,r,1);}
                    for(i=0;r==0&&i<=MAXR;i++){
                        if(!regu[i]&&!regsa[i]&&regok(i,t,0)) {r=i;break;}
                    }
                    if(r){
                        if(r!=nr) insert_allocreg(lfg,p,FREEREG,r);
                            else mustalloc=0;
                        lregv[r]=p->q1.v;regused[r]=regu[r]=1;
                        if(replace_local_reg(&p->q1)!=r) ierror(0);
                        if((DEBUG&9216)&&*p->q1.v->identifier) printf("temporary <%s> assigned to %s\n",p->q1.v->identifier,regnames[r]);
                        if(DEBUG&8192) printf("temporary <%s> assigned to %s\n",p->q1.v->identifier,regnames[r]);
                    }
                }
            }
            if(p->code==CALL){
                lfg->calls++;
                /*  falls Scratchregister bei Funktionsaufruf benutzt   */
                /*  wird, moeglichst auf ein anderes ausweichen         */
                for(i=1;i<=MAXR;i++){
                    if(lregv[i]&&regscratch[i]&&!lregv[i]->reg)
                        free_hreg(lfg,p,i,0);
                }
            }
            /*  die Faelle beachten, wenn schon im IC ein Register          */
            /*  angesprochen wird (sollte nur bei CALL und return auftreten */
            if(p->code==FREEREG){
                ierror(0);
                if(regu[p->q1.reg]) remove_IC_fg(lfg,p);
                regu[p->q1.reg]++;
            }
            if(p->code==ALLOCREG){
                ierror(0);
                if(regu[p->q1.reg]==2) remove_IC_fg(lfg,p);
                regu[p->q1.reg]--;
            }
            if(p==lfg->start) i=1; else i=0;;
            p=p->prev;
            if(nr&&mustalloc) insert_allocreg(lfg,p,ALLOCREG,nr);
            if(i) break;
        }
        lfg=lfg->normalout;
    }
}
void insert_saves(void)
/*  fuegt speichern von Registern bei Funktionsaufrufen ein */
{
    int i,c;struct IC *p;
    if(DEBUG&9216) printf("insert_saves\n");
    for(i=1;i<=MAXR;i++) regs[i]=regsa[i];
    for(p=first_ic;p;p=p->next){
        c=p->code;
        if(c==ALLOCREG) regs[p->q1.reg]=1;
        if(c==FREEREG)  regs[p->q1.reg]=0;
        if(c==CALL){
            struct IC *s;
            /*  das Wiederherstellen nach dem GETRETURN     */
            s=p; i=0;
            if(s->next&&s->next->code==FREEREG) {s=s->next;regs[s->q1.reg]=0;}
            if(s->next&&s->next->code==GETRETURN){
                s=s->next;
                if((s->z.flags&(REG|DREFOBJ))==REG) i=s->z.reg;
            }
            if(s->next&&s->next->code==ALLOCREG&&s->next->next&&s->next->next->code==GETRETURN){
                s=s->next->next;
                if((s->z.flags&(REG|DREFOBJ))==REG) i=s->z.reg;
            }
            savescratch(MOVEFROMREG,p->prev,0);
            savescratch(MOVETOREG,s,i);
        }
    }
}

#endif

void insert_simple_allocreg(struct IC *p,int code,int reg)
/*  Fuegt ein ALLOCREG/FREEREG (in code) hinter p ein - bei p==0 in */
/*  first_ic.                                                       */
{
    struct IC *new=mymalloc(ICS);
    new->line=0;
    new->file=0;
    regused[reg]=1;
    new->code=code;
    new->typf=0;
    new->q1.am=new->q2.am=new->z.am=0;
    new->q1.flags=REG;
    new->q1.reg=reg;
    new->q2.flags=new->z.flags=0;
    new->use_cnt=new->change_cnt=0;
    new->use_list=new->change_list=0;
    insert_IC(p,new);
}

void load_simple_reg_parms(void)
/*  Laedt Registerparameter, falls noetig. Nicht-optimierende Version.  */
{
    int i; struct Var *v;
    for(i=0;i<=1;i++){
        if(i==0) v=merk_varf; else v=first_var[1];
        for(;v;v=v->next){
            if((v->flags&REGPARM)&&regsv[v->reg]!=v){
                struct IC *new; int j;
                insert_simple_allocreg(0,FREEREG,v->reg);
                new=mymalloc(ICS);
                new->line=0;
                new->file=0;
                new->code=ASSIGN;
                new->typf=v->vtyp->flags;
                new->q1.flags=REG;
                new->q1.reg=v->reg;
                new->q2.flags=0;
                new->q2.val.vlong=szof(v->vtyp);
                new->z.flags=VAR;
                new->z.val.vlong=l2zl(0L);
                new->z.v=v;
                for(j=1;j<=MAXR;j++)
                    if(regsv[j]==v){ new->z.flags|=REG;new->z.reg=j;break; }
                new->q1.am=new->q2.am=new->z.am=0;
                new->use_cnt=new->change_cnt=0;
                new->use_list=new->change_list=0;
                insert_IC(0,new);
                insert_simple_allocreg(0,ALLOCREG,v->reg);
                if(new->z.flags&REG){
                    /*  ALLOCREG verschieben    */
                    struct IC *p;
                    insert_simple_allocreg(0,ALLOCREG,new->z.reg);
                    for(p=new->next;p;p=p->next){
                        if(p->code==ALLOCREG&&p->q1.reg==new->z.reg){
                            remove_IC(p);
                            break;
                        }
                    }
                    if(!p) ierror(0);
                }
            }
        }
    }
}

void simple_regs(void)
/*  haelt Variablen in Registern, simple Version            */
{
    int i2,i,j;int pri;struct Var *v;
    struct IC *icp,*start=first_ic;
    if(!first_ic) return;
    for(i=1;i<=MAXR;i++) regsv[i]=0;
    for(i2=0;i2<=MAXR*4;i2++){
        int only_best,pointertype;
        if(i2<=MAXR*2){i=i2;only_best=1;} else {i=i2/2;pointertype=only_best=0;}
        if(i>MAXR||!regsv[i]){
            if(i>MAXR){
                i-=MAXR;
                if(regsv[i]) continue;
            }else{
                /*  Ziehe Scratchregister vor, wenn kein Funktionsaufruf */
                /*  erfolgt, sonst erst andere                           */
                if(!function_calls&&!regscratch[i]) continue;
                if(function_calls&&regscratch[i]) continue;
            }
            if(regused[i]) continue;
            /* Nicht-Scratchregister muessen einmal gesichert und wieder    */
            /* hergestellt werden, Scratchregister bei jedem Call           */
            if(regscratch[i]&&function_calls) continue;
            /*pri=2;*/ pri=0;
            for(j=0;j<=1;j++){
                if(j==0) v=merk_varf; else v=first_var[1];
                while(v){
                    if(v->storage_class==AUTO||v->storage_class==REGISTER){
                        if(!(v->flags&USEDASADR)&&!(v->vtyp->flags&VOLATILE)){
                            if(only_best&&v->vtyp->next) pointertype=v->vtyp->next->flags;
                            if(v->priority>pri&&regok(i,v->vtyp->flags&31,pointertype)){
                                regsv[i]=v;pri=v->priority;
                            }
                        }
                    }
                    v=v->next;
                }
            }
        }
        if(regsv[i]){
            if(DEBUG&1) printf("Assigned <%s> to %s\n",regsv[i]->identifier,regnames[i]);
            regsv[i]->priority=0;regused[i]=1;
            if(!zlleq(l2zl(0L),regsv[i]->offset)&&!(regsv[i]->flags&CONVPARAMETER)){
                icp=mymalloc(ICS);
                icp->line=0;
                icp->file=0;
                icp->q1.am=icp->q2.am=icp->z.am=0;
                icp->code=ASSIGN;
                icp->typf=regsv[i]->vtyp->flags&31;
                icp->q1.flags=VAR;
                icp->q1.v=regsv[i];
                icp->q1.val.vlong=l2zl(0L);
                icp->q2.flags=0;
                icp->q2.val.vlong=szof(regsv[i]->vtyp);
                icp->z.flags=REG;
                icp->z.reg=i;
                icp->next=first_ic;
                icp->prev=0;
                first_ic->prev=icp;
                first_ic=icp;
            }
            icp=mymalloc(ICS);
            icp->line=0;
            icp->file=0;
            icp->q1.am=icp->q2.am=icp->z.am=0;
            icp->code=ALLOCREG;
            icp->q1.flags=REG;
            icp->q1.reg=i;
            icp->q2.flags=icp->z.flags=icp->typf=0;
            icp->next=first_ic;
            icp->prev=0;
            first_ic->prev=icp;
            first_ic=icp;
            icp=mymalloc(ICS);
            icp->q1.am=icp->q2.am=icp->z.am=0;
            icp->code=FREEREG;
            icp->q1.flags=REG;
            icp->q1.reg=i;
            icp->q2.flags=icp->z.flags=icp->typf=0;
            icp->next=0;
            add_IC(icp);
        }
    }
    icp=start;
    while(icp){
        if((icp->code==ALLOCREG||icp->code==FREEREG)&&regsv[icp->q1.reg]){
        /*  irgendwelche allocreg/freereg im Code entfernen     */
        /*  sollte nur beim Returnregister vorkommen            */
            struct IC *m=icp->next;
            remove_IC(icp);
            icp=m;continue;
        }
        for(i=1;i<=MAXR;i++){
            if(!regsv[i]) continue;
            if((icp->q1.flags&(VAR|DONTREGISTERIZE))==VAR&&icp->q1.v==regsv[i]){
                icp->q1.flags|=REG;
                icp->q1.reg=i;
            }
            if((icp->q2.flags&(VAR|DONTREGISTERIZE))==VAR&&icp->q2.v==regsv[i]){
                icp->q2.flags|=REG;
                icp->q2.reg=i;
            }
            if((icp->z.flags&(VAR|DONTREGISTERIZE))==VAR&&icp->z.v==regsv[i]){
                icp->z.flags|=REG;
                icp->z.reg=i;
            }
        }
        icp=icp->next;
    }
}


