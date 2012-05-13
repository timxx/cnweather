#ifndef __WEATHER_WIDGET_H__
#define __WEATHER_WIDGET_H__

#include <gtk/gtk.h>

#define TYPE_WEATHER_WIDGET				(weather_widget_get_type())
#define WEATHER_WIDGET(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),	TYPE_WEATHER_WIDGET, WeatherWidget))
#define WEATHER_WIDGET_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), 	TYPE_WEATHER_WIDGET, WeatherWidgetClass))
#define IS_WEATHER_WIDGET(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),	TYPE_WEATHER_WIDGET))
#define IS_WEATHER_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), 	TYPE_WEATHER_WIDGET))
#define WEATHER_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), 	TYPE_WEATHER_WIDGET, WeatherWidgetClass))

typedef struct _WeatherWidget			WeatherWidget;
typedef struct _WeatherWidgetClass		WeatherWidgetClass;
typedef struct _WeatherWidgetPrivate	WeatherWidgetPrivate;

struct _WeatherWidget
{
    GtkHBox box;
	WeatherWidgetPrivate *priv;
};

struct _WeatherWidgetClass
{
    GtkHBoxClass parent_class;
};

GtkWidget*  weather_widget_new();
GType       weather_widget_get_type();

void		weather_widget_set_image(WeatherWidget *widget, const gchar *uri);
void		weather_widget_set_temperature(WeatherWidget *widget, const gchar *text);
void		weather_widget_set_weather(WeatherWidget *widget, const gchar *text);
void		weather_widget_set_wind(WeatherWidget *widget, const gchar *text);

#endif /* */
