typedef struct _VarString VarString;

#define LASTCHAR    (0x7fffffffL)

extern VarString * createvarstring (const char * initial);
extern int appendtovarstring (VarString * vs, const char * str);
extern char * tostring (VarString * vs);
extern void deletevarstring (VarString * vs);
extern int getvschar (VarString * vs, int pos);
