/*
    Copyright © 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <proto/timer.h>
#include <devices/timer.h>
#include <proto/cybergraphics.h>

#include <GL/gla.h>

#include <stdio.h>

#define SHOW_FPS

GLAContext          glcont=NULL;
double              angle = 0.0;
double              angle_inc = 0.0;
BOOL                finished = FALSE;
struct Window *     win = NULL;
#if defined (SHOW_FPS)
struct Device *     TimerBase = NULL;
struct timerequest  timereq;
struct MsgPort      timeport;
#endif
struct Library *    CyberGfxBase = NULL;
BOOL                fullscreen = FALSE, trace = FALSE;
#define DOTRACE(x)      ({ if (trace) { x } });
GLuint              fragmentShader = 0;
GLuint              vertexShader = 0;
GLuint              shaderProgram = 0;
GLint               angleLocation = 0;

PFNGLCREATESHADERPROC       glCreateShader          = NULL;
PFNGLSHADERSOURCEPROC       glShaderSource          = NULL;
PFNGLCOMPILESHADERPROC      glCompileShader         = NULL;
PFNGLGETSHADERINFOLOGPROC   glGetShaderInfoLog      = NULL;
PFNGLCREATEPROGRAMPROC      glCreateProgram         = NULL;
PFNGLATTACHSHADERPROC       glAttachShader          = NULL;
PFNGLLINKPROGRAMPROC        glLinkProgram           = NULL;
PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog     = NULL;
PFNGLUSEPROGRAMPROC         glUseProgram            = NULL;
PFNGLDETACHSHADERPROC       glDetachShader          = NULL;
PFNGLDELETESHADERPROC       glDeleteShader          = NULL;
PFNGLDELETEPROGRAMPROC      glDeleteProgram         = NULL;
PFNGLUNIFORM1FPROC          glUniform1f             = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation    = NULL;

CONST_STRPTR version = "$VER: glsimplerendering 1.0 (28.04.2019) ©2010-2019 The AROS Development Team";

const GLchar * fragmentShaderSource =
"uniform float angle;"
"void main()"
"{"
"   vec4 v = vec4(gl_Color);"
"   float intensity = abs(1.0f - (mod(angle, 1440.0f) / 720.0f));"
"   v.b = v.b * intensity;"
"   v.g = v.g * (1.0f - intensity);"
"   gl_FragColor = v;"
"}";

const GLchar * vertexShaderSource =
"void main()"
"{  "
"   gl_FrontColor = gl_Color;"
"   gl_Position = ftransform();"
"}";


#define RAND_COL 1.0f
#define DEGREES_PER_SECOND 180.0
#define USE_PERSPECTIVE 1

void prepare_shader_program()
{
#define BUFFER_LEN 2048
    char buffer[BUFFER_LEN] = {0};
    int len;

    DOTRACE(bug("\n[GLSimpeRend] Loading Shader Programs ...\n");)

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderInfoLog(fragmentShader, BUFFER_LEN, &len, buffer);
    printf("Fragment shader compile output: %s\n", buffer);
    
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderInfoLog(vertexShader, BUFFER_LEN, &len, buffer);
    printf("Vertex shader compile output: %s\n", buffer);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram); 
    glGetProgramInfoLog(shaderProgram, BUFFER_LEN, &len, buffer);
    printf("Shader program compile output: %s\n", buffer);

    DOTRACE(bug("\n[GLSimpeRend] Loading finished\n");)

#undef BUFFER_LEN    
}

void cleanup_shader_program()
{
    DOTRACE(bug("\n[GLSimpeRend] Cleanup Shader Programs ...\n");)

    glUseProgram(0);
    glDetachShader(shaderProgram, fragmentShader);
    glDetachShader(shaderProgram, vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteProgram(shaderProgram);

    DOTRACE(bug("\n[GLSimpeRend] Cleanup Shaders finished\n");)
}

void render_face()
{
    DOTRACE(bug("\n[GLSimpeRend] Render Face ...\n");)

    glBegin(GL_QUADS);
        glColor4f(RAND_COL , 0.0, RAND_COL, 0.3);
        glVertex3f(-0.25, -0.25, 0.0);
        glColor4f(0, RAND_COL, RAND_COL, 0.3);
        glVertex3f(-0.25, 0.25, 0.0);
        glColor4f(0 , 0, 0, 0.3);
        glVertex3f(0.25, 0.25, 0.0);
        glColor4f(RAND_COL , RAND_COL, 0, 0.3);
        glVertex3f(0.25, -0.25, 0.0);
    glEnd();

    DOTRACE(bug("\n[GLSimpeRend] Render Face finished\n");)
}

void render_cube()
{
    DOTRACE(bug("\n[GLSimpeRend] Render Cube ...\n");)

    glPushMatrix();
    glRotatef(0.0, 0.0, 1.0, 0.0);
    glTranslatef(0.0, 0.0, 0.25);
    render_face();
    glPopMatrix();


    glPushMatrix();
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glTranslatef(0.0, 0.0, 0.25);
    render_face();
    glPopMatrix();

    glPushMatrix();
    glRotatef(180.0, 0.0, 1.0, 0.0);
    glTranslatef(0.0, 0.0, 0.25);
    render_face();
    glPopMatrix();

    glPushMatrix();
    glRotatef(270.0, 0.0, 1.0, 0.0);
    glTranslatef(0.0, 0.0, 0.25);
    render_face();
    glPopMatrix();

    glPushMatrix();
    glRotatef(90.0, 1.0, 0.0, 0.0);
    glTranslatef(0.0, 0.0, 0.25);
    render_face();
    glPopMatrix();

    glPushMatrix();
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    glTranslatef(0.0, 0.0, 0.25);
    render_face();
    glPopMatrix();

    DOTRACE(bug("\n[GLSimpeRend] Render Cube finished\n");)
}

void render_triangle()
{
    DOTRACE(bug("\n[GLSimpeRend] Render Triangle ...\n");)
   
    glBegin(GL_TRIANGLES);
        glColor4f(1.0, 0.0, 0.0, 1.0);
        glVertex3f(-0.25, -0.25, 0.0);
        glColor4f(0.0, 1.0, 0.0, 1.0);
        glVertex3f(-0.25,  0.25, 0.0);
        glColor4f(0.0, 0.0, 1.0, 1.0);
        glVertex3f( 0.25,  0.25, 0.0);
    glEnd();

    DOTRACE(bug("\n[GLSimpeRend] Render Triangle finished\n");)
}

void render()
{
    DOTRACE(bug("\n[GLSimpeRend] Render ...\n");)

    glLoadIdentity();
    glClearColor(0.3, 0.3, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    glEnable(GL_BLEND);

    angle += angle_inc;
    glUniform1f(angleLocation, angle);

#if USE_PERSPECTIVE == 1
    glTranslatef(0.0, 0.0, -6.0);
#endif    
    glPushMatrix();
    glRotatef(angle, 0.0, 1.0, 0.0);
    glTranslatef(0.0, 0.0, 0.25);
    glRotatef(angle, 1.0, 0.0, 1.0);
    render_cube();
    glPopMatrix();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glASwapBuffers(glcont);

    DOTRACE(bug("\n[GLSimpeRend] Render finished\n");)
}    

#define VISIBLE_WIDTH 300
#define VISIBLE_HEIGHT 300

static void initextensions()
{
    glCreateShader          = (PFNGLCREATESHADERPROC)glAGetProcAddress("glCreateShader");
    glShaderSource          = (PFNGLSHADERSOURCEPROC)glAGetProcAddress("glShaderSource");
    glCompileShader         = (PFNGLCOMPILESHADERPROC)glAGetProcAddress("glCompileShader");
    glGetShaderInfoLog      = (PFNGLGETSHADERINFOLOGPROC)glAGetProcAddress("glGetShaderInfoLog");
    glCreateProgram         = (PFNGLCREATEPROGRAMPROC)glAGetProcAddress("glCreateProgram");
    glAttachShader          = (PFNGLATTACHSHADERPROC)glAGetProcAddress("glAttachShader");
    glLinkProgram           = (PFNGLLINKPROGRAMPROC)glAGetProcAddress("glLinkProgram");
    glGetProgramInfoLog     = (PFNGLGETPROGRAMINFOLOGPROC)glAGetProcAddress("glGetProgramInfoLog");
    glUseProgram            = (PFNGLUSEPROGRAMPROC)glAGetProcAddress("glUseProgram");
    glDetachShader          = (PFNGLDETACHSHADERPROC)glAGetProcAddress("glDetachShader");
    glDeleteShader          = (PFNGLDELETESHADERPROC)glAGetProcAddress("glDeleteShader");
    glDeleteProgram         = (PFNGLDELETEPROGRAMPROC)glAGetProcAddress("glDeleteProgram");
    glUniform1f             = (PFNGLUNIFORM1FPROC)glAGetProcAddress("glUniform1f");
    glGetUniformLocation    = (PFNGLGETUNIFORMLOCATIONPROC)glAGetProcAddress("glGetUniformLocation");
}

void initgl()
{
    struct TagItem attributes [ 14 ]; /* 14 should be more than enough :) */
    int i = 0;
    GLfloat h = 0.0f;
    
    attributes[i].ti_Tag = GLA_Window;      attributes[i++].ti_Data = (IPTR)win;
    attributes[i].ti_Tag = GLA_Left;        attributes[i++].ti_Data = win->BorderLeft;
    attributes[i].ti_Tag = GLA_Top;         attributes[i++].ti_Data = win->BorderTop;
    attributes[i].ti_Tag = GLA_Bottom;      attributes[i++].ti_Data = win->BorderBottom;
    attributes[i].ti_Tag = GLA_Right;       attributes[i++].ti_Data = win->BorderRight;

    // double buffer ?
    attributes[i].ti_Tag = GLA_DoubleBuf;   attributes[i++].ti_Data = GL_TRUE;

    // RGB(A) Mode ?
    attributes[i].ti_Tag = GLA_RGBMode;     attributes[i++].ti_Data = GL_TRUE;
    
    /* Stencil/Accum */
    attributes[i].ti_Tag = GLA_NoStencil;   attributes[i++].ti_Data = GL_TRUE;
    attributes[i].ti_Tag = GLA_NoAccum;     attributes[i++].ti_Data = GL_TRUE;
    
    // done...
    attributes[i].ti_Tag    = TAG_DONE;

    glcont = glACreateContext(attributes);
    if (glcont)
    {
        glAMakeCurrent(glcont);
        h = (GLfloat)VISIBLE_HEIGHT / (GLfloat)VISIBLE_WIDTH ;

        glViewport(0, 0, (GLint) VISIBLE_WIDTH, (GLint) VISIBLE_HEIGHT);
#if USE_PERSPECTIVE == 1
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(-1.0, 1.0, -h, h, 5.0, 200.0);
        glMatrixMode(GL_MODELVIEW);
#endif
        initextensions();
        prepare_shader_program();
        glUseProgram(shaderProgram);
        angleLocation = glGetUniformLocation(shaderProgram, "angle");
    }
    else
        finished = TRUE; /* Failure. Stop */
}

void deinitgl()
{
    if (glcont) 
    {
        cleanup_shader_program();
        glADestroyContext(glcont);
    }
}

static int init_timerbase()
{
#if defined(SHOW_FPS)
    timeport.mp_Node.ln_Type   = NT_MSGPORT;
    timeport.mp_Node.ln_Pri    = 0;
    timeport.mp_Node.ln_Name   = NULL;
    timeport.mp_Flags          = PA_IGNORE;
    timeport.mp_SigTask        = FindTask(NULL);
    timeport.mp_SigBit         = 0;
    NEWLIST(&timeport.mp_MsgList);

    timereq.tr_node.io_Message.mn_Node.ln_Type    = NT_MESSAGE;
    timereq.tr_node.io_Message.mn_Node.ln_Pri     = 0;
    timereq.tr_node.io_Message.mn_Node.ln_Name    = NULL;
    timereq.tr_node.io_Message.mn_ReplyPort       = &timeport;
    timereq.tr_node.io_Message.mn_Length          = sizeof (timereq);

    if(OpenDevice("timer.device",UNIT_VBLANK,(struct IORequest *)&timereq,0) == 0)
    {
        TimerBase = (struct Device *)timereq.tr_node.io_Device;
        return 1;
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}


static void deinit_timerbase()
{
#if defined(SHOW_FPS)
    if (TimerBase != NULL)
        CloseDevice((struct IORequest *)&timereq);
#endif
}


void HandleIntuiMessages(void)
{
    struct IntuiMessage *msg;

    while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
    {
        switch(msg->Class)
        {
        case IDCMP_CLOSEWINDOW:
            finished = TRUE;
            break;
        case IDCMP_VANILLAKEY:
            if (msg->Code == 27 /* ESC */) finished = TRUE;
            break;
        }
        ReplyMsg((struct Message *)msg);
    }
}

#define ARG_FULLSCREEN  0
#define ARG_TRACE       1
#define NUM_ARGS        2

STATIC CONST_STRPTR   TEMPLATE=(CONST_STRPTR) "FULLSCREEN/S,TRACE/S";
static struct RDArgs  *myargs;
static IPTR           args[NUM_ARGS];

void get_arguments(void)
{
    if((myargs = ReadArgs(TEMPLATE, args, NULL)))
    {
        if ((BOOL)args[ARG_FULLSCREEN])
        {
            fullscreen = TRUE;
        }
        if ((BOOL)args[ARG_TRACE])
        {
            trace = TRUE;
        }
        FreeArgs(myargs);
    }
}

/*
** Open a simple window using OpenWindowTagList()
*/
int main(void)
{
    ULONG framecnt = 0;
//    ULONG exitcounter = 0;
    TEXT title[100];
    struct Screen * pubscreen = NULL;
    struct Screen * customscreen = NULL;
    LONG modeid;
    
    struct timeval tv;
    UQUAD lastmicrosecs = 0L;
    UQUAD currmicrosecs = 0L;
    UQUAD fpsmicrosecs = 0L;
    
    get_arguments();
    
    init_timerbase();
    
#if defined(SHOW_FPS)
    GetSysTime(&tv);
    lastmicrosecs = tv.tv_secs * 1000000 + tv.tv_micro;
    fpsmicrosecs = lastmicrosecs;
#endif

    if (fullscreen)
    {
        CyberGfxBase = OpenLibrary("cybergraphics.library", 0L);
        
        modeid = BestCModeIDTags(CYBRBIDTG_NominalWidth, VISIBLE_WIDTH,
                                    CYBRBIDTG_NominalHeight, VISIBLE_HEIGHT,
                                    TAG_DONE);
        
        customscreen = OpenScreenTags(NULL,
                            SA_Type,        CUSTOMSCREEN,
                            SA_DisplayID,   modeid,
                            SA_Width,       VISIBLE_WIDTH,
                            SA_Height,      VISIBLE_HEIGHT,
                            SA_ShowTitle,   FALSE,
                            SA_Quiet,       TRUE,
                            TAG_DONE);

        win = OpenWindowTags(NULL,
                WA_Left,            0,
                WA_Top,             200,
                WA_InnerWidth,      VISIBLE_WIDTH,
                WA_InnerHeight,     VISIBLE_HEIGHT,
                WA_CustomScreen,    (IPTR)customscreen,
                WA_Flags,           WFLG_ACTIVATE | WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_RMBTRAP,
                WA_IDCMP,           IDCMP_VANILLAKEY,
                TAG_DONE);
    }
    else
    {
        if ((pubscreen = LockPubScreen(NULL)) == NULL) return 1;
        
        win = OpenWindowTags(0,
                            WA_Title, (IPTR)"GLSimpleRendering",
                            WA_PubScreen, pubscreen,
                            WA_CloseGadget, TRUE,
                            WA_DragBar, TRUE,
                            WA_DepthGadget, TRUE,
                            WA_Left, 50,
                            WA_Top, 200,
                            WA_InnerWidth, VISIBLE_WIDTH,
                            WA_InnerHeight, VISIBLE_HEIGHT,
                            WA_Activate, TRUE,
                            WA_RMBTrap, TRUE,
                            WA_SimpleRefresh, TRUE,
                            WA_NoCareRefresh, TRUE,
                            WA_IDCMP, IDCMP_VANILLAKEY | IDCMP_CLOSEWINDOW,
                            TAG_DONE);
        
        UnlockPubScreen(NULL, pubscreen);
    }
                   
    initgl();
//    finished = TRUE;
    while(!finished)
    {
        DOTRACE(bug("\n[GLSimpeRend] In Render Loop...\n");)

#if defined(SHOW_FPS)
        GetSysTime(&tv);
        currmicrosecs = tv.tv_secs * 1000000 + tv.tv_micro;

        DOTRACE(bug("\n[GLSimpeRend] currmicrosecs = %ld\n", currmicrosecs);)
        DOTRACE(bug("\n[GLSimpeRend] fpsmicrosecs = %ld\n", fpsmicrosecs);)

        if (currmicrosecs - fpsmicrosecs > 1000000)
        {
            /* FPS counting is naive! */
            sprintf(title, "GLSimpleRendering, FPS: %d", (int)framecnt);
            fpsmicrosecs = currmicrosecs;
            framecnt = 0;

            DOTRACE(bug("\n[GLSimpeRend] updating title (%s)\n", title);)
            SetWindowTitles(win, title, (UBYTE *)~0L);
        }

        angle_inc = ((double)(currmicrosecs - lastmicrosecs) / 1000000.0) * DEGREES_PER_SECOND;
        lastmicrosecs = currmicrosecs;
        
        framecnt++; 
#endif
        render();
        HandleIntuiMessages();
//        exitcounter++;
//        Delay(10);
//        if (exitcounter > 0) finished = TRUE;
    }
    
    deinitgl();
    
    deinit_timerbase();
      
    CloseWindow(win);
    
    if (customscreen) CloseScreen(customscreen);
    
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    
    return 0;
}
