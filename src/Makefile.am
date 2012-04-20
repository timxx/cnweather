bin_PROGRAMS = cnWeather

cnWeather_SOURCES =	\
	cnWeather.c		\
	main.c			\
	tray.c

AM_CFLAGS =	\
		@GTK_CFLAGS@	\
		-Wall -Werror	\
		-DLOCALEDIR='"$(datadir)/locale"'

cnWeather_LDADD =	\
	@GTK_LIBS@
