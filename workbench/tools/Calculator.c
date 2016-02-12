/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
    Command line options:

    1. PUBSCREEN <name>: the name of the public screen to open the window on
    2. TAPE <filename>: the name of a file to record the user interactions
       into
*/
#include <exec/types.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *version = "$VER: Calculator 1.4 (08.08.2013) © AROS Dev Team";

#define ARG_TEMPLATE "PUBSCREEN,TAPE/K"
enum {ARG_PUBSCREEN,ARG_TAPE,NUM_ARGS};

/* The pattern of the tape name in case we log to a RAW: window */
#define RAW_TAPE_NAME "RAW:%d/%d/%d/%d/Calculator Tape/INACTIVE/SCREEN%s"

enum
{
    BTYPE_0 = 0,
    BTYPE_1,
    BTYPE_2,
    BTYPE_3,
    BTYPE_4,
    BTYPE_5,
    BTYPE_6,
    BTYPE_7,
    BTYPE_8,
    BTYPE_9,
    BTYPE_COMMA,
    BTYPE_BS,
    BTYPE_CA,
    BTYPE_CE,
    BTYPE_MUL,
    BTYPE_DIV,
    BTYPE_SUB,
    BTYPE_ADD,
    BTYPE_SIGN,
    BTYPE_EQU    
};

#define NUM_BUTTONS 20
#define DECIMAL_BUTTON_INDEX 16

struct CalcButtonInfo
{
    const char *label;
    ULONG btype;
    char shortcut;
};

struct CalcButtonInfo BUTTONS[] =
{
    {"7", BTYPE_7, '7'}, {"8", BTYPE_8, '8'}, {"9", BTYPE_9, '9'}, {"CA", BTYPE_CA, 'A'}, {"CE", BTYPE_CE, 'E'},
    {"4", BTYPE_4, '4'}, {"5", BTYPE_5, '5'}, {"6", BTYPE_6, '6'}, {"*", BTYPE_MUL, '*'}, {":", BTYPE_DIV, ':'},
    {"1", BTYPE_1, '1'}, {"2", BTYPE_2, '2'}, {"3", BTYPE_3, '3'}, {"+", BTYPE_ADD, '+'}, {"-", BTYPE_SUB, '-'},
    {"0", BTYPE_0, '0'}, {".", BTYPE_COMMA, '.'}, {"<<", BTYPE_BS, 8}, {"+/-", BTYPE_SIGN, 's'}, {"=", BTYPE_EQU, '='}
};

/*
 * Most of the application state is local or in BOOPSI objects.
 * The only global state is to communicate the command line arguments
 */
static char pubscrname[256];
static char tapename[256];
static BOOL use_tape;

/**********************************************************************
 Tape BOOPSI class
 **********************************************************************/

#define TAPEA_FILEHANDLE    (TAG_USER + 20)
#define TAPEM_NEWLINE       (TAG_USER + 21)
#define TAPEM_PRINT_LVAL    (TAG_USER + 22)
#define TAPEM_PRINT_RVAL    (TAG_USER + 23)
#define TAPEM_PRINT_RESULT  (TAG_USER + 24)

struct TapeData
{
    BPTR tapefh;
};

struct MUIMP_PrintLval
{
    STACKED ULONG MethodID;
    STACKED const char *str;
};

struct MUIMP_PrintRval
{
    STACKED ULONG MethodID;
    STACKED char operator;
    STACKED const char *str;
};

struct MUIMP_PrintResult
{
    STACKED ULONG MethodID;
    STACKED const char *str;
};

IPTR mNewTape(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tagListState = msg->ops_AttrList, *tag;
    Object *instance = (Object *) DoSuperMethodA(cl, obj, (APTR) msg);
    struct TapeData *data = INST_DATA(cl, instance);
    data->tapefh = BNULL;

    while ((tag = (struct TagItem *) NextTagItem(&tagListState)))
    {
        switch (tag->ti_Tag)
        {
            case TAPEA_FILEHANDLE:
                data->tapefh = (BPTR) tag->ti_Data;
                break;
            default:
                break;
        }
    }
    return (IPTR) instance;
}

IPTR mDisposeTape(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TapeData *data = INST_DATA(cl, obj);
    if (data->tapefh) Close(data->tapefh);
    return DoSuperMethodA(cl, obj, (APTR) msg);
}

IPTR mTapeNewline(struct IClass *cl, Object *obj, APTR msg)
{
    struct TapeData *data = INST_DATA(cl, obj);
    if (data->tapefh) FPutC(data->tapefh, '\n');
    return (IPTR) obj;
}

IPTR mTapePrintLval(struct IClass *cl, Object *obj, struct MUIMP_PrintLval *msg)
{
    struct TapeData *data = INST_DATA(cl, obj);
    if (data->tapefh)
    {
        FPutC(data->tapefh, '\t');
        FPuts(data->tapefh, msg->str);
        FPutC(data->tapefh, '\n');
        Flush(data->tapefh);
    }
    return (IPTR) obj;
}

IPTR mTapePrintRval(struct IClass *cl, Object *obj, struct MUIMP_PrintRval *msg)
{
    struct TapeData *data = INST_DATA(cl, obj);
    if (data->tapefh)
    {
        FPutC(data->tapefh, msg->operator);
        FPutC(data->tapefh, '\t');
        FPuts(data->tapefh, msg->str);
        FPutC(data->tapefh, '\n');
        Flush(data->tapefh);
  }
  return (IPTR) obj;
}

IPTR mTapePrintResult(struct IClass *cl, Object *obj, struct MUIMP_PrintResult *msg)
{
    struct TapeData *data = INST_DATA(cl, obj);
    if (data->tapefh)
    {
        FPuts(data->tapefh, "=\t");
        FPuts(data->tapefh, msg->str);
        FPutC(data->tapefh, '\n');
        Flush(data->tapefh);
    }
    return (IPTR) obj;
}

ULONG mSet(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tagListState = msg->ops_AttrList, *tag;
    struct TapeData *data = INST_DATA(cl, obj);

    while ((tag = (struct TagItem *) NextTagItem(&tagListState)))
    {
        switch (tag->ti_Tag)
        {
            case TAPEA_FILEHANDLE:
                data->tapefh = (BPTR) tag->ti_Data;
                break;
            default:
                break;
        }
    }
    return (IPTR) obj;
}

BOOPSI_DISPATCHER(IPTR, TapeDispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:           return mNewTape(cl, obj, (APTR) msg);
        case OM_DISPOSE:       return mDisposeTape(cl, obj, (APTR) msg);
        case OM_SET:           return mSet(cl, obj, (APTR) msg);

        case TAPEM_NEWLINE:    return mTapeNewline(cl, obj, (APTR) msg);
        case TAPEM_PRINT_LVAL: return mTapePrintLval(cl, obj, (struct MUIMP_PrintLval *) msg);
        case TAPEM_PRINT_RVAL: return mTapePrintRval(cl, obj, (struct MUIMP_PrintRval *) msg);
        case TAPEM_PRINT_RESULT: return mTapePrintResult(cl, obj, (struct MUIMP_PrintResult *) msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

static Class *make_tape_class(void)
{
    Class *cl;
    cl = MakeClass(NULL, ROOTCLASS, NULL, sizeof(struct TapeData), 0);
    if (cl) cl->cl_Dispatcher.h_Entry = TapeDispatcher;
    return cl;
}

/**********************************************************************
 Calculator BOOPSI class
 **********************************************************************/

#define MAX_DIGITS 13
#define OM_ADD_KEY           (TAG_USER + 30)
#define CALCA_DISPLAY        (TAG_USER + 31)
#define CALCA_TAPE           (TAG_USER + 32)
#define CALCA_DECIMAL_POINT  (TAG_USER + 33)

#define EDIT_BUFFER_SIZE MAX_DIGITS + 2  /* space for '-' sign and terminator byte */
const char *INITIAL_DISPLAY = "\033r0";
enum { STATE_LEFTVAL, STATE_OP, STATE_RIGHTVAL, STATE_EQU };

struct CalculatorData
{
    char edit_buffer[EDIT_BUFFER_SIZE];
    int num_digits;
    int state;
    char decimal_point;
  
    double lvalue, rvalue;
    int op;
    Object *display, *tape;
};

struct MUIMP_CalcKey
{
    STACKED ULONG MethodID;
    STACKED ULONG btype;
};

static char op2char(int op)
{
    switch (op) {
        case BTYPE_MUL: return '*';
        case BTYPE_DIV: return '*';
        case BTYPE_SUB: return '-';
        case BTYPE_ADD: return '+';
        default: return '?';
    }
}

static void clear_edit_buffer(struct CalculatorData *data)
{
    memset(data->edit_buffer, 0, EDIT_BUFFER_SIZE);
    data->num_digits = 0;
}

IPTR mNewCalc(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tagListState = msg->ops_AttrList, *tag;
    Object *instance = (Object *) DoSuperMethodA(cl, obj, (APTR) msg);
    struct CalculatorData *data = INST_DATA(cl, instance);
    clear_edit_buffer(data);
    data->display = NULL;
    data->tape = NULL;
    data->state = STATE_LEFTVAL;
    data->decimal_point = '.';

    while ((tag = (struct TagItem *) NextTagItem(&tagListState)))
    {
        switch (tag->ti_Tag)
        {
            case CALCA_DISPLAY:
                data->display = (Object *) tag->ti_Data;
                break;
            case CALCA_TAPE:
                data->tape = (Object *) tag->ti_Data;
                break;
            case CALCA_DECIMAL_POINT:
                data->decimal_point = (char) tag->ti_Data;
                break;
            default:
                break;
        }
    }
    return (IPTR) instance;
}

IPTR mDisposeCalc(struct IClass *cl, Object *obj, struct opSet *msg)
{
    return DoSuperMethodA(cl, obj, (APTR) msg);
}

static BOOL is_operator(int btype)
{
    return btype >= BTYPE_MUL && btype <= BTYPE_ADD;
}

static BOOL can_insert_comma(struct CalculatorData *data)
{
    if (data->num_digits == 0) return TRUE;
    else
    {
        int i;
        for (i = 0; i < data->num_digits; i++)
        {
            if (data->edit_buffer[i] == '.') return FALSE;
        }
        return TRUE;
    }
}

static double eval_result(double lvalue, double rvalue, int op)
{
    switch (op)
    {
        case BTYPE_MUL: return lvalue * rvalue;
        case BTYPE_DIV: return lvalue / rvalue;
        case BTYPE_SUB: return lvalue - rvalue;
        case BTYPE_ADD: return lvalue + rvalue;
        default: return 0;
    }
}


static void localize_buffer(char *buffer, char decimal_point, int from, int to)
{
    int i;
    for (i = from; i < to; i ++)
    {
        if (buffer[i] == '.')
        {
            buffer[i] = decimal_point;
            break;
        }
    }
}
/*
 * Rendering is centralized in this function, by copying
 * the edit buffer into and manipulating the display buffer.
 * MUI/Zune's text field right-justifies text through the inclusion
 * of the "Esc-r" control sequence.
 */
static void display_state(struct CalculatorData *data)
{
    /* add 2 extra chars: Esc+'r' */
    static char display_buffer[EDIT_BUFFER_SIZE + 2];

    if (data->num_digits == 0)
    {
        SetAttrs(data->display, MUIA_Text_Contents, INITIAL_DISPLAY, TAG_DONE);
    }
    else
    {
        memset(display_buffer, 0, EDIT_BUFFER_SIZE + 2);
        display_buffer[0] = '\033';
        display_buffer[1] = 'r';
        memcpy(display_buffer + 2, data->edit_buffer, data->num_digits);
        localize_buffer(display_buffer, data->decimal_point, 2, MAX_DIGITS + 3);
        SetAttrs(data->display, MUIA_Text_Contents, display_buffer, TAG_DONE);
    }
}

static const char *localize_display(struct CalculatorData *data)
{
    static char buffer[EDIT_BUFFER_SIZE];
    memset(buffer, 0, MAX_DIGITS + 1);
    memcpy(buffer, data->edit_buffer, data->num_digits);
    localize_buffer(buffer, data->decimal_point, 0, MAX_DIGITS + 1);
    return buffer;
}

static void toggle_sign(struct CalculatorData *data)
{
    BOOL sign_found = FALSE;
    int i;
    if (data->state == STATE_LEFTVAL)       data->lvalue = -data->lvalue;
    else if (data->state == STATE_RIGHTVAL) data->rvalue = -data->rvalue;

    for (i = 0; i < EDIT_BUFFER_SIZE; i++)
    {
        if (data->edit_buffer[i] == '-')
        {
            sign_found = TRUE;
            break;
        }
    }

    if (sign_found)
    {
        /* eliminate the sign by shifting to the left */
        data->num_digits--;
        memmove(data->edit_buffer, data->edit_buffer + 1, data->num_digits);
        data->edit_buffer[data->num_digits] = 0;
    }
    else
    {
        /* add sign by shifting to the right and inserting - */
        memmove(data->edit_buffer + 1, data->edit_buffer, data->num_digits);
        data->num_digits++;
        data->edit_buffer[0] = '-';
        data->edit_buffer[data->num_digits] = 0;
    }
    display_state(data);
}

/*
 * This method implements the main logic of the calculator by performing transitions
 * of a state machine.
 */
IPTR mAddCalcKey(struct IClass *cl, Object *obj, struct MUIMP_CalcKey *msg)
{
    struct CalculatorData *data = INST_DATA(cl, obj);
    if (msg->btype <= BTYPE_9 && data->num_digits < MAX_DIGITS)
    {
        if (data->state == STATE_OP)
        {
            data->state = STATE_RIGHTVAL;
            clear_edit_buffer(data);
        }
        if (data->state == STATE_EQU)
        {
            data->state = STATE_LEFTVAL;
            clear_edit_buffer(data);
        }
        char digit = '0' + msg->btype;
        data->edit_buffer[data->num_digits++] = digit;
        display_state(data);

    }
    else if (msg->btype == BTYPE_COMMA && can_insert_comma(data))
    {
        data->edit_buffer[data->num_digits++] = '.';
        display_state(data);
    }
    else if (is_operator(msg->btype))
    {
        if (data->state == STATE_LEFTVAL || data->state == STATE_EQU)
        {
            data->lvalue = strtod(data->edit_buffer, NULL);
            data->state = STATE_OP;
            data->op = msg->btype;
            if (data->tape) DoMethod(data->tape, TAPEM_PRINT_LVAL, localize_display(data));
        }
    }
    else if (msg->btype == BTYPE_EQU)
    {
        if (data->tape) DoMethod(data->tape, TAPEM_PRINT_RVAL, op2char(data->op), localize_display(data));

        data->rvalue = strtod(data->edit_buffer, NULL);
        data->state = STATE_EQU;
        data->lvalue = eval_result(data->lvalue, data->rvalue, data->op);
        snprintf(data->edit_buffer, MAX_DIGITS, "%f", data->lvalue);
        /* note that there is no strnlen() in AROS !!! */
        data->num_digits = strlen(data->edit_buffer);

        display_state(data);
        if (data->tape) DoMethod(data->tape, TAPEM_PRINT_RESULT, localize_display(data));
    }
    else if (msg->btype == BTYPE_CA)
    {
        data->lvalue = 0;
        data->rvalue = 0;
        data->op = BTYPE_ADD;
        data->state = STATE_LEFTVAL;
        clear_edit_buffer(data);

        display_state(data);
        if (data->tape) DoMethod(data->tape, TAPEM_NEWLINE);
    }
    else if (msg->btype == BTYPE_CE &&
             (data->state == STATE_LEFTVAL || data->state == STATE_RIGHTVAL))
    {
        clear_edit_buffer(data);
        display_state(data);      

    }
    else if (msg->btype == BTYPE_SIGN && data->state != STATE_OP)
    {
        toggle_sign(data);
    }
    else if (msg->btype == BTYPE_BS &&
             (data->state == STATE_LEFTVAL || data->state == STATE_RIGHTVAL) &&
             data->num_digits > 0)
    {
        data->edit_buffer[--data->num_digits] = 0;
        display_state(data);
    }
    return (IPTR) obj;
}

BOOPSI_DISPATCHER(IPTR, CalculatorDispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:     return mNewCalc(cl, obj, (APTR) msg);
        case OM_DISPOSE: return mDisposeCalc(cl, obj, (APTR) msg);
        case OM_ADD_KEY: return (IPTR) mAddCalcKey(cl, obj, (struct MUIMP_CalcKey *) msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

static Class *make_calculator_class(void)
{
    Class *cl;
    cl = MakeClass(NULL, ROOTCLASS, NULL, sizeof(struct CalculatorData), 0);
    if (cl) cl->cl_Dispatcher.h_Entry = CalculatorDispatcher;
    return cl;
}

/**********************************************************************
 Main program
 **********************************************************************/

static void cleanup(char *msg)
{
    WORD rc;
    if (msg)
    {
        fprintf(stderr, "Calculator: %s\n", msg);
        rc = RETURN_WARN;
    }
    else
    {
        rc = RETURN_OK;
    }
    exit(rc);
}

static void dos_error(void)
{
    static char tempstring[256];
    Fault(IoErr(), 0, tempstring, 255);
    cleanup(tempstring);
}

static char retrieve_decimal_point(void)
{
    struct Locale *loc;    
    char result = '.';

    if ((loc = OpenLocale(0)))
    {
    	  result = loc->loc_DecimalPoint[0];
    	  CloseLocale(loc);
    }
    return result;
}

static void get_arguments(void)
{
    struct RDArgs *rdargs;
    IPTR args[NUM_ARGS];
    int i;

    for (i = 0; i < NUM_ARGS; i++) args[i] = (IPTR) NULL;

    if (!(rdargs = ReadArgs(ARG_TEMPLATE, (IPTR *) args,0))) dos_error();
    
    if (args[ARG_PUBSCREEN]) {
        strncpy(pubscrname, (const char *) args[ARG_PUBSCREEN], 255);
    }
    
    if (args[ARG_TAPE])
    {
        use_tape = TRUE;
        strncpy(tapename, (const char *) args[ARG_TAPE], 255);
    }
    if (rdargs) FreeArgs(rdargs);
}

static void open_raw_tape_if_needed(Object *window, Object *obj_tape)
{
    if (use_tape && !strlen(tapename))
    {
        int x, y, w, h;
        struct Window *win;
        BPTR tapefh;

        GetAttr(MUIA_Window_Window, window, (IPTR *) &win);
        w = win->Width * 5 / 4;
        h = win->Height;
        x = win->LeftEdge;
        y = win->TopEdge;

        if (x > (win->WScreen->Width - (x + w))) x -= w;
        else                                     x += win->WScreen->Width;

        snprintf(tapename, 255, RAW_TAPE_NAME, x, y, w, h, pubscrname);
        tapefh = Open(tapename, MODE_NEWFILE);
        SetAttrs(obj_tape, TAPEA_FILEHANDLE, tapefh);
    }
}

int main(void)
{
    Class *cl_calc = NULL, *cl_tape = NULL;
    Object *app = NULL, *window = NULL, *display = NULL, *button[NUM_BUTTONS],
      *obj_calc, *obj_tape;
    char decimal_point, decimal_label[2];
    struct Screen *pub_screen = NULL;
    BPTR tapefh = BNULL;
    int i;

    get_arguments();
    decimal_point = retrieve_decimal_point();
    snprintf(decimal_label, 2, "%c", decimal_point);

    display = TextObject,
        StringFrame,
        MUIA_Text_Contents, INITIAL_DISPLAY,
        MUIA_ShortHelp, "Display",
    End;

    cl_tape = make_tape_class();

    if (use_tape && strlen(tapename)) {
      tapefh = Open(tapename, MODE_NEWFILE);
    }

    if (tapefh) {
      obj_tape = NewObject(cl_tape, NULL, TAPEA_FILEHANDLE, tapefh, TAG_DONE);
    }
    else
    {
      obj_tape = NewObject(cl_tape, NULL, TAG_DONE);
    }

    cl_calc = make_calculator_class();
    obj_calc = NewObject(cl_calc, NULL,
                         CALCA_DISPLAY, display,
                         CALCA_TAPE, obj_tape,
                         CALCA_DECIMAL_POINT, decimal_point,
                         TAG_DONE);

    for (i = 0; i < NUM_BUTTONS; i++)
    {
        button[i] = (i != DECIMAL_BUTTON_INDEX) ?
          SimpleButton(BUTTONS[i].label) : SimpleButton(decimal_label);
        if (BUTTONS[i].shortcut)
        {
            SetAttrs(button[i], MUIA_ControlChar, BUTTONS[i].shortcut, TAG_DONE);
        }
    }

    if (strlen(pubscrname))
    {
        pub_screen = LockPubScreen((CONST_STRPTR) pubscrname);
        if (!pub_screen)
        {
            printf("Can't lock public screen '%s' -> fallback to Wanderer!\n", pubscrname);
            memset(pubscrname, 0, 256);
            strcpy(pubscrname, "Workbench");
            pub_screen = LockPubScreen((CONST_STRPTR) pubscrname);
        }
    }

    app = ApplicationObject,
          MUIA_Application_Title, "Calculator",
          MUIA_Application_Version, "1.4",
          MUIA_Application_Copyright, "©2007-2013, AROS Dev Team",
          MUIA_Application_Author, "AROS Team",
          MUIA_Application_Description, "Simple desktop calculator",
          MUIA_Application_Base, "calculator",
          SubWindow, window = WindowObject,
            MUIA_Window_Title, "Calculator",
            MUIA_Window_ID, MAKE_ID('C', 'A', 'L', 'C'),
            MUIA_Window_AppWindow, TRUE,
            MUIA_Window_Screen, pub_screen,
            WindowContents, VGroup,
              Child, display,

              Child, HGroup, GroupSpacing(5), MUIA_Group_SameWidth, TRUE,
                Child, button[0],
                Child, button[1],
                Child, button[2],
                Child, button[3],
                Child, button[4],
              End,

              Child, HGroup, GroupSpacing(5), MUIA_Group_SameWidth, TRUE,
                Child, button[5],
                Child, button[6],
                Child, button[7],
                Child, button[8],
                Child, button[9],
              End,

              Child, HGroup, GroupSpacing(5), MUIA_Group_SameWidth, TRUE,
                Child, button[10],
                Child, button[11],
                Child, button[12],
                Child, button[13],
                Child, button[14],
              End,

              Child, HGroup, GroupSpacing(5), MUIA_Group_SameWidth, TRUE,
                Child, button[15],
                Child, button[16],
                Child, button[17],
                Child, button[18],
                Child, button[19],
              End,

            End,
          End,
        End;

    DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             app, 2, MUIM_Application_ReturnID,
             MUIV_Application_ReturnID_Quit);

    for (i = 0; i < NUM_BUTTONS; i++)
    {
        DoMethod(button[i], MUIM_Notify, MUIA_Pressed, FALSE,
                 obj_calc, 2, OM_ADD_KEY, BUTTONS[i].btype);
    }

    SetAttrs(window, MUIA_Window_Open, TRUE, TAG_DONE);
    open_raw_tape_if_needed(window, obj_tape);

    if (pub_screen) UnlockPubScreen(0, pub_screen);
    {
        BOOL running = TRUE;
        ULONG sigs = 0, id;

        while (running) {
            id = DoMethod(app, MUIM_Application_NewInput, (IPTR) &sigs);

            switch(id)
            {
                case MUIV_Application_ReturnID_Quit:
                    running = FALSE;
                    break;
                default:
                    break;
            }
            if (running && sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                if (sigs & SIGBREAKF_CTRL_C) break;
            }
        }
    }
    set((APTR) window, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);

    DisposeObject(obj_calc);
    DisposeObject(obj_tape);
    FreeClass(cl_calc);
    FreeClass(cl_tape);

    cleanup(NULL);
    return 0;
}
