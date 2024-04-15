#ifndef QEMU_I2C_MUX_CAD251X_H
#define QEMU_I2C_MUX_CAD251X_H

#include "hw/i2c/i2c.h"
#include "slib/inc/aspeed-init.h"

#define TYPE_CAD2512 "cad2512"
#define TYPE_CAD251X "cad251x"

#define CAD251X_NR_REGS 64
#define CAD251X_IN_CHANNEL_NUM 8
/*
 * CAD2512A08 Internal register.
 */
#define CAD2512_CONFIG_REG              0x00
#define CAD2512_INT_STS_REG             0x01
#define CAD2512_INT_MASK_REG            0x03
#define CAD2512_CON_RATE_REG            0x07
#define CAD2512_CHN_DISABLE_REG         0x08
#define CAD2512_ONE_SHOT_REG            0x09
#define CAD2512_DEEP_SHUTDOWN_REG       0x0A
#define CAD2512_ADV_CONFIG_REG          0x0B
#define CAD2512_BUSY_STS_REG            0x0C
#define CAD2512_CHN_READING_IN0_REG     0x20
#define CAD2512_CHN_READING_IN1_REG     0x21
#define CAD2512_CHN_READING_IN2_REG     0x22
#define CAD2512_CHN_READING_IN3_REG     0x23
#define CAD2512_CHN_READING_IN4_REG     0x24
#define CAD2512_CHN_READING_IN5_REG     0x25
#define CAD2512_CHN_READING_IN6_REG     0x26
#define CAD2512_CHN_READING_IN7_REG     0x27
#define CAD2512_LIMIT_IN0_HIGH_REG      0x2A
#define CAD2512_LIMIT_IN0_LOW_REG       0x2B
#define CAD2512_LIMIT_IN1_HIGH_REG      0x2C
#define CAD2512_LIMIT_IN1_LOW_REG       0x2D
#define CAD2512_LIMIT_IN2_HIGH_REG      0x2E
#define CAD2512_LIMIT_IN2_LOW_REG       0x2F
#define CAD2512_LIMIT_IN3_HIGH_REG      0x30
#define CAD2512_LIMIT_IN3_LOW_REG       0x31
#define CAD2512_LIMIT_IN4_HIGH_REG      0x32
#define CAD2512_LIMIT_IN4_LOW_REG       0x33
#define CAD2512_LIMIT_IN5_HIGH_REG      0x34
#define CAD2512_LIMIT_IN5_LOW_REG       0x35
#define CAD2512_LIMIT_IN6_HIGH_REG      0x36
#define CAD2512_LIMIT_IN6_LOW_REG       0x37
#define CAD2512_LIMIT_IN7_HIGH_REG      0x38
#define CAD2512_LIMIT_IN7_LOW_REG       0x39
#define CAD2512_MFR_ID_REG              0x3E
#define CAD2512_REV_ID_REG              0x3F

struct CAD251xState {
    /*< private >*/
    I2CSlave i2c;
    /*< public >*/

    uint8_t len;
    uint8_t pointer;

    uint8_t regs[CAD251X_NR_REGS];
    char *description; /* For debugging purpose only */
};

struct CAD251xClass {
    /*< private >*/
    I2CSlaveClass parent_class;
    /*< public >*/

    uint8_t pin_count;
    uint8_t max_reg;
};

typedef struct CAD251xState CAD251xState;
DECLARE_INSTANCE_CHECKER(CAD251xState, CAD251X,
                         TYPE_CAD251X)

typedef struct CAD251xClass CAD251xClass;
DECLARE_CLASS_CHECKERS(CAD251xClass, CAD251X,
                       TYPE_CAD251X)

void adc_device_add2(void *obj_0, PTR_DEVICE_CONFIG ptrDeviceConfig);
#endif
