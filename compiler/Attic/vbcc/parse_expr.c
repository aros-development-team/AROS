/*  $VER: vbcc (parse_expr.c) V0.5  */

#include "vbc.h"

static char FILE_[]=__FILE__;

np expression(void)
/*  Komma-Ausdruecke  */
{
  np left,right,new;
  left=assignment_expression();
  if(!left->flags) return 0;
  killsp();
  while(*s==','){
    s++;
    killsp();
    right=assignment_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->ntyp=0;
    new->flags=KOMMA;
    left=new;
    killsp();
  }
  return left;
}
np assignment_expression(void)
/*  Zuweisungsausdruecke  */
{
  np left,new;int c=0;
  left=conditional_expression();
  killsp();
  if(*s!='='&&s[1]!='='&&s[2]!='=') return left;
  if(*s=='=') {c=ASSIGN;s++;}
  else if(*s=='*'&&s[1]=='=') {c=ASSIGNMULT;s+=2;}  
  else if(*s=='/'&&s[1]=='=') {c=ASSIGNDIV;s+=2;}   
  else if(*s=='%'&&s[1]=='=') {c=ASSIGNMOD;s+=2;}   
  else if(*s=='+'&&s[1]=='=') {c=ASSIGNADD;s+=2;}   
  else if(*s=='-'&&s[1]=='=') {c=ASSIGNSUB;s+=2;}   
  else if(*s=='&'&&s[1]=='=') {c=ASSIGNAND;s+=2;}   
  else if(*s=='^'&&s[1]=='=') {c=ASSIGNXOR;s+=2;}   
  else if(*s=='|'&&s[1]=='=') {c=ASSIGNOR;s+=2;}    
  else if(*s=='<'&&s[1]=='<') {c=ASSIGNLSHIFT;s+=3;}
  else if(*s=='>'&&s[1]=='>') {c=ASSIGNRSHIFT;s+=3;}
  else return left;
  new=mymalloc(NODES);
  new->left=left;
  new->ntyp=0;
  if(c==ASSIGN){
    new->right=assignment_expression();
    new->flags=ASSIGN;
  }else{
    /*  ASSIGNOP(a,b)->ASSIGN(a,OP(a,b))    */
    new->flags=ASSIGNADD;   /* nur um zum Merken, dass nur einmal */
                            /* ausgewertet werden darf            */
    new->right=mymalloc(NODES);
    new->right->left=left;
    new->right->right=assignment_expression();
    new->right->ntyp=0;
    if(c==ASSIGNMULT) new->right->flags=MULT;
    else if(c==ASSIGNDIV) new->right->flags=DIV;
    else if(c==ASSIGNMOD) new->right->flags=MOD;
    else if(c==ASSIGNADD) new->right->flags=ADD;
    else if(c==ASSIGNSUB) new->right->flags=SUB;
    else if(c==ASSIGNAND) new->right->flags=AND;
    else if(c==ASSIGNXOR) new->right->flags=XOR;
    else if(c==ASSIGNOR) new->right->flags=OR;
    else if(c==ASSIGNLSHIFT) new->right->flags=LSHIFT;
    else if(c==ASSIGNRSHIFT) new->right->flags=RSHIFT;
  }
  return new;
}
np conditional_expression(void)
/*  Erledigt ? :   */
{
  np left,new;
  left=logical_or_expression();
  killsp();
  if(*s=='?'){   
    s++;killsp();
    new=mymalloc(NODES);
    new->flags=COND;
    new->ntyp=0;
    new->left=left;
    new->right=mymalloc(NODES);
    new->right->flags=COLON;
    new->right->ntyp=0;
    new->right->left=expression();
    killsp();
    if(*s==':'){s++;killsp();} else error(70);
    new->right->right=conditional_expression();
    left=new;
    killsp();
  }
  return left;
}
np logical_or_expression(void)
/*  Erledigt ||  */
{
  np left,right,new;
  left=logical_and_expression();
  killsp();
  while(*s=='|'&&s[1]=='|'){
    s+=2;
    killsp();
    right=logical_and_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=LOR;
    new->ntyp=0;
    left=new;
    killsp();
    if(*s=='&'&&s[1]=='&') error(222);
  }
  return left;
}
np logical_and_expression(void)
/*  Erledigt &&  */
{
  np left,right,new;
  left=inclusive_or_expression();
  killsp();
  while(*s=='&'&&s[1]=='&'){
    s+=2;
    killsp();
    right=inclusive_or_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=LAND;
    new->ntyp=0;
    left=new;
    killsp();
    if(*s=='|'&&s[1]=='|') error(222);
  }
  return left;
}
np inclusive_or_expression(void)
/*  Erledigt |  */
{
  np left,right,new;
  left=exclusive_or_expression();
  killsp();
  while(*s=='|'&&s[1]!='|'&&s[1]!='='){
    s++;
    killsp();
    right=exclusive_or_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=OR;
    new->ntyp=0;
    left=new;
    killsp();
  }
  return left;
}
np exclusive_or_expression(void)
/*  Erledigt ^  */
{
  np left,right,new;
  left=and_expression();
  killsp();
  while(*s=='^'&&s[1]!='='){
    s++;
    killsp();
    right=and_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=XOR;
    new->ntyp=0;
    left=new;
    killsp();
  }
  return left;
}
np and_expression(void)
/*  Erledigt &  */
{
  np left,right,new;
  left=equality_expression();
  killsp();
  while(*s=='&'&&s[1]!='&'&&s[1]!='='){
    s++;
    killsp();
    right=equality_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=AND;
    new->ntyp=0;
    left=new;
    killsp();
  }
  return left;
}
np equality_expression(void)
/*  Erledigt == und !=  */
{
  np left,right,new;int c;
  left=relational_expression();
  killsp();
  while((*s=='='||*s=='!')&&s[1]=='='){
    if(*s=='!') c=INEQUAL; else c=EQUAL;
    s+=2;
    killsp();
    right=relational_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=c;
    new->ntyp=0;
    left=new;
    killsp();
  }
  return left;
}
np relational_expression(void)
/*  Erledigt <,>,<=,>=  */
{
  np left,right,new;int c;
  left=shift_expression();
  killsp();
  while((*s=='<'&&s[1]!='<')||(*s=='>'&&s[1]!='>')){
    if(*s++=='<'){
      if(*s=='='){s++;c=LESSEQ;}else c=LESS;
    }else{
      if(*s=='='){s++;c=GREATEREQ;}else c=GREATER;
    }
    killsp();
    right=shift_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=c;
    new->ntyp=0;
    left=new;
    killsp();
  }
  return left;
}
np shift_expression(void)
/*  Erledigt <<,>>  */
{
  np left,right,new;int c;
  left=additive_expression();
  killsp();
  while((*s=='<'&&s[1]=='<'&&s[2]!='=')||(*s=='>'&&s[1]=='>'&&s[2]!='=')){
    if(*s=='<') c=LSHIFT; else c=RSHIFT;
    s+=2;
    killsp();
    right=additive_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=c;
    new->ntyp=0;
    left=new;
    killsp();
  }
  return left;
}
np additive_expression(void)
/*  Erledigt +,-  */
{
  np left,right,new;int c;
  left=multiplicative_expression();
  killsp();
  while((*s=='+'||*s=='-')&&s[1]!='='){
    if(*s++=='+') c=ADD; else c=SUB;
    killsp();
    right=multiplicative_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=c;
    new->ntyp=0;
    left=new;
    killsp();
  }
  return left;
}
np multiplicative_expression(void)
/*  Erledigt *,/,%  */
{
  np left,right,new;int c;
  left=cast_expression();
  killsp();
  while((*s=='*'||*s=='/'||*s=='%')&&s[1]!='='){
    if(*s=='*') c=MULT; else {if(*s=='/') c=DIV; else c=MOD;}
    s++;
    killsp();
    right=cast_expression();
    new=mymalloc(NODES);
    new->left=left;
    new->right=right;
    new->flags=c;
    new->ntyp=0;
    left=new;
    killsp();
  }
  return left;
}
np cast_expression(void)
/*  Erledigt (typ)  */
{
  np new;char *imerk,buff[MAXI];
  killsp();
  if(*s!='('||!declaration(1)) return unary_expression();
  s++;killsp();
  new=mymalloc(NODES);
  new->flags=CAST;
  new->right=0;
  imerk=ident;ident=buff;
  new->ntyp=declarator(declaration_specifiers());
  ident=imerk;
  killsp();
  if(*s!=')') error(59); else s++;
  new->left=cast_expression();
  return new;
}
np unary_expression(void)
/*  Erledigt !,~,++,--,+,-,*,&,sizeof,__typeof  */
{
  np new;char *merk=s,buff[MAXI];
  killsp();
  if((*s!='s'&&*s!='_'&&*s!='+'&&*s!='-'&&*s!='&'&&*s!='*'&&*s!='~'&&*s!='!')||s[1]=='=') return postfix_expression();
  if(*s=='s'||*s=='_'){
    int fszof;
    merk=s;cpbez(buff,0);s=merk;
    if(strcmp("sizeof",buff)&&strcmp("__typeof",buff)){
      return postfix_expression();
    }else{
      if(*buff=='s') fszof=1; else fszof=0;
      s+=strlen(buff);
      killsp();
      new=mymalloc(NODES);
      new->flags=CEXPR;
      new->ntyp=mymalloc(TYPS);
      if(fszof) new->ntyp->flags=UNSIGNED|LONG;
        else   new->ntyp->flags=INT;
      new->ntyp->next=0;
      new->right=0;
      new->left=0;
      if(*s=='('&&declaration(1)){
	struct Typ *t;
	s++;killsp();
	merk=ident;ident=buff;
	t=declarator(declaration_specifiers());
	if(type_uncomplete(t)) error(176);
	ident=merk;
	if(fszof)
	  new->val.vulong=zl2zul(szof(t));
	else
	  new->val.vint=zl2zi(l2zl(t->flags));
	freetyp(t);
	killsp();
	if(*s!=')') error(59); else s++;
      }else{
	np tree;
	killsp();
	tree=unary_expression();
	if(!tree||!type_expression(tree)){
	  if(fszof){
	    new->val.vulong=zl2zul(l2zl(0L));
	    error(73);
	  }else{
	    new->val.vint=zl2zi(l2zl(-1L));
	  }
	}else{
	  if(fszof){
	    if(type_uncomplete(tree->ntyp)) error(176);
	    new->val.vulong=zl2zul(szof(tree->ntyp));
	  }else{
	    new->val.vint=zl2zi(l2zl(tree->ntyp->flags));
	  }
	}
	if(tree) free_expression(tree);killsp();
      }
      return new;
    }
  }
  new=mymalloc(NODES);
  new->right=0;
  new->ntyp=0;
  if(*s=='+'){
    if(s[1]=='+'){
      s+=2;
      new->left=unary_expression();
      new->flags=PREINC;
    }else{
      s++;free(new);
      return cast_expression();
    }
  }else if(*s=='-'){
    if(s[1]=='-'){
      s+=2;
      new->left=unary_expression();
      new->flags=PREDEC;
    }else{
      s++;
      new->left=cast_expression();
      new->flags=MINUS;
    }
  }else if(*s=='&'){
    s++;
    new->left=cast_expression();
    new->flags=ADDRESS;
  }else if(*s=='*'){
    s++;
    new->left=cast_expression();
    new->flags=CONTENT;
  }else if(*s=='~'){
    s++;
    new->left=cast_expression();
    new->flags=KOMPLEMENT;
  }else if(*s=='!'){
    s++;
    new->left=cast_expression();
    new->flags=NEGATION;
  }
  new->right=0;
  new->ntyp=0;
  return new;
}
np postfix_expression(void)
/*  Erledigt [],(),.,->,++,--  */
{
  np new,left;
  left=primary_expression();
  killsp();
  while(*s=='['||*s=='('||*s=='.'||(*s=='-'&&((s[1]=='-')||(s[1]=='>')))
	||(*s=='+'&&s[1]=='+')){
    new=mymalloc(NODES);
    new->ntyp=0;
    new->right=0;
    new->left=left;
    if(*s=='-'){
      s++;
      if(*s=='-'){
	s++;
	new->flags=POSTDEC;
      }else{
	s++; killsp();
	new->flags=DSTRUCT;
	new->right=identifier_expression();
	new->right->flags=MEMBER;
	new->left=mymalloc(NODES);
	new->left->ntyp=0;
	new->left->left=left;
	new->left->right=0;
	new->left->flags=CONTENT;
      }
    }else if(*s=='['){
      s++; killsp();
      new->flags=CONTENT;
      new->left=mymalloc(NODES);
      new->left->flags=ADD;
      new->left->ntyp=0;
      new->left->left=left;
      new->left->right=expression();
      killsp();
      if(*s!=']') error(62); else s++;
    }else if(*s=='+'){
      s+=2;
      new->flags=POSTINC;
    }else if(*s=='.'){
      s++;killsp();
      new->right=identifier_expression();
      new->flags=DSTRUCT;
      new->right->flags=MEMBER;
    }else if(*s=='('){
      struct argument_list *al,*first_alist=0,*last_alist=0;np n;
      s++;killsp();
      new->flags=CALL;
      new->right=0;
      while(*s!=')'){
	n=assignment_expression();
	al=mymalloc(sizeof(struct argument_list));
	al->arg=n;al->next=0;
	if(last_alist){
	  last_alist->next=al;
	  last_alist=al;
	}else{
	  last_alist=first_alist=al;
	}
	killsp();
	if(*s==',') {s++;killsp();if(*s==')') error(59);}
	else if(*s!=')') error(57);
	
      }
      new->alist=first_alist;
      if(*s!=')') error(59); else s++;
    }
    left=new;
    killsp();
  }
  return left;
}
np primary_expression(void)
/*  primary-expressions (Konstanten,Strings,(expr),Identifier)  */
{
  np new;
  if((*s>='0'&&*s<='9')||*s=='.') return constant_expression();
  if(*s=='\"'||*s=='\''||(*s=='L'&&(s[1]=='\''||s[1]=='\"'))) return string_expression();
  if(*s=='('){
    s++;killsp();
    new=expression();
    killsp();
    if(*s!=')') error(59); else s++;
    return new;
  }
  return identifier_expression();
}
np string_expression(void)
/*  Gibt Zeiger auf string oder Zeichenkonstante zurueck  */
{
  np new; char f,string[MAXINPUT],*p;int flag,val;zlong zl;
  if(*s=='L') s++;    /*  Noch keine erweiterten Zeichen  */
  p=string;f=*s++;
  while(1){
    while(*s!=f&&p<&string[MAXINPUT-1]){
      if(*s=='\\'){
	s++;
	if(*s=='\\'){*p++='\\';s++;continue;}
	if(*s=='n'){*p++='\n';s++;continue;}
	if(*s=='t'){*p++='\t';s++;continue;}
	if(*s=='r'){*p++='\r';s++;continue;}
	if(*s=='v'){*p++='\v';s++;continue;}
	if(*s=='b'){*p++='\b';s++;continue;}
	if(*s=='f'){*p++='\f';s++;continue;}
	if(*s=='a'){*p++='\a';s++;continue;}
	if(*s=='\?'){*p++='\?';s++;continue;}
	if(*s=='\''){*p++='\'';s++;continue;}
	if(*s=='\"'){*p++='\"';s++;continue;}
	flag=val=0;
	while(*s>='0'&&*s<='7'&&flag<3){
	  val=val*8+*s-'0';
	  s++;flag++;
	}
	if(flag){*p++=val;continue;}
	if(*s=='x'){
	  s++;val=0;
	  while((*s>='0'&&*s<='9')||(*s>='a'&&*s<='f')||(*s>='A'&&*s<='F')){
	    val=val*16;
	    if(*s>='0'&&*s<='9') val+=*s-'0';
	    if(*s>='a'&&*s<='f') val+=*s-'a'+10;
	    if(*s>='A'&&*s<='F') val+=*s-'A'+10;
	    s++;
	  }
	  *p++=val;continue;
	}
	error(71);
      }
      *p++=*s++;
    }
    if(*s!=f) error(74); else s++;
    killsp();
    if(f!='\"'||*s!=f) break; else s++;
  }
  *p=0;
  new=mymalloc(NODES);
  new->ntyp=mymalloc(TYPS);
  if(f=='\"'){
    struct const_list *cl,**prev;int i;
    new->ntyp->flags=ARRAY;
    new->ntyp->size=l2zl((long)(p-string)+1);
    new->ntyp->next=mymalloc(TYPS);
    new->ntyp->next->flags=STRINGCONST|CHAR;
    new->ntyp->next->next=0;
    new->flags=STRING;
    prev=&new->cl;
    for(i=0;i<p-string+1;i++){
      cl=mymalloc(CLS);
      cl->next=0;
      cl->tree=0;
      cl->other=mymalloc(CLS);
      cl->other->next=cl->other->other=0;
      cl->other->tree=0;
      cl->other->val.vchar=zl2zc(l2zl((long)string[i]));
      *prev=cl;
      prev=&cl->next;
    }
    /*        new->identifier=add_identifier(string,p-string);*/
    new->val.vlong=l2zl(0L);
  }else{
    char *l;
    new->ntyp->flags=CONST|INT;
    new->ntyp->next=0;
    new->flags=CEXPR;
    zl=l2zl(0L);
    p--;
    if(p>string) error(72);
    for(BIGENDIAN?(l=string):(l=p);BIGENDIAN?(l<=p):(l>=string);BIGENDIAN?(l++):(l--)){
      /*  zl=zl<<CHAR_BIT+*p  */
      zl=zllshift(zl,char_bit);
      zl=zladd(zl,l2zl((long)*l));
      new->val.vint=zl2zi(zl);
    }
  }
  new->left=new->right=0;
  return new;
}
np constant_expression(void)
/*  Gibt Zeiger auf erzeugt Struktur fuer Konstante zurueck */
{
  np new; zdouble db;
  zulong value,zbase,digit;unsigned long base=10,t;
  char *merk;int warned=0;
  merk=s;
  value=ul2zul(0L);
  new=mymalloc(NODES);
  new->ntyp=mymalloc(TYPS);
  new->ntyp->flags=0;
  new->ntyp->next=0;
  new->flags=CEXPR;
  new->left=new->right=0;
  new->sidefx=0;
  if(*s=='0'){
    s++;
    if(*s=='x'||*s=='X'){s++;base=16;} else base=8;
  }
  zbase=ul2zul(base);
  if(*s>='0'&&*s<='9') t=*s-'0'; 
  else if(*s>='a'&&*s<='f') t=*s-'a'+10; 
  else if(*s>='A'&&*s<='F') t=*s-'A'+10;
  else t=20;
  while(t<base){
    digit=ul2zul(t);
    if(!warned){
      if(!zulleq(value,zuldiv(zulsub(t_max[UNSIGNED|LONG],digit),zbase)))
	warned=1;
    }
    value=zuladd(zulmult(value,zbase),digit);
    s++;
    if(*s>='0'&&*s<='9') t=*s-'0'; 
    else if(*s>='a'&&*s<='f') t=*s-'a'+10; 
    else if(*s>='A'&&*s<='F') t=*s-'A'+10; 
    else t=20;
  }
  while(*s=='u'||*s=='U'||*s=='l'||*s=='L'){
    if(*s=='u'||*s=='U'){
      if(zulleq(value,t_max[UNSIGNED|INT])) new->ntyp->flags=UNSIGNED|INT;
      else               new->ntyp->flags=UNSIGNED|LONG;
    }else{
      if(zulleq(value,t_max[LONG])) new->ntyp->flags=LONG;
      else               new->ntyp->flags=UNSIGNED|LONG;
    }
    s++;
  }
  if(*s=='.'||*s=='e'||*s=='E'){
    /*  Fliesskommakonstante, ignoriert vorher berechneten Wert, falls er   */
    /*  nicht dezimal und nicht 0 war (da er dann oktal war)                */
    if(*merk=='0'&&!zuleqto(value,ul2zul(0UL))){
      value=ul2zul(0UL);zbase=ul2zul(10UL);
      while(*merk>='0'&&*merk<='9'){
	digit=ul2zul((unsigned long)(*merk-'0'));
	value=zuladd(zulmult(value,zbase),digit);
	merk++;
      }
      if(merk!=s) error(75);
    }
    db=zul2zd(value);
    if(*s=='.'){
      /*  Teil hinter Kommastellen    */
      zdouble zquot,zbased,digit;
      s++;
      zbased=d2zd(10);zquot=d2zd(0.1);
      while(*s>='0'&&*s<='9'){
	digit=d2zd((double)(*s-'0'));
	db=zdadd(db,zdmult(digit,zquot));
	zquot=zddiv(zquot,zbased);
	s++;
      }
      
    }
    if(*s=='e'||*s=='E'){
      /*  Exponentialdarstellung  */
      int exp,vorz,i;zdouble zbased;
      zbased=d2zd((double)10);
      s++;
      if(*s=='-'){
	s++;vorz=-1;
      }else{
	vorz=1;if(*s=='+') s++;
      }
      exp=0;
      while(*s>='0'&&*s<='9') exp=exp*10+*s++-'0';
      for(i=0;i<exp;i++){
	if(vorz>0) db=zdmult(db,zbased);
	else   db=zddiv(db,zbased);
      }
    }
    new->ntyp->flags=DOUBLE;
    if(*s=='f'||*s=='F'){
      new->ntyp->flags=FLOAT;s++;
    }else{
      /*  long double werden nicht unterstuetzt und sind==double :-(  */
      if(*s=='l'||*s=='L') s++;
    }
  }else{
    if(warned) error(211);
    if(new->ntyp->flags==0){
      if(base==10){
	if(zulleq(value,t_max[INT])) new->ntyp->flags=INT;
	else if(zulleq(value,t_max[LONG])) new->ntyp->flags=LONG; 
        else {new->ntyp->flags=UNSIGNED|LONG;error(212);}
      }else{
	if(zulleq(value,t_max[INT])) new->ntyp->flags=INT; 
	else if(zulleq(value,t_max[UNSIGNED|INT])) new->ntyp->flags=UNSIGNED|INT;
	else if(zulleq(value,t_max[LONG])) new->ntyp->flags=LONG;
	else {new->ntyp->flags=UNSIGNED|LONG;error(212);}
      }
    }
  }
  
  if(new->ntyp->flags==FLOAT) new->val.vfloat=zd2zf(db);
  else if(new->ntyp->flags==DOUBLE) new->val.vdouble=db;
  else if(new->ntyp->flags==INT) new->val.vint=zl2zi(zul2zl(value));
  else if(new->ntyp->flags==(UNSIGNED|INT)) new->val.vuint=zul2zui(value);
  else if(new->ntyp->flags==LONG) new->val.vlong=zul2zl(value);
  else if(new->ntyp->flags==(UNSIGNED|LONG)) new->val.vulong=value;
  else ierror(0);
  return new;
}
np identifier_expression(void)
/*  Erzeugt Identifier mit Knoten  */
{
  np new;char buff[MAXI];
  killsp();cpbez(buff,1);
  new=mymalloc(NODES);
  new->flags=IDENTIFIER;
  new->left=new->right=0;
  new->identifier=add_identifier(buff,strlen(buff));
  new->ntyp=0;
  new->sidefx=0;
  new->val.vlong=l2zl(0L);
  if(new->identifier==empty) {error(76);new->flags=0;}
  return new;
}
void free_alist(struct argument_list *p)
/*  Gibt argument_list inkl. expressions frei  */
{
  struct argument_list *merk;
  while(p){
    merk=p->next;
    if(p->arg) free_expression(p->arg);
    free(p);
    p=merk;
  }
}
void free_expression(np p)
/*  Gibt expression mit allen Typen etc. frei  */
{
  if(!p) return;
  if(p->flags==ASSIGNADD){
    if(!p->right){ierror(0);return;}
    if(p->right->left==p->left)  p->left=0;
    if(p->right->right==p->left) p->left=0;
  }
  if(p->flags==CALL&&p->alist) free_alist(p->alist);
  if(p->ntyp) freetyp(p->ntyp);
  if(p->left) free_expression(p->left);
  if(p->right) free_expression(p->right);
  free(p);
}

