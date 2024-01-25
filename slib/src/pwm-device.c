/**************************************************
 * filename     :  pwm_device.c
 * create time  :  2024/1/24
 **************************************************/

#include "pwm-device.h"
#include "aspeed-init.h"

RPM_DUTY gRpmDuty[8] = {0};

static void init_pwm_device(PTR_PWM_TACH_DEVICE ptrPwmTachDevice) {
    dynamic_change_data(PWM_TACH, ptrPwmTachDevice, ptrPwmTachDevice->ptrDeviceConfig->args);
}

void pwm_device_add(PTR_DEVICE_CONFIG ptrDeviceConfig) {
    PTR_PWM_TACH_DEVICE ptrPwmTachDevice = (PTR_PWM_TACH_DEVICE) malloc(sizeof(PWM_TACH_DEVICE));
    memset(ptrPwmTachDevice, 0, sizeof(PWM_TACH_DEVICE));

    ptrPwmTachDevice->ptrDeviceConfig = ptrDeviceConfig;
    ptrPwmTachDevice->pwm_tach_num = ptrDeviceConfig->pwm_tach_num;
    printf("pwm device added - %d\n", ptrPwmTachDevice->pwm_tach_num);
    if (ptrPwmTachDevice->pwm_tach_num < 8) {
        ptrPwmTachDevice->ptrRpmDuty = &gRpmDuty[ptrPwmTachDevice->pwm_tach_num];
    } else {
        /*  */
        return;
    }

    init_pwm_device(ptrPwmTachDevice);
    device_add(PWM_TACH, ptrDeviceConfig->name, ptrPwmTachDevice, NULL);
}
