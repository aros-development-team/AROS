/*  $VER: vbcc (cp.c) V0.4     */
/*  verfuegbare Kopien und copy propagation */

#include "opt.h"

static char FILE_[]=__FILE__;

/*  fuer verfuegbare Kopien */
unsigned int ccount;
size_t csize;
struct IC **clist;

/*  alle Assignments, globaler oder Adr. fuer propagation etc.         */
unsigned char *cp_globals,*cp_address,*cp_statics,*cp_drefs,*cp_act,*cp_dest;
/*  alle Kopieranweisungen, die eine best. Variable als Quelle haben    */
unsigned char **copies;

void available_copies(struct flowgraph *fg)
/*  berechnet die verfuegbaren Kopien fuer jeden Block      */
{
    struct flowgraph *g;struct IC *p;unsigned char *tmp;
    int changed,pass,i,j;
    /*  cp_gen und cp_kill fuer jeden Block berechnen   */
    if(DEBUG&1024) printf("analysing available copies\n");
    tmp=mymalloc(csize);
    for(g=fg;g;g=g->normalout){
        g->cp_in=mymalloc(csize);
        memset(g->cp_in,0,csize);
        g->cp_out=mymalloc(csize);
        memset(g->cp_out,0,csize);
        g->cp_gen=mymalloc(csize);
        memset(g->cp_gen,0,csize);
        g->cp_kill=mymalloc(csize);
        memset(g->cp_kill,0,csize);
        for(p=g->end;p;p=p->prev){
            memset(tmp,0,csize);
            for(j=0;j<p->change_cnt;j++){
                i=p->change_list[j].v->index;
                if(p->change_list[j].flags&DREFOBJ) i+=vcount-rcount;
                if(i>=vcount) continue;
                bvunite(tmp,copies[i],csize);
            }
            i=p->copyindex;
            if(i>=0&&!BTST(g->cp_kill,i)) BSET(g->cp_gen,i);
            bvdiff(tmp,g->cp_gen,csize);
            bvunite(g->cp_kill,tmp,csize);

            if(p==g->start) break;
        }
        if(g==fg){
            memset(g->cp_in,0,csize);
            memcpy(g->cp_out,g->cp_gen,csize);
        }else{
            memset(g->cp_out,UCHAR_MAX,csize);
            bvdiff(g->cp_out,g->cp_kill,csize);
        }
    }
    /*  cp_in und cp_out fuer jeden Block berechnen */
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
                        bvintersect(g->cp_in,lp->graph->cp_out,csize);
                    }else{
                        memcpy(g->cp_in,lp->graph->cp_out,csize);i=1;
                    }
                }
                lp=lp->next;
            }
            /*  out(b)=gen(B) U (in(B)-kill(B)  */
            memcpy(tmp,g->cp_in,csize);
            bvdiff(tmp,g->cp_kill,csize);
            bvunite(tmp,g->cp_gen,csize);
            if(!bvcmp(tmp,g->cp_out,csize)){changed=1;memcpy(g->cp_out,tmp,csize);}
            g=g->normalout;
        }
    }while(changed);
    if(DEBUG&1024) printf("\n");
    free(tmp);
}

int compare_cp(const void *a1,const void *a2)
/*  Stub fuer compare_objs, damit als Vergleichsfunktion fuer qsort geht */
{
    struct IC *p1,*p2;int i1,i2;
    p1=*((struct IC **)a1);p2=*((struct IC **)a2);
    if(!p1||!p2) ierror(0);
    i1=p1->typf; i2=p2->typf;
    if(i1<i2) return(-1);
    if(i1>i2) return(1);
    i1=compare_objs(&p1->q1,&p2->q1,p1->typf);
    if(i1) return(i1);
    i1=compare_objs(&p1->z,&p2->z,p1->typf);
    return(i1);
}

void num_copies(void)
/*  numeriert alle einfachen Kopieranweisungen  */
{
    struct IC *p;struct Var *v;int i,n,c;
    if(DEBUG&1024) printf("numerating copies loop1\n");
    ccount=0;
    for(p=first_ic;p;p=p->next){
        if(p->code==ASSIGN&&(p->q1.flags&(VAR/*|VARADR*/))==VAR) p->copyindex=ccount++;
            else            p->copyindex=-1;
    }
    csize=(ccount+CHAR_BIT-1)/CHAR_BIT;
    if(DEBUG&1024) printf("ccount=%lu, csize=%lu\n",(unsigned long)ccount,(unsigned long)csize);
    clist=mymalloc(ccount*sizeof(struct IC *));
    cp_globals=mymalloc(csize);
    memset(cp_globals,0,csize);
    cp_statics=mymalloc(csize);
    memset(cp_statics,0,csize);
    cp_address=mymalloc(csize);
    memset(cp_address,0,csize);
    cp_drefs=mymalloc(csize);
    memset(cp_drefs,0,csize);
    copies=mymalloc(vcount*sizeof(unsigned char *));
    if(DEBUG&1024){ printf("num_copies loop2\n");}
    for(p=first_ic;p;p=p->next){
        if(p->copyindex>=0){
            clist[p->copyindex]=p;
        }
    }
    if(DEBUG&1024){ printf("sorting copies\n");}
    if(ccount>1) qsort(clist,ccount,sizeof(struct IC *),compare_cp);
    if(DEBUG&1024){ printf("renumbering copies\nnum_copies loop3\n");}
    if(ccount>0){   /*  Aufpassen, da ccount unsigned!  */
        for(c=0;c<ccount-1;c++){
            if(!compare_cp(&clist[c],&clist[c+1]))
                clist[c+1]->copyindex=clist[c]->copyindex;
        }
    }
    if(DEBUG&1024) printf("re-sorting copies\n");
    /*  wieder in die richtige Reihenfolge bringen  */
    for(p=first_ic;p;p=p->next)
        if(p->copyindex>=0) clist[p->copyindex]=p;

    for(i=0;i<vcount;i++){
        copies[i]=mymalloc(csize);
        memset(copies[i],0,csize);
    }

    if(DEBUG&1024) printf("numerating copies loop4\n");
    for(p=first_ic;p;p=p->next){
        i=p->copyindex;
        if(i>=0){
/*            clist[i]=p;*/
            v=p->z.v;
            n=v->index;
            if(p->z.flags&DREFOBJ) n+=vcount-rcount;
            if(n<0||n>=vcount) ierror(0);
            BSET(copies[n],i);
            if(v->nesting==0||v->storage_class==EXTERN) BSET(cp_globals,i);
            if(p->z.flags&DREFOBJ) BSET(cp_drefs,i);
            if(v->storage_class==STATIC) BSET(cp_statics,i);
            if(v->flags&USEDASADR) BSET(cp_address,i);
            v=p->q1.v;
            n=v->index;
            if(p->q1.flags&DREFOBJ) n+=vcount-rcount;
            if(n<0||n>=vcount){pric2(stdout,p);printf("n=%d\n",n); ierror(0);}
            BSET(copies[n],i);
            if(v->nesting==0||v->storage_class==EXTERN) BSET(cp_globals,i);
            if(p->q1.flags&DREFOBJ) BSET(cp_drefs,i);
            if(v->storage_class==STATIC) BSET(cp_statics,i);
            if(v->flags&USEDASADR) BSET(cp_address,i);

        }
    }
    if(DEBUG&2048){
        printf("copy instructions:\n");
        for(i=0;i<ccount;i++){
            printf("%3d: ",i);pric2(stdout,clist[i]);
            if(clist[i]->copyindex!=i) ierror(0);
        }
    }
}
void print_cp(unsigned char *cp)
{
    int i;
    if(!cp) printf("available copies not available\n");
    for(i=0;i<ccount;i++)
        if(BTST(cp,i)){printf("%3d: ",i);pric2(stdout,clist[i]);}
}
int cprop(struct obj *o,int target,zlong size)
/*  ersetzt gegebenenfalls  Kopien, noch aendern, so dass Pointer in DREFOBJS ersetzt werden wie bei target */
{
  struct IC *p,*f=0;int i;struct Var *old;
  old=o->v;
  i=old->index;
  if(!target&&(o->flags&DREFOBJ)) i+=vcount-rcount;
  if(i<0||i>=vcount) ierror(0);
  memcpy(tmp,cp_act,csize);
  bvintersect(tmp,copies[i],csize);
  /*  waehrend diesem Durchlauf geaenderte Kopieranweisungen lieber nicht */
  /*  beachten                                                            */
  bvdiff(tmp,cp_dest,csize);
  for(i=0;i<ccount;i++){
    if(BTST(tmp,i)){
      p=clist[i];
      if(p->z.v==o->v
	 &&(zleqto(size,l2zl(0L))||zleqto(size,p->q2.val.vlong))
	 &&p->q1.v!=o->v&&zleqto(p->z.val.vlong,o->val.vlong)
	 &&((o->v->vtyp->flags&NQ)<=POINTER||(p->typf&NQ)==(o->v->vtyp->flags&NQ))
	 ){
	if(((o->flags&DREFOBJ)&&!(p->q1.flags&DREFOBJ))||!(p->q1.flags&DREFOBJ)){
	  if(DEBUG&1024){ printf("can replace <%s> by copy:\n",o->v->identifier);pric2(stdout,clist[i]);}
	  *o=p->q1;
	  if(target){
	    o->flags|=DREFOBJ;
	    if((o->flags&(VARADR|DREFOBJ))==(VARADR|DREFOBJ))
	      o->flags&=~(VARADR|DREFOBJ);
	    /*  Wenn eine Variable, dadurch zu einer DREF-Variable  */
	    /*  wird, muss num_vars spaeter erneut gemacht werden   */
	    if(o->v->index>=rcount){
	      update_alias(old,o->v);
	      return(2);
	    }
	  }
	  return(1);
	}
      }
    }
  }
  return(0);
}
int copy_propagation(struct flowgraph *fg,int global)
/*  gibt Kopien weiter  */
{
    struct flowgraph *g; struct obj old;int or;
    struct IC *p;struct Var *v;int i,changed,r,j;
    if(DEBUG&1024) printf("searching copies\n");
    cp_act=mymalloc(csize);
    cp_dest=mymalloc(csize);
    memset(cp_dest,0,csize);
    tmp=mymalloc(csize);
    g=fg;changed=0;
    while(g){
        if(!global) memset(cp_act,0,csize); else memcpy(cp_act,g->cp_in,csize);
        p=g->start;
        while(p){
            zlong size;
            if(p->code==ASSIGN||p->code==PUSH) size=p->q2.val.vlong;
                else size=l2zl(0L);
/*            print_cp(cp_act); pric2(stdout,p);*/
            r=0;
            if(p->code!=ADDRESS&&p->code!=NOP){
                if((p->q1.flags&(VAR|VARADR))==VAR){
                    r|=cprop(&p->q1,0,size);
                    if(p->q1.flags&DREFOBJ) r|=cprop(&p->q1,1,0);
                }
                if((p->q2.flags&(VAR|VARADR))==VAR){
                /*  passt auf, dass USEQ2ASZ nicht verletzt wird, ist dabei */
                /*  aber nicht sehr effizient (evtl. koennte kommutiert     */
                /*  werden, o.ae.)                                          */
                    if(!USEQ2ASZ){ old=p->q2;or=r;}
                    r|=cprop(&p->q2,0,size);
                    if(p->q2.flags&DREFOBJ) r|=cprop(&p->q2,1,0);
                    if(!USEQ2ASZ&&!compare_objs(&p->q2,&p->z,p->typf)){
                        if(DEBUG&1024) printf("copy propagation taken back, because of USEQ2ASZ\n");
                        p->q2=old;
                        r=or;
                    }
                }
                if((p->z.flags&(VAR|VARADR|DREFOBJ))==(VAR|DREFOBJ)){
                /*  passt auf, dass USEQ2ASZ nicht verletzt wird, ist dabei */
                /*  aber nicht sehr effizient (evtl. koennte kommutiert     */
                /*  werden, o.ae.                                           */
                    if(!USEQ2ASZ){ old=p->z;or=r;}
                    r|=cprop(&p->z,1,0);
                    if(!USEQ2ASZ&&!compare_objs(&p->q2,&p->z,p->typf)){
                        if(DEBUG&1024) printf("copy propagation taken back, because of USEQ2ASZ\n");
                        p->z=old;
                        r=or;
                    }
                }
            }
            if(r&&p->copyindex>=0) BSET(cp_dest,p->copyindex);
            changed|=r;

            for(j=0;j<p->change_cnt;j++){
                i=p->change_list[j].v->index;
                if(p->change_list[j].flags&DREFOBJ) i+=vcount-rcount;
                if(i>=vcount) continue;
                bvdiff(cp_act,copies[i],csize);
            }
            if(p->copyindex>=0) BSET(cp_act,p->copyindex);

            if(p==g->end) break;
            p=p->next;
        }
        g=g->normalout;
    }
    free(cp_act);
    free(cp_dest);
    free(tmp);
    free(clist);
    free(cp_globals);
    free(cp_statics);
    free(cp_address);
    free(cp_drefs);
    for(i=0;i<vcount;i++) free(copies[i]);
    free(copies);
    gchanged|=changed;
    return(changed);
}

