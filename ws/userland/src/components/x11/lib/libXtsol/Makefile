###############################################################################
#
# libXtsol - client library for TSOL extension for Solaris Trusted Extensions 
#
# Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
#
BUILD_BITS= 64
include ../../../../make-rules/shared-macros.mk

COMPONENT_NAME=         libXtesol
COMPONENT_VERSION=      #
COMPONENT_SRC=          sun-src
COMPONENT_ARCHIVE=
COMPONENT_ARCHIVE_SRC=  none
COMPONENT_BUGDB=        x11/libraries
COMPONENT_ANITYA_ID=    N/A

# nothing to download ; just build & package
BUILD_STYLE= justmake
TEST_TARGET= $(NO_TESTS)

include $(WS_MAKE_RULES)/x11-component.mk

COMPONENT_INSTALL_ENV += INSTALL="$(INSTALL)"
COMPONENT_INSTALL_ENV += libdir="$(USRLIB)"

REQUIRED_PACKAGES += x11/library/libx11
