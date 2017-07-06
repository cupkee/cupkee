/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016-2017 Lixing Ding <ding.lixing@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <cupkee.h>

#include "cupkee_shell_misc.h"
#include "cupkee_shell_device.h"

typedef union device_handle_set_t {
    intptr_t param;
    uint8_t  handles[DEVICE_EVENT_MAX];
} device_handle_set_t;

static int device_is_true(intptr_t ptr);
static void device_op_prop(void *env, intptr_t id, val_t *name, val_t *prop);
static void device_op_elem(void *env, intptr_t id, val_t *which, val_t *elem);
static val_t *device_op_elem_ref(void *env, intptr_t id, val_t *key);
static void device_elem_op_set(void *env, intptr_t id, val_t *val, val_t *res);

static const char *category_names[3] = {
    "MAP", "STREAM", "BLOCK"
};

static const char *device_event_names[] = {
    "error", "data", "drain", "ready"
};

static const val_foreign_op_t device_op = {
    .is_true = device_is_true,
    .prop = device_op_prop,
    .elem = device_op_elem,
    .elem_ref = device_op_elem_ref,
};

static const val_foreign_op_t device_elem_op = {
    .set = device_elem_op_set
};

static const char *device_category_name(uint8_t category)
{
    if (category < DEVICE_CATEGORY_MAX) {
        return category_names[category];
    } else {
        return "?";
    }
}

static void device_list(void)
{
    const cupkee_device_desc_t *desc;
    int i = 0;

    console_log_sync("\r\n%8s%6s%6s%6s:%s\r\n", "DEVICE", "CONF", "INST", "TYPE", "CATEGORY");
    while ((desc = cupkee_device_query_by_index(i++)) != NULL) {
        console_log_sync("%8s%6d%6d%6d:%s\r\n",
                desc->name,
                desc->conf_num,
                hw_device_instances(desc->type),
                desc->type,
                device_category_name(desc->category));
    }
}

static int device_is_true(intptr_t ptr)
{
    (void) ptr;

    return 0;
}

static void device_elem_op_set(void *env, intptr_t id, val_t *val, val_t *res)
{
    cupkee_device_t *dev;
    int index = cupkee_device_elem_index(id, &dev);

    (void) env;

    if (dev && val_is_number(val)) {
        if (0 < cupkee_device_set(dev, index, val_2_integer(val))) {
            *res = *val;
            return;
        }
    }
    *res = VAL_UNDEFINED;
}

static void device_op_prop(void *env, intptr_t id, val_t *name, val_t *prop)
{
    cupkee_device_t *dev = cupkee_device_block(id);
    const char *prop_name = val_2_cstring(name);

    (void) env;

    if (dev && prop_name) {
        if (!strcmp(prop_name, "read")) {
            val_set_native(prop, (intptr_t)native_device_read);
            return;
        } else
        if (!strcmp(prop_name, "write")) {
            val_set_native(prop, (intptr_t)native_device_write);
            return;
        } else
        if (!strcmp(prop_name, "get")) {
            val_set_native(prop, (intptr_t)native_device_get);
            return;
        } else
        if (!strcmp(prop_name, "set")) {
            val_set_native(prop, (intptr_t)native_device_set);
            return;
        } else
        if (!strcmp(prop_name, "config")) {
            val_set_native(prop, (intptr_t)native_device_config);
            return;
        } else
        if (!strcmp(prop_name, "enable")) {
            val_set_native(prop, (intptr_t)native_device_enable);
            return;
        } else
        if (!strcmp(prop_name, "disable")) {
            val_set_native(prop, (intptr_t)native_device_disable);
            return;
        } else
        if (!strcmp(prop_name, "listen")) {
            val_set_native(prop, (intptr_t)native_device_listen);
            return;
        } else
        if (!strcmp(prop_name, "ignore")) {
            val_set_native(prop, (intptr_t)native_device_ignore);
            return;
        } else
        if (!strcmp(prop_name, "isEnabled")) {
            val_set_native(prop, (intptr_t)native_device_is_enabled);
            return;
        } else
        if (!strcmp(prop_name, "destroy")) {
            val_set_native(prop, (intptr_t)native_device_destroy);
            return;
        } else
        if (!strcmp(prop_name, "error")) {
            val_set_number(prop, dev->error);
            return;
        } else
        if (!strcmp(prop_name, "instance")) {
            val_set_number(prop, dev->instance);
            return;
        } else
        if (!strcmp(prop_name, "type")) {
            val_set_foreign_string(prop, (intptr_t) dev->desc->name);
            return;
        }
    }
    val_set_undefined(prop);
}

static void device_op_elem(void *env, intptr_t id, val_t *which, val_t *elem)
{
    if (val_is_number(which)) {
        cupkee_device_t *dev = cupkee_device_block(id);
        uint32_t val;

        if (dev && 0 < cupkee_device_get(dev, val_2_integer(which), &val)) {
            val_set_number(elem, val);
        } else {
            val_set_undefined(elem);
        }
    } else {
        device_op_prop(env, id, which, elem);
    }
}

static val_t *device_op_elem_ref(void *env, intptr_t id, val_t *key)
{
    cupkee_device_t *dev = cupkee_device_block(id);
    int index;

    if (dev && val_is_number(key)) {
        index = val_2_integer(key);
    } else {
        return NULL;
    }

    *key = val_create(env, &device_elem_op, cupkee_device_elem_id(dev, index));

    return key;
}

static void device_get_all(cupkee_device_t *dev, env_t *env, val_t *result)
{
    int status;
    uint32_t data;

    status = cupkee_device_get(dev, -1, &data);
    if (status > 0) {
        // support combine read
        val_set_number(result, data);
        return;
    } else
    if (status == 0 && dev->driver->size){
        int i, size;
        array_t *list;

        size = dev->driver->size(dev->instance);
        list = _array_create(env, size);
        if (list) {
            for (i = 0; i < size; i++) {
                if (cupkee_device_get(dev, i, &data) > 0) {
                    val_set_number(_array_elem(list, i), data);
                } else {
                    val_set_undefined(_array_elem(list, i));
                }
            }

            val_set_array(result, (intptr_t) list);
            return;
        }
    }

    val_set_undefined(result);
}

static void device_get_elem(cupkee_device_t *dev, env_t *env, int offset, val_t *res)
{
    uint32_t data;

    if (offset >= 0) {
        if (cupkee_device_get(dev, offset, &data) > 0) {
            val_set_number(res, data);
        } else {
            *res = VAL_UNDEFINED;
        }
    } else {
        device_get_all(dev, env, res);
    }
}

static int device_read_2_buffer(cupkee_device_t *dev, env_t *env, int n, val_t *buf)
{
    type_buffer_t *b;
    int err;

    b = buffer_create(env, n);
    if (!b) {
        err = -CUPKEE_ERESOURCE;
    } else {
        err = cupkee_device_read(dev, n, b->buf);
    }

    if (err >= 0) {
        val_set_buffer(buf, b);
    }

    return err;
}

static int device_event_handle_set(cupkee_device_t *dev, int event, val_t *cb)
{
    device_handle_set_t *set = (device_handle_set_t *)&dev->handle_param;
    uint8_t ref_id;

    ref_id = set->handles[event];
    if (ref_id) {
        val_t *ref = shell_reference_ptr(ref_id);
        if (!ref) {
            return -CUPKEE_ERROR;
        }
        *ref = *cb;
    } else {
        val_t *ref = shell_reference_create(cb);
        if (!ref) {
            return -CUPKEE_ERESOURCE;
        }
        set->handles[event] = shell_reference_id(ref);
    }

    return CUPKEE_OK;
}

static val_t *device_event_handle_get(cupkee_device_t *dev, int event)
{
    device_handle_set_t *set = (device_handle_set_t *)&dev->handle_param;

    return shell_reference_ptr(set->handles[event]);
}

static void device_event_handle_release(cupkee_device_t *dev)
{
    device_handle_set_t *set = (device_handle_set_t *)&dev->handle_param;
    int i;

    for (i = 0; i < DEVICE_EVENT_MAX; i++) {
        val_t *ref = shell_reference_ptr(set->handles[i]);

        if (ref) {
            shell_reference_release(ref);
        }
    }
}

static void device_map_data_proc(cupkee_device_t *dev, env_t *env, val_t *handle)
{
    val_t info;

    device_get_all(dev, env, &info);

    shell_do_callback(env, handle, 1, &info);
}

static void device_data_proc(cupkee_device_t *dev, env_t *env, val_t *handle)
{
    size_t n;

    if (0 == cupkee_device_io_cached(dev, &n, NULL) && n > 0) {
        int err;
        val_t data;

        err = device_read_2_buffer(dev, env, n, &data);
        if (err < 0) {
            shell_do_callback_error(env, handle, err);
        } else {
            shell_do_callback(env, handle, 1, &data);
        }
    }
}

static void device_event_handle_wrap(cupkee_device_t *dev, uint8_t code, intptr_t param)
{
    env_t *env = cupkee_shell_env();
    val_t *handle = device_event_handle_get(dev, code);

    (void) param;

    if (!handle) {
        return;
    }

    if (code == DEVICE_EVENT_ERR) {
        shell_do_callback_error(env, handle, dev->error);
    } else
    if (code == DEVICE_EVENT_DATA) {
        // Todo: combine process of all type of device
        switch(dev->desc->category) {
        case DEVICE_CATEGORY_MAP:     device_map_data_proc(dev, env, handle); break;
        case DEVICE_CATEGORY_STREAM:
        case DEVICE_CATEGORY_BLOCK:   device_data_proc(dev, env, handle); break;
        default:                break;
        }
    } else
    if (code == DEVICE_EVENT_DRAIN) {
        shell_do_callback(env, handle, 0, NULL);
    } else
    if (code == DEVICE_EVENT_READY) {
        // Todo:
    } else {
        // What happen ?
    }
}

cupkee_device_t *cupkee_val2device(val_t *v)
{
    val_foreign_t *vf;

    if (val_is_foreign(v)) {
        vf = (val_foreign_t *)val_2_intptr(v);
        if (vf->op == &device_op) {
            return cupkee_device_block(vf->data);
        }
    }
    return NULL;
}

val_t cupkee_dev2val(env_t *env, cupkee_device_t *dev)
{
    if (dev) {
        dev->handle = device_event_handle_wrap;
        dev->handle_param = 0;
        return val_create(env, &device_op, cupkee_device_id(dev));
    } else {
        return VAL_UNDEFINED;
    }
}

val_t native_device_destroy(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;

    (void) env;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    device_event_handle_release(dev);
    if (CUPKEE_OK == cupkee_device_release(dev)) {
        return VAL_TRUE;
    } else {
        return VAL_FALSE;
    }
}

val_t native_device_config(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t *which = NULL;
    val_t *setting;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_UNDEFINED;
    }

    if (val_is_number(av) || val_is_string(av)) {
        which = av++; ac--;
    }
    setting = ac > 0 ? av : NULL;

    if (setting) {
        // config set is forbidden, if device is disabled
        if (cupkee_device_is_enabled(dev)) {
            return VAL_FALSE;
        }

        return which ? cupkee_device_config_set_one(dev, env, which, setting) :
                       cupkee_device_config_set_all(dev, env, setting) ? VAL_FALSE : VAL_TRUE;
    } else {
        return which ? cupkee_device_config_get_one(dev, env, which) :
                       cupkee_device_config_get_all(dev);
    }
}

val_t native_device_get(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t result;
    int offset;

    if (ac && (dev= cupkee_val2device(av))) {
        av++; ac--;
    } else {
        return VAL_UNDEFINED;
    }

    if (ac && val_is_number(av)) {
        offset = val_2_integer(av);
    } else {
        offset = -1;
    }

    device_get_elem(dev, env, offset, &result);
    return result;
}

val_t native_device_set(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    int offset, err = 0;
    uint32_t data;

    (void) env;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    if (ac < 1) {
        // Do nothing
        return VAL_TRUE;
    } else
    if (ac == 1) {
        if (val_is_number(av)) {
            offset = -1; // set all
            data = val_2_integer(av);
        } else {
            err = -CUPKEE_EINVAL;
        }
    } else {
        if (val_is_number(av) && val_is_number(av + 1)) {
            offset = val_2_integer(av);
            data = val_2_integer(av + 1);
        } else {
            err = -CUPKEE_EINVAL;
        }
    }

    if (!err) {
        err = cupkee_device_set(dev, offset, data);
    }

    return err > 0 ? VAL_TRUE : VAL_FALSE;
}

val_t native_device_write(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    int size, offset = 0, n = 0;
    int err = 0;
    void  *ptr;
    val_t *data;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    if (ac < 1 || (ptr = cupkee_val2data(av, &size)) == NULL) {
        err = -CUPKEE_EINVAL;
    } else {
        data = av++; ac--;

        if (ac && val_is_number(av)) {
            if (ac > 1 && val_is_number(av + 1)) {
                offset = val_2_integer(av);
                n = val_2_integer(av + 1);
                ac--; av++;
            } else {
                n = val_2_integer(av);
            }
            ac--; av++;
        } else {
            n = size;
        }

        if (offset < 0 || n < 0) {
            err = -CUPKEE_EINVAL;
        }
    }

    if (err) {
        if (ac) {
            shell_do_callback_error(env, av, err);
        }
        return VAL_FALSE;
    }

    if (n > 0 && offset < size) {
        if (offset + n > size) {
            n = size - offset;
        }
        n = cupkee_device_write(dev, n, ptr + offset);
    } else {
        n = 0;
    }

    if (ac) {
        val_t args[3];

        val_set_undefined(args);
        args[1] = *data;
        val_set_number(args + 2, n);

        shell_do_callback(env, av, 3, args);
    }

    return VAL_TRUE;
}

val_t native_device_read(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t args[2];
    int want, err = 0;
    size_t in;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    if (0 != cupkee_device_io_cached(dev, &in, NULL)) {
        in = 0;
    }

    if (ac && val_is_number(av)) {
        want = val_2_integer(av);
        if (want < 0) {
            want = 0;
        }
        ac--; av++;
    } else {
        want = in;
    }

    if (want > 0) {
        size_t n = (size_t) want;

        if (n > in) {
            cupkee_device_read_req(dev, n - in);
            n -= in;
        }
        if (n) {
            err = device_read_2_buffer(dev, env, n, args);
            if (err < 0) {
                shell_do_callback_error(env, av, err);
            } else
            if (err > 0) {
                val_set_undefined(&args[0]);
                shell_do_callback(env, av, 2, args);
            }
        }
    }

    return VAL_TRUE;
}

val_t native_device_listen(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t *callback;
    int event_id;

    (void) env;

    if (ac >= 3 && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    event_id = shell_val_id(av, DEVICE_EVENT_MAX, device_event_names);
    callback = av + 1;
    if (event_id < 0 || event_id > DEVICE_EVENT_MAX || !val_is_function(callback)) {
        return VAL_FALSE;
    }

    if (CUPKEE_OK == device_event_handle_set(dev, event_id, callback)) {
        return VAL_TRUE;
    } else {
        return VAL_FALSE;
    }
}

val_t native_device_ignore(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    int event_id;

    (void) env;

    if (ac >= 2 && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_UNDEFINED;
    }

    event_id = shell_val_id(av, DEVICE_EVENT_MAX, device_event_names);
    if (0 <= event_id && event_id < DEVICE_EVENT_MAX) {
        device_handle_set_t *set = (device_handle_set_t *)&dev->handle_param;
        uint8_t ref_id = set->handles[event_id];

        set->handles[event_id] = 0;
        shell_reference_release(shell_reference_ptr(ref_id));

        return VAL_TRUE;
    } else {
        return VAL_FALSE;
    }
}

val_t native_device_is_enabled(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;

    (void) env;

    if (ac == 0 || (dev = cupkee_val2device(av)) == NULL) {
        return VAL_UNDEFINED;
    } else {
        return cupkee_device_is_enabled(dev) ? VAL_TRUE : VAL_FALSE;
    }
}

val_t native_device_enable(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t *setting, *hnd;
    int err = 0;

    (void) env;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        hnd = av++; ac--;
    } else {
        return VAL_UNDEFINED;
    }

    setting = ac > 0 ? av : NULL;
    if (setting && val_is_object(setting)) {
        if (cupkee_device_is_enabled(dev)) {
            err = -CUPKEE_EENABLED;
        } else {
            err = cupkee_device_config_set_all(dev, env, setting);
        }
        ac--; av++;
    }

    if (!err) {
        err = cupkee_device_enable(dev);
    }

    if (ac && val_is_function(av)) {
        val_t args[2];

        args[0] = err ? val_mk_number(err) : VAL_UNDEFINED;
        args[1] = *hnd;

        shell_do_callback(env, av, 2, args);
    }

    return (err == CUPKEE_OK) ? VAL_TRUE : VAL_FALSE;
}

val_t native_device_disable(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;

    (void) env;

    if (ac == 0 || (dev = cupkee_val2device(av)) == NULL) {
        return VAL_UNDEFINED;
    }
    cupkee_device_disable(dev);

    return  cupkee_device_is_enabled(dev) ? VAL_FALSE : VAL_TRUE;
}

val_t native_device_create(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    const char *name;
    int inst;

    if (ac == 0) {
        device_list();
        return VAL_UNDEFINED;
    }

    name = val_2_cstring(av);
    if (!name) {
        return VAL_UNDEFINED;
    }

    if (ac > 1 && val_is_number(av + 1)) {
        inst= val_2_integer(av + 1);
    } else {
        inst= 0;
    }

    dev = cupkee_device_request(name, inst);
    if (dev) {
        return cupkee_dev2val(env, dev);
    } else {
        return VAL_UNDEFINED;
    }
}

