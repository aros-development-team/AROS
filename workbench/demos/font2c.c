/*
    Copyright © 1997-98, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Convert Amiga font to C code.
    Lang: english
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <graphics/text.h>
#include <dos/dos.h>
#include <dos/rdargs.h>

#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

struct Library *DiskfontBase;
struct GfxBase *GfxBase;

static VOID font2c(struct TextFont *tf, FILE * fh, STRPTR prestring);

int main (int argc, char **argv)
{
    UBYTE fontname[200];
    UBYTE outfilename[200];
    UBYTE prestring_buf[200], *prestring = NULL;
    UWORD ysize = 0;
    
    
    STRPTR s;
    STRPTR args[4] = {NULL, NULL, NULL, NULL};
    struct RDArgs *rda;
    int error = RETURN_OK;
    

    SDInit(); /* Init debugging */
    rda = ReadArgs("NAME,SIZE,OUTFILE,PRESTRING/K", (IPTR *)args, NULL);
    if (rda)
    {
    	if (args[0] == NULL || args[1] == NULL || args[2] == NULL)
	{
	    printf("Usage: NAME,SIZE,OUTFILE,PRESTRING/K\n");
	    error = RETURN_FAIL;
	}
	else
	{
	    strcpy(fontname, args[0]);
	    ysize = atoi(args[1]);
	    strcpy(outfilename, args[2]);
	    if (args[3])
	    {
		strcpy(prestring_buf, args[3]);
		strcat(prestring_buf, "_");
		prestring = prestring_buf;
	    }
	    else
		prestring = "";
		
	}    
	FreeArgs(rda);
    }
    else
	error = RETURN_FAIL;
	
    if (error == RETURN_OK)
    {
	/* Look for .font, append it if it doesn't exist */
    	if (( s = strrchr(fontname, '.')))
    	{
	    if (0 != strcmp(s, ".font"))
		strcat(fontname, ".font");
	}
	else
	{
	    strcat(fontname, ".font");
	}
    
	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	if (!GfxBase)
	{
	    printf("Could not open gfx.library v37\n");
	
	}
	else
	{
	    DiskfontBase = OpenLibrary("diskfont.library", 37);
	    if (!DiskfontBase)
	    {
		printf("Couldn't open diskfont.library v37\n");
	    }
	    else
	    {
		/* Open output file */
		FILE *outfile;
		outfile = fopen(outfilename, "w");
		if (!outfile)
		{
		    printf("Could not open outfile \"%s\"", outfilename);
		}
		else
		{
		    struct TextFont *tf;
		    struct TextAttr ta =
		    {
			fontname, ysize, 0, 0
		    };
	    
		    tf = OpenDiskFont(&ta);
		    if (!tf)
		    {
			printf("Could not open textfont\n");
		    }
		    else
		    {
		        if (tf->tf_Style & FSF_COLORFONT)
			{
			    printf("Does not handle color fonts yet\n");
			}
			else
			{
			     font2c(tf, outfile, prestring);
			}    
			CloseFont(tf);
		    }
	
		    fclose(outfile);
		} /* if (outfile opened) */
	
		CloseLibrary(DiskfontBase);
	    
	    } /* if (diskfont opened) */
	
	   CloseLibrary((struct Library *)GfxBase);
	   
	} /* if (gfx opened) */
	
    } /* if (ReadArgs went ok) */

    return error;
}


#define NUMCHARS(tf) ((tf->tf_HiChar - tf->tf_LoChar) + 2)

static VOID print_word_array( UWORD *array, ULONG len, FILE *fh);
static VOID print_long_array( ULONG *array, ULONG len, FILE *fh);

static VOID print_chardata_ascii( struct TextFont *tf, FILE *fh);


const STRPTR font_styles[] =
{
    "FSF_UNDERLINED",
    "FSF_BOLD",
    "FSF_ITALIC",
    "FSF_EXTENDED",
    "FSF_COLORFONT",
    "NOFLAG",
    "NOFLAG",
    "FSF_TAGGED"
};

const STRPTR font_flags[] =
{
    "FPF_ROMFONT",
    "FPF_DISKFONT",
    "FPF_REVPATH",
    "FPF_TALLDOT",
    "FPF_WIDEDOT",
    "FPF_PROPORTIONAL",
    "FPF_DESIGNED",
    "FPF_REMOVED"
};

static UBYTE print_tf_flags(struct TextFont *tf, FILE *fh, const STRPTR *nametab, UBYTE flags)
{
    UWORD i;
    BOOL flag_found = FALSE;
    UBYTE num_set = 0;
    
    for (i = 0; i < 8; i ++)
    {
        if (flags & (1L << i))
	{
	    if (flag_found)
	    	fprintf(fh, " | ");
	    
	    fprintf(fh, "%s", nametab[i]);
	    num_set ++;
	
	}
    }

    return num_set;
}

/*
static VOID UpdateIndent(UBYTE indent, UBYTE *tabs)
{
    while (indent --)
    	*tabs ++ = '\t';
    *tabs = 0;
}
*/
static VOID font2c(struct TextFont *tf, FILE * fh, STRPTR prestring)
{
    ULONG numchars = NUMCHARS(tf);
/*
    UBYTE tabs[10];
    UBYTE ind = 0;
    
    updateindent(ind, tabs);
*/    
    fprintf(fh, "#include <graphics/text.h>\n");

    fprintf(fh, "\n\n\n");
    print_chardata_ascii(tf, fh); 
    
    fprintf(fh, "\n\n\n");
    
    if ((tf->tf_Style & FSF_COLORFONT) == 0)
    {
    	fprintf(fh, "const UWORD %schardata[] =\n", prestring);
    	print_word_array((UWORD *)tf->tf_CharData, (tf->tf_YSize * tf->tf_Modulo) / 2, fh);
	/*  We divide by two since we want number of *words*, not bytes */
    }
    
    if (tf->tf_CharLoc)
    {
	fprintf(fh, "\n\n\n");
	fprintf(fh, "const ULONG %scharloc[] =\n", prestring);
	print_long_array((ULONG *)tf->tf_CharLoc, numchars , fh);
    }
    
    if (tf->tf_CharKern)
    {
    	fprintf(fh, "\n\n\n");
	fprintf(fh, "const UWORD %scharkern[] =\n", prestring);
	print_word_array((UWORD *)tf->tf_CharKern, numchars, fh);
	
    }
    

    if (tf->tf_CharSpace)
    {
	fprintf(fh, "\n\n\n");
	fprintf(fh, "const WORD %scharspace[] =\n", prestring);
	print_word_array((UWORD *)tf->tf_CharKern, numchars, fh);
    }
    

   /*------------------------------------- */

    fprintf(fh, "\n\n\n");

    fprintf(fh, "const struct TextFont %stf =\n", prestring);

    fprintf(fh, "{\n");

    fprintf(fh, "\t{\t/* tf_Message */\n");
    fprintf(fh, "\t\t{\t/* mn_Node */\n");

    fprintf(fh, "\t\t\tNULL,\n");	/* ln_Succ */
    fprintf(fh, "\t\t\tNULL,\n");	/* ln_Pred */

/* See <exec/nodes.h> */
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    fprintf(fh, "\t\t\t%d,\n", tf->tf_Message.mn_Node.ln_Type);	/* ln_Type */
    fprintf(fh, "\t\t\t0,\n");	/* ln_Pri */
    fprintf(fh, "\t\t\t\"%s\"\n", tf->tf_Message.mn_Node.ln_Name);
#else
    fprintf(fh, "\t\t\t\"%s\",\n", tf->tf_Message.mn_Node.ln_Name);
    fprintf(fh, "\t\t\t%d,\n", tf->tf_Message.mn_Node.ln_Type);	/* ln_Type */
    fprintf(fh, "\t\t\t0\n");	/* ln_Pri */
#endif
    fprintf(fh, "\t\t},\n");
    fprintf(fh, "\t\tNULL,\n");	/* mn_ReplyPort	*/
    fprintf(fh, "\t\t0\n");	/* mn_Length	*/
    fprintf(fh, "\t},\n");
    
    fprintf(fh, "\t%d,\t/* YSize */\n", tf->tf_YSize);
    
    fprintf(fh, "\t");
    if (!print_tf_flags(tf, fh, font_styles, tf->tf_Style))
        fprintf(fh, "FS_NORMAL");

    fprintf(fh, ",\t/* Style */\n");
    
    fprintf(fh, "\t");
    print_tf_flags(tf, fh, font_flags, tf->tf_Flags);
    
    fprintf(fh, ",\t/* Flags */\n");
    
    fprintf(fh, "\t%d,\t/* XSize */\n",			tf->tf_XSize);
    fprintf(fh, "\t%d,\t/* Baseline */\n",		tf->tf_Baseline);
    fprintf(fh, "\t%d,\t/* Boldsmear */\n", 		tf->tf_BoldSmear);
    fprintf(fh, "\t%d,\t/* Accessors */\n",		tf->tf_Accessors);
    fprintf(fh, "\t%d,\t/* LoChar */\n", 		tf->tf_LoChar);
    fprintf(fh, "\t%d,\t/* HiChar */\n", 		tf->tf_HiChar);
    
    fprintf(fh, "\t(APTR)%schardata,\t/* CharData */\n", prestring);

    fprintf(fh, "\t%d,\t/* Modulo */\n", 		tf->tf_Modulo);
    
    if (tf->tf_CharLoc)
    	fprintf(fh, "\t(APTR)%scharloc,\t/* CharLoc   */\n", prestring);
    else
    	fprintf(fh, "\tNULL,\t/* CharLoc   */\n");
	
    if (tf->tf_CharSpace)
	fprintf(fh, "\t(APTR)%scharspace,\t/* CharSpace */\n", prestring);
    else
	fprintf(fh, "\tNULL,\t/* CharSpace */\n");
	
    if (tf->tf_CharKern)
	fprintf(fh, "\t(APTR)%scharkern,\t/* CharKern  */\n", prestring);
    else
	fprintf(fh, "\tNULL\t/* CharKern  */\n");
    

    fprintf(fh, "};\n");
    
    return;
    
}

static VOID print_word_array(UWORD *array, ULONG len, FILE *fh)
{
    ULONG i;
    fprintf(fh, "{");
    
    for (i = 0; i < len - 1; i ++) /* len - 1 because last item needs ',' removed */
    {

	if ((i & 0x07) == 0)
	    fprintf(fh, "\n\t");
	    
    	fprintf(fh,"0x%.4x, ", *array ++);
	
    }
    /* print last entry without ',' at the end */

    if ((i & 0x07) == 0)
	fprintf(fh, "\n\t");
	
    fprintf(fh, "0x%.4x\n", *array);
    fprintf(fh, "};");

    return;

}
static VOID print_long_array(ULONG *array, ULONG len, FILE *fh)
{
    ULONG i;
	
    fprintf(fh, "{");
    
    for (i = 0; i < len - 1; i ++) /* numchars - 1 because last item needs ',' removed */
    {

	if ((i & 0x03) == 0)
	    fprintf(fh, "\n\t");
	    
    	fprintf(fh,"0x%.8lx, ", *array ++);
	
    }
    /* print last entry without ',' at the end */

    if ((i & 0x03) == 0)
	fprintf(fh, "\n\t");
	
    fprintf(fh, "0x%.8lx\n", *array);
    fprintf(fh, "};");

    return;
}


static VOID print_chardata_ascii(struct TextFont *tf, FILE *fh)
{
    UWORD i, numchars = NUMCHARS(tf);
    UBYTE *charptr;
    
    for (i = 0; i < numchars; i ++ )
    {
        UWORD row;
	UWORD charspace;
	UWORD charkern;
	ULONG charloc;
	UWORD bitno, width;
	
	/* Warning: Should these be used for something? */
	(void)charkern;
	(void)charspace;

	charptr = tf->tf_CharData;
	
	/* tf_CharLoc is *allways* non-NULL */
	charloc = ((ULONG *)tf->tf_CharLoc)[i];
	
    	for (row = 0; row < tf->tf_YSize; row ++)
	{
	    fprintf(fh, "/* ");
	    
	    bitno = charloc >> 16;
	    width = charloc & 0xFFFF;
	    /* Extract data for this glyph */
	        
	    while (width --)
	    {
	    
	        if ( charptr[bitno >> 3] & (1 << ((~bitno) & 0x07)) )
		{
		    fprintf(fh, "@");
		}
		else
		{
		    fprintf(fh, ".");
		}
		bitno ++;
	    }
	    
	    /* Go to next row in glyph */
	    charptr += tf->tf_Modulo;
	    
	    fprintf(fh, " */\n");
	    
	}  /* for (each row in the glyph) */

    } /* for (each glyph) */
    return;
}
