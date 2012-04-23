#include "tray.h"


static void weather_tray_init(WeatherTray *tray);
static void weather_trayclass_init(WeatherTrayClass *klass, gpointer data);

static void on_tray_popup_menu(GtkStatusIcon *status_icon,
			guint          button,
            guint          activate_time,
            gpointer       data);

static gboolean on_tray_button_press(GtkStatusIcon *status_icon,
            GdkEvent      *event,
            gpointer       data);

GType weather_tray_get_type()
{
    static GType type = 0;
    if(type == 0)
	{
        const GTypeInfo info =
        {
            sizeof(WeatherTrayClass),
            NULL,
            NULL,
            (GClassInitFunc)weather_trayclass_init,
            NULL,
            NULL,
            sizeof(WeatherTray),
            0,
            (GInstanceInitFunc)weather_tray_init,
            0
        };

        type = g_type_register_static(GTK_TYPE_STATUS_ICON, "WeatherTray", &info, 0);
    }

    return type;
}

WeatherTray* weather_tray_new()
{
    return WEATHER_TRAY(g_object_new(weather_tray_get_type(), NULL));
}

static void weather_tray_init(WeatherTray *tray)
{
	gtk_status_icon_set_from_icon_name(GTK_STATUS_ICON(tray), "cnWeather");
	gtk_status_icon_set_tooltip_markup(GTK_STATUS_ICON(tray), "<b>cnWeather</b>");

	g_signal_connect(tray, "popup-menu", G_CALLBACK(on_tray_popup_menu), NULL);
    g_signal_connect(tray, "button-press-event", G_CALLBACK(on_tray_button_press), NULL);
}

static void weather_trayclass_init(WeatherTrayClass *klass, gpointer data)
{
}

void weather_tray_set_tooltips(WeatherTray *tray, const gchar *text)
{
	g_return_if_fail(tray != NULL && text != NULL);

	gtk_status_icon_set_tooltip_markup(GTK_STATUS_ICON(tray), text);
}

void weather_tray_set_icon(WeatherTray *tray, const gchar *icon)
{
	g_return_if_fail(tray != NULL && icon != NULL);

	gtk_status_icon_set_from_file(GTK_STATUS_ICON(tray), icon);
}

static void on_tray_popup_menu(GtkStatusIcon *status_icon,
			guint          button,
            guint          activate_time,
            gpointer       data)
{
}

static gboolean on_tray_button_press(GtkStatusIcon *status_icon,
            GdkEvent      *event,
            gpointer       data)
{
	return FALSE;
}
