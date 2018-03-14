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

