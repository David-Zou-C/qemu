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

#define PWM0_GENERAL_REG                TO_REG(0x000)
#define PWM0_DUTY_CYC_REG               TO_REG(0x004)
#define TACH0_GENERAL_REG               TO_REG(0x008)
#define TACH0_STS_REG                   TO_REG(0x00C)
#define PWM1_GENERAL_REG                TO_REG(0x010)
#define PWM1_DUTY_CYC_REG               TO_REG(0x014)
#define TACH1_GENERAL_REG               TO_REG(0x018)
#define TACH1_STS_REG                   TO_REG(0x01C)
#define PWM2_GENERAL_REG                TO_REG(0x020)
#define PWM2_DUTY_CYC_REG               TO_REG(0x024)
#define TACH2_GENERAL_REG               TO_REG(0x028)
#define TACH2_STS_REG                   TO_REG(0x02C)
#define PWM3_GENERAL_REG                TO_REG(0x030)
#define PWM3_DUTY_CYC_REG               TO_REG(0x034)
#define TACH3_GENERAL_REG               TO_REG(0x038)
#define TACH3_STS_REG                   TO_REG(0x03C)
#define PWM4_GENERAL_REG                TO_REG(0x040)
#define PWM4_DUTY_CYC_REG               TO_REG(0x044)
#define TACH4_GENERAL_REG               TO_REG(0x048)
#define TACH4_STS_REG                   TO_REG(0x04C)
#define PWM5_GENERAL_REG                TO_REG(0x050)
#define PWM5_DUTY_CYC_REG               TO_REG(0x054)
#define TACH5_GENERAL_REG               TO_REG(0x058)
#define TACH5_STS_REG                   TO_REG(0x05C)
#define PWM6_GENERAL_REG                TO_REG(0x060)
#define PWM6_DUTY_CYC_REG               TO_REG(0x064)
#define TACH6_GENERAL_REG               TO_REG(0x068)
#define TACH6_STS_REG                   TO_REG(0x06C)
#define PWM7_GENERAL_REG                TO_REG(0x070)
#define PWM7_DUTY_CYC_REG               TO_REG(0x074)
#define TACH7_GENERAL_REG               TO_REG(0x078)
#define TACH7_STS_REG                   TO_REG(0x07C)
#define PWM8_GENERAL_REG                TO_REG(0x080)
#define PWM8_DUTY_CYC_REG               TO_REG(0x084)
#define TACH8_GENERAL_REG               TO_REG(0x088)
#define TACH8_STS_REG                   TO_REG(0x08C)
#define PWM9_GENERAL_REG                TO_REG(0x090)
#define PWM9_DUTY_CYC_REG               TO_REG(0x094)
#define TACH9_GENERAL_REG               TO_REG(0x098)
#define TACH9_STS_REG                   TO_REG(0x09C)
#define PWMA_GENERAL_REG                TO_REG(0x0A0)
#define PWMA_DUTY_CYC_REG               TO_REG(0x0A4)
#define TACHA_GENERAL_REG               TO_REG(0x0A8)
#define TACHA_STS_REG                   TO_REG(0x0AC)
#define PWMB_GENERAL_REG                TO_REG(0x0B0)
#define PWMB_DUTY_CYC_REG               TO_REG(0x0B4)
#define TACHB_GENERAL_REG               TO_REG(0x0B8)
#define TACHB_STS_REG                   TO_REG(0x0BC)
#define PWMC_GENERAL_REG                TO_REG(0x0C0)
#define PWMC_DUTY_CYC_REG               TO_REG(0x0C4)
#define TACHC_GENERAL_REG               TO_REG(0x0C8)
#define TACHC_STS_REG                   TO_REG(0x0CC)
#define PWMD_GENERAL_REG                TO_REG(0x0D0)
#define PWMD_DUTY_CYC_REG               TO_REG(0x0D4)
#define TACHD_GENERAL_REG               TO_REG(0x0D8)
#define TACHD_STS_REG                   TO_REG(0x0DC)
#define PWME_GENERAL_REG                TO_REG(0x0E0)
#define PWME_DUTY_CYC_REG               TO_REG(0x0E4)
#define TACHE_GENERAL_REG               TO_REG(0x0E8)
#define TACHE_STS_REG                   TO_REG(0x0EC)
#define PWMF_GENERAL_REG                TO_REG(0x0F0)
#define PWMF_DUTY_CYC_REG               TO_REG(0x0F4)
#define TACHF_GENERAL_REG               TO_REG(0x0F8)
#define TACHF_STS_REG                   TO_REG(0x0FC)
#define PWM_G100_REG                    TO_REG(0x100)
#define PWM_G104_REG                    TO_REG(0x104)
#define PWM_G108_REG                    TO_REG(0x108)
#define TACH_STS_REG                    TO_REG(0x10C)

#define DEVIATION_RATE (5)


static double get_rpm_from_duty(float duty, uint8_t fan_loc)
{
    PTR_RPM_DUTY ptrRpmDuty;
    double ret = 0;

    if (fan_loc < 16) {
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
    if (ptrRpmDuty->rand_deviation_rate > 0 && ptrRpmDuty->rand_deviation_rate < 100) {
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
//    printf("aspeed_pwm_read trigger-addr:%lx,value:%lx\n", temp, val);
    trace_aspeed_pwm_read(addr << 2, val);

    return val;
}

static void aspeed_pwm_write(void *opaque, hwaddr addr, uint64_t data,
                              unsigned int size)
{
    AspeedPWMState *s = ASPEED_PWM(opaque);
    uint8_t tach_enable = 0;
    uint8_t pwm_channel = 0;
    hwaddr offset = addr;
    uint8_t pulsepr = 2;
    uint64_t ahb_clk = 200000000;
    trace_aspeed_pwm_write(addr, data);
//    printf("aspeed_pwm_write trigger-addr:%lx %lx\n", addr, data);

    addr >>= 2;

    if (addr >= ASPEED_PWM_NR_REGS) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Out-of-bounds write at offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr << 2);
        return;
    }

    switch (addr) {
        case TACH0_GENERAL_REG:
        case TACH1_GENERAL_REG:
        case TACH2_GENERAL_REG:
        case TACH3_GENERAL_REG:
        case TACH4_GENERAL_REG:
        case TACH5_GENERAL_REG:
        case TACH6_GENERAL_REG:
        case TACH7_GENERAL_REG:
        case TACH8_GENERAL_REG:
        case TACH9_GENERAL_REG:
        case TACHA_GENERAL_REG:
        case TACHB_GENERAL_REG:
        case TACHC_GENERAL_REG:
        case TACHD_GENERAL_REG:
        case TACHE_GENERAL_REG:
        case TACHF_GENERAL_REG:
            pwm_channel = (uint8_t)((offset >> 4) & 0xf);
            tach_enable = (uint8_t)((s->regs[addr] & (1 << 28)) >> 28);
//            printf("channel:%d, old tach_enable:%d\n", pwm_channel, tach_enable);
            //TACH Enable Bit有变化
            if (tach_enable != ((uint8_t)(data >> 28))){
                s->regs[addr] = data;
                tach_enable = (uint8_t)((s->regs[addr] & (1 << 28)) >> 28);
//                printf("new tach_enable:%d\n", tach_enable);
                /* 0 ==> 1 , 意味着触发了 转速计量 ，应该根据对应的 PWM 改变 TACH 值 */
                if (tach_enable == 1){
                    /* 先获取 PWM 的周期 */
                    uint8_t period = (uint8_t) (s->regs[PWM0_DUTY_CYC_REG + ((pwm_channel * 16) >> 2)] >> 24);
                    /* 获取 PWM 设置的 duty 数 */
                    uint8_t duty = 0;
                    duty = (uint8_t)(s->regs[PWM0_DUTY_CYC_REG + ((pwm_channel * 16) >> 2)] >> 8);
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
                    double rpm = get_rpm_from_duty(duty_rate, pwm_channel);
                    /* 之后，需要反向计算出 TACH 值 */
                    /* 先获取 tach clock division */
                    double fan_tach_clock_division;
                    fan_tach_clock_division = 1 << (((s->regs[addr] >> 20) & 0b111) * 2);
                    uint32_t tacho_value = (uint32_t)((ahb_clk * 60 / rpm / pulsepr / fan_tach_clock_division) - 1);
//                    printf("duty:%d,period:%d,duty_rate:%f,rpm:%2f,devide:%2f,tacho_value:%x\n", duty, period, duty_rate, rpm, fan_tach_clock_division, tacho_value);
                    /* 赋值 */
                    s->regs[TACH0_STS_REG + ((pwm_channel * 16) >> 2)] = tacho_value & ((1 << 20) - 1);
                    s->regs[TACH0_STS_REG + ((pwm_channel * 16) >> 2)] |= (0b11 << 20);
//                    printf("TACH_STS_REG:%x\n", s->regs[TACH0_STS_REG + ((pwm_channel * 16) >> 2)]);
                    //printf(">>> fan-%d : duty-%f , rpm-%f , tacho_value-%x , clock_division-%f \n", i,
                    //duty_rate, rpm, tacho_value, fan_tach_clock_division);
                }
            }
            //printf(">>> old reg trigger: %08x \n", s->regs[TRIGGER]);
            //printf(">>> write reg trigger: %08lx \n", data);
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
