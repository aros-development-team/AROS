#include <freetype.h>
#include "freetype_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(FT_Error, FT_Init_FreeType,

/*  SYNOPSIS */
        AROS_LHA(FT_Library *, library, A0),

/*  LOCATION */
        struct Library *, FreeTypeBase, 5, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Init_FreeType(library);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH1(FT_Error, FT_Done_FreeType,

/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),

/*  LOCATION */
        struct Library *, FreeTypeBase, 6, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Done_FreeType(library);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH4(FT_Error, FT_New_Face,

/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(const char *, filepathname, A1),
	AROS_LHA(FT_Long, face_index, D0),
	AROS_LHA(FT_Face *, face, A2),

/*  LOCATION */
        struct Library *, FreeTypeBase, 7, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_New_Face(library, filepathname, face_index, face);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH4(FT_Error, FT_Open_Face,

/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(FT_Open_Args *, args, A1),
	AROS_LHA(FT_Long, face_index, D0),
	AROS_LHA(FT_Face *, face, A2),

/*  LOCATION */
        struct Library *, FreeTypeBase, 8, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Open_Face(library, args, face_index, face);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Error, FT_Attach_File,

/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(const char *, filepathname, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 9, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Attach_File(face, filepathname);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Error, FT_Attach_Stream,

/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(FT_Open_Args *, parameters, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 10, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Attach_Stream(face, parameters);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH5(FT_Error, FT_Set_Char_Size,

/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(FT_F26Dot6, char_width, D0),
	AROS_LHA(FT_F26Dot6, char_height, D1),
	AROS_LHA(FT_UInt, horz_resolution, D2),
	AROS_LHA(FT_UInt, vert_resolution, D3),

/*  LOCATION */
        struct Library *, FreeTypeBase, 11, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Set_Char_Size(face, char_width, char_height, horz_resolution, vert_resolution);;
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH3(FT_Error, FT_Set_Pixel_Sizes,

/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(FT_UInt, pixel_width, D0),
	AROS_LHA(FT_UInt, pixel_height, D1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 12, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Set_Pixel_Sizes(face, pixel_width, pixel_height);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH3(FT_Error, FT_Load_Glyph,

/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(FT_UInt, glyph_index, D0),
	AROS_LHA(FT_Int, load_flags, D1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 13, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Load_Glyph(face, glyph_index, load_flags);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH3(FT_Error, FT_Load_Char,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(FT_ULong, char_code, D0),
	AROS_LHA(FT_Int, load_flags, D1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 14, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Load_Char(face, char_code, load_flags);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH4(FT_Error, FT_Get_Kerning,

/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(FT_UInt, left_glyph, D0),
	AROS_LHA(FT_UInt, right_glyph, D1),
	AROS_LHA(FT_Vector *, kerning, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 15, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Get_Kerning(face, left_glyph, right_glyph, kerning);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Error, FT_Select_Charmap,

/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(FT_Encoding, encoding, D0),

/*  LOCATION */
        struct Library *, FreeTypeBase, 16, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

#warning FT_Select_Charmap not implemented yet by freetype

    return 0; /* FT_Select_Charmap(face, encoding);*/
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Error, FT_Set_Charmap,

/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(FT_CharMap, charmap, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 17, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

#warning FT_Set_Charmap not implemented yet by freetype

    return 0; /*FT_Set_Charmap(face, charmap);*/
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_UInt, FT_Get_Char_Index,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Face, face, A0),
	AROS_LHA(FT_ULong, charcode, D0),

/*  LOCATION */
        struct Library *, FreeTypeBase, 18, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Get_Char_Index(face, charcode);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH3(FT_Long, FT_MulDiv,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Long, a, D0),
	AROS_LHA(FT_Long, b, D1),
	AROS_LHA(FT_Long, c, D2),

/*  LOCATION */
        struct Library *, FreeTypeBase, 19, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_MulDiv(a, b, c);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Long, FT_MulFix,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Long, a, D0),
	AROS_LHA(FT_Long, b, D1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 20, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_MulFix(a, b);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Long, FT_DivFix,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Long, a, D0),
	AROS_LHA(FT_Long, b, D1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 21, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_DivFix(a, b);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH3(FT_Error, FT_Outline_Get_Bitmap,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(FT_Outline *, outline, A1),
	AROS_LHA(FT_Bitmap *, map, A2),

/*  LOCATION */
        struct Library *, FreeTypeBase, 22, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Outline_Get_Bitmap(library, outline, map);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH3(FT_Error, FT_Outline_Render,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(FT_Outline *, outline, A1),
	AROS_LHA(FT_Raster_Params *, params, A2),

/*  LOCATION */
        struct Library *, FreeTypeBase, 23, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Outline_Render(library, outline, params);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH3(int, FT_Outline_Decompose,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Outline *, outline, A0),
	AROS_LHA(FT_Outline_Funcs *, funcs, A1),
	AROS_LHA(void *, user, A2),

/*  LOCATION */
        struct Library *, FreeTypeBase, 24, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Outline_Decompose(outline, funcs, user);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH4(FT_Error, FT_Outline_New,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(FT_UInt, numPoints, D0),
	AROS_LHA(FT_Int, numContours, D1),
	AROS_LHA(FT_Outline *, outline, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 25, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Outline_New(library, numPoints, numContours, outline);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Error, FT_Outline_Done,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(FT_Outline *, outline, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 26, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Outline_Done(library, outline);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(void, FT_Outline_Get_CBox,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Outline *, outline, A0),
	AROS_LHA(FT_BBox *, cbox, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 27, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    FT_Outline_Get_CBox(outline, cbox);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH3(void, FT_Outline_Translate,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Outline *, outline, A0),
	AROS_LHA(FT_Pos, xOffset, D0),
	AROS_LHA(FT_Pos, yOffset, D1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 28, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    FT_Outline_Translate(outline, xOffset, yOffset);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Error, FT_Set_Raster,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(FT_Raster_Funcs *, raster_funcs, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 29, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Set_Raster(library, raster_funcs);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Error, FT_Unset_Raster,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(FT_Raster_Funcs *, raster_funcs, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 30, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Unset_Raster(library, raster_funcs);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH3(FT_Raster, FT_Get_Raster,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(FT_Glyph_Format, glyph_format, D0),
	AROS_LHA(FT_Raster_Funcs *, raster_funcs, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 31, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Get_Raster(library, glyph_format, raster_funcs);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH4(FT_Error, FT_Set_Raster_Mode,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),
	AROS_LHA(FT_Glyph_Format, format, D0),
	AROS_LHA(unsigned long, mode, D1),
	AROS_LHA(void *, args, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 32, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Set_Raster_Mode(library, format, mode, args);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(FT_Error, FT_Outline_Copy,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Outline *, source, A0),
	AROS_LHA(FT_Outline *, target, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 33, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Outline_Copy(source, target);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(void, FT_Outline_Transform,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Outline *, outline, A0),
	AROS_LHA(FT_Matrix *, matrix, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 34, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    FT_Outline_Transform(outline, matrix);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH1(void, FT_Outline_Reverse,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Outline *, outline, A0),

/*  LOCATION */
        struct Library *, FreeTypeBase, 35, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    FT_Outline_Reverse(outline);
     
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(void, FT_Vector_Transform,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Vector *, vector, A0),
	AROS_LHA(FT_Matrix *, matrix, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 36, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    FT_Vector_Transform(vector, matrix);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(void, FT_Matrix_Multiply,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Matrix *, a, A0),
	AROS_LHA(FT_Matrix *, b, A1),

/*  LOCATION */
        struct Library *, FreeTypeBase, 37, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    FT_Matrix_Multiply(a, b);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH1(FT_Error, FT_Matrix_Invert,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Matrix *, matrix, A0),

/*  LOCATION */
        struct Library *, FreeTypeBase, 38, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    return FT_Matrix_Invert(matrix);
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH1(void, FT_Default_Drivers,
	
/*  SYNOPSIS */
        AROS_LHA(FT_Library, library, A0),

/*  LOCATION */
        struct Library *, FreeTypeBase, 39, FreeType)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, FreeTypeBase)

    FT_Default_Drivers(library);
        
    AROS_LIBFUNC_EXIT
}
