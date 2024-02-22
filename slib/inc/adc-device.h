/**************************************************
 * filename     :  adc-device.h
 * create time  :  2023/10/7
 **************************************************/
#ifndef QEMU_ADC_DEVICE_H
#define QEMU_ADC_DEVICE_H

#include "stdint.h"
#include "slib-common.h"
#include <pthread.h>

#define ADC_DEVICE_TYPE_ID_OFFSET 301
#define ADC_DEVICE_TOTAL_NUM 5

typedef enum ADC_REG_TYPE_ {
    REG_L = 0,
    REG_H,
}ADC_REG_TYPE, *PTR_ADC_REG_TYPE;

typedef union ADC_REG_ {
    uint32_t reg;
    struct {
        uint16_t l;
        uint16_t h;
    } reg_lh;
} ADC_REG, *PTR_ADC_REG;

typedef struct ADC_DEVICE_DATA_ {
    ADC_REG_TYPE adcRegType;
    PTR_ADC_REG ptrAdcReg;
    uint32_t set_adc_value;
    uint32_t division;
    pthread_mutex_t value_mutex;
    uint8_t *active_data;
    uint8_t *passive_data;
    PTR_DEVICE_CONFIG ptrDeviceConfig;
} ADC_DEVICE_DATA, *PTR_ADC_DEVICE_DATA;

#include "aspeed-init.h"

void init_ADCDevice0(PTR_ADC_DEVICE_DATA ptrAdcDeviceData);

void adc_device_add0(void *obj, PTR_DEVICE_CONFIG ptrDeviceConfig);

void adc_device_add1(void *obj_0, void *obj_1, PTR_DEVICE_CONFIG ptrDeviceConfig);

#endif //QEMU_ADC_DEVICE_H
