/*  $VER: vbcc (ic.c) V0.4  */

#include "vbc.h"

static char FILE_[]=__FILE__;


int do_arith(np,struct IC *,np,struct obj *);

void gen_test(struct obj *o,int t,int branch,int label)
/*  Generiert ein test o, branch label und passt auf, dass      */
/*  kein TEST const generiert wird.                             */
{
    struct IC *new;
    if(o->flags&KONST){
        eval_const(&o->val,t);
        if(zdeqto(vdouble,d2zd(0.0))&&zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))){
            if(branch==BEQ) branch=BRA; else branch=0;
        }else{
            if(branch==BNE) branch=BRA; else branch=0;
        }
    }else{
        new=mymalloc(ICS);
        new->code=TEST;
        new->q2.flags=new->z.flags=0;
        new->typf=t;
        new->q1=*o;
        add_IC(new);
    }
    if(branch){
        new=mymalloc(ICS);
        new->code=branch;
        new->typf=label;
        add_IC(new);
    }
}

void inline_memcpy(np z,np q,zlong size)
/*  fuegt ein ASSIGN-IC ein, das memcpy(z,q,size) entspricht    */
{
    struct IC *new=mymalloc(ICS);
    if((z->ntyp->flags&NQ)!=POINTER) ierror(0);
    if((q->ntyp->flags&NQ)!=POINTER) ierror(0);

    gen_IC(z,0,0);
    if(z->o.flags&DREFOBJ){
        struct IC *n2=mymalloc(ICS);
        n2->code=ASSIGN;
        n2->typf=q->ntyp->flags;
        n2->q1=z->o;
        get_scratch(&n2->z,z->ntyp->flags,z->ntyp->next->flags,z->ntyp);
        n2->q2.flags=0;
        n2->q2.val.vlong=sizetab[POINTER];
        new->z=n2->z;
        add_IC(n2);
    }else{
        new->z=z->o;
    }
    if(new->z.flags&VARADR) new->z.flags&=~VARADR; else new->z.flags|=DREFOBJ;

    gen_IC(q,0,0);
    if(q->o.flags&DREFOBJ){
        struct IC *n2=mymalloc(ICS);
        n2->code=ASSIGN;
        n2->typf=q->ntyp->flags;
        n2->q1=q->o;
        get_scratch(&n2->z,q->ntyp->flags,q->ntyp->next->flags,q->ntyp);
        n2->q2.flags=0;
        n2->q2.val.vlong=sizetab[POINTER];
        new->q1=n2->z;
        add_IC(n2);
    }else{
        new->q1=q->o;
    }
    if(new->q1.flags&VARADR) new->q1.flags&=~VARADR; else new->q1.flags|=DREFOBJ;

    new->code=ASSIGN;
    new->typf=UNSIGNED|CHAR;
    new->q2.flags=0;
    new->q2.val.vlong=size;
    add_IC(new);
}

void add_IC(struct IC *new)
/*  fuegt ein IC ein                                            */
{
    int code;
    if(!new) return;
    if(nocode) {free(new);return;}
    new->next=0;
    new->q1.am=new->q2.am=new->z.am=0;
    new->line=line; new->file=0;
    code=new->code;
    if(code>=BEQ&&code<=BRA) new->q1.flags=new->q2.flags=new->z.flags=0;
    if(code==ALLOCREG||code==FREEREG||code==SAVEREGS||code==RESTOREREGS) new->typf=0;
    if(DEBUG&64) pric2(stdout,new);
    if(new->q1.flags&VAR){
        if(!new->q1.v) ierror(0);
        new->q1.v->flags|=USEDASSOURCE;
        if(code==ADDRESS||(new->q1.flags&VARADR)) new->q1.v->flags|=USEDASADR;
        new->q1.v->priority+=currentpri;
    }
    if(new->q2.flags&VAR){
        if(!new->q2.v) ierror(0);
        new->q2.v->flags|=USEDASSOURCE;
        if(code==ADDRESS||(new->q2.flags&VARADR)) new->q2.v->flags|=USEDASADR;
        new->q2.v->priority+=currentpri;
    }
    if(new->z.flags&VAR){
        if(!new->z.v) ierror(0);
        if(new->z.flags&DREFOBJ) new->z.v->flags|=USEDASSOURCE; else new->z.v->flags|=USEDASDEST;
        new->z.v->priority+=currentpri;
    }
    if(/*(c_flags_val[0].l&2)&&*/code==LABEL){
    /*  entfernt Spruenge zu direkt folgenden Labels    */
        struct IC *p=last_ic;
        while(p){
            if(p->typf==new->typf&&p->code>=BEQ&&p->code<=BRA){
                struct IC *n;
                if(DEBUG&1) printf("%s l%d deleted\n",ename[p->code],p->typf);
                n=p->next;
                remove_IC(p);
                p=n;
            }else{
                if(p->code!=LABEL) break;
                p=p->prev;
            }
        }
    }
    if(last_ic){
        if(code==ASSIGN){
            if((last_ic->z.flags&(REG|SCRATCH|DREFOBJ))==(REG|SCRATCH)&&(new->q1.flags==last_ic->z.flags)&&last_ic->z.reg==new->q1.reg/*&&last_ic->code!=CALL*/){
                if(USEQ2ASZ||!(last_ic->q2.flags&REG)||!(new->z.flags&REG)||last_ic->q2.reg!=new->z.reg){
                    if(USEQ2ASZ||!(last_ic->q2.flags&VAR)||!(new->z.flags&VAR)||last_ic->q2.v!=new->z.v){
                    /*  verbindet op a,b->reg,move reg->c zu op a,b->c  */
                    /*  hier fehlt aber noch Registerfreigabe           */
                        last_ic->z=new->z;
                        if(DEBUG&1) printf("move and op combined\n");
                        if((new->q1.flags&SCRATCH)&&(new->q1.reg!=new->z.reg||!(new->z.flags&REG)))
                            free_reg(new->q1.reg);
                        free(new);
                        return;
                    }
                }
            }
            if((last_ic->z.flags&(VAR|SCRATCH|DREFOBJ))==(VAR|SCRATCH)&&(new->q1.flags==last_ic->z.flags)&&last_ic->z.v==new->q1.v/*&&last_ic->code!=CALL*/){
                if(USEQ2ASZ||!(last_ic->q2.flags&REG)||!(new->z.flags&REG)||last_ic->q2.reg!=new->z.reg){
                    if(USEQ2ASZ||!(last_ic->q2.flags&VAR)||!(new->z.flags&VAR)||last_ic->q2.v!=new->z.v){
                    /*  verbindet op a,b->scratch,move scratch->c zu op a,b->c  */
                    /*  hier fehlt aber noch Registerfreigabe           */
                        last_ic->z=new->z;
                        if(DEBUG&1) printf("move and op combined(2)\n");
/*                        if((new->q1.flags&SCRATCH)&&(new->q1.reg!=new->z.reg||!(new->z.flags&REG)))
                            free_reg(new->q1.reg);*/
                        free(new);
                        return;
                    }
                }
            }

        }
        if(last_ic->code==BRA){
            if(code!=LABEL&&code!=ALLOCREG&&code!=FREEREG){
            /*  loescht alles nach bra bis ein Label kommt  */
            /*  momentan noch nicht perfekt, da es bei alloc/freereg stoppt */
                free(new);
                if(DEBUG&1) printf("Unreachable Statement deleted\n");
                return;
            }
            if(last_ic->prev&&code==LABEL){
            /*  ersetzt bcc l1;bra l2;l1 durch b!cc l2      */
                if(last_ic->prev->code>=BEQ&&last_ic->prev->code<=BGT&&new->typf==last_ic->prev->typf){
                    if(DEBUG&1) printf("%s l%d;%s l%d; substituted\n",ename[last_ic->prev->code],last_ic->prev->typf,ename[last_ic->code],last_ic->typf);
                    if(last_ic->prev->code&1) last_ic->prev->code--;
                                    else      last_ic->prev->code++;
                    last_ic->prev->typf=last_ic->typf;
                    last_ic=last_ic->prev;
                    free(last_ic->next);
                    last_ic->next=new;new->prev=last_ic;
                    last_ic=new;
                    return;
                }
            }
        }
/*        }*/
        new->prev=last_ic;
        last_ic->next=new;
        last_ic=new;
    }else{
        last_ic=new;first_ic=new;new->prev=0;
    }
    ic_count++;
    /*  Merken, on Fliesskomma benutzt wurde    */
    if(code!=LABEL&&(code<BEQ||code>BRA)){
        if((new->typf&NQ)==FLOAT||(new->typf&NQ)==DOUBLE) float_used=1;
	if(code==CONVFLOAT||code==CONVDOUBLE) float_used=1;
    }
    if((new->q1.flags&SCRATCH)&&(new->q1.reg!=new->z.reg||!(new->z.flags&REG)))
        free_reg(new->q1.reg);
    if((new->q2.flags&SCRATCH)&&(new->q2.reg!=new->z.reg||!(new->z.flags&REG)))
        free_reg(new->q2.reg);
}
void gen_IC(np p,int ltrue,int lfalse)
/*  Erzeugt eine IC-Liste aus einer expression      */
{
    struct IC *new; struct regargs_list *rl;
    if(!p) return;
    if(p->flags==STRING){
    /*  hier fehlt noch die Verwaltung der String-Inhalte   */
        p->o.v=add_var(empty,clone_typ(p->ntyp),STATIC,(struct const_list *)p->identifier);
        p->o.v->flags|=DEFINED;
        p->o.flags=VAR;
        p->o.reg=0;
        p->o.val=p->val;
        return;
    }
    if(p->flags==IDENTIFIER){
/*        p->o.v=find_var(p->identifier,0);*/
        p->o.flags=VAR;
        p->o.reg=0;
        p->o.val=p->val;
        return;
    }
    if(p->flags==CEXPR||p->flags==PCEXPR){
        if(p->left){
            if(p->left->flags==POSTINC) p->left->flags=PREINC;
            if(p->left->flags==POSTDEC) p->left->flags=PREDEC;
            gen_IC(p->left,0,0);
            if(p->left->o.flags&SCRATCH) free_reg(p->left->o.reg);
        }
        if(p->right){
            if(p->right->flags==POSTINC) p->right->flags=PREINC;
            if(p->right->flags==POSTDEC) p->right->flags=PREDEC;
            gen_IC(p->right,0,0);
            if(p->right->o.flags&SCRATCH) free_reg(p->right->o.reg);
        }
        p->o.flags=KONST;
        p->o.val=p->val;
        p->o.reg=0;
        return;
    }
    if(p->flags==KOMMA){
        if(p->left->sidefx){
            gen_IC(p->left,0,0);
            if(p->left->o.flags&SCRATCH) free_reg(p->left->o.reg);
        } else error(129);
        gen_IC(p->right,0,0);
        p->o=p->right->o;
        return;
    }
    if(p->flags==CAST){
        gen_IC(p->left,0,0);
        if((p->ntyp->flags&NQ)==VOID){
            if(p->left->o.flags&SCRATCH) free_reg(p->left->o.reg);
            p->o.flags=0;
        }else{
            convert(p->left,p->ntyp->flags&NU);
            p->o=p->left->o;
        }
        return;
    }
    if(p->flags==FIRSTELEMENT){
        gen_IC(p->left,0,0);
        p->o=p->left->o;
        return;
    }
    new=mymalloc(ICS);
    new->typf=p->ntyp->flags&NU;
    new->q1.reg=new->q2.reg=new->z.reg=0;
    new->q1.flags=new->q2.flags=new->z.flags=0;
    if((p->flags>=LSHIFT&&p->flags<=MOD)||(p->flags>=OR&&p->flags<=AND)||p->flags==PMULT){
        do_arith(p,new,0,0);
        return;
    }
    if(p->flags==CONTENT){
        gen_IC(p->left,0,0);
        if(p->left->o.flags&VARADR){
            free(new);
            p->o=p->left->o;
            p->o.flags&=~VARADR;
            return;
        }
        if(!(p->left->o.flags&DREFOBJ)){
            free(new);
            p->o=p->left->o;
            p->o.flags|=DREFOBJ;
            return;
        }
        if(p->left->o.flags&SCRATCH){
            new->z=p->left->o;
            new->z.flags&=~DREFOBJ;
        }else{
            /*  hier muss man noch was aender, da das so nicht funktioniert */
            get_scratch(&new->z,p->left->ntyp->flags,p->ntyp->flags,p->left->ntyp);
        }
        new->code=ASSIGN;new->typf=POINTER;
        new->q1=p->left->o;
        new->q2.val.vlong=sizetab[POINTER];
        p->o=new->z;
        add_IC(new);
        p->o.flags|=DREFOBJ;
        return;
    }
    if(p->flags==ASSIGN){
        new->code=ASSIGN;
        gen_IC(p->right,0,0);
        gen_IC(p->left,0,0);
        convert(p->right,p->ntyp->flags&NU);
        new->q1=p->right->o;
        new->z=p->left->o;
        new->q2.val.vlong=szof(p->left->ntyp);
        p->o=new->z;
        add_IC(new);
        return;
    }
    if(p->flags==ASSIGNADD){
    /*  das hier ist nicht besonders schoen */
        struct obj o;struct IC *n;int f;
        if(p->right->right==0){
        /*  sowas wie a+=0 wurde wegoptimiert   */
            free(new);
            p->o=p->left->o;
            return;
        }
        f=do_arith(p->right,new,p->left,&o);
        if(!f) {ierror(0);return;}
        if(f>1) {ierror(0);return;}
        if(!nocode&&(o.flags&(SCRATCH|REG))==(SCRATCH|REG)&&!regs[o.reg]){
        /*  ueberfluessiges FREEREG entfernen   */
            n=last_ic;
            while(n){
                if(n->code==FREEREG&&n->q1.reg==o.reg){
                    remove_IC(n);if(!nocode) regs[o.reg]=1;
                    break;
                }
                n=n->prev;
            }
        }
        convert(p->right,p->ntyp->flags&NU);
        new=mymalloc(ICS);
        new->typf=p->ntyp->flags&NU;
        new->q2.flags=0;
        new->code=ASSIGN;
        new->q1=p->right->o;
        new->z=o;
        new->q2.val.vlong=szof(p->left->ntyp);
        p->o=new->z;
        add_IC(new);
        return;
    }
    if(p->flags==MINUS||p->flags==KOMPLEMENT){
        new->code=p->flags;
        gen_IC(p->left,0,0);
        convert(p->left,p->ntyp->flags);
        if(regok(p->left->o.reg,p->ntyp->flags,0)&&(p->left->o.flags&SCRATCH)){
            new->z=p->left->o;
            new->z.flags&=~DREFOBJ;
        }else{
            get_scratch(&new->z,p->left->ntyp->flags,0,p->left->ntyp);
        }
        new->q1=p->left->o;
        p->o=new->z;
        add_IC(new);
        return;
    }
    if(p->flags==ADDRESS||p->flags==ADDRESSA||p->flags==ADDRESSS){
        new->code=ADDRESS;
        new->typf=p->left->ntyp->flags&NU;
        gen_IC(p->left,0,0);
        if(p->left->o.flags&VAR) p->left->o.v->flags|=USEDASSOURCE|USEDASDEST;
        if(p->left->o.flags&DREFOBJ){
            free(new);
            p->o=p->left->o;
            p->o.flags&=~DREFOBJ;
            return;
        }
        if((p->left->o.flags&VAR)&&!(p->left->o.flags&VARADR)
           &&(p->left->o.v->storage_class==EXTERN||p->left->o.v->storage_class==STATIC)){
            free(new);
            p->o=p->left->o;
            p->o.flags|=VARADR;
            return;
        }
        new->q1=p->left->o;
        get_scratch(&new->z,POINTER,p->ntyp->next->flags,0);
        p->o=new->z;
        add_IC(new);
        return;
    }
    if(p->flags==LAND||p->flags==LOR){
        int l1,l2,l3,l4;
/*        printf("%s true=%d false=%d\n",ename[p->flags],ltrue,lfalse);*/
        l1=++label;if(!ltrue) {l2=++label;l3=++label;l4=++label;}
        if(!ltrue){if(p->flags==LAND) gen_IC(p->left,l1,l3);
                    else gen_IC(p->left,l3,l1);
        }else     {if(p->flags==LAND) gen_IC(p->left,l1,lfalse);
                    else gen_IC(p->left,ltrue,l1);
        }
        if(p->left->o.flags!=0){
            if(p->flags==LAND)
                gen_test(&p->left->o,p->left->ntyp->flags,BEQ,((!ltrue)?l3:lfalse));
            else
                gen_test(&p->left->o,p->left->ntyp->flags,BNE,((!ltrue)?l3:ltrue));
        }
        gen_label(l1);
        if(!ltrue){if(p->flags==LAND) gen_IC(p->right,l2,l3);
                    else gen_IC(p->right,l3,l2);
        }else      gen_IC(p->right,ltrue,lfalse);
        if(p->right->o.flags!=0){
            if(p->flags==LAND)
                gen_test(&p->right->o,p->right->ntyp->flags,BEQ,((!ltrue)?l3:lfalse));
            else
                gen_test(&p->right->o,p->right->ntyp->flags,BNE,((!ltrue)?l3:ltrue));
        }
        if(!ltrue){
            gen_label(l2);
            if(p->flags==LAND) p->o=gen_cond(0,l3,l4); else p->o=gen_cond(1,l3,l4);
        }else{
            new=mymalloc(ICS);
            new->code=BRA;
            if(p->flags==LAND) new->typf=ltrue; else new->typf=lfalse;
            add_IC(new);
        }
        if(ltrue) p->o.flags=0;
        return;
    }
    if(p->flags==NEGATION){
        int l1,l2,l3;
        if(!ltrue) {l1=++label;l2=++label;l3=++label;}
        if(ltrue) gen_IC(p->left,lfalse,ltrue); else gen_IC(p->left,l1,l3);
        if(!p->left->o.flags) {free(new);p->o.flags=0;
        }else{
            gen_test(&p->left->o,p->left->ntyp->flags,BNE,((!ltrue)?l1:lfalse));
        }
        if(ltrue){
            new=mymalloc(ICS);
            new->code=BRA;
            if(!ltrue) new->typf=l2; else new->typf=ltrue;
            add_IC(new);
            p->o.flags=0;
        }else{
            gen_label(l3);
            p->o=gen_cond(0,l1,l2);
        }
        return;
    }
    if(p->flags>=EQUAL&&p->flags<=GREATEREQ){
        int l1,l2,l3,tl,tr,swapped;
        if(!ltrue) {l1=++label;l2=++label;l3=++label;}
        if(p->left->flags==CEXPR){
        /*  Konstanten nach rechts  */
            np merk;merk=p->left;p->left=p->right;p->right=merk;
            swapped=1;
        }else swapped=0;
        new->code=COMPARE;
        tl=p->left->ntyp->flags&NU;tr=p->right->ntyp->flags&NU;
        if(p->right->flags==CEXPR&&(tr&NQ)<FLOAT&&(tl&NQ)<FLOAT){
            int negativ;
            eval_constn(p->right);
            if(zlleq(vlong,0)) negativ=1; else negativ=0;
            if((tl&UNSIGNED)||(tr&UNSIGNED)) negativ=0;
            if((!negativ||zlleq(t_min[tl],vlong))&&(negativ||zulleq(vulong,t_max[tl]))){
                convert(p->right,tl);
                tr=tl;
            }
        }
        if(arith(tl&NQ)&&(tl!=tr||!shortcut(COMPARE,tl))){
            struct Typ *t;
            t=arith_typ(p->left->ntyp,p->right->ntyp);
            new->typf=t->flags&NU;
            freetyp(t);
        }else{
            new->typf=p->left->ntyp->flags&NU;
        }
        gen_IC(p->left,0,0);
        convert(p->left,new->typf);
        gen_IC(p->right,0,0);
        convert(p->right,new->typf);
        new->q1=p->left->o;
        new->q2=p->right->o;
        new->z.flags=0;
        add_IC(new);
        new=mymalloc(ICS);
        if(p->flags==EQUAL) new->code=BEQ;
        if(p->flags==INEQUAL) new->code=BNE;
        if(p->flags==LESS) {if(swapped) new->code =BGT; else new->code=BLT;}
        if(p->flags==LESSEQ){if(swapped) new->code=BGE; else new->code=BLE;}
        if(p->flags==GREATER){if(swapped) new->code=BLT; else new->code=BGT;}
        if(p->flags==GREATEREQ){if(swapped) new->code=BLE; else new->code=BGE;}
        if(ltrue) new->typf=ltrue; else new->typf=l1;
        add_IC(new);
        if(ltrue){
            new=mymalloc(ICS);
            new->code=BRA;
            new->typf=lfalse;
            add_IC(new);
            p->o.flags=0;
        }else{
            gen_label(l3);
            p->o=gen_cond(1,l1,l2);
        }
        return;
    }
    if(p->flags==CALL){
        int r=0;struct obj *op;zlong sz;
        if(p->left->flags==ADDRESS&&p->left->left->flags==IDENTIFIER){
            struct Var *v;
            gen_IC(p->left,0,0); r=1;
            v=p->left->o.v;
            if(v->fi&&v->fi->first_ic&&(c_flags_val[0].l&4096)){
            /*  function call inlining  */
                struct argument_list *al;
                struct Var *vp,**argl1,**argl2;
                struct IC *ip;int lc;
                int arg_cnt=0,i;
                if(DEBUG&1024){
                    printf("inlining call to <%s>\n",v->identifier);
                    for(vp=v->fi->vars;vp;vp=vp->next)
                        printf("%s(%ld)\n",vp->identifier,zl2l(vp->offset));
                }
                for(vp=v->fi->vars;vp;vp=vp->next){
                    if((zleqto(vp->offset,l2zl(0L))||vp->reg)&&*vp->identifier&&(vp->storage_class==AUTO||vp->storage_class==REGISTER)) arg_cnt++;
                }

                /*  Argumente in die ersten Parametervariablen kopieren */
                argl1=mymalloc(arg_cnt*sizeof(struct Var *));
                argl2=mymalloc(arg_cnt*sizeof(struct Var *));

                al=p->alist;vp=v->fi->vars;i=0;
                while(al){
                    while(vp&&(!*vp->identifier||(!zleqto(vp->offset,l2zl(0L))&&!vp->reg)||(vp->storage_class!=REGISTER&&vp->storage_class!=AUTO))) vp=vp->next;
                    if(!vp){ error(39); break; }
                    if(i>=arg_cnt) ierror(0);
                    if(DEBUG&1024) printf("arg: %s(%ld)\n",vp->identifier,zl2l(vp->offset));
                    argl1[i]=vp;
                    argl2[i]=add_var(empty,clone_typ(vp->vtyp),vp->storage_class,0);
                    if(!al->arg) ierror(0);
                    gen_IC(al->arg,0,0);
                    convert(al->arg,vp->vtyp->flags);
                    new=mymalloc(ICS);
                    new->code=ASSIGN;
                    new->q1=al->arg->o;
                    new->q2.flags=0;
                    new->q2.val.vlong=szof(vp->vtyp);
                    new->z.flags=VAR;
                    new->z.val.vlong=l2zl(0L);
                    new->z.v=argl2[i];
                    new->typf=vp->vtyp->flags;
                    add_IC(new);
                    i++;
                    al=al->next;
                    vp=vp->next;
                }
                if(i<arg_cnt){ error(83); arg_cnt=i;}

                /*  Kopien der Variablen erzeugen   */
                for(vp=v->fi->vars;vp;vp=vp->next){
                    vp->inline_copy=0;
                }
                for(i=0;i<arg_cnt;i++){
                    if(argl1[i]){
                        if(!argl2[i]) ierror(0);
                        argl1[i]->inline_copy=argl2[i];
                    }
                }

                /*  Rueckgabewert   */
                if((p->ntyp->flags&NQ)!=VOID){
                    p->o.flags=SCRATCH|VAR;
                    p->o.reg=0;p->o.val.vlong=l2zl(0L);
                    p->o.v=add_var(empty,clone_typ(p->ntyp),AUTO,0);
                }else{
                    p->o.flags=0;
                }

                free(argl1);
                free(argl2);

                /*  Code einfuegen und Labels umschreiben   */
                ip=v->fi->first_ic;lc=0;
                while(ip){
                    struct Var *iv;
                    int c;
                    new=mymalloc(ICS);
                    memcpy(new,ip,ICS);
                    c=ip->code;
                    /*  evtl. ist ein IC praktisch ein SETRETURN, falls das */
                    /*  Rueckgabeziel ueber Parameterzeiger angespr. wird   */
                    if(ip->z.flags&VAR){
                        iv=ip->z.v;
                        if(iv->storage_class==AUTO||iv->storage_class==REGISTER){
                            if(!*iv->identifier&&zleqto(iv->offset,l2zl(0L))){
                                new->z=p->o;
                            }else{
                                if(!iv->inline_copy)
                                    iv->inline_copy=add_var(empty,clone_typ(iv->vtyp),iv->storage_class,0);
                                new->z.v=iv->inline_copy;
                            }/*else if(iv->inline_copy) ierror(0);*/
                        }
                    }
                    /*  Kopien aller auto/register Variablen erzeugen   */
                    if(ip->q1.flags&VAR){
                        iv=ip->q1.v;
                        if(iv->storage_class==AUTO||iv->storage_class==REGISTER){
                            if(!iv->inline_copy)
                                iv->inline_copy=add_var(empty,clone_typ(iv->vtyp),iv->storage_class,0);
                            new->q1.v=iv->inline_copy;
                        }/*else if(iv->inline_copy) ierror(0);*/
                    }
                    if(ip->q2.flags&VAR){
                        iv=ip->q2.v;
                        if(iv->storage_class==AUTO||iv->storage_class==REGISTER){
                            if(!iv->inline_copy)
                                iv->inline_copy=add_var(empty,clone_typ(iv->vtyp),iv->storage_class,0);
                            new->q2.v=iv->inline_copy;
                        }/*else if(iv->inline_copy) ierror(0);*/
                    }
                    if(c==LABEL||(c>=BEQ&&c<=BRA)){
                        if(new->typf>lc) lc=new->typf;
                        new->typf+=label;
                    }
                    if(c==SETRETURN){
                        new->code=ASSIGN;
                        new->z=p->o;
                    }
                    add_IC(new);
                    ip=ip->next;
                }
                label+=lc;
                return;
            }
            /*  einige spezielle Inline-Funktionen; das setzt voraus, dass  */
            /*  diese in den Headerfiles passend deklariert werden          */
            if(v->storage_class==EXTERN){
                if(!strcmp(v->identifier,"strlen")&&p->alist&&p->alist->arg){
                    np n=p->alist->arg;
                    if(n->flags==ADDRESSA&&n->left->flags==STRING){
                        struct const_list *cl;zulong len=ul2zul(0UL);
                        cl=(struct const_list *)n->left->identifier;
                        while(cl){
                            if(zleqto(l2zl(0L),zc2zl(cl->other->val.vchar))) break;
                            len=zuladd(len,ul2zul(1UL));
                            cl=cl->next;
                        }
                        p->o.val.vulong=len;
                        eval_const(&p->o.val,UNSIGNED|LONG);
                        insert_const(p);
                        insert_const2(&p->o.val,p->ntyp->flags);
                        p->flags=CEXPR;
                        p->o.flags=KONST;
                        return;
                    }
                }

                if(INLINEMEMCPY>0&&(c_flags_val[0].l&2)){
                    if(!strcmp(v->identifier,"strcpy")&&p->alist&&p->alist->next&&p->alist->next->arg){
                        np n=p->alist->next->arg;
                        if(n->flags==ADDRESSA&&n->left->flags==STRING){
                            struct const_list *cl;zlong len=l2zl(0L);
                            cl=(struct const_list *)n->left->identifier;
                            while(cl){
                                len=zladd(len,l2zl(1L));
                                if(zleqto(zc2zl(cl->other->val.vchar),l2zl(0L))) break;
                                cl=cl->next;
                            }
                            if(zlleq(len,l2zl((long)INLINEMEMCPY))){
                                inline_memcpy(p->alist->arg,n,len);
                                p->o=p->alist->arg->o;
                                return;
                            }
                        }
                    }
                    if(!strcmp(v->identifier,"memcpy")){
                        if(p->alist&&p->alist->next&&p->alist->next->next
                           &&p->alist->next->next->arg
                           &&p->alist->next->next->arg->flags==CEXPR){
                            eval_constn(p->alist->next->next->arg);
                            if(zlleq(vlong,l2zl((long)INLINEMEMCPY))){
                                inline_memcpy(p->alist->arg,p->alist->next->arg,vlong);
                                p->o=p->alist->arg->o;
                                return;
                            }
                        }
                    }
                }
            }
        }
        rl=0;
#ifdef HAVE_REGPARMS
        {
            struct reg_handle reg_handle=empty_reg_handle;
            sz=push_args(p->alist,p->left->ntyp->next->exact,0,&rl,&reg_handle);
        }
#else
        sz=push_args(p->alist,p->left->ntyp->next->exact,0,&rl);
#endif
        if(!r) gen_IC(p->left,0,0);
        if(!(p->left->o.flags&DREFOBJ)){
            free(new);
            p->o=p->left->o;
            if(p->o.flags&VARADR) p->o.flags&=~VARADR;
             else p->o.flags|=DREFOBJ;
        }else{
            if(p->left->o.flags&VARADR){
                free(new);
                p->o=p->left->o;
                p->o.flags&=~VARADR;
            }else{
                if(p->left->o.flags&SCRATCH){
                    new->z=p->left->o;
                    new->z.flags&=~DREFOBJ;
                }else{
                /* das hier funktioniert vermutlich auch nicht  */
                    get_scratch(&new->z,p->left->ntyp->flags,p->ntyp->flags,p->left->ntyp);
                }
                new->code=ASSIGN;new->typf=POINTER;
                new->q1=p->left->o;
                new->q2.val.vlong=sizetab[POINTER];
                new->q2.flags=0;
                p->o=new->z;
                add_IC(new);
                p->o.flags|=DREFOBJ;
            }
        }
/*            p->left->o.flags-=DREFOBJ|VARADR; Was sollte das??    */

        if(c_flags_val[0].l&2){
            while(rl){
                struct regargs_list *m;
                new=mymalloc(ICS);
                new->code=NOP;
                new->q1.flags=VAR;
                new->q1.v=rl->v;
                new->q1.val.vlong=l2zl(0L);
                new->typf=0;
                new->q2.flags=new->z.flags=0;
                add_IC(new);
                m=rl->next;free(rl);rl=m;
            }
        }
        /*  gegebenenfalls Adresse des Ziels auf den Stack  */
        if(!freturn(p->ntyp)){
            struct obj o;
            new=mymalloc(ICS);
            new->code=ADDRESS;
            new->typf=p->ntyp->flags&NU;
            new->q1.flags=VAR;
            new->q1.v=add_var(empty,clone_typ(p->ntyp),AUTO,0);
            new->q1.val.vlong=l2zl(0L);
            op=&new->q1;
            new->q2.flags=0;
            get_scratch(&new->z,POINTER,p->ntyp->flags&NU,0);
            o=new->z;
            add_IC(new);
            new=mymalloc(ICS);
            new->code=PUSH;
            new->typf=POINTER;
            new->q1=o;
            new->q2.flags=new->z.flags=0;
            new->q2.val.vlong=sizetab[POINTER];
            add_IC(new);
            sz=zladd(sz,sizetab[POINTER]);
        }
        /*  Scratchregister evtl. sichern   */
        savescratch(MOVEFROMREG,last_ic,0);
	function_calls+=currentpri;
        new=mymalloc(ICS);
        new->code=CALL;
        new->typf=FUNKT;
        new->q1=p->o;
        new->q2.flags=new->z.flags=0;
        new->q2.val.vlong=sz; /*  Groesse der Parameter auf dem Stack */
        add_IC(new);
        r=0;
        if((p->ntyp->flags&NQ)!=VOID){
            new=mymalloc(ICS);
            new->code=GETRETURN;
            new->q1.flags=new->q2.flags=0;
            new->q1.reg=freturn(p->ntyp);
            new->q2.val.vlong=szof(p->ntyp);
            new->typf=p->ntyp->flags;
            if(freturn(p->ntyp)){
                int t=p->ntyp->flags&NQ;
                if(t==STRUCT||t==UNION){
                    new->z.v=add_var(empty,clone_typ(p->ntyp),AUTO,0);
                    new->z.flags=VAR;
                    new->z.val.vlong=l2zl(0L);
                }else get_scratch(&new->z,p->ntyp->flags&NU,0,p->ntyp);
            } else new->z=*op;
            if((new->z.flags&(REG|DREFOBJ))==REG) r=new->z.reg;
            p->o=new->z;
            add_IC(new);
        }else{
            p->o.flags=0;
        }
        /*  Scratchregister evtl. wiederherstellen  */
        savescratch(MOVETOREG,last_ic,r);
        /*  Evtl. gespeicherte Registerargumente wiederherstellen.  */
        while(rl){
            struct regargs_list *m;
            if(rl->v){
                new=mymalloc(ICS);
                new->code=MOVETOREG;
                new->typf=0;
                new->q1.flags=VAR;
                new->q1.v=rl->v;
                new->q1.val.vlong=l2zl(0L);
                new->z.flags=REG;
                new->z.reg=rl->reg;
                new->q2.flags=0;
                new->q2.val.vlong=regsize[rl->reg];
                add_IC(new);
            }else{
                new=mymalloc(ICS);
                new->code=FREEREG;
                new->typf=0;
                new->q1.flags=REG;
                new->q2.flags=new->z.flags=0;
                new->q1.reg=rl->reg;
                add_IC(new);
                regs[rl->reg]=0;
            }
            m=rl->next;free(rl);rl=m;
        }
        return;
    }
    if(p->flags>=PREINC&&p->flags<=POSTDEC){
        struct obj o;
        gen_IC(p->left,0,0);
        if(p->flags==POSTINC||p->flags==POSTDEC){
            new=mymalloc(ICS);
            new->code=ASSIGN;
            new->typf=p->ntyp->flags&NU;
            new->q2.val.vlong=sizetab[p->ntyp->flags&NQ];
            new->q1=p->left->o;
            new->q1.flags&=~SCRATCH;
            get_scratch(&new->z,p->left->ntyp->flags&NU,0,p->left->ntyp);
            new->q2.flags=0;
            o=new->z;
            add_IC(new);
            new=mymalloc(ICS);
        }else o=p->left->o;
        if((p->left->ntyp->flags&NQ)==POINTER){
            if(p->flags==PREINC||p->flags==POSTINC) new->code=ADDI2P; else new->code=SUBIFP;
            vlong=szof(p->left->ntyp->next);
            new->q2.val.vint=zl2zi(vlong);
            new->typf=INT;
            new->q1=p->left->o;
            new->z=p->left->o;
            new->q2.flags=KONST;
            add_IC(new);
        }else{
            if(p->flags==PREINC||p->flags==POSTINC) new->code=ADD; else new->code=SUB;
            new->typf=p->ntyp->flags&NU;
            new->q1=p->left->o;
            new->z=p->left->o;
            new->q2.flags=KONST;
            vlong=l2zl(1L);vulong=zl2zul(vlong);vdouble=zl2zd(vlong);
            if(new->typf==CHAR) new->q2.val.vchar=zl2zc(vlong);
            if(new->typf==SHORT) new->q2.val.vshort=zl2zs(vlong);
            if(new->typf==INT) new->q2.val.vint=zl2zi(vlong);
            if(new->typf==LONG) new->q2.val.vlong=vlong;
            if(new->typf==(UNSIGNED|CHAR)) new->q2.val.vuchar=zul2zuc(vulong);
            if(new->typf==(UNSIGNED|SHORT)) new->q2.val.vushort=zul2zus(vulong);
            if(new->typf==(UNSIGNED|INT)) new->q2.val.vuint=zul2zui(vulong);
            if(new->typf==(UNSIGNED|LONG)) new->q2.val.vulong=vulong;
            if(new->typf==DOUBLE) new->q2.val.vdouble=vdouble;
            if(new->typf==FLOAT) new->q2.val.vfloat=zd2zf(vdouble);
            add_IC(new);
        }
        if(p->flags==POSTINC||p->flags==POSTDEC){
            if(p->left->o.flags&SCRATCH) free_reg(p->left->o.reg);
        }
        p->o=o;
        return;
    }
    if(p->flags==COND){
        int ltrue,lfalse,lout;
        ltrue=++label;lfalse=++label;lout=++label;
        gen_IC(p->left,ltrue,lfalse);
        if(!p->left->o.flags){
            free(new);
        }else{
            if(p->left->flags!=CEXPR){
                gen_test(&p->left->o,p->left->ntyp->flags,BEQ,lfalse);
            }else{
                eval_constn(p->left);
                if(zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))&&zdeqto(vdouble,d2zd(0.0))){
                    gen_IC(p->right->right,0,0);
                    p->o=p->right->right->o;
                }else{
                    gen_IC(p->right->left,0,0);
                    p->o=p->right->left->o;
                }
                return;
            }
        }
        gen_label(ltrue);
        gen_IC(p->right->left,0,0);
        if((p->ntyp->flags&NQ)!=VOID){
            convert(p->right->left,p->ntyp->flags&NU);
            if((p->right->left->o.flags&(SCRATCH|DREFOBJ))==SCRATCH){
                p->o=p->right->left->o;
            }else{
                get_scratch(&p->o,p->ntyp->flags&NU,0,p->ntyp);
                new=mymalloc(ICS);
                new->code=ASSIGN;
                new->q1=p->right->left->o;
                new->z=p->o;
                new->q2.flags=0;
                new->q2.val.vlong=szof(p->ntyp);
                new->typf=p->ntyp->flags&NU;
                p->o=new->z;
                add_IC(new);
            }
        }else p->o.flags=0;
        new=mymalloc(ICS);
        new->code=BRA;
        new->typf=lout;
        add_IC(new);
        gen_label(lfalse);
        gen_IC(p->right->right,0,0);
        if((p->ntyp->flags&NQ)!=VOID){
            convert(p->right->right,p->ntyp->flags&NU);
            new=mymalloc(ICS);
            new->code=ASSIGN;
            new->q1=p->right->right->o;
            new->z=p->o;
            new->q2.flags=0;
            new->q2.val.vlong=szof(p->ntyp);
            new->typf=p->ntyp->flags&NU;
            add_IC(new);
        }
        gen_label(lout);
        return;
    }
    printf("Operation: %d=%s\n",p->flags,ename[p->flags]);
    ierror(0);
    free(new);
    p->o.flags=0;
}
#ifdef HAVE_REGPARMS
zlong push_args(struct argument_list *al,struct struct_declaration *sd,int n,struct regargs_list **rl,struct reg_handle *reg_handle)
#else
zlong push_args(struct argument_list *al,struct struct_declaration *sd,int n,struct regargs_list **rl)
#endif
/*  Legt die Argumente eines Funktionsaufrufs in umgekehrter Reihenfolge    */
/*  auf den Stack. Es wird Integer-Erweiterung vorgenommen und float wird   */
/*  nach double konvertiert, falls kein Prototype da ist. Ausserdem werden  */
/*  alle Argumente in ihrer Groesse aligned. Das ist evtl. nicht fuer jede  */
/*  CPU ausreichend. Hier muss evtl. noch etwas getan werden.               */
{
    int t,reg;struct IC *new;struct regargs_list *nrl;zlong sz,of;
#ifdef HAVE_REGPARMS
    int stdreg;
#endif
    if(!al) return(0);
#ifdef HAVE_REGPARMS
    if(n<sd->count) stdreg=reg_parm(reg_handle,(*sd->sl)[n].styp);
        else        stdreg=reg_parm(reg_handle,al->arg->ntyp);
    reg=stdreg;
#else
    reg=0;
#endif
    if(!al->arg) ierror(0);
    if(!sd) ierror(0);
    if(n<sd->count){
      t=(*sd->sl)[n].styp->flags;sz=szof((*sd->sl)[n].styp);
      reg=(*sd->sl)[n].reg;
    }else{
      t=al->arg->ntyp->flags;sz=szof(al->arg->ntyp);
    }
    if((t&NQ)>=CHAR&&(t&NQ)<=LONG) {t=int_erw(t);sz=sizetab[t&NQ];}
    if((t&NQ)==FLOAT&&n>=sd->count) {t=DOUBLE;sz=sizetab[t];}
    sz=zlmult(zldiv(zladd(sz,zlsub(maxalign,l2zl(1L))),maxalign),maxalign);
#ifdef ORDERED_PUSH
    if(reg==0){
      gen_IC(al->arg,0,0);
      convert(al->arg,t&NU);
      /*  Parameteruebergabe ueber Stack. */
      new=mymalloc(ICS);
      new->code=PUSH;
      new->typf=t;
      new->q1=al->arg->o;
      new->q2.flags=new->z.flags=0;
      new->q2.val.vlong=sz;
      add_IC(new);
      if(!al->next) return sz;
    }
#endif
#ifdef HAVE_REGPARMS
    if(al->next) of=push_args(al->next,sd,n+1,rl,reg_handle); else of=l2zl(0L);
#else
    if(al->next) of=push_args(al->next,sd,n+1,rl); else of=l2zl(0L);
#endif
#ifdef ORDERED_PUSH
    if(reg==0) return zladd(of,sz);
#endif
    gen_IC(al->arg,0,0);
    convert(al->arg,t&NU);
    if(reg==0){
        /*  Parameteruebergabe ueber Stack. */
        new=mymalloc(ICS);
        new->code=PUSH;
        new->typf=t;
        new->q1=al->arg->o;
        new->q2.flags=new->z.flags=0;
        new->q2.val.vlong=sz;
        add_IC(new);
        return(zladd(of,sz));
    }else{
        /*  Parameteruebergabe in Register. */
        struct Var *v=0; struct Typ *t2;
        if(c_flags_val[0].l&2){
        /*  Version fuer Optimizer. */
            t2=mymalloc(TYPS);
            t2->flags=t;
            if((t&NQ)==POINTER){
                t2->next=mymalloc(TYPS);
                t2->next->flags=VOID;
                t2->next->next=0;
            }else t2->next=0;
            v=add_var(empty,t2,AUTO,0);
            new=mymalloc(ICS);
            new->code=ASSIGN;
            new->typf=t;
            new->q1=al->arg->o;
            new->q2.flags=0;
            new->q2.val.vlong=sizetab[t&NQ];
            new->z.flags=VAR;
            new->z.v=v;
            new->z.val.vlong=l2zl(0L);
            add_IC(new);
            nrl=mymalloc(sizeof(*nrl));
            nrl->next=*rl;
            nrl->reg=reg;
            nrl->v=v;
            *rl=nrl;
            if(n==0){
            /*  Letztes Argument; jetzt in Register laden.  */
                for(;nrl;nrl=nrl->next){
                    new=mymalloc(ICS);
                    new->code=ASSIGN;
                    new->typf=nrl->v->vtyp->flags|VOLATILE;
                    new->q1.flags=VAR;
                    new->q1.v=nrl->v;
                    new->q1.val.vlong=l2zl(0L);
                    new->q2.flags=0;
                    new->q2.val.vlong=szof(nrl->v->vtyp);
                    new->z.flags=VAR;
                    new->z.val.vlong=l2zl(0L);
                    new->z.v=add_var(empty,clone_typ(nrl->v->vtyp),AUTO,0);
                    new->z.v->reg=nrl->reg;
                    nrl->v=new->z.v;
                    add_IC(new);
                }
            }
            return(of);
        }else{
        /*  Nicht-optimierende Version. */
            if(!regs[reg]){
                new=mymalloc(ICS);
                new->code=ALLOCREG;
                new->typf=0;
                new->q1.flags=REG;
                new->q1.reg=reg;
                new->q2.flags=new->z.flags=0;
                add_IC(new);
                regs[reg]=33;regused[reg]++;
            }else{
                if(al->arg->o.flags!=(REG|SCRATCH)||al->arg->o.reg!=reg){
                    t2=mymalloc(TYPS);
                    t2->flags=ARRAY;
                    t2->size=regsize[reg];
                    t2->next=mymalloc(TYPS);
                    t2->next->flags=CHAR;
                    t2->next->next=0;
                    v=add_var(empty,t2,AUTO,0);
                    new=mymalloc(ICS);
                    new->code=MOVEFROMREG;
                    new->typf=0;
                    new->q1.flags=REG;
                    new->q1.reg=reg;
                    new->q2.flags=0;
                    new->q2.val.vlong=regsize[reg];
                    new->z.flags=VAR;
                    new->z.v=v;
                    new->z.val.vlong=l2zl(0L);
                    add_IC(new);
                }else regs[reg]|=32;
            }
            new=mymalloc(ICS);
            new->code=ASSIGN;
            new->typf=t;
            new->q1=al->arg->o;
            new->q2.flags=new->z.flags=0;
            new->q2.val.vlong=sizetab[t&NQ];
            new->z.flags=REG;
            new->z.reg=reg;
            add_IC(new);
            nrl=mymalloc(sizeof(*nrl));
            nrl->next=*rl;
            nrl->reg=reg;
            nrl->v=v;
            *rl=nrl;
            return(of);
        }
    }
}

void convert(np p,int f)
/*  konvertiert das Objekt in p->o in bestimmten Typ    */
{
    struct IC *new;
    if((f&NQ)==VOID||(p->ntyp->flags&NU)==f) return;
    if(p->flags==CEXPR||p->flags==PCEXPR){
        eval_constn(p);
        p->ntyp->flags=f;
        insert_const(p);
        p->o.val=p->val;
        return;
    }
    if(!must_convert(p,f)) return;
    new=mymalloc(ICS);
    new->q1=p->o;
    new->q2.flags=0;
    new->code=CONVCHAR+(p->ntyp->flags&NQ)-CHAR;
    if(p->ntyp->flags&UNSIGNED) new->code+=8;
    new->typf=f;
    if(!regok(p->o.reg,f,0)||!(p->o.flags&SCRATCH)){
        get_scratch(&new->z,f,0,0);
        p->o=new->z;
        add_IC(new);
    }else{
        new->z=p->o;new->z.flags&=~DREFOBJ;
        p->o=new->z;
        add_IC(new);
    }
}

int allocreg(int f,int mode)
/*  Fordert Register fuer Typ f an                      */
/*  evtl. maschinenabhaengig, aber hier fehlt noch viel */
/*  z.B. Eintragen eines IC                             */
{
    int i;struct IC *new;
    if(nocode) return(1);
    for(i=1;i<=MAXR;i++){
        if(!regs[i]&&regok(i,f,mode)){
            if(DEBUG&16) printf("alocated %s\n",regnames[i]);
            regs[i]=1;regused[i]++;
            new=mymalloc(ICS);
            new->code=ALLOCREG;
            new->typf=0;
            new->q1.flags=REG;
            new->q1.reg=i;
            new->q2.flags=new->z.flags=0;
            add_IC(new);
            return(i);
        }
    }
    if(DEBUG&1) printf(">%sCouldn't allocate register for type %d\n",string,f);
    return(0);
}
void free_reg(int r)
/*  Gibt Register r frei                                */
/*  Eintrag eines IC fehlt noch                         */
{
    struct IC *new;
    if(!r||nocode) return;
    if(regs[r]==0) {printf("Register %s:\n",regnames[r]);ierror(0);}
    if(DEBUG&16) printf("freed %s\n",regnames[r]);
    new=mymalloc(ICS);
    new->code=FREEREG;
    new->typf=0;
    new->q1.flags=REG;
    new->q1.reg=r;
    new->q2.flags=new->z.flags=0;
    add_IC(new);
    regs[r]=0;
}
void gen_label(int l)
/*  Erzeugt ein Label                                   */
{
    struct IC *new;
    new=mymalloc(ICS);
    new->code=LABEL;
    new->typf=l;
    new->q1.flags=new->q2.flags=new->z.flags=0;
    add_IC(new);
}
struct obj gen_cond(int m,int l1,int l2)
/*  Generiert code, der 0 oder 1 in Register schreibt   */
{
    struct IC *new;
    struct obj omerk;
    new=mymalloc(ICS);
    new->code=ASSIGN;
    new->typf=INT;
    new->q1.flags=KONST;
    new->q2.flags=0;
    new->q2.val.vlong=sizetab[INT];
    if(!m) vlong=l2zl(1L); else vlong=l2zl(0L);
    new->q1.val.vint=zl2zi(vlong);
    get_scratch(&new->z,INT,0,0);
    omerk=new->z;
    add_IC(new);
    new=mymalloc(ICS);
    new->code=BRA;
    new->typf=l2;
    add_IC(new);
    gen_label(l1);
    new=mymalloc(ICS);
    new->code=ASSIGN;
    new->typf=INT;
    new->q1.flags=KONST;
    new->q2.flags=0;
    new->q2.val.vlong=sizetab[INT];
    if(!m) vlong=l2zl(0L); else vlong=l2zl(1L);
    new->q1.val.vint=zl2zi(vlong);
    new->z=omerk;
/*    new->z.reg=r;
    new->z.flags=SCRATCH|REG;*/
    add_IC(new);
    gen_label(l2);
    return(omerk);
}
void scratch_var(struct obj *o,int t,struct Typ *typ)
/*  liefert eine temporaere Variable                            */
/*  nicht effizient, aber wer hat schon so wenig Register...    */
{
  struct Typ *nt;
  if((t&NQ)<CHAR||(t&NQ)>POINTER){
    if(!typ) ierror(0);
    nt=clone_typ(typ);
  }else{
    nt=mymalloc(TYPS);
    nt->flags=t&NU;
    if((t&NQ)==POINTER){
      nt->next=mymalloc(TYPS);
      nt->next->flags=VOID;
      nt->next->next=0;
    }else nt->next=0;
  }
  o->flags=SCRATCH|VAR;o->reg=0;
  o->v=add_var(empty,nt,AUTO,0);
  o->val.vlong=l2zl(0L);
}
void get_scratch(struct obj *o,int t1,int t2,struct Typ *typ)
/*  liefert ein Scratchregister oder eine Scratchvariable       */
{
    if(!(c_flags_val[0].l&2)&&(o->reg=allocreg(t1,t2))){
        o->flags=SCRATCH|REG;
    }else{
        scratch_var(o,t1,typ);
    }
}
int do_arith(np p,struct IC *new,np dest,struct obj *o)
/*  erzeugt IC new fuer einen arithmetischen Knoten und speichert das   */
/*  Resultat vom Unterknoten dest in o (fuer a op= b)               */
/*  liefert 0, wenn dest nicht gefunden                             */
{
    int f=0,mflags;
    new->code=p->flags;
    if(new->code==PMULT) new->code=MULT;
    gen_IC(p->left,0,0);
    if(dest&&p->left==dest) {*o=p->left->o;f++;}
    gen_IC(p->right,0,0);
    if(dest&&p->right==dest) {*o=p->right->o;f++;}
    if(dest){ mflags=dest->o.flags;dest->o.flags&=(~SCRATCH);}

    if((p->left->ntyp->flags&NQ)==POINTER&&(p->right->ntyp->flags&NQ)==POINTER){
    /*  Subtrahieren zweier Pointer                                 */
        if(p->flags!=SUB) ierror(0);
        new->typf=INT;
        new->code=SUBPFP;
        new->q1=p->left->o;
        new->q2=p->right->o;
        if(!dest&&p->left->o.flags&SCRATCH&&regok(p->left->o.reg,INT,0)){
            new->z=p->left->o;
            new->z.flags&=~DREFOBJ;
        }else{
            if(USEQ2ASZ&&(p->right->o.flags&SCRATCH)&&regok(p->right->o.reg,INT,0)){
                new->z=p->left->o;
                new->z.flags&=(~DREFOBJ);
            }else{
                get_scratch(&new->z,INT,0,0);
            }
        }
        p->o=new->z;
        add_IC(new);
        if(!zlleq(szof(p->left->ntyp->next),l2zl(1L))){
            new=mymalloc(ICS);
            new->code=DIV;
            new->q1=p->o;
            new->q2.flags=KONST;
            vlong=szof(p->left->ntyp->next);
            vint=zl2zi(vlong);
            new->q2.val.vint=vint;
            new->z=p->o;
            new->typf=INT;
            add_IC(new);
        }
	if(dest) dest->o.flags=mflags;
        return(f);
    }
    if((p->flags==ADD||p->flags==SUB)&&(p->ntyp->flags&NQ)==POINTER){
    /*  Addieren und Subtrahieren eines Integers zu einem Pointer   */
        if(p->flags==ADD) new->code=ADDI2P; else new->code=SUBIFP;
        new->typf=p->right->ntyp->flags&NU;
        new->q1=p->left->o;
        /*  kleinere Typen als MINADDI2P erst in diesen wandeln */
        if((new->typf&NQ)<MINADDI2P){convert(p->right,/*UNSIGNED|*/MINADDI2P);new->typf=/*UNSIGNED|*/MINADDI2P;}
        new->q2=p->right->o;
        if(!dest&&(p->left->o.flags&SCRATCH)&&regok(new->q1.reg,POINTER,p->left->ntyp->next->flags&NU)){
            new->z=p->left->o;
            new->z.flags&=(~DREFOBJ);
        }else{
            get_scratch(&new->z,POINTER,p->left->ntyp->next->flags&NU,0);
        }
        p->o=new->z;
        add_IC(new);
	if(dest) dest->o.flags=mflags;
        return(f);
    }
    convert(p->left,p->ntyp->flags&NU);
    convert(p->right,p->ntyp->flags&NU);
    new->q1=p->left->o;
    new->q2=p->right->o;
    new->typf=p->ntyp->flags&NU;
    /*  Bei dest!=0, d.h. ASSIGNADD, darf q1 nicht als Ziel benuzt werden!  */
    if(!dest&&(new->q1.flags&SCRATCH)&&regok(new->q1.reg,p->ntyp->flags,0)){
        new->z=new->q1;
        new->z.flags&=~DREFOBJ;
    }else{
        if((new->q2.flags&SCRATCH)&&regok(new->q2.reg,p->ntyp->flags,0)){
            if((p->flags>=OR&&p->flags<=AND)||p->flags==ADD||p->flags==MULT||p->flags==PMULT){
            /*  bei kommutativen Operatoren vertauschen     */
                new->z=new->q2;
                new->q2=new->q1;
                new->q1=new->z;
                new->z.flags&=~DREFOBJ;
            }else{
                if(USEQ2ASZ){
                    new->z=new->q2;
                    new->z.flags&=~DREFOBJ;
                }else{
                    get_scratch(&new->z,new->typf,0,0);
                }
            }
        }else{
            get_scratch(&new->z,new->typf,0,0);
        }
    }
    p->o=new->z;
    add_IC(new);
    if(dest) dest->o.flags=mflags;
    return(f);
}
void savescratch(int code,struct IC *p,int dontsave)
/*  speichert Scratchregister bzw. stellt sie wieder her (je nach code  */
/*  entweder MOVEFROMREG oder MOVETOREG)                                */
{
    int i,s,e,b;struct IC *new;
    if(code==MOVETOREG){ s=1;e=MAXR+1;b=1;} else {s=MAXR;e=0;b=-1;}
    for(i=s;i!=e;i+=b){
        if(regs[i]&&!(regs[i]&32)&&regscratch[i]&&i!=dontsave){
            if(!regsbuf[i]){
                struct Typ *t;
                if(code!=MOVEFROMREG) continue;
                t=mymalloc(TYPS);
                t->flags=ARRAY;
                t->size=regsize[i];
                t->next=mymalloc(TYPS);
                t->next->flags=CHAR;
                t->next->next=0;
                regsbuf[i]=add_var(empty,t,AUTO,0);
                regbnesting[i]=nesting;
            }
            new=mymalloc(ICS);
            new->typf=new->q2.flags=0;
            new->line=0;new->file=0;
            new->code=code;
            if(code==MOVEFROMREG){
                new->q1.flags=REG;new->q1.reg=i;
                new->z.flags=VAR;new->z.v=regsbuf[i];
                new->z.val.vlong=l2zl(0L);
            }else{
                new->z.flags=REG;new->z.reg=i;
                new->q1.flags=VAR;new->q1.v=regsbuf[i];
                new->q1.val.vlong=l2zl(0L);
            }
            new->use_cnt=new->change_cnt=0;
            new->use_list=new->change_list=0;
            insert_IC(p,new);
        }
    }
}

