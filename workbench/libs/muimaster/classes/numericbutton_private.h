#ifndef _NUMERICBUTTON_PRIVATE_H_
#define _NUMERICBUTTON_PRIVATE_H_

struct Numericbutton_DATA
{
    struct MUI_EventHandlerNode  ehn;
    struct MUI_ImageSpec_intern *knob_bg;
    struct Window   	    	*popwin;
    LONG    	    	    	 pop_innerw;
    LONG    	    	    	 pop_innerh;
    LONG    	    	    	 pop_innerx;
    LONG    	    	    	 pop_innery;
    LONG    	    	    	 knob_left;
    LONG    	    	    	 knob_top;
    LONG    	    	    	 knob_width;
    LONG    	    	    	 knob_height;
    LONG    	    	    	 knob_prev_left;
    LONG    	    	    	 knob_clickoffset_x;
    LONG    	    	    	 knob_val;
    LONG    	    	    	 max_text_width;
    LONG    	    	    	 text_height;
    BOOL                         needs_to_recalculate_sizes;
};

#endif /* _NUMERICBUTTON_PRIVATE_H_ */
