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

#include <cupkee.h>

#include "cupkee_shell_misc.h"

#define VARIABLE_REF_MAX (32)
#if     VARIABLE_REF_MAX > 255
#error "VARIABLE_REF_MAX should not big than 255"
#endif

static val_t reference_vals[VARIABLE_REF_MAX];

static inline void print_number(val_t *v) {
    char buf[32];

    if (*v & 0xffff) {
        snprintf(buf, 32, "%f\r\n", val_2_double(v));
    } else {
        snprintf(buf, 32, "%lld\r\n", (int64_t)val_2_double(v));
    }
    console_puts(buf);
}

static inline void print_boolean(val_t *v) {
    console_puts(val_2_intptr(v) ? "true\r\n" : "false\r\n");
}

static inline void print_string(val_t *v) {
    console_puts("\"");
    console_puts(val_2_cstring(v));
    console_puts("\"\r\n");
}

void print_simple_value(val_t *v)
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
        console_puts("undefined\r\n");
    } else
    if (val_is_nan(v)) {
        console_puts("NaN\r\n");
    } else
    if (val_is_function(v)) {
        console_puts("<function>\r\n");
    } else
    if (val_is_object(v)) {
        console_puts("<object>\r\n");
    } else
    if (val_is_buffer(v)) {
        console_puts("<buffer>\r\n");
    } else
    if (val_is_array(v)) {
        console_puts("<array>\r\n");
    } else {
        console_puts("<object>\r\n");
    }
}

static void print_object_value(val_t *o)
{
    object_t *obj = (object_t *) val_2_intptr(o);
    object_iter_t it;
    const char *k;
    val_t *v;

    console_puts("{");

    _object_iter_init(&it, obj);
    if (object_iter_next(&it, &k, &v)) {
        console_puts("\r\n");
        do {
            console_puts("  ");
            console_puts(k);
            console_puts(": ");
            print_simple_value(v);
        }while(object_iter_next(&it, &k, &v));
    }
    console_puts("}\r\n");
}

static void print_array_value(val_t *v)
{
    array_t *array = (array_t *) val_2_intptr(v);
    int i, max;

    max = array_len(array);

    if (max == 0) {
        console_puts("[]\r\n");
        return;
    }
    console_puts("[\r\n");
    for (i = 0; i < max; i++) {
        char buf[16];
        snprintf(buf, 16, "  [%2d]:", i);
        console_puts(buf);

        print_simple_value(_array_elem(array, i));
    }
    console_puts("]\r\n");
}

static void print_buffer_value(val_t *v)
{
    int   i, len = _val_buffer_size(v);
    uint8_t *ptr = _val_buffer_addr(v);
    char buf[16];

    snprintf(buf, 16, "<Buffer[%d]:", len);
    console_puts(buf);
    for (i = 0; i < len && i < 8; i++) {
        snprintf(buf, 16, " %.2x", ptr[i]);
        console_puts(buf);
    }

    if (i < len) {
        console_puts(" ...>\r\n");
    } else {
        console_puts(">\r\n");
    }
}

void shell_print_value(val_t *v)
{
    if (val_is_array(v)) {
        print_array_value(v);
    } else
    if (val_is_object(v)) {
        print_object_value(v);
    } else
    if (val_is_buffer(v)) {
        print_buffer_value(v);
    } else {
        print_simple_value(v);
    }
}

void shell_reference_init(env_t *env)
{
    int i;

    for (i = 0; i < VARIABLE_REF_MAX; i++) {
        val_set_undefined(&reference_vals[i]);
    }

    env_reference_set(env, reference_vals, VARIABLE_REF_MAX);
}

val_t *cupkee_shell_reference_create(val_t *v)
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

void cupkee_shell_reference_release(val_t *ref)
{
    if (ref) {
        int pos = ref - reference_vals;

        if (pos >= 0 && pos < VARIABLE_REF_MAX) {
            val_set_undefined(ref);
        }
    }
}

uint8_t shell_reference_id(val_t *ref)
{
    if (ref) {
        int pos = ref - reference_vals;

        if (pos >= 0 && pos < VARIABLE_REF_MAX) {
            return pos + 1;
        }
    }

    return 0;
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

void shell_do_callback(env_t *env, val_t *cb, uint8_t ac, val_t *av)
{
    if (!cb) return;

    if (val_is_native(cb)) {
        function_native_t fn = (function_native_t) val_2_intptr(cb);
        fn(env, ac, av);
    } else
    if (val_is_script(cb)){
        if (ac) {
            int i;
            for (i = ac - 1; i >= 0; i--)
                env_push_call_argument(env, av + i);
        }
        env_push_call_function(env, cb);
        interp_execute_call(env, ac);
    }
}

val_t shell_error(env_t *env, int code)
{
    (void) env;

    return val_mk_number(code);
}

void shell_do_callback_error(env_t *env, val_t *cb, int code)
{
    val_t err = shell_error(env, code);

    shell_do_callback(env, cb, 1, &err);
}

void shell_do_callback_buffer(env_t *env, val_t *cb, type_buffer_t *buffer)
{
    val_t args[2];

    val_set_undefined(args);
    val_set_buffer(args + 1, buffer);

    shell_do_callback(env, cb, 2, args);
}

int shell_str_id(const char *s, int max, const char * const *names)
{
    if (s) {
        int id;
        for (id = 0; id < max && names[id]; id++) {
            if (!strcmp(s, names[id])) {
                return id;
            }
        }
    }
    return -1;
}

int shell_val_id(val_t *v, int max, const char * const *names)
{
    if (val_is_number(v)) {
        return val_2_integer(v);
    } else {
        return shell_str_id(val_2_cstring(v), max, names);
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

val_t native_report(env_t *env, int ac, val_t *av)
{
    uint8_t state_id;

    (void) env;

    if (ac > 0 && val_is_number(av)) {
        state_id = val_2_integer(av);
    } else {
        return VAL_FALSE;
    }

    if (ac > 1) {
        ++av;

        if (val_is_boolean(av)) {
            return cupkee_sdmp_update_state_boolean(state_id, val_is_true(av)) ? VAL_FALSE : VAL_TRUE;
        } else
        if (val_is_number(av)) {
            return cupkee_sdmp_update_state_number(state_id, val_2_double(av)) ? VAL_FALSE : VAL_TRUE;
        } else {
            const char *s = val_2_cstring(av);

            if (s) {
                return cupkee_sdmp_update_state_string(state_id, s) ? VAL_FALSE : VAL_TRUE;
            } else {
                return VAL_FALSE;
            }
        }
    } else {
        return cupkee_sdmp_update_state_trigger(state_id) ? VAL_FALSE : VAL_TRUE;
    }
}


/* PIN */
val_t native_pin_enable(env_t *env, int ac, val_t *av)
{
    int pin, dir;
    const char *str;
    (void) env;

    if (ac < 1 || !val_is_number(av)) {
        return VAL_FALSE;
    }
    pin  = val_2_integer(av);

    if (ac > 2 && val_is_number(av + 1) && val_is_number(av + 2)) {
        int bank = val_2_integer(av + 1);
        int port = val_2_integer(av + 2);

        if (0 != cupkee_pin_map(pin, bank, port)) {
            return VAL_FALSE;
        }
        ac -= 2; av += 2;
    }

    dir = HW_DIR_OUT;  // default is output
    if (ac > 1) {
        str = val_2_cstring(av + 1);
        if (str) {
            if (!strcmp(str, "in")) {
                dir = HW_DIR_IN;
            } else
            if (!strcmp(str, "duplex")) {
                dir = HW_DIR_DUPLEX;
            }
        }
    }

    if (CUPKEE_OK == cupkee_pin_enable(pin, dir)) {
        return VAL_TRUE;
    } else {
        return VAL_FALSE;
    }
}

val_t native_pin(env_t *env, int ac, val_t *av)
{
    (void) env;

    if (ac > 0 && val_is_number(av)) {
        int pin = val_2_integer(av);

        if (ac > 1) {
            return cupkee_pin_set(pin, val_is_true(av + 1)) > 0 ? VAL_TRUE : VAL_FALSE;
        } else {
            return val_mk_number(cupkee_pin_get(pin));
        }
    }

    return VAL_UNDEFINED;
}

val_t native_pin_toggle(env_t *env, int ac, val_t *av)
{
    (void) env;

    if (ac > 0 && val_is_number(av)) {
        return cupkee_pin_toggle(val_2_integer(av)) == 0 ? VAL_TRUE : VAL_FALSE;
    }

    return VAL_UNDEFINED;
}

/* pin group */
static void grp_op_set(void *env, intptr_t p, val_t *val, val_t *res)
{
    uint32_t v;

    (void) env;

    if (val_is_number(val)) {
        v = val_2_integer(val);
    } else
    if (val_is_true(val)) {
        v = -1;
    } else {
        v = 0;
    }

    *res = val_mk_number(v);

    cupkee_pin_group_set((void*)p, v);
}

static void grp_elem_get(void *env, intptr_t p, val_t *k, val_t *elem)
{
    int v;

    (void) env;

    if (p && val_is_number(k)) {
        v = cupkee_pin_group_elem_get((void*)p, val_2_integer(k));
        if (v >= 0) {
            val_set_number(elem, v);
            return;
        }
    }
    val_set_undefined(elem);
}

static void grp_elem_set(void *env, intptr_t p, val_t *k, val_t *v)
{
    (void) env;

    if (p && val_is_number(k)) {
        cupkee_pin_group_elem_set((void*)p, val_2_integer(k), val_is_true(v));
    }
}

static const val_foreign_op_t grp_op = {
    .set  = grp_op_set,
    .elem_get = grp_elem_get,
    .elem_set = grp_elem_set,
};

val_t native_pin_group(env_t *env, int ac, val_t *av)
{
    void *grp = cupkee_pin_group_create();
    uint8_t i;

    if (!grp) {
        return VAL_UNDEFINED;
    }

    for (i = 0; i < ac; i++) {
        if (val_is_number(av + i)) {
            uint8_t pin = val_2_integer(av + i);
            cupkee_pin_group_push(grp, pin);
        }
    }

    if (cupkee_pin_group_size(grp) > 0) {
        return val_create(env, &grp_op, (intptr_t)grp);
    } else {
        cupkee_pin_group_destroy(grp);
        return VAL_FALSE;
    }
}

