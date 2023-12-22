/*
 * Texas Instruments TMP105 temperature sensor.
 *
 * Copyright (C) 2008 Nokia Corporation
 * Written by Andrzej Zaborowski <andrew@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "hw/i2c/i2c.h"
#include "hw/irq.h"
#include "migration/vmstate.h"
#include "hw/sensor/tmp105.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qemu/module.h"
#include "include/hw/qdev-properties.h"
#include "slib/inc/aspeed-init.h"
#include "qemu/main-loop.h"


static void tmp105_interrupt_update(TMP105State *s)
{
    qemu_set_irq(s->pin, s->alarm ^ ((~s->config >> 2) & 1));	/* POL */
}

static void tmp105_alarm_update(TMP105State *s)
{
    if ((s->config >> 0) & 1) {					/* SD */
        if ((s->config >> 7) & 1)				/* OS */
            s->config &= ~(1 << 7);				/* OS */
        else
            return;
    }

    if (s->config >> 1 & 1) {
        /*
         * TM == 1 : Interrupt mode. We signal Alert when the
         * temperature rises above T_high, and expect the guest to clear
         * it (eg by reading a device register).
         */
        if (s->detect_falling) {
            if (s->temperature < s->limit[0]) {
                s->alarm = 1;
                s->detect_falling = false;
            }
        } else {
            if (s->temperature >= s->limit[1]) {
                s->alarm = 1;
                s->detect_falling = true;
            }
        }
    } else {
        /*
         * TM == 0 : Comparator mode. We signal Alert when the temperature
         * rises above T_high, and stop signalling it when the temperature
         * falls below T_low.
         */
        if (s->detect_falling) {
            if (s->temperature < s->limit[0]) {
                s->alarm = 0;
                s->detect_falling = false;
            }
        } else {
            if (s->temperature >= s->limit[1]) {
                s->alarm = 1;
                s->detect_falling = true;
            }
        }
    }

    tmp105_interrupt_update(s);
}

static void tmp105_get_temperature(Object *obj, Visitor *v, const char *name,
                                   void *opaque, Error **errp)
{
    TMP105State *s = TMP105(obj);
    int64_t value = s->temperature * 1000 / 256;

    visit_type_int(v, name, &value, errp);
}

/* Units are 0.001 centigrades relative to 0 C.  s->temperature is 8.8
 * fixed point, so units are 1/256 centigrades.  A simple ratio will do.
 */
static void tmp105_set_temperature(Object *obj, Visitor *v, const char *name,
                                   void *opaque, Error **errp)
{
    TMP105State *s = TMP105(obj);
    int64_t temp;

    if (!visit_type_int(v, name, &temp, errp)) {
        return;
    }
    if (temp >= 128000 || temp < -128000) {
        error_setg(errp, "value %" PRId64 ".%03" PRIu64 " C is out of range",
                   temp / 1000, temp % 1000);
        return;
    }

    s->temperature = (int16_t) (temp * 256 / 1000);

    tmp105_alarm_update(s);
}

static const int tmp105_faultq[4] = { 1, 2, 4, 6 };

static void tmp105_read(TMP105State *s)
{
    s->len = 0;

    if ((s->config >> 1) & 1) {					/* TM */
        s->alarm = 0;
        tmp105_interrupt_update(s);
    }

    switch (s->pointer & 3) {
    case TMP105_REG_TEMPERATURE:
        s->buf[s->len ++] = (((uint16_t) s->temperature) >> 8);
        s->buf[s->len ++] = (((uint16_t) s->temperature) >> 0) &
                (0xf0 << ((~s->config >> 5) & 3));		/* R */
        break;

    case TMP105_REG_CONFIG:
        s->buf[s->len ++] = s->config;
        break;

    case TMP105_REG_T_LOW:
        s->buf[s->len ++] = ((uint16_t) s->limit[0]) >> 8;
        s->buf[s->len ++] = ((uint16_t) s->limit[0]) >> 0;
        break;

    case TMP105_REG_T_HIGH:
        s->buf[s->len ++] = ((uint16_t) s->limit[1]) >> 8;
        s->buf[s->len ++] = ((uint16_t) s->limit[1]) >> 0;
        break;
    }
}

static void tmp105_write(TMP105State *s)
{
    switch (s->pointer & 3) {
    case TMP105_REG_TEMPERATURE:
        break;

    case TMP105_REG_CONFIG:
        if (s->buf[0] & ~s->config & (1 << 0))			/* SD */
            printf("%s: TMP105 shutdown\n", __func__);
        s->config = s->buf[0];
        s->faults = tmp105_faultq[(s->config >> 3) & 3];	/* F */
        tmp105_alarm_update(s);
        break;

    case TMP105_REG_T_LOW:
    case TMP105_REG_T_HIGH:
        if (s->len >= 3)
            s->limit[s->pointer & 1] = (int16_t)
                    ((((uint16_t) s->buf[0]) << 8) | s->buf[1]);
        tmp105_alarm_update(s);
        break;
    }
}

static uint8_t tmp105_rx(I2CSlave *i2c)
{
    TMP105State *s = TMP105(i2c);

    if (s->len < 2) {
        return s->buf[s->len ++];
    } else {
        return 0xff;
    }
}

static int tmp105_tx(I2CSlave *i2c, uint8_t data)
{
    TMP105State *s = TMP105(i2c);

    if (s->len == 0) {
        s->pointer = data;
        s->len++;
    } else {
        if (s->len <= 2) {
            s->buf[s->len - 1] = data;
        }
        s->len++;
        tmp105_write(s);
    }

    return 0;
}

static int tmp105_event(I2CSlave *i2c, enum i2c_event event)
{
    TMP105State *s = TMP105(i2c);

    if (event == I2C_START_RECV) {
        tmp105_read(s);
    }

    s->len = 0;
    return 0;
}

static int tmp105_post_load(void *opaque, int version_id)
{
    TMP105State *s = opaque;

    s->faults = tmp105_faultq[(s->config >> 3) & 3];		/* F */

    tmp105_interrupt_update(s);
    return 0;
}

static bool detect_falling_needed(void *opaque)
{
    TMP105State *s = opaque;

    /*
     * We only need to migrate the detect_falling bool if it's set;
     * for migration from older machines we assume that it is false
     * (ie temperature is not out of range).
     */
    return s->detect_falling;
}

static const VMStateDescription vmstate_tmp105_detect_falling = {
    .name = "TMP105/detect-falling",
    .version_id = 1,
    .minimum_version_id = 1,
    .needed = detect_falling_needed,
    .fields = (VMStateField[]) {
        VMSTATE_BOOL(detect_falling, TMP105State),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_tmp105 = {
    .name = "TMP105",
    .version_id = 0,
    .minimum_version_id = 0,
    .post_load = tmp105_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8(len, TMP105State),
        VMSTATE_UINT8_ARRAY(buf, TMP105State, 2),
        VMSTATE_UINT8(pointer, TMP105State),
        VMSTATE_UINT8(config, TMP105State),
        VMSTATE_INT16(temperature, TMP105State),
        VMSTATE_INT16_ARRAY(limit, TMP105State, 2),
        VMSTATE_UINT8(alarm, TMP105State),
        VMSTATE_I2C_SLAVE(i2c, TMP105State),
        VMSTATE_END_OF_LIST()
    },
    .subsections = (const VMStateDescription*[]) {
        &vmstate_tmp105_detect_falling,
        NULL
    }
};

static void tmp105_reset(I2CSlave *i2c)
{
    TMP105State *s = TMP105(i2c);

    s->temperature = 0;
    s->pointer = 0;
    s->config = 0;
    s->faults = tmp105_faultq[(s->config >> 3) & 3];
    s->alarm = 0;
    s->detect_falling = false;

    s->limit[0] = 0x4b00; /* T_LOW, 75 degrees C */
    s->limit[1] = 0x5000; /* T_HIGH, 80 degrees C */

    tmp105_interrupt_update(s);
}

static void tmp105_realize(DeviceState *dev, Error **errp)
{
    I2CSlave *i2c = I2C_SLAVE(dev);
    TMP105State *s = TMP105(i2c);

    qdev_init_gpio_out(&i2c->qdev, &s->pin, 1);

    tmp105_reset(&s->i2c);
}

static void tmp105_initfn(Object *obj)
{
    object_property_add(obj, "temperature", "int",
                        tmp105_get_temperature,
                        tmp105_set_temperature, NULL, NULL);
}

static void tmp105_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);

    dc->realize = tmp105_realize;
    k->event = tmp105_event;
    k->recv = tmp105_rx;
    k->send = tmp105_tx;
    dc->vmsd = &vmstate_tmp105;
}

static const TypeInfo tmp105_info = {
    .name          = TYPE_TMP105,
    .parent        = TYPE_I2C_SLAVE,
    .instance_size = sizeof(TMP105State),
    .instance_init = tmp105_initfn,
    .class_init    = tmp105_class_init,
};

static void tmp105_register_types(void)
{
    type_register_static(&tmp105_info);
}

type_init(tmp105_register_types)


static void toggle_output(LatchingSwitchState *s, int new_state)
{
    static time_t last_time;
    time_t curr_time;
    time(&curr_time);
    printf("\n\n######################### %d #############################\n\n", new_state);
    if (curr_time - last_time > 5) {
        s->state = !(s->state);
        qemu_set_irq(s->output, s->state);
        last_time = curr_time;
        printf("\n\n######################### out to gpioZ2 : %d #############################\n\n", s->state);
    }
}

static void input_handler(void *opaque, int line, int new_state)
{
    LatchingSwitchState *s = LATCHING_SWITCH(opaque);

    assert(line == 0);

//    if (s->trigger_edge == TRIGGER_EDGE_FALLING && new_state == 0) {
//        toggle_output(s);
//    } else if (s->trigger_edge == TRIGGER_EDGE_RISING && new_state == 1) {
//        toggle_output(s);
//    } else if (s->trigger_edge == TRIGGER_EDGE_BOTH) {
//        toggle_output(s);
//    }
    toggle_output(s, new_state);
}

static void latching_switch_reset(DeviceState *dev)
{
    LatchingSwitchState *s = LATCHING_SWITCH(dev);
    /* reset to off */
    s->state = false;
    /* reset to falling edge trigger */
    s->trigger_edge = TRIGGER_EDGE_FALLING;
}

static const VMStateDescription vmstate_latching_switch = {
        .name = TYPE_LATCHING_SWITCH,
        .version_id = 1,
        .minimum_version_id = 1,
        .fields = (VMStateField[]) {
                VMSTATE_BOOL(state, LatchingSwitchState),
                VMSTATE_UINT8(trigger_edge, LatchingSwitchState),
                VMSTATE_END_OF_LIST()
        }
};

static void latching_switch_realize(DeviceState *dev, Error **errp)
{
    LatchingSwitchState *s = LATCHING_SWITCH(dev);

    /* init a input io */
    qdev_init_gpio_in(dev, input_handler, 1);

    /* init a output io */
    qdev_init_gpio_out(dev, &(s->output), 1);
}

static void latching_switch_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "Latching Switch";
    dc->vmsd = &vmstate_latching_switch;
    dc->reset = latching_switch_reset;
    dc->realize = latching_switch_realize;
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static void latching_switch_get_state(Object *obj, Visitor *v, const char *name,
                                      void *opaque, Error **errp)
{
    LatchingSwitchState *s = LATCHING_SWITCH(obj);
    bool value = s->state;

    visit_type_bool(v, name, &value, errp);
}

static void latching_switch_set_state(Object *obj, Visitor *v, const char *name,
                                      void *opaque, Error **errp)
{
    LatchingSwitchState *s = LATCHING_SWITCH(obj);
    bool value;
    Error *err = NULL;

    visit_type_bool(v, name, &value, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }

    if (value != s->state) {
//        toggle_output(s);
    }
}
static void latching_switch_get_trigger_edge(Object *obj, Visitor *v,
                                             const char *name, void *opaque,
                                             Error **errp)
{
    LatchingSwitchState *s = LATCHING_SWITCH(obj);
    int value = s->trigger_edge;
    char *p = NULL;

    if (value == TRIGGER_EDGE_FALLING) {
        p = g_strdup("falling");
        visit_type_str(v, name, &p, errp);
    } else if (value == TRIGGER_EDGE_RISING) {
        p = g_strdup("rising");
        visit_type_str(v, name, &p, errp);
    } else if (value == TRIGGER_EDGE_BOTH) {
        p = g_strdup("both");
        visit_type_str(v, name, &p, errp);
    } else {
        error_setg(errp, "Invalid trigger edge value");
    }
    g_free(p);
}

static void latching_switch_set_trigger_edge(Object *obj, Visitor *v,
                                             const char *name, void *opaque,
                                             Error **errp)
{
    LatchingSwitchState *s = LATCHING_SWITCH(obj);
    char *value;
    Error *err = NULL;

    visit_type_str(v, name, &value, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }

    if (strcmp(value, "falling") == 0) {
        s->trigger_edge = TRIGGER_EDGE_FALLING;
    } else if (strcmp(value, "rising") == 0) {
        s->trigger_edge = TRIGGER_EDGE_RISING;
    } else if (strcmp(value, "both") == 0) {
        s->trigger_edge = TRIGGER_EDGE_BOTH;
    } else {
        error_setg(errp, "Invalid trigger edge type: %s", value);
    }
}

static void latching_switch_init(Object *obj)
{
    LatchingSwitchState *s = LATCHING_SWITCH(obj);

    s->state = false;
    s->trigger_edge = TRIGGER_EDGE_FALLING;

    object_property_add(obj, "state", "bool",
                        latching_switch_get_state,
                        latching_switch_set_state,
                        NULL, NULL);
    object_property_add(obj, "trigger-edge", "string",
                        latching_switch_get_trigger_edge,
                        latching_switch_set_trigger_edge,
                        NULL, NULL);
}

static const TypeInfo latching_switch_info = {
        .name = TYPE_LATCHING_SWITCH,
        .parent = TYPE_DEVICE,
        .instance_size = sizeof(LatchingSwitchState),
        .class_init = latching_switch_class_init,
        .instance_init = latching_switch_init,
};

static void latching_switch_register_types(void)
{
    type_register_static(&latching_switch_info);
}

type_init(latching_switch_register_types);

LatchingSwitchState *latching_switch_create_simple(Object *parentobj,
                                                   bool state,
                                                   uint8_t trigger_edge)
{
    static const char *name = "latching-switch";
    DeviceState *dev;

    dev = qdev_new(TYPE_LATCHING_SWITCH);

    qdev_prop_set_bit(dev, "state", state);

    if (trigger_edge == TRIGGER_EDGE_FALLING) {
        qdev_prop_set_string(dev, "trigger-edge", "falling");
    } else if (trigger_edge == TRIGGER_EDGE_RISING) {
        qdev_prop_set_string(dev, "trigger-edge", "rising");
    } else if (trigger_edge == TRIGGER_EDGE_BOTH) {
        qdev_prop_set_string(dev, "trigger-edge", "both");
    } else {
        perror("Invalid trigger edge value");
        exit(1);
    }

    object_property_add_child(parentobj, name, OBJECT(dev));
    qdev_realize_and_unref(dev, NULL, &error_fatal);

    return LATCHING_SWITCH(dev);
}

/**************************************** GPIO ****************************************/


/**************************************** GPIO 通用函数 ****************************************/
void connect_gpio_line_for_slib(void *send_dev, int send_line, void *recv_dev, int recv_line) {
//    printf("connect gpio out send_line: %d -> recv_line: %d \n", send_line, recv_line);
    qdev_connect_gpio_out(DEVICE(send_dev), send_line, qdev_get_gpio_in(DEVICE(recv_dev), recv_line));
}

void set_gpio_line_for_slib(void *pin_buf, uint32_t pin, int level) {
//    printf("set irq: pin - %d ==> level: %d  \n", pin, level);
    qemu_irq *irqs = (qemu_irq *)pin_buf;
    if(qemu_mutex_iothread_locked()) {
        qemu_set_irq(irqs[pin], level);
    } else {
        qemu_mutex_lock_iothread();
        qemu_set_irq(irqs[pin], level);
        qemu_mutex_unlock_iothread();
    }
}


/**************************************** 定义设备结构体 ****************************************/
#define EMPTY_GPIO_DECLARE_TYPE(InstanceType, TYPE_NAME) \
    struct InstanceType {                          \
        DeviceState parent_obj;                    \
        GPIO_DEVICE_DATA gpio_device_data;           \
    };                                             \
    typedef struct InstanceType InstanceType;      \
    DECLARE_INSTANCE_CHECKER(InstanceType, TYPE_NAME, TYPE_##TYPE_NAME)

#define EMPTY_GPIO_CREATE_VMStateDescription(InstanceType, TYPE_NAME) \
    static const VMStateDescription vmstate_##InstanceType = {        \
        .name = TYPE_##TYPE_NAME,                                     \
        .version_id = 1,                                              \
        .minimum_version_id = 1,                                      \
        .fields = (VMStateField[]) {                                  \
            VMSTATE_END_OF_LIST()                                     \
        }                                                             \
    };

#define EMPTY_GPIO_CREATE_PIN_HANDLER(InstanceType, TYPE_NAME) \
    static void InstanceType##_input_handler(void *opaque, int line, int new_state) { \
        InstanceType *it = TYPE_NAME(opaque);                  \
        PTR_GPIO_DEVICE_DATA ptrGpioDeviceData = &(it->gpio_device_data); \
        input_handler_##InstanceType(line, new_state, ptrGpioDeviceData);             \
    }

#define EMPTY_GPIO_CREATE_RESET_FUNC(InstanceType, TYPE_NAME) \
    static void InstanceType##_reset(DeviceState *dev) {      \
        InstanceType *it = TYPE_NAME(dev);                    \
        PTR_GPIO_DEVICE_DATA ptrGpioDeviceData = &(it->gpio_device_data); \
        init_##InstanceType(ptrGpioDeviceData);                   \
    };

#define EMPTY_GPIO_CREATE_REALIZE_FUNC(InstanceType, TYPE_NAME) \
    static void InstanceType##_realize(DeviceState *dev, Error **errp){ \
        FUNC_DEBUG("InstanceType##_realize()")                \
        InstanceType *it = TYPE_NAME(dev);                    \
        it->gpio_device_data.pin_nums = it->gpio_device_data.ptrDeviceConfig->pin_nums;      \
        it->gpio_device_data.pin_buf = malloc(sizeof(qemu_irq) * it->gpio_device_data.pin_nums); \
        it->gpio_device_data.pin_state = (uint8_t *)malloc(sizeof(uint8_t) * it->gpio_device_data.pin_nums); \
        it->gpio_device_data.pin_type = (uint8_t *)malloc(sizeof(uint8_t) * it->gpio_device_data.pin_nums); \
        it->gpio_device_data.inputHandler = input_handler_##InstanceType; \
        it->gpio_device_data.outputHandler = output_handler_##InstanceType; \
        qdev_init_gpio_in(dev, InstanceType##_input_handler, it->gpio_device_data.pin_nums); \
        qdev_init_gpio_out(dev, (qemu_irq *)(it->gpio_device_data.pin_buf), it->gpio_device_data.pin_nums); \
        device_add(getGpioDeviceTypeId(InstanceType##_add), it->gpio_device_data.ptrDeviceConfig->name, &(TYPE_NAME(dev)->gpio_device_data), dev); \
        InstanceType##_reset(dev);                              \
    };

#define EMPTY_GPIO_CREATE_CLASS_INIT(InstanceType, TYPE_NAME) \
    static void InstanceType##_class_init(ObjectClass *klass, void *data) { \
        DeviceClass *dc = DEVICE_CLASS(klass);                \
        dc->vmsd = &vmstate_##InstanceType;                   \
        dc->reset = InstanceType##_reset;                     \
        dc->realize = InstanceType##_realize;                  \
    };

#define EMPTY_GPIO_CREATE_TYPEINFO(InstanceType, TYPE_NAME) \
    static const TypeInfo InstanceType##_info = {                     \
        .name = TYPE_##TYPE_NAME,                           \
        .parent = TYPE_DEVICE,                              \
        .instance_size = sizeof(InstanceType),              \
        .class_init = InstanceType##_class_init,           \
    };

#define EMPTY_GPIO_REGISTER_TYPE(InstanceType, TYPE_NAME) \
    static void InstanceType##_register(void){            \
        type_register_static(&InstanceType##_info);       \
    };                                                    \
    type_init(InstanceType##_register);

#define EMPTY_GPIO_DEVICE_ADD_FUNC(InstanceType, TYPE_NAME) \
    void *InstanceType##_add(void *parent_obj, PTR_DEVICE_CONFIG ptrDeviceConfig) { \
        FUNC_DEBUG("InstanceType##_add()")                                                    \
        static uint32_t device_add_index = 0;               \
        char child_name[30] = {0};                                \
        sprintf(child_name, "%s-%d", TYPE_##TYPE_NAME, device_add_index++); \
        DeviceState *it;                                   \
        it = qdev_new(TYPE_##TYPE_NAME);                    \
        TYPE_NAME(it)->gpio_device_data.ptrDeviceConfig = ptrDeviceConfig;                   \
        object_property_add_child(parent_obj, child_name, OBJECT(it));  \
        qdev_realize_and_unref(it, NULL, &error_fatal);     \
        return TYPE_NAME(it);                               \
    };


#define INIT_GPIO_FUNC(InstanceType, TYPE_NAME) \
    EMPTY_GPIO_DECLARE_TYPE(InstanceType, TYPE_NAME) \
    EMPTY_GPIO_CREATE_VMStateDescription(InstanceType, TYPE_NAME) \
    EMPTY_GPIO_CREATE_PIN_HANDLER(InstanceType, TYPE_NAME)        \
    EMPTY_GPIO_CREATE_RESET_FUNC(InstanceType, TYPE_NAME)         \
    EMPTY_GPIO_CREATE_REALIZE_FUNC(InstanceType, TYPE_NAME)       \
    EMPTY_GPIO_CREATE_CLASS_INIT(InstanceType, TYPE_NAME)         \
    EMPTY_GPIO_CREATE_TYPEINFO(InstanceType, TYPE_NAME)           \
    EMPTY_GPIO_REGISTER_TYPE(InstanceType, TYPE_NAME)             \
    EMPTY_GPIO_DEVICE_ADD_FUNC(InstanceType, TYPE_NAME)

#define TYPE_GPIO_EMPTY_0 "gpio-empty-0"
INIT_GPIO_FUNC(GPIOEmptyDevice0, GPIO_EMPTY_0)

#define TYPE_GPIO_EMPTY_1 "gpio-empty-1"
INIT_GPIO_FUNC(GPIOEmptyDevice1, GPIO_EMPTY_1)

#define TYPE_GPIO_EMPTY_2 "gpio-empty-2"
INIT_GPIO_FUNC(GPIOEmptyDevice2, GPIO_EMPTY_2)

#define TYPE_GPIO_EMPTY_3 "gpio-empty-3"
INIT_GPIO_FUNC(GPIOEmptyDevice3, GPIO_EMPTY_3)

#define TYPE_GPIO_EMPTY_4 "gpio-empty-4"
INIT_GPIO_FUNC(GPIOEmptyDevice4, GPIO_EMPTY_4)

#define TYPE_GPIO_EMPTY_5 "gpio-empty-5"
INIT_GPIO_FUNC(GPIOEmptyDevice5, GPIO_EMPTY_5)

#define TYPE_GPIO_EMPTY_6 "gpio-empty-6"
INIT_GPIO_FUNC(GPIOEmptyDevice6, GPIO_EMPTY_6)

#define TYPE_GPIO_EMPTY_7 "gpio-empty-7"
INIT_GPIO_FUNC(GPIOEmptyDevice7, GPIO_EMPTY_7)

#define TYPE_GPIO_EMPTY_8 "gpio-empty-8"
INIT_GPIO_FUNC(GPIOEmptyDevice8, GPIO_EMPTY_8)

#define TYPE_GPIO_EMPTY_9 "gpio-empty-9"
INIT_GPIO_FUNC(GPIOEmptyDevice9, GPIO_EMPTY_9)

static GpioFunctionPtr GpioFunction_list[] = {
        GPIOEmptyDevice0_add,
        GPIOEmptyDevice1_add,
        GPIOEmptyDevice2_add,
        GPIOEmptyDevice3_add,
        GPIOEmptyDevice4_add,
        GPIOEmptyDevice5_add,
        GPIOEmptyDevice6_add,
        GPIOEmptyDevice7_add,
        GPIOEmptyDevice8_add,
        GPIOEmptyDevice9_add
};


GpioFunctionPtr getGpioDeviceAddFunc(int device_type_id) {
    if ((device_type_id < GPIO_DEVICE_TYPE_ID_OFFSET) ||
        (device_type_id - GPIO_DEVICE_TYPE_ID_OFFSET >= GPIO_DEVICE_TOTAL_NUM)) {
        /* 超出 ID 范围 */
        return NULL;
    } else {
        return GpioFunction_list[device_type_id - GPIO_DEVICE_TYPE_ID_OFFSET];
    }
}

int getGpioDeviceTypeId(GpioFunctionPtr gpioFuntionPtr){
    for (int i = 0; i < GPIO_DEVICE_TOTAL_NUM; ++i) {
        if (GpioFunction_list[i] == gpioFuntionPtr) {
            return GPIO_DEVICE_TYPE_ID_OFFSET + i;
        }
    }
    /* 遍历所有也没找到 */
    return -1;
}

