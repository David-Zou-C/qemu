/**************************************************
 * filename     :  pwm-device.h
 * create time  :  2024/1/24
 **************************************************/
#ifndef QEMU_PWM_DEVICE_H
#define QEMU_PWM_DEVICE_H

#include "slib-common.h"

typedef struct RPM_DUTY_ {
    double max_rpm;
    double min_rpm;
    double min_offset;
    double max_offset;
    int rand_deviation_rate;
} RPM_DUTY, *PTR_RPM_DUTY;

typedef struct PWM_TACH_DEVICE_{
    uint8_t pwm_tach_num;
    PTR_RPM_DUTY ptrRpmDuty;
    PTR_DEVICE_CONFIG ptrDeviceConfig;
} PWM_TACH_DEVICE, *PTR_PWM_TACH_DEVICE;

extern RPM_DUTY gRpmDuty[8];

void pwm_device_add(PTR_DEVICE_CONFIG ptrDeviceConfig);

#endif //QEMU_PWM_DEVICE_H
