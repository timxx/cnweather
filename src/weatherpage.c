#include "weatherpage.h"
#include "weatherwidget.h"

#define WEATHER_PAGE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), weather_page_get_type(), WeatherPagePrivate))

#define TOTAL_WIDGETS	3

struct _WeatherPagePrivate
{
	GtkWidget*	widget[TOTAL_WIDGETS];	/* weather widget */
	gint		index;
};

static void weather_page_init(WeatherPage *page);
static void weather_page_class_init(WeatherPageClass *klass);

GType weather_page_get_type()
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
        {
            sizeof(WeatherPageClass),				    // class size
            NULL,									    // base init
            NULL,									    // base finalize
            (GClassInitFunc)weather_page_class_init,	// class init
            NULL,									    // class finalize
            NULL,									    // data
            sizeof(WeatherPage),					    // instance size
            0,										    // prealloc
            (GInstanceInitFunc)weather_page_init	    // instance init
        };

        type = g_type_register_static(GTK_TYPE_HBOX, "WeatherPage", &info, 0);
    }

    return type;
}

GtkWidget* weather_page_new()
{
    return g_object_new(TYPE_WEATHER_PAGE,
				"spacing", 20,
				"expand", FALSE,
				//"margin-left", 20,
				//"margin-top", 50,
				"valign", GTK_ALIGN_CENTER,
				"halign", GTK_ALIGN_CENTER,
				NULL);
}

static void weather_page_init(WeatherPage *page)
{
	WeatherPagePrivate *priv;
	gint i;

	priv = WEATHER_PAGE_GET_PRIVATE(page);
	page->priv = priv;

	priv->index = -1;

	for(i=0; i<TOTAL_WIDGETS; ++i)
	{
		priv->widget[i] = weather_widget_new();

		gtk_box_pack_start(GTK_BOX(page), priv->widget[i], FALSE, FALSE, 0);
	}

	gtk_widget_show_all(GTK_WIDGET(page));
}

static void weather_page_class_init(WeatherPageClass *klass)
{
    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(WeatherPagePrivate));
}

void weather_page_set_weather_info(WeatherPage *page, WeatherInfo *wi)
{
	gint i;
	WeatherPagePrivate *priv;

	g_return_if_fail( page != NULL && wi != NULL);

	priv = page->priv;
	for(i=0; i<TOTAL_WIDGETS; ++i)
	{
		weather_widget_set_weather(WEATHER_WIDGET(priv->widget[i]), wi->weather[i].weather);
		weather_widget_set_temperature(WEATHER_WIDGET(priv->widget[i]), wi->weather[i].temperature);
		weather_widget_set_wind(WEATHER_WIDGET(priv->widget[i]), wi->weather[i].wind);
	}

	weather_widget_set_temperature(WEATHER_WIDGET(priv->widget[0]), wi->temp == NULL ? "0" : wi->temp);
	gtk_widget_set_tooltip_text(priv->widget[0], wi->weather[0].temperature);
}

void weather_page_set_image(WeatherPage *page, gint widget, const gchar *uri)
{
	g_return_if_fail(page != NULL && widget >=0 && widget < TOTAL_WIDGETS);

	weather_widget_set_image(WEATHER_WIDGET(page->priv->widget[widget]), uri);
}

void weather_page_set_index(WeatherPage *page, gint index)
{
	g_return_if_fail(page != NULL);

	page->priv->index = index;
}

gint weather_page_get_index(WeatherPage *page)
{
	g_return_val_if_fail(page != NULL, -1);

	return page->priv->index;
}

void weather_page_get_weather_info(WeatherPage *page, WeatherInfo *wi)
{
	gint i;
	WeatherPagePrivate *priv;

	g_return_if_fail(page != NULL && wi != NULL);

	priv = page->priv;

	for(i=0; i<TOTAL_WIDGETS; ++i)
	{
		if (i==0)
		{
			wi->weather[i].temperature = gtk_widget_get_tooltip_text(priv->widget[0]);
			
			wi->temp = g_strdup(weather_widget_get_temperature(WEATHER_WIDGET(priv->widget[i])));
		}
		else
		{
			wi->weather[i].temperature = g_strdup(weather_widget_get_temperature(WEATHER_WIDGET(priv->widget[i])));
		}

		wi->weather[i].weather = g_strdup(
			weather_widget_get_weather(WEATHER_WIDGET(priv->widget[i]))
			);

		wi->weather[i].wind =
			weather_widget_get_wind(WEATHER_WIDGET(priv->widget[i]));

		wi->weather[i].img =
			weather_widget_get_image(WEATHER_WIDGET(priv->widget[i]));
	}
}
