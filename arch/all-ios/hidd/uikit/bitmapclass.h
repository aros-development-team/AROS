#define AO(x) (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)
#define IS_BM_ATTR(attr, idx) (((idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

struct bitmap_data
{
    int            left;		/* Physical coordinates of top-left corner */
    int            top;
    unsigned int   width;		/* Bitmap size				   */
    unsigned int   height;
    unsigned int   mod;			/* Bytes per line			   */
    unsigned int   win_width;		/* Display window size			   */
    unsigned int   win_height;
    void	  *pixels;		/* Address in memory			   */
    void	  *context;		/* Core Graphics context		   */
    unsigned char  orientation;		/* Orientation (portrait on landscape)	   */
};
