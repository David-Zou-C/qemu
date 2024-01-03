//
// Created by 25231 on 2023/8/17.
//
#include "dataThread.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_SEND_DATA_LEN 102400
#define MAX_READ_DATA_LEN 102400

#define READ_PIPE "i2cReadPipe"     // read pipe：外部使用者只能读此管道的数据
#define WRITE_PIPE "i2cWritePipe"   // write pipe: 外部使用者只能向此管道写数据


pthread_mutex_t pthreadMutex_sendData;

static char *send_pipe_file_Path;
static char *read_pipe_file_Path;

static char receive_times_str[] = "receive_times";
static char write_times_str[] = "write_times";

int (*power_callback)(const char *gpio_name, uint8_t level) = NULL;

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
    PTR_SMBUS_TPA626_sTYPE ptrSmbusTpa626SType;
    PTR_GPIO_SWITCH_sTYPE ptrGpioSwitchSType;
    PMBusPage *pmBusPage;
    pthread_mutex_lock(&pthreadMutex_sendData); /* 获取锁，并不再释放，作为数据发送线程的启动条件 */
    FUNC_DEBUG("function: send_thread() unlocked, it's running !")

    char temp[1024];
    char send_str[MAX_SEND_DATA_LEN] = {0};
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
                        cJSON_AddStringToObject(device, "description", deviceAddList[i].ptrSmbusDeviceData->ptrDeviceConfig->description);
                        /* 发送 buf 的所有数据 */
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
                        cJSON_AddStringToObject(device, "description", deviceAddList[i].ptrSmbusDeviceData->ptrDeviceConfig->description);
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
                        cJSON_AddStringToObject(device, "description", deviceAddList[i].ptrSmbusDeviceData->ptrDeviceConfig->description);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->configuration_reg00);
                        cJSON_AddStringToObject(device, "configuration_reg00", temp);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->shunt_vol_reg01);
                        cJSON_AddStringToObject(device, "shunt_vol_reg01", temp);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->bus_vol_reg02);
                        cJSON_AddStringToObject(device, "bus_vol_reg02", temp);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->power_reg03);
                        cJSON_AddStringToObject(device, "power_reg03", temp);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->current_reg04);
                        cJSON_AddStringToObject(device, "current_reg04", temp);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->calibration_reg05);
                        cJSON_AddStringToObject(device, "calibration_reg05", temp);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->mask_enable_reg06);
                        cJSON_AddStringToObject(device, "mask_enable_reg06", temp);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->alert_limit_reg07);
                        cJSON_AddStringToObject(device, "alert_limit_reg07", temp);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->manufacturer_id_regfe);
                        cJSON_AddStringToObject(device, "manufacturer_id_regfe", temp);

                        sprintf(temp, "0x%04x", ptrSmbusTpa626SType->die_id_regff);
                        cJSON_AddStringToObject(device, "die_id_regff", temp);

                        cJSON_AddNumberToObject(device, receive_times_str, (double) ptrSmbusTpa626SType->receive_times);
                        cJSON_AddNumberToObject(device, write_times_str, (double) ptrSmbusTpa626SType->write_times);
                        /**************************************** 数据处理并发送 - N ****************************************/
                        break;

                    case SMBUS_DIMM_TEMP:
                        ptrSmbusDimmTmpSType = (PTR_SMBUS_DIMM_TMP_sTYPE) (deviceAddList[i].ptrSmbusDeviceData->data_buf);
                        /**************************************** 数据处理并发送 - S ****************************************/
                        /* 先发送 desc 信息 */
                        sprintf(temp, "desc: %s \n", deviceAddList[i].ptrSmbusDeviceData->ptrDeviceConfig->description);
                        strcat(send_str, temp);
                        /* 发送温度数据 */
                        strcat(send_str, "tmp: ");
                        sprintf(temp, "%02x \n", ptrSmbusDimmTmpSType->temperature);
                        strcat(send_str, temp);
                        sprintf(temp, "receive times: %lu \n", ptrSmbusDimmTmpSType->receive_times);
                        strcat(send_str, temp);
                        sprintf(temp, "write times: %lu \n", ptrSmbusDimmTmpSType->write_times);
                        strcat(send_str, temp);
                        /**************************************** 数据处理并发送 - N ****************************************/
                        break;

                    case PMBUS_PSU:
                        pmBusPage = (PMBusPage *) deviceAddList[i].pmBusPage;
                        sprintf(temp, "desc: PMBUS PSU \n");
                        strcat(send_str, temp);

                        sprintf(temp, "PSU_Vin[0x88]: %04x \n", pmBusPage->read_vin);
                        strcat(send_str, temp);

                        sprintf(temp, "PSU_Iin[0x89]: %04x \n", pmBusPage->read_iin);
                        strcat(send_str, temp);

                        sprintf(temp, "PSU_Pin[0x97]: %04x \n", pmBusPage->read_pin);
                        strcat(send_str, temp);

                        sprintf(temp, "PSU_Vout[0x8B]: %04x \n", pmBusPage->read_vout);
                        strcat(send_str, temp);

                        sprintf(temp, "PSU_Iout[0x8C]: %04x \n", pmBusPage->read_iout);
                        strcat(send_str, temp);

                        sprintf(temp, "PSU_Pout[0x96]: %04x \n", pmBusPage->read_pout);
                        strcat(send_str, temp);

                        sprintf(temp, "PSU_Hs_Temp[0x8D]: %04x \n", pmBusPage->read_temperature_1);
                        strcat(send_str, temp);

                        sprintf(temp, "PSU_Amb_Temp[0x8E]: %04x \n", pmBusPage->read_temperature_2);
                        strcat(send_str, temp);

                        sprintf(temp, "PSU_FanSpeed[0x90]: %04x \n", pmBusPage->read_fan_speed_1);
                        strcat(send_str, temp);
                        break;
                    case I2C_EEPROM:
                        /* I2C Device0 - EEPROM */
                        ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) (deviceAddList[i].ptrI2cDeviceData->data_buf);
                        /* 发送 */
                        /* 先发送 desc 信息 */
                        sprintf(temp, "desc: %s \n", deviceAddList[i].ptrI2cDeviceData->ptrDeviceConfig->description);
                        strcat(send_str, temp);
                        for (int j = 0; j < ptrI2CEepromSType->monitor_data_len; ++j) {
                            if (ptrI2CEepromSType->monitor_data[j] < ptrI2CEepromSType->total_size) {
                                sprintf(temp, "monitor addr - '0x%02x' : 0x%02x \n",
                                        ptrI2CEepromSType->monitor_data[j],
                                        ptrI2CEepromSType->buf[ptrI2CEepromSType->monitor_data[j]]);
                                strcat(send_str, temp);
                            } else {
                                sprintf(temp, "monitor addr - '0x%02x' exceed eeprom total size - '0x%02x'!\n",
                                        ptrI2CEepromSType->monitor_data[j],
                                        ptrI2CEepromSType->total_size);
                                strcat(send_str, temp);
                            }
                        }
                        sprintf(temp, "receive times: %lu \n", ptrI2CEepromSType->receive_times);
                        strcat(send_str, temp);
                        sprintf(temp, "write times: %lu \n", ptrI2CEepromSType->write_times);
                        strcat(send_str, temp);
                        break;
                    case I2C_BP_CPLD:
                        ptrI2CBpCpldSType = (PTR_I2C_BP_CPLD_sTYPE) (deviceAddList[i].ptrI2cDeviceData->data_buf);
                        /* 先发送 desc 信息 */
                        sprintf(temp, "desc: %s \n", deviceAddList[i].ptrI2cDeviceData->ptrDeviceConfig->description);
                        strcat(send_str, temp);
                        /* cpld manufacture */
                        sprintf(temp, "[0x74] cpld manufacture   : %02x \n"
                                      "[0x90] max hdd num        : %02x \n"
                                      "[0x96] cpld revision      : %02x \n"
                                      "[0x97] bp_type_id         : %02x \n"
                                      "[0x98] cpld update flag   : %02x \n"
                                      "[0xA0 - 0xBF] bp name     : %s \n"
                                      "|  Hdd_num  |  Predictive_Fail  |  Warning  |  Present  |  Idle/Act  |  Locate  |  Rebuild  |  Fail  |\n"
                                      "|           |                   |           |           |            |          |           |        |\n",
                                      ptrI2CBpCpldSType->i2CBpCpldData.cpld_manufacture,
                                      ptrI2CBpCpldSType->i2CBpCpldData.max_hdd_num,
                                      ptrI2CBpCpldSType->i2CBpCpldData.revision,
                                      ptrI2CBpCpldSType->i2CBpCpldData.bp_type_id,
                                      ptrI2CBpCpldSType->i2CBpCpldData.update_flag,
                                      ptrI2CBpCpldSType->i2CBpCpldData.bp_name);
                        strcat(send_str, temp);

                        for (int j = 0; j < ptrI2CBpCpldSType->i2CBpCpldData.max_hdd_num; ++j) {
                            sprintf(temp,
                                    "|   %3d     |        %d          |    %d      |      %d    |     %d      |    %d     |      %d    |    %d   |\n",
                                    j,
                                    ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Predictive_Fail,
                                    ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Warning,
                                    ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Present,
                                    ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Idle_Act,
                                    ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Locate,
                                    ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Rebuid,
                                    ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[j].Fail);
                            strcat(send_str, temp);
                        }
                        sprintf(temp, "receive times: %lu \n", ptrI2CBpCpldSType->receive_times);
                        strcat(send_str, temp);
                        sprintf(temp, "write times: %lu \n", ptrI2CBpCpldSType->write_times);
                        strcat(send_str, temp);

                        break;
                    case GPIO_SWITCH:
                        /* GPIO Device0 - SWITCH */
                        ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE) (deviceAddList[i].ptrGpioDeviceData->data_buf);
                        sprintf(temp, "desc: %s \n", deviceAddList[i].ptrGpioDeviceData->ptrDeviceConfig->description);
                        strcat(send_str, temp);
                        for (int j = 0; j < deviceAddList[i].ptrGpioDeviceData->pin_nums; ++j) {
                            if (deviceAddList[i].ptrGpioDeviceData->pin_type[j] == 0) {
                                /* 没有配置此 pin */
                                continue;
                            } else if (deviceAddList[i].ptrGpioDeviceData->pin_type[j] == 1) {
                                /* 只配置为 INPUT */
                                sprintf(temp, "[IN]     pin[%d]: ", j);
                            } else if (deviceAddList[i].ptrGpioDeviceData->pin_type[j] == 2) {
                                /* 只配置为 OUTPUT */
                                sprintf(temp, "[OUT]    pin[%d]: ", j);
                            } else if (deviceAddList[i].ptrGpioDeviceData->pin_type[j] == 3) {
                                /* 同时配置为 INPUT 和 OUTPUT */
                                sprintf(temp, "[IN|OUT] pin[%d]: ", j);
                            } else {
                                printf("device[%d] : error pin[%d]_type - %d \n", i, j, deviceAddList[i].ptrGpioDeviceData->pin_type[j]);
                                exit(1);
                            }
                            strcat(send_str, temp);
                            for (uint32_t k = ptrGpioSwitchSType->pinOscilloscope_list[j].front; k != ptrGpioSwitchSType->pinOscilloscope_list[j].rear; k = (k+1)%(PIN_OSCILLOSCOPE_MAX_BITS+1)) {
                                sprintf(temp, "%d", ptrGpioSwitchSType->pinOscilloscope_list[j].status_bit[k]);
                                strcat(send_str, temp);
                            }
                            sprintf(temp, "|\n");
                            strcat(send_str, temp);
                        }
                        break;
                    case ADC:
                        /* ADC  */
                        sprintf(temp, "desc: %s \n", deviceAddList[i].ptrAdcDeviceData->ptrDeviceConfig->description);
                        strcat(send_str, temp);
                        if (deviceAddList[i].ptrAdcDeviceData->adcRegType == REG_L) {
                            sprintf(temp, "adc_value : %04x \n", deviceAddList[i].ptrAdcDeviceData->ptrAdcReg->reg_lh.l);
                        } else {
                            sprintf(temp, "adc_value : %04x \n", deviceAddList[i].ptrAdcDeviceData->ptrAdcReg->reg_lh.h);
                        }
                        strcat(send_str, temp);
                        sprintf(temp, "division: %.3f \n", deviceAddList[i].ptrAdcDeviceData->division/1000.0);
                        strcat(send_str, temp);
                        break;
                    case PCA9546:
                        /* PCA9546 */
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
        send_str = cJSON_PrintUnformatted(root);
        bytes_written = write(fd, send_str, strlen(send_str));
        if (bytes_written == -1) {
            perror("write data to read pipe failed!");
            exit(1);
        }
        close(fd);
        send_str[0] = 0;
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
