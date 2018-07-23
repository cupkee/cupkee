/* GPLv2 License
 *
 * Copyright (C) 2018 Lixing Ding <ding.lixing@gmail.com>
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


#include <stdio.h>
#include <stdint.h>

#include "gconvert.h"

static const uint64_t digit_scal[16] = {
    1,
    10,
    100,
    1000,
    10000,

    100000,
    1000000,
    10000000,
    100000000,
    1000000000,

    10000000000,
    100000000000,
    1000000000000,
    10000000000000,
    100000000000000,

    1000000000000000,
};

char *gconvert(double f, size_t ndigit, char * buf)
{
    uint64_t k, z, scal;
    char *c = buf;
    int pos = 0, n;

    if((int)ndigit == -1)
        ndigit = 5;

    /* Unsigned long long only allows for 20 digits of precision
    *  which is already more than double supports, so we limit the
    *  digits to 15.  long double might require an increase if it is ever
    *  implemented.
    */
    if (ndigit > 15)
        ndigit = 15;

    if (!f) {
        buf[0] = '0';
        buf[1] = 0;
        return c;
    }

    if (f < 0.0) {
        buf[pos++] = '-';
        f = -f;
    }
    scal = digit_scal[ndigit];
    k = f + 0.1 / scal;
    z = (f - k) * scal + 0.5;

    n = 1;
    while (digit_scal[n] <= k) {
        ++n;
    }
    while (n) {
        uint64_t base = digit_scal[n - 1];

        buf[pos++] = '0' + k / base;
        k = k % base;

        --n;
    }

    if (!z) {
        buf[pos] = 0;
        return c;
    } else {
        buf[pos++] = '.';
    }

    n = ndigit;
    while (z && n) {
        uint64_t base = digit_scal[n - 1];
        buf[pos++] = '0' + z / base;
        z = z % base;

        --n;
    }
    buf[pos++] = 0;

    return c;
}

