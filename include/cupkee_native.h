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

#ifndef __CUPKEE_NATIVE_INC__
#define __CUPKEE_NATIVE_INC__

/* cupkee_shell_misc.c */
val_t native_sysinfos(env_t *env, int ac, val_t *av);
val_t native_systicks(env_t *env, int ac, val_t *av);
val_t native_print(env_t *env, int ac, val_t *av);
val_t native_led_map(env_t *env, int ac, val_t *av);
val_t native_pin_map(env_t *env, int ac, val_t *av);
val_t native_led(env_t *env, int ac, val_t *av);

/* cupkee_module.c */
val_t native_require(env_t *env, int ac, val_t *av);

/* cupkee_shell_systice.c */
val_t native_set_timeout(env_t *env, int ac, val_t *av);
val_t native_set_interval(env_t *env, int ac, val_t *av);
val_t native_clear_timeout(env_t *env, int ac, val_t *av);
val_t native_clear_interval(env_t *env, int ac, val_t *av);

/* cupkee_shell_device.c */
val_t native_device_create(env_t *env, int ac, val_t *av);
val_t native_device_destroy(env_t *env, int ac, val_t *av);
val_t native_device_config(env_t *env, int ac, val_t *av);
val_t native_device_is_enabled(env_t *env, int ac, val_t *av);
val_t native_device_enable (env_t *env, int ac, val_t *av);
val_t native_device_disable(env_t *env, int ac, val_t *av);
val_t native_device_query(env_t *env, int ac, val_t *av);
val_t native_device_get(env_t *env, int ac, val_t *av);
val_t native_device_set(env_t *env, int ac, val_t *av);
val_t native_device_write(env_t *env, int ac, val_t *av);
val_t native_device_read (env_t *env, int ac, val_t *av);
val_t native_device_listen(env_t *env, int ac, val_t *av);
val_t native_device_ignore(env_t *env, int ac, val_t *av);

/* cupkee_shell_timer.c */
val_t native_timer_create(env_t *env, int ac, val_t *av);

#endif /* __CUPKEE_NATIVE_INC__ */

