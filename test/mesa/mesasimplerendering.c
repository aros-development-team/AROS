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

#define GL_GLEXT_PROTOTYPES
#include <GL/arosmesa.h>

#include <stdio.h>

AROSMesaContext     glcont=NULL;
double              angle = 0.0;
double              angle_inc = 0.0;
BOOL                finished = FALSE;
struct Window *     win = NULL;
struct Device *     TimerBase = NULL;
struct timerequest  timereq;
struct MsgPort      timeport;
struct Library *    CyberGfxBase = NULL;
BOOL                fullscreen = FALSE;
    
GLuint              fragmentShader = 0;
GLuint              vertexShader = 0;
GLuint              shaderProgram = 0;
GLint               angleLocation = 0;


const GLchar * fragmentShaderSource =
"uniform float angle;"
"void main()"
"{"
"   vec4 v = vec4(gl_Color);"
"   float intensity = abs(1.0f - (mod(angle, 1440.0f) / 720.0f));"
"   v.b = v.b * intensity;"
"   v.g = v.g * (1.0f - intensity);"
"	gl_FragColor = v;"
"}";

const GLchar * vertexShaderSource =
"void main()"
"{	"
"   gl_FrontColor = gl_Color;"
"	gl_Position = ftransform();"
"}";


#define RAND_COL 1.0f
#define DEGREES_PER_SECOND 180.0
#define USE_PERSPECTIVE 1

void prepare_shader_program()
{
#define BUFFER_LEN 2048
    char buffer[BUFFER_LEN] = {0};
    int len;

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

#undef BUFFER_LEN    
}

void cleanup_shader_program()
{
    glUseProgram(0);
    glDetachShader(shaderProgram, fragmentShader);
    glDetachShader(shaderProgram, vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteProgram(shaderProgram);
}

void render_face()
{
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

}

void render_cube()
{
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
}

void render_triangle()
{
    glBegin(GL_TRIANGLES);
        glColor4f(1.0, 0.0, 0.0, 1.0);
        glVertex3f(-0.25, -0.25, 0.0);
        glColor4f(0.0, 1.0, 0.0, 1.0);
        glVertex3f(-0.25,  0.25, 0.0);
        glColor4f(0.0, 0.0, 1.0, 1.0);
        glVertex3f( 0.25,  0.25, 0.0);
    glEnd();
}

void render()
{
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

    AROSMesaSwapBuffers(glcont);
}    

#define VISIBLE_WIDTH 300
#define VISIBLE_HEIGHT 300

void initmesa()
{
    struct TagItem attributes [ 14 ]; /* 14 should be more than enough :) */
    int i = 0;
    GLfloat h = 0.0f;
    
    attributes[i].ti_Tag = AMA_Window;      attributes[i++].ti_Data = (IPTR)win;
    attributes[i].ti_Tag = AMA_Left;        attributes[i++].ti_Data = win->BorderLeft;
    attributes[i].ti_Tag = AMA_Top;         attributes[i++].ti_Data = win->BorderTop;
    attributes[i].ti_Tag = AMA_Bottom;      attributes[i++].ti_Data = win->BorderBottom;
    attributes[i].ti_Tag = AMA_Right;       attributes[i++].ti_Data = win->BorderRight;

    // double buffer ?
    attributes[i].ti_Tag = AMA_DoubleBuf;   attributes[i++].ti_Data = GL_TRUE;

    // RGB(A) Mode ?
    attributes[i].ti_Tag = AMA_RGBMode;     attributes[i++].ti_Data = GL_TRUE;
    
    /* Stencil/Accum */
    attributes[i].ti_Tag = AMA_NoStencil;   attributes[i++].ti_Data = GL_TRUE;
    attributes[i].ti_Tag = AMA_NoAccum;     attributes[i++].ti_Data = GL_TRUE;
    
    // done...
    attributes[i].ti_Tag    = TAG_DONE;

    glcont = AROSMesaCreateContext(attributes);
    if (glcont)
    {
        AROSMesaMakeCurrent(glcont);
        h = (GLfloat)VISIBLE_HEIGHT / (GLfloat)VISIBLE_WIDTH ;

        glViewport(0, 0, (GLint) VISIBLE_WIDTH, (GLint) VISIBLE_HEIGHT);
#if USE_PERSPECTIVE == 1
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(-1.0, 1.0, -h, h, 5.0, 200.0);
        glMatrixMode(GL_MODELVIEW);
#endif
        prepare_shader_program();
        glUseProgram(shaderProgram);
        angleLocation = glGetUniformLocation(shaderProgram, "angle");
    }
    else
        finished = TRUE; /* Failure. Stop */
}

void deinitmesa()
{
    if (glcont) 
    {
        cleanup_shader_program();
        AROSMesaDestroyContext(glcont);
    }
}

static int init_timerbase()
{
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
}


static void deinit_timerbase()
{
    if (TimerBase != NULL)
        CloseDevice((struct IORequest *)&timereq);
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
#define NUM_ARGS        1

STATIC CONST_STRPTR   TEMPLATE=(CONST_STRPTR) "FULLSCREEN/S";
static struct RDArgs  *myargs;
static IPTR           args[NUM_ARGS];

void get_arguments(void)
{
    if((myargs = ReadArgs(TEMPLATE, args, NULL)))
    {
        fullscreen = (BOOL)args[ARG_FULLSCREEN];
        FreeArgs(myargs);
    }
}

/*
** Open a simple window using OpenWindowTagList()
*/
int main(void)
{
    ULONG fps = 0;
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
    
    GetSysTime(&tv);
    lastmicrosecs = tv.tv_secs * 1000000 + tv.tv_micro;
    fpsmicrosecs = lastmicrosecs;

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
                            WA_Title, (IPTR)"MesaSimpleRendering",
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
                   
    initmesa();
//    finished = TRUE;
    while(!finished)
    {
        GetSysTime(&tv);
        currmicrosecs = tv.tv_secs * 1000000 + tv.tv_micro;
        
        if (currmicrosecs - fpsmicrosecs > 1000000)
        {
            /* FPS counting is naive! */
            fpsmicrosecs += 1000000;
            sprintf(title, "MesaSimpleRendering, FPS: %d", (int)fps);
            SetWindowTitles(win, title, (UBYTE *)~0L);
            fps = 0;
        }
        
        angle_inc = ((double)(currmicrosecs - lastmicrosecs) / 1000000.0) * DEGREES_PER_SECOND;
        lastmicrosecs = currmicrosecs;
        
        fps++; 
        render();
        HandleIntuiMessages();
//        exitcounter++;
//        Delay(10);
//        if (exitcounter > 0) finished = TRUE;
    }
    
    deinitmesa();
    
    deinit_timerbase();
      
    CloseWindow(win);
    
    if (customscreen) CloseScreen(customscreen);
    
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    
    return 0;
}
