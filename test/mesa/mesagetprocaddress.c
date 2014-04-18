#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>


#include <GL/gla.h>
#include <GL/gl.h>

#include <stdio.h>

GLAContext glcont=NULL;

#define RAND_COL 1.0

PFNGLUNIFORM4IPROC AROSglUniform4i = NULL;

void testextensions()
{
    printf("Getting pointer to glUniform4i...\n");
    AROSglUniform4i = glAGetProcAddress("glUniform4i");

    if (AROSglUniform4i != NULL)
    {
        printf("Pointer to glUniform4i acquired\n");
        printf("Calling glUniform4i via pointer\n");
        AROSglUniform4i(0, 0, 0, 0, 0);
        printf("glUniform4i called\n");
    }
    else
    {
        printf("Failed to get pointer to glUniform4i");
    }
}

struct Window * win = NULL;

void initmesa()
{
    struct TagItem attributes [ 14 ]; /* 14 should be more than enough :) */
    int i = 0;
    
    // default config. Always used...
    attributes[i].ti_Tag = GLA_Window;      attributes[i++].ti_Data = (IPTR)win;
    attributes[i].ti_Tag = GLA_Left;        attributes[i++].ti_Data = win->BorderLeft;
    attributes[i].ti_Tag = GLA_Top;         attributes[i++].ti_Data = win->BorderTop;
    attributes[i].ti_Tag = GLA_Bottom;      attributes[i++].ti_Data = win->BorderBottom;
    attributes[i].ti_Tag = GLA_Right;       attributes[i++].ti_Data = win->BorderRight;

    // double buffer ?
    attributes[i].ti_Tag = GLA_DoubleBuf;   attributes[i++].ti_Data = GL_TRUE;

    // RGB(A) Mode ?
    attributes[i].ti_Tag = GLA_RGBMode;     attributes[i++].ti_Data = GL_TRUE;
    
    // done...
    attributes[i].ti_Tag    = TAG_DONE;

    glcont = glACreateContext(attributes);
    glAMakeCurrent(glcont);
}

void deinitmesa()
{
    glADestroyContext(glcont);
}



/*
** Open a simple window using OpenWindowTagList()
*/
int main(int argc, char **argv)
{

    win = OpenWindowTags(0,
                WA_Title, (IPTR)"MesaGetProcAddress",
                WA_CloseGadget, TRUE,
                WA_DragBar, TRUE,
                WA_DepthGadget, TRUE,
                WA_Left, 10,
                WA_Top, 10,
                WA_InnerWidth, 300,
                WA_InnerHeight, 300,
                WA_Activate, TRUE,
                WA_RMBTrap, TRUE,
                WA_SimpleRefresh, TRUE,
                WA_NoCareRefresh, TRUE,
                WA_IDCMP, IDCMP_CLOSEWINDOW,
                TAG_DONE);

    initmesa();

    testextensions();

    deinitmesa();

    CloseWindow(win);

    return 0;
}

 
