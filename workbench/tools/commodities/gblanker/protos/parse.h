#ifndef PARSE_H
#define PARSE_H

#define GAD_CYCLE   1
#define GAD_SLIDER  2
#define GAD_FONT    3
#define GAD_STRING  4
#define GAD_DISPLAY 5
#define GAD_DELIM   6

LONG ScanDigit( BPTR );
STRPTR ScanToken( BPTR );
STRPTR *ScanTokenArray( BPTR );
LONG ScanType( BPTR );

#endif /* PARSE_H */
