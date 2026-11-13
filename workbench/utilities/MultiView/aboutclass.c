
#include "global.h"

#include <stdio.h>
#include <string.h>

static char * templBase = "%s\n%s\n%s";

void aboutfmt_Picture(char *fmtbuff)
{
    sprintf(fmtbuff, "%s", templBase);
}

void formatImageStrings(char **dimString, char **colorString, ULONG Width, ULONG Height, UBYTE Depth, ULONG Colors, ULONG ModeID)
{
    ULONG RealDepth = Depth;
    ULONG RealColors;

    if ((RealColors = Colors) > 0)
    {
        if ((Depth <= 8) && ((ModeID == INVALID_ID) || ((ModeID & (HAM_KEY|EXTRAHALFBRITE_KEY)) == 0)))
        {
            *colorString = AllocVec(24, MEMF_ANY);
            sprintf(*colorString, "     Colors:  %d", (int)Colors);
        }
        else
        {
            char *modeStr;
            if (Depth > 8)
            {
                unsigned cnt = Colors;
                RealDepth = 1;

                modeStr = "HAMx";

                while (cnt > 0)
                {
                    RealDepth += 1;
                    cnt >>= 1;
                }
                modeStr[3] = RealDepth + '0';
                RealColors = 1 << ((RealDepth - 2) * 3);
            }
            else if (ModeID & HAM_KEY)
            {
                modeStr = "HAMx";
                modeStr[3] = Depth + '0';
                RealColors = 1 << (Depth - 2);
            }
            else
                modeStr = "EHB"; 

            *colorString = AllocVec(28 + strlen(modeStr), MEMF_ANY);
            sprintf(*colorString, "     Colors:  %d (%s)", (int)RealColors, modeStr);
        }
    }
    else
    {
        *colorString = AllocVec(26, MEMF_ANY);

        if (Depth > 24)
            strcpy(*colorString, "     Colors:  Deepcolor");
        else if (Depth > 16)
            strcpy(*colorString, "     Colors:  Truecolor");
        else
            strcpy(*colorString, "     Colors:  Hi-color");
    }
    sprintf(*dimString, "     Dimensions:  %dx%dx%d", Width, Height, RealDepth);
}

void about_Picture(Object *picture, char *details[])
{
    struct BitMapHeader *pictBMH;
    IPTR        pictCols, pictMode;

    if (GetDTAttrs(dto,
        PDTA_ModeID, &pictMode,
        PDTA_BitMapHeader, &pictBMH,
        PDTA_NumColors, &pictCols, TAG_DONE))
    {
        details[0] = AllocVec(36, MEMF_ANY);
        formatImageStrings(&details[0], &details[1], pictBMH->bmh_Width, pictBMH->bmh_Height, pictBMH->bmh_Depth, pictCols, pictMode);
        details[2] = "";
    }
}

void about_PicDisp(char *details[])
{
    FreeVec(details[0]);
    FreeVec(details[1]);
}

void aboutfmt_Animation(char *fmtbuff)
{
    sprintf(fmtbuff, "%s", templBase);
}

void about_Animation(Object *picture, char *details[])
{
    IPTR        animWidth, animHeight, animDepth,
                animCols, animMode,
            animFrames, animFPS;

    if (GetDTAttrs(dto,
        ADTA_Width, &animWidth,
        ADTA_Height, &animHeight,
        ADTA_Depth, &animDepth,
        ADTA_NumColors, &animCols,
        ADTA_ModeID, &animMode,
        ADTA_Frames, &animFrames,
        ADTA_FramesPerSecond, &animFPS, TAG_DONE))
    {
        details[0] = AllocVec(36, MEMF_ANY);
        formatImageStrings(&details[0], &details[1], animWidth, animHeight, animDepth, animCols, animMode);

        details[2] = AllocVec(32, MEMF_ANY);
        sprintf(details[2], "     %d frames @ %dfps", (int)animFrames, (int)animFPS);
    }
}

void about_AnimDisp(char *details[])
{
    FreeVec(details[0]);
    FreeVec(details[1]);
    FreeVec(details[2]);
}

void aboutfmt_Sample(char *fmtbuff)
{
    sprintf(fmtbuff, "%s", templBase);
}

void about_Sample(Object *picture, char *details[])
{
    details[0] = "";
    details[1] = "";
    details[2] = "";
}

void about_SampDisp(char *details[])
{
}

void aboutfmt_Binary(char *fmtbuff)
{
    sprintf(fmtbuff, "%s", templBase);
}

void about_Binary(Object *picture, char *details[])
{
    details[0] = "";
    details[1] = "";
    details[2] = "";
}

void about_BinDisp(char *details[])
{
}

struct DTClassInfo DTClassAbout[] =
{
    { GID_PICTURE, aboutfmt_Picture, about_Picture, about_PicDisp, 10, 3 },
    { GID_ANIMATION, aboutfmt_Animation, about_Animation, about_AnimDisp, 10, 3 },
    { GID_SOUND, aboutfmt_Sample, about_Sample, about_SampDisp, 10, 3 },
    { ID_BINARY, aboutfmt_Binary, about_Binary, about_BinDisp, 10, 3 },
    { 0 }
};

struct DTClassInfo *FindClassInfo(ULONG classid)
{
    struct DTClassInfo *curHandler;
    int i;
    for (i = 0; (DTClassAbout[i].classID != 0); i++)
    {
        curHandler = &DTClassAbout[i];
        if (classid == curHandler->classID)
        {
            return curHandler;
        }
    }
    return NULL;
}
