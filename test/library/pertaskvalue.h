/*
 * This is a test for global values in code and shared libraries.
 * This has several requirements:
 * - When porting code ofter including AROS/Amiga include must be avoided to
 *   not generate conflicts with possible own definitions. Therefor in this
 *   file not other include files will be included.
 * - On other systems it is possible that a shared library defines a global
 *   variable and this global variable can then be accessed both from other
 *   shared libraries and from the program.
 *   This is tested by defining pertaskvalue in pertask.library and
 *   accessing it both from userel.library and userel test program.
 *   One imperfection it that it is not yet supported that programs just
 *   define pertaskvalue as a global variable without including the .h file.
 * - It is avoided to call a function of pertask.library to achieve this.
 *   A good compiler that has good link time optimizations can than optimize
 *   the access to the global very well.
 */

/*
Mimic following definition:
int pertaskvalue;
*/
int *__pertask_getvalueptr(void);
#define pertaskvalue (*__pertask_getvalueptr())
