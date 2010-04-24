# Creates C files with function prototypes from FD and clib/*_proto.h files.

# Example usage:
#
# Required directory structure:
# amiga/include/fd/mylib_lib.fd
# amiga/include/clib/mylib_protos.h

# Usage: awk -f cint2.awk mylib

# Result:
# mylib/func1.c
# mylib/func2.c
# ...

BEGIN {
    date="27-11-96";

    RTYPES["long"]="LONG";
    RTYPES["unsigned long"]="ULONG";
    RTYPES["short"]="WORD";
    RTYPES["unsigned short"]="UWORD";
    RTYPES["char"]="BYTE";
    RTYPES["unsigned char"]="UBYTE";

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
    TYPES["Tag"]=1;
    TYPES["FLOAT"]=1;
    TYPES["DOUBLE"]=1;
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

    SpecialStructs["exec"]="ExecBase";
    SpecialStructs["graphics"]="GfxBase";
    SpecialStructs["dos"]="DosLibrary";
    SpecialStructs["intuition"]="IntuitionBase";

    spaces=" ";
    for (t=0; t<10; t++)
	spaces=spaces spaces;

    for (arg=1; arg<ARGC; arg++)
    {
	part_name=ARGV[arg];
	Part_name=toupper(substr(part_name,1,1)) substr(part_name,2);

	if (part_name in SpecialStructs)
	    struct_name=SpecialStructs[part_name];
	else
	    struct_name="Library";

print "Working on " part_name "..."

	dir="amiga/" part_name;
	system("mkdir " dir);
    print dir;
	dir=dir"/";
	INPUT="amiga/include/fd/" part_name "_lib.fd"
    print INPUT

	# Read LVO
	delete LVO
	while ((getline line < INPUT) > 0)
	{
	    if (substr(line,1,6) == "##bias")
		offset = int(substr(line,7))/6;
	    else if (substr(line,1,6) == "##base")
	    {
		match(line,"[A-Za-z_][A-Za-z0-9_]*$");
		Base = substr(line,RSTART+1,RLENGTH-1);
	    }
	    else if (!match (line,/[*#]/))
	    {
		match(line,"[A-Za-z_][A-Za-z0-9_]*");
		fname=substr(line,RSTART,RLENGTH);
		line=substr(line,RSTART+RLENGTH);
		if (match(line,"[(][^)]*[)]"))
		{
		    if (RLENGTH!=2)
			fargs=substr(line,RSTART+1,RLENGTH-2);
		    else
			fargs="";
		}
		else
		    fargs="";
		line=substr(line,RSTART+RLENGTH);
		if (match(line,"[(][^)]*[)]"))
		{
		    if (RLENGTH!=2)
			fregs=substr(line,RSTART+1,RLENGTH-2);
		    else
			fregs="";
		}
		else
		    fregs="";
		#print fname " (" fargs ") (" fregs ") " offset
		LVO[fname] = offset;
		offset ++;
		LVO_regs[fname] = toupper(fregs);
	    }
	}

	INPUT="amiga/include/clib/" part_name "_protos.h"
    #print INPUT
	getline line < INPUT
	yyinit(line);
	rt_type="";

	#if (!match(FILENAME,"\/.*_"))
	#{
	#    print "Can't find libname"
	#    exit (10);
	#}
	#dir=substr(FILENAME,RSTART+1,RLENGTH-2);
	#
	#if (dir!="")
	#{
	#    system("mkdir " dir);
	#    part_name=dir;
	#    dir=dir "/";
	#}

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

		    if (rt_ptr!="")
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
    #print 1
			    if (token==",")
				token=yylex();
			    token=read_type(token);
			    if (token=="ident")
			    {
				name=yyval;
				token=yylex();
			    }
			    else
				name="";

    #			     if (plevel != 1)
    #			     {
    #				 plevel --;
    #				 name=name ") "
    #				 while((token==yylex())!=")" || plevel!=1)
    #				 {
    #print 2
    #				     if (token=="(")
    #					 plevel ++;
    #				     else if (token==")")
    #					 plevel --;
    #				     name=name " " yyval;
    #				 }
    #			     }

			    npar++;
			    par_type[npar]=rt_type;
			    par_ptr[npar]=rt_ptr;
			    par_name[npar]=name;
    #print "npar " npar "-" rt_type "-" rt_ptr "-" name
			}
		    }

		    print "Processing "fname"..."
		    #file="/dev/stdout"
		    file=dir tolower(fname) ".c"

		    if (fname in LVO)
		    {
			offset=LVO[fname];
			regs=LVO_regs[fname];
		    }
		    else
		    {
			offset=-1;
			regs="";
		    }

		    printf ("/*\n") > file;
		    print "    Copyright © 2010, The AROS Development Team. All rights reserved." >> file
		    printf ("    %sId$\n", "$") >> file;
		    printf ("\n") >> file;
		    printf ("    Desc:\n") >> file;
		    printf ("    Lang: english\n") >> file;
		    printf ("*/\n") >> file;
		    printf ("#include \"%s_intern.h\"\n\n", part_name) >> file;
		    print "/*****************************************************************************\n" >> file
		    print "    NAME */" >>file
		    print "#include <proto/" part_name ".h>\n">>file

		    if (offset!=-1)
		    {
			if (npar==1 && par_type[1]=="void" && par_ptr[1] == "" &&
			    par_name[1]=="")
			    print "\tAROS_LH0("ret", "fname",\n" >>file
			else
			    print "\tAROS_LH"npar"("ret", "fname",\n" >>file
			print "/*  SYNOPSIS */" >>file

			if (npar==1 && par_type[1]=="void" && par_ptr[1] == "" &&
			    par_name[1]=="")
			{
			    print "\t/* void */" >>file
			}
			else
			{
			    maxlen=0;
			    for (t=1; t<=npar; t++)
			    {
	#print 3
				len=length(par_type[t]);
				len2=length(par_ptr[t]);
				len+=len2 ? len2+1 : 0;
				if (len > maxlen)
				    maxlen = len;
			    }

			    for (t=1; t<=npar; t++)
			    {
	#print 4
				len=length(par_type[t]);
				len2=length(par_ptr[t]);

				printf("\tAROS_LHA(%s", par_type[t])>>file;
				printf("%s", substr(spaces,1,maxlen-len-len2))>>file;
				printf("%s, %s, %s),\n",par_ptr[t],par_name[t],
				    substr(regs,1,2))>>file;
				if (regs!="")
				{
				    regs=substr(regs,4);
				}
			    }
			}
			print "\n/*  LOCATION */\n\tstruct "struct_name" *, "Base", "offset", "Part_name")">>file
		    }
		    else
		    {
			print "\t"ret" "fname" (\n" >>file
			print "/*  SYNOPSIS */" >>file

			if (npar==1 && par_type[1]=="void" && par_ptr[1] == "" &&
			    par_name[1]=="")
			{
			    print "\tvoid)" >>file
			}
			else
			{
			    maxlen=0;
			    for (t=1; t<=npar; t++)
			    {
	#print 3
				len=length(par_type[t]);
				len2=length(par_ptr[t]);
				len+=len2 ? len2+1 : 0;
				if (len > maxlen)
				    maxlen = len;
			    }

			    for (t=1; t<=npar; t++)
			    {
	#print 4
				len=length(par_type[t]);
				len2=length(par_ptr[t]);

				printf("\t%s", par_type[t])>>file;
				printf("%s", substr(spaces,1,maxlen-len-len2))>>file;
				printf("%s %s",par_ptr[t],par_name[t])>>file;
				if (t==npar)
				    printf(")\n")>>file;
				else
				    printf(",\n")>>file;
			    }
			}
		    }
		    print "\n/*  FUNCTION\n\n    INPUTS\n\n    RESULT\n\n    NOTES\n">>file
		    print "    EXAMPLE\n\n    BUGS\n\n    SEE ALSO\n\n    INTERNALS\n">>file
		    print "    HISTORY\n\n">>file;
		    print "*****************************************************************************/" > file
		    print "{">>file;
		    print "    AROS_LIBFUNC_INIT">>file;
		    print "    aros_print_not_implemented (\"" fname "\");\n">>file;
		    print "    AROS_LIBFUNC_EXIT">>file;
		    print "} /* " fname " */">>file;
		    fclose (file);
		} # found "("
	    } # found keyword
	} # while token != EOF
    } # for all args
} # BEGIN

function read_type(pretoken     ,token) {
    token=pretoken;
#print "token3 " token "-" yyval

    rt_type="";
    while ((token=="keyword" && KEYWORDS[yyval]<10) || token=="typedef" ||
	token=="...")
    {
#print 5
	rt_type=rt_type " " yyval;
	if (KEYWORDS[yyval]==2)
	{
	    token=yylex();
	    rt_type=rt_type " " yyval;
	}
	token=yylex();
#print "token2 " token
    }
    rt_type=substr(rt_type,2);
#print "type " rt_type
    if (rt_type in RTYPES)
	rt_type = RTYPES[rt_type];

    rt_ptr="";
    while (token=="*" || token=="(")
    {
#print 6
	rt_ptr=rt_ptr yyval;
	if (token=="(")
	    plevel++;
	token=yylex();
#print "token1 " token
    }
#print "ptr " rt_ptr

    return token;
}

function yyinit(str) {
    yyrest=str;
}

function yylex() {
#print yylex
    for (yyagain=1; yyagain; )
    {
	yyagain=0;
	if (yyrest=="" || match(yyrest,/^[ \t\014\r]+/))
	{
	    while (1)
	    {
		while (yyrest=="")
		{
		    if ((getline yyrest < INPUT) != 1)
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
		if ((getline yyrest < INPUT) != 1)
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
		if ((getline yyrest < INPUT) != 1)
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
