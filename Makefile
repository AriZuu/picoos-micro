#
# Copyright (c) 2012-2013, Ari Suutari <ari@stonepile.fi>.
# All rights reserved. 
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. The name of the author may not be used to endorse or promote
#     products derived from this software without specific prior written
#     permission. 
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT,  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Compile micro-layer library using Pico]OS library Makefile
#

RELROOT = ../picoos/
PORT ?= lpc2xxx
BUILD ?= RELEASE

include $(RELROOT)make/common.mak

TARGET = picoos-micro

SRC_TXT =	$(wildcard *.c) \
                newlib/syscalls.c \
                newlib/mwrap.c \
                fat/src/ff.c fat/src/option/ccsbcs.c

SRC_HDR =	$(wildcard *.h)
SRC_OBJ =
CDEFINES += 
DIR_USRINC += $(PORT)/default fat/src

-include ports/$(PORT)/Makefile

# 
# Set output directory.
#
ifeq '$(strip $(DIR_OUTPUT))' ''
DIR_OUTPUT = $(CURRENTDIR)/bin
endif

include $(MAKE_LIB)

.PHONY: dist dox

#
# Extra stuff to build distribution zip.
#
dist:
	rm -f ../dist/picoos-micro-`date +%Y%m%d`.zip
	cd ..; zip -qr dist/picoos-micro-`date +%Y%m%d`.zip picoos-micro -x "*/.*" "*/bin/*"

dox: doxygen.cfg
	mkdir -p doc
	doxygen doxygen.cfg
