SUBDIRS = lib test

bin_PROGRAMS = cnweather

cnweather_SOURCES =	\
	cnWeather.c		\
	main.c			\
	tray.c			\
	builder.c		\
	signal_handler.c	\
	wsettings.c	\
	common.c	\
	weatherpage.c	\
	weatherwidget.c	\
	weathertab.c

AM_CFLAGS =	\
		@GTK_CFLAGS@	\
		-Wall -Werror	\
		-DLOCALEDIR='"$(datadir)/locale"'	\
		-DUI_DIR='"$(pkgdatadir)/ui"'	\
		-DTHEME_DIR='"$(pkgdatadir)/themes"'

cnweather_LDADD =	\
	@GTK_LIBS@		\
	@LIBCURL_LIBS@	\
	@SQLITE3_LIBS@	\
	lib/libweather.la
