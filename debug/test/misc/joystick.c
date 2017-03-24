/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/lowlevel.h>
#include <libraries/lowlevel_ext.h>

#include <stdio.h>
#include <stdlib.h>


struct Library *LowLevelBase;

static void printbuttons(ULONG val)
{
    if (val & JPF_BUTTON_PLAY)      printf("[PLAY/MMB]");
    if (val & JPF_BUTTON_REVERSE)   printf("[REVERSE]");
    if (val & JPF_BUTTON_FORWARD)   printf("[FORWARD]");
    if (val & JPF_BUTTON_GREEN)     printf("[SHUFFLE]");
    if (val & JPF_BUTTON_RED)       printf("[SELECT/LMB/FIRE]");
    if (val & JPF_BUTTON_BLUE)      printf("[STOP/RMB]");
}

static void printmousedirections(ULONG val)
{
    printf("[%d,%d]", (int)(val & JP_MHORZ_MASK), (int)(val & JP_MVERT_MASK) >> 8);
}

static void printajoydirections(ULONG val)
{
    printf("[%d, %d]", (int)(val & JP_XAXIS_MASK), (int)(val & JP_YAXIS_MASK) >> 8);
}
static void printjoydirections(ULONG val)
{
    if (val & JPF_JOY_UP)       printf("[UP]");
    if (val & JPF_JOY_DOWN)     printf("[DOWN]");
    if (val & JPF_JOY_LEFT)     printf("[LEFT]");
    if (val & JPF_JOY_RIGHT)    printf("[RIGHT]");
}

static void printjoyport(ULONG val)
{
    int i;
    
    for(i = 31; i >= 0; i--)
    {
    	printf("%d", (val & (1 << i)) ? 1 : 0);
    }
    
    printf(" - ");
    
    if ((val & JP_TYPE_MASK) == JP_TYPE_NOTAVAIL) printf("NOT AVAILABLE");
    if ((val & JP_TYPE_MASK) == JP_TYPE_UNKNOWN)  printf("UNKNOWN");
    
    if ((val & JP_TYPE_MASK) == JP_TYPE_JOYSTK)
    {
        printf("JOYSTICK - ");
        printjoydirections(val);
        printbuttons(val);
    }
    
    if ((val & JP_TYPE_MASK) == JP_TYPE_GAMECTLR)
    {
        printf("GAME CONTROLLER - ");
        printjoydirections(val);
        printbuttons(val);
    }

    if ((val & JP_TYPE_MASK) == JP_TYPE_MOUSE)
    {
        printf("MOUSE - ");
        printmousedirections(val);
        printbuttons(val);
    }
    
    if ((val & JP_TYPE_MASK) == JP_TYPE_ANALOGUE)
    {
        printf("JOYSTICK[ANALOGUE] - ");
        printajoydirections(val);
        printbuttons(val);
    }

    printf("\n");
}

int main(int argc, char **argv)
{
    int unit = 1;

    if (argc == 2) unit = atoi(argv[1]);

    LowLevelBase = OpenLibrary("lowlevel.library", 0);

    if (LowLevelBase)
    {
        ULONG old = 0;

        while(!CheckSignal(SIGBREAKF_CTRL_C))
        {
            ULONG new;

            new = ReadJoyPort(unit);
            if (new != old)
            {
	            old = new;
                printjoyport(new);
            }

            Delay(1);
        }
        CloseLibrary(LowLevelBase);
    }

    return 0;
}
