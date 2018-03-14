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



LDSCRIPT    = ${MAKE_DIR}/ld/$(MCU).ld

ARCH_FLAGS	= -mthumb -mcpu=cortex-m3 -msoft-float -mfix-cortex-m3-ldrd

OPENCM3_DIR = ${BASE_DIR}/share/libopencm3
OPENCM3_LIB = opencm3_stm32f1

DEFS		+= -DSTM32F1
DEFS		+= -I$(OPENCM3_DIR)/include

ARCH_LDFLAGS += -T$(LDSCRIPT) -L$(OPENCM3_DIR)/lib -l$(OPENCM3_LIB)
ARCH_LDFLAGS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group
ARCH_LDFLAGS += --static -nostartfiles
ARCH_LDFLAGS += -Wl,--gc-sections
ifeq ($(V),99)
ARCH_LDFLAGS += -Wl,--print-gc-sections
endif

PREFIX  ?= arm-none-eabi-

