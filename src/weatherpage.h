#ifndef __WEATHER_PAGE_H__
#define __WEATHER_PAGE_H__

#include <gtk/gtk.h>

#include "lib/weather.h"

#define TYPE_WEATHER_PAGE				(weather_page_get_type())
#define WEATHER_PAGE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),	TYPE_WEATHER_PAGE, WeatherPage))
#define WEATHER_PAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), 	TYPE_WEATHER_PAGE, WeatherPageClass))
#define IS_WEATHER_PAGE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),	TYPE_WEATHER_PAGE))
#define IS_WEATHER_PAGE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), 	TYPE_WEATHER_PAGE))
#define WEATHER_PAGE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), 	TYPE_WEATHER_PAGE, WeatherPageClass))

typedef struct _WeatherPage			WeatherPage;
typedef struct _WeatherPageClass	WeatherPageClass;
typedef struct _WeatherPagePrivate	WeatherPagePrivate;

struct _WeatherPage
{
    GtkHBox box;
	WeatherPagePrivate *priv;
};

struct _WeatherPageClass
{
    GtkHBoxClass parent_class;
};

GtkWidget*  weather_page_new();
GType       weather_page_get_type();

void		weather_page_set_weather_info(WeatherPage *page, WeatherInfo *wi);
void		weather_page_set_image(WeatherPage *page, gint widget, const gchar *uri);

void		weather_page_set_index(WeatherPage *page, gint index);
gint		weather_page_get_index(WeatherPage *page);

void		weather_page_get_weather_info(WeatherPage *page, WeatherInfo *wi);

#endif /* __WEATHER_PAGE_H__ */
