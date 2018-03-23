/* GPLv2 License
 *
 * Copyright (C) 2016-2018 Lixing Ding <ding.lixing@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 **/

#include "cupkee.h"

#define is_device(d)  cupkee_is_object((d), device_tag)

static uint8_t device_tag = 0xff;
static uint8_t device_type_num = 0;

static cupkee_device_desc_t const *device_descs[CUPKEE_DEVICE_TYPE_MAX];
static cupkee_device_t      *device_work = NULL;

static inline cupkee_device_t *device_data(int id)
{
    return (cupkee_device_t *) cupkee_entry(id, device_tag);
}

static inline int device_is_enabled(cupkee_device_t *dev) {
    return (dev->flags & DEVICE_FL_ENABLE);
}

static void device_join_work_list(cupkee_device_t *device)
{
    device->next = device_work;
    device_work = device;
}

static void device_drop_work_list(cupkee_device_t *device)
{
    cupkee_device_t *cur = device_work;

    if (cur == device) {
        device_work = cur->next;
        return;
    }

    while(cur) {
        cupkee_device_t *next = cur->next;

        if (next == device) {
            cur->next = device->next;
            return;
        }

        cur = next;
    }

    device->next = NULL;
}

static int device_type(const char *name)
{
    int i;

    for (i = 0; i < device_type_num; i++) {
        if (!strcmp(name, device_descs[i]->name)) {
            return i;
        }
    }
    return -1;
}

static void device_reset(cupkee_device_t *dev)
{
    const cupkee_device_desc_t *desc = device_descs[dev->type];
    dev->driver->reset(dev->instance);

    device_drop_work_list(dev);
    dev->flags = 0;

    if (dev->conf && desc->conf_init) {
        desc->conf_init(dev->conf);
    }

    if (dev->s) {
        cupkee_stream_deinit(dev->s);
        cupkee_free(dev->s);
        dev->s = NULL;
    }
}

static cupkee_device_t *device_request(int type, int instance)
{
    cupkee_object_t *obj;
    cupkee_device_t *dev;
    const cupkee_device_desc_t *desc = device_descs[type];

    if (0 != desc->driver->request(instance)) {
        return NULL;
    }

    obj = cupkee_object_create_with_id(device_tag);
    if (!obj) {
        desc->driver->release(instance);
    }

    dev = (cupkee_device_t *)obj->entry;

    dev->instance = instance;
    dev->type = type;
    dev->flags = 0;
    dev->error = 0;
    dev->driver = desc->driver;

    if (desc->conf_init) {
        dev->conf = desc->conf_init(NULL);
    } else {
        dev->conf = NULL;
    }

    dev->s    = NULL;

    dev->handle = NULL;
    dev->handle_param = 0;

    cupkee_buffer_init(&dev->req_buf, 0, NULL, 0);
    cupkee_buffer_init(&dev->res_buf, 0, NULL, 0);

    return dev;
}

static void device_destroy(void *entry)
{
    cupkee_device_t *dev = entry;

    if (device_is_enabled(dev)) {
        device_reset(dev);
    }
    dev->driver->release(dev->instance);

    if (dev->conf) {
        cupkee_struct_release(dev->conf);
    }

    cupkee_buffer_deinit(&dev->req_buf);
    cupkee_buffer_deinit(&dev->res_buf);
}

static int device_read(cupkee_stream_t *s, size_t n, void *buf)
{
    cupkee_device_t *dev = device_data(s->id);

    if (!dev) {
        return -CUPKEE_EINVAL;
    }

    if (dev->driver->read) {
        return dev->driver->read(dev->instance, n, buf);
    } else {
        return -CUPKEE_EIMPLEMENT;
    }
}

static int device_write(cupkee_stream_t *s, size_t n, const void *data)
{
    cupkee_device_t *dev = device_data(s->id);

    if (!dev) {
        return -CUPKEE_EINVAL;
    }

    if (dev->driver->write) {
        return dev->driver->write(dev->instance, n, data);
    } else {
        return -CUPKEE_EIMPLEMENT;
    }
}

static void device_stream_init(cupkee_device_t *dev, int id)
{
    size_t rx_size, tx_size;
    cupkee_stream_t *s;

    if (dev->driver->read) {
        // todo: get from conf
        rx_size = 32;
    } else {
        rx_size = 0;
    }

    if (dev->driver->write) {
        tx_size = 32;
    } else {
        tx_size = 0;
    }

    s = cupkee_malloc(sizeof(cupkee_stream_t));
    if (s) {
        if (0 != cupkee_stream_init(s, id, rx_size, tx_size, device_read, device_write)) {
            cupkee_free(s);
        } else {
            dev->s = s;
        }
    }
}

static int device_query_start(cupkee_device_t *dev, int want, cupkee_callback_t cb, intptr_t param)
{
    int err;

    if (dev->flags & DEVICE_FL_BUSY) {
        return -CUPKEE_EBUSY;
    }

    if (want > 0 && (cupkee_buffer_space_to(&dev->res_buf, want) < want)) {
        return -CUPKEE_ENOMEM;
    }

    dev->handle = cb;
    dev->handle_param = param;

    dev->flags |= DEVICE_FL_BUSY;

    err = dev->driver->query(dev->instance, want);
    if (err < 0) {
        dev->flags &= ~DEVICE_FL_BUSY;
    }

    return err;
}

static void device_error_handle(void *entry, int error)
{
    if (is_device(entry)) {
        cupkee_device_t *dev = entry;

        dev->error = error;
    }
}

static void device_event_handle(void *entry, uint8_t event)
{
    cupkee_device_t *dev = entry;

    if (is_device(dev)) {
        if (dev->handle) {
            dev->handle(entry, event, dev->handle_param);
        }
        if (event == CUPKEE_EVENT_RESPONSE) {
            cupkee_buffer_deinit(&dev->req_buf);
            dev->flags &= ~DEVICE_FL_BUSY;
        }
    }
}

static cupkee_stream_t *device_stream(void *entry)
{
    cupkee_device_t *dev = entry;

    if (is_device(entry)) {
        return dev->s;
    } else {
        return NULL;
    }
}

static void device_listen(void *entry, int event)
{
    cupkee_device_t *dev = entry;

    if (dev && dev->s) {
        cupkee_stream_listen(dev->s, event);
    }
}

static void device_ignore(void *entry, int event)
{
    cupkee_device_t *dev = entry;

    if (dev && dev->s) {
        cupkee_stream_ignore(dev->s, event);
    }
}

static int device_conf_get(cupkee_device_t *dev, const char *k, intptr_t *p)
{
    int i = cupkee_struct_item_id(dev->conf, k);

    if (i >= 0) {
        const char *str;
        const uint8_t *seq;
        int n;

        if (cupkee_struct_get_string(dev->conf, i, &str) > 0) {
            *p = (intptr_t) str;
            return CUPKEE_OBJECT_ELEM_STR;
        } else
        if (cupkee_struct_get_int(dev->conf, i, &n) > 0) {
            *p = n;
            return CUPKEE_OBJECT_ELEM_INT;
        } else
        if ((n = cupkee_struct_get_bytes(dev->conf, i, &seq)) >= 0) {
            *p = (intptr_t) (seq - 1);
            return CUPKEE_OBJECT_ELEM_OCT;
        }
    }

    return CUPKEE_OBJECT_ELEM_NV;
}

static int device_conf_set(cupkee_device_t *dev, const char *k, int t, intptr_t v)
{
    int i, retval;

    if (device_is_enabled(dev) || (i = cupkee_struct_item_id(dev->conf, k)) < 0) {
        return 0;
    }

    if (t == CUPKEE_OBJECT_ELEM_INT) {
        retval = cupkee_struct_set_int(dev->conf, i, v);
        if (retval < 0) {
            retval = cupkee_struct_push(dev->conf, i, v);
        }
    } else
    if (t == CUPKEE_OBJECT_ELEM_STR) {
        return cupkee_struct_set_string(dev->conf, i, (const char *)v);
    } else {
        retval = 0;
    }
    return retval;
}

static int device_prop_get(void *entry, const char *key, intptr_t *p)
{
    int retval;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    retval = device_conf_get(entry, key, p);
    if (retval <= CUPKEE_OBJECT_ELEM_NV) {
        if (!strcmp("isEnabled", key)) {
            *p = device_is_enabled(entry);
            retval = CUPKEE_OBJECT_ELEM_BOOL;
        }
    }

    return retval;
}

static int device_prop_set(void *entry, const char *k, int t, intptr_t v)
{
    int retval;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    retval = device_conf_set(entry, k, t, v);
    if (retval <= CUPKEE_OBJECT_ELEM_NV) {
        // Nothing
    }

    return retval;
}

static int device_elem_get(void *entry, int i, intptr_t *p)
{
    cupkee_device_t *dev = entry;
    uint32_t v;

    if (!is_device(entry) || !device_is_enabled(dev)) {
        return -CUPKEE_EINVAL;
    }

    if (dev->driver->get && dev->driver->get(dev->instance, i, &v) > 0) {
        *p = v;
        return CUPKEE_OBJECT_ELEM_INT;
    }

    return CUPKEE_OBJECT_ELEM_NV;
}

static int device_elem_set(void *entry, int i, int t, intptr_t v)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry) || !device_is_enabled(dev)) {
        return -CUPKEE_EINVAL;
    }

    if (t == CUPKEE_OBJECT_ELEM_INT && dev->driver->set) {
        return dev->driver->set(dev->instance, i, v);
    } else {
        return 0;
    }
}

static const cupkee_desc_t device_desc = {
    .name         = "Device",

    .error_handle = device_error_handle,
    .event_handle = device_event_handle,
    .streaming    = device_stream,
    .listen       = device_listen,
    .ignore       = device_ignore,

    .prop_get     = device_prop_get,
    .prop_set     = device_prop_set,
    .elem_get     = device_elem_get,
    .elem_set     = device_elem_set,

    .destroy      = device_destroy,
};

int cupkee_device_setup(void)
{
    int tag = cupkee_object_register(sizeof(cupkee_device_t), &device_desc);

    if (tag < 0) {
        return -1;
    }

    device_tag  = tag;
    device_work = NULL;
    device_type_num = 0;

    return 0;
}

int cupkee_device_tag(void)
{
    return device_tag;
}

int cupkee_device_register(const cupkee_device_desc_t *desc)
{
    if (!desc || !desc->name || !desc->driver) {
        return -CUPKEE_EINVAL;
    }

    if (device_type_num >= CUPKEE_DEVICE_TYPE_MAX) {
        return -CUPKEE_ELIMIT;
    }

    if (device_type(desc->name) >= 0) {
        return -CUPKEE_ENAME;
    }

    device_descs[device_type_num++] = desc;

    return 0;
}

void cupkee_device_sync(uint32_t systicks)
{
    cupkee_device_t *dev = device_work;

    while (dev) {
        if (dev->s) {
            cupkee_stream_sync(dev->s, systicks);
        }

        dev = dev->next;
    }
}

void cupkee_device_poll(void)
{
    cupkee_device_t *dev = device_work;

    hw_poll();
    while (dev) {
        if (dev->driver->poll) {
            dev->driver->poll(dev->instance);
        }
        dev = dev->next;
    }
}

void *cupkee_device_request(const char *name, int instance)
{
    int type = device_type(name);

    if (type < 0) {
        return NULL;
    } else {
        return device_request(type, instance);
    }
}

int cupkee_device_handle_set(void *entry, cupkee_callback_t handle, intptr_t param)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    dev->handle = handle;
    dev->handle_param = param;

    return 0;
}

cupkee_callback_t cupkee_device_handle_fn(void *entry)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return NULL;
    }
    return dev->handle;
}

intptr_t cupkee_device_handle_param(void *entry)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return 0;
    }
    return dev->handle_param;
}

cupkee_struct_t *cupkee_device_config(void *entry)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return NULL;
    }

    return dev->conf;
}

int cupkee_is_device(void *entry)
{
    return is_device(entry);
}

int cupkee_device_enable(void *entry)
{
    cupkee_device_t *dev = entry;
    int err;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (device_is_enabled(dev)) {
        return -CUPKEE_EBUSY;
    }

    err = dev->driver->setup(dev->instance, entry);
    if (err) {
        return err;
    }

    if (dev->driver->read || dev->driver->write) {
        device_stream_init(dev, CUPKEE_ENTRY_ID(entry));
    }

    if (dev->driver->set || dev->driver->get) {
        //device_vector_init(dev, id);
    }

    device_join_work_list(dev);
    dev->flags = DEVICE_FL_ENABLE;

    return 0;
}

int cupkee_device_disable(void *entry)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (device_is_enabled(dev)) {
        device_reset(dev);
    }

    return CUPKEE_OK;
}

int cupkee_device_is_enabled(void *entry)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    return device_is_enabled(dev);
}

int cupkee_device_request_len(void *entry)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (device_is_enabled(dev)) {
        return dev->req_buf.len;
    } else {
        return 0;
    }
}
void *cupkee_device_request_ptr(void *entry)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return NULL;
    }

    if (device_is_enabled(dev)) {
        return dev->req_buf.ptr;
    } else {
        return NULL;
    }
}

cupkee_buffer_t *cupkee_device_request_buffer(void *entry)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return NULL;
    }

    return &dev->req_buf;
}

int cupkee_device_request_load(void *entry, size_t n, void *data)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (device_is_enabled(dev)) {
        return cupkee_buffer_take(&dev->req_buf, n, data);
    } else {
        return -1;
    }
}

int cupkee_device_response_take(void *entry, void **pptr)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (device_is_enabled(dev)) {
        return cupkee_buffer_xxx(&dev->res_buf, pptr);
    } else {
        return -1;
    }
}

int cupkee_device_response_push(void *entry, size_t n, void *data)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (device_is_enabled(dev)) {
        return cupkee_buffer_give(&dev->res_buf, n, data);
    } else {
        return -1;
    }
}

void cupkee_device_response_end(void *entry)
{
    cupkee_device_t *dev = entry;

    if (is_device(entry)) {
        if (device_is_enabled(dev) && (dev->flags & DEVICE_FL_BUSY)) {
            cupkee_object_event_post(CUPKEE_ENTRY_ID(entry), CUPKEE_EVENT_RESPONSE);
        }
    }
}

int cupkee_device_query(void *entry, size_t req_len, void *req_data, int want, cupkee_callback_t cb, intptr_t param)
{
    int err;
    void *buf = NULL;
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (!device_is_enabled(dev)) {
        return -CUPKEE_EENABLED;
    }

    if (!dev->driver->query) {
        return -CUPKEE_EIMPLEMENT;
    }

    if (req_len) {
        if (!(buf = cupkee_malloc(req_len))) {
            return -CUPKEE_ENOMEM;
        }
        memcpy(buf, req_data, req_len);
        cupkee_buffer_init(&dev->req_buf, req_len, buf, CUPKEE_FLAG_OWNED);
    }

    err = device_query_start(dev, want, cb, param);
    if (err < 0) {
        cupkee_buffer_deinit(&dev->req_buf);
    }

    return err;
}

int cupkee_device_query_nocopy(void *entry, size_t req_len, void *req_data, int want, cupkee_callback_t cb, intptr_t param)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (!device_is_enabled(dev)) {
        return -CUPKEE_EENABLED;
    }

    if (!dev->driver->query) {
        return -CUPKEE_EIMPLEMENT;
    }
    cupkee_buffer_init(&dev->req_buf, req_len, req_data, 0);

    return device_query_start(dev, want, cb, param);
}

int cupkee_device_push(void *entry, size_t n, const void *data)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (!dev->s) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_push(dev->s, n, data);
}

int cupkee_device_pull(void *entry, size_t n, void *buf)
{
    cupkee_device_t *dev = entry;

    if (!is_device(entry)) {
        return -CUPKEE_EINVAL;
    }

    if (!dev->s) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_pull(dev->s, n, buf);
}

