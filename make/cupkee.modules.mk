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

MOD_DIR = ${MAIN_DIR}/modules

mod_NAMES = ${notdir ${wildcard ${MOD_DIR}/*}}
mod_LDFLAGS = $(addprefix -l, ${mod_NAMES})
mod_INC = $(addprefix -I${MOD_DIR}/, ${mod_NAMES})

# ${info modules:${mod_LDFLAGS} ${mod_INC}}

# Marco build_mod_rule
# param ${1}: module name
define build_mod_rule
DUMMY = $(shell mkdir -p ${BUILD_DIR}/modules/${1}/lib)
${1}_SRCS = ${addprefix modules/${1}/lib/, ${notdir ${wildcard ${MAIN_DIR}/modules/${1}/lib/*.c}}}
${1}_CPPFLAGS = -I${INC_DIR} -I${LANG_DIR}/include
${1}_CFLAGS   =
endef

$(foreach mod,${mod_NAMES},$(eval $(call build_mod_rule,${mod})))

lib_NAMES = ${mod_NAMES}

