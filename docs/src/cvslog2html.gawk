/^revision/ { active=1; par=0; next; }
/^----------------------------/ { active=0; next; }
/^=========/ { exit(0); }
/^date:/ {
    date=$2;
    author=substr($5,1,length($5)-1);
    split(date,a,"/");
    print "<DT>"a[3]"."a[2]"."a[1]" " author"<DD>"
    next;
}
/^branches: / { next }
 {
    if (active)
    {
	if ($1=="")
	{
	    if (!par)
	    {
		print "<P>"
		par=1;
	    }
	}
	else
	{
	    par = 0;

	    line=$0;

	    gsub(/&/,"\\&amp;",line);
	    gsub(/</,"\\&lt;",line);
	    gsub(/>/,"\\&gt;",line);
	    gsub(/"/,"\\&quot;",line);

	    print line;
	}
    }
}
