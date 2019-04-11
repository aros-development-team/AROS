/*
 * Based on the code from the ft2.library from MorphOS and the ttf.library by
 * Richard Griffith
 */
#include "ftglyphengine.h"

//#define DEBUG 1
#include <aros/debug.h>
#include <exec/memory.h>
#include <diskfont/oterrors.h>
#include <libraries/codesets.h>

#include <proto/codesets.h>
#include <proto/exec.h>
#include <proto/dos.h>

int set_last_error(FT_GlyphEngine *ge,int error)
{
    ge->last_error=error;
    return(error);
}

void set_default_codepage(FT_GlyphEngine *ge)
{
    struct Library *CodesetsBase;
    struct codeset *sys_codeset;
    
    int i;

    for(i=0;i<256;++i)
	ge->codepage[i]=i;

    D(bug("getting codepage environment var\n"));

    if (GetVar("ftcodepage", (STRPTR)&ge->codepage, 512,
	   LV_VAR | GVF_BINARY_VAR | GVF_DONT_NULL_TERM) == 512)
	return;

    CodesetsBase = OpenLibrary("codesets.library", 0);
    if (!CodesetsBase)
        return;

    sys_codeset = CodesetsFindA(NULL, NULL);
    if (sys_codeset) {
        for (i = 0; i < 256; i++)
	    ge->codepage[i] = sys_codeset->table[i].ucs4;
    }
    
    CloseLibrary(CodesetsBase);
}

/* close down and dispose of GlyphEngine */
void FreeGE(FT_GlyphEngine *ge)
{
    if(ge==NULL) return;
    
    if(ge->face_established)
	FT_Done_Face( ge->face );

    FT_Done_Library( ge->engine );

    FreeVec(ge);
}

/* initialize and setup the GlyphEngine structure */
FT_GlyphEngine *AllocGE(void)
{
    FT_GlyphEngine *ge;
    FT_Error  error;

    D(bug("Allocating new glyph engine\n"));

    ge=AllocVec(sizeof(FT_GlyphEngine),MEMF_PUBLIC | MEMF_CLEAR);

    D(bug(" at 0x%lx\n",ge));

    if(ge!=NULL)
    {
	set_last_error(ge,OTERR_Success);

	D(bug("FT_Init_FreeType( 0x%lx ) ...\n",&ge->engine));

	if ((error = FT_Init_FreeType( &ge->engine ) ))
	{
	    D(bug("Error initializing engine, code = %ld.\n",
		  (LONG) error ));

	    FreeVec(ge);
	    ge=NULL;
	}
	else
	{
	    /* pre-inits */
	    ge->shear_matrix.xx = 0x10000;
	    ge->shear_matrix.yy = 0x10000;
	    ge->rotate_matrix = ge->shear_matrix;
	    ge->matrix = ge->shear_matrix;

	    ge->cmap_index=NO_CMAP_SET;

	    D(bug("Setting default codepage\n"));

	    set_default_codepage(ge);
	}

	D(bug("Returning new glyph engine 0x%lx\n",ge));
    }

    return(ge);
}
