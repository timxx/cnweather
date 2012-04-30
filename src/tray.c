#include "tray.h"
#include "builder.h"
#include "config.h"

#define WEATHER_TRAY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), weather_tray_get_type(), WeatherTrayPrivate))

struct _WeatherTrayPrivate
{
	GtkWidget *main_window;
	GtkWidget *popup_menu;
};

static void weather_tray_init(WeatherTray *tray);
static void weather_trayclass_init(WeatherTrayClass *klass, gpointer data);
static void weather_tray_finalize(GObject *obj);

static void on_tray_popup_menu(GtkStatusIcon *status_icon,
			guint          button,
            guint          activate_time,
            gpointer       data);

static gboolean on_tray_activate(GtkStatusIcon *status_icon, gpointer       data);

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
    return WEATHER_TRAY(g_object_new(weather_tray_get_type(), "icon-name", PACKAGE_NAME, NULL));
}

static void weather_tray_init(WeatherTray *tray)
{
	WeatherTrayPrivate *priv;
	GtkBuilder *builder;

	tray->priv = WEATHER_TRAY_GET_PRIVATE(tray);

	priv = tray->priv;
	priv->popup_menu = NULL;
	priv->main_window = NULL;

	gtk_status_icon_set_tooltip_markup(GTK_STATUS_ICON(tray), "<b>cnWeather</b>");

	builder = builder_new(UI_DIR"/menu.ui");
	if (builder == NULL)
		g_warning("Missing menu.ui file!\n");

	priv->popup_menu = builder_get_widget(builder, "menu_tray");
	if (priv->popup_menu)
		g_object_ref(priv->popup_menu);
	g_object_unref(builder);

	g_signal_connect(tray, "popup-menu", G_CALLBACK(on_tray_popup_menu), NULL);
    g_signal_connect(tray, "activate", G_CALLBACK(on_tray_activate), NULL);
}

static void weather_trayclass_init(WeatherTrayClass *klass, gpointer data)
{
	GObjectClass *obj_class = (GObjectClass *)klass;

	obj_class->finalize = weather_tray_finalize;

	g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(WeatherTrayPrivate));
}

static void weather_tray_finalize(GObject *obj)
{
	WeatherTray *tray = WEATHER_TRAY(obj);
	WeatherTrayPrivate *priv = tray->priv;

	if (priv->popup_menu)
		g_object_unref(priv->popup_menu);
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
	WeatherTray *tray = WEATHER_TRAY(status_icon);

	if (tray->priv->popup_menu != NULL)
		gtk_menu_popup (GTK_MENU (tray->priv->popup_menu), NULL, NULL, NULL, data, button, activate_time); 
}

static gboolean on_tray_activate(GtkStatusIcon *status_icon, gpointer       data)
{
	WeatherTray *tray = WEATHER_TRAY(status_icon);
	WeatherTrayPrivate *priv = tray->priv;

	if (priv->main_window == NULL)
	{
		g_warning("You should call set main window\n");
		return FALSE;
	}

    if (gtk_widget_get_visible(priv->main_window))
    {
        gtk_widget_hide(priv->main_window);
    }
    else
    {
        gtk_widget_show(priv->main_window);
        gtk_window_present(GTK_WINDOW(priv->main_window));
    }

	return FALSE;
}

void weather_tray_set_main_window(WeatherTray *tray, GtkWidget *window)
{
	tray->priv->main_window = window;
}

