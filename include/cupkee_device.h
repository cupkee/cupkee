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

#ifndef __CUPKEE_DEVICE_INC__
#define __CUPKEE_DEVICE_INC__

#define DEVICE_FL_ENABLE    1
#define DEVICE_FL_BUSY      2

typedef struct cupkee_device_t cupkee_device_t;

typedef void (*cupkee_callback_t)(void *hnd, int state, intptr_t param);

typedef void (*cupkee_handle_t)(cupkee_device_t *, uint8_t event, intptr_t param);

typedef struct cupkee_device_desc_t {
    const char *name;
    uint16_t type;
    uint8_t  category;
    uint8_t  conf_num;
    const char * const *conf_names;
    void (*conf_init)(hw_config_t *);
} cupkee_device_desc_t;

struct cupkee_device_t {
    uint8_t id;
    uint8_t flags;
    uint8_t error;
    uint8_t instance;

    hw_config_t config;

    cupkee_handle_t handle;
    intptr_t        handle_param;

    intptr_t        query_param;       // Todo: rename
    cupkee_callback_t reply_handle;    // Todo: rename and combine with handle

    void             *req;
    void             *res;

    const cupkee_device_desc_t *desc;
    const hw_driver_t *driver;

    struct cupkee_device_t *next;
};

int  cupkee_device_init(void);
void cupkee_device_poll(void);
void cupkee_device_sync(uint32_t systicks);
void cupkee_device_event_handle(uint16_t which, uint8_t code);

cupkee_device_t *cupkee_device_request(const char *name, int instance);
cupkee_device_t *cupkee_device_request2(int type, int instance);
int cupkee_device_release(cupkee_device_t *dev);

cupkee_device_t *cupkee_device_block(int id);
hw_config_t     *cupkee_device_config(int id);

int cupkee_device_id(cupkee_device_t *device);
int cupkee_device_elem_id(cupkee_device_t *dev, int index);
int cupkee_device_elem_index(intptr_t id, cupkee_device_t **pdev);

int cupkee_device_reply_push(cupkee_device_t *dev, size_t n, void *data);

static inline int cupkee_device_is_enabled(cupkee_device_t *dev) {
    return (dev && (dev->flags & DEVICE_FL_ENABLE));
}

static inline void *cupkee_device_request_take(cupkee_device_t *dev)
{
    if (cupkee_device_is_enabled(dev)) {
        void *req = dev->req;
        dev->req = NULL;
        return req;
    } else {
        return NULL;
    }
}

static inline void *cupkee_device_response_take(cupkee_device_t *dev) {
    if (cupkee_device_is_enabled(dev)) {
        void *res = dev->res;

        dev->res = NULL;
        return res;
    } else {
        return NULL;
    }
}

void cupkee_device_set_error(int id, uint8_t code);
int cupkee_device_enable(cupkee_device_t *dev);
int cupkee_device_disable(cupkee_device_t *dev);

int cupkee_device_set(cupkee_device_t *dev, int prop, uint32_t data);
int cupkee_device_get(cupkee_device_t *dev, int prop, uint32_t *data);

int cupkee_device_received(cupkee_device_t *dev);
int cupkee_device_recv(cupkee_device_t *dev, int n, void *buf);
int cupkee_device_recv_sync(cupkee_device_t *dev, int n, void *buf);
int cupkee_device_send(cupkee_device_t *dev, int n, const void *data);
int cupkee_device_send_sync(cupkee_device_t *dev, int n, const void *data);

int cupkee_device_read_req(cupkee_device_t *dev, size_t n);

int cupkee_device_read(cupkee_device_t *dev, size_t n, void *buf);
int cupkee_device_write(cupkee_device_t *dev, size_t n, const void *data);

int cupkee_device_read_sync(cupkee_device_t *dev, size_t n, void *buf);
int cupkee_device_write_sync(cupkee_device_t *dev, size_t n, const void *data);

int cupkee_device_io_cached(cupkee_device_t *dev, size_t *in, size_t *out);

// new api
void *cupkee_device_request_ptr(cupkee_device_t *dev);
int cupkee_device_request_load(cupkee_device_t *dev, size_t n, void *data);

void cupkee_device_response_end(cupkee_device_t *dev);
int cupkee_device_response_push(cupkee_device_t *dev, size_t n, void *data);

int cupkee_device_query(cupkee_device_t *dev, size_t n, void *data, int want, cupkee_callback_t cb, intptr_t param);

#endif /* __CUPKEE_DEVICE_INC__ */

