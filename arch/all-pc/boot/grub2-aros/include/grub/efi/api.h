/* efi.h - declare EFI types and functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_EFI_API_HEADER
#define GRUB_EFI_API_HEADER	1

#include <grub/types.h>
#include <grub/symbol.h>

/* For consistency and safety, we name the EFI-defined types differently.
   All names are transformed into lower case, _t appended, and
   grub_efi_ prepended.  */

/* Constants.  */
#define GRUB_EFI_EVT_TIMER				0x80000000
#define GRUB_EFI_EVT_RUNTIME				0x40000000
#define GRUB_EFI_EVT_RUNTIME_CONTEXT			0x20000000
#define GRUB_EFI_EVT_NOTIFY_WAIT			0x00000100
#define GRUB_EFI_EVT_NOTIFY_SIGNAL			0x00000200
#define GRUB_EFI_EVT_SIGNAL_EXIT_BOOT_SERVICES		0x00000201
#define GRUB_EFI_EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE	0x60000202

#define GRUB_EFI_TPL_APPLICATION	4
#define GRUB_EFI_TPL_CALLBACK		8
#define GRUB_EFI_TPL_NOTIFY		16
#define GRUB_EFI_TPL_HIGH_LEVEL		31

#define GRUB_EFI_MEMORY_UC	0x0000000000000001LL
#define GRUB_EFI_MEMORY_WC	0x0000000000000002LL
#define GRUB_EFI_MEMORY_WT	0x0000000000000004LL
#define GRUB_EFI_MEMORY_WB	0x0000000000000008LL
#define GRUB_EFI_MEMORY_UCE	0x0000000000000010LL
#define GRUB_EFI_MEMORY_WP	0x0000000000001000LL
#define GRUB_EFI_MEMORY_RP	0x0000000000002000LL
#define GRUB_EFI_MEMORY_XP	0x0000000000004000LL
#define GRUB_EFI_MEMORY_RUNTIME	0x8000000000000000LL

#define GRUB_EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL	0x00000001
#define GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL		0x00000002
#define GRUB_EFI_OPEN_PROTOCOL_TEST_PROTOCOL		0x00000004
#define GRUB_EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER	0x00000008
#define GRUB_EFI_OPEN_PROTOCOL_BY_DRIVER		0x00000010
#define GRUB_EFI_OPEN_PROTOCOL_BY_EXCLUSIVE		0x00000020

#define GRUB_EFI_VARIABLE_NON_VOLATILE		0x0000000000000001
#define GRUB_EFI_VARIABLE_BOOTSERVICE_ACCESS	0x0000000000000002
#define GRUB_EFI_VARIABLE_RUNTIME_ACCESS	0x0000000000000004

#define GRUB_EFI_TIME_ADJUST_DAYLIGHT	0x01
#define GRUB_EFI_TIME_IN_DAYLIGHT	0x02

#define GRUB_EFI_UNSPECIFIED_TIMEZONE	0x07FF

#define GRUB_EFI_OPTIONAL_PTR	0x00000001

#define GRUB_EFI_LOADED_IMAGE_GUID	\
  { 0x5b1b31a1, 0x9562, 0x11d2, \
    { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define GRUB_EFI_DISK_IO_GUID	\
  { 0xce345171, 0xba0b, 0x11d2, \
    { 0x8e, 0x4f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define GRUB_EFI_BLOCK_IO_GUID	\
  { 0x964e5b21, 0x6459, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define GRUB_EFI_SERIAL_IO_GUID \
  { 0xbb25cf6f, 0xf1d4, 0x11d2, \
    { 0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0xfd } \
  }

#define GRUB_EFI_SIMPLE_NETWORK_GUID	\
  { 0xa19832b9, 0xac25, 0x11d3, \
    { 0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define GRUB_EFI_PXE_GUID	\
  { 0x03c4e603, 0xac28, 0x11d3, \
    { 0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define GRUB_EFI_DEVICE_PATH_GUID	\
  { 0x09576e91, 0x6d3f, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define GRUB_EFI_MPS_TABLE_GUID	\
  { 0xeb9d2d2f, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define GRUB_EFI_ACPI_TABLE_GUID	\
  { 0xeb9d2d30, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define GRUB_EFI_ACPI_20_TABLE_GUID	\
  { 0x8868e871, 0xe4f1, 0x11d3, \
    { 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define GRUB_EFI_SMBIOS_TABLE_GUID	\
  { 0xeb9d2d31, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define GRUB_EFI_SAL_TABLE_GUID \
  { 0xeb9d2d32, 0x2d88, 0x11d3, \
      { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define GRUB_EFI_HCDP_TABLE_GUID \
  { 0xf951938d, 0x620b, 0x42ef, \
      { 0x82, 0x79, 0xa8, 0x4b, 0x79, 0x61, 0x78, 0x98 } \
  }

struct grub_efi_sal_system_table
{
  grub_uint32_t signature;
  grub_uint32_t total_table_len;
  grub_uint16_t sal_rev;
  grub_uint16_t entry_count;
  grub_uint8_t checksum;
  grub_uint8_t reserved1[7];
  grub_uint16_t sal_a_version;
  grub_uint16_t sal_b_version;
  grub_uint8_t oem_id[32];
  grub_uint8_t product_id[32];
  grub_uint8_t reserved2[8];
  grub_uint8_t entries[0];
};

enum
  {
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_ENTRYPOINT_DESCRIPTOR = 0,
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_MEMORY_DESCRIPTOR = 1,
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_PLATFORM_FEATURES = 2,
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_TRANSLATION_REGISTER_DESCRIPTOR = 3,
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_PURGE_TRANSLATION_COHERENCE = 4,
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_AP_WAKEUP = 5
  };

struct grub_efi_sal_system_table_entrypoint_descriptor
{
  grub_uint8_t type;
  grub_uint8_t pad[7];
  grub_uint64_t pal_proc_addr;
  grub_uint64_t sal_proc_addr;
  grub_uint64_t global_data_ptr;
  grub_uint64_t reserved[2];
};

struct grub_efi_sal_system_table_memory_descriptor
{
  grub_uint8_t type;
  grub_uint8_t sal_used;
  grub_uint8_t attr;
  grub_uint8_t ar;
  grub_uint8_t attr_mask;
  grub_uint8_t mem_type;
  grub_uint8_t usage;
  grub_uint8_t unknown;
  grub_uint64_t addr;
  grub_uint64_t len;
  grub_uint64_t unknown2;
};

struct grub_efi_sal_system_table_platform_features
{
  grub_uint8_t type;
  grub_uint8_t flags;
  grub_uint8_t reserved[14];
};

struct grub_efi_sal_system_table_translation_register_descriptor
{
  grub_uint8_t type;
  grub_uint8_t register_type;
  grub_uint8_t register_number;
  grub_uint8_t reserved[5];
  grub_uint64_t addr;
  grub_uint64_t page_size;
  grub_uint64_t reserver;
};

struct grub_efi_sal_system_table_purge_translation_coherence
{
  grub_uint8_t type;
  grub_uint8_t reserved[3];  
  grub_uint32_t ndomains;
  grub_uint64_t coherence;
};

struct grub_efi_sal_system_table_ap_wakeup
{
  grub_uint8_t type;
  grub_uint8_t mechanism;
  grub_uint8_t reserved[6];
  grub_uint64_t vector;
};

enum
  {
    GRUB_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_BUSLOCK = 1,
    GRUB_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IRQREDIRECT = 2,
    GRUB_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IPIREDIRECT = 4,
    GRUB_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_ITCDRIFT = 8,
  };

typedef enum grub_efi_parity_type
  {
    GRUB_EFI_SERIAL_DEFAULT_PARITY,
    GRUB_EFI_SERIAL_NO_PARITY,
    GRUB_EFI_SERIAL_EVEN_PARITY,
    GRUB_EFI_SERIAL_ODD_PARITY
  }
grub_efi_parity_type_t;

typedef enum grub_efi_stop_bits
  {
    GRUB_EFI_SERIAL_DEFAULT_STOP_BITS,
    GRUB_EFI_SERIAL_1_STOP_BIT,
    GRUB_EFI_SERIAL_1_5_STOP_BITS,
    GRUB_EFI_SERIAL_2_STOP_BITS
  }
  grub_efi_stop_bits_t;

/* Enumerations.  */
enum grub_efi_timer_delay
  {
    GRUB_EFI_TIMER_CANCEL,
    GRUB_EFI_TIMER_PERIODIC,
    GRUB_EFI_TIMER_RELATIVE
  };
typedef enum grub_efi_timer_delay grub_efi_timer_delay_t;

enum grub_efi_allocate_type
  {
    GRUB_EFI_ALLOCATE_ANY_PAGES,
    GRUB_EFI_ALLOCATE_MAX_ADDRESS,
    GRUB_EFI_ALLOCATE_ADDRESS,
    GRUB_EFI_MAX_ALLOCATION_TYPE
  };
typedef enum grub_efi_allocate_type grub_efi_allocate_type_t;

enum grub_efi_memory_type
  {
    GRUB_EFI_RESERVED_MEMORY_TYPE,
    GRUB_EFI_LOADER_CODE,
    GRUB_EFI_LOADER_DATA,
    GRUB_EFI_BOOT_SERVICES_CODE,
    GRUB_EFI_BOOT_SERVICES_DATA,
    GRUB_EFI_RUNTIME_SERVICES_CODE,
    GRUB_EFI_RUNTIME_SERVICES_DATA,
    GRUB_EFI_CONVENTIONAL_MEMORY,
    GRUB_EFI_UNUSABLE_MEMORY,
    GRUB_EFI_ACPI_RECLAIM_MEMORY,
    GRUB_EFI_ACPI_MEMORY_NVS,
    GRUB_EFI_MEMORY_MAPPED_IO,
    GRUB_EFI_MEMORY_MAPPED_IO_PORT_SPACE,
    GRUB_EFI_PAL_CODE,
    GRUB_EFI_MAX_MEMORY_TYPE
  };
typedef enum grub_efi_memory_type grub_efi_memory_type_t;

enum grub_efi_interface_type
  {
    GRUB_EFI_NATIVE_INTERFACE
  };
typedef enum grub_efi_interface_type grub_efi_interface_type_t;

enum grub_efi_locate_search_type
  {
    GRUB_EFI_ALL_HANDLES,
    GRUB_EFI_BY_REGISTER_NOTIFY,
    GRUB_EFI_BY_PROTOCOL
  };
typedef enum grub_efi_locate_search_type grub_efi_locate_search_type_t;

enum grub_efi_reset_type
  {
    GRUB_EFI_RESET_COLD,
    GRUB_EFI_RESET_WARM,
    GRUB_EFI_RESET_SHUTDOWN
  };
typedef enum grub_efi_reset_type grub_efi_reset_type_t;

/* Types.  */
typedef char grub_efi_boolean_t;
typedef long grub_efi_intn_t;
typedef unsigned long grub_efi_uintn_t;
typedef grub_int8_t grub_efi_int8_t;
typedef grub_uint8_t grub_efi_uint8_t;
typedef grub_int16_t grub_efi_int16_t;
typedef grub_uint16_t grub_efi_uint16_t;
typedef grub_int32_t grub_efi_int32_t;
typedef grub_uint32_t grub_efi_uint32_t;
typedef grub_int64_t grub_efi_int64_t;
typedef grub_uint64_t grub_efi_uint64_t;
typedef grub_uint8_t grub_efi_char8_t;
typedef grub_uint16_t grub_efi_char16_t;

#define PRIxGRUB_EFI_UINTN_T "lx"

typedef grub_efi_intn_t grub_efi_status_t;

#define GRUB_EFI_ERROR_CODE(value)	\
  ((1L << (sizeof (grub_efi_status_t) * 8 - 1)) | (value))

#define GRUB_EFI_WARNING_CODE(value)	(value)

#define GRUB_EFI_SUCCESS		0

#define GRUB_EFI_LOAD_ERROR		GRUB_EFI_ERROR_CODE (1)
#define GRUB_EFI_INVALID_PARAMETER	GRUB_EFI_ERROR_CODE (2)
#define GRUB_EFI_UNSUPPORTED		GRUB_EFI_ERROR_CODE (3)
#define GRUB_EFI_BAD_BUFFER_SIZE	GRUB_EFI_ERROR_CODE (4)
#define GRUB_EFI_BUFFER_TOO_SMALL	GRUB_EFI_ERROR_CODE (5)
#define GRUB_EFI_NOT_READY		GRUB_EFI_ERROR_CODE (6)
#define GRUB_EFI_DEVICE_ERROR		GRUB_EFI_ERROR_CODE (7)
#define GRUB_EFI_WRITE_PROTECTED	GRUB_EFI_ERROR_CODE (8)
#define GRUB_EFI_OUT_OF_RESOURCES	GRUB_EFI_ERROR_CODE (9)
#define GRUB_EFI_VOLUME_CORRUPTED	GRUB_EFI_ERROR_CODE (10)
#define GRUB_EFI_VOLUME_FULL		GRUB_EFI_ERROR_CODE (11)
#define GRUB_EFI_NO_MEDIA		GRUB_EFI_ERROR_CODE (12)
#define GRUB_EFI_MEDIA_CHANGED		GRUB_EFI_ERROR_CODE (13)
#define GRUB_EFI_NOT_FOUND		GRUB_EFI_ERROR_CODE (14)
#define GRUB_EFI_ACCESS_DENIED		GRUB_EFI_ERROR_CODE (15)
#define GRUB_EFI_NO_RESPONSE		GRUB_EFI_ERROR_CODE (16)
#define GRUB_EFI_NO_MAPPING		GRUB_EFI_ERROR_CODE (17)
#define GRUB_EFI_TIMEOUT		GRUB_EFI_ERROR_CODE (18)
#define GRUB_EFI_NOT_STARTED		GRUB_EFI_ERROR_CODE (19)
#define GRUB_EFI_ALREADY_STARTED	GRUB_EFI_ERROR_CODE (20)
#define GRUB_EFI_ABORTED		GRUB_EFI_ERROR_CODE (21)
#define GRUB_EFI_ICMP_ERROR		GRUB_EFI_ERROR_CODE (22)
#define GRUB_EFI_TFTP_ERROR		GRUB_EFI_ERROR_CODE (23)
#define GRUB_EFI_PROTOCOL_ERROR		GRUB_EFI_ERROR_CODE (24)
#define GRUB_EFI_INCOMPATIBLE_VERSION	GRUB_EFI_ERROR_CODE (25)
#define GRUB_EFI_SECURITY_VIOLATION	GRUB_EFI_ERROR_CODE (26)
#define GRUB_EFI_CRC_ERROR		GRUB_EFI_ERROR_CODE (27)

#define GRUB_EFI_WARN_UNKNOWN_GLYPH	GRUB_EFI_WARNING_CODE (1)
#define GRUB_EFI_WARN_DELETE_FAILURE	GRUB_EFI_WARNING_CODE (2)
#define GRUB_EFI_WARN_WRITE_FAILURE	GRUB_EFI_WARNING_CODE (3)
#define GRUB_EFI_WARN_BUFFER_TOO_SMALL	GRUB_EFI_WARNING_CODE (4)

typedef void *grub_efi_handle_t;
typedef void *grub_efi_event_t;
typedef grub_efi_uint64_t grub_efi_lba_t;
typedef grub_efi_uintn_t grub_efi_tpl_t;
typedef grub_uint8_t grub_efi_mac_address_t[32];
typedef grub_uint8_t grub_efi_ipv4_address_t[4];
typedef grub_uint16_t grub_efi_ipv6_address_t[8];
typedef grub_uint8_t grub_efi_ip_address_t[8] __attribute__ ((aligned(4)));
typedef grub_efi_uint64_t grub_efi_physical_address_t;
typedef grub_efi_uint64_t grub_efi_virtual_address_t;

struct grub_efi_guid
{
  grub_uint32_t data1;
  grub_uint16_t data2;
  grub_uint16_t data3;
  grub_uint8_t data4[8];
} __attribute__ ((aligned(8)));
typedef struct grub_efi_guid grub_efi_guid_t;

/* XXX although the spec does not specify the padding, this actually
   must have the padding!  */
struct grub_efi_memory_descriptor
{
  grub_efi_uint32_t type;
  grub_efi_uint32_t padding;
  grub_efi_physical_address_t physical_start;
  grub_efi_virtual_address_t virtual_start;
  grub_efi_uint64_t num_pages;
  grub_efi_uint64_t attribute;
} __attribute__ ((packed));
typedef struct grub_efi_memory_descriptor grub_efi_memory_descriptor_t;

/* Device Path definitions.  */
struct grub_efi_device_path
{
  grub_efi_uint8_t type;
  grub_efi_uint8_t subtype;
  grub_efi_uint8_t length[2];
};
typedef struct grub_efi_device_path grub_efi_device_path_t;
/* XXX EFI does not define EFI_DEVICE_PATH_PROTOCOL but uses it.
   It seems to be identical to EFI_DEVICE_PATH.  */
typedef struct grub_efi_device_path grub_efi_device_path_protocol_t;

#define GRUB_EFI_DEVICE_PATH_TYPE(dp)		((dp)->type & 0x7f)
#define GRUB_EFI_DEVICE_PATH_SUBTYPE(dp)	((dp)->subtype)
#define GRUB_EFI_DEVICE_PATH_LENGTH(dp)		\
  ((dp)->length[0] | ((grub_efi_uint16_t) ((dp)->length[1]) << 8))

/* The End of Device Path nodes.  */
#define GRUB_EFI_END_DEVICE_PATH_TYPE			(0xff & 0x7f)

#define GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE		0xff
#define GRUB_EFI_END_THIS_DEVICE_PATH_SUBTYPE		0x01

#define GRUB_EFI_END_ENTIRE_DEVICE_PATH(dp)	\
  (GRUB_EFI_DEVICE_PATH_TYPE (dp) == GRUB_EFI_END_DEVICE_PATH_TYPE \
   && (GRUB_EFI_DEVICE_PATH_SUBTYPE (dp) \
       == GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE))

#define GRUB_EFI_NEXT_DEVICE_PATH(dp)	\
  ((grub_efi_device_path_t *) ((char *) (dp) \
                               + GRUB_EFI_DEVICE_PATH_LENGTH (dp)))

/* Hardware Device Path.  */
#define GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE		1

#define GRUB_EFI_PCI_DEVICE_PATH_SUBTYPE		1

struct grub_efi_pci_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint8_t function;
  grub_efi_uint8_t device;
} __attribute__ ((packed));
typedef struct grub_efi_pci_device_path grub_efi_pci_device_path_t;

#define GRUB_EFI_PCCARD_DEVICE_PATH_SUBTYPE		2

struct grub_efi_pccard_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint8_t function;
} __attribute__ ((packed));
typedef struct grub_efi_pccard_device_path grub_efi_pccard_device_path_t;

#define GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE	3

struct grub_efi_memory_mapped_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t memory_type;
  grub_efi_physical_address_t start_address;
  grub_efi_physical_address_t end_address;
} __attribute__ ((packed));
typedef struct grub_efi_memory_mapped_device_path grub_efi_memory_mapped_device_path_t;

#define GRUB_EFI_VENDOR_DEVICE_PATH_SUBTYPE		4

struct grub_efi_vendor_device_path
{
  grub_efi_device_path_t header;
  grub_efi_guid_t vendor_guid;
  grub_efi_uint8_t vendor_defined_data[0];
} __attribute__ ((packed));
typedef struct grub_efi_vendor_device_path grub_efi_vendor_device_path_t;

#define GRUB_EFI_CONTROLLER_DEVICE_PATH_SUBTYPE		5

struct grub_efi_controller_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t controller_number;
} __attribute__ ((packed));
typedef struct grub_efi_controller_device_path grub_efi_controller_device_path_t;

/* ACPI Device Path.  */
#define GRUB_EFI_ACPI_DEVICE_PATH_TYPE			2

#define GRUB_EFI_ACPI_DEVICE_PATH_SUBTYPE		1

struct grub_efi_acpi_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t hid;
  grub_efi_uint32_t uid;
} __attribute__ ((packed));
typedef struct grub_efi_acpi_device_path grub_efi_acpi_device_path_t;

#define GRUB_EFI_EXPANDED_ACPI_DEVICE_PATH_SUBTYPE	2

struct grub_efi_expanded_acpi_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t hid;
  grub_efi_uint32_t uid;
  grub_efi_uint32_t cid;
  char hidstr[0];
} __attribute__ ((packed));
typedef struct grub_efi_expanded_acpi_device_path grub_efi_expanded_acpi_device_path_t;

#define GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp)	\
  (((grub_efi_expanded_acpi_device_path_t *) dp)->hidstr)
#define GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp)	\
  (GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp) \
   + grub_strlen (GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp)) + 1)
#define GRUB_EFI_EXPANDED_ACPI_CIDSTR(dp)	\
  (GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp) \
   + grub_strlen (GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp)) + 1)

/* Messaging Device Path.  */
#define GRUB_EFI_MESSAGING_DEVICE_PATH_TYPE		3

#define GRUB_EFI_ATAPI_DEVICE_PATH_SUBTYPE		1

struct grub_efi_atapi_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint8_t primary_secondary;
  grub_efi_uint8_t slave_master;
  grub_efi_uint16_t lun;
} __attribute__ ((packed));
typedef struct grub_efi_atapi_device_path grub_efi_atapi_device_path_t;

#define GRUB_EFI_SCSI_DEVICE_PATH_SUBTYPE		2

struct grub_efi_scsi_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint16_t pun;
  grub_efi_uint16_t lun;
} __attribute__ ((packed));
typedef struct grub_efi_scsi_device_path grub_efi_scsi_device_path_t;

#define GRUB_EFI_FIBRE_CHANNEL_DEVICE_PATH_SUBTYPE	3

struct grub_efi_fibre_channel_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t reserved;
  grub_efi_uint64_t wwn;
  grub_efi_uint64_t lun;
} __attribute__ ((packed));
typedef struct grub_efi_fibre_channel_device_path grub_efi_fibre_channel_device_path_t;

#define GRUB_EFI_1394_DEVICE_PATH_SUBTYPE		4

struct grub_efi_1394_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t reserved;
  grub_efi_uint64_t guid;
} __attribute__ ((packed));
typedef struct grub_efi_1394_device_path grub_efi_1394_device_path_t;

#define GRUB_EFI_USB_DEVICE_PATH_SUBTYPE		5

struct grub_efi_usb_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint8_t parent_port_number;
  grub_efi_uint8_t interface;
} __attribute__ ((packed));
typedef struct grub_efi_usb_device_path grub_efi_usb_device_path_t;

#define GRUB_EFI_USB_CLASS_DEVICE_PATH_SUBTYPE		15

struct grub_efi_usb_class_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint16_t vendor_id;
  grub_efi_uint16_t product_id;
  grub_efi_uint8_t device_class;
  grub_efi_uint8_t device_subclass;
  grub_efi_uint8_t device_protocol;
} __attribute__ ((packed));
typedef struct grub_efi_usb_class_device_path grub_efi_usb_class_device_path_t;

#define GRUB_EFI_I2O_DEVICE_PATH_SUBTYPE		6

struct grub_efi_i2o_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t tid;
} __attribute__ ((packed));
typedef struct grub_efi_i2o_device_path grub_efi_i2o_device_path_t;

#define GRUB_EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE	11

struct grub_efi_mac_address_device_path
{
  grub_efi_device_path_t header;
  grub_efi_mac_address_t mac_address;
  grub_efi_uint8_t if_type;
} __attribute__ ((packed));
typedef struct grub_efi_mac_address_device_path grub_efi_mac_address_device_path_t;

#define GRUB_EFI_IPV4_DEVICE_PATH_SUBTYPE		12

struct grub_efi_ipv4_device_path
{
  grub_efi_device_path_t header;
  grub_efi_ipv4_address_t local_ip_address;
  grub_efi_ipv4_address_t remote_ip_address;
  grub_efi_uint16_t local_port;
  grub_efi_uint16_t remote_port;
  grub_efi_uint16_t protocol;
  grub_efi_uint8_t static_ip_address;
} __attribute__ ((packed));
typedef struct grub_efi_ipv4_device_path grub_efi_ipv4_device_path_t;

#define GRUB_EFI_IPV6_DEVICE_PATH_SUBTYPE		13

struct grub_efi_ipv6_device_path
{
  grub_efi_device_path_t header;
  grub_efi_ipv6_address_t local_ip_address;
  grub_efi_ipv6_address_t remote_ip_address;
  grub_efi_uint16_t local_port;
  grub_efi_uint16_t remote_port;
  grub_efi_uint16_t protocol;
  grub_efi_uint8_t static_ip_address;
} __attribute__ ((packed));
typedef struct grub_efi_ipv6_device_path grub_efi_ipv6_device_path_t;

#define GRUB_EFI_INFINIBAND_DEVICE_PATH_SUBTYPE		9

struct grub_efi_infiniband_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t resource_flags;
  grub_efi_uint8_t port_gid[16];
  grub_efi_uint64_t remote_id;
  grub_efi_uint64_t target_port_id;
  grub_efi_uint64_t device_id;
} __attribute__ ((packed));
typedef struct grub_efi_infiniband_device_path grub_efi_infiniband_device_path_t;

#define GRUB_EFI_UART_DEVICE_PATH_SUBTYPE		14

struct grub_efi_uart_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t reserved;
  grub_efi_uint64_t baud_rate;
  grub_efi_uint8_t data_bits;
  grub_efi_uint8_t parity;
  grub_efi_uint8_t stop_bits;
} __attribute__ ((packed));
typedef struct grub_efi_uart_device_path grub_efi_uart_device_path_t;

#define GRUB_EFI_VENDOR_MESSAGING_DEVICE_PATH_SUBTYPE	10

struct grub_efi_vendor_messaging_device_path
{
  grub_efi_device_path_t header;
  grub_efi_guid_t vendor_guid;
  grub_efi_uint8_t vendor_defined_data[0];
} __attribute__ ((packed));
typedef struct grub_efi_vendor_messaging_device_path grub_efi_vendor_messaging_device_path_t;

/* Media Device Path.  */
#define GRUB_EFI_MEDIA_DEVICE_PATH_TYPE			4

#define GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE		1

struct grub_efi_hard_drive_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t partition_number;
  grub_efi_lba_t partition_start;
  grub_efi_lba_t partition_size;
  grub_efi_uint8_t partition_signature[8];
  grub_efi_uint8_t mbr_type;
  grub_efi_uint8_t signature_type;
} __attribute__ ((packed));
typedef struct grub_efi_hard_drive_device_path grub_efi_hard_drive_device_path_t;

#define GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE		2

struct grub_efi_cdrom_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint32_t boot_entry;
  grub_efi_lba_t partition_start;
  grub_efi_lba_t partition_size;
} __attribute__ ((packed));
typedef struct grub_efi_cdrom_device_path grub_efi_cdrom_device_path_t;

#define GRUB_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE	3

struct grub_efi_vendor_media_device_path
{
  grub_efi_device_path_t header;
  grub_efi_guid_t vendor_guid;
  grub_efi_uint8_t vendor_defined_data[0];
} __attribute__ ((packed));
typedef struct grub_efi_vendor_media_device_path grub_efi_vendor_media_device_path_t;

#define GRUB_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE		4

struct grub_efi_file_path_device_path
{
  grub_efi_device_path_t header;
  grub_efi_char16_t path_name[0];
} __attribute__ ((packed));
typedef struct grub_efi_file_path_device_path grub_efi_file_path_device_path_t;

#define GRUB_EFI_PROTOCOL_DEVICE_PATH_SUBTYPE		5

struct grub_efi_protocol_device_path
{
  grub_efi_device_path_t header;
  grub_efi_guid_t guid;
} __attribute__ ((packed));
typedef struct grub_efi_protocol_device_path grub_efi_protocol_device_path_t;

#define GRUB_EFI_PIWG_DEVICE_PATH_SUBTYPE		6

struct grub_efi_piwg_device_path
{
  grub_efi_device_path_t header;
  grub_efi_guid_t guid __attribute__ ((packed));
} __attribute__ ((packed));
typedef struct grub_efi_piwg_device_path grub_efi_piwg_device_path_t;


/* BIOS Boot Specification Device Path.  */
#define GRUB_EFI_BIOS_DEVICE_PATH_TYPE			5

#define GRUB_EFI_BIOS_DEVICE_PATH_SUBTYPE		1

struct grub_efi_bios_device_path
{
  grub_efi_device_path_t header;
  grub_efi_uint16_t device_type;
  grub_efi_uint16_t status_flags;
  char description[0];
} __attribute__ ((packed));
typedef struct grub_efi_bios_device_path grub_efi_bios_device_path_t;

struct grub_efi_open_protocol_information_entry
{
  grub_efi_handle_t agent_handle;
  grub_efi_handle_t controller_handle;
  grub_efi_uint32_t attributes;
  grub_efi_uint32_t open_count;
};
typedef struct grub_efi_open_protocol_information_entry grub_efi_open_protocol_information_entry_t;

struct grub_efi_time
{
  grub_efi_uint16_t year;
  grub_efi_uint8_t month;
  grub_efi_uint8_t day;
  grub_efi_uint8_t hour;
  grub_efi_uint8_t minute;
  grub_efi_uint8_t second;
  grub_efi_uint8_t pad1;
  grub_efi_uint32_t nanosecond;
  grub_efi_int16_t time_zone;
  grub_efi_uint8_t daylight;
  grub_efi_uint8_t pad2;
} __attribute__ ((packed));
typedef struct grub_efi_time grub_efi_time_t;

struct grub_efi_time_capabilities
{
  grub_efi_uint32_t resolution;
  grub_efi_uint32_t accuracy;
  grub_efi_boolean_t sets_to_zero;
};
typedef struct grub_efi_time_capabilities grub_efi_time_capabilities_t;

struct grub_efi_input_key
{
  grub_efi_uint16_t scan_code;
  grub_efi_char16_t unicode_char;
};
typedef struct grub_efi_input_key grub_efi_input_key_t;

struct grub_efi_simple_text_output_mode
{
  grub_efi_int32_t max_mode;
  grub_efi_int32_t mode;
  grub_efi_int32_t attribute;
  grub_efi_int32_t cursor_column;
  grub_efi_int32_t cursor_row;
  grub_efi_boolean_t cursor_visible;
};
typedef struct grub_efi_simple_text_output_mode grub_efi_simple_text_output_mode_t;

/* Tables.  */
struct grub_efi_table_header
{
  grub_efi_uint64_t signature;
  grub_efi_uint32_t revision;
  grub_efi_uint32_t header_size;
  grub_efi_uint32_t crc32;
  grub_efi_uint32_t reserved;
};
typedef struct grub_efi_table_header grub_efi_table_header_t;

struct grub_efi_boot_services
{
  grub_efi_table_header_t hdr;

  grub_efi_tpl_t
  (*raise_tpl) (grub_efi_tpl_t new_tpl);

  void
  (*restore_tpl) (grub_efi_tpl_t old_tpl);

  grub_efi_status_t
  (*allocate_pages) (grub_efi_allocate_type_t type,
		     grub_efi_memory_type_t memory_type,
		     grub_efi_uintn_t pages,
		     grub_efi_physical_address_t *memory);

  grub_efi_status_t
  (*free_pages) (grub_efi_physical_address_t memory,
		 grub_efi_uintn_t pages);

  grub_efi_status_t
  (*get_memory_map) (grub_efi_uintn_t *memory_map_size,
		     grub_efi_memory_descriptor_t *memory_map,
		     grub_efi_uintn_t *map_key,
		     grub_efi_uintn_t *descriptor_size,
		     grub_efi_uint32_t *descriptor_version);

  grub_efi_status_t
  (*allocate_pool) (grub_efi_memory_type_t pool_type,
		    grub_efi_uintn_t size,
		    void **buffer);

  grub_efi_status_t
  (*free_pool) (void *buffer);

  grub_efi_status_t
  (*create_event) (grub_efi_uint32_t type,
		   grub_efi_tpl_t notify_tpl,
		   void (*notify_function) (grub_efi_event_t event,
					    void *context),
		   void *notify_context,
		   grub_efi_event_t *event);

  grub_efi_status_t
  (*set_timer) (grub_efi_event_t event,
		grub_efi_timer_delay_t type,
		grub_efi_uint64_t trigger_time);

   grub_efi_status_t
   (*wait_for_event) (grub_efi_uintn_t num_events,
		      grub_efi_event_t *event,
		      grub_efi_uintn_t *index);

  grub_efi_status_t
  (*signal_event) (grub_efi_event_t event);

  grub_efi_status_t
  (*close_event) (grub_efi_event_t event);

  grub_efi_status_t
  (*check_event) (grub_efi_event_t event);

   grub_efi_status_t
   (*install_protocol_interface) (grub_efi_handle_t *handle,
				  grub_efi_guid_t *protocol,
				  grub_efi_interface_type_t interface_type,
				  void *interface);

  grub_efi_status_t
  (*reinstall_protocol_interface) (grub_efi_handle_t handle,
				   grub_efi_guid_t *protocol,
				   void *old_interface,
				   void *new_interface);

  grub_efi_status_t
  (*uninstall_protocol_interface) (grub_efi_handle_t handle,
				   grub_efi_guid_t *protocol,
				   void *interface);

  grub_efi_status_t
  (*handle_protocol) (grub_efi_handle_t handle,
		      grub_efi_guid_t *protocol,
		      void **interface);

  void *reserved;

  grub_efi_status_t
  (*register_protocol_notify) (grub_efi_guid_t *protocol,
			       grub_efi_event_t event,
			       void **registration);

  grub_efi_status_t
  (*locate_handle) (grub_efi_locate_search_type_t search_type,
		    grub_efi_guid_t *protocol,
		    void *search_key,
		    grub_efi_uintn_t *buffer_size,
		    grub_efi_handle_t *buffer);

  grub_efi_status_t
  (*locate_device_path) (grub_efi_guid_t *protocol,
			 grub_efi_device_path_t **device_path,
			 grub_efi_handle_t *device);

  grub_efi_status_t
  (*install_configuration_table) (grub_efi_guid_t *guid, void *table);

  grub_efi_status_t
  (*load_image) (grub_efi_boolean_t boot_policy,
		 grub_efi_handle_t parent_image_handle,
		 grub_efi_device_path_t *file_path,
		 void *source_buffer,
		 grub_efi_uintn_t source_size,
		 grub_efi_handle_t *image_handle);

  grub_efi_status_t
  (*start_image) (grub_efi_handle_t image_handle,
		  grub_efi_uintn_t *exit_data_size,
		  grub_efi_char16_t **exit_data);

  grub_efi_status_t
  (*exit) (grub_efi_handle_t image_handle,
	   grub_efi_status_t exit_status,
	   grub_efi_uintn_t exit_data_size,
	   grub_efi_char16_t *exit_data) __attribute__((noreturn));

  grub_efi_status_t
  (*unload_image) (grub_efi_handle_t image_handle);

  grub_efi_status_t
  (*exit_boot_services) (grub_efi_handle_t image_handle,
			 grub_efi_uintn_t map_key);

  grub_efi_status_t
  (*get_next_monotonic_count) (grub_efi_uint64_t *count);

  grub_efi_status_t
  (*stall) (grub_efi_uintn_t microseconds);

  grub_efi_status_t
  (*set_watchdog_timer) (grub_efi_uintn_t timeout,
			 grub_efi_uint64_t watchdog_code,
			 grub_efi_uintn_t data_size,
			 grub_efi_char16_t *watchdog_data);

  grub_efi_status_t
  (*connect_controller) (grub_efi_handle_t controller_handle,
			 grub_efi_handle_t *driver_image_handle,
			 grub_efi_device_path_protocol_t *remaining_device_path,
			 grub_efi_boolean_t recursive);

  grub_efi_status_t
  (*disconnect_controller) (grub_efi_handle_t controller_handle,
			    grub_efi_handle_t driver_image_handle,
			    grub_efi_handle_t child_handle);

  grub_efi_status_t
  (*open_protocol) (grub_efi_handle_t handle,
		    grub_efi_guid_t *protocol,
		    void **interface,
		    grub_efi_handle_t agent_handle,
		    grub_efi_handle_t controller_handle,
		    grub_efi_uint32_t attributes);

  grub_efi_status_t
  (*close_protocol) (grub_efi_handle_t handle,
		     grub_efi_guid_t *protocol,
		     grub_efi_handle_t agent_handle,
		     grub_efi_handle_t controller_handle);

  grub_efi_status_t
  (*open_protocol_information) (grub_efi_handle_t handle,
				grub_efi_guid_t *protocol,
				grub_efi_open_protocol_information_entry_t **entry_buffer,
				grub_efi_uintn_t *entry_count);

  grub_efi_status_t
  (*protocols_per_handle) (grub_efi_handle_t handle,
			   grub_efi_guid_t ***protocol_buffer,
			   grub_efi_uintn_t *protocol_buffer_count);

  grub_efi_status_t
  (*locate_handle_buffer) (grub_efi_locate_search_type_t search_type,
			   grub_efi_guid_t *protocol,
			   void *search_key,
			   grub_efi_uintn_t *no_handles,
			   grub_efi_handle_t **buffer);

  grub_efi_status_t
  (*locate_protocol) (grub_efi_guid_t *protocol,
		      void *registration,
		      void **interface);

  grub_efi_status_t
  (*install_multiple_protocol_interfaces) (grub_efi_handle_t *handle, ...);

  grub_efi_status_t
  (*uninstall_multiple_protocol_interfaces) (grub_efi_handle_t handle, ...);

  grub_efi_status_t
  (*calculate_crc32) (void *data,
		      grub_efi_uintn_t data_size,
		      grub_efi_uint32_t *crc32);

  void
  (*copy_mem) (void *destination, void *source, grub_efi_uintn_t length);

  void
  (*set_mem) (void *buffer, grub_efi_uintn_t size, grub_efi_uint8_t value);
};
typedef struct grub_efi_boot_services grub_efi_boot_services_t;

struct grub_efi_runtime_services
{
  grub_efi_table_header_t hdr;

  grub_efi_status_t
  (*get_time) (grub_efi_time_t *time,
	       grub_efi_time_capabilities_t *capabilities);

  grub_efi_status_t
  (*set_time) (grub_efi_time_t *time);

  grub_efi_status_t
  (*get_wakeup_time) (grub_efi_boolean_t *enabled,
		      grub_efi_boolean_t *pending,
		      grub_efi_time_t *time);

  grub_efi_status_t
  (*set_wakeup_time) (grub_efi_boolean_t enabled,
		      grub_efi_time_t *time);

  grub_efi_status_t
  (*set_virtual_address_map) (grub_efi_uintn_t memory_map_size,
			      grub_efi_uintn_t descriptor_size,
			      grub_efi_uint32_t descriptor_version,
			      grub_efi_memory_descriptor_t *virtual_map);

  grub_efi_status_t
  (*convert_pointer) (grub_efi_uintn_t debug_disposition, void **address);

#define GRUB_EFI_GLOBAL_VARIABLE_GUID \
  { 0x8BE4DF61, 0x93CA, 0x11d2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B,0x8C }}


  grub_efi_status_t
  (*get_variable) (grub_efi_char16_t *variable_name,
		   const grub_efi_guid_t *vendor_guid,
		   grub_efi_uint32_t *attributes,
		   grub_efi_uintn_t *data_size,
		   void *data);

  grub_efi_status_t
  (*get_next_variable_name) (grub_efi_uintn_t *variable_name_size,
			     grub_efi_char16_t *variable_name,
			     grub_efi_guid_t *vendor_guid);

  grub_efi_status_t
  (*set_variable) (grub_efi_char16_t *variable_name,
		   grub_efi_guid_t *vendor_guid,
		   grub_efi_uint32_t attributes,
		   grub_efi_uintn_t data_size,
		   void *data);

  grub_efi_status_t
  (*get_next_high_monotonic_count) (grub_efi_uint32_t *high_count);

  void
  (*reset_system) (grub_efi_reset_type_t reset_type,
		   grub_efi_status_t reset_status,
		   grub_efi_uintn_t data_size,
		   grub_efi_char16_t *reset_data);
};
typedef struct grub_efi_runtime_services grub_efi_runtime_services_t;

struct grub_efi_configuration_table
{
  grub_efi_guid_t vendor_guid;
  void *vendor_table;
} __attribute__ ((packed));
typedef struct grub_efi_configuration_table grub_efi_configuration_table_t;

#define GRUB_EFIEMU_SYSTEM_TABLE_SIGNATURE 0x5453595320494249LL
#define GRUB_EFIEMU_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552LL

struct grub_efi_serial_io_interface
{
  grub_efi_uint32_t revision;
  void (*reset) (void);
  grub_efi_status_t (*set_attributes) (struct grub_efi_serial_io_interface *this,
				       grub_efi_uint64_t speed,
				       grub_efi_uint32_t fifo_depth,
				       grub_efi_uint32_t timeout,
				       grub_efi_parity_type_t parity,
				       grub_uint8_t word_len,
				       grub_efi_stop_bits_t stop_bits);
  void (*set_control_bits) (void);
  void (*get_control_bits) (void);
  grub_efi_status_t (*write) (struct grub_efi_serial_io_interface *this,
			      grub_efi_uintn_t *buf_size,
			      void *buffer);
  grub_efi_status_t (*read) (struct grub_efi_serial_io_interface *this,
			     grub_efi_uintn_t *buf_size,
			     void *buffer);
};

struct grub_efi_simple_input_interface
{
  grub_efi_status_t
  (*reset) (struct grub_efi_simple_input_interface *this,
	    grub_efi_boolean_t extended_verification);

  grub_efi_status_t
  (*read_key_stroke) (struct grub_efi_simple_input_interface *this,
		      grub_efi_input_key_t *key);

  grub_efi_event_t wait_for_key;
};
typedef struct grub_efi_simple_input_interface grub_efi_simple_input_interface_t;

struct grub_efi_simple_text_output_interface
{
  grub_efi_status_t
  (*reset) (struct grub_efi_simple_text_output_interface *this,
	    grub_efi_boolean_t extended_verification);

  grub_efi_status_t
  (*output_string) (struct grub_efi_simple_text_output_interface *this,
		    grub_efi_char16_t *string);

  grub_efi_status_t
  (*test_string) (struct grub_efi_simple_text_output_interface *this,
		  grub_efi_char16_t *string);

  grub_efi_status_t
  (*query_mode) (struct grub_efi_simple_text_output_interface *this,
		 grub_efi_uintn_t mode_number,
		 grub_efi_uintn_t *columns,
		 grub_efi_uintn_t *rows);

  grub_efi_status_t
  (*set_mode) (struct grub_efi_simple_text_output_interface *this,
	       grub_efi_uintn_t mode_number);

  grub_efi_status_t
  (*set_attributes) (struct grub_efi_simple_text_output_interface *this,
		     grub_efi_uintn_t attribute);

  grub_efi_status_t
  (*clear_screen) (struct grub_efi_simple_text_output_interface *this);

  grub_efi_status_t
  (*set_cursor_position) (struct grub_efi_simple_text_output_interface *this,
			  grub_efi_uintn_t column,
			  grub_efi_uintn_t row);

  grub_efi_status_t
  (*enable_cursor) (struct grub_efi_simple_text_output_interface *this,
		    grub_efi_boolean_t visible);

  grub_efi_simple_text_output_mode_t *mode;
};
typedef struct grub_efi_simple_text_output_interface grub_efi_simple_text_output_interface_t;

typedef grub_uint8_t grub_efi_pxe_packet_t[1472];

typedef struct grub_efi_pxe_mode
{
  grub_uint8_t unused[52];
  grub_efi_pxe_packet_t dhcp_discover;
  grub_efi_pxe_packet_t dhcp_ack;
  grub_efi_pxe_packet_t proxy_offer;
  grub_efi_pxe_packet_t pxe_discover;
  grub_efi_pxe_packet_t pxe_reply;
} grub_efi_pxe_mode_t;

typedef struct grub_efi_pxe
{
  grub_uint64_t rev;
  void (*start) (void);
  void (*stop) (void);
  void (*dhcp) (void);
  void (*discover) (void);
  void (*mftp) (void);
  void (*udpwrite) (void);
  void (*udpread) (void);
  void (*setipfilter) (void);
  void (*arp) (void);
  void (*setparams) (void);
  void (*setstationip) (void);
  void (*setpackets) (void);
  struct grub_efi_pxe_mode *mode;
} grub_efi_pxe_t;

#define GRUB_EFI_BLACK		0x00
#define GRUB_EFI_BLUE		0x01
#define GRUB_EFI_GREEN		0x02
#define GRUB_EFI_CYAN		0x03
#define GRUB_EFI_RED		0x04
#define GRUB_EFI_MAGENTA	0x05
#define GRUB_EFI_BROWN		0x06
#define GRUB_EFI_LIGHTGRAY	0x07
#define GRUB_EFI_BRIGHT		0x08
#define GRUB_EFI_DARKGRAY	0x08
#define GRUB_EFI_LIGHTBLUE	0x09
#define GRUB_EFI_LIGHTGREEN	0x0A
#define GRUB_EFI_LIGHTCYAN	0x0B
#define GRUB_EFI_LIGHTRED	0x0C
#define GRUB_EFI_LIGHTMAGENTA	0x0D
#define GRUB_EFI_YELLOW		0x0E
#define GRUB_EFI_WHITE		0x0F

#define GRUB_EFI_BACKGROUND_BLACK	0x00
#define GRUB_EFI_BACKGROUND_BLUE	0x10
#define GRUB_EFI_BACKGROUND_GREEN	0x20
#define GRUB_EFI_BACKGROUND_CYAN	0x30
#define GRUB_EFI_BACKGROUND_RED		0x40
#define GRUB_EFI_BACKGROUND_MAGENTA	0x50
#define GRUB_EFI_BACKGROUND_BROWN	0x60
#define GRUB_EFI_BACKGROUND_LIGHTGRAY	0x70

#define GRUB_EFI_TEXT_ATTR(fg, bg)	((fg) | ((bg)))

struct grub_efi_system_table
{
  grub_efi_table_header_t hdr;
  grub_efi_char16_t *firmware_vendor;
  grub_efi_uint32_t firmware_revision;
  grub_efi_handle_t console_in_handler;
  grub_efi_simple_input_interface_t *con_in;
  grub_efi_handle_t console_out_handler;
  grub_efi_simple_text_output_interface_t *con_out;
  grub_efi_handle_t standard_error_handle;
  grub_efi_simple_text_output_interface_t *std_err;
  grub_efi_runtime_services_t *runtime_services;
  grub_efi_boot_services_t *boot_services;
  grub_efi_uintn_t num_table_entries;
  grub_efi_configuration_table_t *configuration_table;
};
typedef struct grub_efi_system_table  grub_efi_system_table_t;

struct grub_efi_loaded_image
{
  grub_efi_uint32_t revision;
  grub_efi_handle_t parent_handle;
  grub_efi_system_table_t *system_table;

  grub_efi_handle_t device_handle;
  grub_efi_device_path_t *file_path;
  void *reserved;

  grub_efi_uint32_t load_options_size;
  void *load_options;

  void *image_base;
  grub_efi_uint64_t image_size;
  grub_efi_memory_type_t image_code_type;
  grub_efi_memory_type_t image_data_type;

  grub_efi_status_t (*unload) (grub_efi_handle_t image_handle);
};
typedef struct grub_efi_loaded_image grub_efi_loaded_image_t;

struct grub_efi_disk_io
{
  grub_efi_uint64_t revision;
  grub_efi_status_t (*read) (struct grub_efi_disk_io *this,
			     grub_efi_uint32_t media_id,
			     grub_efi_uint64_t offset,
			     grub_efi_uintn_t buffer_size,
			     void *buffer);
  grub_efi_status_t (*write) (struct grub_efi_disk_io *this,
			     grub_efi_uint32_t media_id,
			     grub_efi_uint64_t offset,
			     grub_efi_uintn_t buffer_size,
			     void *buffer);
};
typedef struct grub_efi_disk_io grub_efi_disk_io_t;

struct grub_efi_block_io_media
{
  grub_efi_uint32_t media_id;
  grub_efi_boolean_t removable_media;
  grub_efi_boolean_t media_present;
  grub_efi_boolean_t logical_partition;
  grub_efi_boolean_t read_only;
  grub_efi_boolean_t write_caching;
  grub_efi_uint8_t pad[3];
  grub_efi_uint32_t block_size;
  grub_efi_uint32_t io_align;
  grub_efi_uint8_t pad2[4];
  grub_efi_lba_t last_block;
};
typedef struct grub_efi_block_io_media grub_efi_block_io_media_t;

typedef grub_uint8_t grub_efi_mac_t[32];

struct grub_efi_simple_network_mode
{
  grub_uint32_t state;
  grub_uint32_t hwaddr_size;
  grub_uint32_t media_header_size;
  grub_uint32_t max_packet_size;
  grub_uint32_t nvram_size;
  grub_uint32_t nvram_access_size;
  grub_uint32_t receive_filter_mask;
  grub_uint32_t receive_filter_setting;
  grub_uint32_t max_mcast_filter_count;
  grub_uint32_t mcast_filter_count;
  grub_efi_mac_t mcast_filter[16];
  grub_efi_mac_t current_address;
  grub_efi_mac_t broadcast_address;
  grub_efi_mac_t permanent_address;
  grub_uint8_t if_type;
  grub_uint8_t mac_changeable;
  grub_uint8_t multitx_supported;
  grub_uint8_t media_present_supported;
  grub_uint8_t media_present;
};

enum
  {
    GRUB_EFI_NETWORK_STOPPED,
    GRUB_EFI_NETWORK_STARTED,
    GRUB_EFI_NETWORK_INITIALIZED,
  };

struct grub_efi_simple_network
{
  grub_uint64_t revision;
  grub_efi_status_t (*start) (struct grub_efi_simple_network *this);
  void (*stop) (void);
  grub_efi_status_t (*initialize) (struct grub_efi_simple_network *this,
				   grub_efi_uintn_t extra_rx,
				   grub_efi_uintn_t extra_tx);
  void (*reset) (void);
  void (*shutdown) (void);
  void (*receive_filters) (void);
  void (*station_address) (void);
  void (*statistics) (void);
  void (*mcastiptomac) (void);
  void (*nvdata) (void);
  grub_efi_status_t (*get_status) (struct grub_efi_simple_network *this,
				   grub_uint32_t *int_status,
				   void **txbuf);
  grub_efi_status_t (*transmit) (struct grub_efi_simple_network *this,
				 grub_efi_uintn_t header_size,
				 grub_efi_uintn_t buffer_size,
				 void *buffer,
				 grub_efi_mac_t *src_addr,
				 grub_efi_mac_t *dest_addr,
				 grub_efi_uint16_t *protocol);
  grub_efi_status_t (*receive) (struct grub_efi_simple_network *this,
				grub_efi_uintn_t *header_size,
				grub_efi_uintn_t *buffer_size,
				void *buffer,
				grub_efi_mac_t *src_addr,
				grub_efi_mac_t *dest_addr,
				grub_uint16_t *protocol);
  void (*waitforpacket) (void);
  struct grub_efi_simple_network_mode *mode;
};
typedef struct grub_efi_simple_network grub_efi_simple_network_t;


struct grub_efi_block_io
{
  grub_efi_uint64_t revision;
  grub_efi_block_io_media_t *media;
  grub_efi_status_t (*reset) (struct grub_efi_block_io *this,
			      grub_efi_boolean_t extended_verification);
  grub_efi_status_t (*read_blocks) (struct grub_efi_block_io *this,
				    grub_efi_uint32_t media_id,
				    grub_efi_lba_t lba,
				    grub_efi_uintn_t buffer_size,
				    void *buffer);
  grub_efi_status_t (*write_blocks) (struct grub_efi_block_io *this,
				     grub_efi_uint32_t media_id,
				     grub_efi_lba_t lba,
				     grub_efi_uintn_t buffer_size,
				     void *buffer);
  grub_efi_status_t (*flush_blocks) (struct grub_efi_block_io *this);
};
typedef struct grub_efi_block_io grub_efi_block_io_t;

#if (GRUB_TARGET_SIZEOF_VOID_P == 4) || defined (__ia64__)

#define efi_call_0(func)		func()
#define efi_call_1(func, a)		func(a)
#define efi_call_2(func, a, b)		func(a, b)
#define efi_call_3(func, a, b, c)	func(a, b, c)
#define efi_call_4(func, a, b, c, d)	func(a, b, c, d)
#define efi_call_5(func, a, b, c, d, e)	func(a, b, c, d, e)
#define efi_call_6(func, a, b, c, d, e, f) func(a, b, c, d, e, f)
#define efi_call_7(func, a, b, c, d, e, f, g) func(a, b, c, d, e, f, g)
#define efi_call_10(func, a, b, c, d, e, f, g, h, i, j)	func(a, b, c, d, e, f, g, h, i, j)

#else

#define efi_call_0(func) \
  efi_wrap_0(func)
#define efi_call_1(func, a) \
  efi_wrap_1(func, (grub_uint64_t) (a))
#define efi_call_2(func, a, b) \
  efi_wrap_2(func, (grub_uint64_t) (a), (grub_uint64_t) (b))
#define efi_call_3(func, a, b, c) \
  efi_wrap_3(func, (grub_uint64_t) (a), (grub_uint64_t) (b), \
	     (grub_uint64_t) (c))
#define efi_call_4(func, a, b, c, d) \
  efi_wrap_4(func, (grub_uint64_t) (a), (grub_uint64_t) (b), \
	     (grub_uint64_t) (c), (grub_uint64_t) (d))
#define efi_call_5(func, a, b, c, d, e)	\
  efi_wrap_5(func, (grub_uint64_t) (a), (grub_uint64_t) (b), \
	     (grub_uint64_t) (c), (grub_uint64_t) (d), (grub_uint64_t) (e))
#define efi_call_6(func, a, b, c, d, e, f) \
  efi_wrap_6(func, (grub_uint64_t) (a), (grub_uint64_t) (b), \
	     (grub_uint64_t) (c), (grub_uint64_t) (d), (grub_uint64_t) (e), \
	     (grub_uint64_t) (f))
#define efi_call_7(func, a, b, c, d, e, f, g) \
  efi_wrap_7(func, (grub_uint64_t) (a), (grub_uint64_t) (b), \
	     (grub_uint64_t) (c), (grub_uint64_t) (d), (grub_uint64_t) (e), \
	     (grub_uint64_t) (f), (grub_uint64_t) (g))
#define efi_call_10(func, a, b, c, d, e, f, g, h, i, j) \
  efi_wrap_10(func, (grub_uint64_t) (a), (grub_uint64_t) (b), \
	      (grub_uint64_t) (c), (grub_uint64_t) (d), (grub_uint64_t) (e), \
	      (grub_uint64_t) (f), (grub_uint64_t) (g),	(grub_uint64_t) (h), \
	      (grub_uint64_t) (i), (grub_uint64_t) (j))

grub_uint64_t EXPORT_FUNC(efi_wrap_0) (void *func);
grub_uint64_t EXPORT_FUNC(efi_wrap_1) (void *func, grub_uint64_t arg1);
grub_uint64_t EXPORT_FUNC(efi_wrap_2) (void *func, grub_uint64_t arg1,
                                       grub_uint64_t arg2);
grub_uint64_t EXPORT_FUNC(efi_wrap_3) (void *func, grub_uint64_t arg1,
                                       grub_uint64_t arg2, grub_uint64_t arg3);
grub_uint64_t EXPORT_FUNC(efi_wrap_4) (void *func, grub_uint64_t arg1,
                                       grub_uint64_t arg2, grub_uint64_t arg3,
                                       grub_uint64_t arg4);
grub_uint64_t EXPORT_FUNC(efi_wrap_5) (void *func, grub_uint64_t arg1,
                                       grub_uint64_t arg2, grub_uint64_t arg3,
                                       grub_uint64_t arg4, grub_uint64_t arg5);
grub_uint64_t EXPORT_FUNC(efi_wrap_6) (void *func, grub_uint64_t arg1,
                                       grub_uint64_t arg2, grub_uint64_t arg3,
                                       grub_uint64_t arg4, grub_uint64_t arg5,
                                       grub_uint64_t arg6);
grub_uint64_t EXPORT_FUNC(efi_wrap_7) (void *func, grub_uint64_t arg1,
                                       grub_uint64_t arg2, grub_uint64_t arg3,
                                       grub_uint64_t arg4, grub_uint64_t arg5,
                                       grub_uint64_t arg6, grub_uint64_t arg7);
grub_uint64_t EXPORT_FUNC(efi_wrap_10) (void *func, grub_uint64_t arg1,
                                        grub_uint64_t arg2, grub_uint64_t arg3,
                                        grub_uint64_t arg4, grub_uint64_t arg5,
                                        grub_uint64_t arg6, grub_uint64_t arg7,
                                        grub_uint64_t arg8, grub_uint64_t arg9,
                                        grub_uint64_t arg10);
#endif

#endif /* ! GRUB_EFI_API_HEADER */
