#ifndef _PENDISPLAY_PRIVATE_H_
#define _PENDISPLAY_PRIVATE_H_

/*** Instance data **********************************************************/
struct Pendisplay_DATA
{
    struct MUI_PenSpec  penspec;
    struct MUI_RGBcolor rgb;
    Object  	    	*refobj;
    LONG    	    	pen;
};

#endif /* _PENDISPLAY_PRIVATE_H_ */
