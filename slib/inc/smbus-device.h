//
// Created by 25231 on 2023/8/8.
//

#ifndef QEMU_SMBUS_DEVICE_H
#define QEMU_SMBUS_DEVICE_H

#include <stdint.h>
#include "slib-common.h"

#define SMBUS_DEVICE_TYPE_ID_OFFSET 1
#define SMBUS_DEVICE_TOTAL_NUM 10

typedef struct SMBUS_DEVICE_DATA_ {
    uint8_t *data_buf;
    uint8_t *active_data;
    uint8_t *passive_data;
    PTR_DEVICE_CONFIG ptrDeviceConfig;
} SMBUS_DEVICE_DATA, *PTR_SMBUS_DEVICE_DATA;


#include "aspeed-init.h"

/****************************************  ****************************************/
#define SMBUS_EEPROM_BUF_SIZE 256

typedef struct SMBUS_EEPROM_sTYPE_ {
    pthread_mutex_t mutex;
    uint8_t buf[SMBUS_EEPROM_BUF_SIZE];
    uint8_t offset;
    uint64_t receive_times;
    uint64_t write_times;
    uint32_t device_index;
} SMBUS_EEPROM_sTYPE, *PTR_SMBUS_EEPROM_sTYPE;

void init_SMBusEmptyDevice0(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice0(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice0(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);


/****************************************  ****************************************/
typedef struct SMBUS_TMP_sTYPE_ {
    pthread_mutex_t mutex;
    uint8_t temperature_integer;
    uint8_t temperature_decimal;
    uint8_t offset;
    uint64_t receive_times;
    uint64_t write_times;
    uint32_t device_index;
} SMBUS_TMP_sTYPE, *PTR_SMBUS_TMP_sTYPE;

void init_SMBusEmptyDevice1(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice1(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice1(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);


/****************************************  ****************************************/
typedef struct SMBUS_TPA626_sTYPE_ {
    pthread_mutex_t mutex;
    uint16_t configuration_reg00; /* 00h  R/W 配置寄存器，初始值为： 0x41 0x27 */
    uint16_t shunt_vol_reg01; /* 01h R 并联电压寄存器，初始值为： 0x00 0x00 */
    uint16_t bus_vol_reg02;  /* 02h R Bus电压寄存器，初始值为：0x00 0x00 */
    uint16_t power_reg03; /* 03h R 电源功率寄存器，初始值为：0x00 0x00 */
    uint16_t current_reg04; /* 04h R 电流寄存器，初始值为：0x00 0x00 */
    uint16_t calibration_reg05; /* 05h R/W 校准寄存器 */
    uint16_t mask_enable_reg06; /* 06h R/W 警报配置和转换就绪标志 */
    uint16_t alert_limit_reg07; /* 07h R/W 警报函数和极限值 */
    uint16_t manufacturer_id_regfe; /* feh R 制造商标识，初始值为： 0x5549 */
    uint16_t die_id_regff; /* ffh R 模具ID，初始值为：0x2260 */
    uint8_t offset;
    uint8_t reg_val_offset; /* 0或1 由于一个寄存器需要返回两个数据，且每次读取都是一个，此值用来计数某个寄存器值的偏移 */
    uint64_t receive_times;
    uint64_t write_times;
    uint32_t device_index;
} SMBUS_TPA626_sTYPE, *PTR_SMBUS_TPA626_sTYPE;


void init_SMBusEmptyDevice2(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice2(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice2(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);


/**************************************** DIMM_TEMP ****************************************/
typedef struct SMBUS_DIMM_TMP_sTYPE_ {
    pthread_mutex_t mutex;
    uint8_t temperature;
    uint8_t offset;
    uint64_t receive_times;
    uint64_t write_times;
    uint32_t device_index;
} SMBUS_DIMM_TMP_sTYPE, *PTR_SMBUS_DIMM_TMP_sTYPE;


void init_SMBusEmptyDevice3(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice3(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice3(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);

void init_SMBusEmptyDevice4(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice4(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice4(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);

void init_SMBusEmptyDevice5(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice5(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice5(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);

void init_SMBusEmptyDevice6(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice6(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice6(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);

void init_SMBusEmptyDevice7(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice7(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice7(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);

void init_SMBusEmptyDevice8(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice8(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice8(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);

void init_SMBusEmptyDevice9(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
uint8_t receive_SMBusEmptyDevice9(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);
int write_SMBusEmptyDevice9(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData);


/**************************************** 自定义设备添加函数 ****************************************/
// 定义函数指针类型
typedef void (*SMBusFunctionPtr)(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void SMBusEmptyDevice0_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice1_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice2_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice3_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice4_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice5_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice6_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice7_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice8_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice9_add(void *smbus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void SMBusEmptyDevice0_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice1_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice2_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice3_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice4_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice5_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice6_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice7_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice8_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);
void SMBusEmptyDevice9_qdev_get_config(void *dev, PTR_DEVICE_CONFIG ptrDeviceConfig);

SMBusFunctionPtr getSMBusDeviceAddFunc(int device_type_id);
int getSMBusDeviceTypeId(SMBusFunctionPtr functionPtr);

#endif //QEMU_SMBUS_DEVICE_H
