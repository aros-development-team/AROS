/*  $VER: vbcc (statements.c) V0.4  */

#include "vbc.h"

static char FILE_[]=__FILE__;

int cont_label=0;
int test_assignment(struct Typ *,np);

#define cr()
#ifndef cr
void cr(void)
/*  tested Registerbelegung */
{
    int i;
    for(i=0;i<=MAXR;i++)
        if(regs[i]!=regsa[i]) {error(149,regnames[i]);regs[i]=regsa[i];}
}
#endif
void statement(void)
/*  bearbeitet ein statement                                    */
{
    char *merk;
    cr();
    killsp();
    if(*s=='{'){
        enter_block();
        if(nesting>0) local_offset[nesting]=local_offset[nesting-1];
        compound_statement();leave_block();return;}
    merk=s;
    cpbez(buff,0);
    if(!strcmp("if",buff)){if_statement();return;}
    if(!strcmp("switch",buff)){switch_statement();return;}
    if(!strcmp("for",buff)){for_statement();return;}
    if(!strcmp("while",buff)){while_statement();return;}
    if(!strcmp("do",buff)){do_statement();return;}
    if(!strcmp("goto",buff)){goto_statement();return;}
    if(!strcmp("continue",buff)){continue_statement();return;}
    if(!strcmp("break",buff)){break_statement();return;}
    if(!strcmp("return",buff)){return_statement();return;}
    if(!strcmp("case",buff)){labeled_statement();return;}
    killsp();if(*s==':'){labeled_statement();return;}
    /*  fehlt Aufruf der anderen statements */
    s=merk;
    expression_statement();
}
void labeled_statement(void)
/*  bearbeitet labeled_statement                                */
{
    struct llist *lp;int def=0;
    nocode=0;
    if(*s==':'){
        s++;
        if(!*buff){error(130);return;}
        if(!strcmp("default",buff)){def=1;lp=0;} else lp=find_label(buff);
        if(lp&&lp->flags&LABELDEFINED){error(131,buff);return;}
        if(!lp) lp=add_label(buff);
        lp->flags|=LABELDEFINED;
        lp->switch_count=0;
        if(def){
            if(switch_act==0) error(150);
            lp->flags|=LABELDEFAULT;
            lp->switch_count=switch_act;
        }
        gen_label(lp->label);
        afterlabel=0;
    }else{
        /*  case    */
        np tree;struct llist *lp;
        tree=expression();
        killsp();
        if(*s==':'){s++;killsp();} else error(70);
        if(!switch_count){
            error(132);
        }else{
            if(!tree||!type_expression(tree)){
            }else{
                if(tree->flags!=CEXPR||tree->sidefx){
                    error(133);
                }else{
                    if((tree->ntyp->flags&15)<CHAR||(tree->ntyp->flags&15)>LONG){
                        error(134);
                    }else{
                        lp=add_label(empty);
                        lp->flags=LABELDEFINED;
                        lp->switch_count=switch_act;
                        eval_constn(tree);
                        if(switch_typ==CHAR) lp->val.vchar=vchar;
                        if(switch_typ==(UNSIGNED|CHAR)) lp->val.vuchar=vuchar;
                        if(switch_typ==SHORT) lp->val.vshort=vshort;
                        if(switch_typ==(UNSIGNED|SHORT)) lp->val.vushort=vushort;
                        if(switch_typ==INT) lp->val.vint=vint;
                        if(switch_typ==(UNSIGNED|INT)) lp->val.vuint=vuint;
                        if(switch_typ==LONG) lp->val.vlong=vlong;
                        if(switch_typ==(UNSIGNED|LONG)) lp->val.vulong=vulong;
                        if(switch_typ==POINTER) lp->val.vpointer=vpointer;
                        gen_label(lp->label);
                    }
                }
            }
        }
        if(tree) free_expression(tree);
    }
    cr();
    killsp();
    if(*s!='}') statement();
}
void if_statement(void)
/*  bearbeitet if_statement                                     */
{
    int ltrue,lfalse,lout,cexpr,cm;char *merk,buff[MAXI];
    np tree;struct IC *new;
    killsp(); if(*s=='(') s++; else error(151);
    killsp();cm=nocode;
    tree=expression();
    if(!tree) {error(135);
    }else{
        ltrue=++label;lfalse=++label;
        if(type_expression(tree)){
            tree=makepointer(tree);
            if(!arith(tree->ntyp->flags&15)&&(tree->ntyp->flags&15)!=POINTER)
                {error(136);
            }else{
                if(tree->flags==ASSIGN) error(164);
                gen_IC(tree,ltrue,lfalse);
                if(tree->flags==CEXPR){
                    eval_const(&tree->val,tree->ntyp->flags&31);
                    if(zdeqto(vdouble,d2zd(0.0))&&zleqto(vlong,l2zl(0))&&zuleqto(vulong,ul2zul(0UL))) cexpr=2; else cexpr=1;
                }else cexpr=0;
                if((tree->o.flags&SCRATCH)&&cexpr) free_reg(tree->o.reg);
                if(tree->o.flags&&!cexpr){
                    new=mymalloc(ICS);
                    new->code=TEST;
                    new->q1=tree->o;
                    new->q2.flags=new->z.flags=0;
                    new->typf=tree->ntyp->flags;
                    add_IC(new);
                    new=mymalloc(ICS);
                    new->code=BEQ;
                    new->typf=lfalse;
                    add_IC(new);
                }
                if(cexpr==2){
                    new=mymalloc(ICS);
                    new->code=BRA;
                    new->typf=lfalse;
                    add_IC(new);
                }
            }
        }
        free_expression(tree);
    }
    killsp(); if(*s==')') s++; else error(59);
    if(cexpr==2) nocode=1;
    if(!cexpr&&!tree->o.flags) gen_label(ltrue);
    statement();
    killsp();
    merk=s;
    cpbez(buff,0);
    if(strcmp("else",buff)) {s=merk;nocode=cm;if(cexpr!=1) gen_label(lfalse);return;}
    lout=++label;
    if(cexpr!=2){
        new=mymalloc(ICS);
        new->code=BRA;
        new->typf=lout;
        add_IC(new);
    }
    if(cexpr!=1) {nocode=cm;gen_label(lfalse);}
    if(cexpr==1) nocode=1; else nocode=cm;
    statement();
    nocode=cm;
    if(cexpr!=2) gen_label(lout);
    cr();
}
void switch_statement(void)
/*  bearbeitet switch_statement                                 */
{
    np tree;int merk_typ,merk_count,merk_break;
    struct IC *merk_fic,*merk_lic,*new;struct llist *lp,*l1,*l2;
    killsp();
    if(*s=='('){s++;killsp();} else error(151);
    tree=expression(); killsp();
    if(*s==')'){s++;killsp();} else error(59);
    merk_typ=switch_typ;merk_count=switch_act;merk_break=break_label;
    if(!tree){
        error(137);
    }else{
        if(!type_expression(tree)){
        }else{
            if((tree->ntyp->flags&15)<CHAR||(tree->ntyp->flags&15)>LONG){
                error(138);
            }else{
                int m1,m2,m3,def=0,rm,minflag;
                zlong l,ml,s;zulong ul,mul,us;
                if(tree->flags==ASSIGN) error(164);
                m3=break_label=++label;m1=switch_act=++switch_count;
                m2=switch_typ=tree->ntyp->flags&31;
                gen_IC(tree,0,0);
                if((tree->o.flags&(DREFOBJ|SCRATCH))!=SCRATCH){
                    new=mymalloc(ICS);
                    new->code=ASSIGN;
                    new->q1=tree->o;
                    new->q2.flags=0;
                    new->q2.val.vlong=sizetab[m2&15];
                    get_scratch(&new->z,m2,0);
                    new->typf=m2;
                    tree->o=new->z;
                    add_IC(new);
                }
                if((tree->o.flags&(SCRATCH|REG))==(SCRATCH|REG)){
                    int r=tree->o.reg;
                    rm=regs[r];
                    regs[r]=regsa[r];
                }
                merk_fic=first_ic;merk_lic=last_ic;
                first_ic=last_ic=0;
                statement();
                if((tree->o.flags&(SCRATCH|REG))==(SCRATCH|REG)) regs[tree->o.reg]=rm;
                minflag=0;s=l2zl(0L);us=ul2zul(0UL);
                for(l1=first_llist;l1;l1=l1->next){
                    if(l1->switch_count!=m1) continue;
                    if(l1->flags&LABELDEFAULT){
                        if(def) error(139);
                        def=l1->label;
                        continue;
                    }
                    lp=0;minflag&=~1;
                    for(l2=first_llist;l2;l2=l2->next){
                        if(l2->switch_count!=m1) continue;
                        if(l2->flags&LABELDEFAULT) continue;
                        eval_const(&l2->val,m2);
                        if(minflag&2){
                            if(m2&UNSIGNED){
                                if(zulleq(vulong,mul)||zuleqto(vulong,mul)) continue;
                            }else{
                                if(zlleq(vlong,ml)||zleqto(vlong,ml)) continue;
                            }
                        }
                        if(minflag&1){
                            if(m2&UNSIGNED){
                                if(!(minflag&4)&&zuleqto(vulong,ul)){ error(201);minflag|=4;}
                                if(zulleq(vulong,ul)){lp=l2;ul=vulong;}
                            }else{
                                if(!(minflag&4)&&zleqto(vlong,l)){ error(201);minflag|=4;}
                                if(zlleq(vlong,l)){lp=l2;l=vlong;}
                            }
                        }else{
                            minflag|=1;
                            l=vlong;
                            ul=vulong;
                            lp=l2;
                        }
                    }
                    if(!lp) continue;
                    ml=l;mul=ul;minflag|=2;
                    if(SWITCHSUBS){
                        new=mymalloc(ICS);
                        new->line=0;
                        new->file=0;
                        new->typf=m2;
                        new->code=SUB;
                        new->q1=tree->o;
                        new->z=tree->o;
                        new->q2.flags=KONST;
                        eval_const(&lp->val,m2);
                        vlong=zlsub(vlong,s);
                        vulong=zulsub(vulong,us);
                        vint=zl2zi(vlong);
                        vshort=zl2zs(vlong);
                        vchar=zl2zc(vlong);
                        vuint=zul2zui(vulong);
                        vushort=zul2zus(vulong);
                        vuchar=zul2zuc(vulong);
                        insert_const2(&new->q2.val,m2);
                        new->q1.am=new->q2.am=new->z.am=0;
                        s=l;us=ul;
                        new->prev=merk_lic;
                        if(merk_lic) merk_lic->next=new; else merk_fic=new;
                        merk_lic=new;
                        new=mymalloc(ICS);
                        new->line=0;
                        new->file=0;
                        new->typf=m2;
                        new->code=TEST;
                        new->q1=tree->o;
                        new->q2.flags=new->z.flags=0;
                        new->prev=merk_lic;
                        new->q1.am=new->q2.am=new->z.am=0;
                        if(merk_lic) merk_lic->next=new; else merk_fic=new;
                        merk_lic=new;
                    }else{
                        new=mymalloc(ICS);
                        new->line=0;
                        new->file=0;
                        new->code=COMPARE;
                        new->typf=m2;
                        new->q1=tree->o;
                        new->q2.flags=KONST;
                        new->q2.val=lp->val;
                        new->z.flags=0;
                        new->prev=merk_lic;
                        new->q1.am=new->q2.am=new->z.am=0;
                        if(merk_lic) merk_lic->next=new; else merk_fic=new;
                        merk_lic=new;
                    }
                    new=mymalloc(ICS);
                    new->line=0;
                    new->file=0;
                    new->code=BEQ;
                    new->typf=lp->label;
                    new->q1.flags=new->q2.flags=new->z.flags=0;
                    new->prev=merk_lic;
                    new->q1.am=new->q2.am=new->z.am=0;
                    merk_lic->next=new;
                    merk_lic=new;
                }
                if((tree->o.flags&(SCRATCH|REG))==(SCRATCH|REG)){   /* free_reg(tree->o.reg); */
                    new=mymalloc(ICS);
                    new->line=0;
                    new->file=0;
                    new->code=FREEREG;new->typf=0;
                    new->q2.flags=new->z.flags=0;
                    new->q1.flags=REG;
                    new->q1.reg=tree->o.reg;
                    new->prev=merk_lic;
                    new->q1.am=new->q2.am=new->z.am=0;
                    if(merk_lic) merk_lic->next=new; else merk_fic=new;
                    merk_lic=new;
                    regs[tree->o.reg]=regsa[tree->o.reg];
                }
                new=mymalloc(ICS);
                new->line=0;
                new->file=0;
                new->code=BRA;
                if(def) new->typf=def; else new->typf=m3;
                new->q1.flags=new->q2.flags=new->z.flags=0;
                if(merk_lic) merk_lic->next=new; else merk_fic=new;
                new->prev=merk_lic;
                first_ic->prev=new;
                new->next=first_ic;
                new->q1.am=new->q2.am=new->z.am=0;
                first_ic=merk_fic;
                gen_label(m3);
            }
        }
    }
    switch_typ=merk_typ;switch_act=merk_count;break_label=merk_break;
    if(tree) free_expression(tree);
    cr();
}
void while_statement(void)
/*  bearbeitet while_statement                                  */
{
    np tree;int lloop,lin,lout,cm,cexpr,contm,breakm;
    struct IC *new;
    killsp();
    if(*s=='(') {s++;killsp();} else error(151);
    tree=expression();
    cexpr=0;
    if(tree){
        if(type_expression(tree)){
            tree=makepointer(tree);
            if(!arith(tree->ntyp->flags&15)&&(tree->ntyp->flags&15)!=POINTER){
                error(140);
                cexpr=-1;
            }else{
                if(tree->flags==ASSIGN) error(164);
                if(tree->flags==CEXPR){
                    eval_const(&tree->val,tree->ntyp->flags&31);
                    if(zdeqto(vdouble,d2zd(0.0))&&zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))) cexpr=1; else cexpr=2;
                    if(cexpr==1) error(152);
                }
            }
        }else cexpr=-1;
    } else error(141);
    lloop=++label;lin=++label;lout=++label;cm=nocode;
    contm=cont_label;breakm=break_label;
    if(!cexpr||tree->sidefx) cont_label=lin; else cont_label=lloop;
    if(!cexpr||tree->sidefx){
        if(c_flags_val[0].l&2){ /*  bei Optimierung */
            gen_IC(tree,lloop,lout);
            if(tree->o.flags){
                new=mymalloc(ICS);
                new->code=TEST;
                new->typf=tree->ntyp->flags&31;
                new->q1=tree->o;
                new->q2.flags=new->z.flags=0;
                add_IC(new);
                new=mymalloc(ICS);
                new->code=BEQ;
                new->typf=lout;
                add_IC(new);
            }
            if(!type_expression(tree)) ierror(0);
        }else{
            new=mymalloc(ICS);
            new->code=BRA;
            new->typf=lin;
            add_IC(new);
        }
    }
    if(cexpr==1){
        new=mymalloc(ICS);
        new->code=BRA;
        new->typf=lout;
        add_IC(new);
    }else gen_label(lloop);
    cm=nocode;break_label=lout;
    if(cexpr==1) nocode=1;
    currentpri*=looppri;
    killsp();
    if(*s==')') {s++;killsp();} else error(59);
    statement();
    nocode=cm;cont_label=contm;break_label=breakm;
    if(!cexpr||tree->sidefx) gen_label(lin);
    if(tree&&cexpr>=0){
        if(cexpr!=1||tree->sidefx){
            gen_IC(tree,lloop,lout);
            if((tree->o.flags&SCRATCH)&&cexpr) free_reg(tree->o.reg);
        }
        if(tree->o.flags&&!cexpr){
            new=mymalloc(ICS);
            new->code=TEST;
            new->typf=tree->ntyp->flags&31;
            new->q1=tree->o;
            new->q2.flags=new->z.flags=0;
            add_IC(new);
            new=mymalloc(ICS);
            new->code=BNE;
            new->typf=lloop;
            add_IC(new);
        }
        if(cexpr==2){
            new=mymalloc(ICS);
            new->code=BRA;
            new->typf=lloop;
            add_IC(new);
        }
    }
    if(tree) free_expression(tree);
    gen_label(lout);
    currentpri/=looppri;
    cr();
}
void for_statement(void)
/*  bearbeitet for_statement                                    */
{
    np tree1=0,tree2=0,tree3=0;int lloop,lin,lout,cm,cexpr,contm,breakm;
    struct IC *new;
    killsp();
    if(*s=='(') {s++;killsp();} else error(59);
    if(*s!=';') tree1=expression();
    if(tree1){
        if(tree1->flags==POSTINC) tree1->flags=PREINC;
        if(tree1->flags==POSTDEC) tree1->flags=PREDEC;
        if(type_expression(tree1)){
            if(tree1->sidefx){
                gen_IC(tree1,0,0);
                if(tree1&&(tree1->o.flags&SCRATCH)) free_reg(tree1->o.reg);
            }else{error(153);}
        }
        free_expression(tree1);
    }
    cexpr=0;
    killsp();
    if(*s==';') {s++;killsp();} else error(54);
    if(*s!=';') {tree2=expression();killsp();} else {cexpr=2;}
    if(*s==';') {s++;killsp();} else error(54);
    if(*s!=')') tree3=expression();
    killsp();
    if(*s==')') {s++;killsp();} else error(59);
    if(tree3){
        if(!type_expression(tree3)){
            free_expression(tree3);
            tree3=0;
        }
    }
    if(tree2){
        if(type_expression(tree2)){
            tree2=makepointer(tree2);
            if(!arith(tree2->ntyp->flags&15)&&(tree2->ntyp->flags&15)!=POINTER){
                error(142);
                cexpr=-1;
            }else{
                if(tree2->flags==ASSIGN) error(164);
                if(tree2->flags==CEXPR){
                    eval_const(&tree2->val,tree2->ntyp->flags&31);
                    if(zdeqto(vdouble,d2zd(0.0))&&zleqto(vlong,l2zl(0L))&&zuleqto(vulong,ul2zul(0UL))) cexpr=1; else cexpr=2;
                    if(cexpr==1) error(152);
                }
            }
        }else cexpr=-1;
    }
    lloop=++label;lin=++label;lout=++label;cm=nocode;
    contm=cont_label;breakm=break_label;
    cont_label=++label;break_label=lout;
    if(!cexpr||(tree2&&tree2->sidefx)){
        if(c_flags_val[0].l&2){ /*  bei Optimierung */
            gen_IC(tree2,lloop,lout);
            if(tree2->o.flags){
                new=mymalloc(ICS);
                new->code=TEST;
                new->typf=tree2->ntyp->flags&31;
                new->q1=tree2->o;
                new->q2.flags=new->z.flags=0;
                add_IC(new);
                new=mymalloc(ICS);
                new->code=BEQ;
                new->typf=lout;
                add_IC(new);
            }
            if(!type_expression(tree2)) ierror(0);
        }else{
            new=mymalloc(ICS);
            new->code=BRA;
            new->typf=lin;
            add_IC(new);
        }
    }
    if(cexpr==1){
        new=mymalloc(ICS);
        new->code=BRA;
        new->typf=lout;
        add_IC(new);
    }else gen_label(lloop);
    cm=nocode;
    if(cexpr==1) nocode=1;
    currentpri*=looppri;
    statement();
    nocode=cm;
    gen_label(cont_label);
    cont_label=contm;break_label=breakm;
    if(tree3){
        if(tree3->flags==POSTINC) tree3->flags=PREINC;
        if(tree3->flags==POSTDEC) tree3->flags=PREDEC;
        if(tree3->sidefx){
            gen_IC(tree3,0,0);
            if(tree3&&(tree3->o.flags&SCRATCH)) free_reg(tree3->o.reg);
        }else error(153);
        free_expression(tree3);
    }
    if(!cexpr||(tree2&&tree2->sidefx)) gen_label(lin);
    if(tree2&&cexpr>=0){
        if(cexpr!=1||tree2->sidefx){
            gen_IC(tree2,lloop,lout);
            if((tree2->o.flags&SCRATCH)&&cexpr) free_reg(tree2->o.reg);
        }
        if(tree2->o.flags&&!cexpr){
            new=mymalloc(ICS);
            new->code=TEST;
            new->typf=tree2->ntyp->flags&31;
            new->q1=tree2->o;
            new->q2.flags=new->z.flags=0;
            add_IC(new);
            new=mymalloc(ICS);
            new->code=BNE;
            new->typf=lloop;
            add_IC(new);
        }
        if(cexpr==2){
            new=mymalloc(ICS);
            new->code=BRA;
            new->typf=lloop;
            add_IC(new);
        }
    }
    if(!tree2&&cexpr==2){
        new=mymalloc(ICS);
        new->code=BRA;
        new->typf=lloop;
        add_IC(new);
    }
    if(tree2) free_expression(tree2);
    gen_label(lout);
    currentpri/=looppri;
    cr();
}
void do_statement(void)
/*  bearbeitet do_statement                                     */
{
    np tree;int lloop,lout,contm,breakm;
    struct IC *new;
    lloop=++label;lout=++label;currentpri*=looppri;
    gen_label(lloop);
    breakm=break_label;contm=cont_label;cont_label=++label;break_label=lout;
    statement();
    killsp();
    gen_label(cont_label);cont_label=contm;break_label=breakm;
    cpbez(buff,0);killsp();
    if(strcmp("while",buff)) error(154);
    if(*s=='(') {s++;killsp();} else error(151);
    tree=expression();
    if(tree){
        if(type_expression(tree)){
            tree=makepointer(tree);
            if(arith(tree->ntyp->flags&15)||(tree->ntyp->flags&15)==POINTER){
                if(tree->flags==ASSIGN) error(164);
                if(tree->flags==CEXPR){
                    eval_const(&tree->val,tree->ntyp->flags&31);
                    if(tree->sidefx) gen_IC(tree,0,0);
                    if(!zdeqto(vdouble,d2zd(0.0))){
                        new=mymalloc(ICS);
                        new->code=BRA;
                        new->typf=lloop;
                        add_IC(new);
                    }
                }else{
                    gen_IC(tree,lloop,lout);
                    if(tree->o.flags){
                        new=mymalloc(ICS);
                        new->code=TEST;
                        new->typf=tree->ntyp->flags&31;
                        new->q1=tree->o;
                        new->q2.flags=new->z.flags=0;
                        add_IC(new);
                        new=mymalloc(ICS);
                        new->code=BNE;
                        new->typf=lloop;
                        add_IC(new);
                    }
                }
            }else error(143);
        }
        free_expression(tree);
    }
    killsp();
    if(*s==')') {s++;killsp();} else error(59);
    if(*s==';') {s++;killsp();} else error(54);
    gen_label(lout);
    currentpri/=looppri;
    cr();
}
void goto_statement(void)
/*  bearbeitet goto_statement                                   */
{
    struct llist *lp;
    struct IC *new;
    killsp();cpbez(buff,1);
    if(!*buff) error(144);
    lp=find_label(buff);
    if(!lp) lp=add_label(buff);
    lp->flags|=LABELUSED;
    new=mymalloc(ICS);
    new->typf=lp->label;
    new->code=BRA;
    new->typf=lp->label;
    add_IC(new);
    killsp();
    if(*s==';'){s++;killsp();} else error(54);
    cr();
    goto_used=1;
}
void continue_statement(void)
/*  bearbeitet continue_statement                               */
{
    struct IC *new;
    if(cont_label==0){error(145);return;}
    new=mymalloc(ICS);
    new->code=BRA;
    new->typf=cont_label;
    add_IC(new);
    killsp();
    if(*s==';') {s++;killsp();} else error(54);
    cr();
}
void break_statement(void)
/*  bearbeitet break_statement                                  */
{
    struct IC *new;
    if(break_label==0){error(146);return;}
    new=mymalloc(ICS);
    new->code=BRA;
    new->typf=break_label;
    add_IC(new);
    killsp();
    if(*s==';') {s++;killsp();} else error(54);
    cr();
}
extern int has_return;
void return_statement(void)
/*  bearbeitet return_statement                                 */
/*  SETRETURN hat Groesse in q2.reg und z.reg==freturn(rtyp)    */
{
    np tree;
    struct IC *new;
    has_return=1;
    killsp();
    if(*s!=';'){
        if(tree=expression()){
            if(!return_typ){
                if(type_expression(tree)){
                    tree=makepointer(tree);
                    if((tree->ntyp->flags&15)!=VOID)
                        error(155);
                    gen_IC(tree,0,0);
                    if(tree->o.flags&SCRATCH) free_reg(tree->o.reg);
                }
            }else{
                if(type_expression(tree)){
                    tree=makepointer(tree);
                    if(!test_assignment(return_typ,tree)){free_expression(tree);return;}
                    gen_IC(tree,0,0);
                    convert(tree,return_typ->flags&31);
#ifdef OLDPARMS   /*  alte CALL/RETURN-Methode    */
                    new=mymalloc(ICS);
                    new->code=ASSIGN;
                    new->typf=return_typ->flags&31;
                    new->q1=tree->o;
                    new->q2.flags=0;
                    new->q2.val.vlong=szof(return_typ);
                    if(freturn(return_typ)){
                        new->z.flags=SCRATCH|REG;
                        new->z.reg=freturn(return_typ);
                        if(!regs[new->z.reg]){
                            struct IC *alloc=mymalloc(ICS);
                            alloc->code=ALLOCREG;
                            alloc->q1.flags=REG;
                            alloc->q2.flags=alloc->z.flags=0;
                            alloc->q1.reg=new->z.reg;
                            regs[new->z.reg]=1;
                            add_IC(alloc);
                        }
                    }else{
                        new->z.reg=0;
                        new->z.v=return_var;
                        new->z.flags=SCRATCH|VAR;
                        new->z.val.vlong=l2zl(0L);
                    }
                    add_IC(new);
                    /*  das hier ist nicht sehr schoen, aber wie sonst? */
                    if(new->z.flags&SCRATCH&&regs[new->z.reg]) free_reg(new->z.reg);
#else
                    new=mymalloc(ICS);
                    if(return_var){ /*  Returnwert ueber Zeiger */
                        new->code=ASSIGN;
                        new->z.flags=VAR|DREFOBJ;
                        new->z.val.vlong=l2zl(0L);
                        new->z.v=return_var;
                    }else{
                        new->code=SETRETURN;
                        new->z.reg=freturn(return_typ);
                        new->z.flags=0;
                    }
                    new->typf=return_typ->flags&31;
                    new->q1=tree->o;
                    new->q2.flags=0;
                    new->q2.val.vlong=szof(return_typ);
                    add_IC(new);
#endif
                }
            }
            free_expression(tree);
            killsp();
            if(*s==';') {s++;killsp();} else error(54);
        }else{
            if(return_typ) error(156);
        }
    }else{ s++; if(return_typ) error(156);}

    new=mymalloc(ICS);
    new->code=BRA;
    new->typf=return_label;
    add_IC(new);
    cr();
}

void expression_statement(void)
/*  bearbeitet expression_statement                             */
{
    np tree;
    killsp();
    if(*s==';') {s++;return;}
    if(tree=expression()){
        if(tree->flags==POSTINC) tree->flags=PREINC;
        if(tree->flags==POSTDEC) tree->flags=PREDEC;
        if(type_expression(tree)){
            if(DEBUG&2){pre(stdout,tree);printf("\n");}
            if(tree->sidefx){
                gen_IC(tree,0,0);
                if((tree->o.flags&(SCRATCH|REG))==REG) ierror(0);
                if(tree&&(tree->o.flags&SCRATCH)) free_reg(tree->o.reg);
            }else{error(153);if(DEBUG&2) prd(stdout,tree->ntyp);}
        }
        free_expression(tree);
    }
    killsp();
    if(*s==';') s++; else error(54);
    cr();
}
void compound_statement(void)
/*  bearbeitet compound_statement (block)                       */
{
    killsp();
    if(*s=='{') s++; else error(157);
    killsp();
    while(declaration(0)){
        var_declaration();
        killsp();
    }
    while(*s!='}'){
        statement();
        killsp();
    }
    s++;/*killsp();*/
}
struct llist *add_label(char *identifier)
/*  Fuegt label in Liste                                        */
{
    struct llist *new;
    new=mymalloc(LSIZE);
    new->next=0;new->label=++label;new->flags=0;
    new->identifier=add_identifier(identifier,strlen(identifier));
    if(first_llist==0){
        first_llist=last_llist=new;
    }else{
        last_llist->next=new;
        last_llist=new;
    }
    return(last_llist); /* return(new) sollte aequiv. sein */
}
struct llist *find_label(char *identifier)
/*  Sucht Label, gibt Zeiger auf llist oder 0 beu Fehler zurueck    */
{
    struct llist *p;
    p=first_llist;
    while(p){
        if(!strcmp(p->identifier,identifier)) return(p);
        p=p->next;
    }
    return(0);
}
void free_llist(struct llist *p)
/*  Gibt llist frei                                             */
{
    struct llist *merk;
    while(p){
        merk=p->next;
        if(!(p->flags&LABELDEFINED)) error(147,p->identifier);
        if(!(p->flags&LABELUSED)&&!p->switch_count) error(148,p->identifier);
        free(p);
        p=merk;
    }
}
