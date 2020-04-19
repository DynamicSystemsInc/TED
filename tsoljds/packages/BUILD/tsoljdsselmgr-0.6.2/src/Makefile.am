INCLUDES = -I. -I$(srcdir) -I/usr/openwin/include/X11/extenstions 	\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 		\
	-DG_DISABLE_DEPRECATED \
	-DGNOME_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED 		\
	-DGNOME_ICONDIR=\"$(prefix)/share/pixmaps\"			\
	$(TSOLJDSSELMGR_CFLAGS)				

bin_PROGRAMS = tsoljdsselmgr

tsoljdsselmgr_SOURCES = selmgr.c	\
			selmgr-dialog.h \
			selmgr-dialog.c

tsoljdsselmgr_LDFLAGS = $(LD_FLAGS) -L/usr/openwin/lib -R/usr/openwin/lib
			
tsoljdsselmgr_LDADD = $(TSOLJDSSELMGR_LIBS) -lsecdb -lbsm -ltsol -lXtsol -lX11

