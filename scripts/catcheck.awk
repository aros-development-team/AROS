#!/usr/bin/awk -f
BEGIN {
	if( ARGC!=4 )
	{
		print "too few arguments, use:"
		print "awk -f catcheck.awk <somemessage> <cd-file> <ct-file>"
	}
	else
	{
		message=ARGV[1];
		cdfile=ARGV[2];
		ctfile=ARGV[3];
#		print "cdfile:", cdfile;
		defline=1;
		countcd = 0;
		while ((getline line <cdfile) > 0)
		{
			if( line ~ /^[;#]/ )
				defline=1;
			else if( line )
			{
				if( defline )
				{
					defline=0;
					countcd++;
					split(line, wrd, "[ \t\r\n]");
#					print "def: ", line, "w:", wrd[1];
					def[toupper(wrd[1])]=1;
				}
			}
		}
#		for(word in def) print "word: ", word, ";def: ", def[word];
#		print "ctfile:", ctfile;
		defline=1;
		countct = 0;
		countmatch = 0;
		while ((getline line <ctfile) > 0)
		{
			if( line ~ /^[;#]/ )
				defline=1;
			else if( line )
			{
				if( defline )
				{
					defline=0;
					countct++;
					split(line, wrd, "[ \t\r\n]");
#					print "def: ",line,"w:", wrd[1];
					if( def[toupper(wrd[1])]==1 )
					{
#						print"Match!";
						def[toupper(wrd[1])]=2
						countmatch++;
					}
					else
					{
						print"+:" wrd[1];
					}
				}
			}
		}
		for(word in def)
		{
			if( def[word]==1 ) print "-:" word;
		}
#		print "cd:" countcd " ct:" countct " match:" countmatch;
		# final output format: message "number of entries in CD" : "missing entries in CT" : "extra entries in CT"
		print message countcd ":" countcd-countmatch ":" countct-countmatch;
	}
}
