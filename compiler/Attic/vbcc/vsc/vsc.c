/* vsc - portable instruction scheduler for vbcc.   */
/* (c) in 1997 by Volker Barthelmann.               */

#include "vsc.h"

char vs_copyright[]="vsc scheduler V0.0 (c) in 1997 by Volker Barthelmann";

#define MAX_INS 128
#define LINELENGTH 1024

#ifndef DEBUG
int DEBUG;
#endif

struct sinfo silist[MAX_INS];
int si_count;
int done;

int bvcmp(unsigned char *dest,unsigned char *src,size_t len)
/*  vergleicht zwei Bitvektoren    */
{
    for(;len>0;len--)
        if(*dest++!=*src++) return(0);
    return(1);
}
void bvunite(unsigned char *dest,unsigned char *src,size_t len)
/*  berechnet Vereinigung zweier Bitvektoren    */
{
    for(;len>0;len--)
        *dest++|=*src++;
}
void bvintersect(unsigned char *dest,unsigned char *src,size_t len)
/*  berechnet Durchschnitt zweier Bitvektoren   */
{
    for(;len>0;len--)
        *dest++&=*src++;
}
void bvdiff(unsigned char *dest,unsigned char *src,size_t len)
/*  berechnet 'Differenz' zweier Bitvektoren    */
{
    for(;len>0;len--)
        *dest++&=~(*src++);
}

void raus(void)
{
  sched_cleanup();
  if(DEBUG&1) printf("raus\n");
  if(done)
    exit(EXIT_SUCCESS);
  else 
    exit(EXIT_FAILURE);
}
void *mymalloc(size_t sz)
{
  void *p=malloc(sz);
  if(!p){
    puts("Out of memory!");
    raus();
  }
  return p;
}
void print_sinfo(void)
{
  struct sinfo *p;int i,j;
  if(DEBUG&2)
    printf("print_sinfo\n");
  else
    return;
  for(j=0;j<si_count;j++){
    p=&silist[j];
    printf("inst: %s",p->txt);
    printf("flags=%u, label=%u latency=%u\n",p->flags,p->label,p->latency);
    printf("available pipelines: ");
    for(i=0;i<PIPES;i++)
      if(BTST(p->pipes,i)) 
	printf("%d ",i);
    printf("\nuses registers (%d=mem): ",MEM);
    for(i=0;i<=MEM;i++)
      if(BTST(p->uses,i)) 
	printf("%d ",i);
    printf("\nmodifies registers (%d=MEM): ",MEM);
    for(i=0;i<=MEM;i++)
      if(BTST(p->modifies,i))
	printf("%d ",i);
    printf("\n\n");
  }
}
void free_sinfo(void)
{
  int j;
  for(j=0;j<si_count;j++)
    free(silist[j].txt);
}
void output_scheduled(FILE *f)
{
  int pipes[PIPES],i,j,k,l,remaining;
  int latency[MEM+1]={0},cycle=0;
  unsigned char used[REGS_SIZE],modified[REGS_SIZE],tmp[REGS_SIZE],empty[REGS_SIZE]={0};
  remaining=si_count;
  if(DEBUG&2) print_sinfo();
  while(remaining){
    cycle++;
    if(DEBUG&4){
      printf("cycle %d\n",cycle);
      for(i=0;i<=MEM;i++) 
	if(latency[i]>0) printf("latency[%d]=%d\n",i,latency[i]);
    }
    /* Mark all instructions which are ready. */
    memset(modified,0,REGS_SIZE);     
    memset(used,0,REGS_SIZE);
    for(i=0;i<si_count;i++){
      if(silist[i].flags&OUT) continue;
      memcpy(tmp,silist[i].uses,REGS_SIZE);
      bvunite(tmp,silist[i].modifies,REGS_SIZE);
      bvintersect(tmp,modified,REGS_SIZE);
      if(bvcmp(tmp,empty,REGS_SIZE)){
	/* uses and modifies nothing that has to be modified before */
	memcpy(tmp,used,REGS_SIZE);
	bvintersect(tmp,silist[i].modifies,REGS_SIZE);
	if(bvcmp(tmp,empty,REGS_SIZE)){
	  /* modifies nothing that has to be used before */
	  for(k=0,j=0;j<=MEM;j++){
	    if(BTST(silist[i].uses,j)&&latency[j]>0) k=1;
	  }
	  if(!k) silist[i].flags|=READY;
	}
      }
      bvunite(modified,silist[i].modifies,REGS_SIZE);
      bvunite(used,silist[i].uses,REGS_SIZE);
    }
    
    /* Fill pipeline slots with ready instructions. */
    for(i=0;i<PIPES;i++) pipes[i]=-1;
    for(i=0;i<si_count;i++){
      if(silist[i].flags&OUT) continue;
      if(!(silist[i].flags&READY)) continue;
      if(DEBUG&4) printf("inst ready: %s",silist[i].txt);
      k=0;
      /* Is there a free slot? */
      for(j=0;!k&&j<PIPES;j++){
	if(pipes[j]<0&&BTST(silist[i].pipes,j)){
	  pipes[j]=i;
	  k=1;
	}
      }
      /* Replace a scheduled instruction with smaller latency. */
      for(j=0;!k&&j<PIPES;j++){
	if(BTST(silist[i].pipes,j)&&silist[i].latency>silist[pipes[j]].latency){
	  pipes[j]=i;
	  k=1;
	}
      }
    }
    if(DEBUG&4) printf("instructions for cycle %d:\n",cycle);
    for(i=0;i<PIPES;i++){
      if(pipes[i]>=0){
	if(DEBUG&4) printf("%3d: %s",i,silist[pipes[i]].txt);
	fprintf(f,"%s",silist[pipes[i]].txt);
	silist[pipes[i]].flags|=OUT;
	remaining--;
	for(j=0;j<=MEM;j++){
	  if(BTST(silist[pipes[i]].modifies,j)) latency[j]=silist[pipes[i]].latency;
	}
      }
    }
    for(i=0;i<=MEM;i++)
      if(latency[i]>0) latency[i]--;
  }
}
int main(int argc,char *argv[])
{
  char s[LINELENGTH];int i,quiet=0;
  FILE *in,*out; char *inname=0,*outname=0;
  for(i=1;i<argc;i++){
    if(*argv[i]!='-'){
      if(!inname){
	inname=argv[i];
      }else{
	if(outname){
	  printf("Only one input file allowed\n");
	  raus();
	}else{
	  outname=argv[i];
	}
      }
    }else{
      if(!strcmp("-quiet",argv[i])) {quiet=1;continue;}
#ifndef DEBUG
      if(!strncmp("-debug=",argv[i],7)){DEBUG=atoi(argv[i]+7);continue;}
#endif
      printf("Unknown option \"%s\"\n",argv[i]);
      raus();
    }
  }
  if(!sched_init()){printf("sched_init failed\n");raus();}
  if(!quiet) printf("%s\n%s\n",vs_copyright,tg_copyright);
  if(!inname){printf("No input file\n");raus();}
  if(!outname){printf("No output file\n");raus();}
  in=fopen(inname,"r");
  if(!in){printf("Could not open input file \"%s\"\n",inname);raus();}
  out=fopen(outname,"w");
  if(!out){printf("Could not open output file \"%s\"\n");raus();}
  while(fgets(s,LINELENGTH-1,in)){
    memset(&silist[si_count],0,sizeof(silist[0]));
    silist[si_count].txt=mymalloc(strlen(s)+1);
    strcpy(silist[si_count].txt,s);
    if(!sched_info(&silist[si_count])){
      printf("Do not understand instruction:\n%s\n",silist[si_count].txt);
      raus();
    }
    if(silist[si_count].flags&(BARRIER|LABEL|COND_BRANCH|UNCOND_BRANCH)){
      if(DEBUG&1) printf("Barrier: %s",silist[si_count].txt);
      output_scheduled(out);
      free_sinfo();
      fprintf(out,silist[si_count].txt);
      si_count=0;
    }else{
      if(++si_count>=MAX_INS){
	if(DEBUG&1) printf("MAX_INS reached: %s",silist[si_count].txt);
	output_scheduled(out);
	free_sinfo();
	si_count=0;
      }
    }
  }
  output_scheduled(out);
  free_sinfo();
  done=1;
  raus();
}

    
