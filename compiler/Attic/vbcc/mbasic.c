/*  Test-language for vbcc. */

#include "supp.h"

#include <ctype.h>
#include <stdio.h>

struct Var *fv;

struct Typ tint,mfunc;
struct struct_declaration msd; /* initialized to zero */

FILE *file;
char *next;

struct obj expression(),factor(),scalar();

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
    new->q1.am=new->q2.am=new->z.am=0;
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
    v->nesting=1;
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
    puts("getvar");
    for(v=fv;v;v=v->next){
        if(!strcmp(name,v->identifier)) return v;
    }
    buf=mymalloc(strlen(name)+1);
    strcpy(buf,name);
    return add_var(buf,&tint,AUTO);
}

char *identifier(void)
{
  static char id[1024];
  char *s=id;
  puts("identifier");
  while(isalnum(*next)) *s++=*next++;
  puts("done");
  return id;
}
struct obj scalar(void)
{
  struct obj o;
  zlong val;
  puts("scalar");
  if(isdigit(*next)){
    o.flags=KONST;
    val=l2zl(0L);
    while(isdigit(*next)){
      val=zlmult(val,l2zl(10L));
      val=zladd(val,l2zl((long)(*next-'0')));
      next++;
    }
    o.val.vint=zl2zi(val);
    return o;
  }
  if(*next=='('){
    next++;
    o=expression();
    next++;
    return o;
  }
  o.flags=VAR;
  o.val.vlong=l2zl(0L);
  o.v=get_var(identifier());
  return o;
}
struct obj factor(void)
{
  struct obj o;
  struct IC *new;
  puts("factor");
  o=scalar();
  while(*next=='*'||*next=='/'){
    new=mymalloc(ICS);
    if(*next=='*') new->code=MULT; else new->code=DIV;
    next++;
    new->typf=INT;
    new->q1=o;
    new->q2=scalar();
    o.flags=VAR;
    o.v=add_tmp_var(&tint);
    o.val.vlong=l2zl(0L);
    new->z=o;
    add_IC(new);
  }
  return o;
}
struct obj expression(void)
{
  struct obj o;
  struct IC *new;
  puts("expression");
  o=factor();
  while(*next=='+'||*next=='-'){
    new=mymalloc(ICS);
    if(*next=='+') new->code=ADD; else new->code=SUB;
    next++;
    new->typf=INT;
    new->q1=o;
    new->q2=factor();
    o.flags=VAR;
    o.v=add_tmp_var(&tint);
    o.val.vlong=l2zl(0L);
    new->z=o;
    add_IC(new);
  }
  return o;
}
void compile(void)
{
  struct IC *new;
  char line[1024],*s;
  struct obj o,last;
  puts("compile");
  while(fgets(line,1023,file)){
    next=line;
    s=identifier();
    if(*next=='='){
      struct Var *v=get_var(s);
      next++;
      o=expression();
      new=mymalloc(ICS);
      new->code=ASSIGN;
      new->typf=INT;
      new->q1=o;
      new->z.flags=VAR;
      new->z.v=v;
      new->z.val.vlong=l2zl(0L);
      new->q2.flags=0;
      new->q2.val.vlong=sizetab[INT];
      last=new->z;
      add_IC(new);
      continue;
    }
  }
  new=mymalloc(ICS);
  new->code=SETRETURN;
  new->typf=INT;
  new->q1=last;
  new->q2.flags=new->z.flags=0;
  new->q2.val.vlong=sizetab[INT];
  new->z.reg=freturn(&tint);
  if(!new->z.reg) puts("problem!");
  add_IC(new);
}
void error(int n,...)
{
    printf("error %d\n",n);
    raus();
}
void savescratch()
{}

main(int argc,char **argv)
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
    file=fopen(argv[1],"r");
    if(!file) {printf("Error opening file\n");raus();}
    compile();
    scanf("%ld",&optflags);
    pric(stdout,first_ic);
    vl1=vl3=0;
    vl2=fv;
    optimize(optflags,main);
    pric(stdout,first_ic);
    gen_code(stdout,first_ic,main,max_offset);
    raus();
}

