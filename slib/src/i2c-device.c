//
// Created by 25231 on 2023/8/23.
//

#include "i2c-device.h"

enum i2c_event {
    I2C_START_RECV,
    I2C_START_SEND,
    I2C_START_SEND_ASYNC,
    I2C_FINISH,
    I2C_NACK /* Masker NACKed a reception byte.  */
};

/**************************************** Device 0  ****************************************/
void init_I2cEmptyDevice0(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    FUNC_DEBUG("function: init_I2cEmptyDevice0()")
    PTR_I2C_EEPROM_sTYPE ptrI2CEepromSType;
    static int inited = FALSE;
    /* 先确定 data_buf */
    if (ptrI2CDeviceData->data_buf == NULL) {
        /* 为 NULL，表示未曾初始化过 */
        ptrI2CDeviceData->data_buf = (uint8_t *) malloc(sizeof(I2C_EEPROM_sTYPE));
        memset(ptrI2CDeviceData->data_buf, 0, sizeof(I2C_EEPROM_sTYPE));
        ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) ptrI2CDeviceData->data_buf;
        /* 如果是第一次创建，则需要进行互斥锁的初始化操作 */
        pthread_mutex_init(&ptrI2CEepromSType->mutex, NULL);
        ptrI2CEepromSType->device_index = get_device_index(ptrI2CDeviceData);
    } else {
        ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) ptrI2CDeviceData->data_buf;
    }

    /**************************************** 普通重置区 ****************************************/
    /* 1. offset 置 0 */
    ptrI2CEepromSType->offset = 0;
    /* 2. 重新初始化 buf 值 */
    if (inited == FALSE) {
        /* 数据的改变，只应一次，EEPROM数据需要改变后仍能保持 */
        dynamic_change_data(I2C_EEPROM, ptrI2CDeviceData, ptrI2CDeviceData->ptrDeviceConfig->args);
        inited = TRUE;
    }
    /**************************************** 普通重置区 ****************************************/


};

int event_I2cEmptyDevice0(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
//    FUNC_DEBUG("function: event_I2cEmptyDevice0()")
    PTR_I2C_EEPROM_sTYPE ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) ptrI2CDeviceData->data_buf;
    switch (i2CEvent) {
        case I2C_START_SEND:
            /* 开始发送数据 前 */

        case I2C_FINISH:
            /* 一次 I2C 通信结束，即一次连续地写或读结束了 */
            /* 刚开始写，或一次I2c流程结束了，都应该清掉 have_addr_size */
            ptrI2CEepromSType->have_addr_size = 0;
            /* fallthrough */
        case I2C_START_RECV:
            /* ! 此处不能将 have_addr_size 重置，
             * 因为在 recv 中还需要判断 have_addr_size 的大小，确定是否地址传完了
             * I2C master 开始要求读数据 */
            break;
        case I2C_NACK:
            break;
        default:
            return -1; /* 其他状态不被支持 */
    }
    return 0;
};

uint8_t recv_I2cEmptyDevice0(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
//    FUNC_DEBUG("function: recv_I2cEmptyDevice0()")

    static char log_temp[1024];

    PTR_I2C_EEPROM_sTYPE ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) ptrI2CDeviceData->data_buf;
    uint8_t ret;

    ptrI2CEepromSType->receive_times++;
    /* 如果已获得地址字节，但是还没有发送来要求的地址字节数量，就开始要求读操作，则应直接返回无效值 - 0xff 表示 */
    if ((ptrI2CEepromSType->have_addr_size > 0) && (ptrI2CEepromSType->have_addr_size < ptrI2CEepromSType->addr_size)) {
        sprintf(log_temp, "device_index - '%d' master recv data but have_addr_size - '%d' < addr_size - '%d' \n",
                ptrI2CEepromSType->device_index,
                ptrI2CEepromSType->have_addr_size,
                ptrI2CEepromSType->addr_size);
        file_log(log_temp, LOG_TIME_END);
        return 0xff;
    }
    ret = ptrI2CEepromSType->buf[ptrI2CEepromSType->offset];
    ptrI2CEepromSType->offset = (ptrI2CEepromSType->offset + 1u) % ptrI2CEepromSType->total_size;

    return ret;
};

void send_I2cEmptyDevice0(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
//    FUNC_DEBUG("function: send_I2cEmptyDevice0()")

    PTR_I2C_EEPROM_sTYPE ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) ptrI2CDeviceData->data_buf;

    /* 逻辑处理前，先获取锁 */
    ptrI2CEepromSType->write_times++;
    if (ptrI2CEepromSType->have_addr_size < ptrI2CEepromSType->addr_size) {
        /* 小于地址要求的字节数，此个 data 为地址值 */
        ptrI2CEepromSType->offset <<= 8; /* 原 offset 左移 */
        ptrI2CEepromSType->offset |= data; /* 将此 data 加上 */
        ptrI2CEepromSType->have_addr_size++; /* 已收到一个地址 */
        if (ptrI2CEepromSType->have_addr_size == ptrI2CEepromSType->addr_size) {
            /* 如果收到一个地址后，已达到 地址要求的字节数，那就确认此次操作的地址 */
            ptrI2CEepromSType->offset %= ptrI2CEepromSType->total_size; /* 偏移地址检查，避免越界 */
        }
    } else {
        /* 已接收的地址数 已等于 要求的地址了，且在之前，偏移地址也已确定，
         * 那么此处再次接收到数据，则意味着此 data 为要写入的数据，
         * 除非结束一次 I2C 流程，在事件处理中 清掉 have_addr_size，
         * 否则这种连续的数据，就意味着连续的写入 */
        ptrI2CEepromSType->buf[ptrI2CEepromSType->offset] = data;
        ptrI2CEepromSType->offset = (ptrI2CEepromSType->offset + 1) % ptrI2CEepromSType->total_size; /* 地址向下递增 */
    }
};


/**************************************** Device 1 ****************************************/
void init_I2cEmptyDevice1(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    PTR_I2C_BP_CPLD_sTYPE ptrI2CBpCpldSType;
    /* 确定 data_buf */
    if (ptrI2CDeviceData->data_buf == NULL) {
        /* 分配空间 */
        ptrI2CDeviceData->data_buf = (uint8_t *) malloc(sizeof(I2C_BP_CPLD_sTYPE));
        memset(ptrI2CDeviceData->data_buf, 0, sizeof(PTR_I2C_BP_CPLD_sTYPE));
        ptrI2CBpCpldSType = (PTR_I2C_BP_CPLD_sTYPE) ptrI2CDeviceData->data_buf;
        /* 初始化线程锁 */
        pthread_mutex_init(&ptrI2CBpCpldSType->mutex, NULL);
        ptrI2CBpCpldSType->device_index = get_device_index(ptrI2CDeviceData);
        /* 释放锁 */
    } else {
        ptrI2CBpCpldSType = (PTR_I2C_BP_CPLD_sTYPE) ptrI2CDeviceData->data_buf;
    }
    ptrI2CBpCpldSType->offset = 0;
    dynamic_change_data(I2C_BP_CPLD, ptrI2CDeviceData, ptrI2CDeviceData->ptrDeviceConfig->args);

};

int event_I2cEmptyDevice1(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    PTR_I2C_BP_CPLD_sTYPE ptrI2CBpCpldSType = (PTR_I2C_BP_CPLD_sTYPE) ptrI2CDeviceData->data_buf;
    switch (i2CEvent) {
        case I2C_START_SEND:
            /* 开始发送数据 前 */
            ptrI2CBpCpldSType->has_sent_len = 0;

            /* fallthrough */
        case I2C_FINISH:
            /* 一次 I2C 通信结束，即一次连续地写或读结束了 */
            /* 刚开始写，或一次I2c流程结束了，都应该清掉 have_addr_size */

            /* fallthrough */
        case I2C_START_RECV:
            /* ! 此处不能将 have_addr_size 重置，
             * 因为在 recv 中还需要判断 have_addr_size 的大小，确定是否地址传完了
             * I2C master 开始要求读数据 */
            ptrI2CBpCpldSType->offset = 0;
            break;
        case I2C_NACK:
            break;
        default:
            return -1; /* 其他状态不被支持 */
    }
    return 0;
};

uint8_t recv_I2cEmptyDevice1(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    PTR_I2C_BP_CPLD_sTYPE ptrI2CBpCpldSType = (PTR_I2C_BP_CPLD_sTYPE) ptrI2CDeviceData->data_buf;
    uint8_t ret = 0xff;
    ptrI2CBpCpldSType->receive_times++;

    if (ptrI2CBpCpldSType->has_sent_len == 0) {
        return 0xff;
    }

    PTR_I2C_BP_CPLD_sTYPE_DATA ptrI2CBpCpldSTypeData = &ptrI2CBpCpldSType->i2CBpCpldData;
    uint8_t hdd_num = 0;
    uint8_t op_code;
    /* 只有一个操作数的时候 */
    switch (ptrI2CBpCpldSType->op_data[0]) {
        case 0x74:  /* 0x74 RO: CPLD/MCU 厂商信息标识 */
            ret = ptrI2CBpCpldSTypeData->cpld_manufacture;
            break;
        case 0x90:  /* 0x90 RO: 背板支持最大硬盘数量（限Exp背板） */
            ret = ptrI2CBpCpldSTypeData->max_hdd_num;
            break;
        case 0x91:  /* 0x91 RO: 获取硬盘状态 */
            ret = 0xff;
            if (ptrI2CBpCpldSType->has_sent_len >= 2) {
                hdd_num = ptrI2CBpCpldSType->op_data[1];
                if (hdd_num < ptrI2CBpCpldSTypeData->max_hdd_num) {
                    ret = *(uint8_t *)&ptrI2CBpCpldSTypeData->hdd_status[hdd_num];
                }
            }
            break;
        case 0x92:  /* 0x92 WO: BMC Locate 硬盘操作 */
            if (ptrI2CBpCpldSType->has_sent_len >= 2) {
                hdd_num = ptrI2CBpCpldSType->op_data[1] & 0b0111111;
                op_code = ptrI2CBpCpldSType->op_data[1] >> 7;
                if (hdd_num < ptrI2CBpCpldSTypeData->max_hdd_num) {
                    if (op_code) {
                        /* 点亮 Locate */
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Locate = 1;
                    } else {
                        /* 关闭 Locate */
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Locate = 0;
                    }
                }
            }
            ret = 0xff;
            break;
        case 0x93:  /* BMC 指示 NVMe 盘的 Fail 和 Rebuild 状态 */
            if (ptrI2CBpCpldSType->has_sent_len >= 2) {
                hdd_num = ptrI2CBpCpldSType->op_data[1] & 0b00111111;
                op_code = ptrI2CBpCpldSType->op_data[1] >> 6;
                if (hdd_num < ptrI2CBpCpldSTypeData->max_hdd_num) {
                    if (op_code == 0b01) {
                        /* 点亮 Fail，同时关闭 Rebuild */
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Fail = 1;
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Rebuild = 0;
                    } else if (op_code == 0b11){
                        /* 点亮 Rebuild，同时关闭 Fail */
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Fail = 0;
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Rebuild = 1;
                    } else if (op_code == 0b00) {
                        /* 都关闭 */
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Fail = 0;
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Rebuild = 0;
                    }
                }
            }
            ret = 0xff;
            break;
        case 0x94:  /* BMC 控制 Predictive Fail 状态灯 */
            if (ptrI2CBpCpldSType->has_sent_len >= 2) {
                hdd_num = ptrI2CBpCpldSType->op_data[1] & 0b01111111;
                op_code = ptrI2CBpCpldSType->op_data[1] >> 7;
                if (hdd_num < ptrI2CBpCpldSTypeData->max_hdd_num) {
                    if (op_code) {
                        /* 点亮 Pre fail */
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Predictive_Fail = 1;
                    } else {
                        /* 关闭 Pre fail */
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Predictive_Fail = 0;
                    }
                }
            }
            ret = 0xff;
            break;
        case 0x95:  /* BMC 控制 Warning 状态灯 */
            if (ptrI2CBpCpldSType->has_sent_len >= 2) {
                hdd_num = ptrI2CBpCpldSType->op_data[1] & 0b01111111;
                op_code = ptrI2CBpCpldSType->op_data[1] >> 7;
                if (hdd_num < ptrI2CBpCpldSTypeData->max_hdd_num) {
                    if (op_code) {
                        /* 点亮 Warning */
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Warning = 1;
                    } else {
                        /* 关闭 Warning */
                        ptrI2CBpCpldSTypeData->hdd_status[hdd_num].Warning = 0;
                    }
                }
            }
            ret = 0xff;
            break;
        case 0x96:  /* 获取板卡 CPLD/MCU 版本信息 */
            ret = ptrI2CBpCpldSTypeData->revision;
            break;
        case 0x97:  /* 获取背板型号配置信息 */
            ret = ptrI2CBpCpldSTypeData->bp_type_id;
            break;
        case 0x98:
            ret = ptrI2CBpCpldSTypeData->update_flag;
            break;
        case 0x99:
            ret = 0xff;
            if (ptrI2CBpCpldSType->has_sent_len >= 2) {
                hdd_num = ptrI2CBpCpldSType->op_data[1];
                if (hdd_num < I2C_BP_MAX_HDD_NUM) {
                    ret = ptrI2CBpCpldSTypeData->hdd_efuse_status[hdd_num];
                }
            }
            break;
        case 0x9A:
            ret = 0xff;
            break;
        case 0x9B:
            ret = ptrI2CBpCpldSTypeData->exp_manufacture;
            break;
        case 0xA0 ... 0xBF:
            ret = ptrI2CBpCpldSTypeData->bp_name[ptrI2CBpCpldSType->op_data[0] - 0xA0];
            break;
        default:
            ret = 0xff;
            break;
    }


    return ret;
};

void send_I2cEmptyDevice1(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    PTR_I2C_BP_CPLD_sTYPE ptrI2CBpCpldSType = (PTR_I2C_BP_CPLD_sTYPE) ptrI2CDeviceData->data_buf;

    ptrI2CBpCpldSType->write_times++;
    ptrI2CBpCpldSType->op_data[ptrI2CBpCpldSType->has_sent_len] = data;
    ptrI2CBpCpldSType->has_sent_len ++;
};

/**************************************** Device 2 ****************************************/

void init_I2cEmptyDevice2(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    PTR_I2C_DIMM_TMP_sTYPE ptrI2CDimmTmpSType;
    if (ptrI2CDeviceData->data_buf == NULL) {
        ptrI2CDeviceData->data_buf = (uint8_t *) malloc(sizeof(I2C_DIMM_TMP_sTYPE));
        memset(ptrI2CDeviceData->data_buf, 0, sizeof(I2C_DIMM_TMP_sTYPE));
        ptrI2CDimmTmpSType = (PTR_I2C_DIMM_TMP_sTYPE) ptrI2CDeviceData->data_buf;
        /* 初始化互斥锁 */
        pthread_mutex_init(&ptrI2CDimmTmpSType->mutex, NULL);
        ptrI2CDimmTmpSType->device_index = get_device_index(ptrI2CDeviceData);
    } else {
        ptrI2CDimmTmpSType = (PTR_I2C_DIMM_TMP_sTYPE) ptrI2CDeviceData->data_buf;
    }

    dynamic_change_data(I2C_DIMM_TEMP, ptrI2CDeviceData, ptrI2CDeviceData->ptrDeviceConfig->args);
    ptrI2CDimmTmpSType->offset = 0;
}

uint8_t recv_I2cEmptyDevice2(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    uint8_t ret;
    PTR_I2C_DIMM_TMP_sTYPE ptrI2CDimmTmpSType = (PTR_I2C_DIMM_TMP_sTYPE) ptrI2CDeviceData->data_buf;

    ptrI2CDimmTmpSType->receive_times++;
    if (ptrI2CDimmTmpSType->offset == 0) {
        ptrI2CDimmTmpSType->offset = 1;
        ret = ptrI2CDimmTmpSType->temperature >> 4;
        return ret;
    } else if (ptrI2CDimmTmpSType->offset == 1) {
        ptrI2CDimmTmpSType->offset = 0;
        ret = ptrI2CDimmTmpSType->temperature << 4;
        return ret;
    } else {
        ptrI2CDimmTmpSType->offset = 0;
        ret = ptrI2CDimmTmpSType->temperature >> 4;
        return ret;
    }
    return 0;
}

void send_I2cEmptyDevice2(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    PTR_I2C_DIMM_TMP_sTYPE ptrI2CDimmTmpSType = (PTR_I2C_DIMM_TMP_sTYPE) ptrI2CDeviceData->data_buf;

    ptrI2CDimmTmpSType->write_times ++;
    /* 写操作，重置offset */
    ptrI2CDimmTmpSType->offset = 0;
}

int event_I2cEmptyDevice2(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2cDeviceData) {
    switch (i2CEvent) {
        case I2C_START_SEND:
            /* 开始发送数据 前 */
        case I2C_FINISH:
            /* 一次 I2C 通信结束，即一次连续地写或读结束了 */
            /* fallthrough */
        case I2C_START_RECV:
            break;
        case I2C_NACK:
            break;
        default:
            return -1; /* 其他状态不被支持 */
    }
    return 0;
}

/**************************************** Device 3 ****************************************/
void init_I2cEmptyDevice3(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {

};

int event_I2cEmptyDevice3(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    return 0;
};

uint8_t recv_I2cEmptyDevice3(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    return 0;
};

void send_I2cEmptyDevice3(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {

};


/**************************************** Device 4 ****************************************/
void init_I2cEmptyDevice4(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {

};

int event_I2cEmptyDevice4(uint8_t i2CEvent, PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    return 0;
};

uint8_t recv_I2cEmptyDevice4(PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {
    return 0;
};

void send_I2cEmptyDevice4(uint8_t data, PTR_I2C_DEVICE_DATA ptrI2CDeviceData) {

};
