#ifndef LAYOUT_H
#define LAYOUT_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

#ifndef GRAPHICS_TEXT_H
#   include <graphics/text.h>
#endif

#ifndef LIBRARIES_GADTOOLS_H
#   include <libraries/gadtools.h>
#endif

struct AslBase_intern;

#define MIN_SPACING 10

/* This command should calculate window's min size 
*/
#define LDCMD_INIT		0 
/* Layout the gadgets */
#define LDCMD_LAYOUT		1 

#define LDCMD_HANDLEEVENTS	2 
/* Cleanup anything requester specific allocated in INIT, LAYOUT or HANDLEEVENTS */
#define LDCMD_CLEANUP		3 

/* Special retuen value for HANDLEEVENTS to express that the
user has successfully requested something and hit "OK" */

#define LDRET_FINISHED 2


/* Structure for storing data between LayoutGadgets and HandleEvents type hooks */
struct LayoutData
{
    UWORD		ld_Command;
    APTR		ld_UserData;
    BOOL		ld_ScreenLocked;
    struct Gadget	*ld_GList;
    struct Window	*ld_Window;
    struct Menu		*ld_Menu;
    struct Screen	*ld_Screen;
    struct IntReq	*ld_IntReq;
    APTR		ld_Req;
    struct RastPort	ld_DummyRP;
	
    /* The font to use in the GUI */
    struct TextFont 	*ld_Font;
    
    /* Texattr describing the above font */
    struct TextAttr 	ld_TextAttr;

    /* Used for passing back info about minimum *inner* window size 
     * From LDCMD_INIT hook
    */
    UWORD		ld_MinWidth;
    UWORD		ld_MinHeight;

    /* Used for passing an inputevent to
     * LDCMD_HANDLEEVENTS hooks
    */
    struct IntuiMessage *ld_Event;

};

#endif /* LAYOUT_H */
