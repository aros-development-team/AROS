/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * Testing WeighTAMatch
 * Flags: FPF_PROPORTIONAL, FPF_TALLDOT, FPF_WIDEDOT
 * Style: FSF_EXTENDED, FSF_BOLD, FSF_UNDERLINED, FSF_ITALIC, FSF_COLORFONT
 */

#include <proto/graphics.h>
#include <graphics/text.h>

#include <stdio.h>
#define TA(ta) ((struct TextAttr *) (ta))

int main(void)
{
    struct TTextAttr tta1, tta2;
    static UBYTE flags[] = {FPF_ROMFONT, FPF_DISKFONT, FPF_REVPATH, FPF_TALLDOT, FPF_WIDEDOT, FPF_PROPORTIONAL,
	    FPF_DESIGNED, FPF_REMOVED
    };
    static UBYTE styles[] = {~0, FSF_UNDERLINED, FSF_BOLD, FSF_ITALIC, FSF_EXTENDED, FSF_COLORFONT};
    static UWORD sizes[] = {6, 10, 16, 17, 18, 19, 32, 64};
    int i;
    
    tta1.tta_Name = tta2.tta_Name = "test.font";
    tta1.tta_YSize = tta2.tta_YSize = 16;
    tta1.tta_Style = tta2.tta_Style = 0;
    tta1.tta_Flags = tta2.tta_Flags = 0;

    /* What is the weight for different sizes ? */
    for (i = 0; i < 7; i++)
    {
	tta2.tta_YSize = sizes[i];
	printf("Size: %d, Weight1: %d, Weight2: %d\n",
	       (int)sizes[i],
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta1), TA(&tta2), NULL),
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta2), TA(&tta1), NULL));
    }
    tta2.tta_YSize = 16;

    printf("\nPositive flags\n");
    /* What is the weight for different styles ? */
    for (i = 0; i < 6; i++)
    {
	tta2.tta_Style = styles[i];
	printf("Style: %d, Weight1: %d, Weight2: %d\n",
	       (int)styles[i],
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta1), TA(&tta2), NULL),
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta2), TA(&tta1), NULL));
    }
    tta2.tta_Style = 0;

    /* What is the weight for different flags ? */
    for (i = 0; i < 8; i++)
    {
	tta2.tta_Flags = flags[i];
	printf("Flags: %d, Weight1: %d, Weight2: %d\n",
	       (int)flags[i],
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta1), TA(&tta2), NULL),
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta2), TA(&tta1), NULL));
    }
    
    tta1.tta_Style = tta2.tta_Style = (UBYTE) ~FSF_TAGGED;
    tta1.tta_Flags = tta2.tta_Flags = ~0;

    printf("\nNegative flags\n");
    /* What is the weight for different styles ? */
    for (i = 0; i < 6; i++)
    {
	tta2.tta_Style = ~styles[i];
	printf("Style: %d, Weight1: %d, Weight2: %d\n",
	       (int)styles[i],
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta1), TA(&tta2), NULL),
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta2), TA(&tta1), NULL));
    }
    tta2.tta_Style = (UBYTE) ~FSF_TAGGED;

    /* What is the weight for different flags ? */
    for (i = 0; i < 8; i++)
    {
	tta2.tta_Flags = ~flags[i];
	printf("Flags: %d, Weight1: %d, Weight2: %d\n",
	       (int)flags[i],
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta1), TA(&tta2), NULL),
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta2), TA(&tta1), NULL));
    }
    
    /* The weight for some random combinations */
    tta1.tta_YSize = 10;
    tta1.tta_Style = FSF_EXTENDED | FSF_BOLD;
    tta1.tta_Flags = FPF_DESIGNED | FPF_WIDEDOT;
    tta2.tta_YSize = 13;
    tta2.tta_Style = FSF_BOLD;
    tta2.tta_Flags = FPF_DESIGNED;
    printf("\nRandom: 1, Weight1: %d, Weight2: %d\n",
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta1), TA(&tta2), NULL),
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta2), TA(&tta1), NULL));

    tta1.tta_YSize = 12;
    tta1.tta_Style = 0;
    tta1.tta_Flags = FPF_DESIGNED;
    tta2.tta_YSize = 12;
    tta2.tta_Style = 0;
    tta2.tta_Flags = FPF_DISKFONT;
    printf("Random: 2, Weight1: %d, Weight2: %d\n",
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta1), TA(&tta2), NULL),
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta2), TA(&tta1), NULL));

    tta1.tta_YSize = 15;
    tta1.tta_Style = FSF_BOLD;
    tta1.tta_Flags = FPF_DESIGNED | FPF_WIDEDOT;
    tta2.tta_YSize = 15;
    tta2.tta_Style = FSF_ITALIC;
    tta2.tta_Flags = FPF_ROMFONT | FPF_REMOVED;
    printf("Random: 3, Weight1: %d, Weight2: %d\n",
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta1), TA(&tta2), NULL),
	       MAXFONTMATCHWEIGHT - WeighTAMatch(TA(&tta2), TA(&tta1), NULL));

    return 0;
}
