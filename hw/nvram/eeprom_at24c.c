/*
 * *AT24C* series I2C EEPROM
 *
 * Copyright (c) 2015 Michael Davidsaver
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the LICENSE file in the top-level directory.
 */

#include "qemu/osdep.h"

#include "qapi/error.h"
#include "qemu/module.h"
#include "hw/i2c/i2c.h"
#include "hw/nvram/eeprom_at24c.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "sysemu/block-backend.h"
#include "qom/object.h"
#include "slib/inc/i2c-device.h"
#include "hw/i3c/i3c.h"

/* #define DEBUG_AT24C */

#ifdef DEBUG_AT24C
#define DPRINTK(FMT, ...) printf(TYPE_AT24C_EE " : " FMT, ## __VA_ARGS__)
#else
#define DPRINTK(FMT, ...) do {} while (0)
#endif

#define ERR(FMT, ...) fprintf(stderr, TYPE_AT24C_EE " : " FMT, \
                            ## __VA_ARGS__)

#define TYPE_AT24C_EE "at24c-eeprom"
typedef struct EEPROMState EEPROMState;
DECLARE_INSTANCE_CHECKER(EEPROMState, AT24C_EE,
                         TYPE_AT24C_EE)

struct EEPROMState {
    I2CSlave parent_obj;

    /* address counter */
    uint16_t cur;
    /* total size in bytes */
    uint32_t rsize;
    /*
     * address byte number
     *  for  24c01, 24c02 size <= 256 byte, use only 1 byte
     *  otherwise size > 256, use 2 byte
     */
    uint8_t asize;

    bool writable;
    /* cells changed since last START? */
    bool changed;
    /* during WRITE, # of address bytes transferred */
    uint8_t haveaddr;

    uint8_t *mem;

    BlockBackend *blk;

    const uint8_t *init_rom;
    uint32_t init_rom_size;
};

static
int at24c_eeprom_event(I2CSlave *s, enum i2c_event event)
{
    EEPROMState *ee = AT24C_EE(s);

    switch (event) {
    case I2C_START_SEND:
    case I2C_FINISH:
        ee->haveaddr = 0;
        /* fallthrough */
    case I2C_START_RECV:
        DPRINTK("clear\n");
        if (ee->blk && ee->changed) {
            int ret = blk_pwrite(ee->blk, 0, ee->rsize, ee->mem, 0);
            if (ret < 0) {
                ERR(TYPE_AT24C_EE
                        " : failed to write backing file\n");
            }
            DPRINTK("Wrote to backing file\n");
        }
        ee->changed = false;
        break;
    case I2C_NACK:
        break;
    default:
        return -1;
    }
    return 0;
}

static
uint8_t at24c_eeprom_recv(I2CSlave *s)
{
    EEPROMState *ee = AT24C_EE(s);
    uint8_t ret;

    /*
     * If got the byte address but not completely with address size
     * will return the invalid value
     */
    if (ee->haveaddr > 0 && ee->haveaddr < ee->asize) {
        return 0xff;
    }

    ret = ee->mem[ee->cur];

    ee->cur = (ee->cur + 1u) % ee->rsize;
    DPRINTK("Recv %02x %c\n", ret, ret);

    return ret;
}

static
int at24c_eeprom_send(I2CSlave *s, uint8_t data)
{
    EEPROMState *ee = AT24C_EE(s);

    if (ee->haveaddr < ee->asize) {
        ee->cur <<= 8;
        ee->cur |= data;
        ee->haveaddr++;
        if (ee->haveaddr == ee->asize) {
            ee->cur %= ee->rsize;
            DPRINTK("Set pointer %04x\n", ee->cur);
        }

    } else {
        if (ee->writable) {
            DPRINTK("Send %02x\n", data);
            ee->mem[ee->cur] = data;
            ee->changed = true;
        } else {
            DPRINTK("Send error %02x read-only\n", data);
        }
        ee->cur = (ee->cur + 1u) % ee->rsize;

    }

    return 0;
}

I2CSlave *at24c_eeprom_init(I2CBus *bus, uint8_t address, uint32_t rom_size)
{
    return at24c_eeprom_init_rom(bus, address, rom_size, NULL, 0);
}

I2CSlave *at24c_eeprom_init_rom(I2CBus *bus, uint8_t address, uint32_t rom_size,
                                const uint8_t *init_rom, uint32_t init_rom_size)
{
    EEPROMState *s;

    s = AT24C_EE(i2c_slave_new(TYPE_AT24C_EE, address));

    qdev_prop_set_uint32(DEVICE(s), "rom-size", rom_size);

    /* TODO: Model init_rom with QOM properties. */
    s->init_rom = init_rom;
    s->init_rom_size = init_rom_size;

    i2c_slave_realize_and_unref(I2C_SLAVE(s), bus, &error_abort);

    return I2C_SLAVE(s);
}

static void at24c_eeprom_realize(DeviceState *dev, Error **errp)
{
    EEPROMState *ee = AT24C_EE(dev);

    if (ee->init_rom_size > ee->rsize) {
        error_setg(errp, "%s: init rom is larger than rom: %u > %u",
                   TYPE_AT24C_EE, ee->init_rom_size, ee->rsize);
        return;
    }

    if (ee->blk) {
        int64_t len = blk_getlength(ee->blk);

        if (len != ee->rsize) {
            error_setg(errp, "%s: Backing file size %" PRId64 " != %u",
                       TYPE_AT24C_EE, len, ee->rsize);
            return;
        }

        if (blk_set_perm(ee->blk, BLK_PERM_CONSISTENT_READ | BLK_PERM_WRITE,
                         BLK_PERM_ALL, &error_fatal) < 0)
        {
            error_setg(errp, "%s: Backing file incorrect permission",
                       TYPE_AT24C_EE);
            return;
        }
    }

    ee->mem = g_malloc0(ee->rsize);
    memset(ee->mem, 0, ee->rsize);

    if (ee->init_rom) {
        memcpy(ee->mem, ee->init_rom, MIN(ee->init_rom_size, ee->rsize));
    }

    if (ee->blk) {
        int ret = blk_pread(ee->blk, 0, ee->rsize, ee->mem, 0);

        if (ret < 0) {
            ERR(TYPE_AT24C_EE
                    " : Failed initial sync with backing file\n");
        }
        DPRINTK("Reset read backing file\n");
    }

    /*
     * If address size didn't define with property set
     *   value is 0 as default, setting it by Rom size detecting.
     */
    if (ee->asize == 0) {
        if (ee->rsize <= 256) {
            ee->asize = 1;
        } else {
            ee->asize = 2;
        }
    }
}

static
void at24c_eeprom_reset(DeviceState *state)
{
    EEPROMState *ee = AT24C_EE(state);

    ee->changed = false;
    ee->cur = 0;
    ee->haveaddr = 0;
}

static Property at24c_eeprom_props[] = {
    DEFINE_PROP_UINT32("rom-size", EEPROMState, rsize, 0),
    DEFINE_PROP_UINT8("address-size", EEPROMState, asize, 0),
    DEFINE_PROP_BOOL("writable", EEPROMState, writable, true),
    DEFINE_PROP_DRIVE("drive", EEPROMState, blk),
    DEFINE_PROP_END_OF_LIST()
};

static
void at24c_eeprom_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);

    dc->realize = &at24c_eeprom_realize;
    k->event = &at24c_eeprom_event;
    k->recv = &at24c_eeprom_recv;
    k->send = &at24c_eeprom_send;

    device_class_set_props(dc, at24c_eeprom_props);
    dc->reset = at24c_eeprom_reset;
}

static
const TypeInfo at24c_eeprom_type = {
    .name = TYPE_AT24C_EE,
    .parent = TYPE_I2C_SLAVE,
    .instance_size = sizeof(EEPROMState),
    .class_size = sizeof(I2CSlaveClass),
    .class_init = at24c_eeprom_class_init,
};

static void at24c_eeprom_register(void)
{
    type_register_static(&at24c_eeprom_type);
}

type_init(at24c_eeprom_register)


// 宏定义收缩定义内容
// InstanceType  OBJ_NAME   TYPENAME
#define OBJECT_DECLARE_INSTANCE(InstanceType, OBJ_NAME) \
    typedef struct InstanceType InstanceType;           \
    DECLARE_INSTANCE_CHECKER(InstanceType, OBJ_NAME, TYPE_ ## OBJ_NAME)

#define INSTANCE_STRUCT(InstanceType) \
    struct InstanceType {             \
        I2CSlave parent_obj;          \
        I2C_DEVICE_DATA i2cDeviceData;\
    };

#define EMPTY_I2C_DEVICE_EVENT_FUNC(InstanceType, OBJ_NAME) \
    static int InstanceType##_event(I2CSlave *s, enum i2c_event event) { \
        InstanceType *it = OBJ_NAME(s);                 \
        PTR_I2C_DEVICE_DATA ptrI2cDeviceData = &(it->i2cDeviceData);     \
        event_##InstanceType(event, ptrI2cDeviceData);  \
        return 0;                                       \
    };

#define EMPTY_I2C_DEVICE_RECV_FUNC(InstanceType, OBJ_NAME) \
    static uint8_t InstanceType##_recv(I2CSlave *s) {  \
        InstanceType *it = OBJ_NAME(s);                \
        PTR_I2C_DEVICE_DATA ptrI2cDeviceData = &(it->i2cDeviceData); \
        return recv_##InstanceType(ptrI2cDeviceData);  \
    };
#define EMPTY_I2C_DEVICE_SEND_FUNC(InstanceType, OBJ_NAME) \
    static int InstanceType##_send(I2CSlave *s, uint8_t data) { \
        InstanceType *it = OBJ_NAME(s);                \
        PTR_I2C_DEVICE_DATA ptrI2cDeviceData = &(it->i2cDeviceData); \
        send_##InstanceType(data, ptrI2cDeviceData);   \
        return 0;                                      \
    };


#define EMPTY_I2C_DEVICE_RESET_FUNC(InstanceType, OBJ_NAME) \
    static void InstanceType##_reset(DeviceState *dev) {    \
        InstanceType *it = OBJ_NAME(dev);                \
        PTR_I2C_DEVICE_DATA ptrI2cDeviceData = &(it->i2cDeviceData); \
        init_##InstanceType(ptrI2cDeviceData);              \
    };

#define EMPTY_I2C_DEVICE_REALIZE_FUNC(InstanceType, OBJ_NAME) \
    static void InstanceType##_realize(DeviceState *dev, Error **errp){ \
        InstanceType *it = OBJ_NAME(dev);                \
        PTR_I2C_DEVICE_DATA ptrI2cDeviceData = &(it->i2cDeviceData); \
        device_add(getI2cDeviceTypeId(InstanceType##_add), ptrI2cDeviceData->ptrDeviceConfig->name, &(OBJ_NAME(dev)->i2cDeviceData), dev); \
        InstanceType##_reset(dev);                       \
    };


#define EMPTY_I2C_DEVICE_PROPERTY(InstanceType, OBJ_NAME) \
    static Property InstanceType##_props[] = {            \
        DEFINE_PROP_END_OF_LIST()                         \
    };


#define EMPTY_I2C_DEVICE_CLASS_INIT(InstanceType, OBJ_NAME) \
    static void InstanceType##_class_init(ObjectClass *klass, void *data) { \
        DeviceClass *dc = DEVICE_CLASS(klass);              \
        I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);          \
        dc->realize = &InstanceType##_realize;              \
        k->event = &InstanceType##_event;                   \
        k->recv = &InstanceType##_recv;                      \
        k->send = &InstanceType##_send;                     \
        device_class_set_props(dc, InstanceType##_props);   \
        dc->reset = InstanceType##_reset;                   \
    };
#define EMPTY_I2C_DEVICE_TYPEINFO(InstanceType, OBJ_NAME) \
    static const TypeInfo InstanceType##_type = {         \
        .name = TYPE_##OBJ_NAME,                          \
        .parent = TYPE_I2C_SLAVE,                         \
        .instance_size = sizeof(InstanceType),            \
        .class_size = sizeof(I2CSlaveClass),              \
        .class_init = InstanceType##_class_init,          \
    };

#define EMPTY_I2C_DEVICE_REGISTER_INIT(InstanceType, OBJ_NAME) \
    static void InstanceType##_register(void)             \
    {                                                   \
        type_register_static(&InstanceType##_type);       \
    };                                                  \
    type_init(InstanceType##_register);

#define EMPTY_I2C_DEVICE_ADD_FUNC(InstanceType, OBJ_NAME) \
    void InstanceType##_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig) {  \
        InstanceType *it;                                              \
        it = OBJ_NAME(i2c_slave_new(TYPE_##OBJ_NAME, ptrDeviceConfig->addr));        \
        it->i2cDeviceData.ptrDeviceConfig = ptrDeviceConfig;                 \
        if(ptrDeviceConfig->master.i2CType == I2C) {      \
            i2c_slave_realize_and_unref(I2C_SLAVE(it), bus, &error_abort); \
        } else {                                          \
            i2c_slave_realize_and_unref(I2C_SLAVE(it), (I3C_BUS(bus))->i2c_bus, &error_abort); \
        }        \
    };

#define QDEV_I2C_CONFIG_ADD_FUNC(InstanceType, OBJ_NAME) \
    void InstanceType##_qdev_get_config(void *bus, void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig) { \
        InstanceType *it;                                              \
        it = OBJ_NAME(get_i2c_slave_dev(dev));        \
        it->i2cDeviceData.ptrDeviceConfig = ptrDeviceConfig;                 \
        if(ptrDeviceConfig->master.i2CType == I2C) {      \
            i2c_slave_realize_and_unref(I2C_SLAVE(it), bus, &error_abort); \
        } else {                                          \
            i2c_slave_realize_and_unref(I2C_SLAVE(it), (I3C_BUS(bus))->i2c_bus, &error_abort); \
        }        \
    } 

#define INIT_I2C_FUNC(InstanceType, OBJ_NAME) \
    OBJECT_DECLARE_INSTANCE(InstanceType, OBJ_NAME) \
    INSTANCE_STRUCT(InstanceType)              \
    EMPTY_I2C_DEVICE_EVENT_FUNC(InstanceType, OBJ_NAME) \
    EMPTY_I2C_DEVICE_RECV_FUNC(InstanceType, OBJ_NAME)  \
    EMPTY_I2C_DEVICE_SEND_FUNC(InstanceType, OBJ_NAME)  \
    EMPTY_I2C_DEVICE_RESET_FUNC(InstanceType, OBJ_NAME) \
    EMPTY_I2C_DEVICE_REALIZE_FUNC(InstanceType, OBJ_NAME) \
    EMPTY_I2C_DEVICE_PROPERTY(InstanceType, OBJ_NAME)   \
    EMPTY_I2C_DEVICE_CLASS_INIT(InstanceType, OBJ_NAME) \
    EMPTY_I2C_DEVICE_TYPEINFO(InstanceType, OBJ_NAME)   \
    EMPTY_I2C_DEVICE_REGISTER_INIT(InstanceType, OBJ_NAME)\
    EMPTY_I2C_DEVICE_ADD_FUNC(InstanceType, OBJ_NAME) \
    QDEV_I2C_CONFIG_ADD_FUNC(InstanceType, OBJ_NAME)

#define TYPE_I2C_EMPTY_0 "i2c-empty-0"
INIT_I2C_FUNC(I2cEmptyDevice0, I2C_EMPTY_0)

#define TYPE_I2C_EMPTY_1 "i2c-empty-1"
INIT_I2C_FUNC(I2cEmptyDevice1, I2C_EMPTY_1)

#define TYPE_I2C_EMPTY_2 "i2c-empty-2"
INIT_I2C_FUNC(I2cEmptyDevice2, I2C_EMPTY_2)

#define TYPE_I2C_EMPTY_3 "i2c-empty-3"
INIT_I2C_FUNC(I2cEmptyDevice3, I2C_EMPTY_3)

#define TYPE_I2C_EMPTY_4 "i2c-empty-4"
INIT_I2C_FUNC(I2cEmptyDevice4, I2C_EMPTY_4)



I2cFunctionPtr getI2cDeviceAddFunc(int device_type_id){
    static I2cFunctionPtr i2CFunctionPtr_static[] = {
        I2cEmptyDevice0_add,
        I2cEmptyDevice1_add,
        I2cEmptyDevice2_add,
        I2cEmptyDevice3_add,
        I2cEmptyDevice4_add
    };
    if ((device_type_id < I2C_DEVICE_TYPE_ID_OFFSET) || (device_type_id - I2C_DEVICE_TYPE_ID_OFFSET >= I2C_DEVICE_TOTAL_NUM)) {
        return NULL;
    } else {
        return i2CFunctionPtr_static[device_type_id - I2C_DEVICE_TYPE_ID_OFFSET];
    }
};

int getI2cDeviceTypeId(I2cFunctionPtr functionPtr){
    static I2cFunctionPtr i2CFunctionPtr_static[] = {
            I2cEmptyDevice0_add,
            I2cEmptyDevice1_add,
            I2cEmptyDevice2_add,
            I2cEmptyDevice3_add,
            I2cEmptyDevice4_add
    };
    for (int i = 0; i < I2C_DEVICE_TOTAL_NUM; ++i) {
        if (i2CFunctionPtr_static[i] == functionPtr) {
            return I2C_DEVICE_TYPE_ID_OFFSET + i;
        }
    }
    return -1;
};

I2CSlave *get_i2c_slave_dev(void *dev)
{
    return I2C_SLAVE(dev);
}