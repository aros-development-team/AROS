/*  $VER: vbcc (main.c) V0.4    */

#include "vbc.h"

static char FILE_[]=__FILE__;

int endok=1;
int line,errors;
int firstfile;

char *multname[]={"","s"};
void raus(void)
/*  Beendet das Programm					    */
{
    if(DEBUG) printf("raus()\n");
    if(!endok) printf("unexpected end of file\n");
    if(errors) printf("%d error%s found!\n",errors,multname[errors>1]);
    while(nesting>=0) leave_block();
    if(in[0]&&(c_flags[17]&USEDFLAG)) fclose(in[0]);
    cleanup_cg(out);
    if(ppout) fclose(ppout);
    if(out) fclose(out);
    if(ic1) fclose(ic1);
    if(ic2) fclose(ic2);
    if(!(c_flags[17]&USEDFLAG)) pp_free();
    if(endok&&!errors) exit(EXIT_SUCCESS); else exit(EXIT_FAILURE);
}

int eof;

void translation_unit(void)
/*  bearbeitet translation_unit 				    */
/*  hier z.Z. nur provisorisch					    */
{
    while(1){
	killsp();
	if(c_flags[18]&USEDFLAG){
	    if(*s==EOF) raus();
	    fputs(string,ppout);fputc('\n',ppout);
	    s=string;*s=0;
	}else{
	    if(eof||(!isalpha((unsigned char)*s)&&*s!='_')){
		if(!eof) error(0);
		raus();
	    }
	    endok=0;
	    var_declaration();
	    endok=1;
	}
    }
}

void dontwarn(char *p)
/*  schaltet flags fuer Meldung auf DONTWARN	*/
{
    int i;
    if(*p!='=') error(4,"-dontwarn");
    i=atoi(p+1);
    if(i>=err_num) error(159,i);
    if(i<0){
	for(i=0;i<err_num;i++)
	    if(!(err_out[i].flags&(ANSIV|FATAL)))
		err_out[i].flags|=DONTWARN;
	return;
    }
    if(err_out[i].flags&(ANSIV|FATAL)) error(160,i);
    err_out[i].flags|=DONTWARN;
}
void warn(char *p)
/*  schaltet Warnung fuer Meldung ein		*/
/*  wenn Nummer<0 sind alle Warnungen ein	*/
{
    int i;
    if(*p!='=') error(4,"-warn");
    i=atoi(p+1);
    if(i>=err_num) error(159,i);
    if(i<0){
	for(i=0;i<err_num;i++) err_out[i].flags&=~DONTWARN;
	return;
    }else err_out[i].flags&=~DONTWARN;
}

extern char *copyright;

int main(int argc,char *argv[])
{
    int i,j,fname=0;
    c_flags_val[9].f=dontwarn;
    c_flags_val[10].f=warn;
    for(i=1;i<argc;i++){
	if(*argv[i]!='-'){  /*  kein Flag   */
	    if(fname){
		error(1);
	    }else fname=i;
	}else{
	    int flag=0;
	    for(j=0;j<MAXCF&&flag==0;j++){
		size_t l;
		if(!c_flags_name[j]) continue;
		l=strlen(c_flags_name[j]);
		if(l>0&&!strncmp(argv[i]+1,c_flags_name[j],l)){
		    flag=1;
		    if((c_flags[j]&(USEDFLAG|FUNCFLAG))==USEDFLAG){error(2,argv[i]);break;}
		    c_flags[j]|=USEDFLAG;
		    if(c_flags[j]&STRINGFLAG){
			if(argv[i][l+1]!='='){error(3,argv[i]);}
			if(argv[i][l+2]||i>=argc-1)
			    c_flags_val[j].p=&argv[i][l+2];
			else
			    c_flags_val[j].p=&argv[++i][0];
		    }
		    if(c_flags[j]&VALFLAG){
			if(argv[i][l+1]!='='){error(4,argv[i]);}
			if(argv[i][l+2]||i>=argc-1)
			    c_flags_val[j].l=atol(&argv[i][l+2]);
			else
			    c_flags_val[j].l=atol(&argv[++i][0]);
		    }
		    if(c_flags[j]&FUNCFLAG) c_flags_val[j].f(&argv[i][l+1]);
		}
	    }
	    for(j=0;j<MAXGF&&flag==0;j++){
		size_t l;
		if(!g_flags_name[j]) continue;
		l=strlen(g_flags_name[j]);
		if(l>0&&!strncmp(argv[i]+1,g_flags_name[j],l)){
		    flag=1;
		    if((g_flags[j]&(USEDFLAG|FUNCFLAG))==USEDFLAG){error(2,argv[i]);break;}
		    g_flags[j]|=USEDFLAG;
		    if(g_flags[j]&STRINGFLAG){
			if(argv[i][l+1]!='='){error(3,argv[i]);}
			if(argv[i][l+2]||i>=argc-1)
			    g_flags_val[j].p=&argv[i][l+2];
			else
			    g_flags_val[j].p=&argv[++i][0];
		    }
		    if(g_flags[j]&VALFLAG){
			if(argv[i][l+1]!='='){error(4,argv[i]);}
			if(argv[i][l+2]||i>=argc-1)
			    g_flags_val[j].l=atol(&argv[i][l+2]);
			else
			    g_flags_val[j].l=atol(&argv[++i][0]);
		    }
		    if(g_flags[j]&FUNCFLAG) g_flags_val[j].f(&argv[i][l+1]);
		}
	    }
	    if(!flag){error(5,argv[i]);}
	}
    }
    if(!(c_flags[6]&USEDFLAG)){
      printf("%s\n",copyright);
      printf("%s\n",cg_copyright);
    }
    if(!(c_flags[17]&USEDFLAG)) pp_init();
    if(!(c_flags[8]&USEDFLAG)) c_flags_val[8].l=10; /* max. Fehlerzahl */
    if(c_flags[22]&USEDFLAG) c_flags[7]|=USEDFLAG;   /*  iso=ansi */
    if(c_flags[7]&USEDFLAG) error(209);
    if(c_flags[0]&USEDFLAG) optflags=c_flags_val[0].l;
    if(c_flags[11]&USEDFLAG) maxoptpasses=c_flags_val[11].l;
    if(c_flags[12]&USEDFLAG) inline_size=c_flags_val[12].l;
    if(c_flags[21]&USEDFLAG) fp_assoc=1;
    if(c_flags[25]&USEDFLAG) unroll_size=c_flags_val[25].l;
    if(c_flags[23]&USEDFLAG) noaliasopt=1;
    if(!fname){error(6);}
    inname=argv[fname];
    strncpy(errfname,inname,FILENAME_MAX);    /*  das hier ist Muell - wird noch geaendert    */
    if(!init_cg()) exit(EXIT_FAILURE);
    if(c_flags[24]&USEDFLAG) multiple_ccs=0;
    if(c_flags[17]&USEDFLAG){
	in[0]=fopen(inname,"r");
	if(!in[0]) {error(7,inname);}
    }else{
	if(!pp_include(inname)) error(7,inname);
    }
    if(!(c_flags[18]&USEDFLAG)&&!(c_flags[5]&USEDFLAG)){
	if(c_flags[1]&USEDFLAG){
	    out=open_out(c_flags_val[1].p,0);
	}else{
	    out=open_out(inname,"asm");
	}
	if(!out){
	    if(c_flags[17]&USEDFLAG){
		fclose(in[0]);
	    }else{
		pp_free();
	    }
	    exit(EXIT_FAILURE);
	}
    }
    if(c_flags[2]&USEDFLAG) ic1=open_out(inname,"ic1");
    if(c_flags[3]&USEDFLAG) ic2=open_out(inname,"ic2");
    if(c_flags[18]&USEDFLAG) ppout=open_out(inname,"i");
    if(c_flags[4]&USEDFLAG) DEBUG=c_flags_val[4].l; else DEBUG=0;
    switch_count=0;break_label=0;
    *string=0;s=string;line=0;
    firstfile = 1;
    killsp();
    nesting=-1;enter_block();
    translation_unit();
}
int mcmp(const char *s1,const char *s2)
/*  Einfachere strcmp-Variante.     */
{
    char c;
    do{
	c=*s1++;
	if(c!=*s2++) return(1);
    }while(c);
    return(0);
}
void cpbez(char *m,int check_keyword)
/*  Kopiert den naechsten Bezeichner von s nach m. Wenn check_keyord!=0 */
/*  wird eine Fehlermeldung ausgegeben, falls das Ergebnis ein		*/
/*  reserviertes Keyword von C ist.					*/
{
    char *p=m,*last=m+MAXI-1;int warned=0;
    if(DEBUG&128) printf("Before cpbez:%s\n",s);
    while(isalpha((unsigned char)*s)||isdigit((unsigned char)*s)||*s=='_'){
	if(m<last){
	    *m++=*s++;
	}else{
	    s++;
	    if(!warned){
		error(206,MAXI-1);
		warned=1;
	    }
	}
    }
    *m=0;
    if(DEBUG&128) printf("After cpbez:%s\n",s);
    if(check_keyword){
	char *n=p+1;
	switch(*p){
	case 'a': if(!mcmp(n,"uto")) error(216,p);
		  return;
	case 'b': if(!mcmp(n,"reak")) error(216,p);
		  return;
	case 'c': if(!mcmp(n,"ase")) error(216,p);
		  if(!mcmp(n,"har")) error(216,p);
		  if(!mcmp(n,"onst")) error(216,p);
		  if(!mcmp(n,"ontinue")) error(216,p);
		  return;
	case 'd': if(!mcmp(n,"efault")) error(216,p);
		  if(!mcmp(n,"o")) error(216,m);
		  if(!mcmp(n,"ouble")) error(216,p);
		  return;
	case 'e': if(!mcmp(n,"lse")) error(216,p);
		  if(!mcmp(n,"num")) error(216,p);
		  if(!mcmp(n,"xtern")) error(216,p);
		  return;
	case 'f': if(!mcmp(n,"loat")) error(216,p);
		  if(!mcmp(n,"or")) error(216,p);
		  return;
	case 'g': if(!mcmp(n,"oto")) error(216,p);
		  return;
	case 'i': if(!mcmp(n,"f")) error(216,p);
		  if(!mcmp(n,"nt")) error(216,p);
		  return;
	case 'l': if(!mcmp(n,"ong")) error(216,p);
		  return;
	case 'r': if(!mcmp(n,"egister")) error(216,p);
		  if(!mcmp(n,"eturn")) error(216,p);
		  return;
	case 's': if(!mcmp(n,"hort")) error(216,p);
		  if(!mcmp(n,"igned")) error(216,p);
		  if(!mcmp(n,"izeof")) error(216,p);
		  if(!mcmp(n,"tatic")) error(216,p);
		  if(!mcmp(n,"truct")) error(216,p);
		  if(!mcmp(n,"witch")) error(216,p);
		  return;
	case 't': if(!mcmp(n,"ypedef")) error(216,p);
		  return;
	case 'u': if(!mcmp(n,"nion")) error(216,p);
		  if(!mcmp(n,"nsigned")) error(216,p);
		  return;
	case 'v': if(!mcmp(n,"oid")) error(216,p);
		  if(!mcmp(n,"olatile")) error(216,p);
		  return;
	case 'w': if(!mcmp(n,"hile")) error(216,p);
		  return;
	default : return;
	}
    }
}
void cpnum(char *m)
/* kopiert die naechste int-Zahl von s nach m	*/
/* muss noch erheblich erweiter werden		*/
{
    if(DEBUG&128) printf("Before cpnum:%s\n",s);
    while(isdigit((unsigned char)*s)) *m++=*s++;
    *m++=0;
    if(DEBUG&128) printf("After cpnum:%s\n",s);

}
static void killsp2(void)
{
    while(isspace((unsigned char)*s)) s++;
}
void killsp(void)
/*  Ueberspringt Fuellzeichen			*/
/*  noch einige unschoene Dinge drin		*/
{
    int r;
    if(DEBUG&128) printf("Before killsp:%s\n",s);
    if(eof) raus();
    while(isspace((unsigned char)*s)){
/*	  if(*s=='\n') {line++;if(DEBUG&1) printf("Line %d\n",line);}*/
	s++;
    }
    if(*s==0){
	do{
	    if(c_flags[17]&USEDFLAG) r=(fgets(string,MAXINPUT,in[0])!=0);
		else		     r=pp_nextline();
	    if(!r){
		/*raus();*/
		if(DEBUG&1) printf("nextline/fgets returned 0\n");
		s=string;*s=0;
		eof=1;
		return;
	    }else{
		line++;
		read_new_line=1;
		if(DEBUG&1) printf("Line %d\n",line);
		if(!strncmp("#pragma",string,7)){
		    error(163);
		    s=string+7;
		    killsp2();
		    if(!strncmp("opt",s,3)){
			s+=3;killsp2();
			c_flags_val[0].l=atol(s);
			if(DEBUG&1) printf("#pragma opt %ld\n",c_flags_val[0].l);
		    }
		    else if(!strncmp("printflike",s,10)){
			struct Var *v;
			s+=10;killsp2();
			cpbez(buff,0);
			if(DEBUG&1) printf("printflike %s\n",buff);
			v=find_var(buff,0);
			if(v){
			    v->flags|=PRINTFLIKE;
			    if(DEBUG&1) printf("succeeded\n");
			}
		    }
		    else if(!strncmp("scanflike",s,9)){
			struct Var *v;
			s+=9;killsp2();
			cpbez(buff,0);
			if(DEBUG&1) printf("scanflike %s\n",buff);
			v=find_var(buff,0);
			if(v){
			    v->flags|=SCANFLIKE;
			    if(DEBUG&1) printf("succeeded\n");
			}
		    }
		    else if(!strncmp("only-inline",s,11)){
			s+=11;killsp2();
			if(!strncmp("on",s,2)){
			    if(DEBUG&1) printf("only-inline on\n");
			    only_inline=1;
			}else{
			    if(DEBUG&1) printf("only-inline off\n");
			    only_inline=2;
			}
		    }
		    else if(!strncmp("type",s,4)){
		    /*	Typ eines Ausdrucks im Klartext ausgeben    */
			np tree;
			s+=4;strcat(s,";");
			tree=expression();
			if(tree&&type_expression(tree)){
			    printf("type of %s is:\n",string+7);
			    prd(stdout,tree->ntyp);printf("\n");
			}
			if(tree) free_expression(tree);
		    }
		    else if(!strncmp("tree",s,4)){
		    /*	gibt eine expression aus    */
			np tree;
			s+=4;strcat(s,";");
			tree=expression();
			if(tree&&type_expression(tree)){
			    printf("tree of %s is:\n",string+7);
			    pre(stdout,tree);printf("\n");
			}
			if(tree) free_expression(tree);
		    }
		}
		if(string[0]=='#'&&isspace((unsigned char)string[1])&&isdigit((unsigned char)string[2])){
		    sscanf(string+2,"%d \" %[^\"]",&line,errfname);
		    if(DEBUG&1) printf("new line: %d (file=%s)\n",line,errfname);
		    line--;
		    if (firstfile)
		    {
			begin_file (out,errfname);
			firstfile = 0;
		    }
		}
		if(!strncmp("#line ",string,6)){
		    sscanf(string+6,"%d \" %[^\"]",&line,errfname);
		    if(DEBUG&1) printf("new line: %d (file=%s)\n",line,errfname);
		    line--;
		    if (firstfile)
		    {
			begin_file (out,errfname);
			firstfile = 0;
		    }
		}
		s=string;
	    }
	}while(*s=='#');
	killsp();
    }
    if(DEBUG&128) printf("After killsp:%s\n",s);
}
void enter_block(void)
/*  Setzt Zeiger/Struckturen bei Eintritt in neuen Block    */
{
    if(nesting>=MAXN){error(9,nesting);return;}
    nesting++;
    if(DEBUG&1) printf("enter block %d\n",nesting);
    first_ilist[nesting]=last_ilist[nesting]=0;
    first_sd[nesting]=last_sd[nesting]=0;
    first_si[nesting]=last_si[nesting]=0;
    first_var[nesting]=last_var[nesting]=0;
    if(nesting==1){
	first_llist=last_llist=0;
	first_clist=last_clist=0;
	merk_varf=merk_varl=0;
	merk_ilistf=merk_ilistl=0;
	merk_sif=merk_sil=0;
/*  struct-declarations erst ganz am Schluss loeschen. Um zu vermeiden,     */
/*  dass struct-declarations in Prototypen frei werden und dann eine	    */
/*  spaetere struct, dieselbe Adresse bekommt und dadurch gleich wird.	    */
/*  Nicht sehr schoen - wenn moeglich noch mal aendern. 		    */
/*	  merk_sdf=merk_sdl=0;*/
	afterlabel=0;
    }
}
void leave_block(void)
/*  Setzt Zeiger/Struckturen bei Verlassen eines Blocks     */
{
    int i;
    for(i=1;i<=MAXR;i++)
	if(regbnesting[i]==nesting) regsbuf[i]=0;

    if(nesting<0){error(10);return;}
    if(DEBUG&1) printf("leave block %d\n",nesting);
    if(nesting>0){
	if(merk_varl) merk_varl->next=first_var[nesting]; else merk_varf=first_var[nesting];
	if(last_var[nesting]) merk_varl=last_var[nesting];
	if(merk_sil) merk_sil->next=first_si[nesting]; else merk_sif=first_si[nesting];
	if(last_si[nesting]) merk_sil=last_si[nesting];
	if(merk_sdl) merk_sdl->next=first_sd[nesting]; else merk_sdf=first_sd[nesting];
	if(last_sd[nesting]) merk_sdl=last_sd[nesting];
	if(merk_ilistl) merk_ilistl->next=first_ilist[nesting]; else merk_ilistf=first_ilist[nesting];
	if(last_ilist[nesting]) merk_ilistl=last_ilist[nesting];
    }
    if(nesting==1){
	if(merk_varf) gen_vars(merk_varf);
	if(first_llist) free_llist(first_llist);
	if(first_clist) free_clist(first_clist);
	if(merk_varf) free_var(merk_varf);
	if(merk_sif) free_si(merk_sif);
/*  struct-declarations erst ganz am Schluss loeschen. Um zu vermeiden,     */
/*  dass struct-declarations in Prototypen frei werden und dann eine	    */
/*  spaetere struct, dieselbe Adresse bekommt und dadurch gleich wird.	    */
/*  Nicht sehr schoen - wenn moeglich noch mal aendern. 		    */
/*	  if(merk_sdf) free_sd(merk_sdf);*/
	if(merk_ilistf) free_ilist(merk_ilistf);
    }
    if(nesting==0){
/*  struct-declarations erst ganz am Schluss loeschen. Um zu vermeiden,     */
/*  dass struct-declarations in Prototypen frei werden und dann eine	    */
/*  spaetere struct, dieselbe Adresse bekommt und dadurch gleich wird.	    */
/*  Nicht sehr schoen - wenn moeglich noch mal aendern. 		    */
	if(merk_sdf) free_sd(merk_sdf);
	if(first_var[0]) gen_vars(first_var[0]);
	if(first_var[0]) free_var(first_var[0]);
	if(first_sd[0]) free_sd(first_sd[0]);
	if(first_si[0]) free_si(first_si[0]);
	if(first_ilist[0]) free_ilist(first_ilist[0]);
    }
    nesting--;
}
void pra(FILE *f,struct argument_list *p)
/*  Gibt argument_list umgekehrt auf Bildschirm aus		*/
{
    if(p->next){ pra(f,p->next);fprintf(f,",");}
    if(p->arg) pre(f,p->arg);
}
void pre(FILE *f,np p)
/*  Gibt expression auf Bildschirm aus				*/
{
    int c;
    c=p->flags;
    if(p->sidefx) fprintf(f,"/");
    if(p->lvalue) fprintf(f,"|");
    if(c==CALL){fprintf(f,"call-function(");pre(f,p->left);fprintf(f,")(");
		if(p->alist) pra(f,p->alist);
		fprintf(f,")");return;}
    if(c==CAST){fprintf(f,"cast(");pre(f,p->left);
		fprintf(f,"->");prd(f,p->ntyp);
		fprintf(f,")");return;}
    if(c==MEMBER){if(p->identifier) fprintf(f,".%s",p->identifier);return;}
    if(c==IDENTIFIER){if(p->identifier) fprintf(f,"%s",p->identifier);
	fprintf(f,"+");printval(f,&p->val,LONG,1); return;}
    fprintf(f,"%s(",ename[c]);
    if(p->left) pre(f,p->left);
    if(p->right){
	fprintf(f,",");
	pre(f,p->right);
    }
    fprintf(f,")");
    if(c==CEXPR||c==PCEXPR){fprintf(f,"(value="); printval(f,&p->val,p->ntyp->flags,1); fprintf(f,")");}
}

void error(int errn,...)
/*  Behandelt Ausgaben wie Fehler und Meldungen */
{
    int type;
    va_list vl;char *errstr="",*txt;
    if(errn==-1) errn=158;
    type=err_out[errn].flags;
    if(type&DONTWARN) return;
    va_start(vl,errn);
    if(type&WARNING) errstr="warning";
    if(type&ERROR) errstr="error";
    if(type&NOLINE){
	printf("%s %d: ",errstr,errn);
    }else if(type&INFUNC){
	if((type&INIC)&&err_ic&&err_ic->line){
	    if(!(c_flags[17]&USEDFLAG)) txt=filename[incnesting];
		else txt=errfname;
	    printf("%s %d in line %d of \"%s\": ",errstr,errn,err_ic->line,txt);
	}else{
	    printf("%s %d in function \"%s\": ",errstr,errn,cur_func);
	}
    }else{
	int n;
	{if(eof) printf(">EOF\n"); else printf(">%s",string);}
	if(!(c_flags[17]&USEDFLAG)){
	    printf("\n");
	    n=linenr;txt=filename[incnesting];
	}else{
	    n=line;txt=errfname;
	}
	if(c_flags[20]&USEDFLAG){   /*  strip-path from filename */
	    char *p=txt,c;
	    while(c=*p++)
		if(c==':'||c=='/'||c=='\\') txt=p;
	}
	printf("%s %d in line %d of \"%s\": ",errstr,errn,n,txt);
    }
    vprintf(err_out[errn].text,vl);
    printf("\n");
    va_end(vl);
    if(type&ERROR){
	errors++;
	if(c_flags_val[8].l&&c_flags_val[8].l<=errors)
	    {printf("Maximum number of errors reached!\n");raus();}
    }
    if(type&FATAL){printf("aborting...\n");raus();}
}
FILE *open_out(char *name,char *ext)
/*  Haengt ext an name an und versucht diese File als output zu oeffnen */
{
    char *s,*p;FILE *f;
    if(ext){
	s=mymalloc(strlen(name)+strlen(ext)+2);
	strcpy(s,name);
	p=s+strlen(s);
	while(p>=s){
	    if(*p=='.'){*p=0;break;}
	    p--;
	}
	strcat(s,".");
	strcat(s,ext);
    }else s=name;
    f=fopen(s,"w");
    if(!f) printf("Couldn't open <%s> for output!\n",s);
    if(ext) free(s);
    return(f);
}
