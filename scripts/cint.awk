
BEGIN {
    TYPES["ULONG"]=1;
    TYPES["LONG"]=1;
    TYPES["UBYTE"]=1;
    TYPES["BYTE"]=1;
    TYPES["UWORD"]=1;
    TYPES["WORD"]=1;
    TYPES["APTR"]=1;
    TYPES["BPTR"]=1;
    TYPES["STRPTR"]=1;
    TYPES["BSTR"]=1;
    TYPES["BOOL"]=1;
    TYPES[""]=1;
    TYPES[""]=1;

    # 1- types
    # 2- commands
    KEYWORDS["struct"]=2;
    KEYWORDS["int"]=1;
    KEYWORDS["char"]=1;
    KEYWORDS["double"]=1;
    KEYWORDS["float"]=1;
    KEYWORDS["for"]=10;
    KEYWORDS["if"]=10;
    KEYWORDS["else"]=10;
    KEYWORDS["do"]=10;
    KEYWORDS["while"]=10;
    KEYWORDS["switch"]=10;
    KEYWORDS["case"]=10;
    KEYWORDS["break"]=10;
    KEYWORDS["continue"]=10;
    KEYWORDS["goto"]=10;
    KEYWORDS["union"]=2;
    KEYWORDS["enum"]=2;
    KEYWORDS["typedef"]=1;
    KEYWORDS["void"]=1;
    KEYWORDS["unsigned"]=3;
    KEYWORDS["signed"]=3;
    KEYWORDS["const"]=3;
    KEYWORDS["long"]=3;
    KEYWORDS["short"]=3;
    KEYWORDS[""]=0;
    KEYWORDS[""]=0;

    spaces=" ";
    for (t=0; t<10; t++)
	spaces=spaces spaces;

    yyinit("");
    rt_type="";

    while ((token=yylex()) != "EOF")
    {
#print "0 -" token "-" yyval "-"
	if ((token=="keyword" && KEYWORDS[yyval]<10) || token=="typedef")
	{
	    plevel=0;
	    token=read_type(token);
	    fname = yyval;
	    token=yylex();

	    if (token=="(")
	    {
		plevel ++;
		token=yylex();

		if (ptr!="")
		    ret=rt_type " " rt_ptr;
		else
		    ret=rt_type;

		if (token==")")
		{
		    npar=1;
		    par_type[1]="void";
		    par_ptr[1]="";
		    par_name[1]="";
		}
		else
		{
		    npar=0;
		    while (token!=")")
		    {
			if (token==",")
			    token=yylex();
			token=read_type(token);
			name=yyval;
			token=yylex();

			if (plevel != 1)
			{
			    plevel --;
			    name=name ") "
			    while((token==yylex())!=")" || plevel!=1)
			    {
				if (token=="(")
				    plevel ++;
				else if (token==")")
				    plevel --;
				name=name " " yyval;
			    }
			}

			npar++;
			par_type[npar]=rt_type;
			par_ptr[npar]=rt_ptr;
			par_name[npar]=name;
print "-" rt_type "-" rt_ptr "-" name
		    }
		}

		print "Processing "fname"..." npar
		file="test/" tolower(fname) ".c"
		print "/*****************************************************************************\n"
		print "    NAME */"
		print "\t"ret" "fname" (\n"
		print "/*  SYNOPSIS */"

		if (npar==1 && par_type[1]=="void" && par_ptr[1] == "" &&
		    par_name[1]=="")
		{
		    print "\tvoid)"
		}
		else
		{
		    maxlen=0;
		    for (t=1; t<=npar; t++)
		    {
			len=length(par_type[t]);
			len2=length(par_ptr[t]);
			len+=len2 ? len2+1 : 0;
			if (len > maxlen)
			    maxlen = len;
		    }

		    for (t=1; t<=npar; t++)
		    {
			len=length(par_type[t]);
			len2=length(par_ptr[t]);

			printf("\t%s", par_type[t]);
			printf("%s", substr(spaces,1,maxlen-len-len2));
			printf("%s %s",par_ptr[t],par_name[t]);
			if (t==npar)
			    print ")"
			else
			    print ","
		    }
		}
		print "\n*/"
	    }
	}
    }
}

function read_type(pretoken     ,token) {
    token=pretoken;
print "token3 " token "-" yyval

    rt_type="";
    while ((token=="keyword" && KEYWORDS[yyval]<10) || token=="typedef")
    {
	rt_type=rt_type " " yyval;
	if (KEYWORDS[yyval]==2)
	{
	    token=yylex();
	    rt_type=rt_type " " yyval;
	}
	token=yylex();
print "token2 " token
    }
    rt_type=substr(rt_type,2);
print "type " rt_type

    rt_ptr="";
    while (token=="*" || token=="(")
    {
	rt_ptr=rt_ptr yyval;
	if (token=="(")
	    plevel++;
	token=yylex();
print "token1 " token
    }
print "ptr " rt_ptr

    return token;
}

function yyinit(str) {
    yyrest=str;
}

function yylex() {
    for (yyagain=1; yyagain; )
    {
	yyagain=0;
	if (yyrest=="" || match(yyrest,/^[ \t\014\r]+/))
	{
	    while (1)
	    {
		while (yyrest=="")
		{
		    if (getline yyrest != 1)
			return "EOF";
#print "1 -" yyrest "-"
		    if (substr(yyrest,1,1) == "#")
			yyrest="";
		}
		if (!match(yyrest,"^[ \t\014\r]+"))
		    break;
		yyrest=substr(yyrest,RSTART+RLENGTH);
	    }
	}
	if (match(yyrest,/^\/\*/))
	{
	    yyrest=substr(yyrest,3);
	    while (!match(yyrest,"\*\/"))
	    {
		if (getline yyrest != 1)
		    return "EOF";
#print "2 -" yyrest "-"
	    }
	    yyrest=substr(yyrest,RSTART+RLENGTH);
	    yyagain=1;
	}
    }
    if (match(yyrest,"^[a-zA-Z_][a-zA-Z0-9_]*"))
    {
	yyval=substr(yyrest,RSTART,RLENGTH);
	yyrest=substr(yyrest,RSTART+RLENGTH);

	if (yyval in TYPES)
	    return "typedef";
	else if (yyval in KEYWORDS)
	    return "keyword";
	else
	    return "ident";
    }
    if (match(yyrest,"^0[xX][0-9a-f]+[lL]?"))
    {
	yyval=substr(yyrest,RSTART,RLENGTH);
	yyrest=substr(yyrest,RSTART+RLENGTH);
	return "int_const";
    }
    if (match(yyrest,/^([0-9]*\.[0-9]+|[0-9]+\.[0-9]*)([eE][0-9]+)?[fF]?/))
    {
	yyval=substr(yyrest,RSTART,RLENGTH);
	yyrest=substr(yyrest,RSTART+RLENGTH);
	return "float_const";
    }
    if (match(yyrest,"^0[0-7]+[lL]?"))
    {
	yyval=substr(yyrest,RSTART,RLENGTH);
	yyrest=substr(yyrest,RSTART+RLENGTH);
	return "int_const";
    }
    if (match(yyrest,"^[1-9][0-9]*[lL]?"))
    {
	yyval=substr(yyrest,RSTART,RLENGTH);
	yyrest=substr(yyrest,RSTART+RLENGTH);
	return "int_const";
    }
    if (match(yyrest,"^\""))
    {
	if (match(yyrest,"^\"(\\\"|[^\"])*\""))
	{
	    yyval=substr(yyrest,RSTART,RLENGTH);
	    yyrest=substr(yyrest,RSTART+RLENGTH);
	    return "str_const";
	}
	else
	{
	    yyval=substr(yyrest,1,length(yyrest)-1);
	    while (1)
	    {
		if (getline yyrest != 1)
		    return "EOF";
		if (match(yyrest,"^(\\\"|[^\"])*\""))
		    break;
		yyval=yyval substr(yyrest,1,length(yyrest)-1);
	    }
	    yyval=yyval substr(yyrest,RSTART,RLENGTH);
	    yyrest=substr(yyrest,RSTART+RLENGTH);
	    return "str_const";
	}
    }
    if (substr(yyrest,1,3)=="...")
    {
	yyval="...";
	yyrest=substr(yyrest,4);
	return yyval;
    }
    yyval=substr(yyrest,1,1);
    yyrest=substr(yyrest,2);
    return yyval;
}
