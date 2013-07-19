/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Filesystem that uses console device for input/output.
    Lang: english
*/

#define CUR_UP    'A'
#define CUR_DOWN  'B'
#define CUR_RIGHT 'C'
#define CUR_LEFT  'D'

#define INP_DONE                0
#define INP_CURSORLEFT          1
#define INP_CURSORRIGHT         2
#define INP_CURSORUP            3
#define INP_CURSORDOWN          4
#define INP_SHIFT_CURSORLEFT    5
#define INP_SHIFT_CURSORRIGHT   6
#define INP_SHIFT_CURSORUP      7
#define INP_SHIFT_CURSORDOWN    8
#define INP_BACKSPACE           9
#define INP_SHIFT_BACKSPACE     10
#define INP_DELETE              11
#define INP_SHIFT_DELETE        12
#define INP_RETURN              13
#define INP_LINEFEED            14
#define INP_TAB                 15
#define INP_SHIFT_TAB           16
#define INP_F1                  17
#define INP_F2                  18
#define INP_F3                  19
#define INP_F4                  20
#define INP_F5                  21
#define INP_F6                  22
#define INP_F7                  23
#define INP_F8                  24
#define INP_F9                  25
#define INP_F10                 26
#define INP_F11                 27
#define INP_F12                 28
#define INP_SHIFT_F1            29
#define INP_SHIFT_F2            30
#define INP_SHIFT_F3            31
#define INP_SHIFT_F4            32
#define INP_SHIFT_F5            33
#define INP_SHIFT_F6            34
#define INP_SHIFT_F7            35
#define INP_SHIFT_F8            36
#define INP_SHIFT_F9            37
#define INP_SHIFT_F10           38
#define INP_SHIFT_F11           39
#define INP_SHIFT_F12           40
#define INP_HELP                41
#define INP_CONTROL_X           42
#define INP_INSERT              43
#define INP_PAGEUP              44
#define INP_PAGEDOWN            45
#define INP_PAUSE               46
#define INP_BREAK               47
#define INP_HOME                48
#define INP_END                 49
#define INP_SHIFT_INSERT        50
#define INP_SHIFT_PAGEUP        51
#define INP_SHIFT_PAGEDOWN      52
#define INP_SHIFT_HOME          53
#define INP_SHIFT_END           54
#define INP_CTRL_C              55
#define INP_CTRL_D              56
#define INP_CTRL_E              57
#define INP_CTRL_F              58
#define INP_EOF                 59
#define INP_PASTE               60

#define INP_UNKNOWN             99
#define INP_STRING              100
#define INP_ECHO_STRING         101

BOOL parse_filename(struct filehandle *fh, char *filename, struct NewWindow *nw);

void do_write(struct filehandle *fh, APTR data, ULONG length);
void do_movecursor(struct filehandle *fh, UBYTE direction, UBYTE howmuch);
void do_cursorvisible(struct filehandle *fh, BOOL on);
void do_deletechar(struct filehandle *fh);
void do_eraseinline(struct filehandle *fh);
void do_eraseindisplay(struct filehandle *fh);

WORD scan_input(struct filehandle *fh, UBYTE *);

void con_read(struct filehandle *fh, struct DosPacket *dp);
void answer_read_request(struct filehandle *fh, struct DosPacket *dp, ULONG dp_Arg3);
BOOL answer_write_request(struct filehandle *fh, struct DosPacket *dp);
void HandlePendingReads(struct filehandle *fh);

void add_to_history(struct filehandle *fh);
void history_walk(struct filehandle *fh, WORD inp);

BOOL process_input(struct filehandle *fh);

void replypkt(struct DosPacket *dp, SIPTR res1);
void replypkt2(struct DosPacket *dp, SIPTR res1, SIPTR res2);
