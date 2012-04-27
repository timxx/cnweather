SUBDIRS = lib test

bin_PROGRAMS = cnWeather

cnWeather_SOURCES =	\
	cnWeather.c		\
	main.c			\
	tray.c			\
	builder.c		\
	signal_handler.c	\
	wsettings.c	\
	common.c

AM_CFLAGS =	\
		@GTK_CFLAGS@	\
		-Wall -Werror	\
		-DLOCALEDIR='"$(datadir)/locale"'	\
		-DUI_DIR='"$(pkgdatadir)/ui"'	\
		-DTHEME_DIR='"$(pkgdatadir)/themes"'

cnWeather_LDADD =	\
	@GTK_LIBS@	\
	lib/libweather.la
