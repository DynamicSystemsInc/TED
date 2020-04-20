## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-D_POSIX_PTHREAD_SEMANTICS \
	@PACKAGE_CFLAGS@

configdir = $(datadir)/gnome
config_DATA = TrustedPathExecutables gtkrc.tjds

bin_PROGRAMS = tsoljds-tstripe tsoljds-xagent-proxy tsoljds-setssheight

tsoljds_tstripe_SOURCES = \
	main.c \
	ui-view.c ui-view.h \
	ui-controller.c ui-controller.h \
	pics.h \
	privs.c privs.h \
	menus.c menus.h \
	tsol-user.c tsol-user.h \
	xagent-management.c xagent-management.h \
	xutils.c xutils.h

tsoljds_xagent_proxy_SOURCES = \
	privs.c privs.h \
	tsol-user.c tsol-user.h \
	xagent-proxy.c

tsoljds_setssheight_SOURCES = \
	stripe-height.c

tsoljds_tstripe_LDADD = @PACKAGE_LIBS@ $(INTLLIBS) -lnsl

tsoljds_xagent_proxy_LDADD = @PACKAGE_LIBS@ -ltsnet -lnsl

tsoljds_setssheight_LDADD = @PACKAGE_LIBS@  -lXtsol

EXTRA_DIST = \
	test-constraint.c \
	menuitem2.png \
	$(config_DATA)
