/*****************
  $Header$

  A comment for the file, RCS header comments are treated specially when first.
  *****************/



/*+ A #include comment +*/
#include <stdio.h>


#include <math.h>  /*+ An alternative #include comment. +*/


/*+ A #define comment. +*/
#define def1 1


#define def2 2  /*+ An alternative #define comment. +*/


/*+++++++++++
  A #define with args

  arg1 The first arg

  arg2 The second arg
  +++++++++++*/

#define def3(arg1,arg2) (arg1+arg2)


/*+ An alternative #define with args. +*/

#define def4(arg1 /*+ The first arg  +*/, \
             arg2 /*+ The second arg +*/) \
           (arg1+arg2)


/*+ An example typedef comment +*/
typedef enum
{
 one,            /*+ one value +*/
 two             /*+ another value +*/
}
type1;


/*+ Another example typedef comment, +*/
typedef struct
{
 int a;         /*+ A variable in a struct. +*/
 union bar
  {
   char a;      /*+ Each element +*/
   int  b,      /*+ of a struct +*/
        c;      /*+ or a union +*/
   long d;      /*+ can have a comment +*/
  }
 e;             /*+ Nested structs and unions also work. +*/
}
type2,          /*+ a type that is a struct. +*/
*ptype2;        /*+ a pointer to a struct type. +*/


/*+ A leading comment only. +*/
int var1,var2;


static int var3; /*+ A trailing comment only. +*/


/*+ A variable for +*/
int var4,    /*+ one thing. +*/
    var5,    /*+ a second thing. +*/
    var6;    /*+ a third thing. +*/

/* Note: The leading comment is combined with each of the trailing comments. */
/* Note: the push through of the comment above on the ',' and ';', see README. */


/*+++++++++++
  A function comment (the comments for the args need to be separated by a blank line).

  int function1 The return value.

  int arg1 The first argument.

  int arg2 The second argument.

  Some more comments

  +none+  Note: this line and the two below are processed specially.
  +html+  This comment is only visible in the HTML output, and can contain HTML markup.
  +latex+ This comment is only visible in the \LaTeX output, and can contain \LaTeX markup.
  +++++++++++*/

int function1(int arg1,int arg2)
{
 /*+ An internal comment in a function that appears as a
   new paragraph at the end of the comment. +*/

 var1=0;

 function2(var3,var4);
}


/*+ An alternative function comment +*/

int function2(int arg1,  /*+ The first argument.  +*/
              int arg2)  /*+ The second argument. +*/
/*+ Returns a value +*/
{
 int (*funcp)()=&function1;
}

/* Note: the push through of the comment above on the ',' and ')', see README. */
