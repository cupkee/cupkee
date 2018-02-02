/*
MIT License

This file is part of cupkee project.

Copyright (c) 2017 Lixing Ding <ding.lixing@gmail.com>

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

#ifndef __CUPKEE_PIN_INC__
#define __CUPKEE_PIN_INC__

#define CUPKEE_PIN_OUT      HW_DIR_OUT
#define CUPKEE_PIN_IN       HW_DIR_IN
#define CUPKEE_PIN_DUPLEX   HW_DIR_DUPLEX

int cupkee_pin_setup(void);
void cupkee_pin_event_dispatch(uint16_t id, uint8_t code);

int cupkee_pin_map(int pin, int bank, int port);

int cupkee_pin_enable(int pin, int dir);
int cupkee_pin_disable(int pin);
int cupkee_pin_listen(int pin, int events, cupkee_callback_t handler, void *entry);
int cupkee_pin_ignore(int pin);

int cupkee_pin_set(int pin, int v);
int cupkee_pin_get(int pin);
int cupkee_pin_toggle(int pin);

void *cupkee_pin_group_create(void);
int cupkee_pin_group_destroy(void *grp);
int cupkee_pin_group_size(void *grp);
int cupkee_pin_group_push(void *grp, int pin);
int cupkee_pin_group_pop(void *grp);

int cupkee_pin_group_get(void *grp);
int cupkee_pin_group_set(void *grp, uint32_t v);
int cupkee_pin_group_elem_get(void *grp, int id);
int cupkee_pin_group_elem_set(void *grp, int id, int v);

#endif /* __CUPKEE_PIN_INC__ */

