//
// Created by 25231 on 2023/8/17.
//
#include "dataThread.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_SEND_DATA_LEN (1024*1024)
#define MAX_READ_DATA_LEN 102400

#define READ_PIPE "ReadPipe"     // read pipe：外部使用者只能读此管道的数据
#define WRITE_PIPE "WritePipe"   // write pipe: 外部使用者只能向此管道写数据


pthread_mutex_t pthreadMutex_sendData;

static char *send_pipe_file_Path;
static char *read_pipe_file_Path;

static char receive_times_str[] = "receive_times";
static char write_times_str[] = "write_times";

int (*power_callback)(const char *gpio_name, uint8_t level) = NULL;
extern I2C_DIMM_TMP_RWCNT_sTYPE i2ctoi3cDataBuff;

static void create_pipe(void *pVoid) {
    FUNC_DEBUG("function: create_pipe()")
    send_pipe_file_Path = get_file_right_path(READ_PIPE);
    read_pipe_file_Path = get_file_right_path(WRITE_PIPE);

    // 创建管道文件
    if (mkfifo(send_pipe_file_Path, 0666) == -1) {
        if (remove(send_pipe_file_Path) == 0) {
            /* 文件删除成功，尝试再创建一次 */
            if (mkfifo(send_pipe_file_Path, 0666) == -1) {
                perror("mkfifo send pipe");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("mkfifo send pipe");
            exit(EXIT_FAILURE);
        }

    }

    if (mkfifo(read_pipe_file_Path, 0666) == -1) {
        if (remove(read_pipe_file_Path) == 0) {
            /* 文件删除成功，尝试再创建一次 */
            if (mkfifo(read_pipe_file_Path, 0666) == -1) {
                perror("mkfifo read pipe");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("mkfifo read pipe");
            exit(EXIT_FAILURE);
        }
    }
}

static void *send_thread(void *pVoid) {
    FUNC_DEBUG("function: send_thread()")
    PTR_SMBUS_EEPROM_sTYPE ptrSmbusEepromSType;
    PTR_SMBUS_TMP_sTYPE ptrSmbusTmpSType;
    PTR_SMBUS_DIMM_TMP_sTYPE ptrSmbusDimmTmpSType;
    PTR_I2C_EEPROM_sTYPE ptrI2CEepromSType;
    PTR_I2C_BP_CPLD_sTYPE ptrI2CBpCpldSType;
    PTR_I3C_DIMM_TMP_sTYPE ptrI3cDimmTmpSType;
    PTR_SMBUS_TPA626_sTYPE ptrSmbusTpa626SType;
    PTR_GPIO_SWITCH_sTYPE ptrGpioSwitchSType;
    PMBusPage *pmBusPage;
    pthread_mutex_lock(&pthreadMutex_sendData); /* 获取锁，并不再释放，作为数据发送线程的启动条件 */
    FUNC_DEBUG("function: send_thread() unlocked, it's running !")

    char temp[1024];
    char addr[10];
    char *json_str;
    int fd;
    ssize_t bytes_written;
    while (1) {
        fd = open(send_pipe_file_Path, O_WRONLY | O_NONBLOCK); /* 非阻塞方式打开文件 */
        if (fd == -1) { /* 没有打开 */
            file_log("open send pipe failed !", LOG_TIME_END);
            usleep(500 * 1000); /* 休眠 0.5 秒 */
            continue; /* 没有打开成功，无需进行后续的数据拼接操作 */
        }
        cJSON *root = cJSON_CreateObject();
        cJSON *devices = cJSON_CreateArray();

        /**************************************** Devices ****************************************/
        for (int i = 0; i < DEVICE_MAX_NUM; ++i) {
            if (deviceAddList[i].exist) {
                cJSON *device = cJSON_CreateObject();
                cJSON_AddNumberToObject(device, "index", i);
                cJSON_AddNumberToObject(device, "device_type_id", deviceAddList[i].device_type_id);
                switch (deviceAddList[i].device_type_id) {
                    case SMBUS_EEPROM_S:
                        /* {
                         *      "index": 0,
                         *      "description": "",
                         *      "eeprom_data": [0, 1, 2, ...],
                         *      "receive_times": 0,
                         *      "write_times": 0
                         * } */
                        /* Device0 - EEPROM */
                        ptrSmbusEepromSType = (PTR_SMBUS_EEPROM_sTYPE) (deviceAddList[i].ptrSmbusDeviceData->data_buf);
                        /**************************************** 数据处理 - S ****************************************/
                        /* 先发送 desc 信息 */
                        cJSON_AddStringToObject(device, "description", deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddStringToObject(device, "master", deviceAddList[i].ptrDeviceConfig->i2c_dev.device_name);
                        cJSON_AddNumberToObject(device, "bus",
                                                deviceAddList[i].ptrDeviceConfig->bus);
                        sprintf(addr, "0x%02x", deviceAddList[i].ptrDeviceConfig->addr);
                        cJSON_AddStringToObject(device, "address", addr);
                        cJSON *eeprom_data = cJSON_CreateArray();
                        for (int j = 0; j < SMBUS_EEPROM_BUF_SIZE; ++j) {
                            cJSON_AddItemToArray(eeprom_data, cJSON_CreateNumber(ptrSmbusEepromSType->buf[j]));
                        }
                        cJSON_AddItemToObject(device, "eeprom_data", eeprom_data);
                        cJSON_AddNumberToObject(device, receive_times_str, (double) ptrSmbusEepromSType->receive_times);
                        cJSON_AddNumberToObject(device, write_times_str, (double) ptrSmbusEepromSType->write_times);
                        /**************************************** 数据处理 - N ****************************************/
                        break;

                    case SMBUS_TMP:
                        /* {
                         *      "index": 0,
                         *      "description": "",
                         *      "temperature": 30,
                         *      "receive_times": 0,
                         *      "write_times": 0
                         * } */
                        /* Device1 - TMP */
                        ptrSmbusTmpSType = (PTR_SMBUS_TMP_sTYPE) (deviceAddList[i].ptrSmbusDeviceData->data_buf);
                        /**************************************** 数据处理并发送 - S ****************************************/
                        /* 先发送 desc 信息 */
                        cJSON_AddStringToObject(device, "description", deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddStringToObject(device, "master", deviceAddList[i].ptrDeviceConfig->i2c_dev.device_name);
                        cJSON_AddNumberToObject(device, "bus",
                                                deviceAddList[i].ptrDeviceConfig->bus);
                        sprintf(addr, "0x%02x", deviceAddList[i].ptrDeviceConfig->addr);
                        cJSON_AddStringToObject(device, "address", addr);
                        /* 发送温度数据 */
                        cJSON_AddNumberToObject(device, "temperature_integer", ptrSmbusTmpSType->temperature_integer);
                        cJSON_AddNumberToObject(device, "temperature_decimal", ptrSmbusTmpSType->temperature_decimal);
                        cJSON_AddNumberToObject(device, receive_times_str, (double) ptrSmbusTmpSType->receive_times);
                        cJSON_AddNumberToObject(device, write_times_str, (double) ptrSmbusTmpSType->write_times);
                        /**************************************** 数据处理并发送 - N ****************************************/
                        break;

                    case SMBUS_TPA626:
                        /* Device2 - TPA626 */
                        ptrSmbusTpa626SType = (PTR_SMBUS_TPA626_sTYPE) (deviceAddList[i].ptrSmbusDeviceData->data_buf);
                        /**************************************** 数据处理并发送 - S ****************************************/
                        cJSON_AddStringToObject(device, "description", deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddStringToObject(device, "master", deviceAddList[i].ptrDeviceConfig->i2c_dev.device_name);
                        cJSON_AddNumberToObject(device, "bus",
                                                deviceAddList[i].ptrDeviceConfig->bus);
                        sprintf(addr, "0x%02x", deviceAddList[i].ptrDeviceConfig->addr);
                        cJSON_AddStringToObject(device, "address", addr);

                        cJSON_AddNumberToObject(device, "configuration_reg00", ptrSmbusTpa626SType->configuration_reg00);
                        cJSON_AddNumberToObject(device, "shunt_vol_reg01", ptrSmbusTpa626SType->shunt_vol_reg01);
                        cJSON_AddNumberToObject(device, "bus_vol_reg02", ptrSmbusTpa626SType->bus_vol_reg02);
                        cJSON_AddNumberToObject(device, "power_reg03", ptrSmbusTpa626SType->power_reg03);
                        cJSON_AddNumberToObject(device, "current_reg04", ptrSmbusTpa626SType->current_reg04);
                        cJSON_AddNumberToObject(device, "calibration_reg05", ptrSmbusTpa626SType->calibration_reg05);
                        cJSON_AddNumberToObject(device, "mask_enable_reg06", ptrSmbusTpa626SType->mask_enable_reg06);
                        cJSON_AddNumberToObject(device, "alert_limit_reg07", ptrSmbusTpa626SType->alert_limit_reg07);
                        cJSON_AddNumberToObject(device, "manufacturer_id_regfe", ptrSmbusTpa626SType->manufacturer_id_regfe);
                        cJSON_AddNumberToObject(device, "die_id_regff", ptrSmbusTpa626SType->die_id_regff);

                        cJSON_AddNumberToObject(device, receive_times_str, (double) ptrSmbusTpa626SType->receive_times);
                        cJSON_AddNumberToObject(device, write_times_str, (double) ptrSmbusTpa626SType->write_times);
                        /**************************************** 数据处理并发送 - N ****************************************/
                        break;

                    case SMBUS_DIMM_TEMP:
                        ptrSmbusDimmTmpSType = (PTR_SMBUS_DIMM_TMP_sTYPE) (deviceAddList[i].ptrSmbusDeviceData->data_buf);
                        /**************************************** 数据处理并发送 - S ****************************************/
                        /* 先发送 desc 信息 */
                        cJSON_AddStringToObject(device, "description",
                                                deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddStringToObject(device, "master", deviceAddList[i].ptrDeviceConfig->i2c_dev.device_name);
                        cJSON_AddNumberToObject(device, "bus",
                                                deviceAddList[i].ptrDeviceConfig->bus);
                        sprintf(addr, "0x%02x", deviceAddList[i].ptrDeviceConfig->addr);
                        cJSON_AddStringToObject(device, "address", addr);
                        /* 发送温度数据 */
                        cJSON_AddNumberToObject(device, "temperature", ptrSmbusDimmTmpSType->temperature);
                        cJSON_AddNumberToObject(device, receive_times_str, (double) ptrSmbusDimmTmpSType->receive_times);
                        cJSON_AddNumberToObject(device, write_times_str, (double) ptrSmbusDimmTmpSType->write_times);
                        /**************************************** 数据处理并发送 - N ****************************************/
                        break;

                    case PMBUS_PSU:
                        pmBusPage = (PMBusPage *) deviceAddList[i].pmBusPage;
                        cJSON_AddStringToObject(device, "description", pmBusPage->ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddStringToObject(device, "master", pmBusPage->ptrDeviceConfig->i2c_dev.device_name);
                        cJSON_AddNumberToObject(device, "bus",
                                                deviceAddList[i].pmBusPage->ptrDeviceConfig->bus);
                        sprintf(addr, "0x%02x", deviceAddList[i].pmBusPage->ptrDeviceConfig->addr);
                        cJSON_AddStringToObject(device, "address", addr);

                        cJSON_AddStringToObject(device, "MFR_id", pmBusPage->mfr_id);
                        cJSON_AddStringToObject(device, "MFR_model", pmBusPage->mfr_model);
                        cJSON_AddStringToObject(device, "MFR_revision", pmBusPage->mfr_revision);
                        cJSON_AddStringToObject(device, "MFR_location", pmBusPage->mfr_location);
                        cJSON_AddStringToObject(device, "MFR_date", pmBusPage->mfr_date);
                        cJSON_AddStringToObject(device, "MFR_serial", pmBusPage->mfr_serial);
                        cJSON_AddStringToObject(device, "MFR_version", pmBusPage->version);
                        if (pmBusPage->status_mfr_specific == 1) {
                            cJSON_AddStringToObject(device, "MFR_specific", "AC");
                        } else if (pmBusPage->status_mfr_specific == 2) {
                            cJSON_AddStringToObject(device, "MFR_specific", "DC");
                        } else {
                            cJSON_AddNullToObject(device, "MFR_specific");
                        }
                        cJSON_AddNumberToObject(device, "Pout_max", pmBusPage->mfr_pout_max);
                        cJSON_AddNumberToObject(device, "Vin", pmBusPage->read_vin);
                        cJSON_AddNumberToObject(device, "Iin", pmBusPage->read_iin);
                        cJSON_AddNumberToObject(device, "Pin", pmBusPage->read_pin);
                        cJSON_AddNumberToObject(device, "Vout", pmBusPage->read_vout);
                        cJSON_AddNumberToObject(device, "Iout", pmBusPage->read_iout);
                        cJSON_AddNumberToObject(device, "Pout", pmBusPage->read_pout);
                        cJSON_AddNumberToObject(device, "Hs_Temp", pmBusPage->read_temperature_1);
                        cJSON_AddNumberToObject(device, "Amb_Temp", pmBusPage->read_temperature_2);
                        cJSON_AddNumberToObject(device, "FanSpeed", pmBusPage->read_fan_speed_1);
                        cJSON_AddNumberToObject(device, receive_times_str, (double) pmBusPage->receive_times);
                        cJSON_AddNumberToObject(device, write_times_str, (double) pmBusPage->write_times);
                        break;
                    case I2C_EEPROM:
                        /* I2C Device0 - EEPROM */
                        ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) (deviceAddList[i].ptrI2cDeviceData->data_buf);
                        /* 发送 */
                        cJSON_AddStringToObject(device, "description",
                                                deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddStringToObject(device, "master", deviceAddList[i].ptrDeviceConfig->i2c_dev.device_name);
                        cJSON_AddNumberToObject(device, "bus",
                                                deviceAddList[i].ptrDeviceConfig->bus);
                        sprintf(addr, "0x%02x", deviceAddList[i].ptrDeviceConfig->addr);
                        cJSON_AddStringToObject(device, "address", addr);

                        cJSON_AddNumberToObject(device, "size", ptrI2CEepromSType->total_size);
                        cJSON *monitor_data = cJSON_CreateObject();
                        for (int j = 0; j < ptrI2CEepromSType->monitor_data_len; ++j) {
                            if (ptrI2CEepromSType->monitor_data[j] < ptrI2CEepromSType->total_size) {
                                sprintf(temp, "0x%02x", ptrI2CEepromSType->monitor_data[j]);
                                cJSON_AddNumberToObject(monitor_data, temp,
                                                        ptrI2CEepromSType->buf[ptrI2CEepromSType->monitor_data[j]]);
                            } else {
                                /* monitor data exceed total size ! */
                            }
                        }
                        cJSON_AddItemToObject(device, "monitor_data", monitor_data);
                        cJSON_AddNumberToObject(device, receive_times_str, (double) ptrI2CEepromSType->receive_times);
                        cJSON_AddNumberToObject(device, write_times_str, (double) ptrI2CEepromSType->write_times);
                        break;
                    case I2C_BP_CPLD:
                        ptrI2CBpCpldSType = (PTR_I2C_BP_CPLD_sTYPE) (deviceAddList[i].ptrI2cDeviceData->data_buf);
                        /* 先发送 desc 信息 */
                        cJSON_AddStringToObject(device, "description",
                                                deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddStringToObject(device, "master", deviceAddList[i].ptrDeviceConfig->i2c_dev.device_name);
                        cJSON_AddNumberToObject(device, "bus",
                                                deviceAddList[i].ptrDeviceConfig->bus);
                        sprintf(addr, "0x%02x", deviceAddList[i].ptrDeviceConfig->addr);
                        cJSON_AddStringToObject(device, "address", addr);
                        /* cpld manufacture */
                        cJSON_AddNumberToObject(device, "cpld_manufacture",
                                                ptrI2CBpCpldSType->i2CBpCpldData.cpld_manufacture);
                        cJSON_AddNumberToObject(device, "max_hdd_num", ptrI2CBpCpldSType->i2CBpCpldData.max_hdd_num);
                        cJSON_AddNumberToObject(device, "cpld_revision", ptrI2CBpCpldSType->i2CBpCpldData.revision);
                        cJSON_AddNumberToObject(device, "bp_type_id", ptrI2CBpCpldSType->i2CBpCpldData.bp_type_id);
                        cJSON_AddNumberToObject(device, "update_flag", ptrI2CBpCpldSType->i2CBpCpldData.update_flag);
                        cJSON_AddNumberToObject(device, "exp_manufacture", ptrI2CBpCpldSType->i2CBpCpldData.exp_manufacture);
                        cJSON_AddStringToObject(device, "bp_name", ptrI2CBpCpldSType->i2CBpCpldData.bp_name);
                        cJSON *hdd_infos = cJSON_CreateArray();

                        for (int j = 0; j < ptrI2CBpCpldSType->i2CBpCpldData.max_hdd_num; ++j) {
                            cJSON *hdd_info = cJSON_CreateObject();
                            cJSON_AddNumberToObject(hdd_info, "Hdd_num", j);
                            cJSON_AddNumberToObject(hdd_info, "Predictive_Fail", ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Predictive_Fail);
                            cJSON_AddNumberToObject(hdd_info, "Warning", ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Warning);
                            cJSON_AddNumberToObject(hdd_info, "Present", ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Present);
                            cJSON_AddNumberToObject(hdd_info, "Idle/Act", ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Idle_Act);
                            cJSON_AddNumberToObject(hdd_info, "Locate", ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Locate);
                            cJSON_AddNumberToObject(hdd_info, "Rebuild", ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Rebuild);
                            cJSON_AddNumberToObject(hdd_info, "Fail", ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Fail);
                            cJSON_AddItemToArray(hdd_infos, hdd_info);
                        }
                        cJSON_AddItemToObject(device, "hdd_infos", hdd_infos);
                        cJSON_AddNumberToObject(device, receive_times_str, (double) ptrI2CBpCpldSType->receive_times);
                        cJSON_AddNumberToObject(device, write_times_str, (double) ptrI2CBpCpldSType->write_times);
                        break;
                    case I3C_DIMM_TEMP:
                        /* {
                         *      "index": 0,
                         *      "description": "",
                         *      "temperature": 30,
                         *      "receive_times": 0,
                         *      "write_times": 0
                         * } */
                        /* Device1 - TMP */
                        ptrI3cDimmTmpSType = (PTR_I3C_DIMM_TMP_sTYPE) (deviceAddList[i].ptrI3cDeviceData->data_buf);
                        ((PTR_I3C_DIMM_TMP_sTYPE) deviceAddList[i].ptrI3cDeviceData->data_buf)->receive_times = *(i2ctoi3cDataBuff.receive_times);
                        ((PTR_I3C_DIMM_TMP_sTYPE) deviceAddList[i].ptrI3cDeviceData->data_buf)->write_times = *(i2ctoi3cDataBuff.write_times);
                        /**************************************** 数据处理并发送 - S ****************************************/
                        /* 先发送 desc 信息 */
                        cJSON_AddStringToObject(device, "description", deviceAddList[i].ptrI3cDeviceData->ptrDeviceConfig->description);
                        /* 发送温度数据 */
                        cJSON_AddNumberToObject(device, "temperature", ptrI3cDimmTmpSType->temperature);
                        cJSON_AddNumberToObject(device, receive_times_str, (double) ptrI3cDimmTmpSType->receive_times);
                        cJSON_AddNumberToObject(device, write_times_str, (double) ptrI3cDimmTmpSType->write_times);
                        /**************************************** 数据处理并发送 - N ****************************************/
                        break;
                    case GPIO_SWITCH:
                        /* GPIO Device0 - SWITCH */
                        ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE) (deviceAddList[i].ptrGpioDeviceData->data_buf);
                        cJSON_AddStringToObject(device, "description",
                                                deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddNumberToObject(device, "pin_nums", deviceAddList[i].ptrGpioDeviceData->pin_nums);
                        cJSON *pins = cJSON_CreateArray();
                        for (int j = 0; j < deviceAddList[i].ptrGpioDeviceData->pin_nums; ++j) {
                            cJSON *pin = cJSON_CreateObject();
                            cJSON_AddNumberToObject(pin, "pin_num", j);
                            if (deviceAddList[i].ptrGpioDeviceData->pin_type[j] == 0) {
                                /* 没有配置此 pin */
                                continue;
                            } else if (deviceAddList[i].ptrGpioDeviceData->pin_type[j] == 1) {
                                /* 只配置为 INPUT */
                                cJSON_AddStringToObject(pin, "pin_type", "in");
                            } else if (deviceAddList[i].ptrGpioDeviceData->pin_type[j] == 2) {
                                /* 只配置为 OUTPUT */
                                cJSON_AddStringToObject(pin, "pin_type", "out");
                            } else if (deviceAddList[i].ptrGpioDeviceData->pin_type[j] == 3) {
                                /* 同时配置为 INPUT 和 OUTPUT */
                                cJSON_AddStringToObject(pin, "pin_type", "in/out");
                            } else {
                                printf("device[%d] : error pin[%d]_type - %d \n", i, j, deviceAddList[i].ptrGpioDeviceData->pin_type[j]);
                                exit(1);
                            }
                            char pin_status[120] = {0};
                            for (uint32_t k = ptrGpioSwitchSType->pinOscilloscope_list[j].front; k != ptrGpioSwitchSType->pinOscilloscope_list[j].rear; k = (k+1)%(PIN_OSCILLOSCOPE_MAX_BITS+1)) {
                                sprintf(temp, "%d", ptrGpioSwitchSType->pinOscilloscope_list[j].status_bit[k]);
                                strcat(pin_status, temp);
                            }
                            cJSON_AddStringToObject(pin, "pin_status", pin_status);
                            cJSON_AddItemToArray(pins, pin);
                        }
                        cJSON_AddItemToObject(device, "pins", pins);
                        break;
                    case ADC:
                        /* ADC  */
                        cJSON_AddStringToObject(device, "description",
                                                deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddNumberToObject(device, "adc_channel",
                                                deviceAddList[i].ptrDeviceConfig->adc_channel);
                        if (deviceAddList[i].ptrAdcDeviceData->adcRegType == REG_L) {
                            sprintf(temp, "%04x", deviceAddList[i].ptrAdcDeviceData->ptrAdcReg->reg_lh.l);
                        } else {
                            sprintf(temp, "%04x", deviceAddList[i].ptrAdcDeviceData->ptrAdcReg->reg_lh.h);
                        }
                        cJSON_AddStringToObject(device, "adc_register", temp);
                        cJSON_AddNumberToObject(device, "adc_value", deviceAddList[i].ptrAdcDeviceData->set_adc_value);
                        cJSON_AddNumberToObject(device, "division",
                                                deviceAddList[i].ptrAdcDeviceData->division / 1000.0);
                        break;
                    case PCA9546:
                        /* PCA9546 */
                        cJSON_AddStringToObject(device, "description",
                                                deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddStringToObject(device, "master", deviceAddList[i].ptrDeviceConfig->i2c_dev.device_name);
                        cJSON_AddNumberToObject(device, "bus",
                                                deviceAddList[i].ptrDeviceConfig->bus);
                        sprintf(addr, "0x%02x", deviceAddList[i].ptrDeviceConfig->addr);
                        cJSON_AddStringToObject(device, "address", addr);

                        break;
                    case PWM_TACH:
                        /* PWM_TACH */
                        cJSON_AddStringToObject(device, "description",
                                                deviceAddList[i].ptrDeviceConfig->description);
                        cJSON_AddStringToObject(device, "name", deviceAddList[i].ptrDeviceConfig->name);
                        cJSON_AddNumberToObject(device, "pwm_tach_num",
                                                deviceAddList[i].ptrPwmTachDevice->pwm_tach_num);
                        cJSON_AddNumberToObject(device, "min_rpm",
                                                deviceAddList[i].ptrPwmTachDevice->ptrRpmDuty->min_rpm);
                        cJSON_AddNumberToObject(device, "max_rpm",
                                                deviceAddList[i].ptrPwmTachDevice->ptrRpmDuty->max_rpm);
                        cJSON_AddNumberToObject(device, "min_offset",
                                                deviceAddList[i].ptrPwmTachDevice->ptrRpmDuty->min_offset);
                        cJSON_AddNumberToObject(device, "max_offset",
                                                deviceAddList[i].ptrPwmTachDevice->ptrRpmDuty->max_offset);
                        cJSON_AddNumberToObject(device, "rand_deviation_rate",
                                                deviceAddList[i].ptrPwmTachDevice->ptrRpmDuty->rand_deviation_rate);
                        break;
                    default:
                        break;
                }
                cJSON_AddItemToArray(devices, device);
            } else {
                break;
            }
        }
        cJSON_AddItemToObject(root, "devices", devices);
        json_str = cJSON_PrintUnformatted(root);
        bytes_written = write(fd, json_str, strlen(json_str));
        if (bytes_written == -1) {
            perror("write data to read pipe failed!");
            exit(1);
        }
        close(fd);
        cJSON_Delete(root);
        free(json_str);
        usleep(500 * 1000); /* 休眠 0.5 秒 */
    }
    pthread_mutex_unlock(&pthreadMutex_sendData);
    return 0;
}

static void *read_data_thread(void *pVoid) {
    FUNC_DEBUG("function: read_data_thread()")
    PTR_SMBUS_EEPROM_sTYPE ptrSmbusEepromSType;
    PTR_SMBUS_TMP_sTYPE ptrSmbusTmpSType;
    PTR_SMBUS_DIMM_TMP_sTYPE ptrSmbusDimmTmpSType;
    PTR_I2C_EEPROM_sTYPE ptrI2CEepromSType;
    PTR_I2C_BP_CPLD_sTYPE ptrI2CBpCpldSType;
    PTR_I3C_DIMM_TMP_sTYPE ptrI3cDimmTmpSType;
    PTR_SMBUS_TPA626_sTYPE ptrSmbusTpa626SType;
    PTR_GPIO_SWITCH_sTYPE ptrGpioSwitchSType;
    /* 读取数据 */
    char buffer[MAX_READ_DATA_LEN];
    char *endPtr = NULL;
    char temp[1024];
    uint32_t device_index;
    int fd;
    uint32_t pin_level = 0;
    char *pinEndPtr = NULL;
    int pinControlStatus = 0;
    FUNC_DEBUG("function: read_data_thread() Enter while()")
    while (1) {
        FUNC_DEBUG("function: read_data_thread() Enter while() wait read pipe")
        fd = open(read_pipe_file_Path, O_RDONLY);
        FUNC_DEBUG("function: read_data_thread() Enter while() read pipe success")
        if (fd == -1) {
            perror("open write pipe failed!");
            exit(EXIT_FAILURE);
        }
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            perror("write data to read pipe failed!");
            exit(EXIT_FAILURE);
        }
        if (bytes_read > 0) {
            buffer[bytes_read] = 0;
            file_log("read data: ", LOG_TIME);
            file_log(buffer, LOG_END);

            /* 根据输入参数更改数据 */
            device_index = strtoul(buffer, &endPtr, 10);
            if (endPtr != buffer && (device_index < DEVICE_MAX_NUM)) {
                /* device_index 有解析到值，且设备索引没有超过 最大边界 */
                if (deviceAddList[device_index].exist) {
                    /* 索引正常，且设备存在 */
                    switch (deviceAddList[device_index].device_type_id) {
                        case SMBUS_EEPROM_S:
                            /* EEPROM */
                            ptrSmbusEepromSType = (PTR_SMBUS_EEPROM_sTYPE) (deviceAddList[device_index].ptrSmbusDeviceData->data_buf);
                            pthread_mutex_lock(&ptrSmbusEepromSType->mutex);
                            /* 动态改变数据 */
                            /**************************************** 动态改变数据 - S ****************************************/
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrSmbusDeviceData, endPtr);
                            /**************************************** 动态改变数据 - N ****************************************/
                            pthread_mutex_unlock(&ptrSmbusEepromSType->mutex);
                            break;
                        case SMBUS_TMP:
                            /* TMP */
                            ptrSmbusTmpSType = (PTR_SMBUS_TMP_sTYPE) (deviceAddList[device_index].ptrSmbusDeviceData->data_buf);
                            pthread_mutex_lock(&ptrSmbusTmpSType->mutex);
                            /* 动态改变数据 */
                            /**************************************** 动态改变数据 - S ****************************************/
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrSmbusDeviceData, endPtr);
                            /**************************************** 动态改变数据 - N ****************************************/
                            pthread_mutex_unlock(&ptrSmbusTmpSType->mutex);
                            sprintf(temp, "device type id - '%d'  ", deviceAddList[device_index].device_type_id);
                            file_log(temp, LOG_TIME_END);
                            break;
                        case SMBUS_TPA626:
                            /* TPA626 */
                            ptrSmbusTpa626SType = (PTR_SMBUS_TPA626_sTYPE) (deviceAddList[device_index].ptrSmbusDeviceData->data_buf);
                            pthread_mutex_lock(&ptrSmbusTpa626SType->mutex);
                            /**************************************** 动态改变数据 - S ****************************************/
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrSmbusDeviceData, endPtr);
                            /**************************************** 动态改变数据 - N ****************************************/
                            pthread_mutex_unlock(&ptrSmbusTpa626SType->mutex);
                            break;
                        case SMBUS_DIMM_TEMP:
                            /* DIMM_TEMP */
                            ptrSmbusDimmTmpSType = (PTR_SMBUS_DIMM_TMP_sTYPE) (deviceAddList[device_index].ptrSmbusDeviceData->data_buf);
                            pthread_mutex_lock(&ptrSmbusDimmTmpSType->mutex);
                            /* 改变数据 */
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrSmbusDeviceData, endPtr);
                            pthread_mutex_unlock(&ptrSmbusDimmTmpSType->mutex);
                            sprintf(temp, "device type id - '%d'  ", deviceAddList[device_index].device_type_id);
                            file_log(temp, LOG_TIME_END);
                            break;
                        case I2C_EEPROM:
                            /* I2C EEPROM */
                            ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) (deviceAddList[device_index].ptrI2cDeviceData->data_buf);
                            pthread_mutex_lock(&ptrI2CEepromSType->mutex);

                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrI2cDeviceData, endPtr);
                            pthread_mutex_unlock(&ptrI2CEepromSType->mutex);
                            break;
                        case I2C_BP_CPLD:
                            /* I2C BP CPLD */
                            ptrI2CBpCpldSType = (PTR_I2C_BP_CPLD_sTYPE) (deviceAddList[device_index].ptrI2cDeviceData->data_buf);
                            pthread_mutex_lock(&ptrI2CBpCpldSType->mutex);
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrI2cDeviceData, endPtr);
                            pthread_mutex_unlock(&ptrI2CBpCpldSType->mutex);
                            break;
                        case I3C_DIMM_TEMP:
                            /* TMP */
                            ptrI3cDimmTmpSType = (PTR_I3C_DIMM_TMP_sTYPE) (deviceAddList[device_index].ptrI3cDeviceData->data_buf);
                            pthread_mutex_lock(&ptrI3cDimmTmpSType->mutex);
                            /* 动态改变数据 */
                            /**************************************** 动态改变数据 - S ****************************************/
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrI3cDeviceData, endPtr);
                            /**************************************** 动态改变数据 - N ****************************************/
                            pthread_mutex_unlock(&ptrI3cDimmTmpSType->mutex);
                            sprintf(temp, "device type id - '%d'  ", deviceAddList[device_index].device_type_id);
                            file_log(temp, LOG_TIME_END);
                            break;
                        case PMBUS_PSU:
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].pmBusPage,
                                                endPtr);
                            break;
                        case GPIO_SWITCH:
                            /* GPIO SWITCH */
                            ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE) deviceAddList[device_index].ptrGpioDeviceData->data_buf;
                            pthread_mutex_lock(&ptrGpioSwitchSType->mutex);
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrGpioDeviceData, endPtr);
                            pthread_mutex_unlock(&ptrGpioSwitchSType->mutex);
                            break;
                        case ADC:
                            /* ADC */
                            pthread_mutex_lock(&deviceAddList[device_index].ptrAdcDeviceData->value_mutex);
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrAdcDeviceData, endPtr);
                            pthread_mutex_unlock(&deviceAddList[device_index].ptrAdcDeviceData->value_mutex);
                            break;
                        case PWM_TACH:
                            /* PWM_TACH */
                            dynamic_change_data(deviceAddList[device_index].device_type_id,
                                                deviceAddList[device_index].ptrPwmTachDevice, endPtr);
                            break;
                        default:
                            sprintf(temp, "device type id - '%d' not processed! ", deviceAddList[device_index].device_type_id);
                            file_log(temp, LOG_TIME_END);
                            break;
                    }

                } else { /* 该索引设备不存在 */
                    sprintf(temp, "device index - '%d' not exist! ", device_index);
                    file_log(temp, LOG_TIME_END);
                }
            } else { /* 设备索引超过了最大边界 或者 没有正常解析到 device_index */
                if (device_index == 0xffff) {
                    /* 特殊索引，表示要进行 pin 操作 */
                    pin_level = strtoul(endPtr, &pinEndPtr, 16);
                    if (*pinEndPtr == ' ') {
                        pinEndPtr++;
                    }
                    if (power_callback == NULL) {
                        break;
                    }
                    pinControlStatus = power_callback(pinEndPtr, pin_level);
                    if (pinControlStatus != 0) {
                        sprintf(temp, "aspeed pin control (%s) failed - status(%d) !\n", pinEndPtr, pinControlStatus);
                        file_log(temp, LOG_TIME_END);
                    } else {
                        sprintf(temp, "aspeed pin control (%s) successful !\n", pinEndPtr);
                        file_log(temp, LOG_TIME_END);
                    }
                } else {
                    sprintf(temp, "device index - '%d' exceed max num - '%d'! ", device_index, DEVICE_MAX_NUM);
                    file_log(temp, LOG_TIME_END);
                }
            }
        }
        close(fd);
        usleep(500 * 1000);
    }
    FUNC_DEBUG("function: read_data_thread() End return.")
    return 0;
}

void data_thread_init(void *pVoid) {
    FUNC_DEBUG("function: data_thread_init()")
    pthread_t pthread_send, pthread_read;
    create_pipe(NULL);
    pthread_mutex_init(&pthreadMutex_sendData, NULL);
    usleep(500 * 1000); /* 500 ms */
    /* 发送数据线程 */
    pthread_mutex_lock(&pthreadMutex_sendData); /* 获取锁，用来控制数据发送线程，让其不立即发送数据，等待其他地方释放此锁 */
    pthread_create(&pthread_send, NULL, send_thread, NULL);
    pthread_detach(pthread_send);
    /* 读取数据线程 */
    pthread_create(&pthread_read, NULL, read_data_thread, NULL);
    pthread_detach(pthread_read);
}
