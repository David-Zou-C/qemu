/**************************************************
 * filename     :  slib-common.h
 * create time  :  2023/9/16
 **************************************************/
#ifndef QEMU_SLIB_COMMON_H
#define QEMU_SLIB_COMMON_H

#include <stdint.h>

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define DEVICE_NAME_MAX_LEN (100)

typedef enum DEVICE_TYPE_ {
    UNDEFINED, /* 0 视为未定义 */
    BMC_DEVICE_TYPE,  /* 实际从1开始，0视为未定义 */
    SMBUS_DEVICE_TYPE,
    PMBUS_DEVICE_TYPE,
    I2C_DEVICE_TYPE,
    GPIO_DEVICE_TYPE,
    ADC_DEVICE_TYPE,
    PCA954X_DEVICE_TYPE,
    PWM_TACH_DEVICE_TYPE,
} DEVICE_TYPE, *PTR_DEVICE_TYPE;

typedef enum DEVICE_TYPE_ID_ {
    /* 第 0 表示为 BMC */
    BMC = 0,
    /* SMBUS */
    SMBUS_EEPROM_S = 1,
    SMBUS_TMP = 2,
    SMBUS_TPA626 = 3,
    SMBUS_DIMM_TEMP = 4,
    /* I2c */
    I2C_EEPROM = 101,
    I2C_BP_CPLD = 102,
    I2C_DIMM_TEMP = 103,
    /* GPIO */
    GPIO_SWITCH = 201,
    /* ADC */
    ADC = 301,
    /* PCA954X */
    PCA9546 = 401,
    PCA9548 = 402,
    /* PMBUS */
    PMBUS_PSU = 501,
    /* PWM_TACH */
    PWM_TACH = 601
} DEVICE_TYPE_ID, *PTR_DEVICE_TYPE_ID;


/**************************************** GPIO ****************************************/
#define PIN_TYPE_INPUT  (1<<0)
#define PIN_TYPE_OUTPUT (1<<1)
#define PIN_TYPE_UNCONFIGURE (3)

#define PIN_FALL_EDGE       (1)
#define PIN_RISING_EDGE     (2)
#define PIN_DOUBLE_EDGE     (3)

typedef enum GPIO_SIGNAL_TYPE_ {
    GPIO_SIGNAL_INPUT,
    GPIO_SIGNAL_OUTPUT
} GPIO_SIGNAL_TYPE, *PTR_GPIO_SIGNAL_TYPE;

typedef struct GPIO_SIGNAL_INPUT_LINE_ {
    int32_t send_device_index;
    char send_device_name[DEVICE_NAME_MAX_LEN];
    uint32_t send_device_line;
    uint32_t recv_line;
} GPIO_SIGNAL_INPUT_LINE, *PTR_GPIO_SIGNAL_INPUT_LINE;

typedef struct GPIO_SIGNAL_OUTPUT_LINE_ {
    int32_t recv_device_index;
    char recv_device_name[DEVICE_NAME_MAX_LEN];
    uint32_t recv_device_line;
    uint32_t send_line;
} GPIO_SIGNAL_OUTPUT_LINE, *PTR_GPIO_SIGNAL_OUTPUT_LINE;

typedef enum PIN_WAVEFORM_TYPE_ {
    PIN_WAVEFORM_TYPE_ONCE,
    PIN_WAVEFORM_TYPE_REPEAT,
    PIN_WAVEFORM_TYPE_TOGGLE
}PIN_WAVEFORM_TYPE, *PTR_PIN_WAVEFORM_TYPE;

typedef struct GPIO_OUTPUT_LOGIC_INIT_ {
    uint32_t output_pin_num;
    char *output_waveform;
    PIN_WAVEFORM_TYPE output_waveform_type;
    struct GPIO_OUTPUT_LOGIC_INIT_ *pre;
    struct GPIO_OUTPUT_LOGIC_INIT_ *next;
}GPIO_OUTPUT_LOGIC_INIT, *PTR_GPIO_OUTPUT_LOGIC_INIT;

typedef struct GPIO_RESPONSE_LOGIC_ {
    uint32_t input_pin_num;
    char *input_status_order;
    uint32_t output_pin_num;
    char *output_waveform;
    PIN_WAVEFORM_TYPE output_waveform_type;
    struct GPIO_RESPONSE_LOGIC_ *pre;
    struct GPIO_RESPONSE_LOGIC_ *next;
}GPIO_RESPONSE_LOGIC, *PTR_GPIO_RESPONSE_LOGIC;

typedef struct GPIO_SIGNAL_ {
    GPIO_SIGNAL_TYPE gpioSignalType;
    union {
        GPIO_SIGNAL_INPUT_LINE gpioSignalInputLine;
        GPIO_SIGNAL_OUTPUT_LINE gpioSignalOutputLine;
    } gpioSignal;
    struct GPIO_SIGNAL_ *pre;
    struct GPIO_SIGNAL_ *next;
} GPIO_SIGNAL, *PTR_GPIO_SIGNAL;

typedef enum I2C_TYPE_ {
    I2C,
    I3C
} I2C_TYPE, *PTR_I2C_TYPE;

typedef struct DEVICE_CONFIG_ {
    char *description;
    DEVICE_TYPE deviceType;
    DEVICE_TYPE_ID device_type_id;
    char name[DEVICE_NAME_MAX_LEN];
    uint8_t bus;
    uint8_t adc_channel;
    double division;
    uint8_t addr;
    struct {
        int device_index;
        char device_name[DEVICE_NAME_MAX_LEN];
        I2C_TYPE i2CType;
    } master;
    uint32_t pin_nums;
    char fru_path[32];
    PTR_GPIO_SIGNAL ptrGpioSignal;
    PTR_GPIO_OUTPUT_LOGIC_INIT ptrGpioOutputLogicInit;
    PTR_GPIO_RESPONSE_LOGIC ptrGpioResponseLogic;
    uint8_t pwm_tach_num;
    char *args;
    struct DEVICE_CONFIG_ *pre;
    struct DEVICE_CONFIG_ *next;
} DEVICE_CONFIG, *PTR_DEVICE_CONFIG;

typedef enum MAC_APPLY_METHOD_ {
    NO_METHOD = 0,
    MAC_METHOD_1 = 1,
    MAC_METHOD_2 = 2
} MAC_APPLY_METHOD, *PTR_MAC_APPLY_METHOD;

typedef struct MAC_DEVICE_INFO_ {
    MAC_APPLY_METHOD macApplyMethod;
    DEVICE_TYPE_ID deviceTypeId;
    uint8_t i2c_bus;
    uint8_t i2c_addr;
    uint32_t mac_value_for_offset[6];
} MAC_DEVICE_INFO, *PTR_MAC_DEVICE_INFO;

typedef struct MAC_CONFIG_ {
    uint8_t mac[6];
    MAC_APPLY_METHOD macApplyMethod;
} MAC_CONFIG, *PTR_MAC_CONFIG;

typedef struct CONFIG_DATA_ {
    PTR_DEVICE_CONFIG ptrDeviceConfig;
    PTR_MAC_CONFIG ptrMacConfig;
} CONFIG_DATA, *PTR_CONFIG_DATA;

typedef void (*gpio_input_handler)(int line, int new_state, void *ptrDeviceData);
typedef void (*gpio_output_handler)(int line, int new_state, void *ptrDeviceData);

#define PIN_OSCILLOSCOPE_MAX_BITS 100
#define PIN_OSCILLOSCOPE_RATE 10

typedef struct PIN_OSCILLOSCOPE_ {
    /**************************************** 采样状态队列 ****************************************/
    uint8_t status_bit[PIN_OSCILLOSCOPE_MAX_BITS+1];
    uint32_t rate;  /* 每秒采样次数 */
    uint32_t front;  /* 头结点 */
    uint32_t rear;  /* 尾节点 */
    uint32_t capacity;  /* 容量 */
    /**************************************** 变化状态队列 ****************************************/
    uint8_t change_status[PIN_OSCILLOSCOPE_MAX_BITS];
    uint32_t change_front;
    uint32_t change_rear;
    uint32_t change_length;
}PIN_OSCILLOSCOPE, *PTR_PIN_OSCILLOSCOPE;

#define PIN_WAVEFORM_BUF_MAX_BIT 100
#define PIN_WAVEFORM_RATE 8
#define PIN_WAVEFORM_RATE_MAX 1000

typedef struct PIN_WAVEFORM_GENERATOR_ {
    /* 仅在循环中被动修改 */
    uint32_t curr_waveform_loc;         /* 当前波形产生状态在波形缓冲区的位置 */
    /* 可被其他地方修改 */
    uint8_t waveform_buf[PIN_WAVEFORM_BUF_MAX_BIT];  /* 波形产生的缓冲区 */
    uint32_t waveform_len;              /* 产生波形在缓冲区的长度 */
    PIN_WAVEFORM_TYPE pinWaveformType;  /* 波形产生类型：一次或重复 */
    uint32_t waveform_rate;             /* 波形频率 */
    uint8_t stop_waveform_generator;    /* 是否停止产生波形 */
    uint8_t has_update;                 /* 有更新 */
}PIN_WAVEFORM_GENERATOR, *PTR_PIN_WAVEFORM_GENERATOR;

int compareIgnoreCase(const char *str1, const char *str2);

void oscilloscope_init(PTR_PIN_OSCILLOSCOPE ptrPinOscilloscope);
uint8_t oscilloscope_add_status_bit(PTR_PIN_OSCILLOSCOPE ptrPinOscilloscope, uint8_t status_bit);

void *oscilloscope_monitor_thread(void *ptrPinOscilloscopeMonitorThreadArgs);

void *waveform_generator_thread(void *ptrWaveformGeneratorThread);

#endif //QEMU_SLIB_COMMON_H
