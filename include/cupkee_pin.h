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

#ifndef __CUPKEE_PIN_INC__
#define __CUPKEE_PIN_INC__

#define CUPKEE_PIN_OUT      HW_DIR_OUT
#define CUPKEE_PIN_IN       HW_DIR_IN
#define CUPKEE_PIN_DUPLEX   HW_DIR_DUPLEX

typedef struct cupkee_pinmap_t {
    uint8_t bank;
    uint8_t port;
} cupkee_pinmap_t;

int  cupkee_pin_setup(void);
void cupkee_pin_event_dispatch(uint16_t id, uint8_t code);

int cupkee_pin_map(uint8_t pin_num, const cupkee_pinmap_t *pin_map);

int cupkee_pin_enable(int pin, int dir);
int cupkee_pin_disable(int pin);
int cupkee_pin_listen(int pin, int events, cupkee_callback_t handler, void *entry);
int cupkee_pin_ignore(int pin);

int cupkee_pin_set(int pin, int v);
int cupkee_pin_get(int pin);
int cupkee_pin_toggle(int pin);

int cupkee_is_pin_group(void *entry);
int cupkee_pin_group_tag(void);
void *cupkee_pin_group_create(void);
int cupkee_pin_group_push(void *entry, int pin);
int cupkee_pin_group_pop(void *entry);

int cupkee_pin_group_get(void *entry);
int cupkee_pin_group_set(void *entry, uint32_t v);

#endif /* __CUPKEE_PIN_INC__ */

