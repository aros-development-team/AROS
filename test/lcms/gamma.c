/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <lcms2.h>

#include <stdio.h>

// get gamma from ICC file

int main(int argc, char **argv)
{
    char *filename;
    cmsHPROFILE hProfile;
    cmsToneCurve *tc_red, *tc_green, *tc_blue;
    int i;

    if (argc == 2)
    {
        filename = argv[1];
    }
    else
    {
        filename = "SYS:Prefs/Presets/Colortables/ICC/sRGB.icc";
    }

    hProfile = cmsOpenProfileFromFile(filename, "r");
    if (hProfile)
    {
        tc_red = cmsReadTag(hProfile, cmsSigRedTRCTag);
        tc_green = cmsReadTag(hProfile, cmsSigGreenTRCTag);
        tc_blue = cmsReadTag(hProfile, cmsSigBlueTRCTag);
        if (tc_red && tc_green && tc_blue)
        {
            for (i=0; i < 256; i++)
            {
                printf("i %d red %u green %u blue %u\n", i, cmsEvalToneCurve16(tc_red, i * 256) >> 8, cmsEvalToneCurve16(tc_green, i * 256) >> 8, cmsEvalToneCurve16(tc_blue, i * 256) >> 8);
            }
        }
        else
        {
            puts("Gamma tags missing");
        }
        cmsCloseProfile(hProfile);
    }
    else
    {
        puts("Couldn't open ICC file");
        return 1;
    }
    return 0;
}
