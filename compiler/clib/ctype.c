/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Arrays for ctype.h
    Lang: english
*/
#include <ctype.h>

const unsigned short int __ctype_b_array[384] =
{
    /* -128 */

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    /* 0 */

    _IScntrl,_IScntrl,_IScntrl,_IScntrl,
    _IScntrl,_IScntrl,
    _IScntrl|_ISprint, /* Bell */
    _IScntrl|_ISprint, /* BackSpace */
    _IScntrl|_ISprint, /* Tab */
    _IScntrl|_ISprint|_ISspace, /* LineFeed */
    _IScntrl|_ISprint|_ISspace, /* Vertical Tab */
    _IScntrl|_ISprint|_ISspace, /* FormFeed */
    _IScntrl|_ISprint|_ISspace, /* Carriage Return */
    _IScntrl,_IScntrl,_IScntrl,
    _IScntrl,_IScntrl,_IScntrl,_IScntrl,
    _IScntrl,_IScntrl,_IScntrl,_IScntrl,
    _IScntrl,_IScntrl,_IScntrl,_IScntrl,
    _IScntrl,_IScntrl,_IScntrl,_IScntrl,

    _ISspace|_ISprint|_ISblank, /* ´ ´ */
    _ISprint|_ISgraph|_ISpunct, /* ´!´ */
    _ISprint|_ISgraph, /* ´"´ */
    _ISprint, /* ´#´ */

    _ISprint, /* ´$´ */
    _ISprint, /* ´%´ */
    _ISprint, /* ´&´ */
    _ISprint, /* ´´´ */

    _ISprint, /* ´(´ */
    _ISprint, /* ´)´ */
    _ISprint, /* ´*´ */
    _ISprint, /* ´+´ */

    _ISprint|_ISpunct, /* ´,´ */
    _ISprint, /* ´-´ */
    _ISprint|_ISpunct, /* ´.´ */
    _ISprint, /* ´/´ */

    _ISprint|_ISdigit|_ISxdigit, /* ´0´ */
    _ISprint|_ISdigit|_ISxdigit, /* ´1´ */
    _ISprint|_ISdigit|_ISxdigit, /* ´2´ */
    _ISprint|_ISdigit|_ISxdigit, /* ´3´ */

    _ISprint|_ISdigit|_ISxdigit, /* ´4´ */
    _ISprint|_ISdigit|_ISxdigit, /* ´5´ */
    _ISprint|_ISdigit|_ISxdigit, /* ´6´ */
    _ISprint|_ISdigit|_ISxdigit, /* ´7´ */

    _ISprint|_ISdigit|_ISxdigit, /* ´8´ */
    _ISprint|_ISdigit|_ISxdigit, /* ´9´ */
    _ISprint|_ISpunct, /* ´:´ */
    _ISprint|_ISpunct, /* ´;´ */

    _ISprint, /* ´<´ */
    _ISprint, /* ´>´ */
    _ISprint, /* ´=´ */
    _ISprint|_ISpunct, /* ´?´ */

    _ISprint, /* ´@´ */
    _ISupper|_ISprint|_ISxdigit|_ISalpha, /* ´A´ */
    _ISupper|_ISprint|_ISxdigit|_ISalpha, /* ´B´ */
    _ISupper|_ISprint|_ISxdigit|_ISalpha, /* ´C´ */

    _ISupper|_ISprint|_ISxdigit|_ISalpha, /* ´D´ */
    _ISupper|_ISprint|_ISxdigit|_ISalpha, /* ´E´ */
    _ISupper|_ISprint|_ISxdigit|_ISalpha, /* ´F´ */
    _ISupper|_ISprint|_ISalpha, /* ´G´ */

    _ISupper|_ISprint|_ISalpha, /* ´H´ */
    _ISupper|_ISprint|_ISalpha, /* ´I´ */
    _ISupper|_ISprint|_ISalpha, /* ´J´ */
    _ISupper|_ISprint|_ISalpha, /* ´K´ */

    _ISupper|_ISprint|_ISalpha, /* ´L´ */
    _ISupper|_ISprint|_ISalpha, /* ´M´ */
    _ISupper|_ISprint|_ISalpha, /* ´N´ */
    _ISupper|_ISprint|_ISalpha, /* ´O´ */

    _ISupper|_ISprint|_ISalpha, /* ´P´ */
    _ISupper|_ISprint|_ISalpha, /* ´Q´ */
    _ISupper|_ISprint|_ISalpha, /* ´R´ */
    _ISupper|_ISprint|_ISalpha, /* ´S´ */

    _ISupper|_ISprint|_ISalpha, /* ´T´ */
    _ISupper|_ISprint|_ISalpha, /* ´U´ */
    _ISupper|_ISprint|_ISalpha, /* ´V´ */
    _ISupper|_ISprint|_ISalpha, /* ´W´ */

    _ISupper|_ISprint|_ISalpha, /* ´X´ */
    _ISupper|_ISprint|_ISalpha, /* ´Y´ */
    _ISupper|_ISprint|_ISalpha, /* ´Z´ */
    _ISprint, /* ´[´ */

    _ISprint, /* ´\´ */
    _ISprint, /* ´]´ */
    _ISprint, /* ´^´ */
    _ISprint, /* ´_´ */

    _ISprint, /* ´`´ */
    _ISlower|_ISprint|_ISxdigit|_ISalpha, /* ´a´ */
    _ISlower|_ISprint|_ISxdigit|_ISalpha, /* ´b´ */
    _ISlower|_ISprint|_ISxdigit|_ISalpha, /* ´c´ */

    _ISlower|_ISprint|_ISxdigit|_ISalpha, /* ´d´ */
    _ISlower|_ISprint|_ISxdigit|_ISalpha, /* ´e´ */
    _ISlower|_ISprint|_ISxdigit|_ISalpha, /* ´f´ */
    _ISlower|_ISprint|_ISalpha, /* ´g´ */

    _ISlower|_ISprint|_ISalpha, /* ´h´ */
    _ISlower|_ISprint|_ISalpha, /* ´i´ */
    _ISlower|_ISprint|_ISalpha, /* ´j´ */
    _ISlower|_ISprint|_ISalpha, /* ´k´ */

    _ISlower|_ISprint|_ISalpha, /* ´l´ */
    _ISlower|_ISprint|_ISalpha, /* ´m´ */
    _ISlower|_ISprint|_ISalpha, /* ´n´ */
    _ISlower|_ISprint|_ISalpha, /* ´o´ */

    _ISlower|_ISprint|_ISalpha, /* ´p´ */
    _ISlower|_ISprint|_ISalpha, /* ´q´ */
    _ISlower|_ISprint|_ISalpha, /* ´r´ */
    _ISlower|_ISprint|_ISalpha, /* ´s´ */

    _ISlower|_ISprint|_ISalpha, /* ´t´ */
    _ISlower|_ISprint|_ISalpha, /* ´u´ */
    _ISlower|_ISprint|_ISalpha, /* ´v´ */
    _ISlower|_ISprint|_ISalpha, /* ´x´ */

    _ISlower|_ISprint|_ISalpha, /* ´x´ */
    _ISlower|_ISprint|_ISalpha, /* ´y´ */
    _ISlower|_ISprint|_ISalpha, /* ´z´ */
    _ISprint, /* ´{´ */

    _ISprint, /* ´|´ */
    _ISprint, /* ´}´ */
    _ISprint, /* ´~´ */
    _IScntrl, /* Delete */

    /* 128 */

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    /* 160 */

    0, /* ´ ´ */
    0, /* ´¡´ */
    0, /* ´¢´ */
    0, /* ´£´ */
    0, /* ´¤´ */
    0, /* ´¥´ */
    0, /* ´¦´ */
    0, /* ´§´ */
    0, /* ´¨´ */
    0, /* ´©´ */
    0, /* ´ª´ */
    0, /* ´«´ */
    0, /* ´¬´ */
    0, /* ´­´ */
    0, /* ´®´ */
    0, /* ´¯´ */
    0, /* ´°´ */
    0, /* ´±´ */
    0, /* ´²´ */
    0, /* ´³´ */
    0, /* ´´´ */
    0, /* ´µ´ */
    0, /* ´¶´ */
    0, /* ´·´ */
    0, /* ´¸´ */
    0, /* ´¹´ */
    0, /* ´º´ */
    0, /* ´»´ */
    0, /* ´¼´ */
    0, /* ´½´ */
    0, /* ´¾´ */
    0, /* ´¿´ */
    0, /* ´À´ */
    0, /* ´Á´ */
    0, /* ´Â´ */
    0, /* ´Ã´ */
    0, /* ´Ä´ */
    0, /* ´Å´ */
    0, /* ´Æ´ */
    0, /* ´Ç´ */
    0, /* ´È´ */
    0, /* ´É´ */
    0, /* ´Ê´ */
    0, /* ´Ë´ */
    0, /* ´Ì´ */
    0, /* ´Í´ */
    0, /* ´Î´ */
    0, /* ´Ï´ */
    0, /* ´Ð´ */
    0, /* ´Ñ´ */
    0, /* ´Ò´ */
    0, /* ´Ó´ */
    0, /* ´Ô´ */
    0, /* ´Õ´ */
    0, /* ´Ö´ */
    0, /* ´×´ */
    0, /* ´Ø´ */
    0, /* ´Ù´ */
    0, /* ´Ú´ */
    0, /* ´Û´ */
    0, /* ´Ü´ */
    0, /* ´Ý´ */
    0, /* ´Þ´ */
    0, /* ´ß´ */
    0, /* ´à´ */
    0, /* ´á´ */
    0, /* ´â´ */
    0, /* ´ã´ */
    0, /* ´ä´ */
    0, /* ´å´ */
    0, /* ´æ´ */
    0, /* ´ç´ */
    0, /* ´è´ */
    0, /* ´é´ */
    0, /* ´ê´ */
    0, /* ´ë´ */
    0, /* ´ì´ */
    0, /* ´í´ */
    0, /* ´î´ */
    0, /* ´ï´ */
    0, /* ´ð´ */
    0, /* ´ñ´ */
    0, /* ´ò´ */
    0, /* ´ó´ */
    0, /* ´ô´ */
    0, /* ´õ´ */
    0, /* ´ö´ */
    0, /* ´÷´ */
    0, /* ´ø´ */
    0, /* ´ù´ */
    0, /* ´ú´ */
    0, /* ´û´ */
    0, /* ´ü´ */
    0, /* ´ý´ */
    0, /* ´þ´ */
    0, /* ´ÿ´ */

    /* 256 */
};

const int __ctype_toupper_array[384] =
{
    /* -128 */

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,-1,

    /* 0 */

    0,1,2,3, 4,5,6,7,
    8,9,10,11, 12,13,14,15,
    16,17,18,19, 20,21,22,23,
    24,25,26,27, 28,29,30,31,

    ' ','!','"','#', '$','%','&','\'',
    '(',')','*','+', ',','-','.','/',
    '0','1','2','3', '4','5','6','7',
    '8','9',':',';', '<','>','=','?',

    '@','A','B','C', 'D','E','F','G',
    'H','I','J','K', 'L','M','N','O',
    'P','Q','R','S', 'T','U','V','W',
    'X','Y','Z','[', '\\',']','^','_',

    '`','A','B','C', 'D','E','F','G',
    'H','I','J','K', 'L','M','N','O',
    'P','Q','R','S', 'T','U','V','W',
    'X','Y','Z','{', '|','}','~',127,

    /* 128 */

    128,129,130,131, 132,133,134,135,
    136,137,138,139, 140,141,142,143,
    144,145,146,147, 148,149,150,151,
    152,153,154,155, 156,157,158,159,

    /* 160 */

    ' ', '¡', '¢', '£', '¤', '¥', '¦', '§',
    '¨', '©', 'ª', '«', '¬', '­', '®', '¯',
    '°', '±', '²', '³', '´', 'µ', '¶', '·',
    '¸', '¹', 'º', '»', '¼', '½', '¾', '¿',

    'À', 'Á', 'Â', 'Ã', 'Ä', 'Å', 'Æ', 'Ç',
    'È', 'É', 'Ê', 'Ë', 'Ì', 'Í', 'Î', 'Ï',
    'Ð', 'Ñ', 'Ò', 'Ó', 'Ô', 'Õ', 'Ö', '×',
    'Ø', 'Ù', 'Ú', 'Û', 'Ü', 'Ý', 'Þ', 'ß',

    'À', 'Á', 'Â', 'Ã', 'Ä', 'Å', 'Æ', 'Ç',
    'È', 'É', 'Ê', 'Ë', 'Ì', 'Í', 'Î', 'Ï',
    'Ð', 'Ñ', 'Ò', 'Ó', 'Ô', 'Õ', 'Ö', '÷',
    'Ø', 'Ù', 'Ú', 'Û', 'Ü', 'Ý', 'Þ', 'ÿ',

    /* 256 */
};

const int __ctype_tolower_array[384] =
{
    /* -128 */

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,-1,

    /* 0 */

    0,1,2,3, 4,5,6,7,
    8,9,10,11, 12,13,14,15,
    16,17,18,19, 20,21,22,23,
    24,25,26,27, 28,29,30,31,

    ' ','!','"','#', '$','%','&','\'',
    '(',')','*','+', ',','-','.','/',
    '0','1','2','3', '4','5','6','7',
    '8','9',':',';', '<','>','=','?',

    '@','a','b','c', 'd','e','f','g',
    'h','i','j','k', 'l','m','n','o',
    'p','q','r','s', 't','u','v','w',
    'x','y','z','[', '\\',']','^','_',

    '`','a','b','c', 'd','e','f','g',
    'h','i','j','k', 'l','m','n','o',
    'p','q','r','s', 't','u','v','w',
    'x','y','z','{', '|','}','~',127,

    /* 128 */

    128,129,130,131, 132,133,134,135,
    136,137,138,139, 140,141,142,143,
    144,145,146,147, 148,149,150,151,
    152,153,154,155, 156,157,158,159,

    /* 160 */

    ' ', '¡', '¢', '£', '¤', '¥', '¦', '§',
    '¨', '©', 'ª', '«', '¬', '­', '®', '¯',
    '°', '±', '²', '³', '´', 'µ', '¶', '·',
    '¸', '¹', 'º', '»', '¼', '½', '¾', '¿',

    'à', 'á', 'â', 'ã', 'ä', 'å', 'æ', 'ç',
    'è', 'é', 'ê', 'ë', 'ì', 'í', 'î', 'ï',
    'ð', 'ñ', 'ò', 'ó', 'ô', 'õ', 'ö', '×',
    'ø', 'ù', 'ú', 'û', 'ü', 'ý', 'þ', 'ß',

    'à', 'á', 'â', 'ã', 'ä', 'å', 'æ', 'ç',
    'è', 'é', 'ê', 'ë', 'ì', 'í', 'î', 'ï',
    'ð', 'ñ', 'ò', 'ó', 'ô', 'õ', 'ö', '÷',
    'ø', 'ù', 'ú', 'û', 'ü', 'ý', 'þ', 'ÿ',

    /* 256 */
};


const unsigned short int * __ctype_b	   = &__ctype_b_array[128];

const int * __ctype_toupper = &__ctype_toupper_array[128];
const int * __ctype_tolower = &__ctype_tolower_array[128];

