//
// Created by 25231 on 2023/8/2.
//

#include "aspeed-init.h"
#include <unistd.h>
#include <libgen.h> // 用于 dirname 函数
#include <time.h>

uint16_t (*adc_get_value)(void *opaque, uint8_t channel) = NULL;
void (*adc_set_value)(void *opaque, uint8_t channel, uint16_t value) = NULL;

void (*connect_gpio_line)(void *send_dev, int send_line, void *recv_dev, int recv_line) = NULL;

void (*set_gpio_line)(void *pin_buf, uint32_t pin, int level) = NULL;

DEVICE_ADD_INFO deviceAddList[DEVICE_MAX_NUM] = {0};
pthread_mutex_t file_log_lock;

MAC_DEVICE_INFO macDeviceInfo[MAC_DEVICE_INFO_MAX_NUM] = {
        {
            .macApplyMethod = 1,
            .deviceTypeId = I2C_EEPROM,
            .i2c_bus = 5,
            .i2c_addr = 0x50,
            .mac_value_for_offset[0] = 0x0f08,
            .mac_value_for_offset[1] = 0x0f09,
            .mac_value_for_offset[2] = 0x0f0a,
            .mac_value_for_offset[3] = 0x0f0b,
            .mac_value_for_offset[4] = 0x0f0c,
            .mac_value_for_offset[5] = 0x0f0d
        },
        { /* end of list */ }
};

int device_add(DEVICE_TYPE_ID device_type_id, const char *device_name, void *vPtrDeviceData, void *vPtrDevGpio) {
    FUNC_DEBUG("function: device_add()")
    for (int i = 0; i < DEVICE_MAX_NUM; ++i) {
        if (deviceAddList[i].exist == 0) {
            deviceAddList[i].exist = 1;
            deviceAddList[i].device_index = i;
            deviceAddList[i].device_type_id = device_type_id;
            strcpy(deviceAddList[i].device_name, device_name);
            deviceAddList[i].dev_gpio = vPtrDevGpio;
            if (device_type_id == BMC) {
                deviceAddList[i].device_type = BMC_DEVICE_TYPE;
            } else if ((device_type_id >= SMBUS_DEVICE_TYPE_ID_OFFSET) &&
                       (device_type_id < SMBUS_DEVICE_TYPE_ID_OFFSET + SMBUS_DEVICE_TOTAL_NUM)) {
                deviceAddList[i].ptrSmbusDeviceData = vPtrDeviceData;
                deviceAddList[i].device_type = SMBUS_DEVICE_TYPE;
                deviceAddList[i].ptrDeviceConfig = deviceAddList[i].ptrSmbusDeviceData->ptrDeviceConfig;
            } else if (device_type_id == PMBUS_PSU){
                deviceAddList[i].pmBusPage = vPtrDeviceData;
                deviceAddList[i].device_type = PMBUS_DEVICE_TYPE;
                deviceAddList[i].ptrDeviceConfig = deviceAddList[i].pmBusPage->ptrDeviceConfig;
            } else if ((device_type_id >= I2C_DEVICE_TYPE_ID_OFFSET) &&
                       (device_type_id < I2C_DEVICE_TYPE_ID_OFFSET + I2C_DEVICE_TOTAL_NUM)) {
                deviceAddList[i].ptrI2cDeviceData = vPtrDeviceData;
                deviceAddList[i].device_type = I2C_DEVICE_TYPE;
                deviceAddList[i].ptrDeviceConfig = deviceAddList[i].ptrI2cDeviceData->ptrDeviceConfig;
            } else if ((device_type_id >= GPIO_DEVICE_TYPE_ID_OFFSET) &&
                       (device_type_id < GPIO_DEVICE_TYPE_ID_OFFSET + GPIO_DEVICE_TOTAL_NUM)) {
                deviceAddList[i].ptrGpioDeviceData = vPtrDeviceData;
                deviceAddList[i].device_type = GPIO_DEVICE_TYPE;
                deviceAddList[i].ptrDeviceConfig = deviceAddList[i].ptrGpioDeviceData->ptrDeviceConfig;
            } else if ((device_type_id >= ADC_DEVICE_TYPE_ID_OFFSET) &&
                       (device_type_id < ADC_DEVICE_TYPE_ID_OFFSET + ADC_DEVICE_TOTAL_NUM)) {
                deviceAddList[i].ptrAdcDeviceData = vPtrDeviceData;
                deviceAddList[i].device_type = ADC_DEVICE_TYPE;
                deviceAddList[i].ptrDeviceConfig = deviceAddList[i].ptrAdcDeviceData->ptrDeviceConfig;
            } else if ((device_type_id >= I3C_DEVICE_TYPE_ID_OFFSET) &&
                       (device_type_id < I3C_DEVICE_TYPE_ID_OFFSET + I3C_DEVICE_TOTAL_NUM)) {
                deviceAddList[i].ptrI3cDeviceData = vPtrDeviceData;
                deviceAddList[i].device_type = I3C_DEVICE_TYPE;
            } else if (device_type_id == PCA9546 || device_type_id == PCA9548) {
                deviceAddList[i].pca954x = vPtrDeviceData;
                deviceAddList[i].device_type = PCA954X_DEVICE_TYPE;
                /* 对于 PCA954X 而言 ptrDeviceConfig 无法在此处直接赋值，返回索引值后，由设备的 realize 函数中赋值 */
            } else if (device_type_id == PWM_TACH) {
                deviceAddList[i].ptrPwmTachDevice = vPtrDeviceData;
                deviceAddList[i].device_type = PWM_TACH_DEVICE_TYPE;
                deviceAddList[i].ptrDeviceConfig = deviceAddList[i].ptrPwmTachDevice->ptrDeviceConfig;
            }
                
            return i;
        }
    }

    printf("device exceed max num! \n");
    exit(1);
    return -1;
}

int get_device_index(void *vPtrDeviceData) {
    FUNC_DEBUG("function: get_device_index()")
    for (int i = 0; i < DEVICE_MAX_NUM; ++i) {
        if (deviceAddList[i].exist == 1) { /* 设备存在 */
            if (deviceAddList[i].device_type == SMBUS_DEVICE_TYPE) { /* SMBUS 类型设备 */
                if (deviceAddList[i].ptrSmbusDeviceData == vPtrDeviceData) {
                    return i;
                }
            } else if (deviceAddList[i].device_type == I2C_DEVICE_TYPE) { /* I2C 类型设备 */
                if (deviceAddList[i].ptrI2cDeviceData == vPtrDeviceData) {
                    return i;
                }
            } else if (deviceAddList[i].device_type == GPIO_DEVICE_TYPE) {
                if (deviceAddList[i].ptrGpioDeviceData == vPtrDeviceData) {
                    return i;
                }
            }
        } else { /* 设备不能存在 */
            /* 没有必要继续，后续设备都不存在 */
            return -1;
        }
    }
    return -1;
}

DEVICE_TYPE get_device_type(DEVICE_TYPE_ID device_type_id) {
    if (device_type_id == BMC) {
        return BMC_DEVICE_TYPE;
    }
    if ((device_type_id >= SMBUS_DEVICE_TYPE_ID_OFFSET) &&
        (device_type_id < SMBUS_DEVICE_TYPE_ID_OFFSET + SMBUS_DEVICE_TOTAL_NUM)) {
        return SMBUS_DEVICE_TYPE;
    } else if (device_type_id == PMBUS_PSU){
        return PMBUS_DEVICE_TYPE;
    } else if ((device_type_id >= I2C_DEVICE_TYPE_ID_OFFSET) &&
               (device_type_id < I2C_DEVICE_TYPE_ID_OFFSET + I2C_DEVICE_TOTAL_NUM)) {
        return I2C_DEVICE_TYPE;
    } else if ((device_type_id >= GPIO_DEVICE_TYPE_ID_OFFSET) &&
               (device_type_id < GPIO_DEVICE_TYPE_ID_OFFSET + GPIO_DEVICE_TOTAL_NUM)) {
        return GPIO_DEVICE_TYPE;
    } else if ((device_type_id >= ADC_DEVICE_TYPE_ID_OFFSET) &&
               (device_type_id < ADC_DEVICE_TYPE_ID_OFFSET + ADC_DEVICE_TOTAL_NUM)) {
        return ADC_DEVICE_TYPE;
    } else if ((device_type_id >= I3C_DEVICE_TYPE_ID_OFFSET) &&
               (device_type_id < I3C_DEVICE_TYPE_ID_OFFSET + I3C_DEVICE_TOTAL_NUM)) {
        return I3C_DEVICE_TYPE;
    } else if (device_type_id == PCA9546 || device_type_id == PCA9548) {
        return PCA954X_DEVICE_TYPE;
    } else if (device_type_id == PWM_TACH) {
        return PWM_TACH_DEVICE_TYPE;
    } else {
        return UNDEFINED;
    }
}

void *get_dev_gpio(int32_t device_index, const char *device_name) {
    FUNC_DEBUG("function: get_dev_gpio()")
    int device_id;
    if (device_index >= DEVICE_MAX_NUM) {
        return NULL;
    }
    if (device_index < 0) {
        get_dev_index_for_name(device_name, &device_id);
    } else {
        device_id = device_index;
    }
    if (device_id < 0) {
        return NULL;
    }

    if (deviceAddList[device_id].exist == 1) {
        return deviceAddList[device_id].dev_gpio;
    }
    return NULL;
}

void get_dev_index_for_name(const char *name, int *index) {
    for (int i = 0; i < DEVICE_MAX_NUM; ++i) {
        if (deviceAddList[i].exist == 0) {
            break;
        } else {
            if (strcmp(name, deviceAddList[i].device_name) == 0) {
                *index = i;
                return;
            }
        }
    }
    printf("get index for name (\"%s\") failed! \n", name);
    *index = -1;
}

/**
 * 记录log信息，以原字符串写入，不加以任何修改
 * @param message
 */
void file_log(const char *message, uint8_t log_type) {
    if (!sDEBUG) {
        return;
    }

    pthread_mutex_lock(&file_log_lock); /* 加锁，避免多线程重复 */

    char *filePath = get_file_right_path(LOG_FILE);
    FILE *file = fopen(filePath, "a");

    if (file == NULL) {
        printf("\n can not open log file: %s \n", filePath);
        exit(1);
    }
    time_t currentTime;
    struct tm *timeInfo;
    char timeFormat[80];
    switch (log_type) {
        case LOG_NOTHING:
            fprintf(file, "%s", message);
            break;
        case LOG_END: /* 加上换行 */
            fprintf(file, "%s\n", message);
            break;
        case LOG_TIME: /* 加上时间戳 */
            // 获取当前系统时间
            time(&currentTime);
            // 格式化时间为字符串
            timeInfo = localtime(&currentTime);
            // 定义时间格式
            strftime(timeFormat, sizeof(timeFormat), "[%Y-%m-%d %H:%M:%S]", timeInfo);
            fprintf(file, "%s %s", timeFormat, message);
            break;
        case LOG_TIME_END:
            // 获取当前系统时间
            time(&currentTime);
            // 格式化时间为字符串
            timeInfo = localtime(&currentTime);
            // 定义时间格式
            strftime(timeFormat, sizeof(timeFormat), "[%Y-%m-%d %H:%M:%S]", timeInfo);
            fprintf(file, "%s %s\n", timeFormat, message);
            break;
        default:
            printf("unknown log_type - %d ! \n", log_type);
            exit(1);
    }

    fclose(file);
    pthread_mutex_unlock(&file_log_lock);
}


/**
 * 获取文件的准确的路径
 * @param filename
 * @return
 */
char *get_file_right_path(const char *filename) {
    const int configFilePath_len = 2048;
    char *configFilePath = (char *) malloc(configFilePath_len);
    static char executablePath[1024] = {0};
    static char programDirectory[1024] = {0};
    if (*programDirectory == 0) { /* 如果 programDirectory 为 0，表示还未获取到程序执行目录，否则就不需要再重复获取了 */
        // 先获取当前执行文件的路径
        if (readlink("/proc/self/exe", executablePath, sizeof(executablePath) - 1) == -1) {
            perror("readlink");
            exit(1);
        }
        // 从绝对路径中提取目录部分
        strcpy(programDirectory, executablePath);
        dirname(programDirectory);
    }

    // 构建以相对主程序文件路径解析的 filename 的绝对完整路径
    snprintf(configFilePath, configFilePath_len, "%s/%s", programDirectory, filename);
//    printf("file：\"%s\" .\n", configFilePath);

    return configFilePath;
}


/**
 * 读取并解析配置文件
 * @param filename
 * @return
 */
cJSON *read_config_file(const char *filename) {
    FUNC_DEBUG("function: read_config_file()")
    char *configFilePath = get_file_right_path(filename); // 获取相对于主程序文件的正确的路径
    // 读取JSON文件内容
    FILE *file = fopen(configFilePath, "r");
    if (!file) {
        printf("Unable to open file：\"%s\" .\n", configFilePath);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    char *json_data = (char *) malloc(file_size + 1);
    size_t elements_read = fread(json_data, 1, file_size, file);
    (void) elements_read;  // 忽略函数返回值
    json_data[file_size] = '\0';
    fclose(file);

    // 解析JSON数据
    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        printf("JSON parsing error.\n");
        free(json_data);
        exit(1);
    }

    printf("read json file (%s) over! \n", CONFIG_FILE);
    return root;
}


uint32_t *detachArgsData(char *oStr, char dataType, const char *ctrl_name, uint64_t *res_len) {
    FUNC_DEBUG("function: detachArgsData()")
    char log[1024];
    sprintf(log, "function detachArgsData(\"%s\", %d, \"%s\", _ )", oStr, dataType, ctrl_name);
    file_log(log, LOG_TIME_END);
    if (oStr == NULL) {
//        printf("args is NULL ! \n");
        *res_len = 0;
        return NULL;
    }

    char state_flag = 0; /* =0: 正常状态可以存入； =1: 已遇到 S 特殊类型控制字符串；=2：开始等待 N 特殊类型控制字符串；=3：已遇到E特殊类型字符 */

    const uint32_t batch_size = 128; /* 每一次分配的大小 */
    uint64_t res_actual_offset = 0; /* 已存入数据的大小 */
    uint64_t res_total_len = 0; /* 初始大小 */
    uint32_t *res = (uint32_t *) malloc(batch_size * sizeof(uint32_t)); /* 分配初始大小 */
    if (res == NULL) {
        printf("malloc failed ! \n");
        exit(1);
    }
    res_total_len += batch_size;

    char *startPtr = oStr;
    char *endPtr = startPtr;

    char S_ctrl[batch_size];
    char E_ctrl[batch_size];
    uint8_t ctrl_i = 0;

    uint64_t ctrl_actual_offset = 0;
    uint64_t ctrl_total_len = 0;
    uint32_t *ctrl_data = (uint32_t *) malloc(batch_size * sizeof(uint32_t)); /* 分配初始大小 */
    if (ctrl_data == NULL) {
        printf("malloc failed ! \n");
        exit(1);
    }
    ctrl_total_len += batch_size;

    uint32_t v;
    while (*startPtr != '\0') {
        if (state_flag == 0) {
            /* state_flag 为 0 ，表示正常读取 initial data 状态 */
            v = strtoul(startPtr, &endPtr, 16);
            if (startPtr != endPtr && (*endPtr == ' ' || *endPtr == '\0')) {
                /* 开始位置和结束位置不一样且停止字符为空格或\0，意味着有解析到数据 */
                if (dataType == DETACH_INITIAL_DATA) { /* 只有要求 initial data 时才需要存入 */
                    res[res_actual_offset++] = v; /* 存入 */
                    /**************************************** 容量检查 - S ****************************************/
                    if (res_actual_offset >= res_total_len - 1 && *endPtr != '\0') { /* 已存满，且还没解析完 */
                        /* 增加内存容量 */
                        res = (uint32_t *) realloc(res, (res_total_len + batch_size) * sizeof(uint32_t));
                        if (res == NULL) {
                            printf("realloc failed ! \n");
                            exit(1);
                        }
                        res_total_len += batch_size; /* 总容量扩增成功 */
                    }
                    /**************************************** 容量检查 - N ****************************************/
                }
                /* 重置起始字符指针 */
                startPtr = endPtr;
            } else if (startPtr == endPtr && *endPtr == ' ') {
                /* 开始和结束一样，且都为 空格，则忽略此字符，进入下一个判断处理 */
                startPtr++;
            } else if (startPtr == endPtr && *endPtr == 'S') {
                /* 如果开始和结束位置一样，都为 S，则进入下一状态 */
                state_flag = 1;
                S_ctrl[ctrl_i++] = *startPtr; /* 记录第一个 S_ctrl 字符 - S */
                startPtr++;
            } else {
                /* 遇到其他字符时，停止！ */
                printf("%s \n", oStr);
                for (int i = 0; i < endPtr - oStr; ++i) {
                    printf("_");
                }
                printf("^ stopped on '%c' \n", *startPtr);
                exit(1);
            }
        } else if (state_flag == 1) {
            /* state_flag 为 1 ，表示已遇到 S 特殊控制字符，遇到下一个空格时将进入下一个状态 */
            if (*startPtr == ' ') {
                state_flag = 2;
                S_ctrl[ctrl_i] = '\0'; /* S_ctrl 已记录结束 */
                ctrl_i = 0;
                ctrl_actual_offset = 0; /* 重置 ctrl_data 的偏移，准备开始记录控制数据 */
                startPtr++;
            } else {
                /* 其他字符，记入 S_ctrl */
                S_ctrl[ctrl_i++] = *startPtr;
                startPtr++;
            }
        } else if (state_flag == 2) {
            /* state_flag 为 2，表示已过 S 特殊控制字符串，在等待遇到 N 特殊字符 */
            if (*startPtr == 'N') {
                /* 遇到了 N 字符，进入下一个状态 */
                state_flag = 3;
                E_ctrl[ctrl_i++] = *startPtr; /* 记录第一个 E_ctrl 字符 - N */
                startPtr++;
            } else {
                /* 其他字符，则代表着是此控制字符串的数据 */
                v = strtoul(startPtr, &endPtr, 16);
                if (startPtr != endPtr && *endPtr == ' ') {
                    /* 结束指针不等于开始指针，意为存在有效数据，且结束为空格，表示正常解析到数据了 */
                    if (dataType == DETACH_CTRL_DATA) { /* 只有要求 ctrl data 时，才有必要存入 */
                        ctrl_data[ctrl_actual_offset++] = v;
                        /**************************************** 容量检查 - S ****************************************/
                        if (ctrl_actual_offset >= ctrl_total_len - 1 && *endPtr != '\0') { /* 已存满，且还没解析完 */
                            /* 增加内存容量 */
                            ctrl_data = (uint32_t *) realloc(ctrl_data,
                                                             (ctrl_total_len + batch_size) * sizeof(uint32_t));
                            if (ctrl_data == NULL) {
                                printf("realloc failed ! \n");
                                exit(1);
                            }
                            ctrl_total_len += batch_size; /* 总容量扩增成功 */
                        }
                        /**************************************** 容量检查 - N ****************************************/
                    }
                    /* 重置 startPtr 位置 */
                    startPtr = endPtr; /* 因为 endPtr 不等于 startPtr，意为有前进 */
                } else if (startPtr == endPtr && *endPtr == ' ') {
                    /* 开始和结束一样，且都为 空格，则忽略此字符，进入下一个判断处理 */
                    startPtr++;
                } else {
                    /* 遇到其他字符时，停止！ */
                    printf("%s \n", oStr);
                    for (int i = 0; i < endPtr - oStr; ++i) {
                        printf(" ");
                    }
                    printf("^ stopped on '%c' \n", *endPtr);
                    exit(1);
                }
            }
        } else {
            /* state_flag 为 3 ，表示已遇到 N 字符，在等待 空格 度过 N 特殊控制字符串 */
            if (*startPtr == ' ') {
                state_flag = 0;
                E_ctrl[ctrl_i] = '\0'; /* 空格：表示 N 字符串已记录结束 */
                ctrl_i = 0;
                startPtr++;
            } else if (*(startPtr + 1) == '\0') {
                state_flag = 0;
                /* 下一个字符是 \0 结束，则先记录将此字符和\0一起写入 N 中 */
                E_ctrl[ctrl_i] = *startPtr;
                E_ctrl[ctrl_i + 1] = '\0';
                ctrl_i = 0;
                startPtr++;
            } else {
                /* 非空格或 \0 ，则是 N 控制字符串 */
                E_ctrl[ctrl_i++] = *startPtr;
                startPtr++;
            }
            /* 如果 flag 从 3 变为 0 ，表示 N 字符已记录完，此处应做一个比较 */
            if (state_flag == 0) {
                if (strcmp(&S_ctrl[1], &E_ctrl[1]) != 0) { /* 比较时应忽略第一个字符 S / N */
                    /* S 和 N 不相等 */
                    printf("control data must be a pair: '%s' != '%s' \n", S_ctrl, E_ctrl);
                    exit(1);
                } else {
                    if (dataType == DETACH_CTRL_DATA) {
                        if (strcmp(&S_ctrl[1], ctrl_name) == 0) {
                            free(res); /* 清掉 initial data 空间 */
                            *res_len = ctrl_actual_offset;
                            return ctrl_data;
                        }
                    }
                }
            }
        }
    }
    if (state_flag != 0) {
        printf("over on state_flag: %d ! \n", state_flag);
        exit(1);
    }
    if (dataType == DETACH_INITIAL_DATA) {
        free(ctrl_data);
    }
    *res_len = res_actual_offset;
    return res;
}


void dynamic_change_data(DEVICE_TYPE_ID device_type_id, void *vPtrDeviceData, char *args) {
    FUNC_DEBUG("function: dynamic_change_data()")
    PTR_SMBUS_EEPROM_sTYPE ptrSmbusEepromSType;
    PTR_SMBUS_TMP_sTYPE ptrSmbusTmpSType;
    PTR_SMBUS_DIMM_TMP_sTYPE ptrSmbusDimmTmpSType;
    PTR_I2C_EEPROM_sTYPE ptrI2CEepromSType;
    PTR_I2C_BP_CPLD_sTYPE ptrI2CBpCpldSType;
    PTR_SMBUS_TPA626_sTYPE ptrSmbusTpa626SType;
    PTR_GPIO_DEVICE_DATA ptrGpioDeviceData;
    PTR_GPIO_SWITCH_sTYPE ptrGpioSwitchSType;
    PTR_ADC_DEVICE_DATA ptrAdcDeviceData;
    PTR_I2C_LEGACY_DIMM_TMP_sTYPE ptrI3cDimmTmpSType;
    PTR_PWM_TACH_DEVICE ptrPwmTachDevice;
    PMBusPage *pmBusPage;
    char temp[128];
    char *end=NULL;
    uint64_t len;
    uint32_t *initial_data = NULL;
    uint32_t *ctrl_data = NULL;
    uint64_t i = 1, j = 0;
    switch (device_type_id) {
        /**************************************** SMBUS ****************************************/
        case SMBUS_EEPROM_S:
            /* SMBUS EEPROM  */
            ptrSmbusEepromSType = (PTR_SMBUS_EEPROM_sTYPE) ((PTR_SMBUS_DEVICE_DATA) vPtrDeviceData)->data_buf;
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            if (len >= 2) {
                /* 至少要两个数据 */
                if (initial_data[0] == 0x00) { /* 全部赋值方式 */
                    /* 全部挨个赋值 */
                    for (i = 1, j = 0;
                         i < len; ++i, ++j) { /* i 为 initial_data 的索引，应该从 1 开始，j 是 buf 的索引，应该从 0 开始。结束条件为 i 的索引 达到最大 */
                        if (j < SMBUS_EEPROM_BUF_SIZE) { /* 避免越界 */
                            ptrSmbusEepromSType->buf[j] = initial_data[i];
                        } else {
                            /* 越界 ！ */
//                            printf("smbus eeprom buf size is 256, but args is more - '%lu'", len - 1);
//                            exit(1);
                        }
                    }
                    i--; /* 此时的 i 就等于 len，所以 i-1 此处等同于 initial data 的最后一个值 */
                    if (j < SMBUS_EEPROM_BUF_SIZE - 1) { /* 如果全部赋值方式没有全部赋到值，用最后一个 Initial data 的值，赋给所有剩下的值 */
                        for (; j < SMBUS_EEPROM_BUF_SIZE; j++) {
                            ptrSmbusEepromSType->buf[j] = initial_data[i];
                        }
                    }
                } else if (initial_data[0] == 0x01) { /* 按地址赋值方式 */
                    /* 一个地址，一个值 */
                    for (uint64_t addr_i = 1, val_i = 2; val_i < len; addr_i += 2, val_i += 2) {
                        if (initial_data[addr_i] < SMBUS_EEPROM_BUF_SIZE) {
                            ptrSmbusEepromSType->buf[initial_data[addr_i]] = initial_data[val_i];
                        } else {
                            /* 越界 ！ */
//                            printf("This address - '%02x' exceeds the maximum! \n", initial_data[addr_i]);
//                            exit(1);
                        }
                    }
                }
            }
            free(initial_data);
            break;

        case SMBUS_TMP:
            /* TMP */
            ptrSmbusTmpSType = (PTR_SMBUS_TMP_sTYPE) ((PTR_SMBUS_DEVICE_DATA) vPtrDeviceData)->data_buf;
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            sprintf(temp, "temperature change - %d ==> ", ptrSmbusTmpSType->temperature_integer);
            file_log(temp, LOG_TIME);
            if (len >= 1) {
                ptrSmbusTmpSType->temperature_integer = initial_data[0];
            }
            if (len >= 2) {
                ptrSmbusTmpSType->temperature_decimal = initial_data[1];
            }
            free(initial_data);
            sprintf(temp, "%d C ", ptrSmbusTmpSType->temperature_integer);
            file_log(temp, LOG_END);
            break;

        case SMBUS_TPA626:
            /* TPA626 */
            ptrSmbusTpa626SType = (PTR_SMBUS_TPA626_sTYPE) ((PTR_SMBUS_DEVICE_DATA) vPtrDeviceData)->data_buf;
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            if (len >= 2) {
                for (i = 0, j = 1; j < len; ++i, ++j) {
                    /* i: 地址的索引， j: 值的索引 */
                    switch (initial_data[i]) {
                        case 0x00: /* 00h  R/W 配置寄存器 */
                            ptrSmbusTpa626SType->configuration_reg00 = (uint16_t) initial_data[j];
                            break;
                        case 0x01: /* 01h R 并联电压寄存器 */
                            ptrSmbusTpa626SType->shunt_vol_reg01 = (uint16_t) initial_data[j];
                            break;
                        case 0x02: /* 02h R Bus电压寄存器 */
                            ptrSmbusTpa626SType->bus_vol_reg02 = (uint16_t) initial_data[j];
                            break;
                        case 0x03:
                            ptrSmbusTpa626SType->power_reg03 = (uint16_t) initial_data[j];
                            break;
                        case 0x04:
                            ptrSmbusTpa626SType->current_reg04 = (uint16_t) initial_data[j];
                            break;
                        case 0x05:
                            ptrSmbusTpa626SType->calibration_reg05 = (uint16_t) initial_data[j];
                            break;
                        case 0x06:
                            ptrSmbusTpa626SType->mask_enable_reg06 = (uint16_t) initial_data[j];
                            break;
                        case 0x07:
                            ptrSmbusTpa626SType->alert_limit_reg07 = (uint16_t) initial_data[j];
                            break;
                        case 0xfe:
                            ptrSmbusTpa626SType->manufacturer_id_regfe = (uint16_t) initial_data[j];
                            break;
                        case 0xff:
                            ptrSmbusTpa626SType->die_id_regff = (uint16_t) initial_data[j];
                            break;
                        default:
                            break;
                    }
                }
            }
            free(initial_data);
            break;
        case SMBUS_DIMM_TEMP:
            /* DIMM_TMP */
            ptrSmbusDimmTmpSType = (PTR_SMBUS_DIMM_TMP_sTYPE) ((PTR_SMBUS_DEVICE_DATA) vPtrDeviceData)->data_buf;
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            sprintf(temp, "temperature change - %d ==> ", ptrSmbusDimmTmpSType->temperature);
            file_log(temp, LOG_TIME);
            if (len >= 1) {
                ptrSmbusDimmTmpSType->temperature = initial_data[0];
            }
            free(initial_data);
            sprintf(temp, "%d C ", ptrSmbusDimmTmpSType->temperature);
            file_log(temp, LOG_END);
            break;
            /**************************************** I2C ****************************************/
        case I2C_EEPROM:
            /* I2C EEPROM */
            ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) ((PTR_I2C_DEVICE_DATA) vPtrDeviceData)->data_buf;
            /**************************************** 初始化检查 ****************************************/
            if (ptrI2CEepromSType->buf == NULL) {
                /* 如果 EEPROM 的 buf 还是 NULL，则意味着刚初始化，需要分配空间内存 */
                /* 而分配空间内存的大小，则使用 args 参数的 S_SIZE ... N_SIZE 控制数据 */
                ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_SIZE", &len);
                /**************************************** 检查 args 参数 ****************************************/
                if (len == 0) {
                    printf("size of i2c-eeprom must exist ! \n");
                    exit(1);
                }
                /**************************************** 赋值-初始化 ****************************************/
                ptrI2CEepromSType->total_size = ctrl_data[0]; /* total_size */
                ptrI2CEepromSType->buf = (uint8_t *) malloc(ptrI2CEepromSType->total_size); /* buf */
                memset(ptrI2CEepromSType->buf, 0xff, ptrI2CEepromSType->total_size);
                if (ptrI2CEepromSType->total_size <= 256) {
                    ptrI2CEepromSType->addr_size = 1; /* addr size */
                } else {
                    ptrI2CEepromSType->addr_size = 2; /* addr size */
                }
                free(ctrl_data);
            } /* 已完成初始化 */
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            if (len >= 2) {
                if (initial_data[0] == 0x00) { /* 全部赋值方式 */
                    /* 全部挨个赋值 */
                    for (i = 1, j = 0;
                         i < len; ++i, ++j) { /* i 为 initial_data 的索引，应该从 1 开始，j 是 buf 的索引，应该从 0 开始。结束条件为 i 的索引达到最大 */
                        if (j < ptrI2CEepromSType->total_size) { /* j为buf的索引，需避免越界 */
                            ptrI2CEepromSType->buf[j] = initial_data[i];
                        } else {
                            /* 已初始赋值的参数数量太多，已超过buf总大小 */
                            /* 应做忽略后续值的处理 */
                        }
                    }
                    --i; /* i 为 initial_data 的索引，此处 --i，则i的值为最后一个 initial_data 值的索引 */
                    /* 使用最后一个 initial_data，为剩下的所有值赋值 */
                    while (j < ptrI2CEepromSType->total_size) {
                        ptrI2CEepromSType->buf[j] = initial_data[i];
                        j++; /* j 为 buf 的索引，自增进行下一个赋值 */
                    }
                } else if (initial_data[0] == 0x01) { /* 按地址赋值方式 */
                    for (uint64_t addr_i = 1, val_i = 2; val_i < len; addr_i += 2, val_i += 2) {
                        if (initial_data[addr_i] < ptrI2CEepromSType->total_size) {
                            ptrI2CEepromSType->buf[initial_data[addr_i]] = initial_data[val_i];
                        } else {
                            /* 要赋值的地址超过了最大地址 */
//                            printf("The buf addr - %u exceed max - %u", initial_data[addr_i],
//                                   ptrI2CEepromSType->total_size);
                        }
                    }
                }
            }
            free(initial_data);
            /* 监视数据地址设置 */
            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_MONITOR", &len);
            if (len > 0 && len <= I2C_EEPROM_TYPE_MAX_MONITOR_DATA_LEN) {
                for (int k = 0; k < len; ++k) {
                    ptrI2CEepromSType->monitor_data[k] = ctrl_data[k];
                }
                ptrI2CEepromSType->monitor_data_len = len;
            }
            free(ctrl_data);
            break;
        case I2C_BP_CPLD:
            ptrI2CBpCpldSType = (PTR_I2C_BP_CPLD_sTYPE) ((PTR_I2C_DEVICE_DATA)vPtrDeviceData)->data_buf;
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            if (len >= 2) {
                for (int reg_i = 0, val_i = 1; val_i < len; ++reg_i, ++val_i) {
                    switch (initial_data[reg_i]) {
                        case 0x74:
                            ptrI2CBpCpldSType->i2CBpCpldData.cpld_manufacture = initial_data[val_i];
                            break;
                        case 0x90:
                            ptrI2CBpCpldSType->i2CBpCpldData.max_hdd_num = initial_data[val_i];
                            break;
                        case 0x96:
                            ptrI2CBpCpldSType->i2CBpCpldData.revision = initial_data[val_i];
                            break;
                        case 0x97:
                            ptrI2CBpCpldSType->i2CBpCpldData.bp_type_id = initial_data[val_i];
                            break;
                        case 0x98:
                            ptrI2CBpCpldSType->i2CBpCpldData.update_flag = initial_data[val_i];
                            break;
                        case 0x9B:
                            ptrI2CBpCpldSType->i2CBpCpldData.exp_manufacture = initial_data[val_i];
                            break;
                        case 0xA0 ... 0xBF:
                            ptrI2CBpCpldSType->i2CBpCpldData.bp_name[initial_data[reg_i] - 0xA0] = (char )initial_data[val_i];
                            break;
                        default:
                            break;
                    }
                }
            }
            free(initial_data);
            /**************************************** HDD status ****************************************/
            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_HDD_STATUS", &len);
            if (len >= 2) {
                for (int hdd_num_i = 0, status_i = 1; status_i < len; ++hdd_num_i, ++status_i) {
                    if (ctrl_data[hdd_num_i] < ptrI2CBpCpldSType->i2CBpCpldData.max_hdd_num) {
                        memset(&ptrI2CBpCpldSType->i2CBpCpldData.hdd_status[ctrl_data[hdd_num_i]], (uint8_t)ctrl_data[status_i], 1);
                    }
                }
            }
            free(ctrl_data);
            break;
        case I3C_DIMM_TEMP:
            /* I3C DIMM_TMP */
            ptrI3cDimmTmpSType = (PTR_I2C_LEGACY_DIMM_TMP_sTYPE) ((PTR_I2C_DEVICE_DATA) vPtrDeviceData)->data_buf;
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            sprintf(temp, "temperature change - %d ==> ", ptrI3cDimmTmpSType->temperature);
            file_log(temp, LOG_TIME);
            if (len >= 1) {
                ptrI3cDimmTmpSType->temperature = initial_data[0];
            }
            free(initial_data);
            sprintf(temp, "%d C ", ptrI3cDimmTmpSType->temperature);
            file_log(temp, LOG_END);
            break;
        case PMBUS_PSU:
            pmBusPage = (PMBusPage *)vPtrDeviceData;
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            if (len < 2) {
                break;
            }
            int data_i = 0;
            uint32_t n = 0, kj = 0;
            uint32_t reg_type;
            char *temp_mfr;
            while (data_i < len) {
                reg_type = initial_data[data_i++];
                switch (reg_type) {
                    case 0 ... 6:
                        /* 0 mfr_id
                         * 1 mfr_model
                         * 2 mfr_revision
                         * 3 mfr_location
                         * 4 mfr_date
                         * 5 mfr_serial
                         * 6 mfr_version   */
                        /* 第一个数表述数据长度 n */
                        /* 后续 n 个字符被视为 char 类型 */
                        if (data_i >= len) {
                            break;
                        }
                        n = initial_data[data_i++];
                        if (data_i - 1 + n >= len) {
                            break;
                        }
                        if (reg_type == 0) {
                            temp_mfr = pmBusPage->mfr_id;
                        } else if (reg_type == 1) {
                            temp_mfr = pmBusPage->mfr_model;
                        } else if (reg_type == 2) {
                            temp_mfr = pmBusPage->mfr_revision;
                        } else if (reg_type == 3) {
                            temp_mfr = pmBusPage->mfr_location;
                        } else if (reg_type == 4) {
                            temp_mfr = pmBusPage->mfr_date;
                        } else if (reg_type == 5) {
                            temp_mfr = pmBusPage->mfr_serial;
                        } else if (reg_type == 6) {
                            temp_mfr = pmBusPage->version;
                        }
                        for (kj = 0; kj < n && kj < 50-1; ++kj) {
                            temp_mfr[kj] = (char) initial_data[data_i++];
                        }
                        temp_mfr[kj] = 0;
                        break;
                    case 7:
                        /* mfr_specific */
                        if (data_i >= len) {
                            break;
                        }
                        pmBusPage->status_mfr_specific = (int8_t) initial_data[data_i++];
                        break;
                    case 8 ... 18:
                        /* mfr_pout_max 
                         * read_vout
                         * read_iout
                         * read_pout
                         * read_vin
                         * read_iin
                         * read_pin
                         * read_temperature_1
                         * read_temperature_2
                         * read_fan_speed_1
                         **/
                        if (data_i >= len) {
                            break;
                        }
                        if (reg_type == 8) {
                            pmBusPage->mfr_pout_max = (uint16_t) initial_data[data_i++];
                        } else if (reg_type == 9) {
                            pmBusPage->read_vout = (uint16_t) initial_data[data_i++];
                        } else if (reg_type == 10) {
                            pmBusPage->read_iout = (uint16_t) initial_data[data_i++];
                        } else if (reg_type == 11) {
                            pmBusPage->read_pout = (uint16_t) initial_data[data_i++];
                        } else if (reg_type == 12) {
                            pmBusPage->read_vin = (uint16_t) initial_data[data_i++];
                        } else if (reg_type == 13) {
                            pmBusPage->read_iin = (uint16_t) initial_data[data_i++];
                        } else if (reg_type == 14) {
                            pmBusPage->read_pin = (uint16_t) initial_data[data_i++];
                        } else if (reg_type == 15) {
                            pmBusPage->read_temperature_1 = (uint16_t) initial_data[data_i++];
                        } else if (reg_type == 16) {
                            pmBusPage->read_temperature_2 = (uint16_t) initial_data[data_i++];
                        } else if (reg_type == 17) {
                            pmBusPage->read_fan_speed_1 = (uint16_t) initial_data[data_i++];
                        }
                        break;
                    default:
                        break;
                }
            }
            free(initial_data);
            break;
        case GPIO_SWITCH:
            ptrGpioDeviceData = (PTR_GPIO_DEVICE_DATA) vPtrDeviceData;
            ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE) ptrGpioDeviceData->data_buf;
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            if (len < 1) {
                break;
            }

            /* 第0个参数表示 pin_num */
            if (initial_data[0] >= ptrGpioDeviceData->pin_nums) {
                /* 指定的 pin_num 超过了总数 */
                break;
            }

            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_WAVEFORM", &len);
            if (len > 0) {
                pthread_mutex_lock(&ptrGpioSwitchSType->pin_state_mutex[initial_data[0]]);
                ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].waveform_len = 0;
                for (int k = 0; k < len && k < PIN_WAVEFORM_BUF_MAX_BIT-1; ++k) {
                    if (ctrl_data[k] == 0) {
                        ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]]
                        .waveform_buf[ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].waveform_len++] = FALSE;
                    } else {
                        ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]]
                        .waveform_buf[ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].waveform_len++] = TRUE;
                    }
                }
                ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].stop_waveform_generator = FALSE;
                ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].has_update = TRUE;
                pthread_mutex_unlock(&ptrGpioSwitchSType->pin_state_mutex[initial_data[0]]);
            }
            free(ctrl_data);

            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_WAVEFORM_TYPE", &len);
            if (len > 0) {
                pthread_mutex_lock(&ptrGpioSwitchSType->pin_state_mutex[initial_data[0]]);
                if (ctrl_data[0] == 0) {
                    ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].pinWaveformType = PIN_WAVEFORM_TYPE_ONCE;
                    ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].stop_waveform_generator = FALSE;
                    ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].has_update = TRUE;
                } else if (ctrl_data[0] == 1) {
                    ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].pinWaveformType = PIN_WAVEFORM_TYPE_REPEAT;
                    ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].stop_waveform_generator = FALSE;
                    ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].has_update = TRUE;
                }
                pthread_mutex_unlock(&ptrGpioSwitchSType->pin_state_mutex[initial_data[0]]);
            }
            free(ctrl_data);

            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_WAVEFORM_RATE", &len);
            if (len > 0) {
                pthread_mutex_lock(&ptrGpioSwitchSType->pin_state_mutex[initial_data[0]]);
                if (ctrl_data[0] <= PIN_WAVEFORM_RATE_MAX) {
                    ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].waveform_rate = ctrl_data[0];
                    ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].stop_waveform_generator = FALSE;
                    ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].has_update = TRUE;
                }
                pthread_mutex_unlock(&ptrGpioSwitchSType->pin_state_mutex[initial_data[0]]);
            }
            free(ctrl_data);

            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_WAVEFORM_STOP", &len);
            if (len > 0) {
                pthread_mutex_lock(&ptrGpioSwitchSType->pin_state_mutex[initial_data[0]]);
                ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].stop_waveform_generator = (ctrl_data[0] != 0);
                ptrGpioSwitchSType->pinWaveformGenerator_list[initial_data[0]].has_update = TRUE;
                pthread_mutex_unlock(&ptrGpioSwitchSType->pin_state_mutex[initial_data[0]]);
            }
            free(ctrl_data);

            free(initial_data);
            break;
        case ADC:
            ptrAdcDeviceData = (PTR_ADC_DEVICE_DATA) vPtrDeviceData;
            initial_data = detachArgsData(args, DETACH_INITIAL_DATA, NULL, &len);
            if (len < 1) {
                ;
            } else {
                uint32_t temp_adc_value;
                sprintf(temp, "%2x", initial_data[0]);
                temp_adc_value = strtoul(temp, &end, 10);
                if (*end != '\0') {
                    /* 停止符 不为 \0，表示遇到非10进制字符，此时非预期值，应该跳出设置 */
                    ;
                } else {
                    ptrAdcDeviceData->set_adc_value = temp_adc_value;
                }
            }

            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_ADC_REGISTER", &len);
            if (len < 1) {
                ;
            } else {
                if (ptrAdcDeviceData->adcRegType == REG_L) {
                    ptrAdcDeviceData->ptrAdcReg->reg_lh.l = ctrl_data[0];
                } else {
                    ptrAdcDeviceData->ptrAdcReg->reg_lh.h = ctrl_data[0];
                }
            }
            free(ctrl_data);

            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_DIVISION", &len);
            if (len < 1) {
                ;
            } else {
                ptrAdcDeviceData->division = ctrl_data[0];
            }
            free(ctrl_data);

            free(initial_data);
            break;
        case PWM_TACH:
            /* PWM_TACH */
            ptrPwmTachDevice = (PTR_PWM_TACH_DEVICE) vPtrDeviceData;
            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_MIN_RPM", &len);
            if (len < 1) {
                ;
            } else {
                ptrPwmTachDevice->ptrRpmDuty->min_rpm = ctrl_data[0];
            }
            free(ctrl_data);
            
            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_MAX_RPM", &len);
            if (len < 1) {
                ;
            } else {
                ptrPwmTachDevice->ptrRpmDuty->max_rpm = ctrl_data[0];
            }
            free(ctrl_data);
            
            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_MIN_OFFSET", &len);
            if (len < 1) {
                ;
            } else {
                ptrPwmTachDevice->ptrRpmDuty->min_offset = (ctrl_data[0] % 1001) / 1000.0;
            }
            free(ctrl_data);
            
            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_MAX_OFFSET", &len);
            if (len < 1) {
                ;
            } else {
                ptrPwmTachDevice->ptrRpmDuty->max_offset = (ctrl_data[0] % 1001) / 1000.0;
            }
            free(ctrl_data);
            
            ctrl_data = detachArgsData(args, DETACH_CTRL_DATA, "_RAND_DEVIATION_RATE", &len);
            if (len < 1) {
                ;
            } else {
                ptrPwmTachDevice->ptrRpmDuty->rand_deviation_rate = (int)ctrl_data[0];
            }
            free(ctrl_data);
            
            break;
        default:
            break;
    }
}


PTR_CONFIG_DATA parse_configuration(void) {
    FUNC_DEBUG("parse_configuration()")
    PTR_CONFIG_DATA ptrConfigData = (PTR_CONFIG_DATA) malloc(sizeof(CONFIG_DATA));
    PTR_DEVICE_CONFIG ptrConfigJson = NULL;
    /**************************************** 解析 JSON 文件内容 ****************************************/
    cJSON *root = read_config_file(CONFIG_FILE);
    // 获取"devices"数组
    cJSON *devices_array = cJSON_GetObjectItem(root, "devices");
    if (!devices_array || !cJSON_IsArray(devices_array)) {
        printf("'devices' array not found.\n");
        cJSON_Delete(root);
        exit(1);
    }
    // 获取 “mac” 对象
    cJSON *mac_obj = cJSON_GetObjectItem(root, "mac");
    if (!mac_obj || !cJSON_IsObject(mac_obj)) {
        printf("'mac' object not found. \n");
        cJSON_Delete(root);
        exit(1);
    }
    /**************************************** 解析 devices ****************************************/
    int num_devices = cJSON_GetArraySize(devices_array);
    char temp_desc[256];
    for (int i = 0; i < num_devices; ++i) {
        FUNC_DEBUG("function: parse_configuration() -> for num_devices")
        /* 获取 devices 数组的第 i 个 */
        PTR_DEVICE_CONFIG tempConfigJson = (PTR_DEVICE_CONFIG) malloc(sizeof(DEVICE_CONFIG));
        memset(tempConfigJson, 0, sizeof(DEVICE_CONFIG));
        cJSON *device = cJSON_GetArrayItem(devices_array, i);
        FUNC_DEBUG("function: parse_configuration() -> desc & device_type_id _ debug1")
        /**************************************** desc  device_type_id ****************************************/
        cJSON *description = cJSON_GetObjectItem(device, "description");
        cJSON *device_name = cJSON_GetObjectItem(device, "name");
        cJSON *device_type_id = cJSON_GetObjectItem(device, "device_type_id");
        cJSON *bus = cJSON_GetObjectItem(device, "bus");
        cJSON *addr = cJSON_GetObjectItem(device, "addr");
        cJSON *i2c = cJSON_GetObjectItem(device, "i2c");
        cJSON *args = cJSON_GetObjectItem(device, "args");
        cJSON *i2c_legacy = cJSON_GetObjectItem(device, "i2c_legacy");

        cJSON *adc_channel = cJSON_GetObjectItem(device, "adc_channel");
        cJSON *division = cJSON_GetObjectItem(device, "division");

        cJSON *pwm_tach_num = cJSON_GetObjectItem(device, "pwm_tach_num");
        /* 校验 */
        if (description == NULL) {
            sprintf(temp_desc, "devices[%d] ", i+1);  /* i+1, 因为第 0 个 device 是 BMC SOC */
            description = cJSON_CreateString(temp_desc);
        } else {
            sprintf(temp_desc, "devices[%d] - %s ", i+1, description->valuestring);  /* i+1, 因为第 0 个 device 是 BMC SOC */
            description = cJSON_CreateString(temp_desc);
        }
        tempConfigJson->description = description->valuestring;

        /* name */
        if (device_name == NULL) {
            sprintf(tempConfigJson->name, "undefined_name-%d", i+1);
        } else if (device_name->type != cJSON_String) {
            printf("The devices[%d]: 'name' is not a string ! \n", i);
            exit(1);
        } else {
            if (strlen(device_name->valuestring) > DEVICE_NAME_MAX_LEN) {
                printf("The devices[%d]: 'name' too long! (exceed max len 100) \n", i);
                exit(1);
            }
            strcpy(tempConfigJson->name, device_name->valuestring);
        }

        /* device_type_id */
        if (device_type_id == NULL) {
            printf("The devices[%d]: 'device_type_id' not found, but it is necessary! \n", i);
            exit(1);
        } else if (device_type_id->type != cJSON_Number) {
            printf("The devices[%d]: 'device_type_id' is not a number ! \n", i);
            exit(1);
        }
        tempConfigJson->device_type_id = device_type_id->valueint;

        DEVICE_TYPE deviceType = get_device_type(device_type_id->valueint);

        tempConfigJson->deviceType = deviceType;

        /**************************************** bus addr i2c args ****************************************/
        FUNC_DEBUG("function: parse_configuration() -> bus & addr")
        if (deviceType == SMBUS_DEVICE_TYPE ||
            deviceType == I2C_DEVICE_TYPE ||
            deviceType == PMBUS_DEVICE_TYPE ||
            deviceType == PCA954X_DEVICE_TYPE ||
            deviceType == I3C_DEVICE_TYPE) {
            /* bus */
            if (bus == NULL) {
                printf("The devices[%d]: 'bus' not found, but it is necessary! \n", i);
                exit(1);
            } else if (bus->type != cJSON_Number) {
                printf("The devices[%d]: 'bus' is not a number ! \n", i);
                exit(1);
            }
            tempConfigJson->bus = bus->valueint;

            /* addr */
            if (addr == NULL) { /* 没有 addr 字段 */
                printf("The devices[%d]: 'addr' not found, but it is necessary! \n", i);
                exit(1);
            } else if (addr->type == cJSON_String) { /* addr 字段是字符串 */
                char *endPtr;
                tempConfigJson->addr = (uint8_t) strtol(addr->valuestring, &endPtr, 16);
                if (*endPtr != '\0') { /* 检验字符串是否正常解析 */
                    printf("The devices[%d]: 'addr - %s' is not one hex string !\n", i, addr->valuestring);
                    exit(1);
                }
            } else if (addr->type == cJSON_Number) { /* addr 字段是数字 */
                tempConfigJson->addr = (uint8_t) addr->valueint;  /* 直接使用 valueint，不考虑小数情况 */
            } else { /* addr 字段是其他非预期值 */
                printf("The devices[%d]: 'addr' can not parse! \n", i);
                exit(1);
            }

            /* i2c */
            if (i2c == NULL) {
                /* 没有指定 i2c 字段，即默认为 0 - BMC，应自动填充 index 和 name 字段，以方便直接使用 */
                tempConfigJson->i2c_dev.device_index = 0;
                sprintf(tempConfigJson->i2c_dev.device_name, "BMC");
            } else if (i2c->type == cJSON_Object) {
                /* i2c_device */
                cJSON *i2c_device = cJSON_GetObjectItem(i2c, "device");
                if (i2c_device == NULL) {
                    printf("The devices[%d]: 'i2c > device' not found ! \n", i);
                    exit(1);
                } else if (i2c_device->type == cJSON_Number) {
                    tempConfigJson->i2c_dev.device_index = i2c_device->valueint;
                    sprintf(tempConfigJson->i2c_dev.device_name, "device[%d]", tempConfigJson->i2c_dev.device_index);
                } else if (i2c_device->type == cJSON_String) {
                    tempConfigJson->i2c_dev.device_index = -1;  /* 设置为 -1，表示是无效的，index 应该留至之后设置 */
                    strcpy(tempConfigJson->i2c_dev.device_name, i2c_device->valuestring);
                } else {
                    printf("The devices[%d]: 'i2c > device' is not a number or string ! \n", i);
                    exit(1);
                }
            } else {
                printf("The devices[%d]: 'i2c' is not a object ! \n", i);
                exit(1);
            }

            /* i2c Legacy mode*/
            if (i2c_legacy == NULL) {
            } else if (i2c_legacy->type != cJSON_False && i2c_legacy->type != cJSON_True) {
                printf("The devices[%d]: 'i2c_legacy' is not a bool ! \n", i);
                exit(1);
            } else {
                tempConfigJson->i2clegacy = i2c_legacy->valueint;
            }
        }
            /**************************************** adc_channel ****************************************/
        else if (deviceType == ADC_DEVICE_TYPE) {
            /* adc channel */
            if (adc_channel == NULL) {
                printf("The devices[%d]: 'adc_channel' not found, but it is necessary! \n", i);
                exit(1);
            } else if (adc_channel->type != cJSON_Number) {
                printf("The devices[%d]: 'adc_channel' it is not a number! \n", i);
                exit(1);
            }
            tempConfigJson->adc_channel = adc_channel->valueint;

            /* division */
            if (division == NULL) {
                printf("The devices[%d]: 'division' not found, but it is necessary! \n", i);
                exit(1);
            } else if (division->type != cJSON_Number) {
                printf("The devices[%d]: 'division' it is not a number! \n", i);
                exit(1);
            }
            tempConfigJson->division = division->valuedouble;
        }
        /**************************************** pwm_tach ****************************************/
        else if (deviceType == PWM_TACH_DEVICE_TYPE) {
            /* pwm_tach_num */
            if (pwm_tach_num == NULL) {
                printf("The devices[%d]: 'pwm_tach_num' not found, but it is necessary! \n", i);
                exit(1);
            } else if (pwm_tach_num->type != cJSON_Number) {
                printf("The devices[%d]: 'pwm_tach_num' it is not a number! \n", i);
                exit(1);
            }
            tempConfigJson->pwm_tach_num = pwm_tach_num->valueint;
        }

        FUNC_DEBUG("function: parse_configuration() -> args")
        /* args */
        if (args == NULL) {
            tempConfigJson->args = NULL;
        } else if (args->type != cJSON_String) {
            printf("The devices[%d]: 'args' is not string! \n", i);
            exit(1);
        } else {
            tempConfigJson->args = args->valuestring;
        }

        FUNC_DEBUG("function: parse_configuration() -> pin_nums")
        /**************************************** pin_nums ****************************************/
        cJSON *pin_nums = cJSON_GetObjectItem(device, "pin_nums");
        /* pin_nums */
        if (pin_nums == NULL) {
            /* 无 pin */
            pin_nums = cJSON_CreateNumber(0);
        } else if (pin_nums->type != cJSON_Number) {
            printf("The devices[%d]: 'pin_nums' is not a number ! \n", i);
            exit(1);
        }
        tempConfigJson->pin_nums = pin_nums->valueint;

        FUNC_DEBUG("function: parse_configuration() -> signal")
        /**************************************** signal ****************************************/
        PTR_GPIO_SIGNAL ptrGpioSignal = NULL;
        PTR_GPIO_OUTPUT_LOGIC_INIT ptrGpioOutputLogicInit = NULL;
        PTR_GPIO_RESPONSE_LOGIC ptrGpioResponseLogic = NULL;
        uint32_t gpioSignal_num = 0;
        cJSON *signal = cJSON_GetObjectItem(device, "signal");
        if (signal == NULL) {
            /* 无 signal */
            ptrGpioSignal = NULL;
        } else if (signal->type != cJSON_Object) {
            printf("The devices[%d]: 'signal' is not a object ! \n", i);
            exit(1);
        } else {
            /* 有 signal */
            FUNC_DEBUG("function: parse_configuration() -> input_lines")
            /**************************************** input_lines ****************************************/
            cJSON *input_lines = cJSON_GetObjectItem(signal, "input_lines");
            if (input_lines == NULL) {
                /* 无 input_lines */
            } else if (input_lines->type != cJSON_Array) {
                /* 非 数组类型 */
                printf("The devices[%d]: 'input_lines' is not a array ! \n", i);
                exit(1);
            } else {
                /* 有 input_lines */
                int input_lines_size = cJSON_GetArraySize(input_lines);
                gpioSignal_num += input_lines_size;
                for (int j = 0; j < input_lines_size; ++j) {
                    cJSON *temp_input_line = cJSON_GetArrayItem(input_lines, j);
                    if (temp_input_line->type != cJSON_Array) {
                        /* 格式不对，input_lines值不为数组 */
                        printf("The devices[%d]: 'input_lines[%d]' is not a array ! \n", i, j);
                        exit(1);
                    } else {
                        int one_line_size = cJSON_GetArraySize(temp_input_line);
                        if (one_line_size != 3) {
                            /* 数量不对 */
                            printf("The devices[%d]: 'input_lines[%d]' - size != 3 . \n", i, j);
                            exit(1);
                        }
                        cJSON *send_dev = cJSON_GetArrayItem(temp_input_line, 0);
                        cJSON *send_dev_line = cJSON_GetArrayItem(temp_input_line, 1);
                        cJSON *recv_line = cJSON_GetArrayItem(temp_input_line, 2);

                        if (send_dev->type != cJSON_Number && send_dev->type != cJSON_String) {
                            printf("The devices[%d]: 'input_lines[%d][0]' is not a number or string ! \n", i, j);
                            exit(1);
                        }
                        if (send_dev_line->type != cJSON_Number) {
                            printf("The devices[%d]: 'input_lines[%d][1]' is not a number ! \n", i, j);
                            exit(1);
                        }
                        if (recv_line->type != cJSON_Number) {
                            printf("The devices[%d]: 'input_lines[%d][1]' is not a number ! \n", i, j);
                            exit(1);
                        }

                        PTR_GPIO_SIGNAL tempPtrGpioSignal = (PTR_GPIO_SIGNAL) malloc(sizeof(GPIO_SIGNAL));
                        if (tempPtrGpioSignal == NULL) {
                            perror("malloc: ");
                        }

                        tempPtrGpioSignal->gpioSignalType = GPIO_SIGNAL_INPUT;
                        if (send_dev->type == cJSON_Number) {
                            tempPtrGpioSignal->gpioSignal.gpioSignalInputLine.send_device_index = send_dev->valueint;
                            tempPtrGpioSignal->gpioSignal.gpioSignalInputLine.send_device_name[0] = 0;
                        } else if (send_dev->type == cJSON_String) {
                            tempPtrGpioSignal->gpioSignal.gpioSignalInputLine.send_device_index = -1;
                            strcpy(tempPtrGpioSignal->gpioSignal.gpioSignalInputLine.send_device_name, send_dev->valuestring);
                        }
                        tempPtrGpioSignal->gpioSignal.gpioSignalInputLine.send_device_line = send_dev_line->valueint;
                        tempPtrGpioSignal->gpioSignal.gpioSignalInputLine.recv_line = recv_line->valueint;
                        tempPtrGpioSignal->pre = NULL;
                        tempPtrGpioSignal->next = NULL;

                        if (ptrGpioSignal == NULL) {
                            ptrGpioSignal = tempPtrGpioSignal;
                        } else {
                            tempPtrGpioSignal->pre = ptrGpioSignal;
                            ptrGpioSignal->next = tempPtrGpioSignal;
                            ptrGpioSignal = ptrGpioSignal->next;
                        }
                    }
                }
            }
            FUNC_DEBUG("function: parse_configuration() -> output_lines")
            /**************************************** output_lines ****************************************/
            cJSON *output_lines = cJSON_GetObjectItem(signal, "output_lines");
            if (output_lines == NULL) {
                /* 无 output_lines */
            } else if (output_lines->type != cJSON_Array) {
                /* 非 数组类型 */
                printf("The devices[%d]: 'output_lines' is not a array ! \n", i);
                exit(1);
            } else {
                /* 有 output_lines */
                int output_lines_size = cJSON_GetArraySize(output_lines);
                gpioSignal_num += output_lines_size;
                for (int j = 0; j < output_lines_size; ++j) {
                    cJSON *temp_output_line = cJSON_GetArrayItem(output_lines, j);
                    if (temp_output_line->type != cJSON_Array) {
                        /* 格式不对，output_lines值不为数组 */
                        printf("The devices[%d]: 'output_lines[%d]' is not a array ! \n", i, j);
                        exit(1);
                    } else {
                        int one_line_size = cJSON_GetArraySize(temp_output_line);
                        if (one_line_size != 3) {
                            /* 数量不对 */
                            printf("The devices[%d]: 'output_lines[%d]' - size != 3 . \n", i, j);
                            exit(1);
                        }
                        cJSON *recv_dev = cJSON_GetArrayItem(temp_output_line, 0);
                        cJSON *recv_dev_line = cJSON_GetArrayItem(temp_output_line, 1);
                        cJSON *send_line = cJSON_GetArrayItem(temp_output_line, 2);

                        if (recv_dev->type != cJSON_Number && recv_dev->type != cJSON_String) {
                            printf("The devices[%d]: 'output_lines[%d][1]' is not a number or string ! \n", i, j);
                            exit(1);
                        }

                        if (recv_dev_line->type != cJSON_Number) {
                            printf("The devices[%d]: 'output_lines[%d][1]' is not a number ! \n", i, j);
                            exit(1);
                        }
                        if (send_line->type != cJSON_Number) {
                            printf("The devices[%d]: 'output_lines[%d][0]' is not a number ! \n", i, j);
                            exit(1);
                        }

                        PTR_GPIO_SIGNAL tempPtrGpioSignal = (PTR_GPIO_SIGNAL) malloc(sizeof(GPIO_SIGNAL));
                        if (tempPtrGpioSignal == NULL) {
                            perror("malloc: ");
                        }

                        tempPtrGpioSignal->gpioSignalType = GPIO_SIGNAL_OUTPUT;
                        tempPtrGpioSignal->gpioSignal.gpioSignalOutputLine.send_line = send_line->valueint;
                        if (recv_dev->type == cJSON_Number) {
                            tempPtrGpioSignal->gpioSignal.gpioSignalOutputLine.recv_device_index = recv_dev->valueint;
                            tempPtrGpioSignal->gpioSignal.gpioSignalOutputLine.recv_device_name[0] = 0;
                        } else if (recv_dev->type == cJSON_String) {
                            tempPtrGpioSignal->gpioSignal.gpioSignalOutputLine.recv_device_index = -1;
                            strcpy(tempPtrGpioSignal->gpioSignal.gpioSignalOutputLine.recv_device_name, recv_dev->valuestring);
                        }
                        tempPtrGpioSignal->gpioSignal.gpioSignalOutputLine.recv_device_line = recv_dev_line->valueint;
                        tempPtrGpioSignal->pre = NULL;
                        tempPtrGpioSignal->next = NULL;

                        if (ptrGpioSignal == NULL) {
                            ptrGpioSignal = tempPtrGpioSignal;
                        } else {
                            tempPtrGpioSignal->pre = ptrGpioSignal;
                            ptrGpioSignal->next = tempPtrGpioSignal;
                            ptrGpioSignal = ptrGpioSignal->next;
                        }
                    }
                }
            }
            while (ptrGpioSignal != NULL && ptrGpioSignal->pre != NULL) {
                ptrGpioSignal = ptrGpioSignal->pre;
            }
            /**************************************** output_logic_init ****************************************/
            cJSON *output_logic_init = cJSON_GetObjectItem(signal, "output_logic_init");
            if (output_logic_init == NULL) {
                /* 无 output_logic_init */
            } else if (output_logic_init->type != cJSON_Array) {
                /* 非 数组类型 */
                printf("The devices[%d]: 'output_logic_init' is not a array. \n", i);
                exit(1);
            } else {
                /* 有 output_logic_init */
                int output_logic_init_size = cJSON_GetArraySize(output_logic_init);
                for (int j = 0; j < output_logic_init_size; ++j) {
                    cJSON *temp_output_logic = cJSON_GetArrayItem(output_logic_init, j);
                    if (temp_output_logic->type != cJSON_Array) {
                        /* 格式不对，output_logic_init 值不为数组 */
                        printf("The devices[%d] : 'output_logic_init[%d]' is not a array.", i, j);
                        exit(1);
                    }
                    int temp_output_logic_size = cJSON_GetArraySize(temp_output_logic);
                    if (temp_output_logic_size != 3) {
                        /* 数量不对 */
                        printf("The devices[%d]: 'output_logic_init[%d]' - size != 3", i, j);
                        exit(1);
                    }
                    cJSON *output_pin_num = cJSON_GetArrayItem(temp_output_logic, 0);
                    cJSON *output_waveform = cJSON_GetArrayItem(temp_output_logic, 1);
                    cJSON *output_waveform_type = cJSON_GetArrayItem(temp_output_logic, 2);

                    if (output_pin_num->type != cJSON_Number) {
                        printf("The devices[%d]: 'output_logic_init[0] is not a number !\n", i);
                        exit(1);
                    }
                    if (output_waveform->type != cJSON_String) {
                        printf("The devices[%d]: 'output_logic_init[1] is not a string !\n", i);
                        exit(1);
                    }
                    if (output_waveform_type->type != cJSON_Number) {
                        printf("The devices[%d]: 'output_logic_init[2] is not a number !\n", i);
                        exit(1);
                    }

                    PTR_GPIO_OUTPUT_LOGIC_INIT ptrGpioOutputLogicInit_temp = (PTR_GPIO_OUTPUT_LOGIC_INIT) malloc(
                            sizeof(GPIO_OUTPUT_LOGIC_INIT));
                    if (ptrGpioOutputLogicInit_temp == NULL) {
                        perror("malloc: ");
                    }

                    ptrGpioOutputLogicInit_temp->output_pin_num = output_pin_num->valueint;
                    ptrGpioOutputLogicInit_temp->output_waveform = output_waveform->valuestring;
                    ptrGpioOutputLogicInit_temp->output_waveform_type = output_waveform_type->valueint;
                    ptrGpioOutputLogicInit_temp->pre = NULL;
                    ptrGpioOutputLogicInit_temp->next = NULL;

                    if (ptrGpioOutputLogicInit == NULL) {
                        ptrGpioOutputLogicInit = ptrGpioOutputLogicInit_temp;
                    } else {
                        ptrGpioOutputLogicInit_temp->pre = ptrGpioOutputLogicInit;
                        ptrGpioOutputLogicInit->next = ptrGpioOutputLogicInit_temp;
                        ptrGpioOutputLogicInit = ptrGpioOutputLogicInit->next;
                    }
                }
            }
            while (ptrGpioOutputLogicInit != NULL && ptrGpioOutputLogicInit->pre != NULL) {
                ptrGpioOutputLogicInit = ptrGpioOutputLogicInit->pre;
            }
            /**************************************** response_logic ****************************************/
            cJSON *response_logic = cJSON_GetObjectItem(signal, "response_logic");
            if (response_logic == NULL) {
                /* 无 response_logic */
            } else if (response_logic->type != cJSON_Array) {
                /* 非 数组类型 */
                printf("The devices[%d]: 'response_logic' is not a array !\n", i);
                exit(1);
            } else {
                /* 有response_logic */
                int response_logic_size = cJSON_GetArraySize(response_logic);
                for (int j = 0; j < response_logic_size; ++j) {
                    cJSON *temp_response_logic = cJSON_GetArrayItem(response_logic, j);
                    if (temp_response_logic->type != cJSON_Array) {
                        /* 格式错误，response_logic值不为数组 */
                        printf("The devices[%d]: 'response_logic[%d] is not a array !\n", i, j);
                        exit(1);
                    }
                    int temp_response_logic_size = cJSON_GetArraySize(temp_response_logic);
                    if (temp_response_logic_size != 5) {
                        /* 数量不对 */
                        printf("The devices[%d]: 'response_logic[%d] - size != 5 \n", i, j);
                        exit(1);
                    }
                    cJSON *input_pin_num = cJSON_GetArrayItem(temp_response_logic, 0);
                    cJSON *input_status_order = cJSON_GetArrayItem(temp_response_logic, 1);
                    cJSON *output_pin_num = cJSON_GetArrayItem(temp_response_logic, 2);
                    cJSON *output_waveform = cJSON_GetArrayItem(temp_response_logic, 3);
                    cJSON *output_waveform_type = cJSON_GetArrayItem(temp_response_logic, 4);

                    if (input_pin_num->type != cJSON_Number) {
                        printf("The devices[%d]: 'response[0] is not a number !\n", i);
                        exit(1);
                    }
                    if (input_status_order->type != cJSON_String) {
                        printf("The devices[%d]: 'response[1] is not a string !\n", i);
                        exit(1);
                    }
                    if (output_pin_num->type != cJSON_Number) {
                        printf("The devices[%d]: 'response[2] is not a number !\n", i);
                        exit(1);
                    }
                    if (output_waveform->type != cJSON_String) {
                        printf("The devices[%d]: 'response[3] is not a string !\n", i);
                        exit(1);
                    }
                    if (output_waveform_type->type != cJSON_Number) {
                        printf("The devices[%d]: 'response[4] is not a number !\n", i);
                        exit(1);
                    }

                    PTR_GPIO_RESPONSE_LOGIC ptrGpioResponseLogic_temp = (PTR_GPIO_RESPONSE_LOGIC) malloc(
                            sizeof(GPIO_RESPONSE_LOGIC));
                    if (ptrGpioResponseLogic_temp == NULL) {
                        perror("malloc");
                    }

                    ptrGpioResponseLogic_temp->input_pin_num = input_pin_num->valueint;
                    ptrGpioResponseLogic_temp->input_status_order = input_status_order->valuestring;
                    ptrGpioResponseLogic_temp->output_pin_num = output_pin_num->valueint;
                    ptrGpioResponseLogic_temp->output_waveform = output_waveform->valuestring;
                    ptrGpioResponseLogic_temp->output_waveform_type = output_waveform_type->valueint;
                    ptrGpioResponseLogic_temp->pre = NULL;
                    ptrGpioResponseLogic_temp->next = NULL;

                    if (ptrGpioResponseLogic == NULL) {
                        ptrGpioResponseLogic = ptrGpioResponseLogic_temp;
                    } else {
                        ptrGpioResponseLogic_temp->pre = ptrGpioResponseLogic;
                        ptrGpioResponseLogic->next = ptrGpioResponseLogic_temp;
                        ptrGpioResponseLogic = ptrGpioResponseLogic->next;
                    }
                }
            }
            while (ptrGpioResponseLogic != NULL && ptrGpioResponseLogic->pre != NULL) {
                ptrGpioResponseLogic = ptrGpioResponseLogic->pre;
            }
            /**************************************** a device config END ****************************************/
        }
        tempConfigJson->ptrGpioSignal = ptrGpioSignal;
        tempConfigJson->ptrGpioOutputLogicInit = ptrGpioOutputLogicInit;
        tempConfigJson->ptrGpioResponseLogic = ptrGpioResponseLogic;

        if (ptrConfigJson == NULL) {
            ptrConfigJson = tempConfigJson;
        } else {
            tempConfigJson->pre = ptrConfigJson;
            ptrConfigJson->next = tempConfigJson;
            ptrConfigJson = ptrConfigJson->next;
        }
    }
    while (ptrConfigJson != NULL && ptrConfigJson->pre != NULL) {
        ptrConfigJson = ptrConfigJson->pre;
    }
    ptrConfigData->ptrDeviceConfig = ptrConfigJson;
    /**************************************** 解析 mac ****************************************/
    ptrConfigData->ptrMacConfig = (PTR_MAC_CONFIG) malloc(sizeof(MAC_CONFIG));
    memset(ptrConfigData->ptrMacConfig, 0, sizeof(MAC_CONFIG));
    /**************************************** mac_address ****************************************/
    cJSON *mac_address = cJSON_GetObjectItem(mac_obj, "mac_address");
    if (!mac_address || !cJSON_IsString(mac_address)) {
        printf("mac_address parse failed!\n");
    } else {
        int res = sscanf(mac_address->valuestring, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &ptrConfigData->ptrMacConfig->mac[0],
               &ptrConfigData->ptrMacConfig->mac[1],
               &ptrConfigData->ptrMacConfig->mac[2],
               &ptrConfigData->ptrMacConfig->mac[3],
               &ptrConfigData->ptrMacConfig->mac[4],
               &ptrConfigData->ptrMacConfig->mac[5]
               );
        if (res != 6) {
            printf("\nmac地址解析失败：'%s' ，请检查格式是否正确！\n", mac_address->valuestring);
            exit(1);
        }
    }
    /**************************************** mac_apply_method ****************************************/
    cJSON *mac_apply_method = cJSON_GetObjectItem(mac_obj, "mac_apply_method");
    if (!mac_apply_method || !cJSON_IsNumber(mac_apply_method)) {
        printf("mac_apply_method parse failed!\n");
    } else {
        ptrConfigData->ptrMacConfig->macApplyMethod = mac_apply_method->valueint;
    }

    FUNC_DEBUG("function: parse_configuration() End")
    return ptrConfigData;
}


void mac_apply_method(PTR_MAC_CONFIG ptrMacConfig) {

    PTR_MAC_DEVICE_INFO ptrMacDeviceInfo = NULL;
    for (int i = 0; i < MAC_DEVICE_INFO_MAX_NUM; ++i) {
        if (macDeviceInfo[i].macApplyMethod != 0) {
            if (ptrMacConfig->macApplyMethod == macDeviceInfo[i].macApplyMethod) {
                ptrMacDeviceInfo = &macDeviceInfo[i];
            }
        } else {
            break;
        }
    }
    if (ptrMacDeviceInfo == NULL) {
        /* 没有找到对应的 Method */
        printf("mac_apply_method-'%d' can't apply!\n", ptrMacConfig->macApplyMethod);
        return;
    }

    PTR_DEVICE_ADD_INFO ptrDeviceAddInfo_mac = NULL;
    for (int i = 0; i < DEVICE_MAX_NUM; ++i) {
        if (deviceAddList[i].exist) {
            /* 设备存在 */
            if (deviceAddList[i].device_type_id == ptrMacDeviceInfo->deviceTypeId) {
                /* device type id 一致 */
                if (deviceAddList[i].ptrI2cDeviceData->ptrDeviceConfig->bus == ptrMacDeviceInfo->i2c_bus) {
                    /* bus 一致 */
                    if (deviceAddList[i].ptrI2cDeviceData->ptrDeviceConfig->addr == ptrMacDeviceInfo->i2c_addr) {
                        /* addr 一致 */
                        ptrDeviceAddInfo_mac = &deviceAddList[i];
                        break;
                    }
                }
            }
        }
    }
    if (ptrDeviceAddInfo_mac == NULL) {
        /* 如果 MAC EEPROM 没有显式添加 ，则无法应用特殊设备的逻辑 */
        return;
    }
    /**************************************** 修改 MAC EEPROM 的值 ****************************************/
    PTR_I2C_EEPROM_sTYPE ptrI2CEepromSType = (PTR_I2C_EEPROM_sTYPE) (ptrDeviceAddInfo_mac->ptrI2cDeviceData->data_buf);

    if (ptrI2CEepromSType->total_size < ptrMacDeviceInfo->mac_value_for_offset[5]) {
        /* 指定设备的总大小 < mac 地址的应用 offset */
        printf("I2c EEPROM size error, can't apply mac address!\n");
        return;
    } else {
        for (int i = 0; i < 6; ++i) {
            /* 往 buf 中写入 mac 值 */
            ptrI2CEepromSType->buf[ptrMacDeviceInfo->mac_value_for_offset[i]] = ptrMacConfig->mac[i];
            printf("%02x  ==>  %02x \n", ptrI2CEepromSType->buf[ptrMacDeviceInfo->mac_value_for_offset[i]], ptrMacConfig->mac[i]);
        }
        /* 应用完成 */
        return;
    }
}

void gpio_signal_init(PTR_GPIO_SIGNAL ptrGpioSignal, uint32_t device_index) {
    while (ptrGpioSignal != NULL && ptrGpioSignal->pre != NULL) {
        ptrGpioSignal = ptrGpioSignal->pre; /* 确保 */
    }
    while (ptrGpioSignal != NULL) {
        if (ptrGpioSignal->gpioSignalType == GPIO_SIGNAL_INPUT) {
            void *send_dev = get_dev_gpio(ptrGpioSignal->gpioSignal.gpioSignalInputLine.send_device_index,
                                          ptrGpioSignal->gpioSignal.gpioSignalInputLine.send_device_name);
            uint32_t send_line = ptrGpioSignal->gpioSignal.gpioSignalInputLine.send_device_line;
            void *recv_dev = get_dev_gpio((int32_t)device_index, NULL);
            uint32_t recv_line = ptrGpioSignal->gpioSignal.gpioSignalInputLine.recv_line;

            connect_gpio_line(send_dev, (int) send_line, recv_dev, (int) recv_line);
        } else if (ptrGpioSignal->gpioSignalType == GPIO_SIGNAL_OUTPUT) {
            void *recv_dev = get_dev_gpio(ptrGpioSignal->gpioSignal.gpioSignalOutputLine.recv_device_index,
                                          ptrGpioSignal->gpioSignal.gpioSignalOutputLine.recv_device_name);
            uint32_t recv_line = ptrGpioSignal->gpioSignal.gpioSignalOutputLine.recv_device_line;
            void *send_dev = get_dev_gpio((int32_t)device_index, NULL);
            uint32_t send_line = ptrGpioSignal->gpioSignal.gpioSignalOutputLine.send_line;

            connect_gpio_line(send_dev, (int) send_line, recv_dev, (int) recv_line);
        }

        /**************************************** next ****************************************/
        if (ptrGpioSignal->next == NULL) {
            break;
        }
        ptrGpioSignal = ptrGpioSignal->next;
    }
}

uint8_t line_has_config_input(int line, PTR_GPIO_SIGNAL ptrGpioSignal){
    uint8_t line_has_input = 0;
    while (ptrGpioSignal != NULL && ptrGpioSignal->pre != NULL) {
        ptrGpioSignal = ptrGpioSignal->pre;
    }

    while (ptrGpioSignal != NULL) {
        if (ptrGpioSignal->gpioSignal.gpioSignalInputLine.recv_line == line) {
            /* 此 input 的 line 有配置  */
            if (ptrGpioSignal->gpioSignalType == GPIO_SIGNAL_INPUT) {
                /* 且配置为 INPUT */
                line_has_input = 1;
            }
        }
        ptrGpioSignal = ptrGpioSignal->next;
    }
    return line_has_input;
}

uint8_t line_has_config_output(int line, PTR_GPIO_SIGNAL ptrGpioSignal){
    uint8_t line_has_output = 0;
    while (ptrGpioSignal != NULL && ptrGpioSignal->pre != NULL) {
        ptrGpioSignal = ptrGpioSignal->pre;
    }

    while (ptrGpioSignal != NULL) {
        if (ptrGpioSignal->gpioSignal.gpioSignalOutputLine.send_line == line) {
            /* 此 input 的 line 有配置  */
            if (ptrGpioSignal->gpioSignalType == GPIO_SIGNAL_OUTPUT) {
                /* 且配置为 INPUT */
                line_has_output = 1;
            }
        }
        ptrGpioSignal = ptrGpioSignal->next;
    }
    return line_has_output;
}

void oscilloscope_init(PTR_PIN_OSCILLOSCOPE ptrPinOscilloscope){
    memset(ptrPinOscilloscope, 0, sizeof(PIN_OSCILLOSCOPE));
    ptrPinOscilloscope->rate = PIN_OSCILLOSCOPE_RATE;
}

uint8_t oscilloscope_add_status_bit(PTR_PIN_OSCILLOSCOPE ptrPinOscilloscope, uint8_t status_bit){
    uint8_t status_changed = FALSE;
    uint8_t before_rear = (ptrPinOscilloscope->rear == 0)
                          ? (PIN_OSCILLOSCOPE_MAX_BITS)
                          : (ptrPinOscilloscope->rear - 1);
    if (ptrPinOscilloscope->capacity < PIN_OSCILLOSCOPE_MAX_BITS) {
        /* 队列未满 */
        ptrPinOscilloscope->status_bit[ptrPinOscilloscope->rear] = status_bit;
        ptrPinOscilloscope->rear = (ptrPinOscilloscope->rear + 1) % (PIN_OSCILLOSCOPE_MAX_BITS + 1);
        ptrPinOscilloscope->capacity += 1;
    } else {
        /* 队列已满，队列需先出一个元素 */
        ptrPinOscilloscope->front = (ptrPinOscilloscope->front + 1) % (PIN_OSCILLOSCOPE_MAX_BITS + 1);
        ptrPinOscilloscope->status_bit[ptrPinOscilloscope->rear] = status_bit;
        ptrPinOscilloscope->rear = (ptrPinOscilloscope->rear + 1) % (PIN_OSCILLOSCOPE_MAX_BITS + 1);
    }

    if (ptrPinOscilloscope->change_length == 0) {
        /* 队列为空 */
        if (status_bit == 0) {
            ptrPinOscilloscope->change_status[ptrPinOscilloscope->change_rear] = PIN_FALL_EDGE;
        } else {
            ptrPinOscilloscope->change_status[ptrPinOscilloscope->change_rear] = PIN_RISING_EDGE;
        }
        ptrPinOscilloscope->change_rear = (ptrPinOscilloscope->change_rear + 1) % (PIN_OSCILLOSCOPE_MAX_BITS);
        ptrPinOscilloscope->change_length += 1;
        status_changed = TRUE;
    } else if (ptrPinOscilloscope->change_length < PIN_OSCILLOSCOPE_MAX_BITS - 1) {
        /* 队列未满 */
        if (status_bit != ptrPinOscilloscope->status_bit[before_rear]) {
            /* 新增的状态不与之前状态相等 */
            status_changed = TRUE;
            if (status_bit == 0) {
                /* 新状态为0，即为电平下降 */
                ptrPinOscilloscope->change_status[ptrPinOscilloscope->change_rear] = PIN_FALL_EDGE;
            } else {
                /* 新状态不为0，记为电平上升 */
                ptrPinOscilloscope->change_status[ptrPinOscilloscope->change_rear] = PIN_RISING_EDGE;
            }
            ptrPinOscilloscope->change_rear = (ptrPinOscilloscope->change_rear + 1) % (PIN_OSCILLOSCOPE_MAX_BITS);
            ptrPinOscilloscope->change_length += 1;
        }
    } else {
        /* 队列已满 */
        if (status_bit != ptrPinOscilloscope->status_bit[before_rear]) {
            /* 需要出一个队列 */
            ptrPinOscilloscope->change_front = (ptrPinOscilloscope->change_front + 1) % PIN_OSCILLOSCOPE_MAX_BITS;
            /* 新增的状态不与之前状态相等 */
            status_changed = TRUE;
            if (status_bit == 0) {
                /* 新状态为0，即为电平下降 */
                ptrPinOscilloscope->change_status[ptrPinOscilloscope->change_rear] = PIN_FALL_EDGE;
            } else {
                /* 新状态不为0，记为电平上升 */
                ptrPinOscilloscope->change_status[ptrPinOscilloscope->change_rear] = PIN_RISING_EDGE;
            }
            ptrPinOscilloscope->change_rear = (ptrPinOscilloscope->change_rear + 1) % (PIN_OSCILLOSCOPE_MAX_BITS);
        }
    }
    return status_changed;
}

void *oscilloscope_monitor_thread(void *ptrPinOscilloscopeMonitorThreadArgs){
    PTR_PIN_OSCILLOSCOPE_MONITOR_THREAD_ARGS ptrPinOscilloscopeMonitorThreadArgs1 = (PTR_PIN_OSCILLOSCOPE_MONITOR_THREAD_ARGS) ptrPinOscilloscopeMonitorThreadArgs;
    PTR_GPIO_DEVICE_DATA ptrGpioDeviceData = ptrPinOscilloscopeMonitorThreadArgs1->ptrGpioDeviceData;
    uint32_t pin_num = ptrPinOscilloscopeMonitorThreadArgs1->pin_num;
    PTR_GPIO_SWITCH_sTYPE ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE)ptrGpioDeviceData->data_buf;
    pthread_mutex_t *pthreadMutex = &(ptrGpioSwitchSType->pin_state_mutex[pin_num]);
    uint32_t sleep_ms = 1000 / ptrGpioSwitchSType->pinOscilloscope_list[pin_num].rate;
    PTR_GPIO_RESPONSE_LOGIC ptrGpioResponseLogic_temp;
    uint32_t change_status_i;
    uint8_t response_condition = FALSE;
    uint8_t status_changed = FALSE;
    uint32_t output_pin_num;
    PTR_PIN_WAVEFORM_GENERATOR ptrPinWaveformGenerator_out = NULL;
    while (1) {
        pthread_mutex_lock(pthreadMutex);
        status_changed = oscilloscope_add_status_bit(&ptrGpioSwitchSType->pinOscilloscope_list[pin_num], ptrGpioDeviceData->pin_state[pin_num]);
        pthread_mutex_unlock(pthreadMutex);
        if (status_changed == TRUE) {
            /* 检查状态序列 */
//            printf("pin[%d] status changed!\n", pin_num);
            ptrGpioResponseLogic_temp = ptrGpioDeviceData->ptrDeviceConfig->ptrGpioResponseLogic;
            response_condition = FALSE;  /* 重置 condition */
            while (ptrGpioResponseLogic_temp != NULL) {
                if (ptrGpioResponseLogic_temp->input_pin_num == pin_num) {
                    /* 解析 status_order 和 change_status */
                    change_status_i = ptrGpioSwitchSType->pinOscilloscope_list[pin_num].change_rear;
                    for (int i = strlen(ptrGpioResponseLogic_temp->input_status_order) - 1; i >= 0; --i) {  /* 从最后一个input order 向前挨个对比 */
                        change_status_i = (change_status_i == 0) ? (PIN_OSCILLOSCOPE_MAX_BITS - 1) : (change_status_i - 1);
//                        printf(">>>>> input_status_order:'%c' ?==? change_status:'%d' <<<<< \n",
//                               ptrGpioResponseLogic_temp->input_status_order[i],
//                               ptrGpioSwitchSType->pinOscilloscope_list[pin_num].change_status[change_status_i]);
                        if (ptrGpioResponseLogic_temp->input_status_order[i] == '1') {
                            /* 1 表示下降沿 */
                            if (ptrGpioSwitchSType->pinOscilloscope_list[pin_num].change_status[change_status_i] != PIN_FALL_EDGE) {
                                /* 状态不一致，退出此比较 */
                                break;
                            }
                        } else if (ptrGpioResponseLogic_temp->input_status_order[i] == '2') {
                            /* 2 表示上升沿 */
                            if (ptrGpioSwitchSType->pinOscilloscope_list[pin_num].change_status[change_status_i] != PIN_RISING_EDGE) {
                                /*  状态不一致，退出 */
                                break;
                            }
                        } else if (ptrGpioResponseLogic_temp->input_status_order[i] == '3') {
                            ;
                        } else {
                            /* 其他 input status order，不认为是一个有效的判断，应 continue 下一个状态判断 */
                            continue;
                        }
                        /* 到此处，意味着此次比较是有效且通过的，需判断是否所有状态都比较完了 */
                        if (i == 0) {
                            /* i 为 0 表示 input_status_order 已遍历完，那就意味着所有 input_status_order 都已满足 */
                            response_condition = TRUE;
                        } else {
                            /* i 不为 0，还需要进入下一轮对比 */
                            if (change_status_i == ptrGpioSwitchSType->pinOscilloscope_list[pin_num].change_front) {
                                /* rear == front, change_status遍历完了 */
//                                printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>> change status over failed! \n");
                                break; /* 没必要继续了 */
                            }
                        }
                    }
                    if (response_condition == TRUE) {
                        break;  /* 已确定输出逻辑 */
                    }
                }
                ptrGpioResponseLogic_temp = ptrGpioResponseLogic_temp->next;

            }
            if (response_condition == TRUE) {
                /* 存在满足条件的响应逻辑，修改输出波形 */
//                printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n response_condition == TRUE !\n");
                output_pin_num = ptrGpioResponseLogic_temp->output_pin_num;
                if (output_pin_num < ptrGpioDeviceData->pin_nums) {
                    ptrPinWaveformGenerator_out = &ptrGpioSwitchSType->pinWaveformGenerator_list[output_pin_num];
                    pthread_mutex_lock(&(ptrGpioSwitchSType->pin_state_mutex[output_pin_num]));

                    ptrPinWaveformGenerator_out->has_update = TRUE;
                    ptrPinWaveformGenerator_out->stop_waveform_generator = FALSE;
                    ptrPinWaveformGenerator_out->waveform_len = 0;
                    if (ptrGpioResponseLogic_temp->output_waveform_type == PIN_WAVEFORM_TYPE_TOGGLE) {
                        ptrPinWaveformGenerator_out->waveform_buf[ptrPinWaveformGenerator_out->waveform_len++] = !ptrGpioDeviceData->pin_state[output_pin_num];
                        ptrPinWaveformGenerator_out->pinWaveformType = PIN_WAVEFORM_TYPE_ONCE;
                    } else {
                        for (int i = 0; i < strlen(ptrGpioResponseLogic_temp->output_waveform); ++i) {
                            if (ptrPinWaveformGenerator_out->waveform_len >= PIN_OSCILLOSCOPE_MAX_BITS-1) {
                                /* 超过可以最大波形设置长度 */
                                break;
                            }
                            if (ptrGpioResponseLogic_temp->output_waveform[i] == '0') {
                                ptrPinWaveformGenerator_out->waveform_buf[ptrPinWaveformGenerator_out->waveform_len++] = FALSE;
                            } else if (ptrGpioResponseLogic_temp->output_waveform[i] == '1') {
                                ptrPinWaveformGenerator_out->waveform_buf[ptrPinWaveformGenerator_out->waveform_len++] = TRUE;
                            }
                        }
                        ptrPinWaveformGenerator_out->pinWaveformType = ptrGpioResponseLogic_temp->output_waveform_type;
                    }
                    /* 已更新完 waveform，清除 change_status 队列 ! */
                    ptrGpioSwitchSType->pinOscilloscope_list[pin_num].change_rear = ptrGpioSwitchSType->pinOscilloscope_list[pin_num].change_front;

                    pthread_mutex_unlock(&(ptrGpioSwitchSType->pin_state_mutex[output_pin_num]));
                }
            }
        }
        usleep(sleep_ms * 1000);
    }

    return NULL;
}


void *waveform_generator_thread(void *ptrWaveformGeneratorThread) {
    PTR_PIN_WAVEFORM_GENERATOR_THREAD_ARGS ptrPinWaveformGeneratorThreadArgs = (PTR_PIN_WAVEFORM_GENERATOR_THREAD_ARGS) ptrWaveformGeneratorThread;
    PTR_GPIO_DEVICE_DATA ptrGpioDeviceData = ptrPinWaveformGeneratorThreadArgs->ptrGpioDeviceData;
    uint32_t pin_num = ptrPinWaveformGeneratorThreadArgs->pin_num;

    PTR_GPIO_SWITCH_sTYPE ptrGpioSwitchSType = (PTR_GPIO_SWITCH_sTYPE) ptrGpioDeviceData->data_buf;
    pthread_mutex_t *pthreadMutex = &(ptrGpioSwitchSType->pin_state_mutex[pin_num]);
    if (ptrGpioDeviceData->pin_type[pin_num] & PIN_TYPE_OUTPUT) {
        /* 可能具有 OUTPUT 功能 */
    } else {
        /* 配置为 INPUT */
        return NULL;
    }
    /**************************************** 先解析初始 output_waveform ****************************************/
    PTR_PIN_WAVEFORM_GENERATOR ptrPinWaveformGenerator = &ptrGpioSwitchSType->pinWaveformGenerator_list[pin_num];
    PTR_DEVICE_CONFIG ptrDeviceConfig = ptrGpioDeviceData->ptrDeviceConfig;
    PTR_GPIO_OUTPUT_LOGIC_INIT ptrGpioOutputLogicInit = ptrDeviceConfig->ptrGpioOutputLogicInit;
    uint8_t *pin_state = &(ptrGpioDeviceData->pin_state[pin_num]);
    while (ptrGpioOutputLogicInit != NULL) {
        if (ptrGpioOutputLogicInit->output_pin_num == pin_num) {
            break;
        } else {
            /* pin_num 不对 */
            ptrGpioOutputLogicInit = ptrGpioOutputLogicInit->next;
        }
    }
    if (ptrGpioOutputLogicInit == NULL) {
        /* 没有配置此 pin 的初始 output */
        ptrPinWaveformGenerator->waveform_len = 0;
        ptrPinWaveformGenerator->stop_waveform_generator = TRUE;
    } else {
        /* 有配置 */
        ptrPinWaveformGenerator->stop_waveform_generator = FALSE;
        ptrPinWaveformGenerator->waveform_len = 0;
        for (int i = 0; i < strlen(ptrGpioOutputLogicInit->output_waveform); ++i) {
            if (ptrGpioOutputLogicInit->output_waveform[i] == '0') {
                ptrPinWaveformGenerator->waveform_buf[ptrPinWaveformGenerator->waveform_len++] = 0;
            } else if (ptrGpioOutputLogicInit->output_waveform[i] == '1') {
                ptrPinWaveformGenerator->waveform_buf[ptrPinWaveformGenerator->waveform_len++] = 1;
            }
        }
        ptrPinWaveformGenerator->pinWaveformType = ptrGpioOutputLogicInit->output_waveform_type;
    }
    ptrPinWaveformGenerator->waveform_rate = PIN_WAVEFORM_RATE;
    uint32_t loop_wait_ms = 0;
    ptrPinWaveformGenerator->has_update = TRUE;
    /**************************************** 死循环持续修改波形 ****************************************/
    while (1) {
        pthread_mutex_lock(pthreadMutex);
        /**************************************** 检查波形产生参数是否有更新 ****************************************/
        if (ptrPinWaveformGenerator->has_update) {
            ptrPinWaveformGenerator->curr_waveform_loc = 0;
            if (ptrPinWaveformGenerator->waveform_rate != 0) {
                loop_wait_ms = 1000 / ptrPinWaveformGenerator->waveform_rate;
            }
            ptrPinWaveformGenerator->has_update = FALSE;
        }
        /**************************************** 检查 stop ****************************************/
        if (ptrPinWaveformGenerator->stop_waveform_generator ||
            ptrPinWaveformGenerator->waveform_rate == 0) {
            pthread_mutex_unlock(pthreadMutex);
            usleep(loop_wait_ms * 1000);
            continue;
        }
        /**************************************** set gpio line ****************************************/
        if (ptrPinWaveformGenerator->waveform_buf[ptrPinWaveformGenerator->curr_waveform_loc] == 0) {
            *pin_state = 0;
            set_gpio_line(ptrGpioDeviceData->pin_buf, pin_num, 0);
        } else {
            *pin_state = 1;
            set_gpio_line(ptrGpioDeviceData->pin_buf, pin_num, 1);
        }
        /**************************************** 检查波形发生位置 ****************************************/
        if (ptrPinWaveformGenerator->curr_waveform_loc == (ptrPinWaveformGenerator->waveform_len - 1)) {
            /* 已产生完 buf 中的波形 */
            if (ptrPinWaveformGenerator->pinWaveformType == PIN_WAVEFORM_TYPE_ONCE) {
                /* 一次性波形产生，不重复产生 */
            } else if (ptrPinWaveformGenerator->pinWaveformType == PIN_WAVEFORM_TYPE_REPEAT) {
                /* 重复产生波形，重置位置，继续产生波形 */
                ptrPinWaveformGenerator->curr_waveform_loc = 0;
            }
        } else {
            /* 波形没有发生完，位置加 1 */
            ptrPinWaveformGenerator->curr_waveform_loc++;
        }

        pthread_mutex_unlock(pthreadMutex);
        usleep(loop_wait_ms * 1000);
    }
}

