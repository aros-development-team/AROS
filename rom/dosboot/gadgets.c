/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>

#include "dosboot_intern.h"

struct ButtonGadget *createButton
	(
		ULONG left, ULONG top, ULONG width, ULONG height,
		struct Gadget *prev,
		STRPTR name,
		UWORD id,
		struct DOSBootBase *DOSBootBase
	)
{
	struct TagItem tags[] =
	{
		{GA_Left,           (IPTR)left}, /* 0 */
		{GA_Top,             (IPTR)top}, /* 1 */
		{GA_Height,       (IPTR)height}, /* 2 */
		{GA_Width,         (IPTR)width}, /* 3 */
		{GA_Border,         0}, /* 4 */
		{GA_SelectRender,   0}, /* 5 */
		{GA_Previous, (IPTR)prev}, /* 6 */
		{GA_Text,     (IPTR)name}, /* 7 */
		{GA_ID,         (IPTR)id}, /* 8 */
		{GA_Immediate,      TRUE},
		{GA_RelVerify,      TRUE},
		{TAG_DONE,           0UL}
	};
	struct ButtonGadget *button;

        D(bug("[BootMenu] createButton()\n"));

	if ((button = AllocMem(sizeof(struct ButtonGadget), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
	{
                if (!(prev)) tags[6].ti_Tag = TAG_IGNORE;

		tags[4].ti_Data = (IPTR)&button->uborder2;
		tags[5].ti_Data = (IPTR)&button->sborder2;
		button->XY1[1] = height;
		button->XY1[4] = width;
		button->XY2[0] = width;
		button->XY2[2] = width;
		button->XY2[3] = height;
		button->XY2[5] = height;
		button->uborder1.FrontPen = 1;
		button->uborder1.DrawMode = JAM1;
		button->uborder1.Count = 3;
		button->uborder1.XY = button->XY2;
		button->uborder2.FrontPen = 2;
		button->uborder2.DrawMode = JAM1;
		button->uborder2.Count = 3;
		button->uborder2.XY = button->XY1;
		button->uborder2.NextBorder = &button->uborder1;
		button->sborder1.FrontPen = 2;
		button->sborder1.DrawMode = JAM1;
		button->sborder1.Count = 3;
		button->sborder1.XY = button->XY2;
		button->sborder2.FrontPen = 1;
		button->sborder2.DrawMode = JAM1;
		button->sborder2.Count = 3;
		button->sborder2.XY = button->XY1;
		button->sborder2.NextBorder = &button->sborder1;
		if ((button->gadget = NewObjectA(NULL, FRBUTTONCLASS, tags)) != NULL)
		{
                    if (prev != NULL)
                    {
			prev->NextGadget = button->gadget;
                    }
                    return button;
		}
		FreeMem(button, sizeof(struct ButtonGadget));
	}
	return NULL;
}

void freeButtonGadget(struct ButtonGadget *button, struct DOSBootBase *DOSBootBase) 
{
        D(bug("[BootMenu] freeButtonGadget()\n"));
	if (button == NULL)
		return;
	DisposeObject(button->gadget);
	FreeMem(button, sizeof(struct ButtonGadget));
}
