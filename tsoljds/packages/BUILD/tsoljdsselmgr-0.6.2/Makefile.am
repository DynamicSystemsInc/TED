SUBDIRS = src po

AUTOMAKE_OPTIONS = dist-bzip2

configdir = $(datadir)/gnome
config_DATA = sel_config

EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(config_DATA) \
	$(man_MANS) \
	mkinstalldirs

DISTCLEANFILES =		\
	intltool-extract	\
	intltool-merge		\
	intltool-update		

