SUBDIRS = icons ui

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = cnWeather.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST =	\
	$(icon_DATA)	\
	$(desktop_DATA)


clean-local:
	rm -f *~
