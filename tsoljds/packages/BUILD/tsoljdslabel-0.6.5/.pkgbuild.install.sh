#!/bin/bash

set -e

RPM_SOURCE_DIR="/export/home/gfaden/packages/SOURCES"
RPM_BUILD_DIR="/export/home/gfaden/packages/BUILD"
RPM_OPT_FLAGS="-i -xO4 -xspace -xstrconst -xpentium -mr -xregs=no%frameptr ${EXTRA_CFLAGS}"
RPM_ARCH="i386"
RPM_OS="solaris"
RPM_OS_REL="5.11"
export RPM_SOURCE_DIR RPM_BUILD_DIR RPM_OPT_FLAGS RPM_ARCH RPM_OS
RPM_DOC_DIR="/usr/share/doc/packages/SUNWtgnome-tsoljdslabel"
export RPM_DOC_DIR
RPM_PACKAGE_NAME="SUNWtgnome-tsoljdslabel"
RPM_PACKAGE_VERSION="0.6.5"
RPM_PACKAGE_RELEASE="0"
export RPM_PACKAGE_NAME RPM_PACKAGE_VERSION RPM_PACKAGE_RELEASE
RPM_BUILD_ROOT="/var/tmp/pkgbuild-gfaden/SUNWtgnome-tsoljdslabel-0.6.5-build"
export RPM_BUILD_ROOT

set -x
umask 022
uname -a

cd /export/home/gfaden/packages/BUILD

cd tsoljdslabel-0.6.5

make install DESTDIR=$RPM_BUILD_ROOT
LANG_LIST='
ar ar_EG.UTF-8
cs_CZ.ISO8859-2
da_DK.ISO8859-15
de.UTF-8 de
el_GR.ISO8859-7
en_GB.ISO8859-15 en_IE.ISO8859-15 en_US.ISO8859-15 en_US.UTF-8
es.UTF-8 es
et_EE.ISO8859-15
fi_FI.ISO8859-15
fr.UTF-8 fr
he he_IL.UTF-8
hi_IN.UTF-8
hr_HR.ISO8859-2
hu hu_HU.ISO8859-2
it.UTF-8 it
ja ja_JP.PCK ja_JP.UTF-8 ja_JP.eucJP
ko.UTF-8 ko
nl_BE.ISO8859-15 nl_NL.ISO8859-15
pl_PL.ISO8859-2 pl_PL.UTF-8
pt_BR.UTF-8 pt_BR pt_PT.ISO8859-15
ru_RU.ISO8859-5 ru_RU.UTF-8
sv.UTF-8 sv 
th_TH.ISO8859-11 th_TH.TIS620 th_TH.UTF-8 th_TH
tr_TR.ISO8859-9
zh.GBK zh.UTF-8 zh zh_HK.BIG5HK zh_TW.BIG5 zh_TW.UTF-8 zh_TW
'

cd /export/home/gfaden/packages/BUILD/tsoljdslabel-0.6.5
bash -x /export/home/gfaden/packages/SOURCES/updatei18n_ips0.sh ${RPM_BUILD_ROOT}/usr/share tsoljdslabel

exit 0
