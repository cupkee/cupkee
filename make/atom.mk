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

ifeq (${MAIN_DIR},)
MAIN_DIR = ${FRAMEWORK_DIR}/atom
endif
include ${MAKE_DIR}/cupkee.modules.mk

elf_NAMES = cupkee
cupkee_SRCS = ${notdir ${wildcard ${MAIN_DIR}/*.c}}

cupkee_CPPFLAGS = -I${INC_DIR} -I${LANG_DIR}/include ${mod_INC}
cupkee_CFLAGS   =
cupkee_LDFLAGS  = -L${BUILD_DIR} -L${BSP_BUILD_DIR} -L${SYS_BUILD_DIR} -L${LANG_BUILD_DIR} \
				  -lsys -lbsp -llang -lm ${mod_LDFLAGS}

include ${MAKE_DIR}/cupkee.ruls.mk

VPATH = ${MAIN_DIR}
