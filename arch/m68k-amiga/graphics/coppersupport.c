/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

/****************************************************************************************/
#include <aros/debug.h>

#include "coppersupport.h"

void pokeCL(UWORD *ci, UWORD target, UWORD table)
{
    ULONG targ = (ULONG)target;
    if(!ci) return;
    targ &= 0x1fe;
    for(;ci;ci+=2)
    {
        if( ((*ci)==0xffff) && ((*(ci+1) == 0xfffe))) return;
        if(*ci == targ) break;
    }
    if (((*ci) == 0xffff) && ((*(ci+1)==0xfffe))) return;
    if(*ci == targ)
    {
        ci++;
        *ci++ = table;
    }
}

/****************************************************************************************/

struct CopIns *pokeCI(struct CopIns *ci, UWORD *field1, short field2)
{
    struct CopIns *c;
    UWORD op = COPPER_MOVE;
    c = ci;
    if(c)
    {
        short out = FALSE;
        while(!out)
        {
            switch(c->OpCode & 3)
            {
                case COPPER_MOVE:
                {
                    if(c->DESTADDR == (((UWORD)field1) & 0x1fe))
                    {
                        short mask;
                        if((mask = op&0xC000))
                        {
                            if(c->OpCode & mask)
                            {
                                c->DESTDATA = field2;
                                return c;
                            }
                        }
                        else
                        {
                            c->DESTDATA = field2;
                            return c;
                        }
                    }
                    c++;
                    break;
                }
                case COPPER_WAIT:
                {
                    if(c->HWAITPOS == 255)
                    {
                        return 0;
                    }
                    else c++;
                    break;
                }
                case CPRNXTBUF:
                {
                    if(c->NXTLIST == NULL)
                    {
                        out = TRUE;
                    }
                    else
                    {
                        if((c = c->NXTLIST->CopIns) == NULL)
                        {
                            out = TRUE;
                        }
                    }
                    break;
                }
                default:
                {
                    out=TRUE;
                    break;
                }
            }
        }
    }
    return 0;
}

/****************************************************************************************/
