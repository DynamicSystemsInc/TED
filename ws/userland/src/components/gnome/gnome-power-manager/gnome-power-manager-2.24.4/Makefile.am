
ACLOCAL_AMFLAGS = -I m4
LIBNOTIFY_CFLAGS = -D_REENTRANT -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng14 -I/usr/include/glib-2.0 -I/usr/lib/amd64/glib-2.0/include -I/usr/include/pcre
HAL_CFLAGS = -DDBUS_API_SUBJECT_TO_CHANGE -I/usr/include/hal -I/usr/include/dbus-1.0 -I/usr/lib/amd64/dbus-1.0/include
GLIB_CFLAGS = -I/usr/include/glib-2.0 -I/usr/lib/amd64/glib-2.0/include -I/usr/include/pcre
GDK_CFLAGS = -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -I/usr/include/gtk-3.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng14 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libdrm -I/usr/include/libpng14 -I/usr/include/glib-2.0 -I/usr/lib/amd64/glib-2.0/include -I/usr/include/pcre
DBUS_CFLAGS = -D_REENTRANT -I/usr/include/dbus-1.0 -I/usr/lib/amd64/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/amd64/glib-2.0/include -I/usr/include/pcre

GLIB_LIBS = -L/usr/lib/amd64 -lgobject-2.0 -lglib-2.0
GPM_EXTRA_LIBS = -lresolv -lXext -lkstat -lgconf-2
HAL_LIBS = -L/usr/lib/amd64 -lhal -ldbus-1
LIBNOTIFY_LIBS = -L/usr/lib/amd64 -lnotify
XRANDR_LIBS = -lXrandr -lXrender -lX11
X_PRE_LIBS =  -lSM -lICE
CAIRO_LIBS = -L/usr/lib/amd64 -lcairo -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0
DBUS_LIBS =  -L/usr/lib/amd64 -ldbus-glib-1 -lgobject-2.0 -ldbus-1 -lgthread-2.0
GDK_LIBS = -L/usr/lib/amd64 -lgdk-3 -lgdk_pixbuf-2.0  -lgobject-2.0 -lglib-2.0

SUBDIRS =						\
	libdbus-glib					\
	libhal-glib					\
	libidletime					\
	libunique					\
	src						\
	po						\
	docs						\
	man						\
	data						\
	tools

if HAVE_TESTS
SUBDIRS += test
endif

EXTRA_DIST =						\
	HACKING						\
	MAINTAINERS					\
	intltool-extract.in				\
	intltool-merge.in				\
	intltool-update.in				\
	omf.make					\
	xmldocs.make					\
	gnome-doc-utils.make

clean-local :
	rm -f *~

DISTCLEANFILES =					\
	intltool-extract				\
	intltool-merge					\
	intltool-update					\
	gnome-power-manager-*.tar.gz

DISTCHECK_CONFIGURE_FLAGS = --disable-schemas-install --disable-scrollkeeper --disable-applets

