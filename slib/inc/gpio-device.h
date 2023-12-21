/**************************************************
 * filename     :  gpio-device.h
 * create time  :  2023/9/14
 **************************************************/
#ifndef QEMU_GPIO_DEVICE_H
#define QEMU_GPIO_DEVICE_H

#include "stdint.h"
#include "slib-common.h"

#define GPIO_DEVICE_TYPE_ID_OFFSET 201
#define GPIO_DEVICE_TOTAL_NUM 10

typedef struct GPIO_DEVICE_DATA_ {
    uint8_t *data_buf;
    uint32_t pin_nums;
    void *pin_buf;
    uint8_t *pin_state;
    uint8_t *pin_type;
    gpio_input_handler inputHandler;
    gpio_output_handler outputHandler;
    uint8_t *active_data;
    uint8_t *passive_data;
    PTR_DEVICE_CONFIG ptrDeviceConfig;
} GPIO_DEVICE_DATA, *PTR_GPIO_DEVICE_DATA;

typedef struct PIN_OSCILLOSCOPE_MONITOR_THREAD_ARGS_ {
    PTR_GPIO_DEVICE_DATA ptrGpioDeviceData;
    uint32_t pin_num;
}PIN_OSCILLOSCOPE_MONITOR_THREAD_ARGS, *PTR_PIN_OSCILLOSCOPE_MONITOR_THREAD_ARGS;

typedef struct PIN_WAVEFORM_GENERATOR_THREAD_ARGS_ {
    PTR_GPIO_DEVICE_DATA ptrGpioDeviceData;
    uint32_t pin_num;
}PIN_WAVEFORM_GENERATOR_THREAD_ARGS, *PTR_PIN_WAVEFORM_GENERATOR_THREAD_ARGS;

#include "aspeed-init.h"

/**************************************** Device-0 SWITCH ****************************************/

typedef struct GPIO_SWITCH_sTYPE_ {
    pthread_mutex_t mutex;
    pthread_t *pthread_oscilloscope;
    pthread_t *pthread_waveform_generator;
    pthread_mutex_t *pin_state_mutex;
    uint32_t device_index;
    PIN_OSCILLOSCOPE *pinOscilloscope_list;
    PIN_WAVEFORM_GENERATOR *pinWaveformGenerator_list;
} GPIO_SWITCH_sTYPE, *PTR_GPIO_SWITCH_sTYPE;

void init_GPIOEmptyDevice0(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice0(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice0(int line, int new_state, void *ptrDeviceData);

/**************************************** device-1 ****************************************/
void init_GPIOEmptyDevice1(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice1(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice1(int line, int new_state, void *ptrDeviceData);

/**************************************** device-2 ****************************************/
void init_GPIOEmptyDevice2(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice2(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice2(int line, int new_state, void *ptrDeviceData);

/**************************************** device-3 ****************************************/
void init_GPIOEmptyDevice3(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice3(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice3(int line, int new_state, void *ptrDeviceData);

/**************************************** device-4 ****************************************/
void init_GPIOEmptyDevice4(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice4(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice4(int line, int new_state, void *ptrDeviceData);

/**************************************** device-5 ****************************************/
void init_GPIOEmptyDevice5(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice5(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice5(int line, int new_state, void *ptrDeviceData);

/**************************************** device-6 ****************************************/
void init_GPIOEmptyDevice6(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice6(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice6(int line, int new_state, void *ptrDeviceData);

/**************************************** device-7 ****************************************/
void init_GPIOEmptyDevice7(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice7(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice7(int line, int new_state, void *ptrDeviceData);

/**************************************** device-8 ****************************************/
void init_GPIOEmptyDevice8(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice8(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice8(int line, int new_state, void *ptrDeviceData);

/**************************************** device-9 ****************************************/
void init_GPIOEmptyDevice9(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData);
void input_handler_GPIOEmptyDevice9(int line, int new_state, void *ptrDeviceData);
void output_handler_GPIOEmptyDevice9(int line, int new_state, void *ptrDeviceData);

/**************************************** 自定义设备添加函数 ****************************************/

typedef void *(*GpioFunctionPtr)(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);

void *GPIOEmptyDevice0_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);
void *GPIOEmptyDevice1_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);
void *GPIOEmptyDevice2_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);
void *GPIOEmptyDevice3_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);
void *GPIOEmptyDevice4_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);
void *GPIOEmptyDevice5_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);
void *GPIOEmptyDevice6_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);
void *GPIOEmptyDevice7_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);
void *GPIOEmptyDevice8_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);
void *GPIOEmptyDevice9_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig);

GpioFunctionPtr getGpioDeviceAddFunc(int device_type_id);
int getGpioDeviceTypeId(GpioFunctionPtr gpioFuntionPtr);

#endif //QEMU_GPIO_DEVICE_H
