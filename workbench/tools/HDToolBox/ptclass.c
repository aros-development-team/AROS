/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <intuition/cghooks.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>

#include <string.h>

#include "ptclass.h"
#include "partitions.h"
#include "partitiontables.h"
#include "platform.h"

#define DEBUG 0
#include "debug.h"

#define G(a) ((struct Gadget *)a)

#define DPTYPE_EMPTY 	      0
#define DPTYPE_EMPTY_SELECTED 1
#define DPTYPE_USED 	      2
#define DPTYPE_USED_SELECTED  3

#define PARTITIONBLOCK_FRAMEWIDTH 2

#define END_FIX 1

#define EMPTY1_RED  	    0x99999999
#define EMPTY1_GREEN	    0x99999999
#define EMPTY1_BLUE 	    0x99999999

#define EMPTY2_RED  	    0x65656565
#define EMPTY2_GREEN	    0x65656565
#define EMPTY2_BLUE 	    0x65656565

#define EMPTY1_DARK_RED     0x66666666
#define EMPTY1_DARK_GREEN   0x66666666
#define EMPTY1_DARK_BLUE    0x66666666

#define EMPTY2_DARK_RED     0x42424242
#define EMPTY2_DARK_GREEN   0x42424242
#define EMPTY2_DARK_BLUE    0x42424242

#define EMPTY1_SEL_RED	    0x40404040
#define EMPTY1_SEL_GREEN    0x85858585
#define EMPTY1_SEL_BLUE     0x99999999

#define EMPTY2_SEL_RED	    0x29292929
#define EMPTY2_SEL_GREEN    0x56565656
#define EMPTY2_SEL_BLUE     0x63636363

#define USED_RED    	    0x02020202
#define USED_GREEN  	    0x75757575
#define USED_BLUE   	    0x02020202

#define USED_SEL_RED	    0x66666666
#define USED_SEL_GREEN	    0xc9c9c9c9
#define USED_SEL_BLUE	    0x66666666

#define PEN_EMPTY1  	0
#define PEN_EMPTY2  	1
#define PEN_EMPTY1_SEL  2
#define PEN_EMPTY2_SEL  3
#define PEN_EMPTY1_DARK 4
#define PEN_EMPTY2_DARK 5
#define PEN_USED    	6
#define PEN_USED_SEL   	7
    
#define NUM_PENS    	8

STATIC CONST ULONG rgbtable[] =
{
    EMPTY1_RED	    , EMPTY1_GREEN  	, EMPTY1_BLUE	    ,
    EMPTY2_RED	    , EMPTY2_GREEN  	, EMPTY2_BLUE	    ,
    EMPTY1_SEL_RED  , EMPTY1_SEL_GREEN	, EMPTY1_SEL_BLUE   ,
    EMPTY2_SEL_RED  , EMPTY2_SEL_GREEN	, EMPTY2_SEL_BLUE   ,
    EMPTY1_DARK_RED , EMPTY1_DARK_GREEN , EMPTY1_DARK_BLUE  ,
    EMPTY2_DARK_RED , EMPTY2_DARK_GREEN , EMPTY2_DARK_BLUE  ,
    USED_RED	    , USED_GREEN    	, USED_BLUE 	    ,
    USED_SEL_RED    , USED_SEL_GREEN	, USED_SEL_BLUE
};

struct PTableData {
    struct DrawInfo *dri;
    struct Image *frame;
    struct HDTBPartition *table;
    struct HDTBPartition *active; /* active partition, or the DE for free space */
    struct DosEnvec gap;
    struct ColorMap *cm;
    ULONG block;
    ULONG flags;
    WORD  pens[NUM_PENS];
    BOOL move;
    BOOL selected;
    BOOL multicolor;
    BOOL firstdraw;
    BOOL pensallocated;
};

STATIC CONST UWORD pattern[] =
{
    0xAAAA, 0xAAAA,
    0x5555, 0x5555
};

STATIC CONST UWORD pattern2[] =
{
        0xFF00,
        0xFF00,
        0xFF00,
        0xFF00,
        0xFF00,
        0xFF00,
        0xFF00,
        0xFF00,
        0x00FF,
        0x00FF,
        0x00FF,
        0x00FF,
        0x00FF,
        0x00FF,
        0x00FF,
        0x00FF		
};

#define SetPattern(r,p,s) {r->AreaPtrn = (UWORD *)p; r->AreaPtSz = s;}

Class *ptcl = NULL;

static void PrepareRP(struct RastPort *rp, struct PTableData *data, WORD dptype)
{
    D(bug("[HDToolBox] PrepareRP()\n"));

    if (data->multicolor)
    {
        switch(dptype)
        {
            case DPTYPE_EMPTY:
                SetPattern(rp, pattern2, 4);
                SetABPenDrMd(rp, data->pens[PEN_EMPTY1], data->pens[PEN_EMPTY2], JAM2);
                break;

            case DPTYPE_EMPTY_SELECTED:
                SetPattern(rp, pattern2, 4);
                SetABPenDrMd(rp, data->pens[PEN_EMPTY1_SEL], data->pens[PEN_EMPTY2_SEL], JAM2);
                break;

            case DPTYPE_USED:
                SetPattern(rp, NULL, 0);
                SetABPenDrMd(rp, data->pens[PEN_USED], 0, JAM2);
                break;

            case DPTYPE_USED_SELECTED:
                SetPattern(rp, NULL, 0);
                SetABPenDrMd(rp, data->pens[PEN_USED_SEL], 0, JAM2);
                break;
        }
    } /* if (data->multicolor) */
    else
    {
        switch(dptype)
        {
            case DPTYPE_EMPTY:
                SetPattern(rp, NULL, 0);
                SetABPenDrMd(rp, 0, 0, JAM2);
                break;

            case DPTYPE_EMPTY_SELECTED:
                SetPattern(rp, pattern, 2);
                SetABPenDrMd(rp, 1, 0, JAM2);
                break;

            case DPTYPE_USED:
                SetPattern(rp, pattern, 2);
                SetABPenDrMd(rp, 2, 0, JAM2);
                break;

            case DPTYPE_USED_SELECTED:
                SetPattern(rp, pattern, 2);
                SetABPenDrMd(rp, 3, 0, JAM2);
                break;
        }
    } /* if (data->multicolor) */
}

STATIC IPTR pt_new(Class *cl, Object *obj, struct opSet *msg) 
{
    struct PTableData *data;
    struct DrawInfo *dri;
    struct Image *frame;
    struct HDTBPartition *table;
    ULONG flags;

    struct TagItem tags[] =
    {
        {IA_Width,      0UL             },
        {IA_Height,     0UL             },
        {IA_Resolution, 0UL             },
        {IA_FrameType,  FRAME_BUTTON    },
        {TAG_DONE                       }
    };

    D(bug("[HDToolBox] pt_new()\n"));

    dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, (IPTR)NULL, msg->ops_AttrList);
    if (!dri)
        return (IPTR)NULL;

    tags[0].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList);
    tags[1].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList);
    table = (struct HDTBPartition *)GetTagData(PTCT_PartitionTable, 0, msg->ops_AttrList);
    flags = GetTagData(PTCT_Flags, 0, msg->ops_AttrList);
    tags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;

    frame = (struct Image *) NewObjectA(NULL, FRAMEICLASS, tags);
    if (!frame)
        return (IPTR)NULL;

    tags[0].ti_Tag = GA_Image;
    tags[0].ti_Data = (IPTR) frame;
    tags[1].ti_Tag = TAG_MORE;
    tags[1].ti_Data = (IPTR) msg->ops_AttrList;
    obj = (Object *) DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
    {
        DisposeObject(frame);
        return (IPTR)NULL;
    }

    data = INST_DATA(cl, obj);
    
    data->dri = dri;
    data->frame = frame;
    data->table = table;
    data->flags = flags;
    data->active = NULL;
    data->move = FALSE;
    data->selected = FALSE;
    data->firstdraw = TRUE;
    
    return (IPTR)obj;
}

STATIC IPTR pt_set(Class *cl, Object *obj, struct opSet *msg) 
{
    struct PTableData *data = INST_DATA(cl, obj);
    IPTR retval = 0UL;
    struct TagItem *tag;
    struct TagItem *taglist;
    struct RastPort *rport;

    D(bug("[HDToolBox] pt_set()\n"));

    if (msg->MethodID != OM_NEW)
        retval = DoSuperMethodA(cl, obj, (Msg)msg);

    taglist = (struct TagItem *)msg->ops_AttrList;
    while ((tag = NextTagItem(&taglist)))
    {
        switch (tag->ti_Tag)
        {
        case GA_Disabled:
            retval = TRUE;
            break;
        case GA_DrawInfo:
            if (msg->MethodID == OM_NEW)
                data->dri = (struct DrawInfo *)tag->ti_Data;
            break;
        case PTCT_PartitionTable:
            data->table = (struct HDTBPartition *)tag->ti_Data;
            retval = TRUE;
            break;
        case PTCT_ActivePartition:
            data->active = (struct HDTBPartition *)tag->ti_Data;
            retval = TRUE;
            break;
        }
    }

    /* redraw if one attribute has changed something */
    if ((retval) && (OCLASS(obj) == cl))
    {
        rport = ObtainGIRPort(msg->ops_GInfo);
        if (rport)
        {
            DoMethod(obj, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rport, GREDRAW_UPDATE);
            ReleaseGIRPort(rport);
            retval = FALSE;
        }
    }
    return retval;
}

STATIC IPTR pt_get(Class *cl, Object *obj, struct opGet *msg)
{
    struct PTableData *data = INST_DATA(cl, obj);
    IPTR retval = 0;

    D(bug("[HDToolBox] pt_get()\n"));

    switch (msg->opg_AttrID)
    {
    case PTCT_PartitionTable:
        *msg->opg_Storage = (IPTR)data->table;
        retval = 1;
        break;
    case PTCT_ActivePartition:
        *msg->opg_Storage = (IPTR)data->active;
        retval = 1;
        break;
    case PTCT_ActiveType:
        if ((struct DosEnvec *)data->active == &data->gap)
            *msg->opg_Storage = (IPTR)PTS_EMPTY_AREA;
        else if (data->active == NULL)
            *msg->opg_Storage = (IPTR)PTS_NOTHING;
        else
            *msg->opg_Storage = (IPTR)PTS_PARTITION;
        retval = 1;
        break;
        
    case PTCT_Flags:
        break;

    default:
        retval = DoSuperMethodA(cl, obj, (Msg)msg);
    }
    return retval;
}

struct DosEnvec *findSpace(struct HDTBPartition *table, struct DosEnvec *de, ULONG block)
{
    struct HDTBPartition *pn;
    ULONG spc;
    ULONG first = 0;
    ULONG last = 0xFFFFFFFF;

    D(bug("[HDToolBox] findSpace()\n"));

    /* inherit */
    CopyMem(&table->de, de, sizeof(struct DosEnvec));
    de->de_SizeBlock = table->dg.dg_SectorSize>>2;
    de->de_Surfaces = table->dg.dg_Heads;
    de->de_BlocksPerTrack = table->dg.dg_TrackSectors;
    de->de_BufMemType = table->dg.dg_BufMemType;

    pn = (struct HDTBPartition *)table->listnode.list.lh_Head;
    while (pn->listnode.ln.ln_Succ != NULL)
    {
        ULONG start;
        ULONG end;

        if (pn->listnode.ln.ln_Type != LNT_Parent)
        {
            spc = pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
            start = pn->de.de_LowCyl*spc;
            end = ((pn->de.de_HighCyl+1)*spc)-1;
            if (block<start)
            {
                if (start<last)
                    last=start-1;
            }
            else if (block>end)
            {
                if (end>first)
                    first = end+1;
            }
        }
        pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
    }

    spc = table->dg.dg_Heads*table->dg.dg_TrackSectors;
    if (first == 0)
        first = table->table->reserved;
    if (first != 0)
        first = (first - 1) / spc + 1;

    last = ((last+1)/spc)-1;
    if (last>=table->dg.dg_Cylinders)
        last=table->dg.dg_Cylinders-1;

    de->de_LowCyl = first;
    de->de_HighCyl = last;

    return de;
}

/*
 * Find the partition containing the currently selected block, or the DE
 * describing the empty space.
 */
struct HDTBPartition *getActive(struct PTableData *data) 
{
    struct HDTBPartition *pn;

    D(bug("[HDToolBox] getActive()\n"));

    pn = (struct HDTBPartition *)data->table->listnode.list.lh_Head;
    while (pn->listnode.ln.ln_Succ != NULL)
    {
        ULONG start;
        ULONG end;

        if (pn->listnode.ln.ln_Type != LNT_Parent)
        {
            start = pn->de.de_LowCyl*pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
            end = (pn->de.de_HighCyl+1)*pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
            if ((data->block>=start) && (data->block<end))
                return pn;
        }
        pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
    }
    return (struct HDTBPartition *)findSpace(data->table,&data->gap,data->block);
}

void DrawBox(struct RastPort *rport, struct DrawInfo *dri,
            UWORD sx, UWORD sy, UWORD ex, UWORD ey, BOOL recessed)
{
    D(bug("[HDToolBox] DrawBox(rport @ %p, dri @ %p, %d,%dx%d,%d)\n", rport, dri, sx, sy, ex, ey));

    SetAPen(rport, dri->dri_Pens[recessed ? SHINEPEN : SHADOWPEN]);
    Move(rport, ex, sy);
    Draw(rport, ex, ey);
    Draw(rport, sx, ey);
    SetAPen(rport, dri->dri_Pens[recessed ? SHADOWPEN : SHINEPEN]);
    Draw(rport, sx, sy);
    Draw(rport, ex, sy);
}

void DrawFilledBox(struct RastPort *rport, struct DrawInfo *dri,
            UWORD sx, UWORD sy, UWORD ex, UWORD ey)
{
    D(bug("[HDToolBox] DrawFilledBox()\n"));

    RectFill(rport, sx+1, sy+1, ex-1, ey-1);
    DrawBox(rport, dri, sx, sy, ex, ey, FALSE);
}

void DrawPartition(struct RastPort *rport, struct PTableData *data,
            struct Gadget *gadget, struct HDTBPartition *table, struct DosEnvec *pn, WORD drawtype)
{
    ULONG start;
    ULONG end;
    ULONG blocks;
    ULONG spc;
    ULONG cyls;
    ULONG skipcyl;

    D(bug("[HDToolBox] DrawPartition()\n"));

    if (!rport) return;
    
#if 0
    blocks =
        (table->table->de_HighCyl+1)*
        table->table->de_Surfaces*
        table->table->de_BlocksPerTrack;
#else
    blocks = table->dg.dg_Heads*table->dg.dg_TrackSectors;
    skipcyl = table->table->reserved/blocks;
    cyls = (table->dg.dg_Cylinders-1)-skipcyl;

#endif
    spc = pn->de_Surfaces*pn->de_BlocksPerTrack;
#if 0
    start = pn->de_LowCyl*spc*gadget->Width/blocks;
    end = (((pn->de_HighCyl+1)*spc)-1)*gadget->Width/blocks;
#else
    start = pn->de_LowCyl;
    end = pn->de_HighCyl;
    if (
            (pn->de_Surfaces!=table->dg.dg_Heads) ||
            (pn->de_BlocksPerTrack!=table->dg.dg_TrackSectors)
        )
    {
        start = start*spc/blocks;
        end   = end*spc/blocks;
    }
    if (start!=0)
        start -= skipcyl;
    if (end !=0)
        end   -= skipcyl;
    start = (start*(gadget->Width - PARTITIONBLOCK_FRAMEWIDTH)/cyls)+1;
#if END_FIX
    end  = ((end + 1) *(gadget->Width - PARTITIONBLOCK_FRAMEWIDTH)/cyls) + 1 - 1;
#else
    end  = (end*(gadget->Width - PARTITIONBLOCK_FRAMEWIDTH)/cyls);
#endif
    start += gadget->LeftEdge;
    end   += gadget->LeftEdge;
#endif

    PrepareRP(rport, data, drawtype);
        
    if (drawtype == DPTYPE_EMPTY)
    {
        RectFill(rport, start, gadget->TopEdge+1+(gadget->Height/2),
                    end, gadget->TopEdge+(gadget->Height)-2);
    }
    else
    {
        DrawFilledBox
        (
            rport,
            data->dri,
            start, gadget->TopEdge+1+(gadget->Height/2),
            end, gadget->TopEdge+(gadget->Height)-2
        );
    }
    
    SetPattern(rport, NULL, 0);
}

ULONG getBlock(UWORD mousex, UWORD width, struct HDTBPartition *table)
{
    ULONG block;

    if ((WORD)mousex < 0) mousex = 0;

    block = mousex*(table->dg.dg_Cylinders-1)/width;
    block *= table->dg.dg_Heads*table->dg.dg_TrackSectors;
    block += table->table->reserved;
    return block;
}

STATIC VOID notify_all(Class *cl, Object *obj, struct GadgetInfo *gi, BOOL interim, BOOL userinput) 
{
    struct PTableData *data = INST_DATA(cl, obj);
    struct opUpdate opu;

    struct TagItem tags[] =
    {
        {GA_ID,               G(obj)->GadgetID     },
#ifndef __AMIGAOS__
        {GA_UserInput,        userinput            },
#endif
        {PTCT_ActivePartition,(IPTR)data->active   },
        {PTCT_PartitionTable, (IPTR)data->table    },
        {PTCT_PartitionMove,  (IPTR)data->move     },
        {PTCT_Selected,       (IPTR)data->selected },
        {TAG_DONE                                  }
    };

    D(bug("[HDToolBox] notify_all()\n"));

    opu.MethodID = OM_NOTIFY;
    opu.opu_AttrList = tags;
    opu.opu_GInfo = gi;
    opu.opu_Flags = interim ? OPUF_INTERIM : 0;
    DoMethodA(obj, (Msg)&opu);
}

STATIC IPTR pt_goactive(Class *cl, Object *obj, struct gpInput *msg)
{
    struct PTableData *data = INST_DATA(cl, obj);
    IPTR retval = GMR_MEACTIVE;
    struct DosEnvec *de;
    struct RastPort *rport;
    WORD	    	 drawtype;

    D(bug("[HDToolBox] pt_goactive()\n"));

    data->block = getBlock(msg->gpi_Mouse.X, G(obj)->Width - PARTITIONBLOCK_FRAMEWIDTH,data->table);
    rport = ObtainGIRPort(msg->gpi_GInfo);
    if (data->active != NULL)
    {
        /* draw previous active as unselected */
        if ((struct DosEnvec *)data->active == &data->gap)
        {
            drawtype = DPTYPE_EMPTY;
            de = &data->gap;
        }
        else
        {
            drawtype = DPTYPE_USED;
            de = &data->active->de;
        }
        DrawPartition(rport, data, (struct Gadget *)obj, data->table, de, drawtype);
    }
    data->active = getActive(data);

    if ((struct DosEnvec *)data->active == &data->gap)
    {
        drawtype = DPTYPE_EMPTY_SELECTED;
        de = &data->gap;
    }
    else
    {
        drawtype = DPTYPE_USED_SELECTED;
        de = &data->active->de;
        if (data->flags & PTCTF_EmptySelectOnly)
            data->active = NULL;
    }
    data->selected = TRUE;
    notify_all(cl, obj, msg->gpi_GInfo, TRUE, TRUE);
    data->selected = FALSE;
    if (data->active && rport)
    {
        DrawPartition(rport, data, (struct Gadget *)obj, data->table, de, drawtype);
    }
    
    if (rport) ReleaseGIRPort(rport);
    
    return retval;
}

STATIC IPTR pt_goinactive(Class *cl, Object *obj, struct gpInput *msg)
{
    struct PTableData *data = INST_DATA(cl, obj);
    IPTR retval = TRUE;
    struct DosEnvec *de;
    struct RastPort *rport;
    WORD drawtype;

    D(bug("[HDToolBox] pt_goinactive()\n"));

    if (data->active)
    {
//		data->block = getBlock(msg->gpi_Mouse.X, G(obj)->Width - PARTITIONBLOCK_FRAMEWIDTH, data->table);
        if (getActive(data) == data->active)
        {
            if ((rport = ObtainGIRPort(msg->gpi_GInfo)))
            {
                if ((struct DosEnvec *)data->active == &data->gap)
                {
                    drawtype = DPTYPE_EMPTY_SELECTED;
                    de = &data->gap;
                }
                else
                {
                        drawtype = DPTYPE_USED_SELECTED;
                    de = &data->active->de;
                }
                DrawPartition(rport, data, (struct Gadget *)obj, data->table, de, drawtype);
                
                ReleaseGIRPort(rport);
            }
        }
    }
    return retval;
}

ULONG overflow_add(ULONG a, LONG b)
{
    ULONG result;

    result = a + b;

    if (a > result)
        result = (ULONG)-1;

    return result;
}

ULONG underflow_add(ULONG a, LONG b) 
{
    ULONG result;

    if (a<-b)
        result = 0;
    else
        result = a+b;

    return result;
}

BOOL overlap(ULONG a, ULONG b, ULONG c, ULONG d) 
{
    if (a>b)
    {
        ULONG e;
        e = b;
        b = a;
        a = e;
    }
    return
        (
            (((a >= c) && (a < d)) || ((b >= c) && (b < d))) ||
            (((c >= a) && (c < b)) || ((d >= a) && (d < b)))
        );
}

LONG getBetterDiff(struct HDTBPartition *table, struct HDTBPartition *current, LONG diff)
{
    struct HDTBPartition *pn;
    ULONG spc;
    ULONG oldblock;
    ULONG block;
    ULONG other;
    ULONG start;
    ULONG end;
    ULONG size;

    spc = current->de.de_Surfaces*current->de.de_BlocksPerTrack;
    start = current->de.de_LowCyl*spc;
    end = ((current->de.de_HighCyl+1)*spc)-1;
    size = end-start;
    if (diff > 0)
    {
        oldblock = end;
        block = overflow_add(end, diff);
        other = block-size;
    }
    else
    {
        oldblock = start;
        block = underflow_add(start, diff);
        other = block+size; /* use correct partition size */
    }
    if (block < table->table->reserved)
    {
        diff = table->table->reserved-oldblock;
        if (diff == 0)
            return 0;
        return getBetterDiff(table, current, diff);
    }
    spc = table->dg.dg_Heads*table->dg.dg_TrackSectors;
    if (block >= ((table->dg.dg_Cylinders)*spc))
    {
        diff = (((table->dg.dg_Cylinders)*spc)-1)-oldblock;
        if (diff == 0)
            return 0;
        return getBetterDiff(table, current, diff);
    }

    for (pn = (struct HDTBPartition *)table->listnode.list.lh_Head;
	 pn->listnode.ln.ln_Succ;
	 pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ)
    {
	/* do NOT include anything that is NOT a partition */
	if (pn->listnode.ln.ln_Type != LNT_Partition)
	    continue;
        /* don't check currently processed partition */
        if (current != pn)
        {
            spc = pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
            if (
                    overlap
                    (
                        block, other,
                        pn->de.de_LowCyl*spc, ((pn->de.de_HighCyl+1)*spc)-1
                    )
                )
            {
                if (diff>0)
                    diff = ((pn->de.de_LowCyl*spc)-1)-oldblock;
                else
                    diff = ((pn->de.de_HighCyl+1)*spc)-oldblock;
                return getBetterDiff(table, current, diff);
            }
        }
    }
    return diff;
}

STATIC IPTR pt_handleinput(Class *cl, Object *obj, struct gpInput *msg) 
{
    struct PTableData *data = INST_DATA(cl, obj);
    IPTR retval = GMR_MEACTIVE;
    struct InputEvent *ie;
    struct RastPort *rport;


    ie = msg->gpi_IEvent;
    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
	D(bug("[HDToolBox] pt_handleinput(): %x / %x / %x\n", IECODE_NOBUTTON, IECODE_LBUTTON | IECODE_UP_PREFIX, ie->ie_Code));
        switch (ie->ie_Code)
        {
        case IECODE_NOBUTTON:
            if (
                    ((struct DosEnvec *)data->active != &data->gap) &&
                    (!(data->flags & PTCTF_NoPartitionMove))
                )
            {
                ULONG block;
                LONG diff;
                ULONG start;
                ULONG end;
                ULONG spc;
                ULONG tocheck;

		D(bug("[HDToolBox] - attempting to obtain block\n", diff));
                block = getBlock(msg->gpi_Mouse.X, G(obj)->Width - PARTITIONBLOCK_FRAMEWIDTH, data->table);
		D(bug("[HDToolBox] - block location: %ld\n", diff));
                diff = block-data->block;
                if (diff)
                {
		    D(bug("[HDToolBox] - odiff: %ld\n", diff));
                    diff = getBetterDiff(data->table, data->active, diff);
		    D(bug("[HDToolBox] - diff : %ld\n", diff));
                    if (diff)
                    {
                        spc=data->active->de.de_Surfaces*data->active->de.de_BlocksPerTrack;
			D(bug("[HDToolBox] - spc  : %ld\n", spc));
                        start = data->active->de.de_LowCyl*spc;
			D(bug("[HDToolBox] - start: %ld\n", start));
                        start += diff;
                        end = ((data->active->de.de_HighCyl+1)*spc)-1;
			D(bug("[HDToolBox] - end  : %ld\n", end));
                        end += diff;
                        tocheck = (diff>0) ? end : start;
                        if (validValue(data->table, data->active, tocheck))
                        {
                            start = start/spc;
                            end = ((end+1)/spc)-1;
                            
                            rport = ObtainGIRPort(msg->gpi_GInfo);
                            
                            DrawPartition
                            (
                                rport,
                                data,
                                (struct Gadget *)obj,
                                data->table,
                                &data->active->de,
                                DPTYPE_EMPTY
                            );
                            data->active->de.de_LowCyl=start;
                            data->active->de.de_HighCyl=end;
                            DrawPartition
                            (
                                rport,
                                data,
                                (struct Gadget *)obj,
                                data->table,
                                &data->active->de,
                                DPTYPE_USED_SELECTED
                            );
                            if (rport) ReleaseGIRPort(rport);

                            data->block = block;
                            data->move = TRUE;
                            notify_all(cl, obj, msg->gpi_GInfo, TRUE, TRUE);
                        }
                        else
                        {
                            D(bug("[HDToolBox] pt_handleinput: !!!!!!!!!!!!!!!!!!!not valid\n"));
                        }
                    }
                }
            }
	    break;
        case (IECODE_LBUTTON | IECODE_UP_PREFIX):
            data->move = FALSE;
            notify_all(cl, obj, msg->gpi_GInfo, FALSE, TRUE);
            data->selected = FALSE;
            retval = GMR_NOREUSE|GMR_VERIFY;
            break;
        }
    }
    else
    {
	D(bug("[HDToolBox] pt_handleinput(): Other class: %x\n", ie->ie_Class));
    }
    D(bug("[HDToolBox] pt_handleinput(): successful\n", ie->ie_Class));
    return retval;
}

void DrawLegend(struct RastPort *rport, struct DrawInfo *dri, LONG sx, LONG sy, LONG ex, LONG ey, char *text) 
{
    D(bug("[HDToolBox] DrawLegend()\n"));

    RectFill(rport, sx+1, sy+1, ex-1, ey-1);
    DrawBox(rport, dri, sx, sy, ex, ey, FALSE);

    SetAPen(rport, 1);
    SetDrMd(rport, JAM1);

    Move(rport, sx, ey+rport->TxHeight+1);
    Text(rport, text, strlen(text));
}

STATIC IPTR pt_render(Class *cl, Object *obj, struct gpRender *msg) 
{
    struct PTableData *data = INST_DATA(cl, obj);
    struct HDTBPartition *pn;
    IPTR retval = 0;

    D(bug("[HDToolBox] pt_render()\n"));

    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (data->firstdraw)
    {
        struct TagItem obp_tags[] =
        {
            {OBP_Precision, PRECISION_EXACT },
            {OBP_FailIfBad, TRUE	    	},
            {TAG_DONE   	    	    	}
        };

        WORD i;

        data->firstdraw = FALSE;

        data->cm = msg->gpr_GInfo->gi_Screen->ViewPort.ColorMap;

        for(i = 0; i < NUM_PENS; i++)
        {
            data->pens[i] = ObtainBestPenA(data->cm,
                                       rgbtable[i * 3],
                           rgbtable[i * 3 + 1],
                           rgbtable[i * 3 + 2],
                           obp_tags);

            if (data->pens[i] == -1)
            {
                while(--i >= 0)
                {
                    ReleasePen(data->cm, data->pens[i]);
                }
                break;
            }
        }

        if (i == NUM_PENS)
        {
            data->multicolor = TRUE;
            data->pensallocated = TRUE;
        }
    }

    EraseRect
    (
        msg->gpr_RPort,
        G(obj)->LeftEdge,
        G(obj)->TopEdge,
        G(obj)->LeftEdge+G(obj)->Width - 1,
        G(obj)->TopEdge+(G(obj)->Height - 1)
    );

    PrepareRP(msg->gpr_RPort, data, DPTYPE_EMPTY);
    DrawLegend
    (
        msg->gpr_RPort,
        data->dri,
        G(obj)->LeftEdge,
        G(obj)->TopEdge,
        G(obj)->LeftEdge+(G(obj)->Width/5),
        G(obj)->TopEdge+(G(obj)->Height/2)-(G(obj)->Height/4),
        "Unselected Empty"
    );

    PrepareRP(msg->gpr_RPort, data, DPTYPE_EMPTY_SELECTED);
    DrawLegend
    (
        msg->gpr_RPort,
        data->dri,
        G(obj)->LeftEdge+(G(obj)->Width/5)+(G(obj)->Width/15),
        G(obj)->TopEdge,
        G(obj)->LeftEdge+(G(obj)->Width/5*2)+(G(obj)->Width/15),
        G(obj)->TopEdge+(G(obj)->Height/2)-(G(obj)->Height/4),
        "Selected Empty"
    );

    PrepareRP(msg->gpr_RPort, data, DPTYPE_USED);
    DrawLegend
    (
        msg->gpr_RPort,
        data->dri,
        G(obj)->LeftEdge+(G(obj)->Width/5*2)+(G(obj)->Width/15*2),
        G(obj)->TopEdge,
        G(obj)->LeftEdge+(G(obj)->Width/5*3)+(G(obj)->Width/15*2),
        G(obj)->TopEdge+(G(obj)->Height/2)-(G(obj)->Height/4),
        "Unselected Used"
    );

    PrepareRP(msg->gpr_RPort, data, DPTYPE_USED_SELECTED);
    DrawLegend
    (
        msg->gpr_RPort,
        data->dri,
        G(obj)->LeftEdge+(G(obj)->Width/5*3)+(G(obj)->Width/15*3),
        G(obj)->TopEdge,
        G(obj)->LeftEdge+(G(obj)->Width/5*4)+(G(obj)->Width/15*3),
        G(obj)->TopEdge+(G(obj)->Height/2)-(G(obj)->Height/4),
        "Selected Used"
    );

    PrepareRP(msg->gpr_RPort, data, DPTYPE_EMPTY);

    RectFill
    (
        msg->gpr_RPort,
        G(obj)->LeftEdge,
        G(obj)->TopEdge+(G(obj)->Height/2),
        G(obj)->LeftEdge+G(obj)->Width - 1,
        G(obj)->TopEdge+(G(obj)->Height - 1)
    );
    SetPattern(msg->gpr_RPort, NULL, 0);

    DrawBox
    (
        msg->gpr_RPort,
        data->dri,
        G(obj)->LeftEdge,
        G(obj)->TopEdge+(G(obj)->Height/2),
        G(obj)->LeftEdge+G(obj)->Width - 1,
        G(obj)->TopEdge+(G(obj)->Height - 1),
        TRUE
    );

    if (data->table)
    {
        SetPattern(msg->gpr_RPort, &pattern, 2);
        pn = (struct HDTBPartition *)data->table->listnode.list.lh_Head;
        while (pn->listnode.ln.ln_Succ)
        {
            if (pn->listnode.ln.ln_Type == LNT_Partition)
            {
                DrawPartition
                (
                    msg->gpr_RPort,
                    data,
                    (struct Gadget *)obj,
                    data->table,
                    &pn->de,
                    (data->active == pn) ? DPTYPE_USED_SELECTED : DPTYPE_USED
                );
            }
            pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
        }
        if ((struct DosEnvec *)data->active == &data->gap)
        {
            DrawPartition
            (
                msg->gpr_RPort,
                data,
                (struct Gadget *)obj,
                data->table,
                &data->gap,
                DPTYPE_EMPTY_SELECTED
            );
        }
        SetPattern(msg->gpr_RPort, 0, 0);
    }
    return retval;
}

STATIC IPTR pt_dispose(Class *cl, Object *obj, Msg msg) 
{
    struct PTableData *data = INST_DATA(cl, obj);

    D(bug("[HDToolBox] pt_dispose()\n"));

    if (data->pensallocated)
    {
        WORD i = 0;
        
        for(i = 0; i < NUM_PENS; i++)
        {
            ReleasePen(data->cm, data->pens[i]);
        }
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

STATIC IPTR pt_hittest(Class *cl, Object *obj, struct gpHitTest *msg) 
{
    D(bug("[HDToolBox] pt_hittest()\n"));

    return GMR_GADGETHIT;
}

AROS_UFH3S(IPTR, dispatch_ptclass,
      AROS_UFHA(Class *, cl, A0),
      AROS_UFHA(Object *, obj, A2),
      AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    IPTR retval;

    D(bug("[HDToolBox] dispatch_ptclass()\n"));

    switch (msg->MethodID)
    {
    case OM_NEW:
        retval = pt_new(cl, obj, (struct opSet *)msg);
        break;
    case OM_DISPOSE:
        retval = pt_dispose(cl, obj, msg);
        break;
    case OM_SET:
    case OM_UPDATE:
        retval = pt_set(cl, obj, (struct opSet *)msg);
        break;
    case OM_GET:
        retval = pt_get(cl, obj, (struct opGet *)msg);
        break;
    case GM_HITTEST:
        retval = pt_hittest(cl, obj, (struct gpHitTest *)msg);
        break;
    case GM_RENDER:
        retval = pt_render(cl, obj, (struct gpRender *)msg);
        break;
    case GM_GOACTIVE:
        retval = pt_goactive(cl, obj, (struct gpInput *)msg);
        break;
    case GM_GOINACTIVE:
        retval = pt_goinactive(cl, obj, (struct gpInput *)msg);
        break;
    case GM_HANDLEINPUT:
        retval = pt_handleinput(cl, obj, (struct gpInput *)msg);
        break;
    default:
        D(bug("[HDToolBox] dispatch_ptclass:default %ld\n", msg->MethodID));
        retval = DoSuperMethodA(cl, obj, msg);
        break;
    }
    return retval;

    AROS_USERFUNC_EXIT
}

Class *makePTClass(void)
{
    D(bug("[HDToolBox] makePTClass()\n"));

    if (!ptcl)
    {
        ptcl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct PTableData), 0);
        if (ptcl)
        {
            ptcl->cl_Dispatcher.h_Entry = AROS_ASMSYMNAME(dispatch_ptclass);
            ptcl->cl_Dispatcher.h_SubEntry = NULL;
            ptcl->cl_UserData = (IPTR)NULL;
        }
    }
    return ptcl;
}
