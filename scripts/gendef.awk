function strip(string)
{
    string=substr(string,match(string,/[^ \t|]/))
    return string;
}

function special(name)
{
    if(name=="Forbid"||name=="Permit"||name=="Disable"||name=="Enable"||
       name=="ObtainSemaphore"||name=="ObtainSemaphoreShared"||
       name=="ReleaseSemaphore"||name=="Switch")
	return "P";
    if(name=="Supervisor")
	return "S";
    return "";
}

BEGIN	    {
		FS="[,()]*";
		body="";
	    }
/__AROS_LH/ {
		head="#define " strip($3) "(";
		body=special(strip($3)) "(" strip($2) ", " strip($3) ", \\\n";
		cnt=0;
		next;
	    }
/__AROS_LA/ {
		if(cnt)
		    head=head ", ";
		head=head strip($3);
		body=body " __AROS_LA(" $2 "," $3 "," $4 "), \\\n";
		cnt++;
		next;
	    }
/SYNOPSIS/  {	next;	}
/LOCATION/  {	next;	}
	    {
		if($0==""||$0=="|")
		    next;
		if(body!="")
		{
		    string=head ") \\\n" "__AROS_LC" cnt body \
			   "           " strip($1) "," $2 "," $3 "," $4 ")\n";
		    head="";
		    body="";
		    all[$3+0]=string;
		    if($3>max)
			max=$3;
		}
	    }
END	    {
		for(i=5;i<=max;i++)
		    if(all[i]!="")
			print all[i];
	    }
