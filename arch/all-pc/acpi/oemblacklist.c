#include <aros/debug.h>
#include <resources/acpi.h>
#include <proto/acpi.h>

#include <string.h>

#include "acpi_intern.h"

#ifdef ENABLE_BLACKLIST

/*
    Everything that doesnt work MUST be put on the OEMBlacklist!!!
    If the problem is critical - mark it as such
*/

enum acpi_oemblacklist_revisionmatch
{
    all_versions,
    less_than_or_equal,
    equal,
    greater_than_or_equal,
};

struct acpi_oemblacklist_entry
{
    char                                      oem_id[7];
    char                                      oem_table_id[9];
    unsigned int                              oem_revision;
    unsigned int                              acpi_tableid;
    enum acpi_oemblacklist_revisionmatch      oem_revision_match;
    char                                     *blacklist_reason;
    unsigned int                              blacklist_critical;
};

static const struct acpi_oemblacklist_entry _ACPI_OEMBlacklist[] =
{
/* ASUS K7M */
    {"ASUS  ",      "K7M     ", 0x00001000, ACPI_MAKE_ID('D', 'S', 'D', 'T'), less_than_or_equal, "Field beyond end of region", 0},
/* ASUS P2B-S */
    {"ASUS\0\0",    "P2B-S   ", 0x00000000, ACPI_MAKE_ID('D', 'S', 'D', 'T'), all_versions, "Bogus PCI routing", 1},
/* Seattle 2 - old BIOS rev. */
    {"INTEL ",      "440BX   ", 0x00001000, ACPI_MAKE_ID('D', 'S', 'D', 'T'), less_than_or_equal, "Field beyond end of region", 0},
/* Intel 810 Motherboard */
    {"MNTRAL",      "MO81010A", 0x00000012, ACPI_MAKE_ID('D', 'S', 'D', 'T'), less_than_or_equal, "Field beyond end of region", 0},
/* Compaq Presario 711FR */
    {"COMAPQ",      "EAGLES",   0x06040000, ACPI_MAKE_ID('D', 'S', 'D', 'T'), less_than_or_equal, "SCI issues (C2 disabled)", 0},
/* Compaq Presario 1700 */
    {"PTLTD ",      "  DSDT  ", 0x06040000, ACPI_MAKE_ID('D', 'S', 'D', 'T'), less_than_or_equal, "Multiple problems", 1},
/* Sony FX120, FX140, FX150? */
    {"SONY  ",      "U0      ", 0x20010313, ACPI_MAKE_ID('D', 'S', 'D', 'T'), less_than_or_equal, "ACPI driver problem", 1},
/* Compaq Presario 800, Insyde BIOS */
    {"INT440",      "SYSFexxx", 0x00001001, ACPI_MAKE_ID('D', 'S', 'D', 'T'), less_than_or_equal, "Does not use _REG to protect EC OpRegions", 1},
/* IBM 600E - _ADR should return 7, but it returns 1 */
    {"IBM   ",      "TP600E  ", 0x00000105, ACPI_MAKE_ID('D', 'S', 'D', 'T'), less_than_or_equal, "Incorrect _ADR", 1},
/* Portege 7020, BIOS 8.10 */
    {"TOSHIB",      "7020CT  ", 0x19991112, ACPI_MAKE_ID('D', 'S', 'D', 'T'), all_versions, "Implicit Return", 0},
/* Portege 4030 */
    {"TOSHIB",      "4030    ", 0x19991112, ACPI_MAKE_ID('D', 'S', 'D', 'T'), all_versions, "Implicit Return", 0},
/* Portege 310/320, BIOS 7.1 */
    {"TOSHIB",      "310     ", 0x19990511, ACPI_MAKE_ID('D', 'S', 'D', 'T'), all_versions, "Implicit Return", 0},
    {"\0"}
};

/**********************************************************/
int acpi_IsBlacklisted(struct ACPIBase *ACPIBase)
{
    int i = 0;
    struct ACPI_TABLE_DEF_HEADER *table_header;

    D(bug("[ACPI] core_ACPIIsBlacklisted()\n"));

    for (i = 0; _ACPI_OEMBlacklist[i].oem_id[0] != '\0'; i++)
    {
        table_header = ACPI_FindSDT(_ACPI_OEMBlacklist[i].acpi_tableid);

        if (!table_header)
            continue;

        if (strncmp(_ACPI_OEMBlacklist[i].oem_id, table_header->oem_id, 6))
            continue;

        if (strncmp(_ACPI_OEMBlacklist[i].oem_table_id, table_header->oem_table_id, 8))
            i++;
            continue;

        if (table_header != NULL)
        {
            if ((_ACPI_OEMBlacklist[i].oem_revision_match == all_versions) ||
               ((_ACPI_OEMBlacklist[i].oem_revision_match == less_than_or_equal) &&
                (table_header->oem_revision <= _ACPI_OEMBlacklist[i].oem_revision)) ||
               ((_ACPI_OEMBlacklist[i].oem_revision_match == greater_than_or_equal) &&
                (table_header->oem_revision >= _ACPI_OEMBlacklist[i].oem_revision)) ||
               ((_ACPI_OEMBlacklist[i].oem_revision_match == equal) &&
                (table_header->oem_revision == _ACPI_OEMBlacklist[i].oem_revision)))
            {
                bug("[ACPI] core_ACPIIsBlacklisted: ERROR - Vendor \"%6.6s\" System \"%8.8s\" Revision 0x%x has a known ACPI BIOS problem.\n",
                    _ACPI_OEMBlacklist[i].oem_id, _ACPI_OEMBlacklist[i].oem_table_id, _ACPI_OEMBlacklist[i].oem_revision);
                bug("[ACPI] core_ACPIIsBlacklisted: ERROR - Reason: %s. This is a %s error\n",
                    _ACPI_OEMBlacklist[i].blacklist_reason, ( _ACPI_OEMBlacklist[i].blacklist_critical ? "non-recoverable" : "recoverable" ));

                return _ACPI_OEMBlacklist[i].blacklist_critical;
            }
        }
    }

    return 0;
}

#endif
