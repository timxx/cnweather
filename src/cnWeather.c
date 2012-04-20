#include "cnWeather.h"
#include "intl.h"
#include "config.h"

#define WEATHER_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), weather_window_get_type(), cnWeatherPrivate))

typedef struct 
{
    gchar*  cur_city;

}cnWeatherPrivate;

static void weather_window_init(cnWeather *window);
static void weather_window_class_init(cnWeatherClass *klass);
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean on_delete(GtkWidget *widget, GdkEvent *event, gpointer data);

GType weather_window_get_type()
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
        {
            sizeof(cnWeatherClass),				        // class size
            NULL,									    // base init
            NULL,									    // base finalize
            (GClassInitFunc)weather_window_class_init,  // class init
            NULL,									    // class finalize
            NULL,									    // data
            sizeof(cnWeather),					        // instance size
            0,										    // prealloc
            (GInstanceInitFunc)weather_window_init	    // instance init
        };

        type = g_type_register_static(GTK_TYPE_WINDOW, "cnWeather", &info, 0);
    }

    return type;
}

GtkWidget* weather_window_new()
{
    return g_object_new(TYPE_WEATHER_WINDOW, NULL);
}

static void weather_window_init(cnWeather *window)
{
    GtkIconTheme *icon_theme;

    gtk_window_set_title(GTK_WINDOW(window), _("cnWeather"));
    gtk_widget_set_app_paintable(GTK_WIDGET(window), TRUE);

    icon_theme = gtk_icon_theme_get_default();
    if (icon_theme)
    {
        GdkPixbuf *pb;
        pb = gtk_icon_theme_load_icon(icon_theme, PACKAGE_NAME, 48, 0, NULL);
        if (pb)
        {
            gtk_window_set_icon(GTK_WINDOW(window), pb);

            g_object_unref(pb);
        }
    }

    g_signal_connect(window, "delete-event", G_CALLBACK(on_delete), NULL);
    g_signal_connect(window, "draw", G_CALLBACK(on_draw), NULL);
}

static void weather_window_class_init(cnWeatherClass *klass)
{
    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(cnWeatherPrivate));
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    return FALSE;
}

static gboolean on_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GtkApplication *app;

    app = gtk_window_get_application(GTK_WINDOW(widget));

    g_application_quit(G_APPLICATION(app));

    return FALSE;
}
