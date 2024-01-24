/*
 * ASPEED PWM Controller
 *
 * Copyright (C) 2017-2021 IBM Corp.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "hw/misc/aspeed_pwm.h"
#include "qapi/error.h"
#include "migration/vmstate.h"
#include "trace.h"

#include "slib/inc/aspeed-init.h"
#include "slib/inc/pwm-device.h"

#define TO_REG(addr) (addr >> 2)

#define GENERAL_CONTROL                 TO_REG(0x00)
#define CLOCK_CONTROL                   TO_REG(0x04)
#define DUTY_CONTROL_0                  TO_REG(0x08)
#define DUTY_CONTROL_1                  TO_REG(0x0C)
#define TYPE_M_CONTROL_0                TO_REG(0x10)
#define TYPE_M_CONTROL_1                TO_REG(0x14)
#define TYPE_N_CONTROL_0                TO_REG(0x18)
#define TYPE_N_CONTROL_1                TO_REG(0x1C)
#define TACH_SOURCE                     TO_REG(0x20)
#define TRIGGER                         TO_REG(0x28)
#define RESULT                          TO_REG(0x2C)
#define INTERRUPT_CONTROL               TO_REG(0x30)
#define INTERRUPT_STATUS                TO_REG(0x34)
#define TYPE_M_LIMIT                    TO_REG(0x38)
#define TYPE_N_LIMIT                    TO_REG(0x3C)
#define GENERAL_CONTROL_EXTENSION_1     TO_REG(0x40)
#define CLOCK_CONTROL_EXTENSION_1       TO_REG(0x44)
#define DUTY_CONTROL_2                  TO_REG(0x48)
#define DUTY_CONTROL_3                  TO_REG(0x4C)
#define TYPE_O_CONTROL_0                TO_REG(0x50)
#define TYPE_O_CONTROL_1                TO_REG(0x54)
#define TACH_SOURCE_EXTENSION_1         TO_REG(0x60)
#define TYPE_O_LIMIT                    TO_REG(0x78)

#define DEVIATION_RATE (5)


static double get_rpm_from_duty(float duty, uint8_t fan_loc)
{
    PTR_RPM_DUTY ptrRpmDuty;
    double ret = 0;

    if (fan_loc < 8) {
        ptrRpmDuty = &gRpmDuty[fan_loc];
    } else {
        return 0;
    }

    if (duty <= ptrRpmDuty->min_offset) {
        ret = ptrRpmDuty->min_rpm;
    } else {
        ret = ptrRpmDuty->min_rpm +
              (duty - ptrRpmDuty->min_offset) / (ptrRpmDuty->max_offset - ptrRpmDuty->min_offset) *
              (ptrRpmDuty->max_rpm - ptrRpmDuty->min_rpm);
    }
    if (ptrRpmDuty->rand_deviation_rate > 0) {
        double a = rand() % (ptrRpmDuty->rand_deviation_rate * 2 * 100) / 100.0;
        ret = ret * (1 - ptrRpmDuty->rand_deviation_rate/100.0 + a/100.0);
    }
    return ret;
}

static uint64_t aspeed_pwm_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    AspeedPWMState *s = ASPEED_PWM(opaque);
    uint64_t val = 0;

    addr >>= 2;

    if (addr >= ASPEED_PWM_NR_REGS) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Out-of-bounds read at offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr << 2);
    } else {
        val = s->regs[addr];
    }

    trace_aspeed_pwm_read(addr << 2, val);

    return val;
}

static void aspeed_pwm_write(void *opaque, hwaddr addr, uint64_t data,
                              unsigned int size)
{
    AspeedPWMState *s = ASPEED_PWM(opaque);

    trace_aspeed_pwm_write(addr, data);

    addr >>= 2;

    if (addr >= ASPEED_PWM_NR_REGS) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Out-of-bounds write at offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr << 2);
        return;
    }
    switch (addr) {
        case TRIGGER:
//            printf(">>> old reg trigger: %08x \n", s->regs[TRIGGER]);
//            printf(">>> write reg trigger: %08lx \n", data);
            for (int i = 0; i < 15; ++i) {
                if ((s->regs[TRIGGER] & (1 << i)) == 0) {
                    /* 原来值是 0 */
                    if (data & (1 << i)) {
                        /* 将要更改为 1 */
                        /* 0 ==> 1 , 意味着触发了 转速计量 ，应该根据对应的 PWM 改变 TACH 值 */
                        if (i < 8) {
                            /* PWM 只有 8 个，默认 8 个 PWM 是 前 8 个 TACH 的输入源 */
                            /* 先获取 PWM 的周期 */
                            uint8_t period = (uint8_t) (s->regs[CLOCK_CONTROL] >> 8);
                            /* 获取 PWM 设置的 duty 数 */
                            uint8_t duty = 0;
                            if (i == 0 || i == 1) {
                                duty = s->regs[DUTY_CONTROL_0] >> 8;
                            } else if (i == 2 || i == 3) {
                                duty = s->regs[DUTY_CONTROL_0] >> 24;
                            } else if (i == 4 || i == 5) {
                                duty = s->regs[DUTY_CONTROL_1] >> 8;
                            } else if (i == 6 || i == 7) {
                                duty = s->regs[DUTY_CONTROL_1] >> 24;
                            }
                            /* 计算占空比 */
                            float duty_rate;
                            if (duty == 0) {
                                duty_rate = (float)1.0;
                            } else if (period != 0) {
                                duty_rate = (float)duty / (float)period;
                            } else {
                                duty_rate = 0;
                            }
                            /* 然后计算出应该的 RPM 值 */
                            double rpm = get_rpm_from_duty(duty_rate, i);
                            /* 之后，需要反向计算出 TACH 值 */
                            /* 先获取 tach clock division */
                            double fan_tach_clock_division;
                            fan_tach_clock_division = 1 << ((((s->regs[TYPE_M_CONTROL_0] >> 1) & 0b11) + 1) * 2);
                            uint32_t tacho_value = (uint32_t)(24000000 * 60 / rpm / 4 / fan_tach_clock_division);
                            /* 赋值 */
                            s->regs[RESULT] = tacho_value & ((1<<20) - 1);
                            s->regs[RESULT] |= 1 << 31;
//                            printf(">>> fan-%d : duty-%f , rpm-%f , tacho_value-%x , clock_division-%f \n", i,
//                                   duty_rate, rpm, tacho_value, fan_tach_clock_division);
                        }
                        break;
                    }
                }
            }
            break;
        default:
            break;
    }

    s->regs[addr] = data;
}

static const MemoryRegionOps aspeed_pwm_ops = {
    .read = aspeed_pwm_read,
    .write = aspeed_pwm_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

static void aspeed_pwm_reset(DeviceState *dev)
{
    struct AspeedPWMState *s = ASPEED_PWM(dev);

    memset(s->regs, 0, sizeof(s->regs));
    s->regs[0x2C>>2] = 0x80013023;
}

static void aspeed_pwm_realize(DeviceState *dev, Error **errp)
{
    AspeedPWMState *s = ASPEED_PWM(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    sysbus_init_irq(sbd, &s->irq);

    memory_region_init_io(&s->iomem, OBJECT(s), &aspeed_pwm_ops, s,
            TYPE_ASPEED_PWM, 0x1000);

    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription vmstate_aspeed_pwm = {
    .name = TYPE_ASPEED_PWM,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, AspeedPWMState, ASPEED_PWM_NR_REGS),
        VMSTATE_END_OF_LIST(),
    }
};

static void aspeed_pwm_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = aspeed_pwm_realize;
    dc->reset = aspeed_pwm_reset;
    dc->desc = "Aspeed PWM Controller";
    dc->vmsd = &vmstate_aspeed_pwm;
}

static const TypeInfo aspeed_pwm_info = {
    .name = TYPE_ASPEED_PWM,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AspeedPWMState),
    .class_init = aspeed_pwm_class_init,
};

static void aspeed_pwm_register_types(void)
{
    type_register_static(&aspeed_pwm_info);
}

type_init(aspeed_pwm_register_types);
