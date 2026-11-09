/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    GadTools gadget creation functions
*/

/****************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/intuition.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>

#if 0
#define SDEBUG 0
#define DEBUG 0
#endif
#include <aros/debug.h>

#include "gadtools_intern.h"

/****************************************************************************************/

#define EG(x) ((struct ExtGadget *)(x))

/****************************************************************************************/

/* Number of entries of a tag array */
#define NUMTAGS(t)  (sizeof(t) / sizeof(struct TagItem))

/****************************************************************************************/

/* NextTagItem() for the makexxx() functions. If the tag that was found is the
   tag of one of the entries of tags[] selected by mask (bit n = tags[n]), its
   data is stored there. The tag is returned as well, so that the caller can
   handle the tags which need more than that. */
static struct TagItem *gt_nexttag(struct GadToolsBase_intern *GadToolsBase,
                                  struct TagItem **tstate,
                                  struct TagItem *tags, ULONG mask)
{
    struct TagItem *tag = NextTagItem(tstate);

    if (tag)
    {
        for (; mask; mask >>= 1, tags++)
        {
            if ((mask & 1) && (tags->ti_Tag == tag->ti_Tag))
            {
                tags->ti_Data = tag->ti_Data;
                break;
            }
        }
    }

    return tag;
}

/****************************************************************************************/

/* Create the gadget object and mark it as GadTools gadget */
static struct Gadget *gt_newgadget(struct GadToolsBase_intern *GadToolsBase,
                                   Class *cl, struct TagItem *tags)
{
    struct Gadget *obj = (struct Gadget *) NewObjectA(cl, NULL, tags);

    if (obj)
    {
        obj->GadgetType |= GTYP_GADTOOLS;
    }
    return obj;
}

/****************************************************************************************/

/* Set a GA_TextAttr tag item. Be sure not to pass GA_TextAttr, NULL */
static void gt_settattr(struct TagItem *tag, struct TextAttr *tattr)
{
    if (tattr)
        tag->ti_Data = (IPTR)tattr;
    else
        tag->ti_Tag = TAG_IGNORE;
}

/****************************************************************************************/

/* Get the bounding box of the gadget from the standard tags and shrink the
   gadget itself by the given border.

   There are always GA_Left, GA_Top, GA_Width and GA_Height tags, and they are
   the first four, thanks to creategadgeta.c (and makelistview()) ! */
static void gt_boundsinset(struct IBox *bbox, struct TagItem stdgadtags[],
                           WORD dx, WORD dy)
{
    /* if there is a bounding box the label position
       will be calculated based on this box and not
       the gadget coords */

    bbox->Left   = stdgadtags[TAG_Left].ti_Data;
    bbox->Top    = stdgadtags[TAG_Top].ti_Data;
    bbox->Width  = stdgadtags[TAG_Width].ti_Data;
    bbox->Height = stdgadtags[TAG_Height].ti_Data;

    stdgadtags[TAG_Left].ti_Data   += dx;
    stdgadtags[TAG_Top].ti_Data    += dy;
    stdgadtags[TAG_Width].ti_Data  -= dx * 2;
    stdgadtags[TAG_Height].ti_Data -= dy * 2;
}

/****************************************************************************************/

struct Gadget *makebutton(struct GadToolsBase_intern *GadToolsBase,
                          struct TagItem stdgadtags[],
                          struct VisualInfo *vi,
                          struct TextAttr *tattr,
                          struct TagItem *taglist,
                          ULONG kind)
{
    struct TagItem tags[] =
    {
        {GA_Disabled    , FALSE         },
        {GA_Immediate   , FALSE         },
        {GA_RelVerify   , TRUE          },
        {TAG_MORE       , (IPTR) NULL   }
    };

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GA_Immediate, FALSE, taglist);
    tags[3].ti_Data = (IPTR) stdgadtags;

    return gt_newgadget(GadToolsBase, GadToolsBase->buttonclass, tags);
}

/****************************************************************************************/

struct Gadget *makecheckbox(struct GadToolsBase_intern *GadToolsBase,
                            struct TagItem stdgadtags[],
                            struct VisualInfo *vi,
                            struct TextAttr *tattr,
                            struct TagItem *taglist,
                            ULONG kind)
{
    struct TagItem tags[] =
    {
        {GA_Disabled    , FALSE         },
        {GTCB_Checked   , FALSE         },
        {TAG_MORE       , (IPTR)NULL    }
    };

    EnterFunc(bug("makecheckbox()\n"));

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GTCB_Checked, FALSE, taglist);
    tags[2].ti_Data = (IPTR) stdgadtags;

    if (!GetTagData(GTCB_Scaled, FALSE, taglist)) {
        stdgadtags[TAG_Width].ti_Data = CHECKBOX_WIDTH;
        stdgadtags[TAG_Height].ti_Data = CHECKBOX_HEIGHT;
    }

    ReturnPtr("makecheckbox()", struct Gadget *,
              gt_newgadget(GadToolsBase, GadToolsBase->checkboxclass, tags));
}

/****************************************************************************************/

struct Gadget *makecycle(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist,
                         ULONG kind)
{
    struct TagItem tags[] =
    {
        {GA_Disabled    , FALSE         }, /* 0 */
        {GTCY_Labels    , FALSE         }, /* 1 */
        {GTCY_Active    , 0             }, /* 2 */
        {GA_RelVerify   , TRUE          }, /* 3 */
        {GA_TextAttr    , 0             }, /* 4 */
        {TAG_MORE       , (IPTR)NULL    }  /* 5 */
    };

    EnterFunc(bug("makecycle()\n"));

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GTCY_Labels, FALSE, taglist);
    tags[2].ti_Data = GetTagData(GTCY_Active, 0, taglist);

    gt_settattr(&tags[4], tattr);

    tags[5].ti_Data = (IPTR) stdgadtags;

    ReturnPtr("makecycle()", struct Gadget *,
              gt_newgadget(GadToolsBase, GadToolsBase->cycleclass, tags));
}

/****************************************************************************************/

struct Gadget *makemx(struct GadToolsBase_intern *GadToolsBase,
                      struct TagItem stdgadtags[],
                      struct VisualInfo *vi,
                      struct TextAttr *tattr,
                      struct TagItem *taglist,
                      ULONG kind)
{
    int labels = 0;
    STRPTR *labellist;
    struct TagItem *tag, tags[] =
    {
        {GA_Disabled            , FALSE                 }, /* 0 */
        {GTMX_Labels            , (IPTR) NULL           }, /* 1 */
        {GTMX_Active            , 0                     }, /* 2 */
        {GTMX_Spacing           , 1                     }, /* 3 */
        {GTMX_TickHeight        , MX_HEIGHT             }, /* 4 */
        {GTMX_TickLabelPlace    , GV_LabelPlace_Right   }, /* 5 */
        {GA_TextAttr            , 0                     }, /* 6 */
        {TAG_MORE               , (IPTR) NULL           }  /* 7 */
    };

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    labellist = (STRPTR *) GetTagData(GTMX_Labels, (IPTR) NULL, taglist);
    if (!labellist)
        return NULL;
    tags[1].ti_Data = (IPTR) labellist;
    tags[2].ti_Data = GetTagData(GTMX_Active, 0, taglist);
    tags[3].ti_Data = GetTagData(GTMX_Spacing, 1, taglist);
    if (GetTagData(GTMX_Scaled, FALSE, taglist))
        tags[4].ti_Data = stdgadtags[TAG_Height].ti_Data;
    else
        stdgadtags[TAG_Width].ti_Data = MX_WIDTH;
    switch (stdgadtags[TAG_LabelPlace].ti_Data & 0x1f)
    {
    case GV_LabelPlace_Left:
        tags[5].ti_Data = GV_LabelPlace_Left;
        break;
    case GV_LabelPlace_Above:
        tags[5].ti_Data = GV_LabelPlace_Above;
        break;
    case GV_LabelPlace_Below:
        tags[5].ti_Data = GV_LabelPlace_Below;
        break;
    }

    gt_settattr(&tags[6], tattr);

    tags[7].ti_Data = (IPTR) stdgadtags;

    tag = FindTagItem(GTMX_TitlePlace, taglist);
    if (tag)
    {
        switch (tag->ti_Data)
        {
        case PLACETEXT_LEFT:
            stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Left;
            break;
        case PLACETEXT_RIGHT:
            stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Right;
            break;
        case PLACETEXT_ABOVE:
            stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Above;
            break;
        case PLACETEXT_BELOW:
            stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Below;
            break;
        default:
            freeitext(GadToolsBase,
                      (struct IntuiText *)stdgadtags[TAG_IText].ti_Data);
            stdgadtags[TAG_IText].ti_Data = (IPTR)NULL;
            break;
        }
    } else
    {
        freeitext(GadToolsBase,
                  (struct IntuiText *)stdgadtags[TAG_IText].ti_Data);
        stdgadtags[TAG_IText].ti_Data = (IPTR)NULL;
    }

    while (labellist[labels])
        labels++;

    return gt_newgadget(GadToolsBase, GadToolsBase->mxclass, tags);
}

/****************************************************************************************/

static const struct TagItem palettetags[] =
{
    {GA_RelVerify,          TRUE    }, /* 0 */
    {GA_Disabled,           FALSE   }, /* 1 */
    {GTPA_Depth,            1       }, /* 2 */
    {GTPA_Color,            0       }, /* 3 */
    {GTPA_ColorOffset,      0       }, /* 4 */
    {GTPA_IndicatorWidth,   0       }, /* 5 */
    {GTPA_IndicatorHeight,  0       }, /* 6 */
    {GTPA_NumColors,        2       }, /* 7 */
    {GTPA_ColorTable,       0       }, /* 8 */
    {TAG_MORE,              (IPTR)NULL      }
};

struct Gadget *makepalette(struct GadToolsBase_intern *GadToolsBase,
                           struct TagItem stdgadtags[],
                           struct VisualInfo *vi,
                           struct TextAttr *tattr,
                           struct TagItem *taglist,
                           ULONG kind)
{
    struct TagItem *tag, tags[NUMTAGS(palettetags)];

    memcpy(tags, palettetags, sizeof(palettetags));

    while ((tag = gt_nexttag(GadToolsBase, (struct TagItem **)&taglist, tags, 0x1fe)))
    {
        /* Note: GTPA_NumColors overrides GTPA_Depth tag! */

        if (tag->ti_Tag == GTPA_Depth)
            tags[7].ti_Data = 1L << tag->ti_Data;

    } /* while (iterate taglist) */

    tags[9].ti_Data = (IPTR)stdgadtags;

    return gt_newgadget(GadToolsBase, GadToolsBase->paletteclass, tags);
}

/****************************************************************************************/

/* TEXT_KIND and NUMBER_KIND differ only in their tags */

static const struct TagItem texttags[] =
{
    {GTTX_Text              , 0                     },
    {GTTX_CopyText          , FALSE                 },
    {GTTX_Clipped           , FALSE                 },
    {GTTX_Border            , FALSE                 },
    {GTTX_FrontPen          , TEXTPEN               },
    {GTTX_BackPen           , BACKGROUNDPEN         },
    {GTTX_Justification     , GTJ_LEFT              },
    {GTA_Text_Format        , (IPTR)"%s"            },
    {GA_TextAttr            , (IPTR)NULL            },
    {GTA_GadgetKind         , TEXT_KIND             },
    {TAG_MORE               , (IPTR) NULL           }
};

static const struct TagItem numbertags[] =
{
    {GTNM_Number            , 0             },
    {GTNM_Format            , (IPTR)"%ld"   },
    {GTNM_Clipped           , FALSE         },
    {GTNM_Border            , FALSE         },
    {GTNM_FrontPen          , TEXTPEN       },
    {GTNM_BackPen           , BACKGROUNDPEN },
    {GTNM_Justification     , GTJ_CENTER    },
    {GTNM_MaxNumberLen      , 100           },
    {GA_TextAttr            , (IPTR)NULL    },
    {GTA_GadgetKind         , NUMBER_KIND   },
    {TAG_MORE               , (IPTR) NULL   }
};

struct Gadget *maketext(struct GadToolsBase_intern *GadToolsBase,
                        struct TagItem stdgadtags[],
                        struct VisualInfo *vi,
                        struct TextAttr *tattr,
                        struct TagItem *taglist,
                        ULONG kind)
{
    BOOL cliptag_found = FALSE;
    struct TagItem *tag, tags[NUMTAGS(texttags)];
    ULONG mask = 0x7f;      /* GTTX_Text ... GTTX_Justification */

    if (kind == NUMBER_KIND)
    {
        memcpy(tags, numbertags, sizeof(numbertags));
        mask = 0xff;        /* GTNM_Number ... GTNM_MaxNumberLen */
    }
    else
    {
        memcpy(tags, texttags, sizeof(texttags));
    }

    while ((tag = gt_nexttag(GadToolsBase, (struct TagItem **)&taglist, tags, mask)))
    {
        if (tag->ti_Tag == GTTX_Clipped)    /* same as GTNM_Clipped */
            cliptag_found = TRUE;

    } /* while (iterate taglist) */

    /* if GTTX_Clipped/GTNM_Clipped was not specified then the default
       value is the GTTX_Border/GTNM_Border value */
    
    if (!cliptag_found) tags[2].ti_Data = tags[3].ti_Data;
    
    gt_settattr(&tags[8], tattr);
    tags[10].ti_Data = (IPTR)stdgadtags;

    return gt_newgadget(GadToolsBase, GadToolsBase->textclass, tags);
}

/****************************************************************************************/

/* This MUST be global, since the gadgetclass doesn't copy ICA_MAPPINGs */
const struct TagItem slider2level[] =
{
    {GTSL_Level,        GTNM_Number},
    {TAG_DONE }
};

/****************************************************************************************/

static const struct TagItem slidertags[] =
{
    {GA_Disabled        , FALSE             },
    {GA_RelVerify       , TRUE              },      /* Georg S.: was false */
    {GA_Immediate       , TRUE              },      /* Georg S.: was false */
    {GTSL_Min           , 0                 },
    {GTSL_Max           , 15                },
    {GTSL_Level         , 0                 },
    {PGA_Freedom        , FREEHORIZ         },
    {PGA_Borderless     , TRUE              },
#ifdef __MORPHOS__
    {PGA_NewLook        , FALSE             },
#else
    {PGA_NewLook        , TRUE              },
#endif
    {GA_Bounds          , (IPTR)NULL        },      /* 9: &bbox */
    {GA_FollowMouse     , TRUE              },
    {PGA_NotifyBehaviour, PG_BEHAVIOUR_NICE },
    {PGA_RenderBehaviour, PG_BEHAVIOUR_NICE },
    {TAG_MORE           , (IPTR)NULL        }
};

static const struct TagItem sliderleveltags[] =
{
     {GA_Left               , 0             },
     {GA_Top                , 0             },
     {GA_Width              , 0             },
     {GA_Height             , 0             },
     {GA_TextAttr           , (IPTR)NULL    },      /* 4: tattr */
     {GTNM_Format           , (IPTR)NULL    },
     {GTNM_Justification    , GTJ_LEFT      },
     {GTA_Text_DispFunc     , (IPTR)NULL    },
     {GA_Next               , (IPTR)NULL    },
     {GA_DrawInfo           , (IPTR)NULL    },
     {GTNM_Number           , 0             },
     {GTA_GadgetKind        , SLIDER_KIND   },
     {GA_Previous           , (IPTR)NULL    },
     {GTA_Text_ParentGadget , 0             },
     {TAG_DONE                              }
};

struct Gadget *makeslider(struct GadToolsBase_intern *GadToolsBase,
                          struct TagItem stdgadtags[],
                          struct VisualInfo *vi,
                          struct TextAttr *tattr,
                          struct TagItem *taglist,
                          ULONG kind)
{

    struct TagItem *tag;
    struct IBox bbox;
    
    struct TagItem stags[NUMTAGS(slidertags)];
    struct TagItem ltags[NUMTAGS(sliderleveltags)];

    STRPTR lformat = NULL;
    WORD lmaxlen = 0;
    LONG lmaxpixellen = 0L;
    UBYTE lplace = PLACETEXT_LEFT;
    WORD level;
    
    struct Gadget *slidergad = NULL, *levelgad = NULL;

    memcpy(stags, slidertags, sizeof(slidertags));
    stags[9].ti_Data = (IPTR)&bbox;
    memcpy(ltags, sliderleveltags, sizeof(sliderleveltags));
    ltags[4].ti_Data = (IPTR)tattr;

    /* Parse tags. The slider tags GA_Disabled ... GTSL_Level are stored by gt_nexttag() */
    while ((tag = gt_nexttag(GadToolsBase, (struct TagItem **)&taglist, stags, 0x3f)))
    {
        IPTR tidata = tag->ti_Data;
            
        switch (tag->ti_Tag)
        {
            case PGA_Freedom:
                if (tidata == LORIENT_HORIZ)
                    stags[6].ti_Data = FREEHORIZ;
                else
                    stags[6].ti_Data = FREEVERT;
                break;

            /* Level tags */
            case GTSL_LevelFormat:   lformat          = (STRPTR)tidata; break;
            case GTSL_MaxLevelLen:   lmaxlen          = (UWORD)tidata;  break;
            case GTSL_MaxPixelLen:   lmaxpixellen     = (ULONG)tidata;  break;
            case GTSL_LevelPlace:    lplace           = (UBYTE)tidata;  break;
            case GTSL_Justification: ltags[6].ti_Data = tidata;         break;
            case GTSL_DispFunc:      ltags[7].ti_Data = tidata;         break;

        }
            
    } /* while (iterate taglist) */

    level = stags[5].ti_Data;
    
    gt_boundsinset(&bbox, stdgadtags, BORDERPROPSPACINGX, BORDERPROPSPACINGY);

    /* Create slider gadget */
    stags[13].ti_Data = (IPTR)stdgadtags;
    slidergad = gt_newgadget(GadToolsBase, GadToolsBase->sliderclass, stags);

    if (!slidergad)
        return (NULL);

    if (lformat || lmaxlen || lmaxpixellen)
    {
        WORD x = 0, y = 0;
        UWORD ysize = 8;
        
        struct TagItem lntags[] =
        {
             {ICA_TARGET, (IPTR)NULL    },
             {ICA_MAP   , (IPTR)NULL    },
             {TAG_DONE                  }
        };
            
        /* Set some defaults */
        if (!lformat)
            lformat = "%ld";
        if (!lmaxlen)
            lmaxlen = 2;
        if (!lmaxpixellen)
        {
            struct TextFont *font;
            UWORD xsize;
            
            ysize = vi->vi_dri->dri_Font->tf_YSize;
            xsize = vi->vi_dri->dri_Font->tf_XSize;

            if (tattr)
            {
                font = OpenFont(tattr);
                if (font)
                {
                    ysize = font->tf_YSize;
                    xsize = font->tf_XSize;
                    CloseFont(font);
                }
                else /* If no valid tattr */
                    ltags[4].ti_Tag = TAG_IGNORE;
            }
            else /* If no valid tattr */
                ltags[4].ti_Tag = TAG_IGNORE;

            lmaxpixellen = lmaxlen * xsize;
                
            ltags[4].ti_Data = (IPTR)tattr;
                
        } /* if (!lmaxpixellen) */
        
        switch (lplace)
        {
            case PLACETEXT_LEFT:
                x = slidergad->LeftEdge - BORDERPROPSPACINGX - lmaxpixellen - 4;
                y = slidergad->TopEdge + (slidergad->Height - ysize) / 2 + 1;
                break;
            case PLACETEXT_RIGHT:
                x = slidergad->LeftEdge + slidergad->Width + BORDERPROPSPACINGX + 5;
                y = slidergad->TopEdge  + (slidergad->Height - ysize) / 2 + 1;
                break;
            case PLACETEXT_ABOVE:
                x = slidergad->LeftEdge - (lmaxpixellen - slidergad->Width) / 2;
                y = slidergad->TopEdge  - BORDERPROPSPACINGY - ysize - 2;
                break;
            case PLACETEXT_BELOW:
                x = slidergad->LeftEdge - (lmaxpixellen - slidergad->Width) / 2;
                y = slidergad->TopEdge  + slidergad->Height + BORDERPROPSPACINGY + 3;
                break;
        }

        /* Create the levelobj */
        
        ltags[0].ti_Data = (IPTR)x;
        ltags[1].ti_Data = (IPTR)y;
        ltags[2].ti_Data = (IPTR)lmaxpixellen;
        ltags[3].ti_Data = (IPTR)ysize;
        ltags[5].ti_Data = (IPTR)lformat;
        ltags[8].ti_Data = (IPTR)slidergad;
        ltags[9].ti_Data = (IPTR)vi->vi_dri;
        ltags[10].ti_Data = (IPTR)level;
        ltags[12].ti_Data = stdgadtags[TAG_Previous].ti_Data;
        ltags[13].ti_Data = (IPTR)slidergad;

        levelgad = gt_newgadget(GadToolsBase, GadToolsBase->textclass, ltags);
        if (!levelgad)
        {
            DisposeObject((Object *)slidergad);
            return (NULL);
        }
        
        /* Set up a notification from the slider to the level */
        lntags[0].ti_Data = (IPTR)levelgad;
        lntags[1].ti_Data = (IPTR)slider2level;
        SetAttrsA((Object *)slidergad, lntags);

        return (levelgad);
            
    } /* if (slider should have a level attached) */
    
    return (slidergad);

}

/****************************************************************************************/

#ifdef SDEBUG
#   undef SDEBUG
#endif
#ifdef DEBUG
#   undef DEBUG
#endif
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

static const struct TagItem scrollertags[] =
{
    {GTSC_Top           , 0                 },
    {GTSC_Total         , 0                 },
    {GTSC_Visible       , 2                 },
    {PGA_Freedom        , FREEHORIZ         },
    {GA_Disabled        , FALSE             },
    {GA_RelVerify       , TRUE              },
    {GA_Immediate       , TRUE              },
    {GTA_GadgetKind     , SCROLLER_KIND     },
    {PGA_Borderless     , TRUE              },
#ifdef __MORPHOS__
    {PGA_NewLook        , FALSE             },
#else
    {PGA_NewLook        , TRUE              },
#endif
    {GA_Bounds          , (IPTR)NULL        },      /* 10: &bbox */
    {GA_FollowMouse     , TRUE              },
    {PGA_NotifyBehaviour, PG_BEHAVIOUR_NICE },
    {PGA_RenderBehaviour, PG_BEHAVIOUR_NICE },
    {TAG_MORE           , (IPTR)NULL        }
};

static const struct TagItem scrollerarrowtags[] =
{
    {GA_Left            , 0             },
    {GA_Top             , 0             },
    {GA_Width           , 0             },
    {GA_Height          , 0             },
    {GTA_Arrow_Type     , 0             },
    {GA_DrawInfo        , (IPTR)NULL    },
    {GA_Next            , (IPTR)NULL    },
    {GTA_Arrow_Scroller , (IPTR)NULL    },
    {GA_RelVerify       , TRUE          },
    {GA_Immediate       , TRUE          },
    {GA_ID              , 0             },
    {GTA_GadgetKind     , 0             },          /* 11: arrowkind */
    {GA_Previous        , (IPTR)NULL    },
    {TAG_DONE                           }
};

struct Gadget *makescroller(struct GadToolsBase_intern *GadToolsBase,
                            struct TagItem stdgadtags[],
                            struct VisualInfo *vi,
                            struct TextAttr *tattr,
                            struct TagItem *taglist,
                            ULONG kind)
{
    struct Gadget *scroller = NULL,
                  *arrow_dec = NULL,
                  *arrow_inc = NULL ;
    struct IBox bbox;

    struct TagItem *tag, stags[NUMTAGS(scrollertags)];
    
    UWORD freedom = FREEHORIZ; /* default */
    WORD arrowdim = 0 /* -1*/, arrowkind = SCROLLER_KIND;
    
    EnterFunc(bug("makescroller(stdgadtags=%p, vi=%p, taglist = %p)\n",
                stdgadtags, vi, taglist));

    memcpy(stags, scrollertags, sizeof(scrollertags));
    stags[10].ti_Data = (IPTR)&bbox;

    /* GTSC_Top, GTSC_Total, GTSC_Visible, GA_Disabled, GA_RelVerify and
       GA_Immediate are stored by gt_nexttag() */
    while ((tag = gt_nexttag(GadToolsBase, (struct TagItem **)&taglist, stags, 0x77)))
    {
        IPTR tidata = tag->ti_Data;
        
        switch (tag->ti_Tag)
        {
            case PGA_Freedom:
                if (tidata == LORIENT_HORIZ)
                    freedom = stags[3].ti_Data = FREEHORIZ;
                else
                    freedom = stags[3].ti_Data = FREEVERT;
                break;

            case GTSC_Arrows:   arrowdim = (WORD)tidata; break;
            case GTA_Scroller_ArrowKind: arrowkind = (WORD)tidata; break;
            case GTA_Scroller_ScrollerKind: stags[7].ti_Data = tidata; break;
            
        }
        
    } /* while (iterate taglist) */
    
    DEBUG_CREATESCROLLER(bug("makescroller: arrowdim %ld\n", arrowdim));

    gt_boundsinset(&bbox, stdgadtags, BORDERPROPSPACINGX, BORDERPROPSPACINGY);

    DEBUG_CREATESCROLLER(bug("makescroller: left %ld top %ld width %ld height %ld\n", bbox.Left, bbox.Top, bbox.Width, bbox.Height));

    stags[14].ti_Data = (IPTR)stdgadtags;
    
    /* Substract the arrow's total size from the sroller's size */

    if (arrowdim == -1)
    {
        DEBUG_CREATESCROLLER(bug("makescroller: default arrowdim\n"));
        arrowdim = (freedom == FREEVERT) ? bbox.Width : bbox.Height;
        DEBUG_CREATESCROLLER(bug("makescroller: arrowdim %ld\n", arrowdim));
    }
    
    stdgadtags[(freedom == FREEVERT) ? TAG_Height : TAG_Width].ti_Data -= 2 * arrowdim;

    scroller = gt_newgadget(GadToolsBase, GadToolsBase->scrollerclass, stags);

    if (!scroller)
        return (NULL);

    DEBUG_CREATESCROLLER(bug("makescroller: scroller gadget 0x%lx scroller Width %ld Height %ld\n",scroller,scroller->Width,scroller->Height));
    DEBUG_CREATESCROLLER(bug("makescroller: scroller nextgadget 0x%lx\n",scroller->NextGadget));
    
    if (arrowdim) /* Scroller has arrows ? */
    {
        struct TagItem atags[NUMTAGS(scrollerarrowtags)];
        struct TagItem tellscrollertags[] =
        {
            {GTA_Scroller_Arrow1, 0             },
            {GTA_Scroller_Arrow2, 0             },
            {TAG_DONE                           }
        };
        
        memcpy(atags, scrollerarrowtags, sizeof(scrollerarrowtags));
        atags[11].ti_Data = arrowkind;

        atags[5].ti_Data = (IPTR)vi->vi_dri;    /* Set GA_DrawInfo */
        atags[6].ti_Data = (IPTR)scroller;      /* Set GA_Previous */
        atags[7].ti_Data = (IPTR)scroller;      /* Set GTA_Arrow_Scroller */
        atags[12].ti_Data = (IPTR)GetTagData(GA_Previous, 0, stdgadtags);

        /* These must be the same as for scroller */
        atags[10].ti_Data = (IPTR)GetTagData(GA_ID, 0, stdgadtags);
        
        if (freedom == FREEVERT)
        {
            DEBUG_CREATESCROLLER(bug("makescroller: Freedom=FREEVERT\n"));
            atags[0].ti_Data = scroller->LeftEdge - BORDERPROPSPACINGX;
            atags[1].ti_Data = scroller->TopEdge + BORDERPROPSPACINGY + scroller->Height;
            atags[2].ti_Data = scroller->Width + BORDERPROPSPACINGX * 2;
            atags[3].ti_Data = arrowdim;
            atags[4].ti_Data = UPIMAGE;
        }
        else
        {
            DEBUG_CREATESCROLLER(bug("makescroller: Freedom=FREEHORIZ\n"));

            atags[0].ti_Data = scroller->LeftEdge + scroller->Width + BORDERPROPSPACINGX;
            atags[1].ti_Data = scroller->TopEdge - BORDERPROPSPACINGY;
            atags[2].ti_Data = arrowdim;
            atags[3].ti_Data = scroller->Height + BORDERPROPSPACINGY * 2;
            atags[4].ti_Data = LEFTIMAGE;
        }

        arrow_dec = gt_newgadget(GadToolsBase, GadToolsBase->arrowclass, atags);
        if (!arrow_dec)
            goto failure;

        DEBUG_CREATESCROLLER(bug("makescroller: arrow_dec gadget 0x%lx\n",arrow_dec));

        /* the second arrow follows the first one in the scroller's direction */
        if (freedom == FREEVERT)
        {
            atags[1].ti_Data += arrowdim;
            atags[4].ti_Data = DOWNIMAGE;
        }
        else
        {
            atags[0].ti_Data += arrowdim;
            atags[4].ti_Data = RIGHTIMAGE;
        }
        atags[6].ti_Data = (IPTR)arrow_dec;

        arrow_inc = gt_newgadget(GadToolsBase, GadToolsBase->arrowclass, atags);
        if (!arrow_inc)
            goto failure;

        DEBUG_CREATESCROLLER(bug("makescroller: arrow_inc gadget 0x%lx\n",arrow_inc));
        
        tellscrollertags[0].ti_Data = (IPTR)arrow_dec;
        tellscrollertags[1].ti_Data = (IPTR)arrow_inc;
        
        DEBUG_CREATESCROLLER(bug("makescroller: tell scroller about arrows\n"));
        SetAttrsA(scroller, tellscrollertags);

        ReturnPtr ("makescroller", struct Gadget *, arrow_inc);
        
    } /* if (scroller should have arrows attached) */
    
    ReturnPtr ("makescroller", struct Gadget *, scroller);
    
failure:
   if (scroller)
        DisposeObject((Object *)scroller);

   if (arrow_dec)
        DisposeObject((Object *)arrow_dec);

   if (arrow_inc)
        DisposeObject((Object *)arrow_inc);
   
   ReturnPtr("makescroller", struct Gadget *, NULL);
}

/****************************************************************************************/

/* STRING_KIND and INTEGER_KIND differ only in three tags, one default and the kind */

static const struct TagItem stringtags[] =
{
    {GA_Disabled            , FALSE                 }, /* 0 */
    {GA_Immediate           , FALSE                 }, /* 1 */
    {GA_RelVerify           , TRUE                  }, /* 2 */
    {GA_TabCycle            , TRUE                  }, /* 3 */
    {GTST_String            , (IPTR)NULL            }, /* 4 */
    {GTST_MaxChars          , 64UL                  }, /* 5 */
    {GTST_EditHook          , (IPTR)NULL            }, /* 6 */
    {STRINGA_ExitHelp       , FALSE                 }, /* 7 */
    {STRINGA_Justification  , GACT_STRINGLEFT       }, /* 8 */
    {STRINGA_ReplaceMode    , FALSE                 }, /* 9 */
    {GA_TextAttr            , (IPTR)NULL            }, /* 10 */
    {GTA_GadgetKind         , STRING_KIND           }, /* 11 */
    {GA_Bounds              , (IPTR)NULL            }, /* 12: &bbox */
    {TAG_MORE               , (IPTR)NULL            }  /* 13 */
};

struct Gadget *makestring(struct GadToolsBase_intern *GadToolsBase,
                          struct TagItem stdgadtags[],
                          struct VisualInfo *vi,
                          struct TextAttr *tattr,
                          struct TagItem *taglist,
                          ULONG kind)
{
    struct IBox bbox;
    struct TagItem tags[NUMTAGS(stringtags)];

    memcpy(tags, stringtags, sizeof(stringtags));
    tags[12].ti_Data = (IPTR)&bbox;

    if (kind == INTEGER_KIND)
    {
        tags[4].ti_Tag   = GTIN_Number;     /* default 0 */
        tags[5].ti_Tag   = GTIN_MaxChars;
        tags[5].ti_Data  = 10L;
        /* GTIN_EditHook is the same tag as GTST_EditHook */
        tags[11].ti_Data = INTEGER_KIND;
    }

    /* All tags (entries 0 to 9) are stored by gt_nexttag() */
    while (gt_nexttag(GadToolsBase, (struct TagItem **)&taglist, tags, 0x3ff))
        ;

    gt_settattr(&tags[10], tattr);

    gt_boundsinset(&bbox, stdgadtags, BORDERSTRINGSPACINGX, BORDERSTRINGSPACINGY);

    tags[13].ti_Data = (IPTR)stdgadtags;

    return gt_newgadget(GadToolsBase, GadToolsBase->stringclass, tags);
}

/****************************************************************************************/

const struct TagItem scroller2lv[] =
{
    {PGA_Top    ,       GTLV_Top},
    {TAG_DONE   ,               }
};

/****************************************************************************************/

/* Spacing between scroller and listview */
#define SCROLLER_SPACING 2

/****************************************************************************************/

static const struct TagItem listviewtags[] =
{
    {GA_Disabled            , FALSE                 }, /* 0  */
    {GTLV_Top               , 0L                    }, /* 1  */
    {GTLV_MakeVisible       , 0L                    }, /* 2  */
    {GTLV_Labels            , (IPTR)NULL            }, /* 3  */
    {GTLV_Selected          , ~0L                   }, /* 4  */
    {GTLV_ItemHeight        , 0L                    }, /* 5  */
    {GTLV_CallBack          , (IPTR)NULL            }, /* 6  */
    {GTLV_MaxPen            , 0L                    }, /* 7  */
    {GTLV_ReadOnly          , 0L                    }, /* 8  */
    {LAYOUTA_Spacing        , 0L                    }, /* 9  */
    {GA_TextAttr            , (IPTR)NULL            }, /* 10 */
    {GA_RelVerify           , TRUE                  }, /* 11 */
    {GA_Bounds              , (IPTR)NULL            }, /* 12: &bbox */
    {GTLV_ShowSelected      , (IPTR)LV_SHOWSELECTED_NONE }, /* 13 */
    {TAG_MORE               , (IPTR)NULL            }  /* 14 */
};

static const struct TagItem lvscrollertags[] =
{
    /* The listview will initialize the scrollers top, visible & total,
     ** in its GM_LAYOUT method
     */
    {GTSC_Arrows                        , 0                     },  /* 0: scroller_width */
    {PGA_Freedom                        , LORIENT_VERT          },
    {GTA_Scroller_ArrowKind             , LISTVIEW_KIND         },
    {GTA_Scroller_ScrollerKind          , LISTVIEW_KIND         },
    {TAG_DONE                                                   }
};

struct Gadget *makelistview(struct GadToolsBase_intern *GadToolsBase,
                            struct TagItem stdgadtags[],
                            struct VisualInfo *vi,
                            struct TextAttr *tattr,
                            struct TagItem *taglist,
                            ULONG kind)
{
    struct Gadget *lvgad = NULL, *showselgad;
    struct Gadget *scrollergad;
    struct IBox bbox;


    WORD scroller_width = 16; /* default */
    
    WORD lv_width, lv_height;
#if CORRECT_LISTVIEWHEIGHT
    WORD viewheight, totalitemheight = 0;
#endif
    WORD ysize;
    
    struct TagItem *tag, lvtags[NUMTAGS(listviewtags)];
    
    EnterFunc(bug("makelistview()\n"));
    
    memcpy(lvtags, listviewtags, sizeof(listviewtags));
    lvtags[12].ti_Data = (IPTR)&bbox;

    /* GA_Disabled ... LAYOUTA_Spacing and GTLV_ShowSelected are stored by gt_nexttag() */
    while ((tag = gt_nexttag(GadToolsBase, (struct TagItem **)&taglist, lvtags, 0x23ff)))
    {
        if (tag->ti_Tag == GTLV_ScrollWidth)
            scroller_width = (UWORD)tag->ti_Data;
        
    } /* while (iterate taglist) */

    showselgad = (struct Gadget *)lvtags[13].ti_Data;

    /* if there is a bounding box the label position
       will be calculated based on this box and not
       the gadget coords */
       
    gt_boundsinset(&bbox, stdgadtags, 0, 0);

    /* Callback supplied ? */
    if (!lvtags[6].ti_Data)
        lvtags[6].ti_Tag = TAG_IGNORE; /* Don't pass GTLV_Callback, NULL */

    if (tattr) /* Text Attr supplied ? */
    {
        lvtags[10].ti_Data = (IPTR)tattr;
        ysize = tattr->ta_YSize;
    }
    else
    {
        lvtags[10].ti_Tag = TAG_IGNORE; /* Don't pass GA_TextAttr, NULL */
        
        ysize = vi->vi_dri->dri_Font->tf_YSize;
    }
    
    /* If not set allready, set ItemHeight */
    if (lvtags[5].ti_Data == 0)
        lvtags[5].ti_Data = ysize;
        
    /* If not set allready, set Spacing */
    if (lvtags[9].ti_Data == 0)
        lvtags[9].ti_Data = LV_DEF_INTERNAL_SPACING;

    DEBUG_CREATELISTVIEW(bug("makelistview: item ysize %ld\n", ysize));
             
    lv_width  = (WORD)stdgadtags[TAG_Width].ti_Data;
    lv_height = (WORD)stdgadtags[TAG_Height].ti_Data;
    
        
    /* Adjust the listview width according to scroller width + some spacing */
    lv_width -= (scroller_width + SCROLLER_SPACING);
    
    /* Adjust the listview height according to showsel gadget height */
    if ((showselgad) && (showselgad != LV_SHOWSELECTED_NONE))
    {
        /* The showselected string gadget must have the same width as the
           listview gadget, otherwise fail (AmigaOS does the same). Fail
           also if the showselected string gadget is too high*/
           
        if ((EG(showselgad)->BoundsWidth != bbox.Width) ||
            (EG(showselgad)->BoundsHeight >= lv_height))
        {
            ReturnPtr ("makelistview", struct Gadget *, NULL);
        }
        
        /* the showselected string gadget shrinks the height of the listview */
        
        lv_height -= EG(showselgad)->BoundsHeight;
        
        /* the showselected string gadget will get its position automatically
           fixed to be under the listview gadget */
           
        EG(showselgad)->BoundsLeftEdge = bbox.Left;
        EG(showselgad)->BoundsTopEdge  = bbox.Top + lv_height;
        EG(showselgad)->LeftEdge       = EG(showselgad)->BoundsLeftEdge + BORDERSTRINGSPACINGX;
        EG(showselgad)->TopEdge        = EG(showselgad)->BoundsTopEdge  + BORDERSTRINGSPACINGY;
        
        D(bug("makelistview: Showselected gadget specified"));

    }
    
#if CORRECT_LISTVIEWHEIGHT
                        /* GTLV_ItemHeight + LAYOUTA_Spacing */
    totalitemheight = lvtags[5].ti_Data + lvtags[9].ti_Data;

 /* stegerg: I think it looks better without this adjustment. Think of
    GTLV_ShowSelected and aligning with other gadgets */
    
    /* Adjust listview height so that an exact number of items fits in it */
    viewheight = lv_height - (2 * LV_BORDER_Y);
    lv_height -= (viewheight % totalitemheight);
    
#endif
    
    /* Reinsert the modified dimension attrs into the listview taglist */
    
    D(bug("makelistview: Dimension passed to listview: w=%d, h=%d\n", lv_width, lv_height));

    stdgadtags[TAG_Width].ti_Data  = (IPTR)lv_width;
    stdgadtags[TAG_Height].ti_Data = (IPTR)lv_height;
        
    lvtags[14].ti_Data = (IPTR)stdgadtags;
    
    D(bug("makelistview: Listview class opened\n"));
    lvgad = gt_newgadget(GadToolsBase, GadToolsBase->listviewclass, lvtags);
    if (lvgad)
    {
        struct TagItem scr_stdtags[] =
        {
            {GA_Left    , 0L            },
            {GA_Top     , 0L            },
            {GA_Width   , 0L            },
            {GA_Height  , 0L            },
            {GA_Next    , (IPTR)NULL    },
            {GA_ID      , 0L            },
            {GA_DrawInfo, (IPTR)NULL    },
            {GA_Previous, (IPTR)NULL    },
            {TAG_DONE                   }
        };
        
        struct TagItem scr_specialtags[NUMTAGS(lvscrollertags)];

        memcpy(scr_specialtags, lvscrollertags, sizeof(lvscrollertags));
        scr_specialtags[0].ti_Data = scroller_width;

        D(bug("makelistview: Listview gadget created: %p\n", lvgad));
        D(bug("makelistview: scroller_width %ld\n",scroller_width));

        /* Create a scroller object to use with the listviev */
        scr_stdtags[0].ti_Data = lvgad->LeftEdge + lvgad->Width  - 1 + SCROLLER_SPACING;
        scr_stdtags[1].ti_Data = lvgad->TopEdge;
        scr_stdtags[2].ti_Data = scroller_width;
        scr_stdtags[3].ti_Data = lvgad->Height;
        scr_stdtags[4].ti_Data = (IPTR)lvgad;
        scr_stdtags[5].ti_Data = lvgad->GadgetID;
        scr_stdtags[6].ti_Data = (IPTR)vi->vi_dri;
        scr_stdtags[7].ti_Data = stdgadtags[TAG_Previous].ti_Data;
        
        scrollergad = makescroller(GadToolsBase, scr_stdtags, vi, NULL, scr_specialtags, SCROLLER_KIND);
        if (scrollergad)
        {
            struct TagItem lvset_tags[] =
            {
                {GTA_Listview_Scroller, (IPTR)NULL      },
                {TAG_DONE,                              }
            };
            struct TagItem scrnotify_tags[] =
            {
                {ICA_TARGET     , (IPTR)NULL    },
                {ICA_MAP        , (IPTR)NULL    },
                {TAG_DONE                       }
            };
            struct Gadget *prop_of_scrollergad;
            
            D(bug("makelistview: Scroller gadget created: %p\n", scrollergad));
                
            /* the scrollergadget is a multigadget gadget: arrowinc->arrowdec->prop */
            /* FIXME: This relies on scroller gadget to always contain arrow gadgets
             *        If this ever changes the line below must be updated.
             */
            prop_of_scrollergad = scrollergad->NextGadget->NextGadget;
                
            scrollergad->Activation &= ~GACT_FOLLOWMOUSE;
                            
            /* Tell the listview about the scroller and the showselgad */
            DEBUG_CREATELISTVIEW(bug("makelistview: tell listview about scroller\n"));
            lvset_tags[0].ti_Data = (IPTR)prop_of_scrollergad;
            SetAttrsA((Object *)lvgad, lvset_tags);
                
            /* Tell the scroller to notify the listview when its PGA_Top attribute changes */
            DEBUG_CREATELISTVIEW(bug("makelistview: tell scroller about notification\n"));
            scrnotify_tags[0].ti_Data = (IPTR)lvgad;
            scrnotify_tags[1].ti_Data = (IPTR)scroller2lv;
                
            SetAttrsA((Object *)prop_of_scrollergad, scrnotify_tags);
                
            ReturnPtr ("makelistview", struct Gadget *, scrollergad);
        }

        DisposeObject(lvgad);
            
    } /* if (lvgad created) */
        
    ReturnPtr ("makelistview", struct Gadget *, NULL);
}

/****************************************************************************************/

struct Gadget *makegeneric(struct GadToolsBase_intern *GadToolsBase,
                           struct TagItem stdgadtags[],
                           struct VisualInfo *vi,
                           struct TextAttr *tattr,
                           struct TagItem *taglist,
                           ULONG kind)
{
    struct GT_GenericGadget *gad;
    struct Gadget           *prevgad;
    
    gad = AllocMem(sizeof(struct GT_GenericGadget), MEMF_PUBLIC | MEMF_CLEAR);
    if (gad)
    {
        gad->gad.LeftEdge       = stdgadtags[TAG_Left].ti_Data;
        gad->gad.TopEdge        = stdgadtags[TAG_Top].ti_Data;
        gad->gad.Width          = stdgadtags[TAG_Width].ti_Data;
        gad->gad.Height         = stdgadtags[TAG_Height].ti_Data;
        gad->gad.Flags          = GFLG_EXTENDED;
        gad->gad.Activation     = 0;
        gad->gad.GadgetType     = GTYP_GADTOOLS;
        gad->gad.GadgetRender   = NULL;
        gad->gad.SelectRender   = NULL;
        gad->gad.GadgetText     = (struct IntuiText *)stdgadtags[TAG_IText].ti_Data;
        gad->gad.MutualExclude  = 0;
        gad->gad.SpecialInfo    = NULL;
        gad->gad.GadgetID       = stdgadtags[TAG_ID].ti_Data;
        gad->gad.UserData       = (APTR)stdgadtags[TAG_UserData].ti_Data;
        gad->gad.MoreFlags      = GMORE_BOUNDS | GMORE_GADGETHELP;
        gad->gad.BoundsLeftEdge = stdgadtags[TAG_Left].ti_Data;
        gad->gad.BoundsTopEdge  = stdgadtags[TAG_Top].ti_Data;
        gad->gad.BoundsWidth    = stdgadtags[TAG_Width].ti_Data;
        gad->gad.BoundsHeight   = stdgadtags[TAG_Height].ti_Data;
        gad->magic              = GENERIC_MAGIC;
        gad->magic2             = GENERIC_MAGIC2;
        gad->itext              = (struct IntuiText *)stdgadtags[TAG_IText].ti_Data;
        
        prevgad = (struct Gadget *)stdgadtags[TAG_Previous].ti_Data;
        prevgad->NextGadget = (struct Gadget *)gad;
    }
    
    ReturnPtr ("makegeneric", struct Gadget *, (struct Gadget *)gad);
}

/****************************************************************************************/
