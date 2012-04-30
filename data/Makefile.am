SUBDIRS = icons ui themes

@INTLTOOL_DESKTOP_RULE@
desktop_in_files = cnweather.desktop.in
desktop_files = $(desktop_in_files:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_files)

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

gsettings_SCHEMAS = com.timxx.cnweather.gschema.xml

EXTRA_DIST =	\
	$(icon_DATA)	\
	$(desktop_DATA)

DISTCLEANFILES =	\
	$(desktop_files)	\
	$(gsettings_SCHEMAS)

clean-local:
	rm -f *~
