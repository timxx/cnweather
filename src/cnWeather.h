#ifndef __CN_WEATHER_H__
#define __CN_WEATHER_H__

#include <gtk/gtk.h>

#include "wsettings.h"

#define TYPE_WEATHER_WINDOW             (weather_window_get_type())
#define WEATHER_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_WEATHER_WINDOW, cnWeather))
#define WEATHER_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), 	TYPE_WEATHER_WINDOW, cnWeatherClass))
#define IS_WEATHER_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_IMAGE_WINDOW))
#define IS_WEATHER_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), 	TYPE_WEATHER_WINDOW))
#define WEATHER_WINDOW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), 	TYPE_WEATHER_WINDOW, cnWeatherClass))

typedef struct _cnWeather       cnWeather;
typedef struct _cnWeatherClass  cnWeatherClass;
typedef struct _cnWeatherPrivate cnWeatherPrivate;

enum
{
	PAGE_WEATHER = 0,
	PAGE_SEARCH,
	PAGE_PREFERENCES
};

enum
{
	CB_PROVINCE = 0,
	CB_CITY,
	CB_TOWN
};

struct _cnWeather
{
    GtkWindow window;
	cnWeatherPrivate *priv;
};

struct _cnWeatherClass
{
    GtkWindowClass parent_class;
};

GtkWidget*  weather_window_new();
GType       weather_window_get_type();

/**
 * update weather by city_id
 */
void		weather_window_get_weather(cnWeather *window, guint city_id);

/**
 * set current page on main window
 */
void		weather_window_set_page(cnWeather *window, int page);

/**
 * search city from local database
 */
void		weather_window_search(cnWeather *window, const gchar *city);

/**
 * update weather information
 */
void		weather_window_update(cnWeather *window);

/**
 * only for text information
 */
void		weather_window_set_search_result(cnWeather *window, const gchar *text);

/**
 * return settings
 */
wSettings*	weather_window_get_settings(cnWeather *window);

/**
 * show/hide tray by state
 */
void		weather_window_show_tray(cnWeather *window, gboolean state);

void		weather_window_quit(cnWeather *window);
void		weather_window_update_cache(cnWeather *window);

/**
 * update duration (in minute)
 * reset timer
 */
void		weather_window_set_duration(cnWeather *window, gint duration);

/**
 * update preferences page
 * @cb: which combobox
 * @name: parent city name
 */
void		weather_window_update_pref_cb(cnWeather *window, gint cb, gchar *name);

#endif /* __CN_WEATHER_H__ */
