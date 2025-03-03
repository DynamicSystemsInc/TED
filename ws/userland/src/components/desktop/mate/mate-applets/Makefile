#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL)". You may
# only use this file in accordance with the terms of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source. A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#

#
# Copyright 2016 Till Wegmueller
# Copyright 2016 Ken Mays
# Copyright 2019 Michal Nowak
#

PREFERRED_BITS=		64

include ../../../../make-rules/shared-macros.mk

COMPONENT_NAME=		mate-applets
COMPONENT_MJR_VERSION=	1.22
COMPONENT_MNR_VERSION=	1
COMPONENT_VERSION=	$(COMPONENT_MJR_VERSION).$(COMPONENT_MNR_VERSION)
COMPONENT_PROJECT_URL=	http://www.mate-desktop.org
COMPONENT_SUMMARY=	Applets components for the MATE Desktop Environment
COMPONENT_SRC=		$(COMPONENT_NAME)-$(COMPONENT_VERSION)
COMPONENT_ARCHIVE=	$(COMPONENT_SRC).tar.xz
COMPONENT_ARCHIVE_HASH=	\
	sha256:dd653d5f14772e0d962a096036f0a0576285912815fd4c8c5028478082a852a7
COMPONENT_ARCHIVE_URL=	http://pub.mate-desktop.org/releases/$(COMPONENT_MJR_VERSION)/$(COMPONENT_ARCHIVE)
COMPONENT_LICENSE=	GPLv2, LGPLv2, FDLv1.1
COMPONENT_LICENSE_FILE=	$(COMPONENT_NAME).license
COMPONENT_CLASSIFICATION= System/Libraries
COMPONENT_FMRI=		desktop/mate/$(COMPONENT_NAME)

include $(WS_MAKE_RULES)/prep.mk
include $(WS_MAKE_RULES)/configure.mk
include $(WS_MAKE_RULES)/ips.mk

#PATH=$(PATH.gnu)

COMPONENT_PREP_ACTION =        cd $(@D) && NOCONFIGURE=1 ./autogen.sh

CONFIGURE_BINDIR.64 = $(CONFIGURE_BINDIR.32)

CONFIGURE_OPTIONS+= --sysconfdir=/etc
CONFIGURE_OPTIONS+= --libexecdir=$(CONFIGURE_LIBDIR.$(BITS))/mate
CONFIGURE_OPTIONS+= --disable-static
CONFIGURE_OPTIONS+= --localstatedir=/var/lib
CONFIGURE_OPTIONS+= --enable-stickynotes 

CONFIGURE_ENV+= PYTHON="$(PYTHON.3.5.64)"
CONFIGURE_ENV+= am_cv_python_pythondir="$(PYTHON.3.5.VENDOR_PACKAGES.64)"
CONFIGURE_ENV+= am_cv_python_pyexecdir="$(PYTHON.3.5.VENDOR_PACKAGES.64)"
CONFIGURE_ENV+= LIBS="-lnsl -lsocket"

build:		$(BUILD_64)

install:	$(INSTALL_64)

test:		$(NO_TESTS)

REQUIRED_PACKAGES += desktop/character-map/gucharmap
REQUIRED_PACKAGES += library/python/pygobject-3-27
# Auto-generated dependencies
REQUIRED_PACKAGES += desktop/mate/mate-panel
REQUIRED_PACKAGES += library/desktop/atk
REQUIRED_PACKAGES += library/desktop/cairo
REQUIRED_PACKAGES += library/desktop/gdk-pixbuf
REQUIRED_PACKAGES += library/desktop/gtk3
REQUIRED_PACKAGES += library/desktop/gtksourceview3
REQUIRED_PACKAGES += library/desktop/libwnck3
REQUIRED_PACKAGES += library/desktop/mate/libmateweather
REQUIRED_PACKAGES += library/desktop/pango
REQUIRED_PACKAGES += library/glib2
REQUIRED_PACKAGES += library/libgtop
REQUIRED_PACKAGES += library/libnotify
REQUIRED_PACKAGES += library/libxml2
REQUIRED_PACKAGES += system/library
REQUIRED_PACKAGES += system/library/math
REQUIRED_PACKAGES += x11/library/libx11
