INCLUDES = -I. -I$(srcdir) -I/usr/openwin/include/X11/extenstions 	\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 		\
	-DG_DISABLE_DEPRECATED 	\
	-DGNOME_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED 		\
	-DGNOME_ICONDIR=\"$(prefix)/share/pixmaps\"			\
	$(TSOLJDSLABEL_CFLAGS)				

bin_PROGRAMS = tsoljdslabel tsoljdslabel-ui
bin_SCRIPTS = txfailsafe

tsoljdslabel_SOURCES = suidwrapper.c					\
		       exec.c						\
		       exec.h

tsoljdslabel_ui_SOURCES = tsolmotd.c					\
		       tsolmotd.h					\
		       tsoljdslabel.c					\
		       exec.c						\
		       exec.h

tsoljdslabel_ui_LDFLAGS = $(LD_FLAGS) -L/usr/openwin/lib -R/usr/openwin/lib
			
tsoljdslabel_ui_LDADD = $(TSOLJDSLABEL_LIBS) -lsecdb -lcontract -ltsol -lbsm -lXtsol

install-exec-local:
	chmod u+s $(DESTDIR)$(bindir)/tsoljdslabel
