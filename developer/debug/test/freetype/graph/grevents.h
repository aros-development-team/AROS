#ifndef GREVENTS_H_
#define GREVENTS_H_


#define gr_event_none   0
#define gr_event_wait   1
#define gr_event_poll   2
#define gr_event_flush  3

#define gr_mouse_down  0x04
#define gr_mouse_move  0x08
#define gr_mouse_up    0x10
#define gr_mouse_drag  0x20

#define gr_key_down  0x40
#define gr_key_up    0x80


#define gr_event_mouse  0x3C
#define gr_event_key    0xC0

#define gr_event_type  ( gr_event_mouse | gr_event_key )


  typedef enum grKey_
  {
    grKeyNone = 0,

    grKeySpace = ' ',
    grKey0     = '0',
    grKey1     = '1',
    grKey2     = '2',
    grKey3     = '3',
    grKey4     = '4',
    grKey5     = '5',
    grKey6     = '6',
    grKey7     = '7',
    grKey8     = '8',
    grKey9     = '9',

    grKeyPlus       = '+',
    grKeyLess       = '-',
    grKeyEqual      = '=',
    grKeyMult       = '*',
    grKeyDollar     = '$',
    grKeySmaller    = '<',
    grKeyGreater    = '>',
    grKeyQuestion   = '?',
    grKeyComma      = ',',
    grKeyDot        = '.',
    grKeySemiColon  = ';',
    grKeyColon      = ':',
    grKeyDiv        = '/',
    grKeyExclam     = '!',
    grKeyPercent    = '%',
    grKeyLeftParen  = '(',
    grKeyRightParen = ')',
    grKeyAt         = '@',

    grKey_A = 'A',
    grKey_B = 'B',
    grKey_C = 'C',
    grKey_D = 'D',
    grKey_E = 'E',
    grKey_F = 'F',
    grKey_G = 'G',
    grKey_H = 'H',
    grKey_I = 'I',
    grKey_J = 'J',
    grKey_K = 'K',
    grKey_L = 'L',
    grKey_M = 'M',
    grKey_N = 'N',
    grKey_O = 'O',
    grKey_P = 'P',
    grKey_Q = 'Q',
    grKey_R = 'R',
    grKey_S = 'S',
    grKey_T = 'T',
    grKey_U = 'U',
    grKey_V = 'V',
    grKey_W = 'W',
    grKey_X = 'X',
    grKey_Y = 'Y',
    grKey_Z = 'Z',

    grKeyLeftB      = '[',
    grKeyBackSlash  = '\\',
    grKeyRightB     = ']',
    grKeyCircumflex = '^',
    grKeyUnder      = '_',
    grKeyBackTick   = '`',

    grKey_a = 'a',
    grKey_b = 'b',
    grKey_c = 'c',
    grKey_d = 'd',
    grKey_e = 'e',
    grKey_f = 'f',
    grKey_g = 'g',
    grKey_h = 'h',
    grKey_i = 'i',
    grKey_j = 'j',
    grKey_k = 'k',
    grKey_l = 'l',
    grKey_m = 'm',
    grKey_n = 'n',
    grKey_o = 'o',
    grKey_p = 'p',
    grKey_q = 'q',
    grKey_r = 'r',
    grKey_s = 's',
    grKey_t = 't',
    grKey_u = 'u',
    grKey_v = 'v',
    grKey_w = 'w',
    grKey_x = 'x',
    grKey_y = 'y',
    grKey_z = 'z',

    grKeyBackSpace = 0x100,
    grKeyTab,
    grKeyReturn,
    grKeyEsc,

    grKeyIns,
    grKeyDel,
    grKeyHome,
    grKeyEnd,
    grKeyPageUp,
    grKeyPageDown,

    grKeyF1,
    grKeyF2,
    grKeyF3,
    grKeyF4,
    grKeyF5,
    grKeyF6,
    grKeyF7,
    grKeyF8,
    grKeyF9,
    grKeyF10,
    grKeyF11,
    grKeyF12,

    grKeyLeft,
    grKeyRight,
    grKeyUp,
    grKeyDown,

    grKeyForceShort = 0x7FFF, /* this forces the grKey to be stored */
                              /* on at least one short              */
    grKeyMax

  } grKey;


#define grKEY( c )  ( (grKey)( c ) )
  /* masks - to be used as enums they would have to be included */
  /* in the grKey enum                                          */
#define grKeyAlt    ( (grKey)0x8000 )
#define grKeyCtrl   ( (grKey)0x4000 )
#define grKeyShift  ( (grKey)0x2000 )

#define grKeyModifiers ( (grKey)0xE000 )


  typedef struct grEvent_
  {
    int    type;
    grKey  key;
    int    x, y;

  } grEvent;


#endif /* GREVENTS_H_ */
