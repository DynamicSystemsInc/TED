#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="tsoljds-tstripe"

(test -f $srcdir/configure.in \
  && test -f $srcdir/configure.in \
  && test -d $srcdir/src) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

if [ -d /usr/share/aclocal/gnome2-macros ]; then
  ACLOCAL_FLAGS="-I /usr/share/aclocal -I /usr/share/aclocal/gnome2-macros"
  export ACLOCAL_FLAGS
fi

libtoolize --force || exit 1
intltoolize -c -f --automake || exit 1
#glib-gettextize --copy --force || exit 1
aclocal $ACLOCAL_FLAGS || exit 1
autoconf || exit 1
autoheader || exit 1
automake -a -c -f || exit 1
./configure "${@}"

