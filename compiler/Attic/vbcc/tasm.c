/*  Test-language for vbcc. */

#include "supp.h"

struct Var *fv;

struct Typ tint,mfunc;
struct struct_declaration msd; /* initialized to zero */

void raus(void)
{
    while(fv){
        struct Var *m=fv->next;
        free(fv);
        fv=m;
    }
    while(first_ic){
        struct IC *m=first_ic->next;
        free(first_ic);
        first_ic=m;
    }
    exit(0);
}
void add_IC(struct IC *new)
{
    new->next=0;
    new->prev=last_ic;
    new->change_cnt=new->use_cnt=0;
    new->line=0;
    new->file=0;
    if(!last_ic){
        first_ic=new;
    }else{
        last_ic->next=new;
    }
    last_ic=new;
}
struct Var *add_var(char *name,struct Typ *t,int sc)
{
    struct Var *v=mymalloc(sizeof(*v));
    v->vtyp=t;
    v->storage_class=sc;
    v->reg=0;
    v->identifier=name;
    v->offset=max_offset;
    if(sc==AUTO) max_offset=zladd(max_offset,sizetab[t->flags&NQ]);
    v->priority=1;
    v->flags=0;
    v->next=fv;
    v->clist=0;
    v->fi=0;
    v->inline_copy=0;
    fv=v;
    return v;
}
struct Var *add_tmp_var(struct Typ *t)
{
    return add_var(empty,t,AUTO);
}
struct Var *get_var(char *name)
{
    struct Var *v;char *buf;
    for(v=fv;v;v=v->next){
        if(!strcmp(name,v->identifier)) return v;
    }
    buf=mymalloc(strlen(name)+1);
    strcpy(buf,name);
    return add_var(buf,&tint,AUTO);
}

void read_ics()
{
    char s[400],q1[100],q2[100],z[100],op;
    struct IC *new;
    gets(s);
    while(sscanf(s,"%99s = %99s %c %99s",z,q1,&op,q2)==4){
        new=mymalloc(ICS);
        switch(op){
            case '+': new->code=ADD;break;
            case '*': new->code=MULT;break;
            case '-': new->code=SUB;break;
            case '/': new->code=DIV;break;
            default: return;
        }
        new->typf=INT;
        new->q1.flags=new->q2.flags=new->z.flags=VAR;
        new->q1.am=new->q2.am=new->z.am=0;
        new->q1.val.vlong=l2zl(0L);
        new->q2.val.vlong=l2zl(0L);
        new->z.val.vlong=l2zl(0L);
        new->q1.v=get_var(q1);
        new->q2.v=get_var(q2);
        new->z.v=get_var(z);
        add_IC(new);
        gets(s);
    }
}
void error(int n,...)
{
    printf("error %d\n",n);
    raus();
}
void savescratch()
{}

main()
{
    struct Var *main;
    max_offset=l2zl(0L);
    if(!init_cg()) raus();
    tint.flags=INT;
    tint.next=0;
    mfunc.flags=FUNKT;
    mfunc.next=&tint;
    mfunc.exact=&msd;
    main=add_var("main",&mfunc,EXTERN);
    read_ics();
    printf("optflags: ");
    scanf("%ld",&optflags);
    pric(stdout,first_ic);
    vl1=vl3=0;
    vl2=fv;
    optimize(optflags,main);
    pric(stdout,first_ic);
    gen_code(stdout,first_ic,main,max_offset);
    raus();
}
