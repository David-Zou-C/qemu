/**************************************************
 * filename     :  gpio-device.c
 * create time  :  2023/9/14
 **************************************************/

#include "gpio-device.h"
#include <pthread.h>

/**************************************** device-0 ****************************************/
void init_GPIOEmptyDevice0(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {
    FUNC_DEBUG("init_GPIOEmptyDevice0()")
    PTR_GPIO_SWITCH_sTYPE ptrGpioSwitchSType;
    /* 先初始化 data_buf */
    if (ptrGpioDeviceData->data_buf == NULL) {
        /**************************************** DeviceData 的部分值的初始化 ****************************************/
        for (int i = 0; i < ptrGpioDeviceData->pin_nums; ++i) {
            ptrGpioDeviceData->pin_state[i] = 0;  /* 将所有 pin_state 初始化为 0 */
            ptrGpioDeviceData->pin_type[i] = 0;
            if (line_has_config_input(i, ptrGpioDeviceData->ptrDeviceConfig->ptrGpioSignal)) {
                ptrGpioDeviceData->pin_type[i] = PIN_TYPE_INPUT;
            } else if (line_has_config_output(i, ptrGpioDeviceData->ptrDeviceConfig->ptrGpioSignal)) {
                ptrGpioDeviceData->pin_type[i] = PIN_TYPE_OUTPUT;
            } else {
                ptrGpioDeviceData->pin_type[i] = PIN_TYPE_UNCONFIGURE;
            }
        }
        /**************************************** sTYPE 的初始化动作 ****************************************/
        ptrGpioDeviceData->data_buf = (uint8_t *) malloc(sizeof(GPIO_SWITCH_sTYPE));
        memset(ptrGpioDeviceData->data_buf, 0, sizeof(GPIO_SWITCH_sTYPE));
        ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE) ptrGpioDeviceData->data_buf;
        pthread_mutex_init(&ptrGpioSwitchSType->mutex, NULL);
//        pthread_mutex_lock(&ptrGpioSwitchSType->mutex);
        /* 获取 device_index */
        ptrGpioSwitchSType->device_index = get_device_index(ptrGpioDeviceData);
        /* pin_state mutex */
        ptrGpioSwitchSType->pin_state_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t) * ptrGpioDeviceData->pin_nums);
        /**************************************** pinOscilloscope 初始化 ****************************************/
        if (ptrGpioSwitchSType->pinOscilloscope_list == NULL) {
            ptrGpioSwitchSType->pinOscilloscope_list = (PTR_PIN_OSCILLOSCOPE)malloc(sizeof(PIN_OSCILLOSCOPE) * ptrGpioDeviceData->pin_nums);
            memset(ptrGpioSwitchSType->pinOscilloscope_list, 0, sizeof(PIN_OSCILLOSCOPE) * ptrGpioDeviceData->pin_nums);
            ptrGpioSwitchSType->pthread_oscilloscope = (pthread_t *) malloc(sizeof(pthread_t) * ptrGpioDeviceData->pin_nums);
        }
        /**************************************** pinWaveformGenerator 初始化 ****************************************/
        if (ptrGpioSwitchSType->pinWaveformGenerator_list == NULL) {
            ptrGpioSwitchSType->pinWaveformGenerator_list = (PTR_PIN_WAVEFORM_GENERATOR) malloc(sizeof(PIN_WAVEFORM_GENERATOR) * ptrGpioDeviceData->pin_nums);
            memset(ptrGpioSwitchSType->pinWaveformGenerator_list, 0, sizeof(PIN_WAVEFORM_GENERATOR) * ptrGpioDeviceData->pin_nums);
            ptrGpioSwitchSType->pthread_waveform_generator = (pthread_t *) malloc(sizeof(pthread_t) * ptrGpioDeviceData->pin_nums);
        }
        /**************************************** GPIO connect init ****************************************/
        gpio_signal_init(ptrGpioDeviceData->ptrDeviceConfig->ptrGpioSignal, ptrGpioSwitchSType->device_index);
        /**************************************** create thread ****************************************/
        for (int i = 0; i < ptrGpioDeviceData->pin_nums; ++i) {
            /* init pin_state_mutex */
            pthread_mutex_init(&ptrGpioSwitchSType->pin_state_mutex[i], NULL);
            /* oscilloscope init */
            oscilloscope_init(&ptrGpioSwitchSType->pinOscilloscope_list[i]);
            /**************************************** oscilloscope monitor thread ****************************************/
            PTR_PIN_OSCILLOSCOPE_MONITOR_THREAD_ARGS ptrPinOscilloscopeMonitorThreadArgs = (PTR_PIN_OSCILLOSCOPE_MONITOR_THREAD_ARGS) malloc(
                    sizeof(PIN_OSCILLOSCOPE_MONITOR_THREAD_ARGS));
            ptrPinOscilloscopeMonitorThreadArgs->ptrGpioDeviceData = ptrGpioDeviceData;
            ptrPinOscilloscopeMonitorThreadArgs->pin_num = i;
            pthread_create(&ptrGpioSwitchSType->pthread_oscilloscope[i], NULL, oscilloscope_monitor_thread,
                           ptrPinOscilloscopeMonitorThreadArgs);
            pthread_detach(ptrGpioSwitchSType->pthread_oscilloscope[i]);
            /**************************************** waveform generator thread ****************************************/
            PTR_PIN_WAVEFORM_GENERATOR_THREAD_ARGS ptrPinWaveformGeneratorThreadArgs = (PTR_PIN_WAVEFORM_GENERATOR_THREAD_ARGS) malloc(
                    sizeof(PIN_WAVEFORM_GENERATOR_THREAD_ARGS));
            ptrPinWaveformGeneratorThreadArgs->ptrGpioDeviceData = ptrGpioDeviceData;
            ptrPinWaveformGeneratorThreadArgs->pin_num = i;
            pthread_create(&ptrGpioSwitchSType->pthread_waveform_generator[i], NULL, waveform_generator_thread,
                           ptrPinWaveformGeneratorThreadArgs);
            pthread_detach(ptrGpioSwitchSType->pthread_waveform_generator[i]);
        }
//        pthread_mutex_unlock(&ptrGpioSwitchSType->mutex);
    } else {
        ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE) ptrGpioDeviceData->data_buf;
    }

    /* 解析 args 参数 */
    dynamic_change_data(GPIO_SWITCH, ptrGpioDeviceData, ptrGpioDeviceData->ptrDeviceConfig->args);

}

void input_handler_GPIOEmptyDevice0(int line, int new_state, void *ptrDeviceData) {
    FUNC_DEBUG("input_handler_GPIOEmptyDevice0()")
    PTR_GPIO_DEVICE_DATA ptrGpioDeviceData = (PTR_GPIO_DEVICE_DATA) ptrDeviceData;
//    PTR_GPIO_SWITCH_sTYPE ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE) ptrGpioDeviceData->data_buf;

    if (line >= ptrGpioDeviceData->pin_nums || line < 0) {
        file_log("error: line exceed pin_nums or < 0. ", LOG_TIME_END);
        return;
    }

    /* 无论此 line 配置为 input 还是 output 都应该被改变 */
//    pthread_mutex_lock(&ptrGpioSwitchSType->pin_state_mutex[line]);
    ptrGpioDeviceData->pin_state[line] = new_state;
//    pthread_mutex_unlock(&ptrGpioSwitchSType->pin_state_mutex[line]);
}

void output_handler_GPIOEmptyDevice0(int line, int new_state, void *ptrDeviceData) {
    FUNC_DEBUG("output_handler_GPIOEmptyDevice0()")
    PTR_GPIO_DEVICE_DATA ptrGpioDeviceData = (PTR_GPIO_DEVICE_DATA) ptrDeviceData;
//    PTR_GPIO_SWITCH_sTYPE ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE) ptrGpioDeviceData->data_buf;

    if (line >= ptrGpioDeviceData->pin_nums || line < 0) {
        file_log("error: line exceed pin_nums or < 0. ", LOG_TIME_END);
        return;
    }

    if (ptrGpioDeviceData->pin_type[line] & PIN_TYPE_OUTPUT) {
        /* 此 line 有配置为 OUTPUT */
//        pthread_mutex_lock(&ptrGpioSwitchSType->pin_state_mutex[line]);
        ptrGpioDeviceData->pin_state[line] = new_state;
        set_gpio_line(ptrGpioDeviceData->pin_buf, line, new_state);  /* 设置 output gpio line */
//        pthread_mutex_unlock(&ptrGpioSwitchSType->pin_state_mutex[line]);
    } else {
        /* 此 line 未曾配置为 OUTPUT */
        file_log("error: not configure to OUTPUT !", LOG_TIME_END);
    }
}


/**************************************** device-1 ****************************************/
void init_GPIOEmptyDevice1(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {

}

void input_handler_GPIOEmptyDevice1(int line, int new_state, void *ptrDeviceData) {

}

void output_handler_GPIOEmptyDevice1(int line, int new_state, void *ptrDeviceData) {

}


/**************************************** device-2 ****************************************/
void init_GPIOEmptyDevice2(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {

}

void input_handler_GPIOEmptyDevice2(int line, int new_state, void *ptrDeviceData) {

}

void output_handler_GPIOEmptyDevice2(int line, int new_state, void *ptrDeviceData) {

}


/**************************************** device-3 ****************************************/
void init_GPIOEmptyDevice3(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {

}

void input_handler_GPIOEmptyDevice3(int line, int new_state, void *ptrDeviceData) {

}

void output_handler_GPIOEmptyDevice3(int line, int new_state, void *ptrDeviceData) {

}


/**************************************** device-4 ****************************************/
void init_GPIOEmptyDevice4(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {

}

void input_handler_GPIOEmptyDevice4(int line, int new_state, void *ptrDeviceData) {

}

void output_handler_GPIOEmptyDevice4(int line, int new_state, void *ptrDeviceData) {

}


/**************************************** device-5 ****************************************/
void init_GPIOEmptyDevice5(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {

}

void input_handler_GPIOEmptyDevice5(int line, int new_state, void *ptrDeviceData) {

}

void output_handler_GPIOEmptyDevice5(int line, int new_state, void *ptrDeviceData) {

}


/**************************************** device-6 ****************************************/
void init_GPIOEmptyDevice6(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {

}

void input_handler_GPIOEmptyDevice6(int line, int new_state, void *ptrDeviceData) {

}

void output_handler_GPIOEmptyDevice6(int line, int new_state, void *ptrDeviceData) {

}


/**************************************** device-7 ****************************************/
void init_GPIOEmptyDevice7(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {

}

void input_handler_GPIOEmptyDevice7(int line, int new_state, void *ptrDeviceData) {

}

void output_handler_GPIOEmptyDevice7(int line, int new_state, void *ptrDeviceData) {

}


/**************************************** device-8 ****************************************/
void init_GPIOEmptyDevice8(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {

}

void input_handler_GPIOEmptyDevice8(int line, int new_state, void *ptrDeviceData) {

}

void output_handler_GPIOEmptyDevice8(int line, int new_state, void *ptrDeviceData) {

}


/**************************************** device-9 ****************************************/
void init_GPIOEmptyDevice9(PTR_GPIO_DEVICE_DATA ptrGpioDeviceData) {

}

void input_handler_GPIOEmptyDevice9(int line, int new_state, void *ptrDeviceData) {

}

void output_handler_GPIOEmptyDevice9(int line, int new_state, void *ptrDeviceData) {

}


