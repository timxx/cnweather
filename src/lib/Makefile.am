lib_LTLIBRARIES = libweather.la

libweather_la_CFLAGS =	\
	@LIBCURL_CFLAGS@	\
	@GLIB_CFLAGS@	\
	@SQLITE3_CFLAGS@	\
	-Wall -Werror

libweather_la_LIBADD =	\
	@LIBCURL_LIBS@	\
	@GLIB_LIBS@	\
	@SQLITE3_LIBS@

libweather_la_SOURCES =	\
	weather.c

