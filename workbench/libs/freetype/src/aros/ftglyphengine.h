#ifndef _FT_AROS_FTGLYPHENGINE_H
#define _FT_AROS_FTGLYPHENGINE_H

#include <exec/libraries.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_IMAGE_H
#include FT_RENDER_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H

#define OT_Spec1_FontFile	(OT_Spec1  | OT_Indirect)
#define OT_Spec2_DefCodePage    (OT_Level1 | 0x102)
#define OT_Spec2_CodePage       (OT_Level1 | OT_Indirect | 0x102)
#define OT_Spec3_AFMFile	(OT_Level1 | OT_Indirect | 0x103)
#define OT_Spec4_Metric		(OT_Level1 | 0x104)
#define OT_Spec5_BBox		(OT_Level1 | 0x105)
#define OT_Spec6_FaceNum	(OT_Level1 | 0x106)			// index for .ttc files
#define OT_Spec7_BMSize		(OT_Level1 | 0x107)			// embbeded bitmap size
									// 0 == scalable
#define OT_GlyphMap8Bit_Old	(OT_Level1 | 0x108)
#define OT_Spec9_Hinter		(OT_Level1 | 0x109)

// Values for OT_Spec4_Metric
#define METRIC_GLOBALBBOX	0	// default
#define METRIC_RAW_EM		1
#define METRIC_ASCEND		2
#define METRIC_TYPOASCEND	3
#define METRIC_USWINASCEND	4
#define METRIC_CUSTOMBBOX	5
#define METRIC_BMSIZE		6

// Values for OT_Spec9_Hinter
#define HINTER_DEFAULT		0	// default: postscript hinter for PS,
					// truetype hinter for TT if bytecode interpreter
					// is compiled in, autohinter for all other cases.
#define HINTER_FORCEAUTO	1	// Use autohinter instead of PS or TT hinter.
#define HINTER_NONE		2	// Use NO hinter (may speed up things, but bad results).
					// Default for bitmap fonts.

struct FT_GlyphEngine_ {
    /* diskfont standard */
    struct Library		*gle_Library;	/* should be our lib base */

    char			*gle_Name;	/* library name "freetype" */

    /* freetype.library specific - private data */

    /* freetype specific entites */
    FT_Library			engine;

    FT_Face			face;
    FT_CharMap			char_map;

    /* freetype.library current request/state */
    int				last_error;
    int				face_established;
    char			ft_filename[256];
    char			base_filename[256];
    char			afm_filename[256];
    //char			bold_filename[256];
    //char			italic_filename[256];
    //char			bold_italic_filename[256];
    int				face_num;
    
    unsigned long		cmap_index;
    unsigned long		requested_cmap;
    unsigned short		platform, encoding;
    int				request_char;
    int				request_char2;
    int				glyph_code;

    int				instance_changed;
    int				point_size;
    int				corrected_upem;
    int				metric_source;
    int				metric_custom;
    int				xres, yres;
    int				hinted;

    FT_Fixed			hold_sin, hold_cos;

    int				do_rotate;
    FT_Matrix			rotate_matrix;

    int				do_shear;
    FT_Matrix			shear_matrix;

    FT_Matrix			matrix;

    //	int			do_embold;
    //	int			embold_x, embold_y;

    //	int			bold_sig, italic_sig;

    unsigned short int		codepage[256];

    struct GlyphMap		*GMap;
};

typedef struct FT_GlyphEngine_ FT_GlyphEngine ;

int set_last_error(FT_GlyphEngine *, int);
void set_default_codepage(FT_GlyphEngine *);
void FreeGE(FT_GlyphEngine *);
FT_GlyphEngine *AllocGE(void);

/* flag that we need to pick an encoding table */
#define NO_CMAP_SET 0xFFFFFFFF

#endif /*_FT_AROS_FTGLYPHENGINE_H*/
