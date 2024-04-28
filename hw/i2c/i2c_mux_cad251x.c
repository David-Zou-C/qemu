/*
 * CAD2512A08 is a system monitor chip with I2C interface
 *
 *     Refer to the CAD2512A08_datasheet.pdf
 *
 * Copyright (c) 2024 zhanghong2 <505144104@.com>
 *
 * The CAD2512A08 include a local temperature sensor, 
 * 8-channel analog input.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/bitops.h"
#include "hw/qdev-properties.h"
#include "hw/irq.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "trace.h"
#include "qom/object.h"
#include "hw/i2c/i2c.h"
#include "hw/i2c/i2c_mux_cad251x.h"
#include "slib/inc/aspeed-init.h"
#include "hw/adc/aspeed_adc.h"

extern uint8_t adc_channel_disable_flag[CAD251X_IN_CHANNEL_NUM];

#if 0
static uint8_t cad251x_get_mode(CAD251xState *s)
{
    uint8_t reg = CAD2512_ADV_CONFIG_REG;
    uint8_t value = 0;

    value = extract8(s->regs[reg], 0, 1);

    return (value >> 1);
}
#endif

static uint8_t cad251x_get_channel_disable(CAD251xState *s, uint8_t channel)
{
    uint8_t reg = CAD2512_CHN_DISABLE_REG;
    uint8_t value = 0;

    if (channel >= CAD251X_IN_CHANNEL_NUM) {
        return 0;
    }

    value = extract8(s->regs[reg], 0, 8);

    return ((value & ((uint8_t)1 << channel)) >> channel) ;
}

static uint8_t cad251x_read(CAD251xState *s, uint8_t reg)
{
    uint8_t channel;
    switch (reg) {
    case CAD2512_CONFIG_REG:
    case CAD2512_INT_STS_REG:
    case CAD2512_INT_MASK_REG:
    case CAD2512_CON_RATE_REG:
    case CAD2512_CHN_DISABLE_REG:
    case CAD2512_DEEP_SHUTDOWN_REG:
    case CAD2512_ADV_CONFIG_REG:
    case CAD2512_BUSY_STS_REG:
    case CAD2512_LIMIT_IN0_HIGH_REG:
    case CAD2512_LIMIT_IN0_LOW_REG:
    case CAD2512_LIMIT_IN1_HIGH_REG:
    case CAD2512_LIMIT_IN1_LOW_REG:
    case CAD2512_LIMIT_IN2_HIGH_REG:
    case CAD2512_LIMIT_IN2_LOW_REG:
    case CAD2512_LIMIT_IN3_HIGH_REG:
    case CAD2512_LIMIT_IN3_LOW_REG:
    case CAD2512_LIMIT_IN4_HIGH_REG:
    case CAD2512_LIMIT_IN4_LOW_REG:
    case CAD2512_LIMIT_IN5_HIGH_REG:
    case CAD2512_LIMIT_IN5_LOW_REG:
    case CAD2512_LIMIT_IN6_HIGH_REG:
    case CAD2512_LIMIT_IN6_LOW_REG:
    case CAD2512_LIMIT_IN7_HIGH_REG:
    case CAD2512_LIMIT_IN7_LOW_REG:
    case CAD2512_MFR_ID_REG:
    case CAD2512_REV_ID_REG:
        return s->regs[reg];
    case CAD2512_CHN_READING_IN0_REG:
    case CAD2512_CHN_READING_IN1_REG:
    case CAD2512_CHN_READING_IN2_REG:
    case CAD2512_CHN_READING_IN3_REG:
    case CAD2512_CHN_READING_IN4_REG:
    case CAD2512_CHN_READING_IN5_REG:
    case CAD2512_CHN_READING_IN6_REG:
    case CAD2512_CHN_READING_IN7_REG:
        //Channel enabled
        channel = reg & 0xF;
        if(!cad251x_get_channel_disable(s, channel)) {
            adc_channel_disable_flag[channel] = 0;
            return s->regs[reg];
        }
        else {
            adc_channel_disable_flag[channel] = 1;
            return s->regs[reg];  //return last value
        }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: unexpected read to register %d\n",
                      __func__, reg);
        return 0xFF;
    }
}

static void cad251x_write(CAD251xState *s, uint8_t reg, uint8_t data)
{
    switch (reg) {
    case CAD2512_CONFIG_REG:
    case CAD2512_INT_MASK_REG:
    case CAD2512_CON_RATE_REG:
    case CAD2512_CHN_DISABLE_REG:
    case CAD2512_DEEP_SHUTDOWN_REG:
    case CAD2512_ADV_CONFIG_REG:
    case CAD2512_LIMIT_IN0_HIGH_REG:
    case CAD2512_LIMIT_IN0_LOW_REG:
    case CAD2512_LIMIT_IN1_HIGH_REG:
    case CAD2512_LIMIT_IN1_LOW_REG:
    case CAD2512_LIMIT_IN2_HIGH_REG:
    case CAD2512_LIMIT_IN2_LOW_REG:
    case CAD2512_LIMIT_IN3_HIGH_REG:
    case CAD2512_LIMIT_IN3_LOW_REG:
    case CAD2512_LIMIT_IN4_HIGH_REG:
    case CAD2512_LIMIT_IN4_LOW_REG:
    case CAD2512_LIMIT_IN5_HIGH_REG:
    case CAD2512_LIMIT_IN5_LOW_REG:
    case CAD2512_LIMIT_IN6_HIGH_REG:
    case CAD2512_LIMIT_IN6_LOW_REG:
    case CAD2512_LIMIT_IN7_HIGH_REG:
    case CAD2512_LIMIT_IN7_LOW_REG:
        s->regs[reg] = data;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: unexpected write to register %d\n",
                      __func__, reg);
    }
}

static uint8_t cad251x_recv(I2CSlave *i2c)
{
    CAD251xState *s = CAD251X(i2c);
    uint8_t ret;

    ret = cad251x_read(s, s->pointer & 0xf);

    return ret;
}

static int cad251x_send(I2CSlave *i2c, uint8_t data)
{
    CAD251xState *s = CAD251X(i2c);

    cad251x_write(s, s->pointer & 0xf, data);

    return 0;
}

static int cad251x_event(I2CSlave *i2c, enum i2c_event event)
{
    //CAD251xState *s = CAD251X(i2c);

    //s->len = 0;
    return 0;
}

static const VMStateDescription cad2512_vmstate = {
    .name = "CAD2512",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8(len, CAD251xState),
        VMSTATE_UINT8(pointer, CAD251xState),
        VMSTATE_UINT8_ARRAY(regs, CAD251xState, CAD251X_NR_REGS),
        VMSTATE_I2C_SLAVE(i2c, CAD251xState),
        VMSTATE_END_OF_LIST()
    }
};

static void cad2512_reset(DeviceState *dev)
{
    CAD251xState *s = CAD251X(dev);

    memset(s->regs, 0x00, CAD251X_NR_REGS);
    s->regs[CAD2512_CONFIG_REG] = 0x08;
    s->regs[CAD2512_BUSY_STS_REG] = 0x02;
    s->regs[CAD2512_MFR_ID_REG] = 0x59;
    s->regs[CAD2512_REV_ID_REG] = 0x01;

    s->pointer = 0xFF;
    s->len = 0;
}

static void cad251x_initfn(Object *obj)
{
    //CAD251xClass *k = CAD251X_GET_CLASS(obj);
}

static void cad251x_realize(DeviceState *dev, Error **errp)
{
    uint8_t index;
    CAD251xState *s = CAD251X(dev);
    if (!s->description) {
        s->description = g_strdup("cad-unspecified");
    }

    if (s->ptrDeviceConfig == NULL) {
        return;
    }

    index = device_add(CAD2512, s->ptrDeviceConfig->name, s, NULL);

    if (index > 0) {
        deviceAddList[index].ptrDeviceConfig = s->ptrDeviceConfig;
    }
}

static Property cad251x_properties[] = {
    DEFINE_PROP_STRING("description", CAD251xState, description),
    DEFINE_PROP_END_OF_LIST(),
};

static void cad251x_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);

    k->event = cad251x_event;
    k->recv = cad251x_recv;
    k->send = cad251x_send;
    dc->realize = cad251x_realize;
    device_class_set_props(dc, cad251x_properties);
}

static const TypeInfo cad251x_info = {
    .name          = TYPE_CAD251X,
    .parent        = TYPE_I2C_SLAVE,
    .instance_init = cad251x_initfn,
    .instance_size = sizeof(CAD251xState),
    .class_init    = cad251x_class_init,
    .class_size    = sizeof(CAD251xClass),
    .abstract      = true,
};

static void cad2512_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->reset = cad2512_reset;
    dc->vmsd = &cad2512_vmstate;
}

static const TypeInfo cad2512_info = {
    .name          = TYPE_CAD2512,
    .parent        = TYPE_CAD251X,
    .class_init    = cad2512_class_init,
};

static void cad251x_register_types(void)
{
    type_register_static(&cad251x_info);
    type_register_static(&cad2512_info);
}

type_init(cad251x_register_types)

void adc_device_add2(PTR_DEVICE_CONFIG ptrDeviceConfig) {
    PTR_ADC_DEVICE_DATA ptrAdcDeviceData = (PTR_ADC_DEVICE_DATA) malloc(sizeof(ADC_DEVICE_DATA));
    memset(ptrAdcDeviceData, 0, sizeof(ADC_DEVICE_DATA));

    ptrAdcDeviceData->ptrDeviceConfig = ptrDeviceConfig;
    assert(ptrAdcDeviceData->ptrDeviceConfig->adc_channel <= 7);

    ptrAdcDeviceData->adcRegType = REG_EXTERNAL;
    uint32_t reg_addr = CAD2512_CHN_READING_IN0_REG + ptrAdcDeviceData->ptrDeviceConfig->adc_channel;

    int index = get_cad2512_device_index();
    CAD251xState *s = (CAD251xState *)(deviceAddList[index].cad251x);
    ptrAdcDeviceData->ptrExtAdcReg = (uint8_t *)&(s->regs[reg_addr]);
    ptrAdcDeviceData->division = (uint32_t )(ptrAdcDeviceData->ptrDeviceConfig->division * 1000);

    init_ADCDevice0(ptrAdcDeviceData);
    device_add(ADC, ptrDeviceConfig->name, ptrAdcDeviceData, NULL);
}

void cad_device_add(I2CBus *bus, PTR_DEVICE_CONFIG ptrDeviceConfig)
{
    file_log("cad_device_add", LOG_TIME_END);
    DeviceState *dev;

    if (ptrDeviceConfig->device_type_id == CAD2512) {
        dev = qdev_new(TYPE_CAD2512);
    } else {
        printf("device add error: not pca device ! \n");
        exit(1);
    }

    qdev_prop_set_uint8(dev, "address", ptrDeviceConfig->addr);

    CAD251xState *s;
    s = CAD251X(dev);
    s->ptrDeviceConfig = ptrDeviceConfig;

    qdev_realize_and_unref(dev, (BusState *)bus, &error_fatal);
    printf("external adc device added ! \n");
}