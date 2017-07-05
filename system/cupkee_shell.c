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
#include "cupkee_sysdisk.h"

static const char *logo = "\r\n\
    ______               __                  \r\n\
  /   ___ \\ __ ________ |  | __ ____   ____  \r\n\
 /    \\  \\/|  |  \\____ \\|  |/ // __ \\_/ __ \\ \r\n\
 \\     \\___|  |  /  |_> |    <\\  ___/\\  ___/ \r\n\
  \\________/____/|   __/|__|_ \\\\____> \\____>\r\n\
                 |__|        \\/ www.cupkee.cn\r\n";

static void *core_mem_ptr;
static int   core_mem_sz;

static char *input_mem_ptr;
static int   input_mem_sz;
static int   input_cached;
static uint8_t   shell_mode;
static uint8_t   shell_logo_show;
static env_t shell_env;

static void shell_memory_location(int *heap_mem_sz, int *stack_mem_sz)
{
    void *memory;
    size_t size, block_size = 512;
    int core_blocks;

    size = hw_malloc_all(&memory, 16);
    if (size < 1024 * 8) {
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

static int shell_do_complete(const char *sym, void *param)
{
    cupkee_auto_complete_update(param, sym);
    return 0;
}

static int shell_auto_complete(void)
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

static char *shell_parser_cb(void)
{
    shell_mode = 1;
    return NULL;
}

static void shell_error_proc(env_t *env, int err)
{
    shell_print_error(err);

    // Todo: stack dump

    env_set_error(env, 0);
}

static void shell_execute_input(env_t *env, int len, char *script)
{
    val_t *res;
    int    err;

    err = interp_execute_interactive(env, script, shell_parser_cb, &res);
    if (err < 0) {
        if (shell_mode == 0 || err != -ERR_InvalidToken) {
            shell_mode = 0;
            shell_error_proc(env, -err);
        }
    } else
    if (err > 0) {
        if (res) shell_print_value(res);

        shell_mode = 0;
        input_cached = 0;
        cupkee_history_push(len, script);
    }

    if (shell_mode == 1) {
        input_cached = len;
        console_prompt_set(". ");
    }
}

static int shell_execute(void)
{
    int len;

    if (shell_mode != 0) {
        if (input_cached >= input_mem_sz) {
            console_puts("Warning! input over flow... \r\n");

            shell_mode = 0;
            input_cached = 0;
            console_prompt_set(NULL);
            return CON_EXECUTE_DEF;
        }

        len = console_input_load(input_mem_sz - input_cached, input_mem_ptr + input_cached);
        if (len > 1) {
            input_cached += len;
            return CON_EXECUTE_DEF;
        } else {
            // input end
            len += input_cached;
            console_prompt_set(NULL);
        }
    } else {
        len = console_input_load(input_mem_sz, input_mem_ptr);
    }

    input_mem_ptr[len] = 0;

    shell_execute_input(&shell_env, len, input_mem_ptr);

    return CON_EXECUTE_DEF;
}

static int shell_console_handle(int type, int ch)
{
    (void) ch;

    if (!shell_logo_show) {
        shell_logo_show = 1;
        console_puts_sync(logo);
    }

    if (type == CON_CTRL_ENTER) {
        return shell_execute();
    } else
    if (type == CON_CTRL_TABLE) {
        return shell_auto_complete();
    } else
    if (type == CON_CTRL_UP) {
        return cupkee_history_load(-1);
    } else
    if (type == CON_CTRL_DOWN) {
        return cupkee_history_load(1);
    }

    return CON_EXECUTE_DEF;
}

static void shell_console_init(cupkee_device_t *tty)
{
    if (!tty) {
        hw_halt();
    }
    cupkee_device_enable(tty);

    cupkee_history_init();
    cupkee_console_init(tty, shell_console_handle);

    input_cached = 0;
    shell_logo_show = 0;
}

static void shell_interp_init(int heap_mem_sz, int stack_mem_sz, int n, const native_t *entrys)
{

    if(0 != interp_env_init_interactive(&shell_env, core_mem_ptr, core_mem_sz,
                NULL, heap_mem_sz, NULL, stack_mem_sz/ sizeof(val_t))) {
        hw_halt();
    }

    shell_reference_init(&shell_env);

    env_native_set(&shell_env, entrys, n);

    shell_mode = 0;
}

env_t *cupkee_shell_env(void)
{
    return &shell_env;
}

int cupkee_shell_init(cupkee_device_t *tty, int n, const native_t *natives)
{
    int heap_mem_sz, stack_mem_sz;

    shell_console_init(tty);

    shell_memory_location(&heap_mem_sz, &stack_mem_sz);

    shell_interp_init(heap_mem_sz, stack_mem_sz, n, natives);

    return 0;
}

int cupkee_shell_start(const char *initial)
{
    const char *app = cupkee_sysdisk_app_script();
    val_t *res;
    int   err = 0;

    (void) initial;

    if (app) {
        err = interp_execute_stmts(&shell_env, app, &res);
        //console_log("\r\nrun app: %s\r\n", err < 0 ? "fail" : "ok");
    }

    return err;
}

int cupkee_execute_function(val_t *fn, int ac, val_t *av)
{
    if (fn) {
        env_t *env = &shell_env;
        if (val_is_native(fn)) {
            function_native_t f = (function_native_t) val_2_intptr(fn);
            f(env, ac, av);
        } else
        if (val_is_script(fn)){
            if (ac) {
                int i;
                for (i = ac - 1; i >= 0; i--)
                    env_push_call_argument(env, av + i);
            }
            env_push_call_function(env, fn);
            interp_execute_call(env, ac);
        }
    }
}
