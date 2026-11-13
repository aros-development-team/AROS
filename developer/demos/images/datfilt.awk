BEGIN {
    for (t=1; t<ARGC; t++)
    {
	fin=ARGV[t];
	name=fin;
	sub(/.*\//,"",name); # strip path
	sub(/\..*$/,"",name);
	fout=name ".h"
	getline colors < fin;

	depth=0;
	bit=1;

	while (bit < length(colors))
	{
	    depth++;
	    bit*=2;
	}

	height=0;
	width=0;
	while ((getline line < fin) > 0)
	{
	    pattern[height++] = line;
	    if (length(line)>width)
		width=length(line);
	}

	printf ("Processing %s -> %s (%d x %d x %d)\n", fin, fout, width, height, depth);

	for (y=0; y<height; y++)
	{
	    line=pattern[y];
	    xoff=0;

	    while (line!="")
	    {
		for (d=0; d<depth; d++)
		    out[y,xoff,d]=0;

		left=substr(line,1,16);
		line=substr(line,17);
		bit=32768;

		for (x=1; x<=16; x++)
		{
		    cpen=substr(left,x,1);

		    if (cpen=="")
			break;

		    pen=index(colors,cpen);
		    if (!pen)
		    {
			printf ("Error: Unknown pen '%s' in file in line %d\n", cpen, y+2);
			exit 10;
		    }

		    pen --;

		    for (d=0; d<depth; d++)
		    {
			if (pen/2.0 != int(pen/2.0))
			{
#printf ("Pos: %d/%d Plane:%d %04x\n", y, xoff, d, bit);
			    out[y,xoff,d]+=bit;
			}
			pen=int(pen/2);
		    }

		    bit=bit/2;
		}

		xoff ++;
	    }
	}

	printf ("") > fout;

	NAME=toupper(name);
	printf ("#define %s_WIDTH    %d\n", NAME, width) >> fout;
	printf ("#define %s_HEIGHT   %d\n\n", NAME, height) >> fout;

#	printf ("UWORD %sData[] =\n", name) >> fout;
	printf ("UBYTE %sData[] =\n", name) >> fout;
	
	printf ("{\n") >> fout;
	xmax=int((width+15)/16);
	pick=0;
	bit=1;
	for (d=0; d<depth; d++)
	{
	    pick+=bit;
	    bit=bit*2;
	    for (y=0; y<height; y++)
	    {
		printf ("   ") >> fout;

		for (x=0; x<xmax; x++)
		{
#		    printf (" 0x%04X,", out[y,x,d]) >> fout;
		    printf (" 0x%02X,", out[y,x,d]/256) >> fout;
		    printf (" 0x%02X,", out[y,x,d]%256) >> fout;
		}

		printf ("\n") >> fout;
	    }

	    if (d+1!=depth)
		printf ("\n") >> fout;
	}
	printf ("};\n\n") >> fout;


	printf ("struct Image %sImage =\n{\n", name) >> fout;
	printf ("    0, 0, /* Left, Top */\n") >> fout;
	printf ("    %s_WIDTH, %s_HEIGHT, /* Width, Height */\n", NAME, NAME) >> fout;
	printf ("    %d, /* Depth */\n", depth) >> fout;
	printf ("    (UWORD *)%sData, /* ImageData */\n", name) >> fout;
	printf ("    0x%02X, /* PlanePick */\n", pick) >> fout;
	printf ("    0x00, /* PlaneOnOff */\n") >> fout;
	printf ("    NULL /* NextImage */\n") >> fout;
	printf ("};\n") >> fout;

	close (fin);
	close (fout);
    }
}
