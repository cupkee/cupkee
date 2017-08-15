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

typedef void (*cupkee_callback_t)(int id, int event, intptr_t param);

typedef void (*cupkee_handle_t)(cupkee_device_t *, uint8_t event, intptr_t param);

typedef struct cupkee_driver_t {
    int (*request)(int inst);
    int (*release)(int inst);
    int (*setup)(int inst, int id);
    int (*reset)(int inst);
    int (*poll)(int inst);

    int (*query)(int inst, int want);
} cupkee_driver_t;

typedef struct cupkee_device_desc_t {
    const char *name;
    uint8_t  inst_max;
    uint8_t  conf_num;
    const cupkee_struct_desc_t *conf_desc;
    const cupkee_driver_t *driver;
} cupkee_device_desc_t;

struct cupkee_device_t {
    uint8_t instance;
    uint8_t type;
    uint8_t flags;
    uint8_t error;

    cupkee_callback_t handle;
    intptr_t          handle_param;

    void             *req;
    void             *res;

    cupkee_struct_t   *conf;
    const cupkee_driver_t *driver;

    cupkee_device_t *next;
};

int  cupkee_device_setup(void);
void cupkee_device_sync(uint32_t systicks);
void cupkee_device_poll(void);
int  cupkee_device_register(const cupkee_device_desc_t *desc);

int cupkee_device_request(const char *name, int instance);
int cupkee_device_release(int id);
int cupkee_is_device(int id);

int cupkee_device_enable(int id);
int cupkee_device_disable(int id);
int cupkee_device_is_enabled(int id);
int cupkee_device_query(int id, size_t req_len, void *req_data, int want, cupkee_callback_t cb, intptr_t param);
int cupkee_device_query2(int id, void *req, int want, cupkee_callback_t cb, intptr_t param);

int cupkee_device_read(int id, size_t n, void *buf);
int cupkee_device_write(int id, size_t n, const uint8_t *data);
int cupkee_device_write_sync(int id, size_t n, const uint8_t *data);

void *cupkee_device_request_take(int id);
void *cupkee_device_response_take(int id);

void *cupkee_device_request_ptr(int id);
int   cupkee_device_request_load(int id, size_t n, void *data);

void cupkee_device_response_end(int id);
int  cupkee_device_response_push(int id, size_t n, void *data);

static inline void cupkee_device_set_error(int id, uint8_t code) {
    cupkee_object_error_set(id, code);
}

#endif /* __CUPKEE_DEVICE_INC__ */

