/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016 Lixing Ding <ding.lixing@gmail.com>

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

#ifndef __HW_TIMER_INC__
#define __HW_TIMER_INC__

#define HW_TIMER_INSTANCE           4
#define HW_INSTANCES_PWM            HW_TIMER_INSTANCE
#define HW_INSTANCES_PULSE          HW_TIMER_INSTANCE
#define HW_INSTANCES_TIMER          HW_TIMER_INSTANCE
#define HW_INSTANCES_COUNTER        HW_TIMER_INSTANCE

void  hw_setup_timer(void);
const hw_driver_t *hw_request_pwm(int instance);
const hw_driver_t *hw_request_pulse(int instance);
const hw_driver_t *hw_request_timer(int instance);
const hw_driver_t *hw_request_counter(int instance);

#endif /* __HW_TIMER_INC__ */

