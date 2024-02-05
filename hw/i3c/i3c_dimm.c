/*
 * QEMU SMBus EEPROM device
 *
 * Copyright (c) 2007 Arastra, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "slib/inc/i3c-device.h"
#include "qom/object.h"
#include "slib/inc/smbus-device.h"
#include "hw/i3c/i3c.h"
#include "hw/i2c/i2c.h"

/**************************************** 自定义 SMBus 空壳设备 ****************************************/
#define OBJECT_DECLARE_I3C_EMPTY_TYPE(InstanceType, OBJ_NAME) \
    typedef struct InstanceType InstanceType;           \
    \
    G_DEFINE_AUTOPTR_CLEANUP_FUNC(InstanceType, object_unref) \
    \
    DECLARE_INSTANCE_CHECKER(InstanceType, OBJ_NAME, TYPE_ ## OBJ_NAME)

#define INSTANCE_I3C_STRUCT(InstanceType) \
    struct InstanceType {             \
        I3CTarget parent_obj;          \
        I3C_DEVICE_DATA i3cDeviceData;\
    };

#define EMPTY_I3C_DEVICE_EVENT_FUNC(InstanceType, OBJ_NAME) \
    static int InstanceType##_event(I3CTarget *s, enum I3CEvent event) {   \
        InstanceType *it = OBJ_NAME(s);                 \
        PTR_I3C_DEVICE_DATA ptrI3cDeviceData = &(it->i3cDeviceData);     \
        event_##InstanceType(event, ptrI3cDeviceData);  \
        return 0;                                       \
    };

#define EMPTY_I3C_DEVICE_RECV_FUNC(InstanceType, OBJ_NAME) \
    static uint32_t InstanceType##_recv(I3CTarget *s, uint8_t *data, uint32_t num_to_read) {  \
        InstanceType *it = OBJ_NAME(s);                \
        PTR_I3C_DEVICE_DATA ptrI3cDeviceData = &(it->i3cDeviceData); \
        return recv_##InstanceType(ptrI3cDeviceData);  \
    };

#define EMPTY_I3C_DEVICE_SEND_FUNC(InstanceType, OBJ_NAME) \
    static int InstanceType##_send(I3CTarget *s, const uint8_t *data, uint32_t num_to_send, uint32_t *num_sent) { \
        InstanceType *it = OBJ_NAME(s);                \
        PTR_I3C_DEVICE_DATA ptrI3cDeviceData = &(it->i3cDeviceData); \
        send_##InstanceType(data, ptrI3cDeviceData);   \
        return 0;                                      \
    };

#define EMPTY_I3C_DEVICE_CCC_READ_FUNC(InstanceType, OBJ_NAME) \
    static int InstanceType##_ccc_read(I3CTarget *s, uint8_t *data, uint32_t num_to_read, uint32_t *num_read) {  \
        InstanceType *it = OBJ_NAME(s);                \
        PTR_I3C_DEVICE_DATA ptrI3cDeviceData = &(it->i3cDeviceData); \
        return ccc_read_##InstanceType(ptrI3cDeviceData);  \
    };

#define EMPTY_I3C_DEVICE_CCC_WRITE_FUNC(InstanceType, OBJ_NAME) \
    static int InstanceType##_ccc_write(I3CTarget *s, const uint8_t *data, uint32_t num_to_send, uint32_t *num_sent) {  \
        InstanceType *it = OBJ_NAME(s);                \
        PTR_I3C_DEVICE_DATA ptrI3cDeviceData = &(it->i3cDeviceData); \
        return ccc_write_##InstanceType(data, ptrI3cDeviceData);  \
    };

#define EMPTY_I3C_DEVICE_RESET_FUNC(InstanceType, OBJ_NAME) \
    static void InstanceType##_reset(DeviceState *dev) {    \
        InstanceType *it = OBJ_NAME(dev);                \
        PTR_I3C_DEVICE_DATA ptrI3cDeviceData = &(it->i3cDeviceData); \
        init_##InstanceType(ptrI3cDeviceData);              \
    };

#define EMPTY_I3C_DEVICE_REALIZE_FUNC(InstanceType, OBJ_NAME) \
    static void InstanceType##_realize(DeviceState *dev, Error **errp){ \
        InstanceType *it = OBJ_NAME(dev);                \
        PTR_I3C_DEVICE_DATA ptrI3cDeviceData = &(it->i3cDeviceData); \
        device_add(getI3cDeviceTypeId(InstanceType##_add), ptrI3cDeviceData->ptrDeviceConfig->name, &(OBJ_NAME(dev)->i3cDeviceData), dev); \
        InstanceType##_reset(dev);                       \
    };


#define EMPTY_I3C_DEVICE_PROPERTY(InstanceType, OBJ_NAME) \
    static Property InstanceType##_props[] = {            \
        DEFINE_PROP_END_OF_LIST()                         \
    };


#define EMPTY_I3C_DEVICE_CLASS_INIT(InstanceType, OBJ_NAME) \
    static void InstanceType##_class_init(ObjectClass *klass, void *data) { \
        DeviceClass *dc = DEVICE_CLASS(klass);              \
        I3CTargetClass *k = I3C_TARGET_CLASS(klass);          \
        dc->realize = &InstanceType##_realize;              \
        k->event = &InstanceType##_event;                   \
        k->recv = &InstanceType##_recv;                      \
        k->send = &InstanceType##_send;                     \
        k->handle_ccc_read = &InstanceType##_ccc_read;                     \
        k->handle_ccc_write = &InstanceType##_ccc_write;                     \
        device_class_set_props(dc, InstanceType##_props);   \
        dc->reset = InstanceType##_reset;                   \
    };
#define EMPTY_I3C_DEVICE_TYPEINFO(InstanceType, OBJ_NAME) \
    static const TypeInfo InstanceType##_type = {         \
        .name = TYPE_##OBJ_NAME,                          \
        .parent = TYPE_I3C_TARGET,                         \
        .instance_size = sizeof(InstanceType),            \
        .class_size = sizeof(I3CTargetClass),              \
        .class_init = InstanceType##_class_init,          \
    };

#define EMPTY_I3C_DEVICE_REGISTER_INIT(InstanceType, OBJ_NAME) \
    static void InstanceType##_register(void)             \
    {                                                   \
        type_register_static(&InstanceType##_type);       \
    };                                                  \
    type_init(InstanceType##_register);

#define EMPTY_I3C_DEVICE_ADD_FUNC(InstanceType, OBJ_NAME) \
    void InstanceType##_add(void *bus, PTR_DEVICE_CONFIG ptrDeviceConfig) {  \
        InstanceType *it;                                               \
        I3CTarget *ptr;                                                \
        ptr = i3c_target_new(TYPE_##OBJ_NAME, ptrDeviceConfig->addr, 0 ,0, 0);      \
        it = OBJ_NAME(ptr);        \
        it->i3cDeviceData.ptrDeviceConfig = ptrDeviceConfig;           \
        i3c_target_realize_and_unref(ptr, bus, &error_abort);  \
    };

#define INIT_I3C_FUNC(InstanceType, OBJ_NAME) \
    OBJECT_DECLARE_I3C_EMPTY_TYPE(InstanceType, OBJ_NAME) \
    INSTANCE_I3C_STRUCT(InstanceType)              \
    EMPTY_I3C_DEVICE_EVENT_FUNC(InstanceType, OBJ_NAME) \
    EMPTY_I3C_DEVICE_RECV_FUNC(InstanceType, OBJ_NAME)  \
    EMPTY_I3C_DEVICE_SEND_FUNC(InstanceType, OBJ_NAME)  \
    EMPTY_I3C_DEVICE_CCC_READ_FUNC(InstanceType, OBJ_NAME)  \
    EMPTY_I3C_DEVICE_CCC_WRITE_FUNC(InstanceType, OBJ_NAME)  \
    EMPTY_I3C_DEVICE_RESET_FUNC(InstanceType, OBJ_NAME) \
    EMPTY_I3C_DEVICE_REALIZE_FUNC(InstanceType, OBJ_NAME) \
    EMPTY_I3C_DEVICE_PROPERTY(InstanceType, OBJ_NAME)   \
    EMPTY_I3C_DEVICE_CLASS_INIT(InstanceType, OBJ_NAME) \
    EMPTY_I3C_DEVICE_TYPEINFO(InstanceType, OBJ_NAME)   \
    EMPTY_I3C_DEVICE_REGISTER_INIT(InstanceType, OBJ_NAME)\
    EMPTY_I3C_DEVICE_ADD_FUNC(InstanceType, OBJ_NAME)

#define TYPE_I3C_EMPTY_0 "i3c-empty-0"
INIT_I3C_FUNC(I3cEmptyDevice0, I3C_EMPTY_0)

#define TYPE_I3C_EMPTY_1 "i3c-empty-1"
INIT_I3C_FUNC(I3cEmptyDevice1, I3C_EMPTY_1)

#define TYPE_I3C_EMPTY_2 "i3c-empty-2"
INIT_I3C_FUNC(I3cEmptyDevice2, I3C_EMPTY_2)

#define TYPE_I3C_EMPTY_3 "i3c-empty-3"
INIT_I3C_FUNC(I3cEmptyDevice3, I3C_EMPTY_3)

I3cFunctionPtr getI3cDeviceAddFunc(int device_type_id){
    static I3cFunctionPtr i3CFunctionPtr_static[] = {
        I3cEmptyDevice0_add,
        I3cEmptyDevice1_add,
        I3cEmptyDevice2_add,
        I3cEmptyDevice3_add
    };
    if ((device_type_id < I3C_DEVICE_TYPE_ID_OFFSET) || (device_type_id - I3C_DEVICE_TYPE_ID_OFFSET >= I3C_DEVICE_TOTAL_NUM)) {
        return NULL;
    } else {
        return i3CFunctionPtr_static[device_type_id - I3C_DEVICE_TYPE_ID_OFFSET];
    }
};

int getI3cDeviceTypeId(I3cFunctionPtr functionPtr){
    static I3cFunctionPtr i3CFunctionPtr_static[] = {
            I3cEmptyDevice0_add,
            I3cEmptyDevice1_add,
            I3cEmptyDevice2_add,
            I3cEmptyDevice3_add
    };
    for (int i = 0; i < I3C_DEVICE_TOTAL_NUM; ++i) {
        if (i3CFunctionPtr_static[i] == functionPtr) {
            return I3C_DEVICE_TYPE_ID_OFFSET + i;
        }
    }
    return -1;
}
