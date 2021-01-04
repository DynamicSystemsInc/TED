#!/bin/bash

set -e

RPM_SOURCE_DIR="/export/home/gfaden/packages/SOURCES"
RPM_BUILD_DIR="/export/home/gfaden/packages/BUILD"
RPM_OPT_FLAGS="-i -xO4 -xspace -xstrconst -xpentium -mr -xregs=no%frameptr ${EXTRA_CFLAGS}"
RPM_ARCH="i386"
RPM_OS="solaris"
RPM_OS_REL="5.11"
export RPM_SOURCE_DIR RPM_BUILD_DIR RPM_OPT_FLAGS RPM_ARCH RPM_OS
RPM_DOC_DIR="/usr/share/doc/packages/SUNWtgnome-tsol-libs"
export RPM_DOC_DIR
RPM_PACKAGE_NAME="SUNWtgnome-tsol-libs"
RPM_PACKAGE_VERSION="0.6.2"
RPM_PACKAGE_RELEASE="0"
export RPM_PACKAGE_NAME RPM_PACKAGE_VERSION RPM_PACKAGE_RELEASE
RPM_BUILD_ROOT="/var/tmp/pkgbuild-gfaden/SUNWtgnome-tsol-libs-0.6.2-build"
export RPM_BUILD_ROOT

set -x
umask 022
uname -a

cd /export/home/gfaden/packages/BUILD

cd libgnometsol-0.6.2

make install DESTDIR=$RPM_BUILD_ROOT
rm $RPM_BUILD_ROOT/usr/lib/libgnometsol.la
rm $RPM_BUILD_ROOT/usr/lib/libgnometsol.a

cd /export/home/gfaden/packages/BUILD/libgnometsol-0.6.2
bash -x /export/home/gfaden/packages/SOURCES/updatei18n_ips0.sh ${RPM_BUILD_ROOT}/usr/share libgnometsol

exit 0
