#define PREFIX_STR "DEVS:Keymaps"
#define PREFIX_LEN 14			/* Two more bytes for '/' and NULL terminator */

struct kms_base
{
    struct KMSLibrary pub;
    struct KeyMapResource *kmr;
    struct Interrupt input_Int;
    BOOL active;
    APTR rom_MapRawKey;
    APTR rom_MapANSI;
};
