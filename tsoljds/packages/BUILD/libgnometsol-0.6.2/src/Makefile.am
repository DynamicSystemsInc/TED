NULL =

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(LIBGNOMETSOL_CFLAGS)			\
	-DDATADIR=\"$(datadir)\"		\
	-DLIBDIR=\"$(libdir)\"			\
	-DPREFIX=\"$(prefix)\"			\
	-DLIBGNOMETSOL_LOCALEDIR=\"$(prefix)/@DATADIRNAME@/locale\" \
	-DSYSCONFDIR=\"$(sysconfdir)\"		\
	-DG_LOG_DOMAIN=\"libgnometsol\"		\
	$(NULL)

lib_LTLIBRARIES = libgnometsol.la

libgnometsol_la_LIBADD =			\
	$(LIBGNOMETSOL_LIBS)			\
	-ltsol					\
	$(NULL)

libgnometsol_la_LDFLAGS =			\
	-no-undefined \
	$(NULL)

libgnometsol_la_SOURCES =			\
	label_builder.c				\
	userattr.c				\
	sel_config.c				\
	pam_conv.c				\
	message_dialog.c			\
	pam_dialog.c				\
	constraint-scaling.c			\
	tsol-pics.h				\
	$(NULL)

libgnometsolincludedir = $(includedir)/libgnometsol

libgnometsolinclude_HEADERS = 			\
	label_builder.h				\
	userattr.h				\
	sel_config.h				\
	pam_conv.h				\
	message_dialog.h			\
	pam_dialog.h				\
	constraint-scaling.h			\
	$(NULL)

noinst_PROGRAMS = test

test_SOURCES = test.c
test_LDADD = libgnometsol.la
test_DEPENDENCIES = libgnometsol.la

bin_PROGRAMS = tgnome-selectlabel

tgnome_selectlabel_SOURCES = selectlabel.c
tgnome_selectlabel_LDADD = libgnometsol.la
tgnome_selectlabel_DEPENDENCIES = libgnometsol.la


