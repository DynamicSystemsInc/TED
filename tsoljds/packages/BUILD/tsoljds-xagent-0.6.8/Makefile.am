SUBDIRS =			\
	egg			\
	gnome-session		\
	capplet			\
	tools			\
	compat			\
	po			\
	doc			\
	data

if ENABLE_SPLASH
SUBDIRS += splash
endif

ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = \
	$(srcdir)/INSTALL \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/config.guess \
	$(srcdir)/config.h.in \
	$(srcdir)/config.sub \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/missing \
	$(srcdir)/mkinstalldirs \
	`find "$(srcdir)" -type f -name Makefile.in -print` \
	$(srcdir)/configure \
	$(srcdir)/m4/intltool.m4

-include $(top_srcdir)/git.mk
