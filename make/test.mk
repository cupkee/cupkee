##
## MIT License
##
## This file is part of cupkee project.
##
## Copyright (c) 2016 Lixing Ding <ding.lixing@gmail.com>
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
##

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
