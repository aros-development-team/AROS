#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct dtlist {char *spec,*descr;} dts[]={
#include "datatypes.h"
};

struct dtconvlist {char *from,*to,*filef,*filet;int size;} cnvs[]={
#include "dtconv.h"
};

int dtcnt=sizeof(dts)/sizeof(dts[0]);
int cnvcnt=sizeof(cnvs)/sizeof(cnvs[0]);

char *have;

#define CHAR 1
#define UCHAR 2
#define SHORT 3 
#define USHORT 4
#define INT 5
#define UINT 6
#define LONG 7
#define ULONG 8
#define FLOAT 9
#define DOUBLE 10
#define POINTER 11

#define TYPECNT POINTER

char *typen[TYPECNT+1]={"error","char","uchar","short","ushort","int","uint",
		        "long","ulong","float","double","pointer"};
char *ftypen[TYPECNT+1]={"error","char","unsigned char","short","unsigned short",
                                 "int","unsigned int","long","unsigned long",
                                 "float","double","char *"};

int dt[TYPECNT+1],cnv[TYPECNT+1];
char *nt[TYPECNT+1];
FILE *fin,*cout,*hout;
int crosscompiler;

void *mymalloc(size_t size)
{
  void *p=malloc(size);
  if(!p){
    printf("Out of memory!\n");
    exit(EXIT_FAILURE);
  }
  return p;
}

int askyn(void)
{
  char in[8];
  do{
    printf("Type y or n: ");
    fflush(stdout);
    fgets(in,127,stdin);
  }while(*in!='y'&&*in!='n');
  return *in=='y';
}

char *asktype(void)
{
  char *in=mymalloc(128);
  printf("Enter that type: ");
  fflush(stdout);
  fgets(in,127,stdin);
  if(in[strlen(in)-1]=='\n') in[strlen(in)-1]=0;
  return in;
}

int tst(int type,char *spec)
{
  int i,j;
  for(i=0;i<dtcnt;i++){
    if(strstr(spec,dts[i].spec)){ 
      if(have[i]==-2) continue;
      if(have[i]>=0){
/* 	printf("auto: %s == %s\n",dts[i].spec,nt[have[i]]); */
	dt[type]=i;
	nt[type]=nt[have[i]];
	cnv[type]=-1;
	return 1;
      }else{
	printf("Does your system/compiler support a type implemented as\n%s?\n",dts[i].descr);
	if(askyn()){
	  dt[type]=i;
	  nt[type]=asktype();
	  have[i]=type;
	  cnv[type]=-1;
	  return 1;
	}else{
	  have[i]=-2;
	}
      }
    }
  }
  for(j=0;j<cnvcnt;j++){
    char *s=0;
    if(strstr(spec,cnvs[j].from)) s=cnvs[j].to;
/*     if(strstr(spec,cnvs[j].to)) s=cnvs[j].from; */
    if(s){
      for(i=0;i<dtcnt;i++){
	if(!strcmp(s,dts[i].spec)){
	  if(have[i]==-2) continue;
	  if(have[i]>=0){
	    dt[type]=i;
	    nt[type]=nt[have[i]];
	    cnv[type]=j;
	    return 2;
	  }else{
	    printf("Does your system/compiler support a type implemented as\n%s?\n",dts[i].descr);
	    if(askyn()){
	      dt[type]=i;
	      nt[type]=asktype();
	      have[i]=type;
	      cnv[type]=j;
	      return 2;
	    }else{
	      have[i]=-2;
	    }
	  }
	}
      }
    }
  }
  return 0;
}
    
char *castfrom(int type)
{
  if(cnv[type]>=0){
    char *s=mymalloc(16);
    sprintf(s,"dtcnv%df",type);
    return s;
  }else{
    return "";
  }
}
char *castto(int type)
{
  if(cnv[type]>=0){
    char *s=mymalloc(16);
    sprintf(s,"dtcnv%dt",type);
    return s;
  }else{
    return "";
  }
} 
void gen_cast(char *name,int from,int to)
{
  fprintf(hout,"#define %s(x) %s((%s)%s(x))\n",name,castto(to),nt[to],castfrom(from));
}
void gen_2op(char *name,char *op,int type)
{
  fprintf(hout,"#define %s(a,b) %s(%s(a)%s%s(b))\n",name,castto(type),castfrom(type),op,castfrom(type));
} 
void gen_1op(char *name,char *op,int type)
{
  fprintf(hout,"#define %s(a) %s(%s%s(a))\n",name,castto(type),op,castfrom(type));
} 
void gen_cmp(char *name,char *op,int type)
{
  fprintf(hout,"#define %s(a,b) (%s(a)%s%s(b))\n",name,castfrom(type),op,castfrom(type));
} 
main(int argc,char **argv)
{
  char type[128],spec[128];
  int i,r;
  if(argc!=4){ printf("Usage: dtgen <config-file> <output-file.h> <output-file.c>\n");exit(EXIT_FAILURE);}
/*   printf("%d datatypes, %d conversions\n",dtcnt,cnvcnt); */
  have=mymalloc(dtcnt*sizeof(*have));
  memset(have,-1,sizeof(*have)*dtcnt);
  fin=fopen(argv[1],"r");
  if(!fin){ printf("Could not open <%s> for input!\n",argv[1]);exit(EXIT_FAILURE);}
  hout=fopen(argv[2],"w");
  if(!hout){ printf("Could not open <%s> for output!\n",argv[2]);exit(EXIT_FAILURE);}
  cout=fopen(argv[3],"w");
  if(!hout){ printf("Could not open <%s> for output!\n",argv[3]);exit(EXIT_FAILURE);}
  printf("Are you building a cross-compiler?\n");
  crosscompiler=askyn();
  for(i=1;i<=TYPECNT;i++){
    fgets(spec,127,fin);
/*     printf("Specs for z%s:\n%s\n",typen[i],spec); */
    if(!crosscompiler){
      dt[i]=i;
      nt[i]=ftypen[i];
      have[i]=i;
      cnv[i]=-1;
    }else{
      if(!tst(i,spec)){
	printf("Problem! Your system does not seem to provide all of the data types\n"
	       "this version of vbcc needs.\nWrite to volker@vb.franken.de!\n");
	exit(EXIT_FAILURE);
      }
    }
  }
  fprintf(hout,"\n\n/* Machine generated file. DON'T TOUCH ME! */\n\n\n");
  fprintf(cout,"\n\n/* Machine generated file. DON'T TOUCH ME! */\n\n\n");
  fprintf(cout,"#include \"dt.h\"\n\n");
  for(i=1;i<=TYPECNT;i++){
    if(cnv[i]>=0){
      fprintf(hout,"typedef struct {char a[%d];} dt%df;\n",cnvs[cnv[i]].size,i);
      fprintf(hout,"typedef dt%df z%s;\n",i,typen[i]);
      fprintf(hout,"typedef %s dt%dt;\n",nt[i],i);
      fprintf(hout,"dt%dt dtcnv%df(dt%df);\n",i,i,i);
      fprintf(hout,"dt%df dtcnv%dt(dt%dt);\n",i,i,i);
      fprintf(cout,"#undef DTTTYPE\n#define DTTTYPE dt%dt\n",i);
      fprintf(cout,"#undef DTFTYPE\n#define DTFTYPE dt%df\n",i);
      fprintf(cout,"dt%dt dtcnv%df(dt%df\n",i,i,i);
      fprintf(cout,"#include \"%s\"\n",cnvs[cnv[i]].filef);
      fprintf(cout,"dt%df dtcnv%dt(dt%dt\n",i,i,i);
      fprintf(cout,"#include \"%s\"\n",cnvs[cnv[i]].filet);
    }else{
      fprintf(hout,"typedef %s z%s;\n",nt[i],typen[i]);
    }
  }

  gen_cast("zc2zl",CHAR,LONG);
  gen_cast("zs2zl",SHORT,LONG);
  gen_cast("zi2zl",INT,LONG);
  gen_cast("zl2zc",LONG,CHAR);
  gen_cast("zl2zs",LONG,SHORT);
  gen_cast("zl2zi",LONG,INT);
  gen_cast("zuc2zul",UCHAR,ULONG);
  gen_cast("zus2zul",USHORT,ULONG);
  gen_cast("zui2zul",UINT,ULONG);
  gen_cast("zul2zuc",ULONG,UCHAR);
  gen_cast("zul2zus",ULONG,USHORT);
  gen_cast("zul2zui",ULONG,UINT);
  gen_cast("zul2zl",ULONG,LONG);
  gen_cast("zl2zul",LONG,ULONG);
  gen_cast("zf2zd",FLOAT,DOUBLE);
  gen_cast("zd2zf",DOUBLE,FLOAT);
  gen_cast("zd2zl",DOUBLE,LONG);
  gen_cast("zl2zd",LONG,DOUBLE);
  gen_cast("zd2zul",DOUBLE,ULONG);
  gen_cast("zul2zd",ULONG,DOUBLE);
  gen_cast("zp2zul",POINTER,ULONG);
  gen_cast("zul2zp",ULONG,POINTER);
 
  fprintf(hout,"#define l2zl(x) %s((%s)(x))\n",castto(LONG),nt[LONG]);
  fprintf(hout,"#define ul2zul(x) %s((%s)(x))\n",castto(ULONG),nt[ULONG]);
  fprintf(hout,"#define d2zd(x) %s((%s)(x))\n",castto(DOUBLE),nt[DOUBLE]);
  fprintf(hout,"#define zl2l(x) ((long)%s(x))\n",castfrom(LONG));
  fprintf(hout,"#define zul2ul(x) ((unsigned long)%s(x))\n",castfrom(ULONG));
  fprintf(hout,"#define zd2d(x) ((double)%s(x))\n",castfrom(DOUBLE));

  gen_2op("zladd","+",LONG);
  gen_2op("zuladd","+",ULONG);
  gen_2op("zdadd","+",DOUBLE);
  gen_2op("zlsub","-",LONG);
  gen_2op("zulsub","-",ULONG);
  gen_2op("zdsub","-",DOUBLE);
  gen_2op("zlmult","*",LONG);
  gen_2op("zulmult","*",ULONG);
  gen_2op("zdmult","*",DOUBLE);
  gen_2op("zldiv","/",LONG);
  gen_2op("zuldiv","/",ULONG);
  gen_2op("zddiv","/",DOUBLE);
  gen_2op("zlmod","%",LONG);
  gen_2op("zulmod","%",ULONG);
  gen_2op("zllshift","<<",LONG);
  gen_2op("zullshift","<<",ULONG);
  gen_2op("zlrshift",">>",LONG);
  gen_2op("zulrshift",">>",ULONG);
  gen_2op("zland","&",LONG);
  gen_2op("zuland","&",ULONG);
  gen_2op("zlor","|",LONG);
  gen_2op("zulor","|",ULONG);
  gen_2op("zlxor","^",LONG);
  gen_2op("zulxor","^",ULONG);
  gen_2op("zlmod","%",LONG);
  gen_2op("zulmod","%",ULONG);

  gen_1op("zlkompl","~",LONG);
  gen_1op("zulkompl","~",ULONG);

  gen_cmp("zlleq","<=",LONG);
  gen_cmp("zulleq","<=",ULONG);
  gen_cmp("zdleq","<=",DOUBLE);
  gen_cmp("zpleq","<=",POINTER);
  gen_cmp("zleqto","==",LONG);
  gen_cmp("zuleqto","==",ULONG);
  gen_cmp("zdeqto","==",DOUBLE);
  gen_cmp("zpeqto","==",POINTER);


  fclose(fin);
  fclose(hout);
  fclose(cout);
  free(have);  
  return 0;
}






