# $Id$
#
# This script converts autodoc sources into HTML files. Just call it with the
# names of all source to convert. It will create cross references as
# neccessary.
#
BEGIN {
    # Here is a list of special items in the sources which should be
    # replaced by a link
    special_item["NEWLIST()"]="<A HREF=\"../srcs/include/exec/lists.h\">NEWLIST()</A>";

    # Long names for directories
    LIBS["clib"]="ANSI C linklib";
    LIBS["alib"]="amiga.lib";
    LIBS["devs"]="Devices";
    LIBS["aros"]="AROS";
    LIBS["arossupport"]="AROS Support";
    LIBS["intuition"]="Intuition";

    # Process all files...
    for(t=1; t<ARGC; t++)
    {
	file=ARGV[t];
	bn=basename(file);

	# mode can be head, field or footer. head means that I'm currently
	# looking for the headers. field means that I found the header
	# and that I'm currently processing the fields in the header
	# (eg. NAME, FUNCTION, RETURNS). footer means that I have
	# processed the header.
	mode="head";

	# Here I count the lines while I read them. This is used below
	# the the EXAMPLES section.
	fnr=0;

	# Read the file line by line
	while ((getline < file) > 0)
	{
	    fnr ++; # Count the lines.

	    if (mode=="head") # Looking for the header
	    {
		if (match($0,/^.\*\*\*\*\*+$/)) # Found it ?
		{
		    fname=""; # No function name yet

		    # Distinguish to which part of AROS this function
		    # belongs. To achieve this, I examine the filename.
		    # It contains the name of the part as the last
		    # directory in the path.
		    lib=file;
		    gsub(/\/[^/]+$/,"",lib); # Strip off the filename
		    gsub(/^.*\//,"",lib); # Strip off all leading dirs

		    # The filename of the resulting HTML file
		    out="../html/autodocs/" bn ".html";

		    location=0; # The field LOCATION hasn't been read yet

		    # Next mode
		    mode="field";
		    lastfield="";
		}
	    }
	    else if (mode=="field") # Reading the header
	    {
		# This might be the name of a new field.
		newfield=$1;

		# If we found something, make sure it's not a comment
		# If it is a comment, read the next word. This must be
		# the fields' name.
		if (newfield!="")
		{
		    if (newfield=="#" || newfield=="/*")
			newfield=$2;
		}

		# Check if this is a field. We do this by comparing
		# the name of the field against all known field names.
		if (newfield=="NAME" || newfield=="SYNOPSIS" || newfield=="LOCATION" ||
		    newfield=="FUNCTION" || newfield=="INPUTS" || newfield=="RESULT" ||
		    newfield=="NOTES" || newfield=="EXAMPLE" || newfield=="BUGS" ||
		    newfield=="SEE" || newfield=="INTERNALS" || newfield=="HISTORY")
		{
		    mode="field"; # Obsolete ?
		    # Store the name of the field so we know how to
		    # terminate it correctly when we encounter the
		    # next one.
		    lastfield=newfield;

		    # Set a flag that we are at the top of the field.
		    # Some fields need this to emit HTML code at the
		    # beginning.
		    first=1;

		    # Store the name of the current field
		    field=newfield;
		}
		else if (match($0,/^.\*\*\*\*\*+\/?$/)) # Is this the end ?
		{
		    # If we don't know the name of the function yet, then
		    # it makes no sense to create an entry in the list
		    # of functions.
		    if (fname!="")
		    {
			# If no LOCATION field has been encountered, then
			# I must use the directory in which the function
			# has been found as a hint to which part this
			# function belongs.
			if (!location)
			{
			    # Do I have a special name for this part ?
			    if (!(lib in LIBS) )
			    {
				# Print a warning.
				# Disabled because of problems when run
				# as a cron job.
				#print "Unknown lib: "lib" in file "file >> "/dev/stderr";
				print "Unknown lib: "lib" in file "file
			    }
			    else
			    {
				# Print a line for the TOC
				print out":"fname":"LIBS[lib];
			    }
			}
			else
			{
			    # Print a line for the TOC
			    print out":"fname":"lib;
			}
		    }

		    # New mode: Process the footer next (the header is
		    # complete)
		    mode="footer";
		}
		else
		{
		    # Strip off comment characters at the beginning of the
		    # lines.
		    if ($1=="#")
			line=substr($0,2);
		    else
			line=$0;

		    # Now do the special processing for each kind of field.
		    if (field=="NAME")
		    {
			if (match(line,/[(]/))
			{
			    gsub(/,[ \t]/,",",line);
			    if (split(line,a,",")==3)
			    {
				gsub(/AROS_(L|UF)H.*[(]/,"",a[1]);
				gsub(/[ \t]*[(][ \t]*/,"",a[1]);
				fname=a[2];
			    }
			    else
			    {
				fname=line;
				gsub(/[ \t]*[(][ \t]*$/,"",fname);
				type=fname;
				gsub(/^.*[ \t]+/,"",fname);
				gsub(/[ \t]+[^ \t]+$/,"",type);
			    }
			}
			else if (match(line,"#include"))
			{
			    match (line,/<.*>/);
			    hfile=substr(line,RSTART+1,RLENGTH-2);
			    link="srcs/include/" hfile;

			    if (exists("../html/" link))
				line="<A HREF=\"../"link"\">"hfile"</A>";
			    else
				line=hfile;
			}
		    }
		    else if (field=="LOCATION")
		    {
			if (match(line,/,/))
			{
			    gsub(/[)]/,"",line);
			    gsub(/,[ \t]/,",",line);

			    split(line,a,",");
			    lib=a[2];
			    location=1;
			}
		    }
		}
	    }
	    else if (mode=="footer")
	    {
	    }
	}

	# Close all files
	close (file);
    }

#    printf ("\n") >> stderr;
}

# Return the filename from a complete path
function basename(file) {
    return gensub(/.*\/([a-zA-Z0-9_]+)(\.[a-zA-Z0-9_]+)?$/,"\\1",1,file) "";
}

# Check if a file exists
function exists(file        ,err) {
    # Try to read a line from the file
    err=getline < file;

    # No Error ?
    if (err >= 0)
    {
	# Close the file
	close (file);

	# The file exists
	return 1;
    }

    # The file doesn't exist
    return 0;
}
