BEGIN {
    HOME=ENVIRON["HOME"];
    db_file=HOME"/Mail/jobs";

    nid = 0;

    while ((getline < db_file) > 0)
    {
	if (match($0,/^[a-zA-Z_/.0-9]+ (WORK|DONE|FREE)/))
	{
	    db[$1,"stat"]=$2;
	    db[$1,"email"]=$3;
	    text="";
	    for(t=4; t<=NF; t++)
		text=text " " $t;
	    db[$1,"text"]=substr(text,2);
	    id_a[nid++]=$1;
	}
    }

    for(t=0; t<nid; t++)
    {
	id=id_a[t];
	if (db[id,"stat"]!="DONE")
	{
	    print "<TR><TD>"
	    if (db[id,"stat"]=="FREE")
	    {
		print "<INPUT TYPE=\"checkbox\" NAME=\"REQ\" VALUE=\""id"\"> Allocate";
	    }
	    else if (db[id,"stat"]=="WORK")
	    {
		print "<INPUT TYPE=\"checkbox\" NAME=\"FREE\" VALUE=\""id"\"> Free";
		print "<INPUT TYPE=\"checkbox\" NAME=\"DONE\" VALUE=\""id"\"> Done";
	    }
	    if (db[id,"text"]=="")
		print "</TD><TD>"id"</TD></TR>";
	    else
		print "</TD><TD>"db[id,"text"]"</TD></TR>";
	}
    }
}
