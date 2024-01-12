#include <stdio.h>  
#include <cJSON.h>
#include <stdlib.h>
  
int main() {  
    // 创建一个新的JSON对象  
    cJSON *root = cJSON_CreateObject();  
  
    // 添加qemu_id项  
    cJSON_AddStringToObject(root, "qemu_id", "SG40_TEST");  
  
    // 添加devices数组  
    cJSON *devices = cJSON_CreateArray();  
    for (int i = 0; i < 3; i++) {  
        cJSON *device = cJSON_CreateObject();  
        cJSON_AddNumberToObject(device, "index", i + 1);  
        cJSON_AddNumberToObject(device, "device_type_id", 2);  
        cJSON_AddStringToObject(device, "name", "CPU0_Temp_Die" + i);  
        cJSON_AddItemToArray(devices, device);  
    }  
    cJSON_AddItemToObject(root, "devices", devices);
  
    // 添加nums项  
    cJSON_AddNumberToObject(root, "nums", 3);  
  
    // 将JSON对象转换为字符串并打印出来  
    char *jsonString = cJSON_PrintUnformatted(root);

    printf("%s\n", jsonString);  
  
    // 释放内存  
    cJSON_Delete(root);  
    free(jsonString);  
  
    return 0;  
}