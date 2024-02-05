//
// Created by zhanghong on 2024/1/11.
//

#include "slib/inc/i3c-device.h"

typedef enum I3CEvent {
    I3C_START_RECV,
    I3C_START_SEND,
    I3C_STOP,
    I3C_NACK,
} I3CEvent;

/**************************************** I3cEmptyDevice0 ****************************************/
void init_I3cEmptyDevice0(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    PTR_I3C_DIMM_TMP_sTYPE ptrI3cDimmTmpSType;
    if (ptrI3cDeviceData->data_buf == NULL) {
        ptrI3cDeviceData->data_buf = (uint8_t *) malloc(sizeof(I3C_DIMM_TMP_sTYPE));
        memset(ptrI3cDeviceData->data_buf, 0, sizeof(I3C_DIMM_TMP_sTYPE));
        ptrI3cDimmTmpSType = (PTR_I3C_DIMM_TMP_sTYPE) ptrI3cDeviceData->data_buf;
        /* 第一次创建，需要初始化互斥锁 */
        pthread_mutex_init(&ptrI3cDimmTmpSType->mutex, NULL);
        ptrI3cDimmTmpSType->device_index = get_device_index(ptrI3cDeviceData);
    } else {
        /* 初始值 */
        ptrI3cDimmTmpSType = (PTR_I3C_DIMM_TMP_sTYPE) ptrI3cDeviceData->data_buf;
    }
    /* 根据参数修改数据 */
    dynamic_change_data(I3C_DIMM_TEMP, ptrI3cDeviceData, ptrI3cDeviceData->ptrDeviceConfig->args);
    ptrI3cDimmTmpSType->offset = 0;
}

uint8_t recv_I3cEmptyDevice0(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
#if 0
    uint8_t ret;
    PTR_I3C_DIMM_TMP_sTYPE ptrI3cDimmTmpSType = (PTR_I3C_DIMM_TMP_sTYPE) ptrI3cDeviceData->data_buf;

    printf("%x, %x\n", ptrI3cDimmTmpSType->offset, ptrI3cDimmTmpSType->temperature);
    ptrI3cDimmTmpSType->receive_times ++;
    if (ptrI3cDimmTmpSType->offset == 0) {
        ptrI3cDimmTmpSType->offset = 1;
        ret = ptrI3cDimmTmpSType->temperature >> 4;
        return ret;
    } else if (ptrI3cDimmTmpSType->offset == 1) {
        ptrI3cDimmTmpSType->offset = 0;
        ret = ptrI3cDimmTmpSType->temperature << 4;
        return ret;
    } else {
        ptrI3cDimmTmpSType->offset = 0;
        ret = ptrI3cDimmTmpSType->temperature >> 4;
        return ret;
    }
#endif
    return 0;
}

int send_I3cEmptyDevice0(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
#if 0
    printf("send_I3cEmptyDevice0 trigger\n");
    PTR_I3C_DIMM_TMP_sTYPE ptrI3cDimmTmpSType = (PTR_I3C_DIMM_TMP_sTYPE) ptrI3cDeviceData->data_buf;

    ptrI3cDimmTmpSType->write_times ++;
    /* 有写操作，直接重置 offset */
    ptrI3cDimmTmpSType->offset = 0;
#endif
    return 0;
}

int event_I3cEmptyDevice0(uint8_t event, PTR_I3C_DEVICE_DATA ptrI3cDeviceData)
{
#if 0
    switch (event) {
    case I3C_START_RECV:
        break;
    case I3C_START_SEND:
        break;
    case I3C_STOP:
        //ptrI3cDimmTmpSType->offset = 0;
        break;
    case I3C_NACK:
        break;
    default:
        return -1;
    }
#endif
    return 0;
}

uint8_t ccc_read_I3cEmptyDevice0(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
#if 0
    uint8_t ret = 0;
    PTR_I3C_DIMM_TMP_sTYPE ptrI3cDimmTmpSType = (PTR_I3C_DIMM_TMP_sTYPE) ptrI3cDeviceData->data_buf;

    ptrI3cDimmTmpSType->receive_times ++;
    if (ptrI3cDimmTmpSType->offset == 0) {
        ptrI3cDimmTmpSType->offset = 1;
        ret = ptrI3cDimmTmpSType->temperature >> 4;
        printf("ccc_read_I3cEmptyDevice0 trigger:%d\n", ptrI3cDimmTmpSType->temperature);
        return ret;
    } else if (ptrI3cDimmTmpSType->offset == 1) {
        ptrI3cDimmTmpSType->offset = 0;
        ret = ptrI3cDimmTmpSType->temperature << 4;
        printf("ccc_read_I3cEmptyDevice0 trigger:%d\n", ptrI3cDimmTmpSType->temperature);
        return ret;
    } else {
        ptrI3cDimmTmpSType->offset = 0;
        ret = ptrI3cDimmTmpSType->temperature >> 4;
        printf("ccc_read_I3cEmptyDevice0 trigger:%d\n", ptrI3cDimmTmpSType->temperature);
        return ret;
    }
#endif
    return 0;
}

int ccc_write_I3cEmptyDevice0(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
#if 0
    PTR_I3C_DIMM_TMP_sTYPE ptrI3cDimmTmpSType = (PTR_I3C_DIMM_TMP_sTYPE) ptrI3cDeviceData->data_buf;
    printf("ccc_write_I3cEmptyDevice0 trigger：%d, %x, %d\n", ptrI3cDimmTmpSType->temperature, ptrI3cDimmTmpSType->offset, ptrI3cDimmTmpSType->device_index);
    ptrI3cDimmTmpSType->write_times ++;
    /* 有写操作，直接重置 offset */
    ptrI3cDimmTmpSType->offset = 0;
#endif
    return 0;
}

/**************************************** Device 1 ****************************************/
void init_I3cEmptyDevice1(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
};

uint8_t recv_I3cEmptyDevice1(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}

int send_I3cEmptyDevice1(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}
int event_I3cEmptyDevice1(uint8_t event, PTR_I3C_DEVICE_DATA ptrI3cDeviceData){
    return 0;
}

uint8_t ccc_read_I3cEmptyDevice1(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}

int ccc_write_I3cEmptyDevice1(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}

/**************************************** Device 2 ****************************************/
void init_I3cEmptyDevice2(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
};

uint8_t recv_I3cEmptyDevice2(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}

int send_I3cEmptyDevice2(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}
int event_I3cEmptyDevice2(uint8_t event, PTR_I3C_DEVICE_DATA ptrI3cDeviceData){
    return 0;
}

uint8_t ccc_read_I3cEmptyDevice2(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}

int ccc_write_I3cEmptyDevice2(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}

/**************************************** Device 3 ****************************************/
void init_I3cEmptyDevice3(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
};

uint8_t recv_I3cEmptyDevice3(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}

int send_I3cEmptyDevice3(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}
int event_I3cEmptyDevice3(uint8_t event, PTR_I3C_DEVICE_DATA ptrI3cDeviceData){
    return 0;
}

uint8_t ccc_read_I3cEmptyDevice3(PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}

int ccc_write_I3cEmptyDevice3(const unsigned char *buf, PTR_I3C_DEVICE_DATA ptrI3cDeviceData) {
    return 0;
}