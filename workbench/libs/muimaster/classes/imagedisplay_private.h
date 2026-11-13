#ifndef _IMAGEDISPLAY_PRIVATE_H_
#define _IMAGEDISPLAY_PRIVATE_H_

/*** Instance data **********************************************************/
struct Imagedisplay_DATA
{
    char *spec;
    struct MUI_ImageSpec_intern *img;
    ULONG flags;
    WORD defwidth;
    WORD defheight;
};

#endif /* _IMAGEDISPLAY_PRIVATE_H_ */
