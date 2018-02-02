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

#ifndef __CUPKEE_ERRNO_INC__
#define __CUPKEE_ERRNO_INC__

/************************************************************************
 * Cupkee error code
 ***********************************************************************/
#define CUPKEE_OK               0       // no error
#define CUPKEE_ERROR            1       // error
#define CUPKEE_EIMPLEMENT       2       // not implemented
#define CUPKEE_EINVAL           3       // invalid argument
#define CUPKEE_ELIMIT           4       // out of limit
#define CUPKEE_EEMPTY           5       // buffer is empty
#define CUPKEE_EOVERFLOW        6       // buffer is overflow
#define CUPKEE_ERESOURCE        7       // not enought resource
#define CUPKEE_ENOMEM           8       // out of memory
#define CUPKEE_ETIMEOUT         9       // time out
#define CUPKEE_EHARDWARE        10      // hardware error
#define CUPKEE_EBUSY            11      // busy

#define CUPKEE_ENAME            16      // invalid device name
#define CUPKEE_EENABLED         17      // config set for device that already enabled
#define CUPKEE_ENOTENABLED      18      // write & read device that not enabled
#define CUPKEE_ESETTINGS        20      // invalid settings

#endif /* __CUPKEE_ERRNO_INC__ */

