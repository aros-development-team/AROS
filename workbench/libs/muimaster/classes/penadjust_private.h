#ifndef _PENADJUST_PRIVATE_H_
#define _PENADJUST_PRIVATE_H_

/*** Instance data **********************************************************/
struct Penadjust_DATA
{
    struct MUI_PenSpec      	penspec;
    struct MUI_PenSpec_intern	intpenspec;
    Object  	    	    	*listobj;
    Object  	    	    	*sliderobj;
    Object  	    	    	*coloradjobj;
};

#endif /* _PENADJUST_PRIVATE_H_ */
