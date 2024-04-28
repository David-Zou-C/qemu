/**************************************************
 * filename     :  adc-device.c
 * create time  :  2023/10/7
 **************************************************/


#include "adc-device.h"
#include <time.h>
#include <unistd.h>

uint8_t adc_reg10_has_read = FALSE;
uint8_t adc_channel_disable_flag[8] = {0};
static void *adc_control_thread(void *pVoidAdcDeviceData) {
    PTR_ADC_DEVICE_DATA ptrAdcDeviceData = (PTR_ADC_DEVICE_DATA) pVoidAdcDeviceData;
    uint32_t real_vol = 0;
    double sample_vol = 0;
    uint16_t real_reg = 0;
    double division = 1.0;
    file_log("adc control_thread", LOG_TIME_END);

    usleep(10 * 1000 * 1000);  /* 延时 10 秒 */
    while (adc_reg10_has_read == FALSE) {
        /* 必须得让 reg10 先被 BMC 读取之后，才能进行设置 */
        usleep(1 * 1000 * 1000);
    }
    usleep(1 * 1000 * 1000);

    while (TRUE) {
//        pthread_mutex_lock(&ptrAdcDeviceData->value_mutex);
        real_vol = ptrAdcDeviceData->set_adc_value;

        division = (double )(ptrAdcDeviceData->division) / 1000;
        sample_vol = ((double )real_vol) / division;
        real_reg = (uint16_t )(sample_vol * 1024 / 2500 - 256);

        if (ptrAdcDeviceData->adcRegType == REG_L) {
            if (ptrAdcDeviceData->ptrAdcReg->reg_lh.l != real_reg) {
                ptrAdcDeviceData->ptrAdcReg->reg_lh.l = real_reg;
            }
        } else if (ptrAdcDeviceData->adcRegType == REG_H){
            if (ptrAdcDeviceData->ptrAdcReg->reg_lh.h != real_reg) {
                ptrAdcDeviceData->ptrAdcReg->reg_lh.h = real_reg;
            }
        } else {
            if (*(ptrAdcDeviceData->ptrExtAdcReg) != real_reg && 
            adc_channel_disable_flag[ptrAdcDeviceData->ptrDeviceConfig->adc_channel] == 0) {
                *ptrAdcDeviceData->ptrExtAdcReg = real_reg;
            }
        }

//        pthread_mutex_unlock(&ptrAdcDeviceData->value_mutex);

        usleep(500 * 1000);  /* 0.5 s */
    }

    return NULL;
}

void init_ADCDevice0(PTR_ADC_DEVICE_DATA ptrAdcDeviceData) {
    /* ADC 设备的初始化函数，每个设备只能执行一次 */
    pthread_t pthread;
    pthread_mutex_init(&ptrAdcDeviceData->value_mutex, NULL);
    pthread_create(&pthread, NULL, adc_control_thread, ptrAdcDeviceData);
    pthread_detach(pthread);

//    pthread_mutex_lock(&ptrAdcDeviceData->value_mutex);
    dynamic_change_data(ADC, ptrAdcDeviceData, ptrAdcDeviceData->ptrDeviceConfig->args);
//    pthread_mutex_unlock(&ptrAdcDeviceData->value_mutex);
}
