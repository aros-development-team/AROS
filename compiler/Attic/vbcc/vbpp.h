#ifndef _VBPP_H
#define _VBPP_H 1

/* vbpp.h
 * last change: 17.08.1995 Thorsten Schaaps
 */

/* strnode-types */
#define NORMAL      0          /* anything: brackets,+,-,/,*, etc.. */
#define PP_IDENT    1          /* possible identifier               */
#define ARGUMENT    2          /* argument: see number              */
#define PP_STR      3          /* strings                           */
#define NUMBER      4          /* numbers (123,0x00,0L,..)          */
#define SPACE       5          /* spaces, tabs, etc.                */
#define SPECIAL     6          /* flags=1->#,flags=2->##            */

/* flags for type==SPECIAL */
#define NONE        0
#define TOSTRING    1          /* #define t(c) #c                   */
#define KILLSPACES  2          /* #define t(a,b) a##b               */

struct strnode{
  char           *str;         /* the string =8) ah, you guessed that. */
  int             len;         /* the length of the string             */
  int             flags;       /* flags: see above                     */
  int             type;        /* type:  see above                     */
  int             number;      /* only valid if type==ARGUMENT         */
  struct strnode *prev,*next;  /* pointers to previous and next node or NULL */
};

/* Macro-Node-Flags */
#define FUNCTION  1     /* for macros changing from line to line, e.g. */
                        /*          __LINE__, __FILE__, __TIME__ etc.. */
#define PARAMETER 2     /* Macro has arguments                         */
#define NODELETE  4     /* Macro cannot be UNDEFined, e.g. __TIME__,   */
                        /*          __DATE__, __STDC__                 */
#define NOREDEF   8     /* Macro cannot be reDEFINED, (s. above, but   */
                        /*          not __STDC__)                      */

/* Function-Numbers for FUNCTION-Macros */
#define FUNCLINE  1     /* __LINE__ */
#define FUNCFILE  2     /* __FILE__ */
#define FUNCDATE  3     /* __DATE__ */
#define FUNCTIME  4     /* __TIME__ */
       /* __STDC__ is a normal macro, but cannot be deleted */

struct mnode{
  char           *name;       /* name, e.g. SQR                         */
  char           *args;       /* arguments, e.g. (x)                    */
  char           *token;      /* definition as string, e.g. ((x)*(x))   */
                              /* BE CAREFULL: may be NULL in the future */
  struct strnode *tokenlist;  /* definition as list                     */
  int             flags;      /* flags, see above                       */
  int             numargs;    /* number of arguments                    */
  int             funcnum;    /* number of function (see above)         */
  struct mnode   *prev,*next; /* pointers to previos and next node or NULL */
};

/* Return-Codes for ExpandList/ExpandArgMakro/CloneArg-Functions */
#define OK            0
#define OUT_OF_MEM   -1
#define NUM_OF_ARGS  -2
#define ARG_EXPECTED -3

void            AddMakroNode(struct mnode **, struct mnode *);
void            InsertMakroNode(struct mnode **, struct mnode *, struct mnode *);
void            RemMakroNode(struct mnode **, struct mnode *);
struct mnode   *FindMakroNode(struct mnode *, char *, int);
void            DelMakroNode(struct mnode **, struct mnode *);
void            DelMakroList(struct mnode **);

void            AddStrNode(struct strnode **, struct strnode *, char *);
void            RemStrNode(struct strnode **, struct strnode *);
/* struct strnode *FindStrNode(struct strnode *, char *, int); */
void            DelStrNode(struct strnode **, struct strnode *);
void            DelStrList(struct strnode **);
struct strnode *CloneStrList(struct strnode *, struct strnode *);
struct strnode *DoMakroFunction(struct mnode *);

struct strnode *Str2List(char *);
int             List2Str(struct strnode *, char *, int);

int             ExpandList(struct strnode **);

struct mnode   *ParseIdentifier(char *);
int             PreParse(void);

#endif

