
#include "weatherwidget.h"
#include "config.h"

#define WEATHER_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), weather_widget_get_type(), WeatherWidgetPrivate))

struct _WeatherWidgetPrivate
{
	GtkWidget*	image;
	GtkWidget*	weather;
	GtkWidget*	temperature;
};

static void weather_widget_init(WeatherWidget *widget);
static void weather_widget_class_init(WeatherWidgetClass *klass);

GType weather_widget_get_type()
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
        {
            sizeof(WeatherWidgetClass),				    // class size
            NULL,									    // base init
            NULL,									    // base finalize
            (GClassInitFunc)weather_widget_class_init,  // class init
            NULL,									    // class finalize
            NULL,									    // data
            sizeof(WeatherWidget),					    // instance size
            0,										    // prealloc
            (GInstanceInitFunc)weather_widget_init	    // instance init
        };

        type = g_type_register_static(GTK_TYPE_HBOX, "WeatherWidget", &info, 0);
    }

    return type;
}

GtkWidget* weather_widget_new()
{
    return g_object_new(TYPE_WEATHER_WIDGET,
				"expand", FALSE,
				NULL);
}

static void weather_widget_init(WeatherWidget *widget)
{
	WeatherWidgetPrivate *priv;
	GtkWidget *box1;

	priv = WEATHER_WIDGET_GET_PRIVATE(widget);
	widget->priv = priv;

	box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	if (box1 == NULL)
		g_error("gtk_box_new failed (%s, %d)\n", __FILE__, __LINE__);

	priv->image = gtk_image_new_from_icon_name(PACKAGE_NAME, GTK_ICON_SIZE_LARGE_TOOLBAR);
	priv->weather = gtk_label_new("");
	priv->temperature = gtk_label_new("");

	gtk_box_pack_start(GTK_BOX(box1), priv->temperature, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(box1), priv->weather, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(widget), priv->image, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(widget), box1, FALSE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(widget));
}

static void weather_widget_class_init(WeatherWidgetClass *klass)
{
    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(WeatherWidgetPrivate));
}

void weather_widget_set_image(WeatherWidget *widget, const gchar *uri)
{
	g_return_if_fail(widget != NULL);

	if (uri != NULL)
		gtk_image_set_from_file(GTK_IMAGE(widget->priv->image), uri);
	else
		gtk_image_set_from_icon_name(GTK_IMAGE(widget->priv->image), PACKAGE_NAME, GTK_ICON_SIZE_LARGE_TOOLBAR);
}

void weather_widget_set_temperature(WeatherWidget *widget, const gchar *text)
{
	g_return_if_fail(widget != NULL);

	gtk_label_set_text(GTK_LABEL(widget->priv->temperature), text);
}

void weather_widget_set_weather(WeatherWidget *widget, const gchar *text)
{
	g_return_if_fail(widget != NULL);

	gtk_label_set_text(GTK_LABEL(widget->priv->weather), text);
}

void weather_widget_set_wind(WeatherWidget *widget, const gchar *text)
{
	g_return_if_fail(widget != NULL);

	gtk_widget_set_tooltip_text(widget->priv->image, text);
}
