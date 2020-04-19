SUBDIRS = src po

AUTOMAKE_OPTIONS = dist-bzip2

desktopdir = $(datadir)/xsessions/multilabel
desktop_in_files = tgnome.desktop.in txfailsafe.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

sessiondir = /usr/dt/config
session_SCRIPTS = Xinitrc.tjds Xsession.tjds Xsession2.tjds

EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(session_SCRIPTS) \
	$(desktop_in_files) \
	$(man_MANS) \
	mkinstalldirs

DISTCLEANFILES =		\
	intltool-extract	\
	intltool-merge		\
	intltool-update		

