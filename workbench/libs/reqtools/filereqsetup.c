/**************************************************************
*                                                             *
*      File/Font/Screenmode requester                         *
*                                                             *
*                                 (c) Nico François 1991-1994 *
**************************************************************/

#include "filereq.h"

/****************************************************************************************/

/***************************
*                          *
*  REQUESTER WINDOW SETUP  *
*                          *
***************************/

/****************************************************************************************/

static void REGARGS CheckGadgetsSize (ULONG *gadlen, ULONG *width, ULONG availwidth, ULONG num)
{
    ULONG overlap;
    int   i;

    if (availwidth >= *width) return;
    
    overlap = ((((*width - availwidth) << 16) / num) + 0xffff) >> 16;
    for (i = 0; i < num; i++)
    {
	gadlen[i] -= overlap;
	*width -= overlap;
    }
}

/****************************************************************************************/

static char *oscanlabs_cat[] =
	{ MSG_REGULAR_SIZE, MSG_TEXT_SIZE, MSG_GFX_SIZE, MSG_MAX_SIZE, NULL };

/****************************************************************************************/

void REGARGS RenderLED (GlobData *glob)
{
    if (glob->led_x)
    {
	SetDrMd (glob->reqrp, JAM2);
	SetAPen (glob->reqrp, glob->pens[glob->ledon ? FILLPEN : BACKGROUNDPEN]);
	RectFill (glob->reqrp, glob->led_x + 2, glob->led_y + 1,
		  glob->led_x + glob->led_w - 3, glob->led_y + glob->led_h - 2);
    }
}

/****************************************************************************************/

void REGARGS RenderReqWindow (GlobData *glob, int refresh, int dowait)
{
    if (refresh)
	GT_BeginRefresh (glob->reqwin);
	
    RenderLED (glob);
    if (glob->numselectedgad)
	UpdateNumSelGad (glob);
	    
    if (glob->reqtype == RT_FONTREQ)
	ShowFontSample (glob, refresh, dowait);
	
    if (refresh)
    {
	PrintFiles (glob);
	GT_EndRefresh (glob->reqwin, TRUE);
    }
}

/****************************************************************************************/

static int scrollpens[] =
	{ TEXTPEN,FILLPEN,FILLTEXTPEN,BACKGROUNDPEN,HIGHLIGHTTEXTPEN,-1 };

/****************************************************************************************/

int REGARGS SetupReqWindow (GlobData *glob, int resized)
{
    struct NewGadget 		ng;
    struct Gadget 		*gad;
    struct Image 		*img;
    struct ReqDefaults 		*reqdefs;
    struct ReqToolsPrefs 	*reqtoolsprefs;
    int 			top, val, val2, buttonheight, spacing, winheight;
    int 			scrwidth, scrheight, createpatgad, createstyle, winwidth, start_top;
    int 			i, width1, width2, num1, num2;
    int 			firsttime = TRUE;
    int 			isfilereq, isfontreq, isvolreq, isscreenmodereq, checkboxcount;
    int 			stdgadheight, defaultheight, dotinfowidth;
    int 			gadlen[8] , gadpos[8], reqdefnum = 0, maxpen;
    int 			overscanstrlen = 0, widthstrlen = 0, heightstrlen = 0, widthheightlen = 0, dimgadwidth = 0;
    int 			reqpos, check, led_off = 0, do_led;
    int 			checkw, checkh, checkskip, checktopoff;
    int 			leftoff, rightoff, totaloff;
    char 			**gadtxt = glob->gadtxt, *str, *dotinfostr;
    char 			*overscanstr = NULL, *widthstr = NULL, *heightstr = NULL, *defaultstr = NULL;
    ULONG 			mask;



    defaultheight = (glob->reqheight == 0);
    spacing = rtGetVScreenSize (glob->scr, (ULONG *)&scrwidth, (ULONG *)&scrheight);
    createpatgad = (glob->flags & FREQF_PATGAD) && !(glob->flags & FREQF_NOFILES);
    createstyle = (glob->flags & FREQF_STYLE);
    isvolreq = (glob->volumerequest);
    isfilereq = (glob->reqtype == RT_FILEREQ) && !isvolreq;
    isfontreq = (glob->reqtype == RT_FONTREQ);
    isscreenmodereq = (glob->reqtype == RT_SCREENMODEREQ);
    stdgadheight = glob->fontheight + 6;

    glob->entryheight = glob->fontheight + 1;


    switch (glob->reqtype)
    {
	case RT_FILEREQ:
	    if (isvolreq) reqdefnum = RTPREF_VOLUMEREQ;
	    else reqdefnum = RTPREF_FILEREQ;
	    break;
	    
	case RT_FONTREQ:
	    reqdefnum = RTPREF_FONTREQ;
	    break;
	    
	case RT_SCREENMODEREQ:
	    reqdefnum = RTPREF_SCREENMODEREQ;
	    break;
	    
    }

    leftoff = glob->scr->WBorLeft + 5;
    rightoff = glob->scr->WBorRight + 5;
    totaloff = (leftoff + rightoff);


//rebuildwindow:
    reqdefs = &rtLockPrefs()->ReqDefaults[reqdefnum];

    if (!glob->reqheight)
    {
	glob->reqheight = ((int)reqdefs->Size * scrheight) / 100;
    }
    else if (!resized)
    {
	if (glob->reqheight > scrheight) glob->reqheight = scrheight;
    }

    rtUnlockPrefs();


    start_top = (glob->scr->WBorTop + glob->scr->Font->ta_YSize + 1) + spacing;

    ng.ng_VisualInfo = glob->visinfo;
    ng.ng_TextAttr = &glob->font;
    glob->itxt.ITextFont = &glob->font;


    checkw = CheckBoxWidth (&ng);
    checkh = checkskip = CheckBoxHeight (&ng);
    if (glob->fontheight > checkskip) checkskip = glob->fontheight;


    if (isfilereq)
    {
	val = (stdgadheight + spacing) * 4 + 4;
	if (createpatgad) val += stdgadheight + spacing / 2;
	if (glob->flags & FREQF_NOFILES) val -= stdgadheight + spacing;
	if (!(glob->flags & FREQF_MULTISELECT)) val -= stdgadheight + spacing;
    }
    else if (isfontreq)
    {
	val = stdgadheight * 2 + spacing * 3 + spacing / 2 + 8 + glob->sampleheight;
	if (createstyle) val += checkskip + 4 + spacing;
    }
    else if (isvolreq)
    {
	val = (stdgadheight + spacing) * 2 + spacing / 2 + 4;
    }
    else
    {
	val = stdgadheight + glob->fontheight + spacing * 2 + 8;
	if (glob->flags & SCREQF_SIZEGADS)
		val += spacing + stdgadheight * 2 + spacing / 2;
	if (glob->flags & SCREQF_DEPTHGAD)
		val += glob->fontheight + 3 + spacing;
	if (glob->flags & SCREQF_OVERSCANGAD)
		val += stdgadheight + spacing;
	if (glob->flags & SCREQF_AUTOSCROLLGAD)
		val += checkskip + spacing;
    }
    glob->numentries = (glob->reqheight - val - start_top
	    		- BottomBorderHeight (glob->scr)) / glob->entryheight;


retryopenwindow:

    top = start_top;

    gad = (struct Gadget *)CreateContext (&glob->buttoninfo.glist);
    img = &glob->labelimages;


    reqtoolsprefs = rtLockPrefs();

    do_led = !(reqtoolsprefs->Flags & RTPRF_NOLED);
    reqdefs = &(reqtoolsprefs->ReqDefaults[reqdefnum]);

    val = (!firsttime || !defaultheight) ? 3 : reqdefs->MinEntries;
    if (glob->numentries < val) glob->numentries = val;
    firsttime = FALSE;
    val = !defaultheight ? 50 : reqdefs->MaxEntries;
    if (glob->numentries > val) glob->numentries = val;

    rtUnlockPrefs();


    /* calculate width of gadgets and window */
    gadtxt[7] = GetStr (glob->catalog, MSG_CANCEL);
    glob->led_x = 0;
    if (isfilereq || isfontreq)
    {
	for (i = 0; i < 4; i++) gadlen[i] = 0;
	if (isfilereq)
	{
	    if (do_led)
	    {
		glob->led_h = glob->fontheight - 4;
		if (glob->led_h < 7) glob->led_h = 7;
		glob->led_w = 15 + (glob->led_h - 7) * 2;
		glob->led_x = leftoff;
		led_off = glob->led_w + 6;
	    }
	    gadtxt[0] = GetStr (glob->catalog, MSG_SELECTED);
	    gadlen[0] = StrWidth_noloc (&glob->itxt, "0000");
	    gadtxt[1] = GetStr (glob->catalog, MSG_ALL);
	    gadtxt[2] = GetStr (glob->catalog, MSG_MATCH);
	    gadtxt[3] = GetStr (glob->catalog, MSG_CLEAR);
	    num1 = 4;
	    gadtxt[5] = GetStr (glob->catalog, MSG_VOLUMES);
	    gadtxt[6] = GetStr (glob->catalog, MSG_PARENT);
	    num2 = 4;
	}
	else
	{
	    gadtxt[0] = GetStr (glob->catalog, MSG_BOLD);
	    gadtxt[1] = GetStr (glob->catalog, MSG_ITALIC);
	    gadtxt[2] = GetStr (glob->catalog, MSG_UNDERLINE);
	    for (i = 0; i < 3; i++) gadlen[i] = checkw + 8 - 16;
	    num1 = 3;
	    num2 = 2;
	}

	/* Calculate width of top row of gadgets */
	width1 = 0;
	for (i = 0; i < num1; i++)
	{
	    gadlen[i] += StrWidth_noloc (&glob->itxt, gadtxt[i]) + 16;
	    width1 += gadlen[i];
	}
    }
    else
    {
	/* isvolreq or screenmode request */
	num1 = 4; num2 = 2;
	width1 = 0;
    }
    if (num2 == 2) gadtxt[5] = gadtxt[7];


    /* Calculate width of button row of gadgets */
    width2 = 0;
    for (i = 0; i < num2; i++)
    {
	val = StrWidth_noloc (&glob->itxt, gadtxt[i+4]) + 16;
	if (val > width2) width2 = val;
    }
    for (i = 0; i < num2; i++) gadlen[i+4] = width2;
    width2 *= num2;


    if (isfilereq && !(glob->flags & FREQF_MULTISELECT)) width1 = 0;


    if (isscreenmodereq)
    {
	int len;


	overscanstr = GetStr (glob->catalog, MSG_OVERSCAN);
	overscanstrlen = StrWidth_noloc (&glob->itxt, overscanstr);
	widthstr = GetStr (glob->catalog, MSG_WIDTH);
	widthstrlen = widthheightlen = StrWidth_noloc (&glob->itxt, widthstr);
	heightstr = GetStr (glob->catalog, MSG_HEIGHT);
	heightstrlen = StrWidth_noloc (&glob->itxt, heightstr);	
	if (heightstrlen > widthheightlen) widthheightlen = heightstrlen;
	defaultstr = GetStr (glob->catalog, MSG_DEFAULT);
	winwidth = 276;
	val = len = 0;
	
	if (glob->flags & SCREQF_OVERSCANGAD)
	{
	    if (overscanstrlen > widthheightlen) widthheightlen = overscanstrlen;
	    for (i = 0; oscanlabs_cat[i]; i++)
	    {
		glob->oscanlabs[i] = GetStr (glob->catalog, oscanlabs_cat[i]);
		val = StrWidth_noloc (&glob->itxt, glob->oscanlabs[i]);
		if (val > len) len = val;
	    }
	    val = len + overscanstrlen + 36 + 8 + (rightoff + leftoff) + 2;
	}
	if (glob->flags & SCREQF_SIZEGADS)
	{
	    dimgadwidth = StrWidth_noloc (&glob->itxt, "000000") + 12;
	    val2 = widthheightlen + StrWidth_noloc (&glob->itxt, defaultstr) +
		   dimgadwidth + 8 + 8 + checkw + 8 + 4 + totaloff;
	    if (val2 > val) val = val2;
	}
    }
    else
    {
	winwidth = width1 + (num1-1) * 8 + totaloff;
	if (isfontreq) winwidth += 12;
	if (winwidth < 300) winwidth = 300;
	val = width2 + (num2-1) * 8 + totaloff;
    }

    if (val > winwidth) winwidth = val;
    if (winwidth > scrwidth) winwidth = scrwidth;
    if (isfontreq || (isfilereq && (glob->flags & FREQF_MULTISELECT)))
    {
        CheckGadgetsSize ((ULONG *)gadlen, (ULONG *)&width1, winwidth - totaloff, num1);
	rtSpread ((ULONG *)gadpos, (ULONG *)gadlen, width1, leftoff, winwidth - rightoff, num1);
    }

    CheckGadgetsSize ((ULONG *)gadlen + 4, (ULONG *)&width2, winwidth - totaloff, num2);
    
    rtSpread ((ULONG *)gadpos + 4, (ULONG *)gadlen + 4, width2, leftoff, winwidth - rightoff, num2);
 
    if (num2 == 2)
    {
	gadpos[7] = gadpos[5];
	gadlen[7] = gadlen[5];
    }


    if (isfilereq && (glob->flags & FREQF_MULTISELECT) && (width2 > width1))
    {
	for (i = 1; i < 4; i++)
	{
	    val = gadpos[i-1] + gadlen[i-1] + 8;
	    gadlen[i] += (gadpos[i] - val);
	    gadpos[i] = val;
	}
    }


    glob->boxleft = leftoff + 2; glob->boxtop = top + 2;
    glob->boxheight = glob->numentries * glob->entryheight;
    glob->boxright = winwidth - 21 - rightoff;

    /* create files gadget and scroller gadget */
    ng.ng_Flags = 0;
    InitNewGadget (&ng, leftoff + 4, top + 2,
		   winwidth - 26 - totaloff, glob->boxheight, NULL, FILES);

    gad = myCreateGadget (GENERIC_KIND, gad, &ng, TAG_END);
    if (gad)
    {
	gad->GadgetType |= GTYP_BOOLGADGET;
	gad->Flags |= GFLG_GADGHNONE;
	gad->Activation |= GACT_IMMEDIATE|GACT_FOLLOWMOUSE|GACT_RELVERIFY;
    }

    ng.ng_LeftEdge -= 4;
    ng.ng_Width += 8;
    ng.ng_TopEdge -= 2;
    ng.ng_Height += 4;
    ng.ng_GadgetID = 0;

    gad = my_CreateButtonGadget (gad, 0, &ng);

    if (gad) gad->Flags |= GFLG_GADGHNONE;

    InitNewGadget (&ng, winwidth - 18 - rightoff, top, 18, glob->boxheight + 4, NULL, FPROP);
    gad = glob->scrollergad = myCreateGadget (SCROLLER_KIND, gad, &ng, GTSC_Visible, glob->numentries,
    								       GTSC_Arrows, glob->fontheight + 1,
								       PGA_Freedom, LORIENT_VERT,
	    							       GTSC_Top, glob->buff->pos,
								       GTSC_Total, glob->buff->currentnum,
	    							       GA_RelVerify, TRUE,
								       GA_Immediate, TRUE,
								       TAG_END);
    top += glob->boxheight + 4 + spacing / 2;


    if (isfilereq || isvolreq)
    {

	/*
	 * File Requester gadgets
	 */

	glob->strgaduserdata.flags = USERFLAG_UP_DOWN_ARROW;
	glob->fnamegaduserdata.flags = USERFLAG_UP_DOWN_ARROW|USERFLAG_MATCH_FILE;
	glob->fnamegaduserdata.proc = ThisProcess();

	/* create string gadgets */
	if (createpatgad)
	{
	    str = GetStr (glob->catalog, MSG_PATTERN);
	    glob->patkey = KeyFromStr (str, '_');
	    val = StrWidth_noloc (&glob->itxt, str) + 8;
	    InitNewGadget (&ng, leftoff + 2 + val, top, winwidth - 2 - val - totaloff,
						    stdgadheight, NULL, PATSTR);
	    glob->patgad = gad = my_CreateStringGadget (gad, &ng, 60, glob->freq->patstr);
	    if (gad)
	    {
		glob->patgadstr = ((struct StringInfo *)gad->SpecialInfo)->Buffer;
		gad->UserData = &glob->strgaduserdata;
	    }
	    img = my_CreateGadgetLabelImage (img, &ng, str, leftoff + 2, top + 3, TEXTPEN);
	    top += ng.ng_Height + spacing / 2;
	}
	else glob->patgad = NULL;

	dotinfostr = GetStr (glob->catalog, MSG_DOT_INFO);
	dotinfowidth = StrWidth_noloc (&glob->itxt, dotinfostr) + 8;
	if (isfilereq)
	{
	    str = GetStr (glob->catalog, MSG_GET);
	    val = StrWidth_noloc (&glob->itxt, str) + 8;
	    
	    if (val > dotinfowidth) dotinfowidth = val;
	    
	    val = dotinfowidth;
	    InitNewGadget (&ng, winwidth - rightoff - val, top, val,
						    stdgadheight, str, GETDIR);
	    gad = my_CreateButtonGadget (gad, '_', &ng);
	}
	else val = 0;

	InitNewGadget (&ng, leftoff + led_off, top, winwidth - totaloff - val - led_off,
		       stdgadheight, NULL, DRAWERSTR);
	gad = glob->drawergad =
	      my_CreateStringGadget (gad, &ng, 255, glob->freq->dirname);
	      
	if (gad)
	{
	    glob->led_y = top + (stdgadheight - glob->led_h - 1) / 2;
	    glob->drawerstr = ((struct StringInfo *)gad->SpecialInfo)->Buffer;
	    if (!(glob->flags & FREQF_NOFILES))
		gad->UserData = &glob->strgaduserdata;
	}
		
	top += ng.ng_Height + spacing / 2;
	if (!(glob->flags & FREQF_NOFILES))
	{
	    ng.ng_TopEdge = top;
	    ng.ng_LeftEdge -= led_off;
	    ng.ng_Width += led_off;
	    ng.ng_GadgetID = FILESTR;
	    gad = glob->filegad = my_CreateStringGadget (gad, &ng, 107, NULL);
	    if ((glob->mainstrgad = gad))
		gad->UserData = &glob->fnamegaduserdata;

	    ng.ng_LeftEdge = winwidth - rightoff - val;
	    ng.ng_Width = dotinfowidth;
	    ng.ng_GadgetText = dotinfostr;
	    ng.ng_GadgetID = INFO;
	    gad = my_CreateButtonGadget (gad, '_', &ng);
	    if (gad)
	    {
		gad->Activation |= GACT_TOGGLESELECT;
		if (!glob->freq->hideinfo) gad->Flags |= GFLG_SELECTED;
	    }

	    top += ng.ng_Height + spacing;
	}
	else
	{
	    glob->filegad = NULL;
	    glob->mainstrgad = glob->drawergad;
	    top += spacing / 2;
	}

	if (glob->led_x)
	{
	    InitNewGadget (&ng, glob->led_x, glob->led_y, glob->led_w, glob->led_h, NULL, 0);
	    gad = myCreateGadget (TEXT_KIND, gad, &ng, GTTX_Border, TRUE, TAG_END);
	}
		
    } /* if (isfilereq || isvolreq) */
    else if (isfontreq)
    {

	/*
	 * Font Requester gadgets
	 */

	InitNewGadget (&ng, leftoff, top, winwidth - 65 - totaloff, stdgadheight, NULL, FILESTR);
	gad = glob->filegad = my_CreateStringGadget (gad, &ng, 107, NULL);
	glob->mainstrgad = glob->filegad;
	ng.ng_LeftEdge = winwidth - 57 - rightoff; ng.ng_Width = 57; ng.ng_GadgetID = FONTSIZE;
	gad = glob->drawergad = my_CreateIntegerGadget (gad, &ng, 4,
							glob->fontreq->Attr.ta_YSize, GACT_STRINGLEFT);
	top += ng.ng_Height + spacing;

	glob->fontdisplayleft = leftoff + 4; glob->fontdisplayright = winwidth - rightoff - 5;
	glob->fontdisplaytop = top + 2;
	InitNewGadget (&ng, leftoff, top, winwidth - totaloff, glob->sampleheight + 4, NULL, 0);
	gad = myCreateGadget (TEXT_KIND, gad, &ng, GTTX_Border, TRUE, TAG_END);
	top += glob->sampleheight + 4 + spacing;

	glob->fontstyle = glob->fontreq->Attr.ta_Style;
    }
    else
    {


	/*
	 * ScreenMode Requester gadgets
	 */

	top -= spacing / 2;
	InitNewGadget (&ng, leftoff, top, winwidth - totaloff, glob->fontheight + 4, NULL, 0);
	/* Remove this one please. ;) */
	gad = glob->modetxtgad = myCreateGadget (TEXT_KIND, gad, &ng,
						 GTTX_Text, (IPTR) glob->nameinfo.Name,
						 GTTX_Border, TRUE, TAG_END);
	top += ng.ng_Height + spacing;


	if (glob->flags & SCREQF_OVERSCANGAD)
	{
	    glob->overscankey = KeyFromStr (overscanstr, '_');
	    val = overscanstrlen + 8;
	    InitNewGadget (&ng, leftoff + 2 + val, top, winwidth - rightoff - leftoff - 2 - val,
						    stdgadheight, NULL, OVERSCN);
						    
	    gad = glob->overscangad = myCreateGadget (CYCLE_KIND, gad, &ng,
						      GTCY_Labels, (IPTR) glob->oscanlabs,
						      GTCY_Active, glob->overscantype,
						      TAG_END);
						      
	    img = my_CreateGadgetLabelImage (img, &ng, overscanstr,
					     leftoff + 2, top + 3, TEXTPEN);
	    top += stdgadheight + spacing;
	}


	if (glob->flags & SCREQF_SIZEGADS)
	{

	    /* Screen width and height gadgets */
	    glob->widthkey = KeyFromStr (widthstr, '_');

	    val = widthheightlen + 8 + leftoff + 2;
	    InitNewGadget (&ng, val, top, dimgadwidth, stdgadheight, NULL, SCRWIDTH);

	    gad = glob->widthgad = my_CreateIntegerGadget (gad, &ng, 5,
							   glob->width, GACT_STRINGLEFT);

	    img = my_CreateGadgetLabelImage (img, &ng, widthstr,

												    val - 8 - widthstrlen, top + 3, TEXTPEN);


	    ng.ng_Flags = PLACETEXT_RIGHT;

	    checktopoff = 3 - (checkh - glob->fontheight + 1) / 2;
	    ng.ng_TopEdge = top + (glob->os30 ? checktopoff : 1);
	    ng.ng_LeftEdge += dimgadwidth + 8;
	    ng.ng_Width = checkw;
	    ng.ng_Height = checkh;
	    ng.ng_GadgetText = defaultstr;
	    ng.ng_GadgetID = DEFWIDTH;

	    gad = glob->defwgad = myCreateGadget (CHECKBOX_KIND, gad, &ng, GTCB_Scaled, TRUE,
									   GTCB_Checked, glob->usedefwidth,
									   TAG_END);

	    top += stdgadheight + spacing / 2;

	    ng.ng_TopEdge = top + (glob->os30 ? checktopoff : 1);
	    ng.ng_GadgetID = DEFHEIGHT;

	    gad = glob->defhgad = myCreateGadget (CHECKBOX_KIND, gad, &ng, GTCB_Scaled, TRUE,
									   GTCB_Checked, glob->usedefheight,
									   TAG_END);


	    ng.ng_Flags = 0;

	    ng.ng_TopEdge = top;
	    ng.ng_LeftEdge = val;
	    ng.ng_Width = dimgadwidth;
	    ng.ng_Height = stdgadheight;
	    ng.ng_GadgetText = NULL;
	    ng.ng_GadgetID = SCRHEIGHT;
	    str = heightstr;
	    glob->heightkey = KeyFromStr (str, '_');

	    gad = glob->heightgad = my_CreateIntegerGadget (gad, &ng, 5,

														    glob->height, GACT_STRINGLEFT);

	    img = my_CreateGadgetLabelImage (img, &ng, str,
					     val - 8 - heightstrlen, top + 3, TEXTPEN);


	    top += ng.ng_Height + spacing;
	}

	if (glob->flags & SCREQF_DEPTHGAD)
	{


	    /* Colors gadget */
	    str = GetStr (glob->catalog, MSG_COLORS);
	    glob->depthkey = KeyFromStr (str, '_');
	    val = StrWidth_noloc (&glob->itxt, str) + 8;
	    val2 = StrWidth_noloc (&glob->itxt, GetStr (glob->catalog, MSG_MAX));
	    InitNewGadget (&ng, leftoff + 2 + val, top, StrWidth_noloc (&glob->itxt, "0000 "),
						    glob->fontheight + 3, NULL, 0);
	    BuildColStr (glob->currcolstr, glob->depth, glob->modeid);
	    gad = glob->currcolgad = myCreateGadget (TEXT_KIND, gad, &ng,
				    GTTX_Text, (IPTR) glob->currcolstr, GTTX_Clipped, TRUE,
				    (GadToolsBase->lib_Version >= 40) ? GTTX_Justification : TAG_IGNORE, GTJ_RIGHT, TAG_END);
	    ng.ng_LeftEdge += ng.ng_Width + 8;
	    val = StrWidth_noloc (&glob->itxt, "0000 ");
	    ng.ng_Width = winwidth - 22 - rightoff - ng.ng_LeftEdge - val - val2;
	    ng.ng_GadgetID = DEPTH;

	    gad = glob->depthgad = myCreateGadget (SLIDER_KIND, gad, &ng,
						   GA_RelVerify, TRUE, GA_Immediate , TRUE,
						   GTSL_Min, glob->currmindepth, GTSL_Max, glob->currmaxdepth,
						   GTSL_Level, glob->depth,
						   GA_Disabled, (glob->currmindepth == glob->currmaxdepth), TAG_END);

	    img = my_CreateGadgetLabelImage (img, &ng, str,
				    leftoff + 2, top + 2, TEXTPEN);

	    ng.ng_LeftEdge += ng.ng_Width + val2 + 20;
	    ng.ng_GadgetText = GetStr (glob->catalog, MSG_MAX);
	    ng.ng_Width = val;
	    BuildColStr (glob->maxcolstr, glob->currmaxdepth, 0);
	    gad = glob->maxcolgad = myCreateGadget (TEXT_KIND, gad, &ng,
						    GTTX_Text, (IPTR) glob->maxcolstr, GTTX_Clipped, TRUE,
						    (GadToolsBase->lib_Version >= 40) ? GTTX_Justification : TAG_IGNORE, GTJ_RIGHT, TAG_END);

	    top += ng.ng_Height + spacing;
	}

	if (glob->flags & SCREQF_AUTOSCROLLGAD)
	{

	    str = GetStr (glob->catalog, MSG_AUTOSCROLL);
	    glob->gadkey[CHECKBOX_AUTOSCROLL] = KeyFromStr (str, '_');
	    val = StrWidth_noloc (&glob->itxt, str) + 8;
	    InitNewGadget (&ng, leftoff + 2 + val, top + (checkskip - checkh + 1) / 2, checkw,
			   checkh, NULL, AUTOSCR);
	    gad = glob->checkboxgad[CHECKBOX_AUTOSCROLL] =
		 myCreateGadget (CHECKBOX_KIND, gad, &ng, GTCB_Scaled, TRUE,
							  GTCB_Checked, glob->autoscroll, TAG_END);
	    img = my_CreateGadgetLabelImage (img, &ng, str, leftoff + 2,
					     top + (checkskip - glob->fontheight + 1) / 2, TEXTPEN);
	    top += checkskip + spacing;
	}

	ng.ng_Flags = 0;
    }


    /* create buttons */
    buttonheight = createstyle ? (checkskip + 4) : (glob->fontheight + 6);
    ng.ng_TextAttr = &glob->font;
    checkboxcount = CHECKBOX_BOLD;

    for (i = 0; i < 8; i++)
    {

	if (i == num1)
	{
	    if (createstyle || (isfilereq && (glob->flags & FREQF_MULTISELECT)))
		top += buttonheight + spacing;
	    if (createstyle) buttonheight = (glob->fontheight + 6);
	}

	ng.ng_TopEdge = top;
	ng.ng_LeftEdge = gadpos[i];
	ng.ng_Width = gadlen[i];
	ng.ng_Height = buttonheight;
	ng.ng_GadgetText = gadtxt[i];
	ng.ng_GadgetID = INFO + i;
	
	if (!isfilereq)
	{
	    if (isfontreq)
	    {
		if ((i == 3) || (i == 5) || (i == 6)) continue;
		if (i < 3)
		{
		    if (createstyle)
		    {

			check = FALSE;
			
			switch (i)
			{
			    case 0: check = (glob->fontstyle & FSF_BOLD); break;
			    case 1: check = (glob->fontstyle & FSF_ITALIC); break;
			    case 2: check = (glob->fontstyle & FSF_UNDERLINED); break;
			}
			
			glob->gadkey[i] = KeyFromStr (gadtxt[i], '_');
			ng.ng_Width = checkw;
			ng.ng_Height = checkh;
			ng.ng_GadgetID = BOLD + i;
			ng.ng_GadgetText = NULL;
			ng.ng_LeftEdge += (2 - i);
			img = my_CreateGadgetLabelImage (img, &ng, gadtxt[i],
						ng.ng_LeftEdge, top + 2 + (checkskip - glob->fontheight + 1) / 2, TEXTPEN);
			ng.ng_LeftEdge += gadlen[i] - checkw;
			ng.ng_TopEdge += 2 + (checkskip - checkh + 1) / 2;
			glob->checkboxgad[checkboxcount++] = gad = myCreateGadget
			    (CHECKBOX_KIND, gad, &ng, GTCB_Checked, check, GTCB_Scaled, TRUE, TAG_END);
		    }
		    continue;
		    
		}
		
	    } /* if (isfontreq) */
	    else if (i < 4 || i == 5 || i == 6) continue;

	} /* if (!isfilereq) */

	if (i == 0)
	{
	    if (glob->flags & FREQF_MULTISELECT)
	    {
		ng.ng_GadgetText = NULL;
		gad = glob->numselectedgad =
		      myCreateGadget (TEXT_KIND, gad, &ng, GTTX_Border, TRUE, TAG_END);
	    }
	}
	else
	{
	    if ((i > 3) || (glob->flags & FREQF_MULTISELECT))
		gad = my_CreateButtonGadget (gad, (i == 4) ? glob->underchar : '_', &ng);
	    if (i == 4) glob->okgad = gad;
	    if (i == 7) glob->cancelgad = gad;
	}

    } /* for (i = 0; i < 8; i++) */


    ng.ng_LeftEdge = ng.ng_TopEdge = ng.ng_Width = ng.ng_Height = 0;
    ng.ng_GadgetText = NULL;
    gad = myCreateGadget (GENERIC_KIND, gad, &ng, TAG_END);
    
    if (gad)
    {
	gad->GadgetType |= GTYP_BOOLGADGET;
	gad->Flags |= GFLG_GADGIMAGE|GFLG_GADGHNONE;
	gad->GadgetRender = (APTR)glob->labelimages.NextImage;
	
	if (glob->numselectedgad)
	{
	    glob->selitxt.LeftEdge = glob->numselectedgad->LeftEdge + 8;
	    glob->selitxt.TopEdge = glob->numselectedgad->TopEdge + 3;
	    glob->selitxt.DrawMode = JAM1;
	    glob->selitxt.FrontPen = glob->pens[TEXTPEN];
	    glob->selitxt.IText = gadtxt[0];
	    glob->selitxt.ITextFont = &glob->font;
	    gad->GadgetText = &glob->selitxt;
	    glob->numselectedoff = IntuiTextLength (&glob->selitxt);
	}
    }

    /* is the window being rebuild or do we open it for the first time? */
    winheight = top + buttonheight + spacing + BottomBorderHeight (glob->scr);
    if (!glob->reqwin)
    {
	glob->newreqwin.Width = winwidth;
	glob->newreqwin.Height = winheight;
	glob->newreqwin.IDCMPFlags = glob->shareidcmp ? 0 : REQ_IDCMP;
	glob->newreqwin.Flags = WFLG_SIZEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET
			       |WFLG_CLOSEGADGET|WFLG_SIZEBBOTTOM|WFLG_ACTIVATE
			       |WFLG_RMBTRAP|WFLG_SIMPLE_REFRESH;
	glob->newreqwin.DetailPen = ( UBYTE ) -1;  /* glob->pens[BACKGROUNDPEN]; */
	glob->newreqwin.BlockPen = ( UBYTE ) -1;   /* glob->pens[SHADOWPEN]; */
	glob->newreqwin.Title = glob->title;
	glob->newreqwin.LeftEdge = glob->leftedge;
	glob->newreqwin.TopEdge = glob->topedge;
	glob->newreqwin.MinWidth = glob->newreqwin.MaxWidth = winwidth;
	glob->newreqwin.MinHeight = winheight;
	
	if (glob->numentries > 3)
		glob->newreqwin.MinHeight -= (glob->numentries - 3) * glob->entryheight;
		
	glob->newreqwin.MaxHeight =
			winheight + (50 - glob->numentries) * glob->entryheight;

	reqpos = CheckReqPos (glob->reqpos, reqdefnum, &glob->newreqwin);
	
	if (reqpos == REQPOS_POINTER)
	{
	    glob->newreqwin.LeftEdge = -winwidth / 2;
	    glob->newreqwin.TopEdge = -winheight / 2;
	}
	
	rtSetReqPosition (reqpos, &glob->newreqwin, glob->scr, glob->prwin);

	glob->zoom[2] = glob->newreqwin.MinWidth;
	glob->zoom[3] = glob->newreqwin.MinHeight;
	
	if (!img || !gad || !(glob->reqwin = OpenWindowBF (&glob->newreqwin,
	    &glob->backfillhook, glob->pens, &glob->rpmask, glob->zoom, FALSE)))
	{	 
	    my_FreeGadgets (glob->buttoninfo.glist);
	    glob->buttoninfo.glist = NULL;
	    my_FreeLabelImages (&glob->labelimages);
	    glob->labelimages.NextImage = NULL;
	    
	    if (gad && img && (glob->numentries > 3))
	    {
		glob->numentries--;
		goto retryopenwindow;
	    }
	    
	    return (FALSE);
	}
	
	glob->buttoninfo.win = glob->reqwin;

	glob->winaddr = (struct Window **)&(ThisProcess()->pr_WindowPtr);
	glob->oldwinptr = *glob->winaddr;
	*glob->winaddr = glob->reqwin;

	if (glob->shareidcmp)
	{
	    glob->reqwin->UserPort = glob->prwin->UserPort;
	    ModifyIDCMP (glob->reqwin, REQ_IDCMP);
	}
	
	glob->fnamegaduserdata.msgport = glob->reqwin->UserPort;
	glob->timereq.tr_node.io_Message.mn_ReplyPort = glob->reqwin->UserPort;

	if ((WorkbenchBase = OpenLibrary ("workbench.library", 0)))
	{
	    if ((glob->appwinport = CreateMsgPort()))
	    {
		glob->appwindow = AddAppWindowA (0, 0, glob->reqwin, glob->appwinport, NULL);
	    }
	}

	maxpen = 0;
	for (i = 0; scrollpens[i] >= 0; i++)
		if (glob->pens[scrollpens[i]] > maxpen)
			maxpen = glob->pens[scrollpens[i]];
	mask = 1;
	while (mask < maxpen)
	{
	    mask <<= 1;
	    mask |= 1;
	}
	glob->entrymask = mask;
    }
    else
    {
	if (!img || !gad)
	{
	    my_FreeGadgets (glob->buttoninfo.glist);
	    glob->buttoninfo.glist = NULL;
	    my_FreeLabelImages (&glob->labelimages);
	    glob->labelimages.NextImage = NULL;
	    return (FALSE);
	}
	
	RefreshWindowFrame (glob->reqwin);
	SetAPen (glob->reqrp, glob->pens[BACKGROUNDPEN]);
	RectFill (glob->reqrp, glob->reqwin->BorderLeft,
			       glob->reqwin->BorderTop,
			       glob->reqwin->Width - glob->reqwin->BorderRight - 1,
			       glob->reqwin->Height - glob->reqwin->BorderBottom - 1);
    }

    ((FI_REQ)glob->req)->ReqHeight = winheight;

    if (glob->filegad)
	glob->filestr = ((struct StringInfo *)glob->filegad->SpecialInfo)->Buffer;

    glob->reqrp = glob->reqwin->RPort;
    SetFont (glob->reqrp, glob->reqfont);
    AddGList (glob->reqwin, glob->buttoninfo.glist, -1L, -1L, NULL);
    RefreshGList (glob->buttoninfo.glist, glob->reqwin, NULL, -1L);
    GT_RefreshWindow (glob->reqwin, NULL);

    glob->activegadget = glob->mainstrgad;
    
    return (1);
}

/****************************************************************************************/
