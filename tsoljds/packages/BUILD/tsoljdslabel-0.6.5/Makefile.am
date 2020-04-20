SUBDIRS = src po

AUTOMAKE_OPTIONS = dist-bzip2


sessiondir = /usr/dt/config
session_SCRIPTS = Xinitrc.tjds \
	tjds-exec_attr \
	tjds-prof_attr \
	tjds-user_attr \
	policy.conf \
	txzonemgr \
	pkgcreatezone \
	common.ksh


EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(session_SCRIPTS) \
	$(man_MANS) \
	mkinstalldirs

DISTCLEANFILES =		\
	intltool-extract	\
	intltool-merge		\
	intltool-update		

