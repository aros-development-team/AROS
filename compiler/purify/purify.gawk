BEGIN {
    if (cpu=="i586") cpu="i386";

    if (cpu!="i386")
    {
	print "Unsupported cpu: "cpu;
	exit (10);
    }
    print "# Purify V0.1" > out;
    mainfunc=0;
}
/^[ \t]call malloc/ {
    print "\t\tcall Purify_malloc" >> out;
    next;
}
/^[ \t]call free/ {
    print "\t\tcall Purify_free" >> out;
    next;
}
/^[ \t]call calloc/ {
    print "\t\tcall Purify_calloc" >> out;
    next;
}
/^[ \t]call realloc/ {
    print "\t\tcall Purify_realloc" >> out;
    next;
}
/^[ \t]*pushl/ {
    print "\t\tpushl $4" >> out
    print "\t\tpushl %esp" >> out;
    print "\t\tcall _Purify_Push" >> out;
    print "\t\taddl $8,%esp" >> out;
    print >> out ; next
}
/^[ \t]*popl/ {
    print "\t\tpushl $4" >> out
    print "\t\tpushl %esp" >> out;
    print "\t\tcall _Purify_Pop" >> out;
    print "\t\taddl $8,%esp" >> out;
    print >> out ; next
}
/^[ \t]*subl.*,%esp$/ {
    na=split($2,a,",");
    print "\t\tpushl "a[1] >> out
    print "\t\tpushl %esp" >> out;
    print "\t\tcall _Purify_Alloca" >> out;
    print "\t\taddl $8,%esp" >> out;
    print >> out ; next
}
/^[ \t]*movl.*,%esp$/ {
    na=split($2,a,",");
    print "\t\tpushl "a[1] >> out
    print "\t\tpushl %esp" >> out;
    print "\t\tcall _Purify_MoveSP" >> out;
    print "\t\taddl $8,%esp" >> out;
    print >> out;
    next;
}
/^[ \t]*((lea|jmp)|.*%st[(][0-9]+[)])/ { print >> out; next }
/^[ \t]*[a-z]+ .*([^(]+)?[(][^)]+[)]/ {
    #print $0

    if (match($2,/^([^(,]+)?[(][^)]+[)]/))
    {
	arg1=substr($2,RSTART,RLENGTH);
	check1=1;
    }
    else
    {
	match($2,/[^,]+/);
	arg1=substr($2,RSTART,RLENGTH);
	check1=0;
    }

    arg2=substr($2,RSTART+RLENGTH+1);
    check2=match(arg2,/[(]/);

    #print "arg1="arg1" c1="check1" arg2="arg2" c2="check2

    if (check1 && check2)
    {
	print "Warning: unsupported double indirect access "NR
	print $0
    }
    else
    {
	cmd=substr($1,1,3);

	if (match($1,/^movs/))
	    size=substr($1,5,1);
	else if (match($1,/^or/))
	    size=substr($1,3,1);
	else
	    size=substr($1,4,1);

	if (match($1,/^cmp/))
	{
	    check1=check2;
	    arg1=arg2;
	    check2=0;
	}
	else if (match ($1,/^(inc|dec)/))
	{
	    check2=1;
	    arg2=arg1;
	}

	if (check1)
	    arg=arg1;

	if (check2)
	    arg=arg2;

	if (check1 || check2)
	{
	    print "\t\tpushl %eax" >> out
	    if (size == "l")
		print "\t\tpushl $4" >> out
	    else if (size == "w")
		print "\t\tpushl $2" >> out
	    else
		print "\t\tpushl $1" >> out
	    print "\t\tleal "arg",%eax" >> out
	    print "\t\tpushl %eax" >> out
	    if (check1)
		print "\t\tcall _Purify_CheckRead" >> out;
	    if (check2)
		print "\t\tcall _Purify_CheckWrite" >> out;
	    print "\t\taddl $8,%esp" >> out;
	    print "\t\tpopl %eax" >> out
	}
    }

    print >> out;
    next;
}
/^[a-zA-Z0-9_]+:$/ {
    print >> out;
    if (newfunc)
    {
	if (funcname == "main")
	{
	    print "\t\tcall _Purify_Init" >> out;
	    print "\t\tpushl $10240" >> out;
	    print "\t\tpushl %esp" >> out;
	    print "\t\tcall _Purify_InitStack" >> out;
	    print "\t\taddl $8,%esp" >> out;
	    mainfunc=1;
	}

    }
    next;
}
/^main:$/ {
    next;
}
/^[ \t]ret$/ {
    print "\t\tcall _Purify_LeaveFunction" >> out
    if (mainfunc)
    {
	print "\t\tcall _Purify_Exit" >> out;
	mainfunc = 0;
    }
}
/^[ \t]*.size/ {
    print >> out;
    print ".section\t.rodata" >> out;
    print ".L"funcname":" >> out;
    print "\t\t.string \""funcname"\"" >> out;
}
/^[ \t]*.type/ {
    match($2,/^[^,]+/);
    funcname=substr($2,RSTART,RLENGTH);
    newfunc=1;
}
/^.stabn 68/ {
    na=split($2,a,",");
    line=a[3];
    if (newfunc)
    {
	print "\t\tpushl %ebp" >> out;
	print "\t\tpushl $"line >> out;
	print "\t\tpushl $.L"funcname >> out;
	print "\t\tpushl $.Lfilename" >> out;
	print "\t\tcall _Purify_EnterFunction" >> out;
	print "\t\taddl $16,%esp" >> out;
	newfunc=0;
    }
    else
	print "\t\tmovl $"line",Purify_Lineno" >> out;
}
/^.stabs "\/[^"]+",100/ {
    match($2,/"[^"]+"/);
    path=substr($2,RSTART+1,RLENGTH-2);
}
/^.stabs "[^\/][^"]+",100/ {
    match($2,/"[^"]+"/);
    filename=substr($2,RSTART+1,RLENGTH-2);
    print >> out;
    print ".section\t.rodata" >> out;
    print ".Lfilename:" >> out;
    print "\t\t.string \""path filename"\"" >> out;
    next
}
/^[ \t]*\./ { print >> out; next }
/^[ \t]*.*[(][^)]+[)]/ {
    print "Warning: Unsupported memory access found in line "line" ("NR")"
    print $0
    print $0 >> out;
}
 { print >> out; }
