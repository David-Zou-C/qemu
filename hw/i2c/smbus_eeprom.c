/*
 * QEMU SMBus EEPROM device
 *
 * Copyright (c) 2007 Arastra, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/i2c/i2c.h"
#include "hw/i2c/smbus_slave.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "hw/i2c/smbus_eeprom.h"
#include "qom/object.h"
#include "slib/inc/smbus-device.h"
#include "hw/i3c/i3c.h"

//#define DEBUG

#define TYPE_SMBUS_EEPROM "smbus-eeprom"

OBJECT_DECLARE_SIMPLE_TYPE(SMBusEEPROMDevice, SMBUS_EEPROM)

#define SMBUS_EEPROM_SIZE 256

struct SMBusEEPROMDevice {
    /* 父类型设备 */
    SMBusDevice smbusdev;
    /* 设备自定义 */
    uint8_t data[SMBUS_EEPROM_SIZE];
    uint8_t *init_data;
    uint8_t offset;
    bool accessed;
};

static uint8_t eeprom_receive_byte(SMBusDevice *dev)
{
    SMBusEEPROMDevice *eeprom = SMBUS_EEPROM(dev);
    uint8_t *data = eeprom->data;
    uint8_t val = data[eeprom->offset++];

    eeprom->accessed = true;
#ifdef DEBUG
    printf("eeprom_receive_byte: addr=0x%02x val=0x%02x\n",
           dev->i2c.address, val);
#endif
    return val;
}

static int eeprom_write_data(SMBusDevice *dev, uint8_t *buf, uint8_t len)
{
    SMBusEEPROMDevice *eeprom = SMBUS_EEPROM(dev);
    uint8_t *data = eeprom->data;

    eeprom->accessed = true;
#ifdef DEBUG
    printf("eeprom_write_byte: addr=0x%02x cmd=0x%02x val=0x%02x\n",
           dev->i2c.address, buf[0], buf[1]);
#endif
    /* len is guaranteed to be > 0 */
    eeprom->offset = buf[0];
    buf++;
    len--;

    for (; len > 0; len--) {
        data[eeprom->offset] = *buf++;
        eeprom->offset = (eeprom->offset + 1) % SMBUS_EEPROM_SIZE;
    }

    return 0;
}

/* 确定SMBus EEPROM设备是否需要保存到虚拟机状态中 */
static bool smbus_eeprom_vmstate_needed(void *opaque)
{
    MachineClass *mc = MACHINE_GET_CLASS(qdev_get_machine());
    SMBusEEPROMDevice *eeprom = opaque;

    return (eeprom->accessed || smbus_vmstate_needed(&eeprom->smbusdev)) &&
        !mc->smbus_no_migration_support; /* eeprom 设备有被访问 或 设备不处于 IDLE 状态 并且 设备不是不可迁移的 */
}

static const VMStateDescription vmstate_smbus_eeprom = {
    .name = "smbus-eeprom",
    .version_id = 1,
    .minimum_version_id = 1,
    .needed = smbus_eeprom_vmstate_needed,
    .fields      = (VMStateField[]) {
        VMSTATE_SMBUS_DEVICE(smbusdev, SMBusEEPROMDevice),
        VMSTATE_UINT8_ARRAY(data, SMBusEEPROMDevice, SMBUS_EEPROM_SIZE),
        VMSTATE_UINT8(offset, SMBusEEPROMDevice),
        VMSTATE_BOOL(accessed, SMBusEEPROMDevice),
        VMSTATE_END_OF_LIST()
    }
};

/*
 * Reset the EEPROM contents to the initial state on a reset.  This
 * isn't really how an EEPROM works, of course, but the general
 * principle of QEMU is to restore function on reset to what it would
 * be if QEMU was stopped and started.
 *
 * The proper thing to do would be to have a backing blockdev to hold
 * the contents and restore that on startup, and not do this on reset.
 * But until that time, act as if we had been stopped and restarted.
 */
static void smbus_eeprom_reset(DeviceState *dev)
{
    SMBusEEPROMDevice *eeprom = SMBUS_EEPROM(dev);

    memcpy(eeprom->data, eeprom->init_data, SMBUS_EEPROM_SIZE);
    eeprom->offset = 0;
}

static void smbus_eeprom_realize(DeviceState *dev, Error **errp)
{
    SMBusEEPROMDevice *eeprom = SMBUS_EEPROM(dev);

    smbus_eeprom_reset(dev);
    if (eeprom->init_data == NULL) {
        error_setg(errp, "init_data cannot be NULL");
    }
}

static void smbus_eeprom_class_initfn(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SMBusDeviceClass *sc = SMBUS_DEVICE_CLASS(klass);

    dc->realize = smbus_eeprom_realize;
    dc->reset = smbus_eeprom_reset;
    sc->receive_byte = eeprom_receive_byte;
    sc->write_data = eeprom_write_data;
    dc->vmsd = &vmstate_smbus_eeprom;
    /* Reason: init_data */
    dc->user_creatable = false;
}

static const TypeInfo smbus_eeprom_info = {
    .name          = TYPE_SMBUS_EEPROM,
    .parent        = TYPE_SMBUS_DEVICE,
    .instance_size = sizeof(SMBusEEPROMDevice),
    .class_init    = smbus_eeprom_class_initfn,
};

static void smbus_eeprom_register_types(void)
{
    type_register_static(&smbus_eeprom_info);
}

/* 注册该设备类型，并且初始化该设备的对象类型 */
type_init(smbus_eeprom_register_types)

void smbus_eeprom_init_one(I2CBus *smbus, uint8_t address, uint8_t *eeprom_buf)
{
    DeviceState *dev;

    dev = qdev_new(TYPE_SMBUS_EEPROM);                                  /* 创建一个 smbus-eeprom 类型的对象 */
    qdev_prop_set_uint8(dev, "address", address);                 /* 设置该对象的 address 属性的值 */
    /* FIXME: use an array of byte or block backend property? */
    SMBUS_EEPROM(dev)->init_data = eeprom_buf;                                /* 设置该对象的 init_data 属性的值 */
    qdev_realize_and_unref(dev, (BusState *)smbus, &error_fatal);   /* 将该对象实例化为设备，并释放该对象本身（实例化的设备不受影响） */
}

void smbus_eeprom_init(I2CBus *smbus, int nb_eeprom,
                       const uint8_t *eeprom_spd, int eeprom_spd_size)
{
    int i;
     /* XXX: make this persistent */

    assert(nb_eeprom <= 8);
    uint8_t *eeprom_buf = g_malloc0(8 * SMBUS_EEPROM_SIZE);
    if (eeprom_spd_size > 0) {
        memcpy(eeprom_buf, eeprom_spd, eeprom_spd_size);
    }

    for (i = 0; i < nb_eeprom; i++) {
        smbus_eeprom_init_one(smbus, 0x50 + i,
                              eeprom_buf + (i * SMBUS_EEPROM_SIZE));
    }
}

/* Generate SDRAM SPD EEPROM data describing a module of type and size */
uint8_t *spd_data_generate(enum sdram_type type, ram_addr_t ram_size)
{
    uint8_t *spd;
    uint8_t nbanks;
    uint16_t density;
    uint32_t size;
    int min_log2, max_log2, sz_log2;
    int i;

    switch (type) {
    case SDR:
        min_log2 = 2;
        max_log2 = 9;
        break;
    case DDR:
        min_log2 = 5;
        max_log2 = 12;
        break;
    case DDR2:
        min_log2 = 7;
        max_log2 = 14;
        break;
    default:
        g_assert_not_reached();
    }
    size = ram_size >> 20; /* work in terms of megabytes */
    sz_log2 = 31 - clz32(size);
    size = 1U << sz_log2;
    assert(ram_size == size * MiB);
    assert(sz_log2 >= min_log2);

    nbanks = 1;
    while (sz_log2 > max_log2 && nbanks < 8) {
        sz_log2--;
        nbanks *= 2;
    }

    assert(size == (1ULL << sz_log2) * nbanks);

    /* split to 2 banks if possible to avoid a bug in MIPS Malta firmware */
    if (nbanks == 1 && sz_log2 > min_log2) {
        sz_log2--;
        nbanks++;
    }

    density = 1ULL << (sz_log2 - 2);
    switch (type) {
    case DDR2:
        density = (density & 0xe0) | (density >> 8 & 0x1f);
        break;
    case DDR:
        density = (density & 0xf8) | (density >> 8 & 0x07);
        break;
    case SDR:
    default:
        density &= 0xff;
        break;
    }

    spd = g_malloc0(256);
    spd[0] = 128;   /* data bytes in EEPROM */
    spd[1] = 8;     /* log2 size of EEPROM */
    spd[2] = type;
    spd[3] = 13;    /* row address bits */
    spd[4] = 10;    /* column address bits */
    spd[5] = (type == DDR2 ? nbanks - 1 : nbanks);
    spd[6] = 64;    /* module data width */
                    /* reserved / data width high */
    spd[8] = 4;     /* interface voltage level */
    spd[9] = 0x25;  /* highest CAS latency */
    spd[10] = 1;    /* access time */
                    /* DIMM configuration 0 = non-ECC */
    spd[12] = 0x82; /* refresh requirements */
    spd[13] = 8;    /* primary SDRAM width */
                    /* ECC SDRAM width */
    spd[15] = (type == DDR2 ? 0 : 1); /* reserved / delay for random col rd */
    spd[16] = 12;   /* burst lengths supported */
    spd[17] = 4;    /* banks per SDRAM device */
    spd[18] = 12;   /* ~CAS latencies supported */
    spd[19] = (type == DDR2 ? 0 : 1); /* reserved / ~CS latencies supported */
    spd[20] = 2;    /* DIMM type / ~WE latencies */
    spd[21] = (type < DDR2 ? 0x20 : 0); /* module features */
                    /* memory chip features */
    spd[23] = 0x12; /* clock cycle time @ medium CAS latency */
                    /* data access time */
                    /* clock cycle time @ short CAS latency */
                    /* data access time */
    spd[27] = 20;   /* min. row precharge time */
    spd[28] = 15;   /* min. row active row delay */
    spd[29] = 20;   /* min. ~RAS to ~CAS delay */
    spd[30] = 45;   /* min. active to precharge time */
    spd[31] = density;
    spd[32] = 20;   /* addr/cmd setup time */
    spd[33] = 8;    /* addr/cmd hold time */
    spd[34] = 20;   /* data input setup time */
    spd[35] = 8;    /* data input hold time */

    /* checksum */
    for (i = 0; i < 63; i++) {
        spd[63] += spd[i];
    }
    return spd;
}

/**************************************** 自定义 SMBus 空壳设备 ****************************************/
#define OBJECT_DECLARE_SIMPLE_EMPTY_TYPE(InstanceType, MODULE_OBJ_NAME) \
    typedef struct InstanceType{                                        \
    SMBusDevice smbusdev;                                               \
    SMBUS_DEVICE_DATA smbus_device_data;                                \
    } InstanceType; \
    \
    G_DEFINE_AUTOPTR_CLEANUP_FUNC(InstanceType, object_unref) \
    \
    DECLARE_INSTANCE_CHECKER(InstanceType, MODULE_OBJ_NAME, TYPE_##MODULE_OBJ_NAME)

/**************************************** 接收数据处理函数 ****************************************/
#define TEMPLATE_EMPTY_RECEIVE_FUNC(DeviceName, NAME) \
    static uint8_t DeviceName##_receive_byte(SMBusDevice *dev) { \
        DeviceName *device = NAME(dev);                   \
        PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData = &(device->smbus_device_data); \
        return receive_##DeviceName(ptrSmbusDeviceData);  \
    };

/**************************************** 写入数据处理函数 ****************************************/
#define TEMPLATE_EMPTY_WRITE_FUNC(DeviceName, NAME) \
    static int DeviceName##_write_byte(SMBusDevice *dev, uint8_t *buf, uint8_t len) { \
    DeviceName *device = NAME(dev);           \
    PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData = &(device->smbus_device_data);          \
    write_##DeviceName(buf, len, ptrSmbusDeviceData); \
    return 0;};

#define TEMPLATE_EMPTY_NEEDED_FUNC(DeviceName) \
    static bool DeviceName##_vmstate_needed(void *opaque){return 1;};

#define TEMPLATE_EMPTY_VMSTATEDESC_STRUCT(NAME, DeviceName) \
    static const VMStateDescription vmstate_##DeviceName = {    \
        .name = TYPE_##NAME,        \
        .version_id=1,             \
        .minimum_version_id = 1,   \
        .needed = DeviceName##_vmstate_needed,                      \
        .fields = (VMStateField[]){\
            VMSTATE_SMBUS_DEVICE(smbusdev, DeviceName),     \
            VMSTATE_END_OF_LIST()}                          \
    };

/**************************************** 重置函数 ****************************************/
#define TEMPLATE_EMPTY_RESET_FUNC(DeviceName, NAME) \
    static void DeviceName##_reset(DeviceState *dev){ \
        DeviceName *device = NAME(dev);           \
        PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData = &(device->smbus_device_data); \
        init_##DeviceName(ptrSmbusDeviceData);    \
    };

/**************************************** 实现函数 ****************************************/
#define TEMPLATE_EMPTY_REALIZE_FUNC(DeviceName, NAME) \
    static void DeviceName##_realize(DeviceState *dev, Error **errp) { \
        DeviceName *device = NAME(dev);           \
        PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData = &(device->smbus_device_data); \
        device_add(getSMBusDeviceTypeId(DeviceName##_add), ptrSmbusDeviceData->ptrDeviceConfig->name, &(NAME(dev)->smbus_device_data), dev); \
        DeviceName##_reset(dev);                       \
    };

/**************************************** 类初始化函数 ****************************************/
#define TEMPLATE_EMPTY_CLASS_INIT(NAME, DeviceName) \
    static void DeviceName##_class_init(ObjectClass *klass, void *data) { \
    DeviceClass *dc = DEVICE_CLASS(klass); \
    SMBusDeviceClass *sc = SMBUS_DEVICE_CLASS(klass);\
    dc->realize = DeviceName##_realize; \
    dc->reset = DeviceName##_reset; \
    sc->receive_byte = DeviceName##_receive_byte; \
    sc->write_data = DeviceName##_write_byte; \
    dc->vmsd = &vmstate_##DeviceName; \
    dc->user_creatable = false;}

/**************************************** TypeInfo 结构体 ****************************************/
#define TEMPLATE_EMPTY_TYPEINFO(NAME, DeviceName) \
    static const TypeInfo DeviceName##_info = {\
        .name = TYPE_##NAME,                   \
        .parent = TYPE_SMBUS_DEVICE,           \
        .instance_size = sizeof(DeviceName),   \
        .class_init = DeviceName##_class_init, \
        };

/**************************************** 类型注册函数 ****************************************/
#define TEMPLATE_EMPTY_REGISTER_TYPES(DeviceName) \
    static void DeviceName##_register_types(void) { \
        type_register_static(&DeviceName##_info);   \
    }

/**************************************** 类型初始化宏注册此类型 ****************************************/
#define TEMPLATE_EMPTY_TYPE_INIT(DeviceName) \
    type_init(DeviceName##_register_types)   \
    ;


/**************************************** 设备添加函数 ****************************************/
#define TEMPLATE_EMPTY_DEVICE_ADD_FUNC(NAME, DeviceName) \
    void DeviceName##_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig) { \
        DeviceState *dev;                                \
        dev = qdev_new(TYPE_##NAME);                     \
        qdev_prop_set_uint8(dev, "address", ptrDeviceConfig->addr);    \
        NAME(dev)->smbus_device_data.ptrDeviceConfig= ptrDeviceConfig;      \
        if (ptrDeviceConfig->master.i2CType == I2C) {    \
            qdev_realize_and_unref(dev, (BusState *)smbus, &error_fatal);   \
        } else {                                         \
            qdev_realize_and_unref(dev, (BusState *)((I3C_BUS(bus))->i2c_bus), &error_fatal);   \
        }    \
    }


/**************************************** 组合以上宏定义函数 ****************************************/

#define INIT_FUNC(NAME, DeviceName) \
    OBJECT_DECLARE_SIMPLE_EMPTY_TYPE(DeviceName, NAME)  \
    TEMPLATE_EMPTY_RECEIVE_FUNC(DeviceName, NAME) \
    TEMPLATE_EMPTY_WRITE_FUNC(DeviceName, NAME)   \
    TEMPLATE_EMPTY_NEEDED_FUNC(DeviceName)  \
    TEMPLATE_EMPTY_VMSTATEDESC_STRUCT(NAME, DeviceName) \
    TEMPLATE_EMPTY_RESET_FUNC(DeviceName, NAME)   \
    TEMPLATE_EMPTY_REALIZE_FUNC(DeviceName, NAME) \
    TEMPLATE_EMPTY_CLASS_INIT(NAME, DeviceName)         \
    TEMPLATE_EMPTY_TYPEINFO(NAME, DeviceName)           \
    TEMPLATE_EMPTY_REGISTER_TYPES(DeviceName)           \
    TEMPLATE_EMPTY_TYPE_INIT(DeviceName)   \
    TEMPLATE_EMPTY_DEVICE_ADD_FUNC(NAME, DeviceName)

/* 声明简单类型对象 */

#define TYPE_SMBUS_EMPTY_0 "smbus-empty-0"
INIT_FUNC(SMBUS_EMPTY_0, SMBusEmptyDevice0)

#define TYPE_SMBUS_EMPTY_1 "smbus-empty-1"
INIT_FUNC(SMBUS_EMPTY_1, SMBusEmptyDevice1)

#define TYPE_SMBUS_EMPTY_2 "smbus-empty-2"
INIT_FUNC(SMBUS_EMPTY_2, SMBusEmptyDevice2)

#define TYPE_SMBUS_EMPTY_3 "smbus-empty-3"
INIT_FUNC(SMBUS_EMPTY_3, SMBusEmptyDevice3)

#define TYPE_SMBUS_EMPTY_4 "smbus-empty-4"
INIT_FUNC(SMBUS_EMPTY_4, SMBusEmptyDevice4)

#define TYPE_SMBUS_EMPTY_5 "smbus-empty-5"
INIT_FUNC(SMBUS_EMPTY_5, SMBusEmptyDevice5)

#define TYPE_SMBUS_EMPTY_6 "smbus-empty-6"
INIT_FUNC(SMBUS_EMPTY_6, SMBusEmptyDevice6)

#define TYPE_SMBUS_EMPTY_7 "smbus-empty-7"
INIT_FUNC(SMBUS_EMPTY_7, SMBusEmptyDevice7)

#define TYPE_SMBUS_EMPTY_8 "smbus-empty-8"
INIT_FUNC(SMBUS_EMPTY_8, SMBusEmptyDevice8)

#define TYPE_SMBUS_EMPTY_9 "smbus-empty-9"
INIT_FUNC(SMBUS_EMPTY_9, SMBusEmptyDevice9)

SMBusFunctionPtr getSMBusDeviceAddFunc(int device_type_id) {
    static SMBusFunctionPtr SMBusFunctions_temp[SMBUS_DEVICE_TOTAL_NUM] = {
        SMBusEmptyDevice0_add,
        SMBusEmptyDevice1_add,
        SMBusEmptyDevice2_add,
        SMBusEmptyDevice3_add,
        SMBusEmptyDevice4_add,
        SMBusEmptyDevice5_add,
        SMBusEmptyDevice6_add,
        SMBusEmptyDevice7_add,
        SMBusEmptyDevice8_add,
        SMBusEmptyDevice9_add
    };
    if ((device_type_id < SMBUS_DEVICE_TYPE_ID_OFFSET) || (device_type_id - SMBUS_DEVICE_TYPE_ID_OFFSET >= SMBUS_DEVICE_TOTAL_NUM)) {
        return NULL;
    } else {
        return SMBusFunctions_temp[device_type_id - SMBUS_DEVICE_TYPE_ID_OFFSET];
    }
}

int getSMBusDeviceTypeId(SMBusFunctionPtr functionPtr){
    static SMBusFunctionPtr FuncList[10] = {
            SMBusEmptyDevice0_add,
            SMBusEmptyDevice1_add,
            SMBusEmptyDevice2_add,
            SMBusEmptyDevice3_add,
            SMBusEmptyDevice4_add,
            SMBusEmptyDevice5_add,
            SMBusEmptyDevice6_add,
            SMBusEmptyDevice7_add,
            SMBusEmptyDevice8_add,
            SMBusEmptyDevice9_add
    };
    for (int i = 0; i < SMBUS_DEVICE_TOTAL_NUM; ++i) {
        if (FuncList[i] == functionPtr) {
            return SMBUS_DEVICE_TYPE_ID_OFFSET + i;
        }
    }
    return -1;
}

