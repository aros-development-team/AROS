#include "vsc.h"

char tg_copyright[]="Alpha scheduler V0.0 (c) in 1997 by Volker Barthelmann";

int sched_init(void)
{
  return 1;
}
void sched_cleanup(void)
{
}
int sched_info(struct sinfo *p)
{
  char buf[20];int q1,q2,z,i;
  if(sscanf(p->txt,"$C%d:",&i)==1){
    p->label=i;
    p->flags=LABEL;
    return 1;
  }
  /* lda $r1,imm($r2) */
  if(sscanf(p->txt,"lda $%d,%d($%d)",&z,&i,&q1)==3){
    p->latency=1;
    BSET(p->pipes,0);
    BSET(p->pipes,1);
    BSET(p->uses,q1);
    BSET(p->modifies,z);
    return 1;
  }
  /* lda $r1,... */
  if(sscanf(p->txt,"lda $%d,",&z)==1){
    BSET(p->pipes,0);
    BSET(p->pipes,1);
    BSET(p->modifies,z);
    return 1;
  }
  /* op $r1,$r2,$r3 */
  if(sscanf(p->txt,"%19s $%d,$%d,$%d",buf,&q1,&q2,&z)==4){
    p->latency=1;
    BSET(p->pipes,0);
    BSET(p->pipes,1);
    BSET(p->uses,q1);
    BSET(p->uses,q2);
    BSET(p->modifies,z);
    return 1;
  }
  /* op $r1,$r2 */
  if(sscanf(p->txt,"%19s $%d,$%d",buf,&q1,&z)==3){
    p->latency=1;
    BSET(p->pipes,0);
    BSET(p->pipes,1);
    BSET(p->uses,q1);
    BSET(p->modifies,z);
    return 1;
  }
  /* op $r1,imm,$r2 */
  if(sscanf(p->txt,"%19s $%d,%d,$%d",buf,&q1,&i,&z)==4){
    p->latency=1;
    BSET(p->pipes,0);
    BSET(p->pipes,1);
    BSET(p->uses,q1);
    BSET(p->modifies,z);
    return 1;
  }
  /* op $fr1,$fr2,$fr3 */
  if(sscanf(p->txt,"%19s $f%d,$f%d,$f%d",buf,&q1,&q2,&z)==4){
    p->latency=1;
    BSET(p->pipes,2);
    BSET(p->pipes,3);
    BSET(p->uses,q1+32);
    BSET(p->uses,q2+32);
    BSET(p->modifies,z+32);
    return 1;
  }
  /* load/store $r1,c($r2) */
  if(sscanf(p->txt,"%19s $%d,%d($%d)",buf,&z,&i,&q1)==4){
    p->latency=3;
    BSET(p->pipes,0);
    BSET(p->uses,q1);
    if(*buf=='l'){
      BSET(p->pipes,1);
      BSET(p->modifies,z);
      BSET(p->uses,MEM);
    }else{
      BSET(p->uses,z);
      BSET(p->modifies,MEM);
    }
    return 1;
  }
  /* load/store $fr1,c($r2) */
  if(sscanf(p->txt,"%19s $f%d,%d($%d)",buf,&z,&i,&q1)==4){
    p->latency=3;
    BSET(p->pipes,0);
    BSET(p->uses,q1);
    if(*buf=='l'){
      BSET(p->pipes,1);
      BSET(p->modifies,z+32);
      BSET(p->uses,MEM);
    }else{
      BSET(p->uses,z+32);
      BSET(p->modifies,MEM);
    }
    return 1;
  }
  p->flags=BARRIER;
  return 1;
}
