desktop/colord-gtk: desktop/colord
desktop/gstreamer1/gst-plugins-base: desktop/gstreamer1/gstreamer
desktop/gstreamer1/gst-plugins-good: desktop/gstreamer1/gstreamer desktop/gstreamer1/gst-plugins-base
desktop/gstreamer1/gst-python: desktop/gstreamer1/gstreamer
desktop/speech-dispatcher: desktop/dotconf
gnome/clutter-gtk: gnome/gtk3
gnome/libgdata: gnome/gnome-online-accounts
# Temporary until build machines install pycairo for python3
gnome/pygobject3: python/pycairo
gnupg:	libksba libassuan gpgme pinentry
libxslt:	libxml2
openjade: opensp
openldap:      cyrus-sasl
perl_modules/xml-simple:	perl_modules/xml-parser
php/suhosin-extension: php/php56 php/php71
php/xdebug: php/php56 php/php71
python/cryptography:	python/cffi
python/xattr:	python/cffi
rdiff-backup: librsync
ruby/puppet:		ruby/facter ruby/hiera
ruby/puppet-modules/oracle-solaris_providers:		ruby/puppet
tcl/expect:		tcl/tcl
tcl/tk:		tcl/tcl
x11/app/xfs-utilities: x11/lib/libFS
x11/data/xcursor-themes: x11/app/xcursorgen
x11/doc: x11/util/util-macros
x11/driver/xf86-video-intel: x11/lib/libXvMC
# sun-src/src/VisGamma.c requires libX11 private headers for _Xcms functions
x11/lib/libXmu: x11/lib/libX11
x11/lib/libglu: x11/lib/mesa
x11/lib/mesa: x11/lib/libdrm
x11/proto: x11/doc
# util-macros must be built before any other x11 component as they may need
# the macros it provides to regenerate their configure scripts with autoreconf

X11_COMPONENT_DIRS = \
    $(filter-out x11/util/util-macros, $(filter x11/%, $(COMPONENT_DIRS)))

$(X11_COMPONENT_DIRS): x11/util/util-macros
# Delete this & Makefile section when build servers are installed with fltk
x11/xserver/xvnc: x11/lib/fltk
