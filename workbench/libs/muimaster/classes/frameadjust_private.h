#ifndef _FRAMEADJUST_PRIVATE_H_
#define _FRAMEADJUST_PRIVATE_H_

/*** Instance data **********************************************************/
struct Frameadjust_DATA
{
    struct MUI_FrameSpec_intern fs_intern;
    char spec[10];
    Object *FD_display;
    Object *SL_top;
    Object *SL_left;
    Object *SL_right;
    Object *SL_bottom;
    struct Hook slider_hook;
    struct Hook frames_hook;
};

#endif /* _FRAMEADJUST_PRIVATE_H_ */
