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

static cupkee_device_t *devices[APP_DEV_MAX];
static cupkee_device_t *device_work = NULL;

static cupkee_device_t *device_block_alloc(void)
{
    int i;

    for (i = 0; i < APP_DEV_MAX; i++) {
        if (devices[i] == NULL) {
            cupkee_device_t *dev = (cupkee_device_t *)cupkee_malloc(sizeof(cupkee_device_t));
            if (dev) {
                devices[i] = dev;
                dev->id = i;
            }
            return dev;
        }
    }
    return NULL;
}

static void device_block_release(cupkee_device_t *dev)
{
    memset(dev, 0, sizeof(cupkee_device_t));

    if (dev == devices[dev->id]) {
        devices[dev->id] = NULL;
        cupkee_free(dev);
    }
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

static cupkee_device_t *device_request(const cupkee_device_desc_t *desc, int instance)
{
    cupkee_device_t *dev;
    const hw_driver_t *driver;

    dev = device_block_alloc();
    if (!dev) {
        return NULL;
    }

    driver = hw_device_request(desc->type, instance);
    if (!driver) {
        device_block_release(dev);
        return NULL;
    }

    dev->flags = 0;
    dev->error = 0;
    dev->instance = instance;

    dev->driver = driver;
    dev->desc   = desc;

    dev->next   = NULL;

    return dev;
}

int cupkee_device_init(void)
{
    /* Device blocks initial */
    memset(devices, 0, sizeof(devices));
    device_work = NULL;

    return 0;
}

int cupkee_device_id(cupkee_device_t *device)
{
    return device->id;
}

int cupkee_device_elem_id(cupkee_device_t *dev, int index)
{
    int id = cupkee_device_id(dev);

    return id + (index << 16);
}

int cupkee_device_elem_index(intptr_t id, cupkee_device_t **pdev)
{
    int index = id >> 16;

    if (pdev) {
        id = (uint8_t) id;
        *pdev = cupkee_device_block(id);
    }
    return index;
}


cupkee_device_t *cupkee_device_block(int id)
{
    if (id >= 0 && id < APP_DEV_MAX) {
        cupkee_device_t *dev = devices[id];

        if (dev->desc) {
            return dev;
        }
    }

    return NULL;
}

hw_config_t *cupkee_device_config(int id)
{
    if (id >= 0 && id < APP_DEV_MAX) {
        cupkee_device_t *dev = devices[id];

        if (dev->desc) {
            return &dev->config;
        }
    }

    return NULL;
}

void cupkee_device_set_error(int id, uint8_t code)
{
    if (id >= 0 && id < APP_DEV_MAX) {
        cupkee_device_t *dev = devices[id];

        if (cupkee_device_is_enabled(dev)) {
            dev->error = code;
            cupkee_event_post_device_error(id);
        }
    }
}

void cupkee_device_event_handle(uint16_t which, uint8_t code)
{
    cupkee_device_t *dev = devices[which];

    if ((dev->flags & DEVICE_FL_ENABLE) && dev->handle) {
        dev->handle(dev, code, dev->handle_param);
    }
}

void cupkee_device_sync(uint32_t systicks)
{
    cupkee_device_t *dev = device_work;

    while (dev) {
        if (dev->driver->sync) {
            dev->driver->sync(dev->instance, systicks);
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

cupkee_device_t *cupkee_device_request(const char *name, int instance)
{
    const cupkee_device_desc_t *desc;

    desc = cupkee_device_query_by_name(name);
    if (!desc) {
        return NULL;
    }

    return device_request(desc, instance);
}

cupkee_device_t *cupkee_device_request2(int type, int instance)
{
    const cupkee_device_desc_t *desc;

    desc = cupkee_device_query_by_type(type);
    if (!desc) {
        return NULL;
    }

    return device_request(desc, instance);
}

int cupkee_device_release(cupkee_device_t *dev)
{
    cupkee_device_disable(dev);

    if (dev->driver->release) {
        dev->driver->release(dev->instance);
    }
    device_block_release(dev);

    return CUPKEE_OK;
}

int cupkee_device_enable(cupkee_device_t *dev)
{
    int id, err;

    if (cupkee_device_is_enabled(dev)) {
        return CUPKEE_OK;
    }

    id = cupkee_device_id(dev);
    if (id < 0) {
        return -CUPKEE_EINVAL;
    }

    err = dev->driver->setup(dev->instance, id, &dev->config);
    if (0 == err) {
        dev->flags |= DEVICE_FL_ENABLE;
        device_join_work_list(dev);
        return CUPKEE_OK;
    }

    return err;
}

int cupkee_device_disable(cupkee_device_t *dev)
{
    if (cupkee_device_is_enabled(dev)) {
        dev->driver->reset(dev->instance);
        device_drop_work_list(dev);
        dev->flags &= ~DEVICE_FL_ENABLE;
    }

    return CUPKEE_OK;
}

int cupkee_device_set(cupkee_device_t *dev, int n, uint32_t data)
{
    if (cupkee_device_is_enabled(dev)) {
        if (dev->driver->set) {
            return dev->driver->set(dev->instance, n, data);
        } else {
            return -CUPKEE_EIMPLEMENT;
        }
    } else {
        return -CUPKEE_EENABLED;
    }
}

int cupkee_device_get(cupkee_device_t *dev, int n, uint32_t *data)
{
    if (cupkee_device_is_enabled(dev)) {
        if (dev->driver->get) {
            return dev->driver->get(dev->instance, n, data);
        } else {
            return -CUPKEE_EIMPLEMENT;
        }
    } else {
        return -CUPKEE_EENABLED;
    }
}

int cupkee_device_read_req(cupkee_device_t *dev, size_t n)
{
    if (cupkee_device_is_enabled(dev)) {
        if (dev->driver->read_req) {
            return dev->driver->read_req(dev->instance, n);
        } else {
            return -CUPKEE_EIMPLEMENT;
        }
    } else {
        return -CUPKEE_EENABLED;
    }
}

int cupkee_device_read(cupkee_device_t *dev, size_t n, void *buf)
{
    if (cupkee_device_is_enabled(dev)) {
        if (dev->driver->read) {
            return dev->driver->read(dev->instance, n, buf);
        } else {
            return -CUPKEE_EIMPLEMENT;
        }
    } else {
        return -CUPKEE_EENABLED;
    }
}

int cupkee_device_write(cupkee_device_t *dev, size_t n, const void *data)
{
    if (cupkee_device_is_enabled(dev)) {
        if (dev->driver->write) {
            return dev->driver->write(dev->instance, n, data);
        } else {
            return -CUPKEE_EIMPLEMENT;
        }
    } else {
        return -CUPKEE_EENABLED;
    }
}

int cupkee_device_read_sync(cupkee_device_t *dev, size_t n, void *buf)
{
    if (cupkee_device_is_enabled(dev)) {
        if (dev->driver->read_sync) {
            return dev->driver->read_sync(dev->instance, n, buf);
        } else {
            return -CUPKEE_EIMPLEMENT;
        }
    } else {
        return -CUPKEE_EENABLED;
    }
}

int cupkee_device_write_sync(cupkee_device_t *dev, size_t n, const void *data)
{
    if (cupkee_device_is_enabled(dev)) {
        if (dev->driver->write_sync) {
            return dev->driver->write_sync(dev->instance, n, data);
        } else {
            return -CUPKEE_EIMPLEMENT;
        }
    } else {
        return -CUPKEE_EENABLED;
    }
}

int cupkee_device_io_cached(cupkee_device_t *dev, size_t *in, size_t *out)
{
    if (cupkee_device_is_enabled(dev)) {
        if (dev->driver->io_cached) {
            return dev->driver->io_cached(dev->instance, in, out);
        } else {
            return -CUPKEE_EIMPLEMENT;
        }
    } else {
        return -CUPKEE_EENABLED;
    }
}

int cupkee_device_request_pull(cupkee_device_t *dev, size_t n, void *data)
{
    if (cupkee_device_is_enabled(dev)) {
        if (dev->req) {
            return cupkee_buffer_take(dev->req, n, data);
        } else {
            return 0;
        }
    } else {
        return -1;
    }
}

int cupkee_device_response_push(cupkee_device_t *dev, size_t n, void *data)
{
    if (cupkee_device_is_enabled(dev) && dev->res) {
        return cupkee_buffer_give(dev->res, n, data);
    } else {
        return -1;
    }
}

void cupkee_device_response_end(cupkee_device_t *dev)
{
    if (cupkee_device_is_enabled(dev) && (dev->flags & DEVICE_FL_BUSY)) {
        if (dev->reply_handle) {
            cupkee_callback_t cb = dev->reply_handle;
            intptr_t param = dev->query_param;

            dev->reply_handle = NULL;
            dev->query_param = 0;

            cb(dev, CUPKEE_OK, param);
        }
        if (dev->res) {
            cupkee_buffer_release(dev->res);
        }
        if (dev->req) {
            cupkee_buffer_release(dev->req);
        }

        dev->flags &= ~DEVICE_FL_BUSY;
    }
}

static int device_request_setup(cupkee_device_t *dev, size_t send, void *data, int want)
{
    if (dev->flags & DEVICE_FL_BUSY) {
        return -CUPKEE_EBUSY;
    }

    if (send && NULL == (dev->req = cupkee_buffer_create(send, data))) {
        return -CUPKEE_ENOMEM;
    }

    if (want && NULL == (dev->res = cupkee_buffer_alloc(want))) {
        if (dev->req) {
            cupkee_buffer_release(dev->req);
        }
        return -CUPKEE_ENOMEM;
    }

    dev->flags |= DEVICE_FL_BUSY;


    return 0;
}

int cupkee_device_query(cupkee_device_t *dev, size_t n, void *data, int want, cupkee_callback_t cb, intptr_t param)
{
    int retv;

    if (!cupkee_device_is_enabled(dev)) {
        return -CUPKEE_EENABLED;
    }

    if (!dev->driver->query) {
        return -CUPKEE_EIMPLEMENT;
    }

    retv = device_request_setup(dev, n, data, want);
    if (retv) {
        return retv;
    }
    dev->reply_handle = cb;
    dev->query_param = param;

    retv = dev->driver->query(dev->instance, n, want);
    if (retv) {
        if (dev->req) {
            cupkee_buffer_release(dev->req);
        }
        if (dev->res) {
            cupkee_buffer_release(dev->res);
        }
    }

    return retv;
}

