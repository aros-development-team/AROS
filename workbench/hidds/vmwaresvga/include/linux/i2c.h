/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_I2C_H_
#define _LINUX_I2C_H_

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/acpi.h>
#include <linux/bits.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/swab.h>
#include <linux/rwsem.h>

#define I2C_NAME_SIZE                   20
#define I2C_M_RD                        0x0001
#define I2C_M_TEN                       0x0010
#define I2C_M_RECV_LEN                  0x0400
#define I2C_M_NO_RD_ACK                 0x0800
#define I2C_M_IGNORE_NAK                0x1000
#define I2C_M_REV_DIR_ADDR              0x2000
#define I2C_M_NOSTART                   0x4000
#define I2C_M_STOP                      0x8000
#define I2C_FUNC_I2C                    0x00000001
#define I2C_FUNC_10BIT_ADDR             0x00000002
#define I2C_FUNC_PROTOCOL_MANGLING      0x00000004
#define I2C_FUNC_SMBUS_PEC              0x00000008
#define I2C_FUNC_NOSTART                0x00000010
#define I2C_FUNC_SMBUS_QUICK            0x00010000
#define I2C_FUNC_SMBUS_READ_BYTE        0x00020000
#define I2C_FUNC_SMBUS_WRITE_BYTE       0x00040000
#define I2C_FUNC_SMBUS_READ_BYTE_DATA   0x00080000
#define I2C_FUNC_SMBUS_WRITE_BYTE_DATA  0x00100000
#define I2C_FUNC_SMBUS_READ_WORD_DATA   0x00200000
#define I2C_FUNC_SMBUS_WRITE_WORD_DATA  0x00400000
#define I2C_FUNC_SMBUS_PROC_CALL        0x00800000
#define I2C_FUNC_SMBUS_BLOCK_PROC_CALL  0x00008000
#define I2C_FUNC_SMBUS_READ_BLOCK_DATA  0x01000000
#define I2C_FUNC_SMBUS_WRITE_BLOCK_DATA 0x02000000
#define I2C_FUNC_SMBUS_READ_I2C_BLOCK   0x04000000
#define I2C_FUNC_SMBUS_WRITE_I2C_BLOCK  0x08000000
#define I2C_FUNC_SMBUS_BYTE             (I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE)
#define I2C_FUNC_SMBUS_BYTE_DATA        (I2C_FUNC_SMBUS_READ_BYTE_DATA | I2C_FUNC_SMBUS_WRITE_BYTE_DATA)
#define I2C_FUNC_SMBUS_WORD_DATA        (I2C_FUNC_SMBUS_READ_WORD_DATA | I2C_FUNC_SMBUS_WRITE_WORD_DATA)
#define I2C_FUNC_SMBUS_BLOCK_DATA       (I2C_FUNC_SMBUS_READ_BLOCK_DATA | I2C_FUNC_SMBUS_WRITE_BLOCK_DATA)
#define I2C_FUNC_SMBUS_I2C_BLOCK        (I2C_FUNC_SMBUS_READ_I2C_BLOCK | I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)
#define I2C_FUNC_SMBUS_EMUL             (I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_PROC_CALL | I2C_FUNC_SMBUS_WRITE_BLOCK_DATA | I2C_FUNC_SMBUS_I2C_BLOCK | I2C_FUNC_SMBUS_PEC)
#define I2C_CLASS_HWMON                 (1 << 0)
#define I2C_CLASS_DDC                   (1 << 3)
#define I2C_CLASS_SPD                   (1 << 7)
#define I2C_AQ_NO_ZERO_LEN              0
#define I2C_SMBUS_BLOCK_MAX             32

struct i2c_msg {
    __u16 addr;
    __u16 flags;
    __u16 len;
    __u8 *buf;
};

struct i2c_adapter;
struct i2c_algorithm {
    int (*master_xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
    int (*xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
    int (*smbus_xfer)(struct i2c_adapter *adap, u16 addr, unsigned short flags, char read_write, u8 command, int size, void *data);
    u32 (*functionality)(struct i2c_adapter *adap);
};
struct i2c_adapter_quirks {
    u64 flags;
    int max_num_msgs;
    u16 max_write_len;
    u16 max_read_len;
    u16 max_comb_1st_msg_len;
    u16 max_comb_2nd_msg_len;
};
struct i2c_lock_operations {
    void (*lock_bus)(struct i2c_adapter *adapter, unsigned int flags);
    int (*trylock_bus)(struct i2c_adapter *adapter, unsigned int flags);
    void (*unlock_bus)(struct i2c_adapter *adapter, unsigned int flags);
};

/*
 * type tells i2c_transfer how to drive the bus: through the adapter's own
 * algorithm, or - for a bit-banged nvkm bus - through the AROS i2c.hidd
 * driver object created by i2c_bit_add_bus().
 */
#define ADAP_TYPE_DEFAULT       1
#define ADAP_TYPE_ALGO          2

struct i2c_adapter {
    struct module *owner;
    unsigned int class;
    const struct i2c_algorithm *algo;
    void *algo_data;
    const struct i2c_lock_operations *lock_ops;
    const struct i2c_adapter_quirks *quirks;
    struct device dev;
    int nr;
    char name[48];
    int timeout;
    int retries;
    IPTR i2cdriver;
    BYTE type;
};
#define to_i2c_adapter(d)       container_of(d, struct i2c_adapter, dev)
static inline void *i2c_get_adapdata(const struct i2c_adapter *adap) { return dev_get_drvdata(&adap->dev); }
static inline void i2c_set_adapdata(struct i2c_adapter *adap, void *data) { dev_set_drvdata(&adap->dev, data); }

struct i2c_board_info {
    char type[I2C_NAME_SIZE];
    unsigned short flags;
    unsigned short addr;
    const char *dev_name;
    void *platform_data;
    struct device_node *of_node;
    struct fwnode_handle *fwnode;
    int irq;
};
#define I2C_BOARD_INFO(dev_type, dev_addr) .type = dev_type, .addr = (dev_addr)

struct i2c_client {
    unsigned short flags;
    unsigned short addr;
    char name[I2C_NAME_SIZE];
    struct i2c_adapter *adapter;
    struct device dev;
    int irq;
};
struct i2c_driver {
    int (*probe)(struct i2c_client *client);
    void (*remove)(struct i2c_client *client);
    struct device_driver driver;
    const struct i2c_device_id *id_table;
    int (*detect)(struct i2c_client *client, struct i2c_board_info *info);
    const unsigned short *address_list;
};
#define to_i2c_client(d)        container_of(d, struct i2c_client, dev)
#define to_i2c_driver(d)        container_of(d, struct i2c_driver, driver)
static inline void *i2c_get_clientdata(const struct i2c_client *client) { return dev_get_drvdata(&client->dev); }
static inline void i2c_set_clientdata(struct i2c_client *client, void *data) { dev_set_drvdata(&client->dev, data); }

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
int __i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
int i2c_add_adapter(struct i2c_adapter *adap);
void i2c_del_adapter(struct i2c_adapter *adap);
int i2c_master_send(const struct i2c_client *client, const char *buf, int count);
int i2c_master_recv(const struct i2c_client *client, char *buf, int count);
struct i2c_client *i2c_new_client_device(struct i2c_adapter *adap, struct i2c_board_info const *info);
void i2c_unregister_device(struct i2c_client *client);
static inline bool i2c_client_has_driver(struct i2c_client *client) { return false; }
static inline int i2c_add_driver(struct i2c_driver *driver) { return 0; }
static inline void i2c_del_driver(struct i2c_driver *driver) { }
#define i2c_register_driver(o, d)   i2c_add_driver(d)
#define module_i2c_driver(d)
static inline u32 i2c_get_functionality(struct i2c_adapter *adap)
{
    return adap->algo && adap->algo->functionality ? adap->algo->functionality(adap) : I2C_FUNC_I2C;
}
static inline int i2c_check_functionality(struct i2c_adapter *adap, u32 func)
{
    return (func & i2c_get_functionality(adap)) == func;
}
#define i2c_lock_bus(a, f)      do { } while (0)
#define i2c_unlock_bus(a, f)    do { } while (0)
#define I2C_LOCK_SEGMENT        1
#define I2C_LOCK_ROOT_ADAPTER   2
struct i2c_smbus_alert_setup;

#endif /* _LINUX_I2C_H_ */
