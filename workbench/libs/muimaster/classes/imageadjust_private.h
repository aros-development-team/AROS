#ifndef _IMAGEADJUST_PRIVATE_H_
#define _IMAGEADJUST_PRIVATE_H_

/*** Instance data **********************************************************/
struct Imageadjust_DATA
{
    Object *bitmap_string;
    Object *bitmap_image;
    struct Hook bitmap_hook;

    struct Hook gradient_hook;
    struct Hook gradient_swap_hook;
    Object *gradient_imagedisplay;
    Object *gradient_type_cycle;
    Object *gradient_horiz_button;
    Object *gradient_vert_button;
    Object *gradient_angle_slider;
    Object *gradient_start_poppen;
    Object *gradient_swap_button;
    Object *gradient_end_poppen;
    char gradient_imagespec[128];

    Object *pattern_image[18];
    ULONG last_pattern_selected;
    struct Hook pattern_select_hook;

    Object *vector_image[24];
    ULONG last_vector_selected;
    struct Hook vector_select_hook;

    Object *color_group;

    Object *external_list;
    struct Hook external_display_hook;
    struct Hook external_construct_hook;
    struct Hook external_destruct_hook;

    char *imagespec;
    LONG adjust_type;
    Object *originator;
};

#endif /* _IMAGEADJUST_PRIVATE_H_ */
