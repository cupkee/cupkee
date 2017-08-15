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

#include "cupkee.h"
#include "cupkee_shell_device.h"

#define DEVICE_TYPE_MAX  16

static uint8_t device_tag = 0xff;
static uint8_t device_type_num = 0;

static cupkee_device_desc_t const *device_descs[DEVICE_TYPE_MAX];
static cupkee_device_t      *device_work = NULL;

static inline cupkee_device_t *device_block(int id)
{
    return (cupkee_device_t *) cupkee_object_data(id, device_tag);
}

static inline int device_is_enabled(cupkee_device_t *dev) {
    return (dev && (dev->flags & DEVICE_FL_ENABLE));
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
    dev->driver->reset(dev->instance);

    device_drop_work_list(dev);
    dev->flags = 0;

    if (dev->conf) {
        cupkee_struct_clear(dev->conf);
    }

    if (dev->s) {
        cupkee_stream_deinit(dev->s);
        cupkee_free(dev->s);
        dev->s = NULL;
    }
}

static int device_request(int type, int instance)
{
    int id;
    const cupkee_device_desc_t *desc;

    if (type < device_type_num) {
        desc = device_descs[type];
    } else {
        return -CUPKEE_EINVAL;
    }

    if (0 != desc->driver->request(instance)) {
        return -CUPKEE_ERESOURCE;
    }

    id = cupkee_object_alloc(device_tag);
    if (id >= 0) {
        cupkee_device_t *dev = device_block(id);

        dev->instance = instance;
        dev->type = type;
        dev->flags = 0;
        dev->error = 0;
        dev->driver = desc->driver;

        dev->conf = cupkee_struct_alloc(desc->conf_num, desc->conf_desc);
        dev->s    = NULL;

        dev->handle = NULL;
        dev->handle_param = 0;

        dev->req = dev->res = NULL;
    } else {
        desc->driver->release(instance);
    }

    return id;
}

static void device_destroy(int id)
{
    cupkee_device_t *dev = device_block(id);

    if (dev) {
        if (device_is_enabled(dev)) {
            device_reset(dev);
        }

        dev->driver->release(dev->instance);

        if (dev->conf) {
            cupkee_struct_release(dev->conf);
        }
    }
}

static int device_read(cupkee_stream_t *s, size_t n, void *buf)
{
    cupkee_device_t *dev = device_block(s->id);

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
    cupkee_device_t *dev = device_block(s->id);

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

static int device_query_start(cupkee_device_t *dev, void *req, int want, cupkee_callback_t cb, intptr_t param)
{
    int err;

    if (dev->flags & DEVICE_FL_BUSY) {
        return -CUPKEE_EBUSY;
    }

    if (want <= 0) {
        dev->res = NULL;
    } else
    if (!(dev->res = cupkee_buffer_alloc(want))) {
        return -CUPKEE_ENOMEM;
    }

    dev->handle = cb;
    dev->handle_param = param;

    dev->req = req;
    dev->flags |= DEVICE_FL_BUSY;

    err = dev->driver->query(dev->instance, want);
    if (err < 0) {
        if (dev->res) {
            cupkee_buffer_release(dev->res);
            dev->res = NULL;
        }
        dev->req = NULL;
        dev->flags &= ~DEVICE_FL_BUSY;
    }

    return err;
}

static void device_event_handle(int id, uint8_t event)
{
    cupkee_device_t *dev = device_block(id);

    if (dev) {
        if (dev->handle) {
            dev->handle(id, event, dev->handle_param);
        }
        if (event == CUPKEE_EVENT_RESPONSE) {
            if (dev->res) {
                cupkee_buffer_release(dev->res);
            }
            if (dev->req) {
                cupkee_buffer_release(dev->req);
            }

            dev->flags &= ~DEVICE_FL_BUSY;
        }
    }
}

static cupkee_stream_t *device_stream(int id)
{
    cupkee_device_t *dev = device_block(id);

    if (dev) {
        return dev->s;
    } else {
        return NULL;
    }
}

static void device_listen(int id, int event)
{
    cupkee_device_t *dev = device_block(id);

    if (dev) {
        switch(event) {
        case CUPKEE_EVENT_DATA:
        case CUPKEE_EVENT_DRAIN:
            if (dev->s) {
                cupkee_stream_listen(dev->s, event);
            }
            break;
        default:
            break;
        }
    }
}

static void device_ignore(int id, int event)
{
    cupkee_device_t *dev = device_block(id);

    if (dev) {
        switch(event) {
        case CUPKEE_EVENT_DATA:
        case CUPKEE_EVENT_DRAIN:
            if (dev->s) {
                cupkee_stream_ignore(dev->s, event);
            }
            break;
        default:
            break;
        }
    }
}

const static cupkee_meta_t device_meta = {
    .event_handle = device_event_handle,
    .streaming    = device_stream,
    .listen       = device_listen,
    .ignore       = device_ignore,
    .destroy      = device_destroy,
};

int cupkee_device_setup(void)
{
    if (0 > (device_tag = cupkee_object_register(sizeof(cupkee_device_t), &device_meta))) {
        return -1;
    }

    device_work = NULL;
    device_type_num = 0;

    return 0;
}

int cupkee_device_register(const cupkee_device_desc_t *desc)
{
    if (!desc || !desc->name || !desc->driver) {
        return -CUPKEE_EINVAL;
    }

    if (device_type_num >= DEVICE_TYPE_MAX) {
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

int cupkee_device_request(const char *name, int instance)
{
    int type = device_type(name);

    if (type < 0) {
        return -CUPKEE_EINVAL;
    } else {
        return device_request(type, instance);
    }
}

int cupkee_device_handle_set(int id, cupkee_callback_t handle, intptr_t param)
{
    cupkee_device_t *dev = device_block(id);

    if (!dev) {
        return -1;
    }

    dev->handle = handle;
    dev->handle_param = param;

    return 0;
}

cupkee_callback_t cupkee_device_handle_fn(int id)
{
    cupkee_device_t *dev = device_block(id);

    if (!dev) {
        return NULL;
    }
    return dev->handle;
}

intptr_t cupkee_device_handle_param(int id)
{
    cupkee_device_t *dev = device_block(id);

    if (!dev) {
        return 0;
    }
    return dev->handle_param;
}

int cupkee_is_device(int id)
{
    return cupkee_object_tag(id) == device_tag;
}

int cupkee_device_enable(int id)
{
    cupkee_device_t *dev = device_block(id);
    int err;

    if (device_is_enabled(dev)) {
        return -1;
    }

    err = dev->driver->setup(dev->instance, id);
    if (err) {
        return err;
    }

    device_stream_init(dev, id);
    //device_vector_init(dev, id);

    device_join_work_list(dev);
    dev->flags = DEVICE_FL_ENABLE;

    return 0;
}

int cupkee_device_disable(int id)
{
    cupkee_device_t *dev = device_block(id);

    if (device_is_enabled(dev)) {
        device_reset(dev);
    }

    return CUPKEE_OK;
}

int cupkee_device_is_enabled(int id)
{
    return device_is_enabled(device_block(id));
}

void *cupkee_device_request_take(int id)
{
    cupkee_device_t *dev = device_block(id);

    if (device_is_enabled(dev)) {
        void *req = dev->req;
        dev->req = NULL;
        return req;
    } else {
        return NULL;
    }
}

void *cupkee_device_response_take(int id)
{
    cupkee_device_t *dev = device_block(id);

    if (device_is_enabled(dev)) {
        void *res = dev->res;

        dev->res = NULL;
        return res;
    } else {
        return NULL;
    }
}

void *cupkee_device_request_ptr(int id)
{
    cupkee_device_t *dev = device_block(id);

    if (device_is_enabled(dev)) {
        if (dev->req) {
            return cupkee_buffer_ptr(dev->req);
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

int cupkee_device_request_load(int id, size_t n, void *data)
{
    cupkee_device_t *dev = device_block(id);

    if (device_is_enabled(dev)) {
        if (dev->req) {
            return cupkee_buffer_take(dev->req, n, data);
        } else {
            return 0;
        }
    } else {
        return -1;
    }
}

int cupkee_device_response_push(int id, size_t n, void *data)
{
    cupkee_device_t *dev = device_block(id);

    if (device_is_enabled(dev) && dev->res) {
        return cupkee_buffer_give(dev->res, n, data);
    } else {
        return -1;
    }
}

void cupkee_device_response_end(int id)
{
    cupkee_device_t *dev = device_block(id);

    if (device_is_enabled(dev) && (dev->flags & DEVICE_FL_BUSY)) {
        cupkee_object_event_post(id, CUPKEE_EVENT_RESPONSE);
    }
}

int cupkee_device_query(int id, size_t req_len, void *req_data, int want, cupkee_callback_t cb, intptr_t param)
{
    int err;
    void *req = NULL;
    cupkee_device_t *dev = device_block(id);

    if (!device_is_enabled(dev)) {
        return -CUPKEE_EENABLED;
    }

    if (!dev->driver->query) {
        return -CUPKEE_EIMPLEMENT;
    }

    if (req_len && NULL == (req = cupkee_buffer_create(req_len, req_data))) {
        return -CUPKEE_ENOMEM;
    }

    err = device_query_start(dev, req, want, cb, param);
    if (err < 0 && req) {
        cupkee_buffer_release(req);
    }

    return err;
}

int cupkee_device_query2(int id, void *req, int want, cupkee_callback_t cb, intptr_t param)
{
    cupkee_device_t *dev = device_block(id);

    if (!device_is_enabled(dev)) {
        return -CUPKEE_EENABLED;
    }

    if (!dev->driver->query) {
        return -CUPKEE_EIMPLEMENT;
    }

    return device_query_start(dev, req, want, cb, param);
}

int cupkee_device_push(int id, size_t n, const void *data)
{
    cupkee_device_t *dev = device_block(id);

    if (!dev || !dev->s) {
        return -CUPKEE_EINVAL;
    }

    return cupkee_stream_push(dev->s, n, data);
}

int cupkee_device_pull(int id, size_t n, void *buf)
{
    cupkee_device_t *dev = device_block(id);

    if (!dev || !dev->s) {
        return -CUPKEE_EINVAL;
    }

    return cupkee_stream_pull(dev->s, n, buf);
}

int cupkee_device_read(int id, size_t n, void *buf)
{
    (void) id;
    (void) n;
    (void) buf;

    return 0;
}

int cupkee_device_write(int id, size_t n, const uint8_t *data)
{
    (void) id;
    (void) n;
    (void) data;
    return 0;
}

int cupkee_device_write_sync(int id, size_t n, const uint8_t *data)
{
    (void) id;
    (void) n;
    (void) data;

    return 0;
}


