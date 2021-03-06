#ifndef INLINE4_MUIMASTER_H
#define INLINE4_MUIMASTER_H

/*
** This file was auto generated by idltool 50.10.
**
** It provides compatibility to OS3 style library
** calls by substituting functions.
**
** Do not edit manually.
*/ 

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif
#ifndef EXEC_INTERFACES_H
#include <exec/interfaces.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

/* Inline macros for Interface "main" */
#define MUI_NewObjectA(par1, last) IMUIMaster->MUI_NewObjectA(par1, last) 
#define MUI_NewObject IMUIMaster->MUI_NewObject
#define MUI_DisposeObject(last) IMUIMaster->MUI_DisposeObject(last) 
#define MUI_RequestA(par1, par2, par3, par4, par5, par6, last) IMUIMaster->MUI_RequestA(par1, par2, par3, par4, par5, par6, last) 
#if !defined(__cplusplus) && (__STDC_VERSION__ >= 199901L || __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
#define MUI_Request(par1, par2, par3, par4, par5, ...) IMUIMaster->MUI_Request(par1, par2, par3, par4, par5, __VA_ARGS__) 
#endif
#define MUI_AllocAslRequest(par1, last) IMUIMaster->MUI_AllocAslRequest(par1, last) 
#if !defined(__cplusplus) && (__STDC_VERSION__ >= 199901L || __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
#define MUI_AllocAslRequestTags(...) IMUIMaster->MUI_AllocAslRequestTags(__VA_ARGS__) 
#endif
#define MUI_AslRequest(par1, last) IMUIMaster->MUI_AslRequest(par1, last) 
#if !defined(__cplusplus) && (__STDC_VERSION__ >= 199901L || __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
#define MUI_AslRequestTags(...) IMUIMaster->MUI_AslRequestTags(__VA_ARGS__) 
#endif
#define MUI_FreeAslRequest(last) IMUIMaster->MUI_FreeAslRequest(last) 
#define MUI_Error() IMUIMaster->MUI_Error() 
#define MUI_SetError(last) IMUIMaster->MUI_SetError(last) 
#define MUI_GetClass(last) IMUIMaster->MUI_GetClass(last) 
#define MUI_FreeClass(last) IMUIMaster->MUI_FreeClass(last) 
#define MUI_RequestIDCMP(par1, last) IMUIMaster->MUI_RequestIDCMP(par1, last) 
#define MUI_RejectIDCMP(par1, last) IMUIMaster->MUI_RejectIDCMP(par1, last) 
#define MUI_Redraw(par1, last) IMUIMaster->MUI_Redraw(par1, last) 
#define MUI_CreateCustomClass(par1, par2, par3, par4, last) IMUIMaster->MUI_CreateCustomClass(par1, par2, par3, par4, last) 
#define MUI_DeleteCustomClass(last) IMUIMaster->MUI_DeleteCustomClass(last) 
#define MUI_MakeObjectA(par1, last) IMUIMaster->MUI_MakeObjectA(par1, last) 
#if !defined(__cplusplus) && (__STDC_VERSION__ >= 199901L || __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
#define MUI_MakeObject(...) IMUIMaster->MUI_MakeObject(__VA_ARGS__) 
#endif
#define MUI_Layout(par1, par2, par3, par4, par5, last) IMUIMaster->MUI_Layout(par1, par2, par3, par4, par5, last) 
#define MUI_ObtainPen(par1, par2, last) IMUIMaster->MUI_ObtainPen(par1, par2, last) 
#define MUI_ReleasePen(par1, last) IMUIMaster->MUI_ReleasePen(par1, last) 
#define MUI_AddClipping(par1, par2, par3, par4, last) IMUIMaster->MUI_AddClipping(par1, par2, par3, par4, last) 
#define MUI_RemoveClipping(par1, last) IMUIMaster->MUI_RemoveClipping(par1, last) 
#define MUI_AddClipRegion(par1, last) IMUIMaster->MUI_AddClipRegion(par1, last) 
#define MUI_RemoveClipRegion(par1, last) IMUIMaster->MUI_RemoveClipRegion(par1, last) 
#define MUI_BeginRefresh(par1, last) IMUIMaster->MUI_BeginRefresh(par1, last) 
#define MUI_EndRefresh(par1, last) IMUIMaster->MUI_EndRefresh(par1, last) 

#endif /* INLINE4_MUIMASTER_H */
