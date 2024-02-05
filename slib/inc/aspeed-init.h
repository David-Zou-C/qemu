//
// Created by 25231 on 2023/8/2.
//

#ifndef QEMU_ASPEED_INIT_H
#define QEMU_ASPEED_INIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#include "slib-common.h"
#include "smbus-device.h"
#include "i2c-device.h"
#include "gpio-device.h"
#include "adc-device.h"
#include "pwm-device.h"
#include "dataThread.h"
#include "i3c-device.h"

#define sDEBUG 1

#define FUNC_DEBUG(message) file_log(message, LOG_TIME_END);

#define CONFIG_FILE "config_ast2600_G50.json"

#define LOG_FILE "log.txt"

#define LOG_NOTHING 0
#define LOG_END 1
#define LOG_TIME 2
#define LOG_TIME_END 3

#define DETACH_INITIAL_DATA 0
#define DETACH_CTRL_DATA 1


enum pmbus_registers {
    PMBUS_PAGE                      = 0x00, /* R/W byte */
    PMBUS_OPERATION                 = 0x01, /* R/W byte */
    PMBUS_ON_OFF_CONFIG             = 0x02, /* R/W byte */
    PMBUS_CLEAR_FAULTS              = 0x03, /* Send Byte */
    PMBUS_PHASE                     = 0x04, /* R/W byte */
    PMBUS_PAGE_PLUS_WRITE           = 0x05, /* Block Write-only */
    PMBUS_PAGE_PLUS_READ            = 0x06, /* Block Read-only */
    PMBUS_WRITE_PROTECT             = 0x10, /* R/W byte */
    PMBUS_STORE_DEFAULT_ALL         = 0x11, /* Send Byte */
    PMBUS_RESTORE_DEFAULT_ALL       = 0x12, /* Send Byte */
    PMBUS_STORE_DEFAULT_CODE        = 0x13, /* Write-only Byte */
    PMBUS_RESTORE_DEFAULT_CODE      = 0x14, /* Write-only Byte */
    PMBUS_STORE_USER_ALL            = 0x15, /* Send Byte */
    PMBUS_RESTORE_USER_ALL          = 0x16, /* Send Byte */
    PMBUS_STORE_USER_CODE           = 0x17, /* Write-only Byte */
    PMBUS_RESTORE_USER_CODE         = 0x18, /* Write-only Byte */
    PMBUS_CAPABILITY                = 0x19, /* Read-Only byte */
    PMBUS_QUERY                     = 0x1A, /* Write-Only */
    PMBUS_SMBALERT_MASK             = 0x1B, /* Block read, Word write */
    PMBUS_VOUT_MODE                 = 0x20, /* R/W byte */
    PMBUS_VOUT_COMMAND              = 0x21, /* R/W word */
    PMBUS_VOUT_TRIM                 = 0x22, /* R/W word */
    PMBUS_VOUT_CAL_OFFSET           = 0x23, /* R/W word */
    PMBUS_VOUT_MAX                  = 0x24, /* R/W word */
    PMBUS_VOUT_MARGIN_HIGH          = 0x25, /* R/W word */
    PMBUS_VOUT_MARGIN_LOW           = 0x26, /* R/W word */
    PMBUS_VOUT_TRANSITION_RATE      = 0x27, /* R/W word */
    PMBUS_VOUT_DROOP                = 0x28, /* R/W word */
    PMBUS_VOUT_SCALE_LOOP           = 0x29, /* R/W word */
    PMBUS_VOUT_SCALE_MONITOR        = 0x2A, /* R/W word */
    PMBUS_VOUT_MIN                  = 0x2B, /* R/W word */
    PMBUS_COEFFICIENTS              = 0x30, /* Read-only block 5 bytes */
    PMBUS_POUT_MAX                  = 0x31, /* R/W word */
    PMBUS_MAX_DUTY                  = 0x32, /* R/W word */
    PMBUS_FREQUENCY_SWITCH          = 0x33, /* R/W word */
    PMBUS_VIN_ON                    = 0x35, /* R/W word */
    PMBUS_VIN_OFF                   = 0x36, /* R/W word */
    PMBUS_INTERLEAVE                = 0x37, /* R/W word */
    PMBUS_IOUT_CAL_GAIN             = 0x38, /* R/W word */
    PMBUS_IOUT_CAL_OFFSET           = 0x39, /* R/W word */
    PMBUS_FAN_CONFIG_1_2            = 0x3A, /* R/W byte */
    PMBUS_FAN_COMMAND_1             = 0x3B, /* R/W word */
    PMBUS_FAN_COMMAND_2             = 0x3C, /* R/W word */
    PMBUS_FAN_CONFIG_3_4            = 0x3D, /* R/W byte */
    PMBUS_FAN_COMMAND_3             = 0x3E, /* R/W word */
    PMBUS_FAN_COMMAND_4             = 0x3F, /* R/W word */
    PMBUS_VOUT_OV_FAULT_LIMIT       = 0x40, /* R/W word */
    PMBUS_VOUT_OV_FAULT_RESPONSE    = 0x41, /* R/W byte */
    PMBUS_VOUT_OV_WARN_LIMIT        = 0x42, /* R/W word */
    PMBUS_VOUT_UV_WARN_LIMIT        = 0x43, /* R/W word */
    PMBUS_VOUT_UV_FAULT_LIMIT       = 0x44, /* R/W word */
    PMBUS_VOUT_UV_FAULT_RESPONSE    = 0x45, /* R/W byte */
    PMBUS_IOUT_OC_FAULT_LIMIT       = 0x46, /* R/W word */
    PMBUS_IOUT_OC_FAULT_RESPONSE    = 0x47, /* R/W byte */
    PMBUS_IOUT_OC_LV_FAULT_LIMIT    = 0x48, /* R/W word */
    PMBUS_IOUT_OC_LV_FAULT_RESPONSE = 0x49, /* R/W byte */
    PMBUS_IOUT_OC_WARN_LIMIT        = 0x4A, /* R/W word */
    PMBUS_IOUT_UC_FAULT_LIMIT       = 0x4B, /* R/W word */
    PMBUS_IOUT_UC_FAULT_RESPONSE    = 0x4C, /* R/W byte */
    PMBUS_OT_FAULT_LIMIT            = 0x4F, /* R/W word */
    PMBUS_OT_FAULT_RESPONSE         = 0x50, /* R/W byte */
    PMBUS_OT_WARN_LIMIT             = 0x51, /* R/W word */
    PMBUS_UT_WARN_LIMIT             = 0x52, /* R/W word */
    PMBUS_UT_FAULT_LIMIT            = 0x53, /* R/W word */
    PMBUS_UT_FAULT_RESPONSE         = 0x54, /* R/W byte */
    PMBUS_VIN_OV_FAULT_LIMIT        = 0x55, /* R/W word */
    PMBUS_VIN_OV_FAULT_RESPONSE     = 0x56, /* R/W byte */
    PMBUS_VIN_OV_WARN_LIMIT         = 0x57, /* R/W word */
    PMBUS_VIN_UV_WARN_LIMIT         = 0x58, /* R/W word */
    PMBUS_VIN_UV_FAULT_LIMIT        = 0x59, /* R/W word */
    PMBUS_VIN_UV_FAULT_RESPONSE     = 0x5A, /* R/W byte */
    PMBUS_IIN_OC_FAULT_LIMIT        = 0x5B, /* R/W word */
    PMBUS_IIN_OC_FAULT_RESPONSE     = 0x5C, /* R/W byte */
    PMBUS_IIN_OC_WARN_LIMIT         = 0x5D, /* R/W word */
    PMBUS_POWER_GOOD_ON             = 0x5E, /* R/W word */
    PMBUS_POWER_GOOD_OFF            = 0x5F, /* R/W word */
    PMBUS_TON_DELAY                 = 0x60, /* R/W word */
    PMBUS_TON_RISE                  = 0x61, /* R/W word */
    PMBUS_TON_MAX_FAULT_LIMIT       = 0x62, /* R/W word */
    PMBUS_TON_MAX_FAULT_RESPONSE    = 0x63, /* R/W byte */
    PMBUS_TOFF_DELAY                = 0x64, /* R/W word */
    PMBUS_TOFF_FALL                 = 0x65, /* R/W word */
    PMBUS_TOFF_MAX_WARN_LIMIT       = 0x66, /* R/W word */
    PMBUS_POUT_OP_FAULT_LIMIT       = 0x68, /* R/W word */
    PMBUS_POUT_OP_FAULT_RESPONSE    = 0x69, /* R/W byte */
    PMBUS_POUT_OP_WARN_LIMIT        = 0x6A, /* R/W word */
    PMBUS_PIN_OP_WARN_LIMIT         = 0x6B, /* R/W word */
    PMBUS_STATUS_BYTE               = 0x78, /* R/W byte */
    PMBUS_STATUS_WORD               = 0x79, /* R/W word */
    PMBUS_STATUS_VOUT               = 0x7A, /* R/W byte */
    PMBUS_STATUS_IOUT               = 0x7B, /* R/W byte */
    PMBUS_STATUS_INPUT              = 0x7C, /* R/W byte */
    PMBUS_STATUS_TEMPERATURE        = 0x7D, /* R/W byte */
    PMBUS_STATUS_CML                = 0x7E, /* R/W byte */
    PMBUS_STATUS_OTHER              = 0x7F, /* R/W byte */
    PMBUS_STATUS_MFR_SPECIFIC       = 0x80, /* R/W byte */
    PMBUS_STATUS_FANS_1_2           = 0x81, /* R/W byte */
    PMBUS_STATUS_FANS_3_4           = 0x82, /* R/W byte */
    PMBUS_READ_EIN                  = 0x86, /* Read-Only block 5 bytes */
    PMBUS_READ_EOUT                 = 0x87, /* Read-Only block 5 bytes */
    PMBUS_READ_VIN                  = 0x88, /* Read-Only word */
    PMBUS_READ_IIN                  = 0x89, /* Read-Only word */
    PMBUS_READ_VCAP                 = 0x8A, /* Read-Only word */
    PMBUS_READ_VOUT                 = 0x8B, /* Read-Only word */
    PMBUS_READ_IOUT                 = 0x8C, /* Read-Only word */
    PMBUS_READ_TEMPERATURE_1        = 0x8D, /* Read-Only word */
    PMBUS_READ_TEMPERATURE_2        = 0x8E, /* Read-Only word */
    PMBUS_READ_TEMPERATURE_3        = 0x8F, /* Read-Only word */
    PMBUS_READ_FAN_SPEED_1          = 0x90, /* Read-Only word */
    PMBUS_READ_FAN_SPEED_2          = 0x91, /* Read-Only word */
    PMBUS_READ_FAN_SPEED_3          = 0x92, /* Read-Only word */
    PMBUS_READ_FAN_SPEED_4          = 0x93, /* Read-Only word */
    PMBUS_READ_DUTY_CYCLE           = 0x94, /* Read-Only word */
    PMBUS_READ_FREQUENCY            = 0x95, /* Read-Only word */
    PMBUS_READ_POUT                 = 0x96, /* Read-Only word */
    PMBUS_READ_PIN                  = 0x97, /* Read-Only word */
    PMBUS_REVISION                  = 0x98, /* Read-Only byte */
    PMBUS_MFR_ID                    = 0x99, /* R/W block */
    PMBUS_MFR_MODEL                 = 0x9A, /* R/W block */
    PMBUS_MFR_REVISION              = 0x9B, /* R/W block */
    PMBUS_MFR_LOCATION              = 0x9C, /* R/W block */
    PMBUS_MFR_DATE                  = 0x9D, /* R/W block */
    PMBUS_MFR_SERIAL                = 0x9E, /* R/W block */
    PMBUS_APP_PROFILE_SUPPORT       = 0x9F, /* Read-Only block-read */
    PMBUS_MFR_VIN_MIN               = 0xA0, /* Read-Only word */
    PMBUS_MFR_VIN_MAX               = 0xA1, /* Read-Only word */
    PMBUS_MFR_IIN_MAX               = 0xA2, /* Read-Only word */
    PMBUS_MFR_PIN_MAX               = 0xA3, /* Read-Only word */
    PMBUS_MFR_VOUT_MIN              = 0xA4, /* Read-Only word */
    PMBUS_MFR_VOUT_MAX              = 0xA5, /* Read-Only word */
    PMBUS_MFR_IOUT_MAX              = 0xA6, /* Read-Only word */
    PMBUS_MFR_POUT_MAX              = 0xA7, /* Read-Only word */
    PMBUS_MFR_TAMBIENT_MAX          = 0xA8, /* Read-Only word */
    PMBUS_MFR_TAMBIENT_MIN          = 0xA9, /* Read-Only word */
    PMBUS_MFR_EFFICIENCY_LL         = 0xAA, /* Read-Only block 14 bytes */
    PMBUS_MFR_EFFICIENCY_HL         = 0xAB, /* Read-Only block 14 bytes */
    PMBUS_MFR_PIN_ACCURACY          = 0xAC, /* Read-Only byte */
    PMBUS_IC_DEVICE_ID              = 0xAD, /* Read-Only block-read */
    PMBUS_IC_DEVICE_REV             = 0xAE, /* Read-Only block-read */
    PMBUS_MFR_MAX_TEMP_1            = 0xC0, /* R/W word */
    PMBUS_MFR_MAX_TEMP_2            = 0xC1, /* R/W word */
    PMBUS_MFR_MAX_TEMP_3            = 0xC2, /* R/W word */
    PMBUS_IDLE_STATE                = 0xFF,
};


/*
 * According to the spec, each page may offer the full range of PMBus commands
 * available for each output or non-PMBus device.
 * Therefore, we can't assume that any registers will always be the same across
 * all pages.
 * The page 0xFF is intended for writes to all pages
 */
typedef struct PMBusPage {
    uint64_t page_flags;

    uint8_t page;                      /* R/W byte */
    uint8_t operation;                 /* R/W byte */
    uint8_t on_off_config;             /* R/W byte */
    uint8_t write_protect;             /* R/W byte */
    uint8_t phase;                     /* R/W byte */
    uint8_t vout_mode;                 /* R/W byte */
    uint16_t vout_command;             /* R/W word */
    uint16_t vout_trim;                /* R/W word */
    uint16_t vout_cal_offset;          /* R/W word */
    uint16_t vout_max;                 /* R/W word */
    uint16_t vout_margin_high;         /* R/W word */
    uint16_t vout_margin_low;          /* R/W word */
    uint16_t vout_transition_rate;     /* R/W word */
    uint16_t vout_droop;               /* R/W word */
    uint16_t vout_scale_loop;          /* R/W word */
    uint16_t vout_scale_monitor;       /* R/W word */
    uint16_t vout_min;                 /* R/W word */
    uint8_t coefficients[5];           /* Read-only block 5 bytes */
    uint16_t pout_max;                 /* R/W word */
    uint16_t max_duty;                 /* R/W word */
    uint16_t frequency_switch;         /* R/W word */
    uint16_t vin_on;                   /* R/W word */
    uint16_t vin_off;                  /* R/W word */
    uint16_t iout_cal_gain;            /* R/W word */
    uint16_t iout_cal_offset;          /* R/W word */
    uint8_t fan_config_1_2;            /* R/W byte */
    uint16_t fan_command_1;            /* R/W word */
    uint16_t fan_command_2;            /* R/W word */
    uint8_t fan_config_3_4;            /* R/W byte */
    uint16_t fan_command_3;            /* R/W word */
    uint16_t fan_command_4;            /* R/W word */
    uint16_t vout_ov_fault_limit;      /* R/W word */
    uint8_t vout_ov_fault_response;    /* R/W byte */
    uint16_t vout_ov_warn_limit;       /* R/W word */
    uint16_t vout_uv_warn_limit;       /* R/W word */
    uint16_t vout_uv_fault_limit;      /* R/W word */
    uint8_t vout_uv_fault_response;    /* R/W byte */
    uint16_t iout_oc_fault_limit;      /* R/W word */
    uint8_t iout_oc_fault_response;    /* R/W byte */
    uint16_t iout_oc_lv_fault_limit;   /* R/W word */
    uint8_t iout_oc_lv_fault_response; /* R/W byte */
    uint16_t iout_oc_warn_limit;       /* R/W word */
    uint16_t iout_uc_fault_limit;      /* R/W word */
    uint8_t iout_uc_fault_response;    /* R/W byte */
    uint16_t ot_fault_limit;           /* R/W word */
    uint8_t ot_fault_response;         /* R/W byte */
    uint16_t ot_warn_limit;            /* R/W word */
    uint16_t ut_warn_limit;            /* R/W word */
    uint16_t ut_fault_limit;           /* R/W word */
    uint8_t ut_fault_response;         /* R/W byte */
    uint16_t vin_ov_fault_limit;       /* R/W word */
    uint8_t vin_ov_fault_response;     /* R/W byte */
    uint16_t vin_ov_warn_limit;        /* R/W word */
    uint16_t vin_uv_warn_limit;        /* R/W word */
    uint16_t vin_uv_fault_limit;       /* R/W word */
    uint8_t vin_uv_fault_response;     /* R/W byte */
    uint16_t iin_oc_fault_limit;       /* R/W word */
    uint8_t iin_oc_fault_response;     /* R/W byte */
    uint16_t iin_oc_warn_limit;        /* R/W word */
    uint16_t power_good_on;            /* R/W word */
    uint16_t power_good_off;           /* R/W word */
    uint16_t ton_delay;                /* R/W word */
    uint16_t ton_rise;                 /* R/W word */
    uint16_t ton_max_fault_limit;      /* R/W word */
    uint8_t ton_max_fault_response;    /* R/W byte */
    uint16_t toff_delay;               /* R/W word */
    uint16_t toff_fall;                /* R/W word */
    uint16_t toff_max_warn_limit;      /* R/W word */
    uint16_t pout_op_fault_limit;      /* R/W word */
    uint8_t pout_op_fault_response;    /* R/W byte */
    uint16_t pout_op_warn_limit;       /* R/W word */
    uint16_t pin_op_warn_limit;        /* R/W word */
    uint16_t status_word;              /* R/W word */
    uint8_t status_vout;               /* R/W byte */
    uint8_t status_iout;               /* R/W byte */
    uint8_t status_input;              /* R/W byte */
    uint8_t status_temperature;        /* R/W byte */
    uint8_t status_cml;                /* R/W byte */
    uint8_t status_other;              /* R/W byte */
    uint8_t status_mfr_specific;       /* R/W byte */
    uint8_t status_fans_1_2;           /* R/W byte */
    uint8_t status_fans_3_4;           /* R/W byte */
    uint8_t read_ein[5];               /* Read-Only block 5 bytes */
    uint8_t read_eout[5];              /* Read-Only block 5 bytes */
    uint16_t read_vin;                 /* Read-Only word */
    uint16_t read_iin;                 /* Read-Only word */
    uint16_t read_vcap;                /* Read-Only word */
    uint16_t read_vout;                /* Read-Only word */
    uint16_t read_iout;                /* Read-Only word */
    uint16_t read_temperature_1;       /* Read-Only word */
    uint16_t read_temperature_2;       /* Read-Only word */
    uint16_t read_temperature_3;       /* Read-Only word */
    uint16_t read_fan_speed_1;         /* Read-Only word */
    uint16_t read_fan_speed_2;         /* Read-Only word */
    uint16_t read_fan_speed_3;         /* Read-Only word */
    uint16_t read_fan_speed_4;         /* Read-Only word */
    uint16_t read_duty_cycle;          /* Read-Only word */
    uint16_t read_frequency;           /* Read-Only word */
    uint16_t read_pout;                /* Read-Only word */
    uint16_t read_pin;                 /* Read-Only word */
    uint8_t revision;                  /* Read-Only byte */
    char mfr_id[50];                /* R/W block */
    char mfr_model[50];             /* R/W block */
    char mfr_revision[50];          /* R/W block */
    char mfr_location[50];          /* R/W block */
    char mfr_date[50];              /* R/W block */
    char mfr_serial[50];            /* R/W block */
    const char *app_profile_support;   /* Read-Only block-read */
    uint16_t mfr_vin_min;              /* Read-Only word */
    uint16_t mfr_vin_max;              /* Read-Only word */
    uint16_t mfr_iin_max;              /* Read-Only word */
    uint16_t mfr_pin_max;              /* Read-Only word */
    uint16_t mfr_vout_min;             /* Read-Only word */
    uint16_t mfr_vout_max;             /* Read-Only word */
    uint16_t mfr_iout_max;             /* Read-Only word */
    uint16_t mfr_pout_max;             /* Read-Only word */
    uint16_t mfr_tambient_max;         /* Read-Only word */
    uint16_t mfr_tambient_min;         /* Read-Only word */
    uint8_t mfr_efficiency_ll[14];     /* Read-Only block 14 bytes */
    uint8_t mfr_efficiency_hl[14];     /* Read-Only block 14 bytes */
    uint8_t mfr_pin_accuracy;          /* Read-Only byte */
    uint16_t mfr_max_temp_1;           /* R/W word */
    uint16_t mfr_max_temp_2;           /* R/W word */
    uint16_t mfr_max_temp_3;           /* R/W word */
    char version[50];
    PTR_DEVICE_CONFIG ptrDeviceConfig;
    uint64_t receive_times;
    uint64_t write_times;
} PMBusPage;


/**************************************** DEVICE ADD INFO ****************************************/
#define DEVICE_MAX_NUM 256


typedef struct DEVICE_ADD_INFO_ {
    uint8_t exist; /* 此条目是否有设备，0: 不存在。 1：存在 */
    uint32_t device_index;
    DEVICE_TYPE device_type; /* 设备类型 */
    DEVICE_TYPE_ID device_type_id;
    char device_name[DEVICE_NAME_MAX_LEN];
    void *dev_gpio;
    PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData;
    PTR_I2C_DEVICE_DATA ptrI2cDeviceData;
    PTR_I3C_DEVICE_DATA ptrI3cDeviceData;
    PTR_GPIO_DEVICE_DATA ptrGpioDeviceData;
    PTR_ADC_DEVICE_DATA ptrAdcDeviceData;
    PMBusPage *pmBusPage;
    void *pca954x;
    PTR_PWM_TACH_DEVICE ptrPwmTachDevice;
    PTR_DEVICE_CONFIG ptrDeviceConfig;
} DEVICE_ADD_INFO, *PTR_DEVICE_ADD_INFO;

extern DEVICE_ADD_INFO deviceAddList[DEVICE_MAX_NUM];
extern pthread_mutex_t file_log_lock;

#define MAC_DEVICE_INFO_MAX_NUM (10)

extern MAC_DEVICE_INFO macDeviceInfo[MAC_DEVICE_INFO_MAX_NUM];

int device_add(DEVICE_TYPE_ID device_type_id, const char *device_name, void *vPtrDeviceData, void *vPtrDevGpio);
int get_device_index(void *vPtrDeviceData);
DEVICE_TYPE get_device_type(DEVICE_TYPE_ID device_type_id);
void *get_dev_gpio(int32_t device_index, const char *device_name);
void get_dev_index_for_name(const char *name, int *index);


void file_log(const char *message, uint8_t log_type);
char *get_file_right_path(const char *filename);
cJSON *read_config_file(const char *filename);
uint32_t *detachArgsData(char *oStr, char dataType, const char *ctrl_name, uint64_t *res_len);
void dynamic_change_data(DEVICE_TYPE_ID device_type_id, void *vPtrDeviceData, char *args);

PTR_CONFIG_DATA parse_configuration(void);

extern void (*connect_gpio_line)(void *send_dev, int send_line, void *recv_dev, int recv_line);
extern void (*set_gpio_line)(void *pin_buf, uint32_t pin, int level);

void mac_apply_method(PTR_MAC_CONFIG ptrMacConfig);
void gpio_signal_init(PTR_GPIO_SIGNAL ptrGpioSignal, uint32_t device_index);
uint8_t line_has_config_input(int line, PTR_GPIO_SIGNAL ptrGpioSignal);
uint8_t line_has_config_output(int line, PTR_GPIO_SIGNAL ptrGpioSignal);

extern uint16_t (*adc_get_value)(void *opaque, uint8_t channel);
extern void (*adc_set_value)(void *opaque, uint8_t channel, uint16_t value);

extern uint8_t adc_reg10_has_read;

#endif //QEMU_ASPEED_INIT_H
