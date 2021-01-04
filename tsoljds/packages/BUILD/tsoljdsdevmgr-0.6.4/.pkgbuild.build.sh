#!/bin/bash

set -e

RPM_SOURCE_DIR="/export/home/gfaden/packages/SOURCES"
RPM_BUILD_DIR="/export/home/gfaden/packages/BUILD"
RPM_OPT_FLAGS="-i -xO4 -xspace -xstrconst -xpentium -mr -xregs=no%frameptr ${EXTRA_CFLAGS}"
RPM_ARCH="i386"
RPM_OS="solaris"
RPM_OS_REL="5.11"
export RPM_SOURCE_DIR RPM_BUILD_DIR RPM_OPT_FLAGS RPM_ARCH RPM_OS
RPM_DOC_DIR="/usr/share/doc/packages/SUNWtgnome-tsoljdsdevmgr"
export RPM_DOC_DIR
RPM_PACKAGE_NAME="SUNWtgnome-tsoljdsdevmgr"
RPM_PACKAGE_VERSION="0.6.4"
RPM_PACKAGE_RELEASE="0"
export RPM_PACKAGE_NAME RPM_PACKAGE_VERSION RPM_PACKAGE_RELEASE
RPM_BUILD_ROOT="/var/tmp/pkgbuild-gfaden/SUNWtgnome-tsoljdsdevmgr-0.6.4-build"
export RPM_BUILD_ROOT

set -x
umask 022
uname -a

cd /export/home/gfaden/packages/BUILD

cd tsoljdsdevmgr-0.6.4

export ACLOCAL_FLAGS="-I /usr/share/aclocal"

libtoolize -f
intltoolize --copy --force --automake

aclocal $ACLOCAL_FLAGS
autoconf
autoheader
automake -acf

CFLAGS="-i -xO4 -xspace -xstrconst -xpentium -mr -xregs=no%frameptr ${EXTRA_CFLAGS} -D_POSIX_PTHREAD_SEMANTICS" \
./configure --with-gnome-prefix=/usr \
            --prefix=/usr \
            --sysconfdir=/etc \
	    --mandir=/usr/share/man

exit 0
