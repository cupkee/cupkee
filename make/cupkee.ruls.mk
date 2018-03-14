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


###############################################################################
# WRANING:
#
#   User should not modify anything there, unless you do known what your doing!
#
###############################################################################

# include board config files
ifeq (${CPU},)
	include ${MAKE_DIR}/cup/test.mk
else
ifneq (${wildcard ${MAKE_DIR}/cpu/${CPU}}.mk,)
	include ${MAKE_DIR}/cpu/${CPU}.mk
else
	$(error CPU: ${CPU}, is not support now)
endif
endif

###############################################################################
# Arch defines
include ${MAKE_DIR}/arch/${ARCH}.mk

###############################################################################
# Silent at default, but 'make V=1' will show all build calls.
ifneq ($(V),1)
Q		:= @
NULL 	:= 2>/dev/null
endif

###############################################################################
# Executables
AS  	:= ${PREFIX}as
CC 		:= ${PREFIX}gcc
LD  	:= ${PREFIX}gcc
AR  	:= ${PREFIX}ar
CXX 	:= $(PREFIX)g++
OBJCOPY	:= $(PREFIX)objcopy
OBJDUMP	:= $(PREFIX)objdump
GDB		:= $(PREFIX)gdb

RM 		:= rm -rf
MAKE 	:= make

OPT		:= -Os
#CSTD    ?= -std=c99


###############################################################################
# Assembler flags
ASFLAGS     += -D__ASSEMBLY__
ASFLAGS     += -D__NEWLIB__

###############################################################################
# C flags

TGT_CFLAGS		+= $(OPT) $(CSTD) -g
TGT_CFLAGS		+= $(ARCH_FLAGS)
TGT_CFLAGS		+= -Wextra -Wshadow -Wimplicit-function-declaration
TGT_CFLAGS		+= -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
TGT_CFLAGS		+= -fno-common -ffunction-sections -fdata-sections

###############################################################################
# C++ flags

TGT_CXXFLAGS	+= $(OPT) $(CXXSTD) -g
TGT_CXXFLAGS	+= $(ARCH_FLAGS)
TGT_CXXFLAGS	+= -Wextra -Wshadow -Wredundant-decls  -Weffc++
TGT_CXXFLAGS	+= -fno-common -ffunction-sections -fdata-sections

###############################################################################
# C & C++ preprocessor common flags

TGT_CPPFLAGS	+= -MD
TGT_CPPFLAGS	+= -Wall -Wundef
TGT_CPPFLAGS	+= $(DEFS) -I${INC_DIR}


###############################################################################
# Linker flags
TGT_LDFLAGS     += ${ARCH_LDFLAGS}
TGT_LDFLAGS		+= $(ARCH_FLAGS)

###############################################################################
# Build ruls

%.images: %.bin %.hex %.srec %.list %.map
	@printf "*** $* images generated ***\n"

%.bin: %.elf
	@printf "  OBJCOPY $(*).bin\n"
	$(Q)$(OBJCOPY) -Obinary $(*).elf $(*).bin

%.hex: %.elf
	@printf "  OBJCOPY $(*).hex\n"
	$(Q)$(OBJCOPY) -Oihex $(*).elf $(*).hex

%.srec: %.elf
	@printf "  OBJCOPY $(*).srec\n"
	$(Q)$(OBJCOPY) -Osrec $(*).elf $(*).srec

%.list: %.elf
	@printf "  OBJDUMP $(*).list\n"
	$(Q)$(OBJDUMP) -S $(*).elf > $(*).list


# Marco build_obj_rule [C file]
# param ${1}: source file
# param ${2}: program or library name
define build_obj_rule
${1:%.c=%.o}: ${1}
	@printf "[CC]\t$$@\n"
	$(Q)${CC} ${TGT_CPPFLAGS} ${${2}_CPPFLAGS} ${TGT_CFLAGS} ${${2}_CFLAGS} -MD -c $$< -o $$@
endef

# Marco build_obj_rule [S file]
# param ${1}: source file
# param ${2}: program or library name
define build_obj_rule_s
${1:%.S=%.o}: ${1}
	@printf "[AS]\t$$@\n"
	$(Q)${CC} ${ASFLAGS} ${TGT_CPPFLAGS} ${${2}_CPPFLAGS} ${TGT_CFLAGS} ${${2}_CFLAGS} -MD -c $$< -o $$@
endef

# Marco build_elf_rule
# param ${1}: target name
define build_elf_rule
${1}_OBJS = ${${1}_SRCS:%.c=%.o}
${1}_DEPS = ${${1}_SRCS:%.c=%.d}
${1}_OBJS += ${${1}_ASRCS:%.S=%.o}
${1}_DEPS += ${${1}_ASRCS:%.S=%.d}

${1}: ${1}.info ${1}.elf

ifeq (${PREFIX},)
${1}.elf: $${${1}_OBJS}
	@printf "[LD]\t$$@\n\n"
	$(Q)${LD} -o $$@ $${${1}_OBJS} ${TGT_CPPFLAGS} ${${1}_CPPFLAGS} $${${1}_LDFLAGS} ${TGT_LDFLAGS}
else
${1}.elf: $${${1}_OBJS}
	@printf "[LD]\t$$@\n\n"
	$(Q)${LD} -o $$@ $${${1}_OBJS} ${TGT_CPPFLAGS} ${${1}_CPPFLAGS} $${${1}_LDFLAGS} ${TGT_LDFLAGS} -Wl,-Map=${1}.map
endif


${1}.info:
	@printf "build ${1}.elf\n"

${1}.clean:
	$(Q)${RM} $${${1}_OBJS} $${${1}_DEPS} ${1}.*

$(foreach src,${${1}_SRCS},$(eval $(call build_obj_rule,${src},${1})))
$(foreach asm,${${1}_ASRCS},$(eval $(call build_obj_rule_s,${asm},${1})))

-include $${${1}_DEPS}
endef

# Marco build_lib_rule
# param ${1}: library name
define build_lib_rule
${1}_OBJS = ${${1}_SRCS:%.c=%.o}
${1}_DEPS = ${${1}_SRCS:%.c=%.d}
${1}_OBJS += ${${1}_ASRCS:%.S=%.o}
${1}_DEPS += ${${1}_ASRCS:%.S=%.d}

${1}: lib${1}.a

lib${1}.a: $${${1}_OBJS}
	@printf "[AR]\t$$@\n\n"
	$(Q)${AR} rcs lib${1}.a $${${1}_OBJS}

${1}.clean:
	$(Q)${RM} $${${1}_OBJS} $${${1}_DEPS} lib${1}.a

$(foreach src,${${1}_SRCS},$(eval $(call build_obj_rule,${src},${1})))
$(foreach asm,${${1}_ASRCS},$(eval $(call build_obj_rule_s,${asm},${1})))

-include $${${1}_DEPS}
endef

###############################################################################
###############################################################################
# Build target

all: lib elf

extend: lib elf bin hex list srec

lib: ${lib_NAMES}

elf: ${elf_NAMES}

bin: ${elf_NAMES} ${elf_NAMES:%=%.bin}

hex: ${elf_NAMES} ${elf_NAMES:%=%.hex}

srec: ${elf_NAMES} ${elf_NAMES:%=%.srec}

list: ${elf_NAMES} ${elf_NAMES:%=%.list}

clean: ${elf_NAMES:%=%.clean} ${lib_NAMES:%=%.clean}
	@${RM} ${elf_NAMES:%=%.bin}
	@${RM} ${elf_NAMES:%=%.hex}
	@${RM} ${elf_NAMES:%=%.srec}
	@${RM} ${elf_NAMES:%=%.list}

$(foreach elf,${elf_NAMES},$(eval $(call build_elf_rule,${elf})))
$(foreach lib,${lib_NAMES},$(eval $(call build_lib_rule,${lib})))


.PHONY: clean lib all images elf bin hex srec list

