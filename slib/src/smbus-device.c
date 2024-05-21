//
// Created by 25231 on 2023/8/8.
//

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "smbus-device.h"

static double SMBusDevice_get_rpm_from_duty(float duty)
{
    double max_rpm = 0x4268;
    double min_rpm = 0x708;
    double min_offset = (0x64 % 1001) / 1000.0;
    double max_offset = (0x3e8 % 1001) / 1000.0;
    int rand_deviation_rate = 0x05;
    double ret = 0;

    if (duty <= min_offset) {
        ret = min_rpm;
    } else {
        ret = min_rpm +
              (duty - min_offset) / (max_offset - min_offset) *
              (max_rpm - min_rpm);
    }
    if (rand_deviation_rate > 0 && rand_deviation_rate < 100) {
        double a = rand() % (rand_deviation_rate * 2 * 100) / 100.0;
        ret = ret * (1 - rand_deviation_rate/100.0 + a/100.0);
    }
    return ret;
}

/**************************************** SMBusEmptyDevice0 ****************************************/



/**
 * 初始化函数
 * @param ptrSmbusDeviceData
 */
void init_SMBusEmptyDevice0(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    PTR_SMBUS_EEPROM_sTYPE ptrSmbusEepromSType;
    /* 先判断 data_buf 似乎为 NULL */
    if (ptrSmbusDeviceData->data_buf == NULL) {
        /* 为 NULL，表示未曾初始化过 */
        ptrSmbusDeviceData->data_buf = (uint8_t *) malloc(sizeof(SMBUS_EEPROM_sTYPE));
        memset(ptrSmbusDeviceData->data_buf, 0, sizeof(SMBUS_EEPROM_sTYPE));
        ptrSmbusEepromSType = (PTR_SMBUS_EEPROM_sTYPE) ptrSmbusDeviceData->data_buf;
        /* 如果是第一次创建，则需要进行互斥锁的初始化操作 */
        pthread_mutex_init(&ptrSmbusEepromSType->mutex, NULL);
        /* 获取 device_index */
        ptrSmbusEepromSType->device_index = get_device_index(ptrSmbusDeviceData);
    } else {
        ptrSmbusEepromSType = (PTR_SMBUS_EEPROM_sTYPE) ptrSmbusDeviceData->data_buf;
    }

    /* 1. buf 全部置为 0xff，offset置0 */
    for (int i = 0; i < SMBUS_EEPROM_BUF_SIZE; ++i) {
        ptrSmbusEepromSType->buf[i] = 0xff;
    }
    ptrSmbusEepromSType->offset = 0;


    /* 2. 解析 args 的第一个十六进制数 - 赋值方式 （SMBUS上的EEPROM不需指定大小） */

    dynamic_change_data(SMBUS_EEPROM_S, ptrSmbusDeviceData, ptrSmbusDeviceData->ptrDeviceConfig->args);

}

/**
 * 接收数据处理函数
 * @param ptrSmbusDeviceData
 * @return
 */
uint8_t receive_SMBusEmptyDevice0(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    PTR_SMBUS_EEPROM_sTYPE ptrSmbusEepromSType = (PTR_SMBUS_EEPROM_sTYPE) ptrSmbusDeviceData->data_buf;
    /* 逻辑处理前，先获取锁 */
    ptrSmbusEepromSType->receive_times++;
    uint8_t res = ptrSmbusEepromSType->buf[ptrSmbusEepromSType->offset++];


    return res;
}

/**
 * 发送数据处理函数
 * @param buf
 * @param len
 * @param ptrSmbusDeviceData
 * @return
 */
int write_SMBusEmptyDevice0(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    PTR_SMBUS_EEPROM_sTYPE ptrSmbusEepromSType = (PTR_SMBUS_EEPROM_sTYPE) ptrSmbusDeviceData->data_buf;
    uint8_t fanNo;
    uint16_t rpm;
    /* 逻辑处理前，先获取锁 */

    ptrSmbusEepromSType->write_times++;
    ptrSmbusEepromSType->offset = buf[0];
    uint8_t addr = ptrSmbusEepromSType->offset;
    buf++; /* 第一个是地址，先排除 */
    len--; /* 第一个是地址，先排除 */
    for (; len > 0; len--) {
        ptrSmbusEepromSType->buf[ptrSmbusEepromSType->offset++] = *buf++;
    }
    
    if (strncmp(ptrSmbusDeviceData->ptrDeviceConfig->name, "FAN CPLD", 8) == 0){
        if ((addr >= 0x10) && (addr <= 0x19)){
            fanNo = addr & 0x0F;
            rpm = (uint16_t)SMBusDevice_get_rpm_from_duty(ptrSmbusEepromSType->buf[addr]);
            ptrSmbusEepromSType->buf[0x30 + fanNo * 2] = (uint8_t)rpm;
            ptrSmbusEepromSType->buf[0x31 + fanNo * 2] = (uint8_t)(rpm >> 8);
        }
    }

    return 0;
}

/**************************************** SMBusEmptyDevice1 ****************************************/
/* 温度传感器 TMP75 的简化版本 */


void init_SMBusEmptyDevice1(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    /* 先判断 data_buf 是否曾初始化 */
    PTR_SMBUS_TMP_sTYPE ptrSmbusTmpSType;
    if (ptrSmbusDeviceData->data_buf == NULL) {
        ptrSmbusDeviceData->data_buf = (uint8_t *) malloc(sizeof(SMBUS_TMP_sTYPE));
        memset(ptrSmbusDeviceData->data_buf, 0, sizeof(SMBUS_TMP_sTYPE));
        ptrSmbusTmpSType = (PTR_SMBUS_TMP_sTYPE) ptrSmbusDeviceData->data_buf;
        /* 如果是第一次创建，则需要进行互斥锁的初始化操作 */
        pthread_mutex_init(&ptrSmbusTmpSType->mutex, NULL);
        ptrSmbusTmpSType->device_index = get_device_index(ptrSmbusDeviceData); /* 获取 device index,仅一次即可 */
    } else {
        /* 初始值的设定 */
        ptrSmbusTmpSType = (PTR_SMBUS_TMP_sTYPE) ptrSmbusDeviceData->data_buf;
    }

    /* 根据参数修改数据 */
    dynamic_change_data(SMBUS_TMP, ptrSmbusDeviceData, ptrSmbusDeviceData->ptrDeviceConfig->args);
    ptrSmbusTmpSType->offset = 0;

}

uint8_t receive_SMBusEmptyDevice1(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    PTR_SMBUS_TMP_sTYPE ptrSmbusTmpSType = (PTR_SMBUS_TMP_sTYPE) ptrSmbusDeviceData->data_buf;

    ptrSmbusTmpSType->receive_times++;
    if (ptrSmbusTmpSType->offset == 0) {
        ptrSmbusTmpSType->offset = 1;
        return ptrSmbusTmpSType->temperature_integer;
    } else if (ptrSmbusTmpSType->offset == 1) {
        ptrSmbusTmpSType->offset = 0;
        return ptrSmbusTmpSType->temperature_decimal;
    } else {
        ptrSmbusTmpSType->offset = 0;
        return ptrSmbusTmpSType->temperature_integer;
    }
}

int write_SMBusEmptyDevice1(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    PTR_SMBUS_TMP_sTYPE ptrSmbusTmpSType = (PTR_SMBUS_TMP_sTYPE) ptrSmbusDeviceData->data_buf;

    ptrSmbusTmpSType->write_times++;
    /* 有写操作，直接重置 offset  */
    ptrSmbusTmpSType->offset = 0;
    return 0;
}

/**************************************** SMBusEmptyDevice2 ****************************************/
void init_SMBusEmptyDevice2(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    PTR_SMBUS_TPA626_sTYPE ptrSmbusTpa626SType;
    /* 先判断 data_buf 是否为空，即是否曾初始化 */
    if (ptrSmbusDeviceData->data_buf == NULL) {
        /* 为 NULL，表示还未进行过初始化操作 */
        ptrSmbusDeviceData->data_buf = (uint8_t *) malloc(sizeof(SMBUS_TPA626_sTYPE));
        memset(ptrSmbusDeviceData->data_buf, 0, sizeof(SMBUS_TPA626_sTYPE));
        ptrSmbusTpa626SType = (PTR_SMBUS_TPA626_sTYPE) ptrSmbusDeviceData->data_buf;
        /* 如果第一次创建，还需要进行互斥锁的初始化操作 */
        pthread_mutex_init(&ptrSmbusTpa626SType->mutex, NULL);
        ptrSmbusTpa626SType->device_index = get_device_index(ptrSmbusDeviceData); /* 获取 device_index，获取一次即可 */
    } else {
        /* 并非第一次初始化，变量赋值即可 */
        ptrSmbusTpa626SType = (PTR_SMBUS_TPA626_sTYPE) ptrSmbusDeviceData->data_buf;
    }

    /* 根据参数修改数据 */
    dynamic_change_data(SMBUS_TPA626, ptrSmbusDeviceData, ptrSmbusDeviceData->ptrDeviceConfig->args);
    ptrSmbusTpa626SType->offset = 0;
    ptrSmbusTpa626SType->reg_val_offset = 0;

}

uint8_t receive_SMBusEmptyDevice2(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    PTR_SMBUS_TPA626_sTYPE ptrSmbusTpa626SType = (PTR_SMBUS_TPA626_sTYPE) ptrSmbusDeviceData->data_buf;
    /* 逻辑处理 */
    uint8_t res;
    uint8_t success = 1;
    ptrSmbusTpa626SType->receive_times++;
    switch (ptrSmbusTpa626SType->offset) {
        case 0x00: /* 00h  R/W 配置寄存器 */
            res = ptrSmbusTpa626SType->configuration_reg00 >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        case 0x01: /* 01h R 并联电压寄存器 */
            res = ptrSmbusTpa626SType->shunt_vol_reg01 >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        case 0x02: /* 02h R Bus电压寄存器 */
            res = ptrSmbusTpa626SType->bus_vol_reg02 >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        case 0x03:
            res = ptrSmbusTpa626SType->power_reg03 >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        case 0x04:
            res = ptrSmbusTpa626SType->current_reg04 >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        case 0x05:
            res = ptrSmbusTpa626SType->calibration_reg05 >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        case 0x06:
            res = ptrSmbusTpa626SType->mask_enable_reg06 >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        case 0x07:
            res = ptrSmbusTpa626SType->alert_limit_reg07 >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        case 0xfe:
            res = ptrSmbusTpa626SType->manufacturer_id_regfe >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        case 0xff:
            res = ptrSmbusTpa626SType->die_id_regff >> (8 * (1- ptrSmbusTpa626SType->reg_val_offset));
            break;
        default:
            success = 0;
            /* 不支持的地址，重置 offset */
            ptrSmbusTpa626SType->offset = 0;
            ptrSmbusTpa626SType->reg_val_offset = 0;
            res = 0xff;
            break;
    }
    if(success) { /* 返回数据成功了，reg_val_offset + 1 */
        ptrSmbusTpa626SType->reg_val_offset  = (ptrSmbusTpa626SType->reg_val_offset + 1) % 2;
    } else {
    }

    return res;
}

int write_SMBusEmptyDevice2(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    PTR_SMBUS_TPA626_sTYPE ptrSmbusTpa626SType = (PTR_SMBUS_TPA626_sTYPE) ptrSmbusDeviceData->data_buf;

    /* 逻辑处理区 */
    uint16_t write_data;
    ptrSmbusTpa626SType->write_times ++;
    ptrSmbusTpa626SType->offset = buf[0];
    ptrSmbusTpa626SType->reg_val_offset = 0;
    if (len >= 3) { /* if 语句，最多只接受一个寄存器的写操作，之后的忽略 */
        /* 长度在 3 个字节获以上，为写操作 */
        /* 解析写入的值 */
        write_data = buf[1];
        write_data = (write_data << 8) | buf[2];
        /* 赋值写入的 */
        switch (ptrSmbusTpa626SType->offset) {
            case 0x00: /* 00h  R/W 配置寄存器 */
                ptrSmbusTpa626SType->configuration_reg00 = write_data;
                break;
            case 0x01: /* 01h R 并联电压寄存器 */
                ptrSmbusTpa626SType->shunt_vol_reg01 = write_data;
                break;
            case 0x02: /* 02h R Bus电压寄存器 */
                ptrSmbusTpa626SType->bus_vol_reg02 = write_data;
                break;
            case 0x03:
                ptrSmbusTpa626SType->power_reg03 = write_data;
                break;
            case 0x04:
                ptrSmbusTpa626SType->current_reg04 = write_data;
                break;
            case 0x05:
                ptrSmbusTpa626SType->calibration_reg05 = write_data;
                break;
            case 0x06:
                ptrSmbusTpa626SType->mask_enable_reg06= write_data;
                break;
            case 0x07:
                ptrSmbusTpa626SType->alert_limit_reg07= write_data;
                break;
            case 0xfe:
                ptrSmbusTpa626SType->manufacturer_id_regfe = write_data;
                break;
            case 0xff:
                ptrSmbusTpa626SType->die_id_regff= write_data;
                break;
            default:
                /* 不支持的地址，重置 offset */
                ptrSmbusTpa626SType->offset = 0;
                break;
        }
    }
    
    return 0;
}

/**************************************** SMBusEmptyDevice3 ****************************************/
void init_SMBusEmptyDevice3(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
}

uint8_t receive_SMBusEmptyDevice3(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

int write_SMBusEmptyDevice3(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

/**************************************** SMBusEmptyDevice4 ****************************************/
void init_SMBusEmptyDevice4(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {

}

uint8_t receive_SMBusEmptyDevice4(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

int write_SMBusEmptyDevice4(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

/**************************************** SMBusEmptyDevice5 ****************************************/
void init_SMBusEmptyDevice5(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {

}

uint8_t receive_SMBusEmptyDevice5(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

int write_SMBusEmptyDevice5(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

/**************************************** SMBusEmptyDevice6 ****************************************/
void init_SMBusEmptyDevice6(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {

}

uint8_t receive_SMBusEmptyDevice6(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

int write_SMBusEmptyDevice6(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

/**************************************** SMBusEmptyDevice7 ****************************************/
void init_SMBusEmptyDevice7(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {

}

uint8_t receive_SMBusEmptyDevice7(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

int write_SMBusEmptyDevice7(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

/**************************************** SMBusEmptyDevice8 ****************************************/
void init_SMBusEmptyDevice8(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {

}

uint8_t receive_SMBusEmptyDevice8(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

int write_SMBusEmptyDevice8(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

/**************************************** SMBusEmptyDevice9 ****************************************/
void init_SMBusEmptyDevice9(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {

}

uint8_t receive_SMBusEmptyDevice9(PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}

int write_SMBusEmptyDevice9(unsigned char *buf, unsigned char len, PTR_SMBUS_DEVICE_DATA ptrSmbusDeviceData) {
    return 0;
}
