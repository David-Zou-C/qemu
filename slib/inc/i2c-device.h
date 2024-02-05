//
// Created by 25231 on 2023/8/23.
//

#ifndef QEMU_I2C_DEVICE_H
#define QEMU_I2C_DEVICE_H

#include "stdint.h"
#include "slib-common.h"

#define I2C_DEVICE_TYPE_ID_OFFSET 101
#define I2C_DEVICE_TOTAL_NUM 5

typedef struct I2C_DEVICE_DATA_ {
    uint8_t *data_buf;
    uint8_t *active_data;
    uint8_t *passive_data;
    PTR_DEVICE_CONFIG ptrDeviceConfig;
} I2C_DEVICE_DATA, *PTR_I2C_DEVICE_DATA;


#include "aspeed-init.h"

/**************************************** Device 0 ****************************************/
#define I2C_EEPROM_TYPE_MAX_MONITOR_DATA_LEN 256
typedef struct I2C_EEPROM_sTYPE_ {
    pthread_mutex_t mutex;
    uint32_t offset; /* 偏移地址 */
    uint8_t *buf;   /* 存储内容缓冲区 */
    uint32_t total_size; /* EEPROM 的总容量大小，也是 buf 的大小 */
    uint8_t addr_size; /* 地址的字节表述数， > 256 就用 2 个字节作为地址表示，<=256 就为 1 个字节 */
    uint8_t have_addr_size; /* 已收到 addr 的数量，因为每次 send 一个字节都会进行处理，如果 addr_size 是 2，那么需要记录已收到的 addr_size */
    uint64_t receive_times;
    uint64_t write_times;
    uint32_t monitor_data[I2C_EEPROM_TYPE_MAX_MONITOR_DATA_LEN];
    uint32_t monitor_data_len;
    uint32_t device_index;
} I2C_EEPROM_sTYPE, *PTR_I2C_EEPROM_sTYPE;


void init_I2cEmptyDevice0(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cEmptyDevice0(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cEmptyDevice0(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cEmptyDevice0(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

/**************************************** SAS EXP BP CPLD ****************************************/
#define I2C_BP_MAX_HDD_NUM 64

typedef struct BP_HDD_STATUS_ {
    uint8_t Fail :1;
    uint8_t Rebuild :1;
    uint8_t Locate :1;
    uint8_t Idle_Act :1;
    uint8_t Present :2;
    uint8_t Warning :1;
    uint8_t Predictive_Fail :1;
} BP_HDD_STATUS, *PTR_BP_HDD_STATUS;

typedef struct I2C_BP_CPLD_sTYPE_DATA_ {
    /* 0x74: CPLD/MCU 厂商信息标识 */
    uint8_t cpld_manufacture;
    /* 0x90: 背板支持最大硬盘数量（限Exp背板） */
    uint8_t max_hdd_num;
    /* 0x91: 获取硬盘状态 */
    BP_HDD_STATUS hdd_status[I2C_BP_MAX_HDD_NUM];
    /* 0x92: BMC Locate 硬盘操作 */
    uint8_t hdd_locate[I2C_BP_MAX_HDD_NUM];
    /* 0x93: BMC 指示 NVMe 盘的 Fail 和 Rebuild 状态 */
    uint8_t hdd_fail_rebuild[I2C_BP_MAX_HDD_NUM];
    /* 0x94: BMC 控制 Predictive Fail 状态灯 */
    uint8_t hdd_pred_fail[I2C_BP_MAX_HDD_NUM];
    /* 0x95: BMC 控制 Warning 状态灯 */
    uint8_t hdd_warning[I2C_BP_MAX_HDD_NUM];
    /* 0x96: 获取板卡 CPLD/MCU 版本信息 */
    uint8_t revision;
    /* 0x97: 获取背板型号配置信息 */
    uint8_t bp_type_id;
    /* 0x98: update flag */
    uint8_t update_flag;
    /* 0x99: 获取背板上指定硬盘槽位的外部 efuse 状态 */
    uint8_t hdd_efuse_status[I2C_BP_MAX_HDD_NUM];
    /* 0x9A: BMC 控制指定硬盘槽位外部 efuse 操作 */
    uint8_t hdd_control_efuse[I2C_BP_MAX_HDD_NUM];
    /* 0x9B: SAS Exp 厂商信息标识 */
    uint8_t exp_manufacture;
    /* 0xA0-0xBF: 获取硬盘背板命名 */
    char bp_name[32];
} I2C_BP_CPLD_sTYPE_DATA, *PTR_I2C_BP_CPLD_sTYPE_DATA;


typedef struct I2C_BP_CPLD_sTYPE_ {
    pthread_mutex_t mutex;
    uint8_t offset;
    uint8_t has_sent_len;
    uint8_t op_data[10]; /* 操作数据记录 */
    I2C_BP_CPLD_sTYPE_DATA i2CBpCpldData;  /* 数据存储区 */
    uint64_t receive_times;
    uint64_t write_times;
    uint32_t device_index;
} I2C_BP_CPLD_sTYPE, *PTR_I2C_BP_CPLD_sTYPE;

void init_I2cEmptyDevice1(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cEmptyDevice1(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cEmptyDevice1(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cEmptyDevice1(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void init_I2cEmptyDevice2(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cEmptyDevice2(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cEmptyDevice2(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cEmptyDevice2(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void init_I2cEmptyDevice3(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cEmptyDevice3(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cEmptyDevice3(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cEmptyDevice3(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void init_I2cEmptyDevice4(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cEmptyDevice4(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cEmptyDevice4(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cEmptyDevice4(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

/**************************************** DIMM_TEMP ****************************************/
typedef struct I2C_LEGACY_DIMM_TMP_sTYPE_ {
    pthread_mutex_t mutex;
    uint8_t temperature;
    uint8_t offset;
    uint64_t receive_times;
    uint64_t write_times;
    uint32_t device_index;
} I2C_LEGACY_DIMM_TMP_sTYPE, *PTR_I2C_LEGACY_DIMM_TMP_sTYPE;

typedef struct I2C_DIMM_TMP_RWCNT_sTYPE_ {
    uint64_t *receive_times;
    uint64_t *write_times;
} I2C_DIMM_TMP_RWCNT_sTYPE, *PTR_I2C_DIMM_TMP_RWCNT_sTYPE;

/**************************************** I2C Legacy ****************************************/
void init_I2cLegacyEmptyDevice0(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cLegacyEmptyDevice0(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cLegacyEmptyDevice0(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cLegacyEmptyDevice0(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void init_I2cLegacyEmptyDevice1(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cLegacyEmptyDevice1(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cLegacyEmptyDevice1(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cLegacyEmptyDevice1(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void init_I2cLegacyEmptyDevice2(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cLegacyEmptyDevice2(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cLegacyEmptyDevice2(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cLegacyEmptyDevice2(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void init_I2cLegacyEmptyDevice3(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cLegacyEmptyDevice3(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cLegacyEmptyDevice3(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cLegacyEmptyDevice3(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void init_I2cLegacyEmptyDevice4(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

int event_I2cLegacyEmptyDevice4(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

uint8_t recv_I2cLegacyEmptyDevice4(PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

void send_I2cLegacyEmptyDevice4(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData);

/**************************************** 自定义设备添加函数 ****************************************/
// 定义函数指针类型
typedef void (*I2cFunctionPtr)(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I2cEmptyDevice0_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I2cEmptyDevice1_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I2cEmptyDevice2_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I2cEmptyDevice3_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I2cEmptyDevice4_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

I2cFunctionPtr getI2cDeviceAddFunc(int device_type_id);

int getI2cDeviceTypeId(I2cFunctionPtr functionPtr);

void I2cLegacyEmptyDevice0_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I2cLegacyEmptyDevice1_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I2cLegacyEmptyDevice2_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I2cLegacyEmptyDevice3_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I2cLegacyEmptyDevice4_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

I2cFunctionPtr getI2cLegacyDeviceAddFunc(int device_type_id);

#endif //QEMU_I2C_DEVICE_H
