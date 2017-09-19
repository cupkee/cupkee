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

#include "board.h"

int main(void)
{
    void *tty;

    /**********************************************************
     * Cupkee system initial
     *********************************************************/
    cupkee_init();

#ifdef USE_USB_CONSOLE
    tty = cupkee_device_request("usb-cdc", 0);
#else
    tty = cupkee_device_request("uart", 0);
#endif
    if (cupkee_device_enable(tty)) {
        hw_halt();
    }

    cupkee_shell_init(tty, board_native_number(), board_native_entries());

    /**********************************************************
     * user setup code
     *********************************************************/
    board_setup();

    /**********************************************************
     * Let's Go!
     *********************************************************/
    cupkee_shell_loop(board_initial_script());

    /**********************************************************
     * Let's Go!
     *********************************************************/
    return 0;
}

