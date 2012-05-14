#ifndef __WEATHER_TAB_H__
#define __WEATHER_TAB_H__

#include <gtk/gtk.h>

#define TYPE_WEATHER_TAB				(weather_tab_get_type())
#define WEATHER_TAB(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),	TYPE_WEATHER_TAB, WeatherTab))
#define WEATHER_TAB_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), 	TYPE_WEATHER_TAB, WeatherTabClass))
#define IS_WEATHER_TAB(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj),	TYPE_WEATHER_TAB))
#define IS_WEATHER_TAB_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), 	TYPE_WEATHER_TAB))
#define WEATHER_TAB_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), 	TYPE_WEATHER_TAB, WeatherTabClass))

typedef struct _WeatherTab			WeatherTab;
typedef struct _WeatherTabClass		WeatherTabClass;
typedef struct _WeatherTabPrivate	WeatherTabPrivate;

struct _WeatherTab
{
    GtkHBox box;
	WeatherTabPrivate *priv;
};

struct _WeatherTabClass
{
    GtkHBoxClass parent_class;
};

GtkWidget*		weather_tab_new(GtkWidget *parent, GtkWidget *page);
GType			weather_tab_get_type();

void			weather_tab_set_title(WeatherTab *tab, const gchar *title);
const gchar*	weather_tab_get_title(WeatherTab *tab);

#endif /* __WEATHER_TAB_H__ */
