/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

static void ShowSemaphore(const char *, struct SignalSemaphore *);
static void ShowLayer(struct Layer *);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

AROS_LH0(void, DumpIntuiState,

         /*  SYNOPSIS */

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 152, Intuition)

/*  FUNCTION
    Private: dump the internal state of intuition.

    INPUTS

    RESULT
        none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#if 1

    struct IntIntuitionBase *IBase = GetPrivIBase(IntuitionBase);
    struct Screen *scr;
    struct Window *win;

    Forbid();

    dprintf("----------------------------------------------------------------\n");
    dprintf("IntuitionBase 0x%lx\n", (ULONG) IntuitionBase);
    ShowSemaphore("IBaseLock", IBase->IBaseLock);
    ShowSemaphore("PubScrListLock", &IBase->PubScrListLock);
    ShowSemaphore("GadgetLock", &IBase->GadgetLock);
    ShowSemaphore("MenuLock", &IBase->MenuLock);
    ShowSemaphore("IntuiActionLock", &IBase->IntuiActionLock);
    ShowSemaphore("InputHandlerLock", &IBase->InputHandlerLock);
    ShowSemaphore("ClassListLock", &IBase->ClassListLock);

    dprintf("\nScreens:\n");

    for (scr = IntuitionBase->FirstScreen; scr; scr = scr->NextScreen)
    {
        dprintf("\nScreen 0x%lx <%s>\n", (ULONG) scr, scr->Title ? scr->Title : (UBYTE *)"NULL");

        dprintf("LayerInfo 0x%lx LockLayersCount %ld\n",
            (ULONG) &scr->LayerInfo, (LONG) scr->LayerInfo.LockLayersCount);
        ShowSemaphore("LayerInfo", &scr->LayerInfo.Lock);
        ShowLayer(GetPrivScreen(scr)->rootLayer);
        ShowLayer(scr->BarLayer);

        for (win = scr->FirstWindow; win; win = win->NextWindow)
        {
            dprintf("\nWindow 0x%lx <%s>\n", (ULONG) win, win->Title ? win->Title : (UBYTE *)"NULL");

            ShowLayer(WLAYER(win));

            if (IS_GZZWINDOW(win))
            {
                ShowLayer(BLAYER(win));
            }
        }
    }

    dprintf("----------------------------------------------------------------\n");

    Permit();

#endif

    AROS_LIBFUNC_EXIT
}

#if 1

void *CheckValidPtr(void *Ptr)
{
    if (((ULONG) Ptr >= 0x20000000UL) &&
        ((ULONG) Ptr < 0x30000000UL))
    {
        return(Ptr);
    }
    if (((ULONG) Ptr >= 0x7000000UL) &&
        ((ULONG) Ptr < 0xb510000UL))
    {
        return(Ptr);
    }
    else if (((ULONG) Ptr >= ((ULONG) 0x11000000)) &&
         ((ULONG) Ptr < ((ULONG) 0x14000000)))
    {
        return(Ptr);
    }
    else if (((ULONG) Ptr >= ((ULONG) 0x10100000)) &&
         ((ULONG) Ptr < ((ULONG) 0x10400000)))
    {
        return(Ptr);
    }
    else if (((ULONG) Ptr >= ((ULONG) 0x10800000)) &&
         ((ULONG) Ptr < ((ULONG) 0x10900000)))
    {
        return(Ptr);
    }
    else if (((ULONG) Ptr >= 0x1000) &&
         ((ULONG) Ptr < 0x00200000))
    {
        return(Ptr);
    }
    else if (((ULONG) Ptr >= 0x00f80000) &&
         ((ULONG) Ptr <  0x01000000))
    {
        return(Ptr);
    }
    else if ((ULONG) Ptr == (ULONG) 0x100000f0)
    {
        return(Ptr);
    }
    return(NULL);
}

#pragma pack(2)
struct  SegTrackerSemaphore
{
    struct  SignalSemaphore Semaphore;
    void                    (*Search)(void);
    struct  MinList         List;
};

struct  SegArray
{
    ULONG   Address;
    ULONG   Size;
};

struct  SegTrackerNode
{
    struct  MinNode         Node;
    char                    *Name;
    struct  SegArray        Array[1];
};
#pragma pack()

static struct SegTrackerSemaphore       *MySegInfo;

#define DEBUG_SEGTRACKER(x)     ;

#undef SysBase

char    *FindAddress(ULONG      Address,
                     ULONG      *Hunk,
                     ULONG      *Offset)
{
    struct SegTrackerNode   *MyNode;
    char                    *Name=NULL;
    ULONG                   Count;

    DEBUG_SEGTRACKER(dprintf("FindAddress(Address 0x%lx Hunk 0x%lx Offset 0x%lx)\n",
                 Address,
                 Hunk,
                 Offset));

    /* MySegInfo isn`t valid after a coldreboot
     * therefore scan for it everytime
     */
    if (TRUE)//(MySegInfo==NULL)
    {
        struct ExecBase *SysBase = *(struct ExecBase **)4;
        MySegInfo =(struct SegTrackerSemaphore*) FindSemaphore("SegTracker");
        DEBUG_SEGTRACKER(dprintf("FindAddress: SegInfo 0x%lx\n",
                     MySegInfo));
    }
    if (MySegInfo)
    {
        if (CheckValidPtr(MySegInfo))
        {
            MyNode    =(struct SegTrackerNode*) MySegInfo->List.mlh_Head;
            DEBUG_SEGTRACKER(dprintf("FindAddress: 1st Node 0x%lx\n",
                         MyNode));
            while ((CheckValidPtr(MyNode)) &&
                (MyNode->Node.mln_Succ) &&
                (!Name))
            {
                Count   =       0;
                DEBUG_SEGTRACKER(dprintf("FindAddress: Current Segment %s\n",
                             MyNode->Name));

                while (MyNode->Array[Count].Address)
                {
                    DEBUG_SEGTRACKER(dprintf("FindAddress: Current Address 0x%lx Size 0x%lx\n",
                                 MyNode->Array[Count].Address,
                                 MyNode->Array[Count].Size));

                    if ((Address > MyNode->Array[Count].Address) &&
                        (Address < (MyNode->Array[Count].Address + MyNode->Array[Count].Size)))
                    {
                        *Hunk               =       Count;
                        *Offset             =       Address - MyNode->Array[Count].Address - 4;
                        Name                =       MyNode->Name;

                        if (Hunk==Offset)
                        {
                            *Hunk     =       MyNode->Array[0].Address;
                        }
                        DEBUG_SEGTRACKER(dprintf("FindAddress: Found %s\n",
                                     Name));
                    }
                    Count++;
                }
                MyNode  =(struct SegTrackerNode*) MyNode->Node.mln_Succ;
            }
        }
    }
    return(Name);
}

void    ShowStack(ULONG Address,
                  ULONG Size)
{
    ULONG   Ptr;
    char    *MyName;
    ULONG   Hunk;
    ULONG   Offset;

    if (CheckValidPtr((ULONG*) Address))
    {
        Ptr =       Address;
        while (Ptr < (Address+Size))
        {
            if ((MyName=FindAddress(*((ULONG*) Ptr),
                            &Hunk,
                            &Offset)))
            {
                dprintf("\t\t0x%lx -> %s Hunk %ld Offset 0x%08lx\n",
                    *((ULONG*) Ptr),
                    MyName,
                    Hunk,
                    Offset);
                Ptr     +=      4;
            }
            else
            {
                Ptr     +=      2;
            }
        }
    }
}

#define MAXLEVEL        10
#define DEBUG_SHOWHISTORY(x)    ;

void    ShowPPCStackHistory(ULONG       *Stack,
                            ULONG       *StackEnd)
{
    char    *MyName;
    ULONG   Hunk;
    ULONG   Offset;
    ULONG   *CurrentStack;
    ULONG   *StackPtr;
    int     i;


    i             =       0;
    CurrentStack  =       Stack;

    DEBUG_SHOWHISTORY(dprintf("ShowPPCStackHistory: Stack 0x%lx StackEnd 0x%lx\n",
                  Stack,
                  StackEnd));

    while (CurrentStack < StackEnd)
    {
        StackPtr    =(ULONG*) CurrentStack[0];      /* Get previous stackframe */
        DEBUG_SHOWHISTORY(dprintf("ShowPPCStackHistory: StackPtr 0x%lx\n",
                      StackPtr));
        if ((StackPtr >= Stack) &&
            (StackPtr < StackEnd))
        {
            if (CheckValidPtr((ULONG*) StackPtr[1]))
            {
                /* Legal LR */

                if ((MyName=FindAddress(StackPtr[1],
                                &Hunk,
                                &Offset)))
                {
                    dprintf("\t\tStackFrame[%ld].LR-> Address 0x%08lx -> %s Hunk %ld Offset 0x%08lx\n",
                        (LONG) i,
                        StackPtr[1],
                        MyName,
                        Hunk,
                        Offset);
                }
                else
                {
                    dprintf("\t\tStackFrame[%ld].LR-> Address 0x%08lx\n",
                        (LONG) i,
                        StackPtr[1]);
                }
#if 0
                PPCDissasemble((APTR) StackPtr[1],
                           4,
                           PPCLibBase);
#endif
            }
            else
            {
                dprintf("\t\tStackFrame[%ld].LR-> Address 0x%08lx **Not Valid**\n\n",
                    (LONG) i,
                    StackPtr[1]);
            }
            i--;
            CurrentStack      =       StackPtr;
            if (i < -MAXLEVEL)
            {
                DEBUG_SHOWHISTORY(dprintf("ShowPPCStackHistory: MaxLevel end\n"));
                break;
            }
        }
        else
        {
            DEBUG_SHOWHISTORY(dprintf("ShowPPCStackHistory: address outside\n"));
            break;
        }
    }
}


/* hack-o-rama */
#if 0
typedef unsigned int        u_int32_t;
typedef double          float64_t;
struct __QVector
{
    u_int32_t   A;
    u_int32_t   B;
    u_int32_t   C;
    u_int32_t   D;
};

typedef struct __QVector        vector128_t;
#endif

#if 1
/*
 * Yeah yeah, this is ugly.. who cares..
 */
typedef unsigned int        u_int32_t;
typedef double          float64_t;
struct __QVector
{
    u_int32_t   A;
    u_int32_t   B;
    u_int32_t   C;
    u_int32_t   D;
};
typedef struct __QVector        vector128_t;


struct PPCRegFrame
{
    u_int32_t       StackGap[4];        /* StackFrame Gap..so a function working
                                 * with the PPCRegFrame as the GPR1 doesn`t
                                 * overwrite any contents with a LR store at 4(1)
                                 */

    u_int32_t       Version;        /* Version of the structure */
    u_int32_t       Type;           /* Type of the regframe */
    u_int32_t       Flags;          /* The filled up registers */
    u_int32_t       State;          /* State of the Thread(only used for Get) */

    u_int32_t       SRR0;
    u_int32_t       SRR1;
    u_int32_t       LR;
    u_int32_t       CTR;

    u_int32_t       CR;
    u_int32_t       XER;

    u_int32_t       GPR[32];

    float64_t       FPR[32];
    float64_t       FPSCR;

    u_int32_t       VSAVE;
    u_int32_t       AlignPad0;
    u_int32_t       AlignPad1;
    u_int32_t       AlignPad2;
    vector128_t     VSCR;
    vector128_t     VMX[32];
    /* no size
        */
};
#endif

void ShowTaskState(struct Task *task)
{
    struct PPCRegFrame *frame = task->tc_ETask->PPCRegFrame;
    struct EmulHandle *emul = (APTR)frame->GPR[2];
    char *MyName;
    ULONG Offset, Hunk;

    dprintf("\t\tSRR0 0x%lx LR 0x%lx CTR 0x%lx PC 0x%lx\n",
        (ULONG) frame->SRR0, (ULONG) frame->LR, (ULONG) frame->CTR,
        emul->Flags & EMULFLAGSF_PPC ? (ULONG) emul->PC : (ULONG) frame->GPR[13]);

    if (emul->Flags & EMULFLAGSF_PPC)
    {
        if ((MyName=FindAddress((ULONG) frame->SRR0,
                        &Hunk,
                        &Offset)))
        {
            dprintf("\t\tSRR0 -> %s Hunk %ld Offset 0x%08lx\n",
                MyName,
                Hunk,
                Offset);
        }

        if ((MyName=FindAddress((ULONG) frame->LR,
                        &Hunk,
                        &Offset)))
        {
            dprintf("\t\t  LR -> %s Hunk %ld Offset 0x%08lx\n",
                MyName,
                Hunk,
                Offset);
        }

        if ((MyName=FindAddress((ULONG) frame->CTR,
                        &Hunk,
                        &Offset)))
        {
            dprintf("\t\t CTR -> %s Hunk %ld Offset 0x%08lx\n",
                MyName,
                Hunk,
                Offset);
        }


        ShowPPCStackHistory((ULONG*) frame->GPR[1],
                    (ULONG*) (frame->GPR[1] + 1000));

        if ((MyName=FindAddress((ULONG) emul->PC,
                        &Hunk,
                        &Offset)))
        {
            dprintf("\t\tPC -> %s Hunk %ld Offset 0x%08lx\n",
                MyName,
                Hunk,
                Offset);
        }
        ShowStack(emul->An[7],
              0x80);
    }
    else
    {
        if ((MyName=FindAddress((ULONG) frame->GPR[13],
                        &Hunk,
                        &Offset)))
        {
            dprintf("\t\tPC -> %s Hunk %ld Offset 0x%08lx\n",
                MyName,
                Hunk,
                Offset);
        }
        ShowStack(frame->GPR[31],
              0x80);
    }
}

void ShowSemaphore(const char *name, struct SignalSemaphore *sem)
{
    struct SemaphoreRequest *req;

    if (name)
        dprintf("Semaphore %s:\n", name);

    dprintf("Semaphore 0x%lx NestCount %d QueueCount %d Owner 0x%lx <%s>\n",
        (ULONG) sem, sem->ss_NestCount, sem->ss_QueueCount,
        (ULONG) sem->ss_Owner, sem->ss_Owner && sem->ss_Owner->tc_Node.ln_Name ?
        sem->ss_Owner->tc_Node.ln_Name : "NULL");
    if (sem->ss_Owner)
        ShowTaskState(sem->ss_Owner);

    ForeachNode(&sem->ss_WaitQueue, req)
    {
        dprintf("\tTask 0x%lx <%s>\n", (ULONG) req->sr_Waiter,
            req->sr_Waiter->tc_Node.ln_Name ? req->sr_Waiter->tc_Node.ln_Name : "NULL");
        ShowTaskState(req->sr_Waiter);
    }
}

void ShowLayer(struct Layer *layer)
{
    if (layer)
    {
        dprintf("Layer 0x%lx\n", (ULONG) layer);
        ShowSemaphore(NULL, &layer->Lock);
    }
}

#endif
