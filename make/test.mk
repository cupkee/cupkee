## GPLv2 License
##
## Copyright (C) 2016-2018 Lixing Ding <ding.lixing@gmail.com>
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

elf_NAMES = test
test_SRCS = ${notdir ${wildcard ${BASE_DIR}/test/*.c}}
test_SRCS +=	CUnit_Basic.c \
				CUnit_Error.c \
				CUnit_Mem.c \
				CUnit_TestDB.c \
				CUnit_TestRun.c \
				CUnit_Util.c \

test_CPPFLAGS = -I${INC_DIR} -I${LANG_DIR}/include
test_CPPFLAGS += -I${TST_DIR}/cunit -I${BSP_DIR}/test

test_CFLAGS   =
test_LDFLAGS  = -L${BSP_BUILD_DIR} -L${SYS_BUILD_DIR} -L${LANG_BUILD_DIR} -lsys -llang

include ${MAKE_DIR}/cupkee.ruls.mk

VPATH = ${BASE_DIR}/test:${BASE_DIR}/test/cunit
