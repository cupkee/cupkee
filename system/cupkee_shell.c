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

#include <cupkee.h>

#include "cupkee_shell_util.h"
#include "cupkee_shell_sdmp.h"
#include "cupkee_shell_device.h"
#include "cupkee_sysdisk.h"

#define CONSOLE_INPUT_LINE      0
#define CONSOLE_INPUT_MULTI     1

static const char *logo = "\r\n\
    ______               __                  \r\n\
  /   ___ \\ __ ________ |  | __ ____   ____  \r\n\
 /    \\  \\/|  |  \\____ \\|  |/ // __ \\_/ __ \\ \r\n\
 \\     \\___|  |  /  |_> |    <\\  ___/\\  ___/ \r\n\
  \\________/____/|   __/|__|_ \\\\____> \\____>\r\n\
                 |__|        \\/ V" CUPKEE_VERSION "\r\n";

static void *core_mem_ptr;
static int   core_mem_sz;

static char *input_mem_ptr;
static int   input_mem_sz;
static int   input_cached;
static uint8_t   shell_console_mode;
static env_t shell_env;

static void shell_memory_location(int *heap_mem_sz, int *stack_mem_sz)
{
    void *memory;
    size_t size, block_size = 512;
    int core_blocks;

    size = 16 * 1024;
    memory = cupkee_malloc(size);
    if (!memory) {
        // memory not enought !
        hw_halt();
    }

    core_blocks = size / block_size - 2;

    core_mem_ptr = memory;
    core_mem_sz  = core_blocks * block_size;

    input_mem_ptr = memory + core_mem_sz;
    input_mem_sz  = size - core_mem_sz;

    *heap_mem_sz  = core_blocks * 3 / 7 * block_size;
    *stack_mem_sz = core_blocks / 8 * block_size;
}

static void shell_error_proc(env_t *env, int err)
{
    shell_print_error(err);

    // Todo: stack dump

    // resume env, after error occuried
    env_set_error(env, 0);
}

static int shell_do_complete(const char *sym, void *param)
{
    cupkee_auto_complete_update(param, sym);
    return 0;
}

static int shell_console_complete(void)
{
    // To used the free space of input buffer as auto_complete buffer
    void *buf = input_mem_ptr + input_cached;
    int   len = input_mem_sz - input_cached;
    void *ac;

    ac = cupkee_auto_complete_init(buf, len);
    if (ac) {
        env_symbal_foreach(&shell_env, shell_do_complete, ac);
        cupkee_auto_complete_finish(ac);
    }

    return CON_EXECUTE_DEF;
}

// Would be call, when more token need by parser
static char *shell_console_parser_cb(void)
{
    shell_console_mode = CONSOLE_INPUT_MULTI;
    return NULL;
}

static void shell_console_execute(env_t *env, int len, char *script)
{
    val_t *res;
    int    err;

    input_cached = 0;
    shell_console_mode = CONSOLE_INPUT_LINE;

    //console_log_sync("\r\nexecute: '%s'\r\n", script);

    err = interp_execute_interactive(env, script, shell_console_parser_cb, &res);
    //console_log_sync("state: %d\r\n", err);
    if (err > 0) {
        cupkee_history_push(len, script);

        if (res)
            shell_print_value(res);

        shell_console_mode = CONSOLE_INPUT_LINE;
    } else
    if (shell_console_mode == CONSOLE_INPUT_MULTI && (err == -ERR_InvalidToken || err == 0)) {
        input_cached = len;
        console_prompt_set(". ");
    } else
    if (err < 0) {
        shell_error_proc(env, -err);
    }
}

static int shell_console_load(void)
{
    int len;

    if (shell_console_mode == CONSOLE_INPUT_MULTI) {
        if (input_cached >= input_mem_sz) {
            console_puts("Warning! input over flow... \r\n");

            shell_console_mode = CONSOLE_INPUT_LINE;
            input_cached = 0;
            console_prompt_set(NULL);

            return CON_EXECUTE_DEF;
        }

        len = console_input_load(input_mem_sz - input_cached, input_mem_ptr + input_cached);
        if (len >= 1) {
            // load more
            input_cached += len;
            return CON_EXECUTE_DEF;
        } else {
            // load finish, if meet empty line
            len += input_cached;

            console_prompt_set(NULL);
        }
    } else {
        len = console_input_load(input_mem_sz, input_mem_ptr);
    }

    input_mem_ptr[len] = 0;

    shell_console_execute(&shell_env, len, input_mem_ptr);

    return CON_EXECUTE_DEF;
}

static int shell_console_handle(int type, int ch)
{
    (void) ch;


    if (type == CON_CTRL_ENTER) {
        return shell_console_load();
    } else
    if (type == CON_CTRL_TABLE) {
        return shell_console_complete();
    } else
    if (type == CON_CTRL_UP) {
        return cupkee_history_load(-1);
    } else
    if (type == CON_CTRL_DOWN) {
        return cupkee_history_load(1);
    }

    return CON_EXECUTE_DEF;
}

static void shell_console_init(void)
{
    cupkee_history_init();
    cupkee_console_init(shell_console_handle);

    input_cached = 0;
}

static void shell_env_callback(env_t *env, int event)
{
    if (event == PANDA_EVENT_GC_START) {
        shell_reference_gc(env);
    } else
    if (event == PANDA_EVENT_GC_END) {
    }
}

static void shell_interp_init(int heap_mem_sz, int stack_mem_sz, int n, const native_t *entrys)
{

    if(0 != interp_env_init_interactive(&shell_env, core_mem_ptr, core_mem_sz,
                NULL, heap_mem_sz, NULL, stack_mem_sz/ sizeof(val_t))) {
        hw_halt();
    }

    shell_reference_init(&shell_env);

    env_native_set(&shell_env, entrys, n);

    env_callback_set(&shell_env, shell_env_callback);

    shell_console_mode = CONSOLE_INPUT_LINE;
}

val_t foreign_set(void *env, val_t *self, val_t *data)
{
    void *entry = (void *)val_2_intptr(self);
    int retval = -1;

    (void) env;

    if (val_is_number(data)) {
        retval = cupkee_set(entry, CUPKEE_OBJECT_ELEM_INT, val_2_integer(data));
    } else
    if (val_is_string(data)) {
        retval = cupkee_set(entry, CUPKEE_OBJECT_ELEM_STR, (intptr_t)val_2_cstring(data));
    }

    if (retval < 0) {
        *self = *data;
    }

    return *data;
}

void foreign_keep(intptr_t entry)
{
    (void) entry;
}

int foreign_is_true(val_t *self)
{
    (void) self;
    return 0;
}

int foreign_is_equal(val_t *self, val_t *other)
{
    (void) self;
    (void) other;
    return 0;
}

double foreign_value_of(val_t *self)
{
    (void) self;
    return 0;
}

val_t foreign_get_prop(void *env, val_t *self, const char *key)
{
    void *entry = (void *)val_2_intptr(self);
    const cupkee_meta_t *meta;
    intptr_t v;
    int t;

    (void) env;

    meta = cupkee_meta(entry);
    if (meta && meta->prop_get) {
        val_t prop;
        if (meta->prop_get(entry, key, &prop) > 0) {
            return prop;
        }
    }

    t = cupkee_prop_get(entry, key, &v);
    switch (t) {
    case CUPKEE_OBJECT_ELEM_INT:
        return val_mk_number(v);
    case CUPKEE_OBJECT_ELEM_STR:
        return val_mk_foreign_string(v);
    case CUPKEE_OBJECT_ELEM_BOOL:
        return val_mk_boolean(v);
    case CUPKEE_OBJECT_ELEM_OCT: {
        uint8_t *ptr = (uint8_t *)v;
        uint8_t len = *ptr++;

        return val_mk_array(array_alloc_u8(env, len, ptr));
    }
    default:
        break;
    }
    return VAL_UNDEFINED;
}

val_t foreign_get_elem(void *env, val_t *self, int id)
{
    void *entry = (void *)val_2_intptr(self);
    intptr_t v;
    int t;

    (void) env;

    t = cupkee_elem_get(entry, id, &v);
    switch (t) {
    case CUPKEE_OBJECT_ELEM_INT:
        return val_mk_number(v);
    case CUPKEE_OBJECT_ELEM_STR:
        return val_mk_foreign_string(v);
    case CUPKEE_OBJECT_ELEM_BOOL:
        return val_mk_boolean(v);
    case CUPKEE_OBJECT_ELEM_OCT:
    default:
        break;
    }
    return VAL_UNDEFINED;
}

void foreign_set_prop(void *env, val_t *self, const char *key, val_t *data)
{
    void *entry = (void *)val_2_intptr(self);

    (void) env;

    if (val_is_number(data)) {
        cupkee_prop_set(entry, key, CUPKEE_OBJECT_ELEM_INT, val_2_integer(data));
    } else
    if (val_is_string(data)) {
        cupkee_prop_set(entry, key, CUPKEE_OBJECT_ELEM_STR, (intptr_t)val_2_cstring(data));
    } else
    if (val_is_array(data)){
        array_t *array = (array_t *)val_2_intptr(data);
        val_t   *elems = array_values(array);
        int n = array_length(array), i;

        for (i = 0; i < n; i++, elems++) {
            if (val_is_number(elems) && cupkee_prop_set(entry, key, CUPKEE_OBJECT_ELEM_INT, val_2_integer(elems)) < 1) {
                break;
            }
        }
    }
}

void foreign_set_elem(void *env, val_t *self, int id, val_t *data)
{
    void *entry = (void *)val_2_intptr(self);

    (void) env;

    if (val_is_number(data)) {
        cupkee_elem_set(entry, id, CUPKEE_OBJECT_ELEM_INT, val_2_integer(data));
    } else
    if (val_is_string(data)) {
        cupkee_elem_set(entry, id, CUPKEE_OBJECT_ELEM_STR, (intptr_t)val_2_cstring(data));
    }
}

void foreign_opx_prop(void *env, val_t *self, const char *key, val_t *res, val_opx_t op)
{
    (void) env;
    (void) self;
    (void) key;
    (void) res;
    (void) op;
}

void foreign_opx_elem(void *env, val_t *self, int id, val_t *res, val_opx_t op)
{
    (void) env;
    (void) self;
    (void) id;
    (void) res;
    (void) op;
}

void foreign_opxx_prop(void *env, val_t *self, const char *key, val_t *data, val_t *res, val_opxx_t op)
{
    (void) env;
    (void) self;
    (void) key;
    (void) data;
    (void) res;
    (void) op;
}

void foreign_opxx_elem(void *env, val_t *self, int id, val_t *data, val_t *res, val_opxx_t op)
{
    (void) env;
    (void) self;
    (void) id;
    (void) data;
    (void) res;
    (void) op;
}
env_t *cupkee_shell_env(void)
{
    return &shell_env;
}

int cupkee_shell_init(int n, const native_t *natives)
{
    int heap_mem_sz, stack_mem_sz;

    shell_console_init();

    shell_sdmp_init();

    shell_memory_location(&heap_mem_sz, &stack_mem_sz);

    shell_interp_init(heap_mem_sz, stack_mem_sz, n, natives);

    cupkee_shell_init_timer();
    cupkee_shell_init_device();

    console_puts_sync(logo);

    return 0;
}

int cupkee_shell_start(const char *initial)
{
    const char *app = cupkee_sysdisk_app();

    (void) initial;

    if (hw_boot_state() == HW_BOOT_STATE_PRODUCT && app) {
        val_t *res;

        if (0 > interp_execute_stmts(&shell_env, app, &res)) {
            console_log("execute app scripts fail..\r\n");
            return -1;
        }
        console_log("execute app scripts ok..\r\n");
    }

    return 0;
}

int cupkee_execute_string(const char *script, val_t **res)
{
    if (!res) {
        val_t *p;
        return interp_execute_stmts(&shell_env, script, &p);
    } else {
        return interp_execute_stmts(&shell_env, script, res);
    }
}

val_t cupkee_execute_function(val_t *fn, int ac, val_t *av)
{
    if (fn) {
        env_t *env = &shell_env;
        if (val_is_native(fn)) {
            function_native_t f = (function_native_t) val_2_intptr(fn);
            return f(env, ac, av);
        } else
        if (val_is_script(fn)){
            if (ac) {
                int i;
                for (i = ac - 1; i >= 0; i--)
                    env_push_call_argument(env, av + i);
            }
            env_push_call_function(env, fn);
            return interp_execute_call(env, ac);
        }
    }
    return VAL_UNDEFINED;
}

