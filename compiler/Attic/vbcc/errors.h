/*  $VER: vbcc (errors.h) V0.4  */

"declaration expected",ERROR|ANSIV|FATAL,                           /*  0   */
"only one input file allowed",ERROR|NOLINE|FATAL,                   /*  1   */
"Flag <%s> specified more than once",NOLINE|WARNING,                /*  2   */
"Flag <%s> needs string",NOLINE|FATAL|ERROR,                        /*  3   */
"Flag <%s> needs value",NOLINE|FATAL|ERROR,                         /*  4   */
"Unknown Flag <%s>",NOLINE|FATAL|ERROR,                             /*  5   */
"No input file",ERROR|NOLINE|FATAL,                                 /*  6   */
"Could not open <%s> for input",NOLINE|FATAL|ERROR,                 /*  7   */
"need a struct or union to get a member",ERROR|ANSIV,
"too many (%d) nested blocks",ERROR|FATAL,
"left block 0",ERROR|ANSIV,                                         /*  10  */
"incomplete struct",ERROR|ANSIV,
"out of memory",ERROR|FATAL,
"redeclaration of struct <%s>",ERROR|ANSIV,
"incomplete type (%s) in struct",ERROR|ANSIV,
"function (%s) in struct/union",ERROR|ANSIV,                        /*  15  */
"redeclaration of struct/union member <%s>",ERROR|ANSIV,
"redeclaration of <%s>",ERROR|ANSIV,
"invalid constant expression",ERROR|ANSIV,
"array dimension must be constant integer",ERROR|ANSIV,
"no declarator and no identifier in prototype",ERROR|ANSIV,         /*  20  */
"invalid storage-class in prototype",ERROR|ANSIV,
"void not the only function argument",ERROR|ANSIV,
"<%s> no member of struct/union",ERROR|ANSIV,
"increment/decrement is only allowed for aithmetic and pointer types",ERROR|ANSIV,
"functions may not return arrays or functions",ERROR|ANSIV,         /*  25  */
"only pointers to functions can be called",ERROR|ANSIV,
"redefinition of var <%s>",ERROR|ANSIV,
"redeclaration of var <%s> with new storage-class",ERROR|ANSIV,
"first operand of conditional-expression must be arithmetic or pointer type",ERROR|ANSIV,
"multiple definitions of var <%s>",ERROR|ANSIV,                     /*  30  */
"operands of : do not match",ERROR|ANSIV,
"function definition in inner block",ERROR|ANSIV,
"redefinition of function <%s>",ERROR|ANSIV,
"invalid storage-class for function",ERROR|ANSIV,
"declaration-specifiers expected",ERROR|ANSIV,                      /*  35  */
"declarator expected",ERROR|ANSIV,
"<%s> is no parameter",ERROR|ANSIV,
"assignment of different structs/unions",ERROR|ANSIV,
"invalid types for assignment",ERROR|ANSIV,
"only 0 can be compared against pointer",WARNING|ANSIV,             /*  40  */
"pointers do not point to the same type",WARNING|ANSIV,
"function initialized",ERROR|ANSIV,
"initialization of incomplete struct",ERROR|FATAL|ANSIV,
"initialization of incomplete union",ERROR|FATAL|ANSIV,
"empty initialization",ERROR|ANSIV,                                 /*  45  */
"initializer not a constant",ERROR|ANSIV,
"double type-specifier",WARNING|ANSIV,
"illegal type-specifier",WARNING|ANSIV,
"multiple storage-classes",WARNING|ANSIV,
"storage-class specifier should be first",WARNING|ANSIV,            /*  50  */
"bitfields must be ints",WARNING|ANSIV,
"bitfield width must be constant integer",WARNING|ANSIV,
"struct/union member needs identifier",WARNING|ANSIV,
"; expected",WARNING|ANSIV,
"struct/union has no members",WARNING|ANSIV,                        /*  55  */
"} expected",WARNING|ANSIV,
", expected",WARNING|ANSIV,
"invalid type-qualifier",WARNING|ANSIV,
") expected",WARNING|ANSIV,
"array dimension has sidefx (will be ignored)",WARNING|ANSIV,       /*  60  */
"array of size 0 (set to 1)",WARNING|ANSIV,
"] expected",WARNING|ANSIV,
"mixed identifier- and parameter-type-list",WARNING|ANSIV,
"var <%s> was never assigned a value",WARNING|DONTWARN|INFUNC,
"var <%s> was never used",WARNING|DONTWARN|INFUNC,                  /*  65  */
"invalid storage-class",WARNING|ANSIV,
"type defaults to int",WARNING|DONTWARN,
"redeclaration of var <%s> with new type",WARNING|ANSIV,
"redeclaration of parameter <%s>",WARNING|ANSIV,
": expected",WARNING|ANSIV,                                         /*  70  */
"illegal escape-sequence in string",WARNING|ANSIV,
"character constant contains multiple chars",WARNING,
"could not evaluate sizeof-expression",ERROR|ANSIV,
"\" expected",ERROR|ANSIV,
"something wrong with numeric constant",ERROR|ANSIV,                /*  75  */
"identifier expected",ERROR|ANSIV|FATAL,
"definition does not match previous declaration",WARNING|ANSIV,
"integer added to illegal pointer",WARNING|ANSIV,
"offset equals size of object",WARNING|DONTWARN,
"offset out of object",WARNING|ANSIV,                               /*  80  */
"only 0 should be cast to pointer",WARNING|DONTWARN,
"unknown identifier <%s>",ERROR|ANSIV,
"too few function arguments",WARNING|ANSIV,
"division by zero (result set to 0)",WARNING|ANSIV,
"assignment of different pointers",WARNING|ANSIV,                   /*  85  */
"lvalue required for assignment",ERROR|ANSIV,
"assignment to constant type",ERROR|ANSIV,
"assignment to incomplete type",ERROR|ANSIV,
"operands for || and && have to be arithmetic or pointer",ERROR|ANSIV,
"bitwise operations need integer operands",ERROR|ANSIV,             /*  90  */
"assignment discards const",WARNING|ANSIV,
"relational expression needs arithmetic or pointer type",ERROR|ANSIV,
"both operands of comparison must be pointers",ERROR|ANSIV,
"operand needs arithmetic type",ERROR|ANSIV,
"pointer arithmetic with void * is not possible",ERROR|ANSIV,       /*  95  */
"pointers can only be subtracted",ERROR|ANSIV,
"invalid types for operation <%s>",ERROR|ANSIV,
"invalid operand type",ERROR|ANSIV,
"integer-pointer is not allowed",ERROR|ANSIV,
"assignment discards volatile",WARNING|ANSIV,                       /*  100 */
"<<, >> and % need integer operands",ERROR|ANSIV,
"casting from void is not allowed",ERROR|ANSIV,
"integer too large to fit into pointer",ERROR|ANSIV,
"only integers can be cast to pointers",ERROR|ANSIV,
"invalid cast",ERROR|ANSIV,                                         /*  105 */
"pointer too large to fit into integer",ERROR|ANSIV,
"unary operator needs arithmetic type",ERROR|ANSIV,
"negation type must be arithmetic or pointer",ERROR|ANSIV,
"complement operator needs integer type",ERROR|ANSIV,
"pointer assignment with different qualifiers",WARNING|ANSIV,       /*  110 */
"dereferenced object is no pointer",ERROR|ANSIV,
"dereferenced object is incomplete",ERROR|ANSIV,
"only 0 should be assigned to pointer",WARNING|ANSIV,
"typedef <%s> is initialized",WARNING|ANSIV,
"lvalue required to take address",ERROR|ANSIV,                      /*  115 */
"unknown var <%s>",ERROR|ANSIV,
"address of register variables not available",ERROR|ANSIV,
"var <%s> initialized after \'extern\'",WARNING,
"const var <%s> not initialized",WARNING,
"function definition after \'extern\'",WARNING|ANSIV,               /*  120 */
"return type of main is not int",WARNING|ANSIV,
"invalid storage-class for function parameter",WARNING|ANSIV,
"formal parameters conflict with parameter-type-list",WARNING|ANSIV,
"parameter type defaults to int",WARNING|DONTWARN,
"no declaration-specifier, used int",WARNING|ANSIV,                 /*  125 */
"no declarator in prototype",WARNING|ANSIV,
"static var <%s> never defined",WARNING,
"} expected",WARNING,
"left operand of comma operator has no side-effects",WARNING,
"label empty",ERROR|ANSIV,                                          /*  130 */
"redefinition of label <%s>",ERROR|ANSIV,
"case without switch",ERROR|ANSIV,
"case-expression must be constant",ERROR|ANSIV,
"case-expression must be integer",ERROR|ANSIV,
"empty if-expression",ERROR|ANSIV,                                  /*  135 */
"if-expression must be arithmetic or pointer",ERROR|ANSIV,
"empty switch-expression",ERROR|ANSIV,
"switch-expression must be integer",ERROR|ANSIV,
"multiple default labels",ERROR|ANSIV,
"while-expression must be arithmetic or pointer",ERROR|ANSIV,       /*  140 */
"empty while-expression",ERROR|ANSIV,
"for-expression must be arithmetic or pointer",ERROR|ANSIV,
"do-while--expression must be arithmetic or pointer",ERROR|ANSIV,
"goto without label",ERROR|ANSIV,
"continue not within loop",ERROR|ANSIV,                             /*  145 */
"break not in matching construct",ERROR|ANSIV,
"label <%s> was never defined",ERROR|ANSIV|INFUNC,
"label <%s> was never used",WARNING|INFUNC,
"register %s not ok",WARNING,
"default not in switch",WARNING|ANSIV,                              /*  150 */
"( expected",WARNING|ANSIV,
"loop eliminated",WARNING,
"statement has no effect",WARNING,
"\'while\' expected",WARNING|ANSIV,
"function should not return a value",WARNING|ANSIV,                 /*  155 */
"function should return a value",WARNING,
"{ expected",WARNING|ANSIV,
"internal error %d in line %d of file %s !!",ERROR|INTERNAL|FATAL,
"there is no message number %d",NOLINE|FATAL|ERROR,
"message number %d cannot be suppressed",ERROR|NOLINE|FATAL,        /*  160 */
"implicit declaration of function <%s>",WARNING|DONTWARN,
"function call without prototype in scope",WARNING|DONTWARN,
"#pragma used",WARNING|DONTWARN,
"assignment in comparison context",WARNING|DONTWARN,
"comparison redundant because operand is unsigned",WARNING,         /*  165 */
"cast to narrow type may cause loss of precision",WARNING|DONTWARN,
"pointer cast may cause alignment problems",WARNING|DONTWARN,
"no declaration of global variable <%s> before definition",WARNING|DONTWARN,
"'extern' inside function",WARNING|DONTWARN,
"dead assignment to <%s%s> eliminated",WARNING|INFUNC|INIC|DONTWARN,/*  170 */
"var <%s> is used before defined",WARNING|INFUNC,
"would need more than %ld optimizer passes for best results",WARNING|INFUNC,
"function <%s> has no return statement",WARNING|INFUNC,
"function <%s> has no return statement",WARNING|DONTWARN|INFUNC,
"this code is weird",WARNING|INFUNC,                                /*  175 */
"size of incomplete type not available",WARNING|ANSIV,
"line too long",FATAL|ERROR|ANSIV|PREPROC,
"identifier must begin with a letter or underscore",FATAL|ERROR|ANSIV|PREPROC,
"cannot redefine macro",ERROR|ANSIV|PREPROC,
"missing ) after argumentlist",ERROR|ANSIV|PREPROC,                 /*  180 */
"identifier expected",ERROR|ANSIV|PREPROC,
"illegal character in identifier",ERROR|ANSIV|PREPROC,
"missing operand before/after ##",ERROR|ANSIV|PREPROC,
"no macro-argument after #-operator",ERROR|ANSIV|PREPROC,
"macro redefinition not allowed",ERROR|ANSIV|PREPROC,               /*  185 */
"unexpected end of file (unterminated comment)",FATAL|ERROR|PREPROC,
"too many nested includes",FATAL|ERROR|PREPROC,
"#else without #if/#ifdef/#ifndef",FATAL|ERROR|ANSIV|PREPROC,
"#else after #else",ERROR|ANSIV|PREPROC,
"#endif without #if",ERROR|ANSIV|PREPROC,                           /*  190 */
"cannot include file",FATAL|ERROR|PREPROC,
"expected \" or < in #include-directive",ERROR|ANSIV|PREPROC,
"unknown #-directive",WARNING|PREPROC,
"wrong number of macro arguments",ERROR|ANSIV|PREPROC,
"macro argument expected",ERROR|ANSIV|PREPROC,                      /*  195 */
"out of memory",FATAL|ERROR|PREPROC,
"macro redefinition",WARNING|PREPROC,
"/* in comment",WARNING|PREPROC,
"cannot undefine macro",ERROR|ANSIV|PREPROC,
"characters after #-directive ignored",WARNING|PREPROC,             /*  200 */
"duplicate case labels",WARNING|ANSIV,
"var <%s> is incomplete",WARNING|ANSIV,
"long float is no longer valid",WARNING|ANSIV,
"long double is not really supported by vbcc",WARNING,
"empty struct-declarations are not yet handled correct",WARNING,    /*  205 */
"identifier too long (only %d characters are significant)",WARNING,
"illegal initialization of var <%s>",WARNING|ANSIV,
"suspicious loop",WARNING|INFUNC,
"ansi/iso-mode turned on",NOLINE|WARNING,
"division by zero (result set to 0)",WARNING|ANSIV|INFUNC|INIC,     /*  210 */
"constant out of range",WARNING|ANSIV,
"constant is unsigned due to size",WARNING|DONTWARN,
"varargs function called without prototype in scope",WARNING,
"suspicious format string",WARNING,
"format string contains \'\\0\'",WARNING,                           /*  215 */
"illegal use of keyword <%s>",WARNING|ANSIV,
"register <%s> used with wrong type",ERROR,
"register <%s> is not free",ERROR,
"'__reg' used in old-style function definition",WARNING,
"unknown register \"%s\"",WARNING,                                  /*  220 */
"'...' only allowed with prototypes",WARNING|ANSIV,
"Hey, do you really know the priority of '&&' vs. '||'?",WARNING|DONTWARN,
"be careful with priorities of <</>> vs. +/-",WARNING,
"adress of auto variable returned",WARNING,
"void function returns a void expression",WARNING,                  /*  225 */
"redeclaration of typedef <%s>",WARNING|ANSIV,

