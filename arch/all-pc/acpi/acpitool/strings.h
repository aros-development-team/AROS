/****************************************************************
   This file was created automatically by `FlexCat 2.4'
   from "/home/nostromo/AROS/arch/all-pc/acpi/acpitool/catalogs/acpitool.cd".

   Do NOT edit by hand!
****************************************************************/

#ifndef acpitool_H
#define acpitool_H


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif


#ifdef CATCOMP_ARRAY
#undef CATCOMP_NUMBERS
#undef CATCOMP_STRINGS
#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#endif

#ifdef CATCOMP_BLOCK
#undef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif

/***************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_TITLE 0
#define MSG_WINTITLE 1
#define MSG_DESCRIPTION 2
#define MSG_ERROR_LOCALE 3
#define MSG_ERROR_OK 4
#define MSG_ERROR_HEADER 5
#define MSG_ERROR_NO_ACPI 6
#define MSG_UNKNOWN 7
#define MSG_TABLE_SIGNATURE 8
#define MSG_REVISION 9
#define MSG_OEM_ID 10
#define MSG_OEM_TABLE_ID 11
#define MSG_CREATOR_ID 12
#define MSG_PM_PROFILE 13
#define MSG_SCI_INT 14
#define MSG_SMI_CMD 15
#define MSG_ACPI_ENABLE 16
#define MSG_ACPI_DISABLE 17
#define MSG_S4BIOS 18
#define MSG_PSTATE 19
#define MSG_FLUSH_SIZE 20
#define MSG_FLUSH_STRIDE 21
#define MSG_RTC_DAY_ALARM 22
#define MSG_RTC_MON_ALARM 23
#define MSG_RTC_CENTURY 24
#define MSG_RESET_REG 25
#define MSG_RESET_VAL 26
#define MSG_SPACE_FIXED 27
#define MSG_SPACE_OEM 28
#define MSG_SPACE_UNKNOWN 29
#define MSG_FMT_UNKNOWN_SPACE 30
#define MSG_FMT_KNOWN_SPACE 31
#define MSG_YES 32
#define MSG_NO 33
#define MSG_PC_FLAGS 34
#define MSG_FF_FLAGS 35

#endif /* CATCOMP_NUMBERS */


/***************************************************************/

#ifdef CATCOMP_STRINGS

#define MSG_TITLE_STR "ACPI Tool"
#define MSG_WINTITLE_STR "ACPI Tool"
#define MSG_DESCRIPTION_STR "ACPI querying and managment"
#define MSG_ERROR_LOCALE_STR "Can't open locale"
#define MSG_ERROR_OK_STR "Ok"
#define MSG_ERROR_HEADER_STR "ERROR"
#define MSG_ERROR_NO_ACPI_STR "This machine has no ACPI, or ACPI support has been disabled"
#define MSG_UNKNOWN_STR "Unknown"
#define MSG_TABLE_SIGNATURE_STR "Table signature"
#define MSG_REVISION_STR "Revision"
#define MSG_OEM_ID_STR "OEM ID"
#define MSG_OEM_TABLE_ID_STR "OEM Table ID"
#define MSG_CREATOR_ID_STR "Creator ID"
#define MSG_PM_PROFILE_STR "Preferred system profile"
#define MSG_SCI_INT_STR "SCI interrupt"
#define MSG_SMI_CMD_STR "SMI command register"
#define MSG_ACPI_ENABLE_STR "ACPI Enable command"
#define MSG_ACPI_DISABLE_STR "ACPI Disable command"
#define MSG_S4BIOS_STR "Enter S4BIOS state command"
#define MSG_PSTATE_STR "Processor performance state command"
#define MSG_FLUSH_SIZE_STR "CPU Cache flush size"
#define MSG_FLUSH_STRIDE_STR "CPU Cache flush stride"
#define MSG_RTC_DAY_ALARM_STR "RTC alarm day offset"
#define MSG_RTC_MON_ALARM_STR "RTC alarm month offset"
#define MSG_RTC_CENTURY_STR "RTC century offset"
#define MSG_RESET_REG_STR "Reset register address"
#define MSG_RESET_VAL_STR "Reset value"
#define MSG_SPACE_FIXED_STR "Fixed hardware"
#define MSG_SPACE_OEM_STR "OEM space"
#define MSG_SPACE_UNKNOWN_STR "Unknown space"
#define MSG_FMT_UNKNOWN_SPACE_STR "%s: 0x%llX, %u bits from %u in %s (%u)"
#define MSG_FMT_KNOWN_SPACE_STR "%s: 0x%llX, %u bits from %u in %s"
#define MSG_YES_STR "Yes"
#define MSG_NO_STR "No"
#define MSG_PC_FLAGS_STR "PC Architecture flags"
#define MSG_FF_FLAGS_STR "Fixed Feature flags"

#endif /* CATCOMP_STRINGS */


/***************************************************************/


#ifdef CATCOMP_ARRAY

struct CatCompArrayType
{
  LONG   cca_ID;
  STRPTR cca_Str;
};

static const struct CatCompArrayType CatCompArray[] =
{
  {MSG_TITLE,(STRPTR)MSG_TITLE_STR},
  {MSG_WINTITLE,(STRPTR)MSG_WINTITLE_STR},
  {MSG_DESCRIPTION,(STRPTR)MSG_DESCRIPTION_STR},
  {MSG_ERROR_LOCALE,(STRPTR)MSG_ERROR_LOCALE_STR},
  {MSG_ERROR_OK,(STRPTR)MSG_ERROR_OK_STR},
  {MSG_ERROR_HEADER,(STRPTR)MSG_ERROR_HEADER_STR},
  {MSG_ERROR_NO_ACPI,(STRPTR)MSG_ERROR_NO_ACPI_STR},
  {MSG_UNKNOWN,(STRPTR)MSG_UNKNOWN_STR},
  {MSG_TABLE_SIGNATURE,(STRPTR)MSG_TABLE_SIGNATURE_STR},
  {MSG_REVISION,(STRPTR)MSG_REVISION_STR},
  {MSG_OEM_ID,(STRPTR)MSG_OEM_ID_STR},
  {MSG_OEM_TABLE_ID,(STRPTR)MSG_OEM_TABLE_ID_STR},
  {MSG_CREATOR_ID,(STRPTR)MSG_CREATOR_ID_STR},
  {MSG_PM_PROFILE,(STRPTR)MSG_PM_PROFILE_STR},
  {MSG_SCI_INT,(STRPTR)MSG_SCI_INT_STR},
  {MSG_SMI_CMD,(STRPTR)MSG_SMI_CMD_STR},
  {MSG_ACPI_ENABLE,(STRPTR)MSG_ACPI_ENABLE_STR},
  {MSG_ACPI_DISABLE,(STRPTR)MSG_ACPI_DISABLE_STR},
  {MSG_S4BIOS,(STRPTR)MSG_S4BIOS_STR},
  {MSG_PSTATE,(STRPTR)MSG_PSTATE_STR},
  {MSG_FLUSH_SIZE,(STRPTR)MSG_FLUSH_SIZE_STR},
  {MSG_FLUSH_STRIDE,(STRPTR)MSG_FLUSH_STRIDE_STR},
  {MSG_RTC_DAY_ALARM,(STRPTR)MSG_RTC_DAY_ALARM_STR},
  {MSG_RTC_MON_ALARM,(STRPTR)MSG_RTC_MON_ALARM_STR},
  {MSG_RTC_CENTURY,(STRPTR)MSG_RTC_CENTURY_STR},
  {MSG_RESET_REG,(STRPTR)MSG_RESET_REG_STR},
  {MSG_RESET_VAL,(STRPTR)MSG_RESET_VAL_STR},
  {MSG_SPACE_FIXED,(STRPTR)MSG_SPACE_FIXED_STR},
  {MSG_SPACE_OEM,(STRPTR)MSG_SPACE_OEM_STR},
  {MSG_SPACE_UNKNOWN,(STRPTR)MSG_SPACE_UNKNOWN_STR},
  {MSG_FMT_UNKNOWN_SPACE,(STRPTR)MSG_FMT_UNKNOWN_SPACE_STR},
  {MSG_FMT_KNOWN_SPACE,(STRPTR)MSG_FMT_KNOWN_SPACE_STR},
  {MSG_YES,(STRPTR)MSG_YES_STR},
  {MSG_NO,(STRPTR)MSG_NO_STR},
  {MSG_PC_FLAGS,(STRPTR)MSG_PC_FLAGS_STR},
  {MSG_FF_FLAGS,(STRPTR)MSG_FF_FLAGS_STR},
  {0,NULL}
};

#endif /* CATCOMP_ARRAY */

/***************************************************************/


#ifdef CATCOMP_BLOCK

//static const chat CatCompBlock[] =
//{
//     
//};

#endif /* CATCOMP_BLOCK */

/***************************************************************/

struct LocaleInfo
{
  APTR li_LocaleBase;
  APTR li_Catalog;
};


#ifdef CATCOMP_CODE

STRPTR GetString(struct LocaleInfo *li, LONG stringNum)
{
  LONG *l;
  UWORD *w;
  STRPTR  builtIn;

  l = (LONG *)CatCompBlock;

  while (*l != stringNum)
  {
    w = (UWORD *)((IPTR)l + 4);
    l = (LONG *)((IPTR)l + (LONG)*w + 6);
  }
  builtIn = (STRPTR)((IPTR)l + 6);

#define XLocaleBase LocaleBase
#define LocaleBase li->li_LocaleBase

  if (LocaleBase)
    return(GetCatalogStr(li->li_Catalog,stringNum,builtIn));
#define LocaleBase XLocaleBase
#undef XLocaleBase

  return (builtIn);
}  

#endif /* CATCOMP_CODE */

/***************************************************************/


#endif
