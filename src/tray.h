#ifndef __TRAY_H__
#define __TRAY_H__

#include <gtk/gtk.h>

#define WEATHER_TRAY(obj)		G_TYPE_CHECK_INSTANCE_CAST(obj, weather_tray_get_type(), WeatherTray)
#define WEATHER_TRAYCLASS(c)	G_TYPE_CHECK_CLASS_CAST(c, weather_tray_get_type(), WeatherTrayClass)
#define WEATHER_IS_TRAY(obj)	G_TYPE_CHECK_INSTANCE_TYPE(obj, weather_tray_get_type())

typedef struct _WeatherTray WeatherTray;
typedef struct _WeatherTrayClass WeatherTrayClass;
typedef struct _WeatherTrayPrivate WeatherTrayPrivate;

struct _WeatherTray
{
	GtkStatusIcon parent;
	WeatherTrayPrivate *priv;
};

struct _WeatherTrayClass
{
	GtkStatusIconClass parent_class;
};

WeatherTray*	weather_tray_new();
GType			weather_tray_get_type();

void	weather_tray_set_tooltips(WeatherTray *tray, const gchar *text);
void	weather_tray_set_icon(WeatherTray *tray, const gchar *icon);

#endif
