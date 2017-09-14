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

typedef void (*cupkee_handle_t)(cupkee_device_t *, uint8_t event, intptr_t param);

typedef struct cupkee_driver_t {
    int (*request)(int inst);
    int (*release)(int inst);
    int (*setup)(int inst, int id);
    int (*reset)(int inst);
    int (*poll)(int inst);

    int (*query)(int inst, int want);

    int (*read )(int inst, size_t n, void *buf);
    int (*write)(int inst, size_t n, const void *data);

    int (*set)(int inst, int id, uint32_t v);
    int (*get)(int inst, int id, uint32_t *v);
} cupkee_driver_t;

typedef struct cupkee_device_desc_t {
    const char *name;
    uint8_t  inst_max;
    uint8_t  conf_num;
    const cupkee_struct_desc_t *conf_desc;
    const cupkee_driver_t *driver;
} cupkee_device_desc_t;

struct cupkee_device_t {
    cupkee_device_t *next;

    uint8_t instance;
    uint8_t type;
    uint8_t flags;
    uint8_t error;

    cupkee_callback_t handle;
    intptr_t          handle_param;

    void             *req;
    void             *res;

    const cupkee_driver_t *driver;

    cupkee_struct_t  *conf;
    cupkee_stream_t  *s;
};

int  cupkee_device_setup(void);
void cupkee_device_sync(uint32_t systicks);
void cupkee_device_poll(void);
int  cupkee_device_register(const cupkee_device_desc_t *desc);

int cupkee_device_request(const char *name, int instance);
int cupkee_is_device(int id);

int cupkee_device_handle_set(int id, cupkee_callback_t handle, intptr_t param);
cupkee_callback_t cupkee_device_handle_fn(int id);
intptr_t cupkee_device_handle_param(int id);

int cupkee_device_enable(int id);
int cupkee_device_disable(int id);
int cupkee_device_is_enabled(int id);
int cupkee_device_query(int id, size_t req_len, void *req_data, int want, cupkee_callback_t cb, intptr_t param);
int cupkee_device_query2(int id, void *req, int want, cupkee_callback_t cb, intptr_t param);

int cupkee_device_streaming( int id,
    int (*_read)(cupkee_stream_t *s, size_t n, void *buf),
    int (*_write)(cupkee_stream_t *s, size_t n, const void *data)
);

void *cupkee_device_request_take(int id);
void *cupkee_device_response_take(int id);

int   cupkee_device_request_len(int id);
void *cupkee_device_request_ptr(int id);
int   cupkee_device_request_load(int id, size_t n, void *data);

void cupkee_device_response_end(int id);
int  cupkee_device_response_push(int id, size_t n, void *data);

int cupkee_device_push(int id, size_t n, const void *data);
int cupkee_device_pull(int id, size_t n, void *buf);
int cupkee_device_unshift(int id, uint8_t data);

static inline void cupkee_device_release(int id) {
    cupkee_release(id);
}

static inline void cupkee_device_set_error(int id, uint8_t code) {
    cupkee_error_set(id, code);
}

#endif /* __CUPKEE_DEVICE_INC__ */

