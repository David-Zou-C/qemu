//
// Created by 25231 on 2023/8/17.
//

#ifndef QEMU_DATATHREAD_H
#define QEMU_DATATHREAD_H

#include <pthread.h>
#include "aspeed-init.h"

extern pthread_mutex_t pthreadMutex_sendData;
void data_thread_init(void *pVoid);

extern int (*power_callback)(const char *gpio_name, uint8_t level);

#endif //QEMU_DATATHREAD_H

