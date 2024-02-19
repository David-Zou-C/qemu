//
// Created by zhanghong on 2024/1/11.
//

#ifndef QEMU_I3C_DEVICE_H
#define QEMU_I3C_DEVICE_H

#include "stdint.h"
#include "slib-common.h"

#define I3C_DEVICE_TYPE_ID_OFFSET 701
#define I3C_DEVICE_TOTAL_NUM 4

typedef struct I3C_DEVICE_DATA_ {
    uint8_t *data_buf;
    uint8_t *active_data;
    uint8_t *passive_data;
    PTR_DEVICE_CONFIG ptrDeviceConfig;
} I3C_DEVICE_DATA, *PTR_I3C_DEVICE_DATA;

#include "aspeed-init.h"

/**************************************** DIMM_TEMP ****************************************/
typedef struct I3C_DIMM_TMP_sTYPE_ {
    pthread_mutex_t mutex;
    uint8_t temperature;
    uint8_t offset;
    uint64_t receive_times;
    uint64_t write_times;
    uint32_t device_index;
} I3C_DIMM_TMP_sTYPE, *PTR_I3C_DIMM_TMP_sTYPE;

void init_I3cEmptyDevice0(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
uint8_t recv_I3cEmptyDevice0(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int send_I3cEmptyDevice0(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int event_I3cEmptyDevice0(uint8_t event, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
uint8_t ccc_read_I3cEmptyDevice0(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int ccc_write_I3cEmptyDevice0(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
void init_I3cEmptyDevice1(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
uint8_t recv_I3cEmptyDevice1(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int send_I3cEmptyDevice1(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int event_I3cEmptyDevice1(uint8_t event, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
uint8_t ccc_read_I3cEmptyDevice1(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int ccc_write_I3cEmptyDevice1(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
void init_I3cEmptyDevice2(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
uint8_t recv_I3cEmptyDevice2(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int send_I3cEmptyDevice2(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int event_I3cEmptyDevice2(uint8_t event, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
uint8_t ccc_read_I3cEmptyDevice2(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int ccc_write_I3cEmptyDevice2(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
void init_I3cEmptyDevice3(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
uint8_t recv_I3cEmptyDevice3(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int send_I3cEmptyDevice3(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int event_I3cEmptyDevice3(uint8_t event, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
uint8_t ccc_read_I3cEmptyDevice3(PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
int ccc_write_I3cEmptyDevice3(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData);
/**************************************** 自定义设备添加函数 ****************************************/
// 定义函数指针类型
typedef void (*I3cFunctionPtr)(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I3cEmptyDevice0_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I3cEmptyDevice1_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I3cEmptyDevice2_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

void I3cEmptyDevice3_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig);

I3cFunctionPtr getI3cDeviceAddFunc(int device_type_id);

int getI3cDeviceTypeId(I3cFunctionPtr functionPtr);

#endif //QEMU_I3C_DEVICE_H
