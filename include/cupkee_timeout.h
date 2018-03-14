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

#ifndef __CUPKEE_TIMEOUT_INC__
#define __CUPKEE_TIMEOUT_INC__

extern volatile uint32_t _cupkee_systicks;

typedef void (*cupkee_timeout_handle_t)(int drop, void *param);
typedef struct cupkee_timeout_t {
    struct cupkee_timeout_t *next;
    cupkee_timeout_handle_t handle;
    int      id;
    int      flags;
    uint32_t wait;
    uint32_t from;
    void    *param;
} cupkee_timeout_t;

void cupkee_timeout_setup(void);
void cupkee_timeout_sync(uint32_t ticks);

cupkee_timeout_t *cupkee_timeout_register(uint32_t wait, int repeat, cupkee_timeout_handle_t handle, void *param);
void cupkee_timeout_unregister(cupkee_timeout_t *t);

int cupkee_timeout_clear_all(void);
int cupkee_timeout_clear_with_flags(uint32_t flags);
int cupkee_timeout_clear_with_id(uint32_t id);

static inline uint32_t cupkee_systicks(void) {
    return _cupkee_systicks;
}


#endif /* __CUPKEE_TIMEOUT_INC__ */

