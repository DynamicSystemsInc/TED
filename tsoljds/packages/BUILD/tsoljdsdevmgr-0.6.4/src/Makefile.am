INCLUDES = -I. -I$(srcdir) -I/usr/openwin/include/X11/extenstions 	\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 		\
	-DG_DISABLE_DEPRECATED 	\
	-DGNOME_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED 		\
	-DGNOME_ICONDIR=\"$(prefix)/share/pixmaps\"			\
	$(TSOLJDSDEVMGR_CFLAGS)				

bin_PROGRAMS = tsoljdsdevmgr

tsoljdsdevmgr_SOURCES = devmgr.c	\
			devmgr.h \
			devmgr-dialog.h \
			devmgr-dialog.c \
			devmgr-help.h \
			devmgr-help.c \
			devmgr-props-dialog.c \
			devmgr-props-dialog.h  

tsoljdsdevmgr_LDFLAGS = $(LD_FLAGS) -L/usr/openwin/lib -R/usr/openwin/lib
			
tsoljdsdevmgr_LDADD = $(TSOLJDSDEVMGR_LIBS) -lsecdb -lbsm -ltsol -lXtsol -lX11

