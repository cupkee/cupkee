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

#include "cupkee_shell_util.h"

#define VARIABLE_REF_MAX (32)
#if     VARIABLE_REF_MAX > 255
#error "VARIABLE_REF_MAX should not big than 255"
#endif

static val_t reference_vals[VARIABLE_REF_MAX];

static inline void print_number(val_t *v) {
    if (*v & 0xffff) {
        console_log("%f", val_2_double(v));
    } else {
        console_log("%lld", (int64_t)val_2_double(v));
    }
}

static inline void print_boolean(val_t *v) {
    console_puts(val_2_intptr(v) ? "true" : "false");
}

static inline void print_string(val_t *v) {
    console_putc('"');
    console_puts(val_2_cstring(v));
    console_putc('"');
}

static void print_simple_value(val_t *v)
{
    if (val_is_number(v)) {
        print_number(v);
    } else
    if (val_is_boolean(v)) {
        print_boolean(v);
    } else
    if (val_is_string(v)) {
        print_string(v);
    } else
    if (val_is_undefined(v)) {
        console_puts("undefined");
    } else
    if (val_is_nan(v)) {
        console_puts("NaN");
    } else
    if (val_is_function(v)) {
        console_puts("<function>");
    } else
    if (val_is_object(v)) {
        console_puts("<object>");
    } else
    if (val_is_array(v)) {
        console_puts("<array>");
    } else
    if (val_is_foreign(v)) {
        console_log("<object %s>", cupkee_name((void*) val_2_intptr(v)));
    } else {
        console_puts("<object>");
    }
}

static void print_object_value(val_t *o)
{
    object_t *obj = (object_t *) val_2_intptr(o);

    if (obj) {
        object_iter_t it;
        const char *k;
        val_t *v;
        int i = 0;

        console_puts("{");
        _object_iter_init(&it, obj);
        if (object_iter_next(&it, &k, &v)) {
            do {
                console_puts(i++ == 0 ? " " : ", ");
                console_puts(k);
                console_puts(": ");
                print_simple_value(v);
            } while(object_iter_next(&it, &k, &v));
        }
        if (i > 0) console_putc(' ');
        console_puts("}\r\n");
    } else {
        console_puts("null\r\n");
    }
}

static void print_array_value(val_t *v)
{
    array_t *array = (array_t *) val_2_intptr(v);
    int i, max;

    max = array_length(array);

    console_puts("[");
    for (i = 0; i < max; i++) {
        console_puts(i == 0 ? " " : ", ");
        print_simple_value(array_elem(array, i));
    }
    if (i > 0) console_putc(' ');

    console_puts("]\r\n");
}

void shell_print_value(val_t *v)
{
    if (val_is_array(v)) {
        print_array_value(v);
    } else
    if (val_is_object(v)) {
        print_object_value(v);
    } else {
        print_simple_value(v);
        console_puts("\r\n");
    }
}

void shell_reference_init(env_t *env)
{
    int i;

    for (i = 0; i < VARIABLE_REF_MAX; i++) {
        val_set_undefined(&reference_vals[i]);
    }
}

void shell_reference_gc(env_t *env)
{
    gc_types_copy(env, VARIABLE_REF_MAX, reference_vals);
}

val_t *shell_reference_create(val_t *v)
{
    int i;

    for (i = 0; i < VARIABLE_REF_MAX; i++) {
        val_t *r = reference_vals + i;

        if (val_is_undefined(r)) {
            *r = *v;
            return r;
        }
    }
    return NULL;
}

void shell_reference_release(val_t *ref)
{
    if (ref) {
        int pos = ref - reference_vals;

        if (pos >= 0 && pos < VARIABLE_REF_MAX) {
            val_set_undefined(ref);
        }
    }
}

val_t  *shell_reference_ptr(uint8_t id)
{
    if (id > 0 && id <= VARIABLE_REF_MAX) {
        return &reference_vals[id - 1];
    }
    return NULL;
}

void shell_print_error(int error)
{
    switch (error) {
    case ERR_NotEnoughMemory:   console_puts("Error: Not enought memory\r\n");      break;
    case ERR_NotImplemented:    console_puts("Error: Not implemented\r\n");         break;
    case ERR_StackOverflow:     console_puts("Error: Stack overflow\r\n");          break;
    case ERR_ResourceOutLimit:  console_puts("Error: Resource out of limit\r\n");   break;

    case ERR_InvalidToken:      console_puts("Error: Invalid Token\r\n");           break;
    case ERR_InvalidSyntax:     console_puts("Error: Invalid syntax\r\n");          break;
    case ERR_InvalidLeftValue:  console_puts("Error: Invalid left value\r\n");      break;
    case ERR_InvalidSementic:   console_puts("Error: Invalid Sementic\r\n");        break;

    case ERR_InvalidByteCode:   console_puts("Error: Invalid Byte code\r\n");       break;
    case ERR_InvalidInput:      console_puts("Error: Invalid input\r\n");           break;
    case ERR_InvalidCallor:     console_puts("Error: Invalid callor\r\n");          break;
    case ERR_NotDefinedId:      console_puts("Error: Not defined ID\r\n");          break;

    case ERR_SysError:          console_puts("Error: System error\r\n");            break;

    default: console_puts("Error: unknown error\r\n");
    }
}

val_t native_sysinfos(env_t *env, int ac, val_t *av)
{
    hw_info_t hw;

    (void) ac;
    (void) av;

    hw_info_get(&hw);

    console_log_sync("FREQ: %dM\r\n", hw.sys_freq / 1000000);
    console_log_sync("RAM: %dK\r\n", hw.ram_sz / 1024);
    console_log_sync("ROM: %dK\r\n\r\n", hw.rom_sz / 1024);

    console_log_sync("=============================\r\n");
    console_log_sync("Symbal: %d/%d, ", env->symbal_tbl_hold, env->symbal_tbl_size);
    console_log_sync("String: %d/%d, ", env->exe.string_num, env->exe.string_max);
    console_log_sync("Number: %d/%d, ", env->exe.number_num, env->exe.number_max);
    console_log_sync("Function: %d/%d, ", env->exe.func_num, env->exe.func_max);
    console_log_sync("Variable: %d\r\n", env->main_var_num);

    return val_mk_undefined();
}

val_t native_systicks(env_t *env, int ac, val_t *av)
{
    (void) env;
    (void) ac;
    (void) av;

    return val_mk_number(cupkee_systicks());
}

val_t native_print(env_t *env, int ac, val_t *av)
{
    int i;

    (void) env;

    if (ac) {
        for (i = 0; i < ac; i++) {
            shell_print_value(av+i);
        }
    }

    return VAL_UNDEFINED;
}

val_t native_erase(env_t *env, int ac, val_t *av)
{
    (void) env;
    (void) ac;
    (void) av;

    return cupkee_storage_erase(CUPKEE_STORAGE_BANK_APP) ? VAL_FALSE : VAL_TRUE;
}

