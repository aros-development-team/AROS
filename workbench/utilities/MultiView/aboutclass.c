
#include "global.h"

#include <stdio.h>
#include <string.h>

static char * templBase = "%s\n%s\n%s";

void aboutfmt_Picture(char *fmtbuff)
{
    sprintf(fmtbuff, "%s", templBase);
}

void about_Picture(Object *picture, char *details[])
{
    details[0] = "";
    details[1] = "";
    details[2] = "";
}

void about_PicDisp(char *details[])
{
}

void aboutfmt_Animation(char *fmtbuff)
{
    sprintf(fmtbuff, "%s", templBase);
}

void about_Animation(Object *picture, char *details[])
{
    IPTR        animWidth, animHeight, animDepth,
                animCols,
            animFrames, animFPS;

    if (GetDTAttrs(dto,
        ADTA_Width, &animWidth,
        ADTA_Height, &animHeight,
        ADTA_Depth, &animDepth,
        ADTA_NumColors, &animCols,
        ADTA_Frames, &animFrames,
        ADTA_FramesPerSecond, &animFPS, TAG_DONE))
    {
        details[0] = AllocVec(36, MEMF_ANY);
        sprintf(details[0], "     Dimensions:  %dx%dx%d", (int)animWidth, (int)animHeight, (int)animDepth);
        details[1] = AllocVec(24, MEMF_ANY);
        sprintf(details[1], "     Colors:  %d", (int)animCols);
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
    { NULL }
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
