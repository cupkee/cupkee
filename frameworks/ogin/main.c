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

#include "board.h"

#ifndef COMMAND_BUF_SIZE
#define COMMAND_BUF_SIZE    80
#endif

static char command_buf[COMMAND_BUF_SIZE];

int main(void)
{
    void *tty;

    cupkee_init();


#ifdef USE_USB_CONSOLE
    tty = cupkee_device_request("usb-cdc", 0);
#else
    tty = cupkee_device_request("uart", 0);
#endif
    cupkee_device_enable(tty);

    cupkee_command_init(board_commands(), board_command_entries(),
                        COMMAND_BUF_SIZE, command_buf);
    cupkee_history_init();
    cupkee_console_init(tty, cupkee_command_handle);

    /**********************************************************
     * app device create & setup
     *********************************************************/
    board_setup();

    /**********************************************************
     * Let's Go!
     *********************************************************/
    cupkee_loop();

    /**********************************************************
     * Should not go here, make gcc happy!
     *********************************************************/
    return 0;
}

