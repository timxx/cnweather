icons =	\
	apps_16x16_cnWeather.png	\
	apps_22x22_cnWeather.png	\
	apps_24x24_cnWeather.png	\
	apps_32x32_cnWeather.png	\
	apps_48x48_cnWeather.png	\
	apps_64x64_cnWeather.png

update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-icons:
	for icon in $(icons); do \
		CONTEXT=`echo $$icon | cut -d_ -f1`; \
		SIZE=`echo $$icon | cut -d_ -f2`; \
		ICONFILE=`echo $$icon | cut -d_ -f3`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;	\
	$(update_icon_cache)

uninstall-icons:
	for icon in $(icons); do \
		CONTEXT=`echo $$icon | cut -d_ -f1`; \
		SIZE=`echo $$icon | cut -d_ -f2`; \
		ICONFILE=`echo $$icon | cut -d_ -f3`; \
		rm -f $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;	\
	$(update_icon_cache)

install-data-local: install-icons
uninstall-local: uninstall-icons
