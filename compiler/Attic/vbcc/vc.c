/*  Frontend for vbcc                       */
/*  (c) in 1995-96 by Volker Barthelmann    */
/*  #define AMIGA for Amiga-Version         */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef AMIGA
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <exec/libraries.h>
#include <clib/dos_protos.h>

extern struct Library *DOSBase;

/*  Must be 32bit-aligned - I know it will be if compiled with vbcc.    */
struct FileInfoBlock fib;
#endif

struct NameList{
    struct NameList *next;
    char *obj;
} *first_obj=0,*last_obj=0,*first_scratch=0,*last_scratch=0;

/*  Limit fuer Laenge der Namen (wegen Wildcards)   */
#define NAMEBUF 1000    /*  MUST BE >= ~TMPNAM_L+7  */
#define USERLIBS 1000

/*  Ab dieser Laenge werden Objektfiles nicht direkt uebergeben,    */
/*  sondern aus einem File an den Linker uebergeben                 */
#ifdef AMIGA
#define MAXCLEN 500
#else
#define MAXCLEN 32000
#endif

#define NOTMPFILE 2048
#define OUTPUTSET 1024
#define NOSTDLIB 512
#define VERBOSE 256
#define VERYVERBOSE 128
#define KEEPSCRATCH 64

#define PPSRC 1
#define CCSRC 2
#define ASSRC 3
#define OBJ 4

char empty[]="";
/*  Namen der einzelnen Phasen  */
char *ppname=empty,*ccname=empty,*asname=empty,*ldname=empty,*l2name=empty,*rmname=empty;
/*  dasselbe fuer VERBOSE   */
char *ppv=empty,*ccv=empty,*asv=empty,*ldv=empty,*l2v=empty,*rmv=empty;

#ifdef AMIGA
const char *config_names[]={"vc.config","ENV:vc.config","VBCC:vc.config"};
#else
const char *config_names[]={"vc.config","/etc/vc.config"};
#endif

/*  String fuer die Default libraries   */
char userlibs[USERLIBS];
char *nomem="Not enough memory!\n";

char *destname="a.out";
char namebuf[NAMEBUF+1],namebuf2[NAMEBUF+1];
char oldfile[NAMEBUF+2];

char *config;
char **confp;

int typ(char *);
char *add_suffix(char *,char *);
void raus(int);

char *command,*options,*linkcmd,*objects,*libs,*ppopts;
#ifdef AMIGA
struct AnchorPath *ap;
#endif

int linklen=10,flags=0;

void free_namelist(struct NameList *p)
{
    struct NameList *m;
    while(p){
        m=p->next;
        if(flags&VERYVERBOSE){
            puts("free p->obj");
            if(!p->obj) puts("IS ZERO!!"); else puts(p->obj);
        }
        free((void *)p->obj);
        if(flags&VERYVERBOSE){puts("free p"); if(!p) puts("IS ZERO!!");}
        free((void *)p);
        p=m;
    }
}
void del_scratch(struct NameList *p)
{
    while(p){
        sprintf(command,rmname,p->obj);
        if(flags&VERBOSE) printf("%s\n",command);
        if(system(command)){printf("%s failed\n",command);raus(20);}
        p=p->next;
    }
}
void raus(int rc)
{
    if(confp)   free(confp);
    if(config)  free(config);
    if(objects) free(objects);
    if(libs)    free(libs);
    if(command) free(command);
    if(ppopts)  free(ppopts);
    if(options) free(options);
    if(linkcmd) free(linkcmd);
#ifdef AMIGA
    if(ap) free(ap);
#endif
    free_namelist(first_obj);
    free_namelist(first_scratch);
    exit(rc);
}
void add_name(char *obj,struct NameList **first,struct NameList **last)
{
    struct NameList *new;
    if(flags&VERYVERBOSE) printf("add_name: %s\n",obj);
    if(!(new=(struct NameList *)malloc(sizeof(struct NameList))))
        {printf(nomem);raus(20);}
    if(!(new->obj=(char *)malloc(strlen(obj)+1)))
        {free((void *)new);printf(nomem);raus(20);}
    if(first==&first_obj) linklen+=strlen(obj)+1;
    strcpy(new->obj,obj);
    new->next=0;
    if(!*first){
        *first=*last=new;
    }else{
        (*last)->next=new;*last=new;
    }
}
int read_config(void)
{
    int i,count; long size;
    char *p;
#ifdef AMIGA
    BPTR file;
    for(i=0;i<sizeof(config_names)/sizeof(config_names[0]);i++){
        file=Lock((STRPTR)config_names[i],-2);
        if(file) break;
    }
    if(!file) {puts("No config file!");raus(EXIT_FAILURE);}
    if(!Examine(file,&fib)){puts("Examine() failed!");raus(EXIT_FAILURE);}
    size=fib.fib_Size;
    config=malloc(size);
    if(!config){printf(nomem);raus(EXIT_FAILURE);}
    UnLock(file);
    file=Open((STRPTR)config_names[i],MODE_OLDFILE);
    if(!file) return(0);
    size=Read(file,config,size);
    Close(file);
#else
    FILE *file;
    for(i=0;i<sizeof(config_names)/sizeof(config_names[0]);i++){
        file=fopen(config_names[i],"rb");
        if(file) break;
    }
    if(!file) {puts("No config file!");raus(EXIT_FAILURE);}
    if(fseek(file,0,SEEK_END)) return(0);
    size=ftell(file);   /*  noch errorcheck */
    if(fseek(file,0,SEEK_SET)) return(0);
    config=malloc(size);
    if(!config){printf(nomem);raus(EXIT_FAILURE);}
    fread(config,1,size,file);
    fclose(file);
#endif
    count=0;p=config;
    while(*p&&p<config+size){
        count++;
        while(*p!='\n'&&p<config+size) p++;
        if(*p=='\n') *p++=0;
    }
    confp=malloc(count*sizeof(char *));
    for(p=config,i=0;i<count;i++){
        confp[i]=p;
        while(*p) p++;
        p++;
    }
    return(count);
}

int main(int argc,char *argv[])
{
    int tfl,i,opt=1,len=10,pm,count;char *parm;
    if(argc>=2&&argv[1][0]=='+'){
        config_names[0]=argv[1]+1;argv[1][0]=0;
    }
    count=read_config();
#ifdef AMIGA
    if(pm=DOSBase->lib_Version>=36){
        if(ap=(struct AnchorPath *)calloc(sizeof(struct AnchorPath)+NAMEBUF,1))
            {ap->ap_Strlen=NAMEBUF;ap->ap_BreakBits=0;} else pm=0;
    }
#endif
    for(i=1;i<argc+count;i++){
        if(i<argc) parm=argv[i]; else parm=confp[i-argc];
/*        printf("Parameter %d=%s\n",i,parm);*/
        if(!strncmp(parm,"-pp=",4)){ppname=parm+4;*parm=0;}
        if(!strncmp(parm,"-cc=",4)){ccname=parm+4;*parm=0;}
        if(!strncmp(parm,"-as=",4)){asname=parm+4;*parm=0;}
        if(!strncmp(parm,"-ld=",4)){ldname=parm+4;*parm=0;}
        if(!strncmp(parm,"-l2=",4)){l2name=parm+4;*parm=0;}
        if(!strncmp(parm,"-rm=",4)){rmname=parm+4;*parm=0;}
        if(!strncmp(parm,"-ppv=",5)){ppv=parm+5;*parm=0;}
        if(!strncmp(parm,"-ccv=",5)){ccv=parm+5;*parm=0;}
        if(!strncmp(parm,"-asv=",5)){asv=parm+5;*parm=0;}
        if(!strncmp(parm,"-ldv=",5)){ldv=parm+5;*parm=0;}
        if(!strncmp(parm,"-l2v=",5)){l2v=parm+5;*parm=0;}
        if(!strncmp(parm,"-rmv=",5)){rmv=parm+5;*parm=0;}
        if(!strcmp(parm,"-notmpfile")) {flags|=NOTMPFILE;*parm=0;}
        if(!strcmp(parm,"-E")) {flags|=CCSRC;*parm=0;}
        if(!strcmp(parm,"-S")) {flags|=ASSRC;*parm=0;}
        if(!strcmp(parm,"-c")) {flags|=OBJ;*parm=0;}
        if(!strcmp(parm,"-v")) {flags|=VERBOSE;*parm=0;}
        if(!strcmp(parm,"-k")) {flags|=KEEPSCRATCH;*parm=0;}
        if(!strcmp(parm,"-vv")) {flags|=VERBOSE|VERYVERBOSE;*parm=0;}
        if(!strcmp(parm,"-nostdlib")) {flags|=NOSTDLIB;*parm=0;}
        if(!strncmp(parm,"-O",2)){
            if(parm[2]=='0') opt=0;
            if(parm[2]=='1'||parm[2]==0) opt=991;
            if(parm[2]=='2') opt=1023;
            if(parm[2]>='3') opt=~0;
            if(parm[2]=='=') opt=atoi(&parm[3]);
            *parm=0;
        }
        if(!strcmp(parm,"-o")&&i<argc-1) {
            *argv[i++]=0;destname=argv[i];
            flags|=OUTPUTSET;argv[i]="";continue;
        }
        if(!strncmp(parm,"-o=",3)){
            destname=parm+3;
            flags|=OUTPUTSET;*parm=0;continue;
        }
        if(parm[0]=='-'&&parm[1]=='l'){
            if((strlen(userlibs)+strlen(parm)+15)>=USERLIBS){puts("Userlibs too long");exit(20);}
#ifdef AMIGA
            strcat(userlibs," vlib:");
            strcat(userlibs,parm+2);
            strcat(userlibs,".lib");
#else
            strcat(userlibs," ");
            strcat(userlibs,parm);
#endif
            *parm=0;continue;
        }
        len+=strlen(parm)+10;
    }
    if(flags&VERBOSE) printf("vc frontend for vbcc (c) in 1995-96 by Volker Barthelmann\n");
    if(!(flags&7)) flags|=5;
    tfl=flags&7;
    if(flags&VERYVERBOSE){ppname=ppv;ccname=ccv;asname=asv;ldname=ldv;rmname=rmv;l2name=l2v;}
    if(flags&NOSTDLIB){ldname=l2name;}
    /*  Nummer sicher...    */
    len+=strlen(ppname)+strlen(ccname)+strlen(asname)+strlen(rmname)+strlen(userlibs)+NAMEBUF;
    if(!(command=malloc(len))){printf(nomem);raus(20);}
    if(!(options=malloc(len))){printf(nomem);raus(20);}
    if(!(ppopts=malloc(len))){printf(nomem);raus(20);}
    *options=0;*ppopts=0;
    for(i=1;i<argc+count;i++){
        if(i<argc) parm=argv[i]; else parm=confp[i-argc];
        if(*parm=='-'){
            if(parm[1]!='D'&&parm[1]!='I'&&parm[1]!='+'){
                strcat(options,parm);strcat(options," ");
            }else{
                strcat(ppopts,parm);strcat(ppopts," ");
            }
        }
    }
    if(flags&VERYVERBOSE) printf("flags=%d opt=%d len=%d\n",flags,opt,len);
    namebuf[0]='\"'; namebuf2[0]='\"';
    for(i=1;i<argc;i++){
        int t,j;char *file;
#ifdef AMIGA
        BPTR lock;
#endif
        if(i<argc) parm=argv[i]; else parm=confp[i-argc];
        if(*parm=='-'||!*parm) continue;
        if(flags&VERYVERBOSE) printf("Argument %d:%s\n",i,parm);
#ifdef AMIGA
        if(pm) if(MatchFirst((STRPTR)parm,ap)) {printf("No match for %s\n",parm);continue;}
#endif
        do{
#ifdef AMIGA
            if(pm) file=(char *)&ap->ap_Buf[0]; else file=parm;
            t=typ(file);
            if(pm&&(lock=Lock((STRPTR)file,-2))){
                NameFromLock(lock,(STRPTR)namebuf+1,NAMEBUF-2);
                strcat(namebuf,"\"");
                file=namebuf;
                UnLock(lock);
            }else{
                strcpy(namebuf+1,file);
                strcat(namebuf,"\"");
                file=namebuf;
            }
#else
            file=parm;
            t=typ(file);
            strcpy(namebuf+1,file);
            strcat(namebuf,"\"");
            file=namebuf;
#endif
            if(flags&VERYVERBOSE) printf("File %s=%d\n",file,t);
            for(j=t;j<tfl;j++){
                if(j==OBJ){ if(j==t) add_name(file,&first_obj,&last_obj);
                            continue;}
                strcpy(oldfile,file);
                if(j==t&&j!=tfl-1&&!(flags&(NOTMPFILE|KEEPSCRATCH))){
                    file=namebuf2;
                    tmpnam(file+1);
                }
                if(j==tfl-1) file=namebuf;
                if(j==PPSRC){
                    file=add_suffix(file,".i");
                    if(tfl==CCSRC&&(flags&OUTPUTSET)) file=destname;
                    sprintf(command,ppname,ppopts,oldfile,file);
                    if((tfl)!=CCSRC) add_name(file,&first_scratch,&last_scratch);
                }
                if(j==CCSRC){
                    file=add_suffix(file,".asm");
                    if(tfl==ASSRC&&(flags&OUTPUTSET)) file=destname;
                    sprintf(command,"%s -O=%-4d %s %s -o= %s",ccname,opt,options,oldfile,file);
                    if((tfl)!=ASSRC) add_name(file,&first_scratch,&last_scratch);
                }
                if(j==ASSRC){
                    file=add_suffix(file,".o");
                    if(tfl==OBJ&&(flags&OUTPUTSET)) file=destname;
                    sprintf(command,asname,oldfile,file);
                    add_name(file,&first_obj,&last_obj);
                    if((tfl)!=OBJ) add_name(file,&first_scratch,&last_scratch);
                }
                if(flags&VERBOSE) printf("%s\n",command);
                if(system(command)){printf("%s failed\n",command);raus(20);}
            }
#ifdef AMIGA
        }while(pm&&!MatchNext(ap));
        if(pm) MatchEnd(ap);
#else
        }while(0);
#endif
    }
    if((tfl)>OBJ){
    /*  Zu Executable linken    */
        struct NameList *p;
        FILE *objfile=0;
        objects=malloc(linklen);
        if(!objects){printf(nomem);raus(EXIT_FAILURE);}
        linklen+=strlen(ldname)+strlen(destname)+strlen(userlibs)+10;
        if(flags&VERYVERBOSE) printf("linklen=%d\n",linklen);
        if(!(linkcmd=(char *)malloc(linklen))){printf(nomem);raus(20);}
        p=first_obj;
        if(linklen>=MAXCLEN){
            strcpy(objects+1,tmpnam(0));
            *objects='@';
            if(!(objfile=fopen(objects+1,"w"))){
                printf("Could not open <%s>!\n",objects+1);
                raus(20);
            }
        }else *objects=0;
        while(p){
            if(p->obj){
                if(linklen>=MAXCLEN){
                    fputs(p->obj,objfile);
                    fputs("\n",objfile);
                }else{
                    strcat(objects,p->obj);strcat(objects," ");
                }
            }
            p=p->next;
        }
        if(objfile) fclose(objfile);
        if(*objects){
            sprintf(linkcmd,ldname,objects,userlibs,destname);
            if(flags&VERBOSE) printf("%s\n",linkcmd);
            /*  hier wird objfile bei Fehler nicht geloescht    */
            if(system(linkcmd)){printf("%s failed\n",linkcmd);raus(20);}
#ifdef AMIGA
            if(flags&VERBOSE){
                BPTR l;
                if(l=Lock((STRPTR)destname,-2)){
                    if(Examine(l,&fib)) printf("Size of executable: %lu bytes\n",(unsigned long)fib.fib_Size);
                    UnLock(l);
                }
            }
#endif
        }else puts("No objects to link");
        if(objfile) remove(objects+1);
    }
    if(!(flags&KEEPSCRATCH)) del_scratch(first_scratch);
    raus(0);
}

int typ(char *p)
{
    p=strrchr(p,'.');
    if(!p) return(5);
    if(!strcmp(p,".c")) return(PPSRC);
    if(!strcmp(p,".i")) return(CCSRC);
    if(!strcmp(p,".s")) return(ASSRC);
    if(!strcmp(p,".asm")) return(ASSRC);
    if(!strcmp(p,".o")) return(OBJ);
    if(!strcmp(p,".obj")) return(OBJ);
    return(5);
}

char *add_suffix(char *s,char *suffix)
{
    static char str[NAMEBUF+3],*p;
    if(strlen(s)+strlen(suffix)>NAMEBUF){printf("string too long\n");raus(20);}
    if(s!=str) strcpy(str,s);
    p=strrchr(str,'.');
    if(!p) p=str+strlen(s);
    strcpy(p,suffix);
    strcat(p,"\"");
    return(str);
}
