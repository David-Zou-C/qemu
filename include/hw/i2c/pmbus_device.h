/*
 * QEMU PMBus device emulation
 *
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_PMBUS_DEVICE_H
#define HW_PMBUS_DEVICE_H

#include "qemu/bitops.h"
#include "hw/i2c/smbus_slave.h"
#include "slib/inc/aspeed-init.h"

/* STATUS_WORD */
#define PB_STATUS_VOUT           BIT(15)
#define PB_STATUS_IOUT_POUT      BIT(14)
#define PB_STATUS_INPUT          BIT(13)
#define PB_STATUS_WORD_MFR       BIT(12)
#define PB_STATUS_POWER_GOOD_N   BIT(11)
#define PB_STATUS_FAN            BIT(10)
#define PB_STATUS_OTHER          BIT(9)
#define PB_STATUS_UNKNOWN        BIT(8)
/* STATUS_BYTE */
#define PB_STATUS_BUSY           BIT(7)
#define PB_STATUS_OFF            BIT(6)
#define PB_STATUS_VOUT_OV        BIT(5)
#define PB_STATUS_IOUT_OC        BIT(4)
#define PB_STATUS_VIN_UV         BIT(3)
#define PB_STATUS_TEMPERATURE    BIT(2)
#define PB_STATUS_CML            BIT(1)
#define PB_STATUS_NONE_ABOVE     BIT(0)

/* STATUS_VOUT */
#define PB_STATUS_VOUT_OV_FAULT         BIT(7) /* Output Overvoltage Fault */
#define PB_STATUS_VOUT_OV_WARN          BIT(6) /* Output Overvoltage Warning */
#define PB_STATUS_VOUT_UV_WARN          BIT(5) /* Output Undervoltage Warning */
#define PB_STATUS_VOUT_UV_FAULT         BIT(4) /* Output Undervoltage Fault */
#define PB_STATUS_VOUT_MAX              BIT(3)
#define PB_STATUS_VOUT_TON_MAX_FAULT    BIT(2)
#define PB_STATUS_VOUT_TOFF_MAX_WARN    BIT(1)

/* STATUS_IOUT */
#define PB_STATUS_IOUT_OC_FAULT    BIT(7) /* Output Overcurrent Fault */
#define PB_STATUS_IOUT_OC_LV_FAULT BIT(6) /* Output OC And Low Voltage Fault */
#define PB_STATUS_IOUT_OC_WARN     BIT(5) /* Output Overcurrent Warning */
#define PB_STATUS_IOUT_UC_FAULT    BIT(4) /* Output Undercurrent Fault */
#define PB_STATUS_CURR_SHARE       BIT(3) /* Current Share Fault */
#define PB_STATUS_PWR_LIM_MODE     BIT(2) /* In Power Limiting Mode */
#define PB_STATUS_POUT_OP_FAULT    BIT(1) /* Output Overpower Fault */
#define PB_STATUS_POUT_OP_WARN     BIT(0) /* Output Overpower Warning */

/* STATUS_INPUT */
#define PB_STATUS_INPUT_VIN_OV_FAULT    BIT(7) /* Input Overvoltage Fault */
#define PB_STATUS_INPUT_VIN_OV_WARN     BIT(6) /* Input Overvoltage Warning */
#define PB_STATUS_INPUT_VIN_UV_WARN     BIT(5) /* Input Undervoltage Warning */
#define PB_STATUS_INPUT_VIN_UV_FAULT    BIT(4) /* Input Undervoltage Fault */
#define PB_STATUS_INPUT_IIN_OC_FAULT    BIT(2) /* Input Overcurrent Fault */
#define PB_STATUS_INPUT_IIN_OC_WARN     BIT(1) /* Input Overcurrent Warning */
#define PB_STATUS_INPUT_PIN_OP_WARN     BIT(0) /* Input Overpower Warning */

/* STATUS_TEMPERATURE */
#define PB_STATUS_OT_FAULT              BIT(7) /* Overtemperature Fault */
#define PB_STATUS_OT_WARN               BIT(6) /* Overtemperature Warning */
#define PB_STATUS_UT_WARN               BIT(5) /* Undertemperature Warning */
#define PB_STATUS_UT_FAULT              BIT(4) /* Undertemperature Fault */

/* STATUS_CML */
#define PB_CML_FAULT_INVALID_CMD     BIT(7) /* Invalid/Unsupported Command */
#define PB_CML_FAULT_INVALID_DATA    BIT(6) /* Invalid/Unsupported Data  */
#define PB_CML_FAULT_PEC             BIT(5) /* Packet Error Check Failed */
#define PB_CML_FAULT_MEMORY          BIT(4) /* Memory Fault Detected */
#define PB_CML_FAULT_PROCESSOR       BIT(3) /* Processor Fault Detected */
#define PB_CML_FAULT_OTHER_COMM      BIT(1) /* Other communication fault */
#define PB_CML_FAULT_OTHER_MEM_LOGIC BIT(0) /* Other Memory Or Logic Fault */

/* OPERATION*/
#define PB_OP_ON                BIT(7) /* PSU is switched on */
#define PB_OP_MARGIN_HIGH       BIT(5) /* PSU vout is set to margin high */
#define PB_OP_MARGIN_LOW        BIT(4) /* PSU vout is set to margin low */

/* PAGES */
#define PB_MAX_PAGES            0x1F
#define PB_ALL_PAGES            0xFF

#define PMBUS_ERR_BYTE          0xFF

#define TYPE_PMBUS_DEVICE "pmbus-device"
OBJECT_DECLARE_TYPE(PMBusDevice, PMBusDeviceClass,
                    PMBUS_DEVICE)

/* flags */
#define PB_HAS_COEFFICIENTS        BIT_ULL(9)
#define PB_HAS_VIN                 BIT_ULL(10)
#define PB_HAS_VOUT                BIT_ULL(11)
#define PB_HAS_VOUT_MARGIN         BIT_ULL(12)
#define PB_HAS_VIN_RATING          BIT_ULL(13)
#define PB_HAS_VOUT_RATING         BIT_ULL(14)
#define PB_HAS_VOUT_MODE           BIT_ULL(15)
#define PB_HAS_VCAP                BIT_ULL(16)
#define PB_HAS_IOUT                BIT_ULL(21)
#define PB_HAS_IIN                 BIT_ULL(22)
#define PB_HAS_IOUT_RATING         BIT_ULL(23)
#define PB_HAS_IIN_RATING          BIT_ULL(24)
#define PB_HAS_IOUT_GAIN           BIT_ULL(25)
#define PB_HAS_POUT                BIT_ULL(30)
#define PB_HAS_PIN                 BIT_ULL(31)
#define PB_HAS_EIN                 BIT_ULL(32)
#define PB_HAS_EOUT                BIT_ULL(33)
#define PB_HAS_POUT_RATING         BIT_ULL(34)
#define PB_HAS_PIN_RATING          BIT_ULL(35)
#define PB_HAS_TEMPERATURE         BIT_ULL(40)
#define PB_HAS_TEMP2               BIT_ULL(41)
#define PB_HAS_TEMP3               BIT_ULL(42)
#define PB_HAS_TEMP_RATING         BIT_ULL(43)
#define PB_HAS_FAN                 BIT_ULL(44)
#define PB_HAS_MFR_INFO            BIT_ULL(50)
#define PB_HAS_STATUS_MFR_SPECIFIC BIT_ULL(51)

struct PMBusDeviceClass {
    SMBusDeviceClass parent_class;
    uint8_t device_num_pages;

    /**
     * Implement quick_cmd, receive byte, and write_data to support non-standard
     * PMBus functionality
     */
    void (*quick_cmd)(PMBusDevice *dev, uint8_t read);
    int (*write_data)(PMBusDevice *dev, const uint8_t *buf, uint8_t len);
    uint8_t (*receive_byte)(PMBusDevice *dev);
};

/* State */
struct PMBusDevice {
    SMBusDevice smb;

    uint8_t num_pages;
    uint8_t code;
    uint8_t page;

    /*
     * PMBus registers are stored in a PMBusPage structure allocated by
     * calling pmbus_pages_alloc()
     */
    PMBusPage *pages;
    uint8_t capability;


    int32_t in_buf_len;
    uint8_t *in_buf;
    int32_t out_buf_len;
    uint8_t out_buf[SMBUS_DATA_MAX_LEN];

    /* 添加 JSON 文件中的配置信息描述 */
    PTR_DEVICE_CONFIG ptrDeviceConfig;
};

/**
 * Direct mode coefficients
 * @var m - mantissa
 * @var b - offset
 * @var R - exponent
 */
typedef struct PMBusCoefficients {
    int32_t m;     /* mantissa */
    int64_t b;     /* offset */
    int32_t R;     /* exponent */
} PMBusCoefficients;

/**
 * VOUT_Mode bit fields
 */
typedef struct PMBusVoutMode {
    uint8_t  mode:3;
    int8_t   exp:5;
} PMBusVoutMode;

/**
 * Convert sensor values to direct mode format
 *
 * Y = (m * x - b) * 10^R
 *
 * @return uint16_t
 */
uint16_t pmbus_data2direct_mode(PMBusCoefficients c, uint32_t value);

/**
 * Convert direct mode formatted data into sensor reading
 *
 * X = (Y * 10^-R - b) / m
 *
 * @return uint32_t
 */
uint32_t pmbus_direct_mode2data(PMBusCoefficients c, uint16_t value);

/**
 * Convert sensor values to linear mode format
 *
 * L = D * 2^(-e)
 *
 * @return uint16
 */
uint16_t pmbus_data2linear_mode(uint16_t value, int exp);

/**
 * Convert linear mode formatted data into sensor reading
 *
 * D = L * 2^e
 *
 * @return uint16
 */
uint16_t pmbus_linear_mode2data(uint16_t value, int exp);

/**
 * @brief Send a block of data over PMBus
 * Assumes that the bytes in the block are already ordered correctly,
 * also assumes the length has been prepended to the block if necessary
 *     | low_byte | ... | high_byte |
 * @param state - maintains state of the PMBus device
 * @param data - byte array to be sent by device
 * @param len - number
 */
void pmbus_send(PMBusDevice *state, const uint8_t *data, uint16_t len);
void pmbus_send8(PMBusDevice *state, uint8_t data);
void pmbus_send16(PMBusDevice *state, uint16_t data);
void pmbus_send32(PMBusDevice *state, uint32_t data);
void pmbus_send64(PMBusDevice *state, uint64_t data);

/**
 * @brief Send a string over PMBus with length prepended.
 * Length is calculated using str_len()
 */
void pmbus_send_string(PMBusDevice *state, const char *data);

/**
 * @brief Receive data sent with Block Write.
 * @param dest - memory with enough capacity to receive the write
 * @param len - the capacity of dest
 */
uint8_t pmbus_receive_block(PMBusDevice *pmdev, uint8_t *dest, size_t len);

/**
 * @brief Receive data over PMBus
 * These methods help track how much data is being received over PMBus
 * Log to GUEST_ERROR if too much or too little is sent.
 */
uint8_t pmbus_receive8(PMBusDevice *pmdev);
uint16_t pmbus_receive16(PMBusDevice *pmdev);
uint32_t pmbus_receive32(PMBusDevice *pmdev);
uint64_t pmbus_receive64(PMBusDevice *pmdev);

/**
 * PMBus page config must be called before any page is first used.
 * It will allocate memory for all the pages if needed.
 * Passed in flags overwrite existing flags if any.
 * @param page_index the page to which the flags are applied, setting page_index
 * to 0xFF applies the passed in flags to all pages.
 * @param flags
 */
int pmbus_page_config(PMBusDevice *pmdev, uint8_t page_index, uint64_t flags);

/**
 * Update the status registers when sensor values change.
 * Useful if modifying sensors through qmp, this way status registers get
 * updated
 */
void pmbus_check_limits(PMBusDevice *pmdev);

/**
 * Enter an idle state where only the PMBUS_ERR_BYTE will be returned
 * indefinitely until a new command is issued.
 */
void pmbus_idle(PMBusDevice *pmdev);

extern const VMStateDescription vmstate_pmbus_device;

#define VMSTATE_PMBUS_DEVICE(_field, _state) {                       \
    .name       = (stringify(_field)),                               \
    .size       = sizeof(PMBusDevice),                               \
    .vmsd       = &vmstate_pmbus_device,                             \
    .flags      = VMS_STRUCT,                                        \
    .offset     = vmstate_offset_value(_state, _field, PMBusDevice), \
}

#endif
