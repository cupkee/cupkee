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

#ifndef __CUPKEE_DEVICE_INC__
#define __CUPKEE_DEVICE_INC__

#define DEVICE_FL_ENABLE    1
#define DEVICE_FL_BUSY      2

typedef struct cupkee_device_t cupkee_device_t;

typedef void (*cupkee_handle_t)(cupkee_device_t *, uint8_t event, intptr_t param);

typedef struct cupkee_driver_t {
    int (*request)(int inst);
    int (*release)(int inst);
    int (*setup)(int inst, void *entry);
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
    cupkee_struct_t *(*conf_init)(void *curr);
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

int cupkee_device_setup(void);
int cupkee_device_tag(void);
void cupkee_device_sync(uint32_t systicks);
void cupkee_device_poll(void);
int  cupkee_device_register(const cupkee_device_desc_t *desc);

void *cupkee_device_request(const char *name, int instance);
int  cupkee_is_device(void *entry);

int cupkee_device_handle_set(void *entry, cupkee_callback_t handle, intptr_t param);
cupkee_callback_t cupkee_device_handle_fn(void *entry);
intptr_t cupkee_device_handle_param(void *entry);

cupkee_struct_t *cupkee_device_config(void *entry);

int cupkee_device_enable(void *entry);
int cupkee_device_disable(void *entry);
int cupkee_device_is_enabled(void *entry);

int cupkee_device_query(void *entry, size_t req_len, void *req_data, int want, cupkee_callback_t cb, intptr_t param);
int cupkee_device_query2(void *entry, void *req, int want, cupkee_callback_t cb, intptr_t param);

/* used by driver */
void *cupkee_device_request_take(void *entry);
void *cupkee_device_response_take(void *entry);

int   cupkee_device_request_len(void *entry);
void *cupkee_device_request_ptr(void *entry);
int   cupkee_device_request_load(void *entry, size_t n, void *data);

void cupkee_device_response_end(void *entry);
int  cupkee_device_response_push(void *entry, size_t n, void *data);

int cupkee_device_push(void *entry, size_t n, const void *data);
int cupkee_device_pull(void *entry, size_t n, void *buf);

static inline void cupkee_device_set_error(void *entry, uint8_t code) {
    cupkee_object_error_set(CUPKEE_OBJECT_PTR(entry), code);
}

#endif /* __CUPKEE_DEVICE_INC__ */

