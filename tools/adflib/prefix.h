#ifndef _PREFIX_H
#define _PREFIX_H 1

/*
 * prefix.h
 *
 */ 

#ifdef WIN32DLL
#define PREFIX __declspec(dllexport)
#else
#define PREFIX 
#endif /* WIN32DLL */

#endif /* _PREFIX_H */
