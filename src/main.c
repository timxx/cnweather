#include <gtk/gtk.h>

#include "intl.h"
#include "config.h"
#include "cnWeather.h"

static void on_activate(GtkApplication *app);

int main(int argc, char **argv)
{
    GtkApplication *app;
    gint status;

    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    app = gtk_application_new("org.timxx.cnWeather", 0);
    if (app == NULL)
    {
        g_error("cnWeather: create app failed (%s, %d)\n", __FILE__, __LINE__);
        return -1;
    }

    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

	return status;
}

static void on_activate(GtkApplication *app)
{
    GList *list;
    GtkWidget *window;
    list = gtk_application_get_windows(app);

    if (list)
    {
        gtk_window_present(GTK_WINDOW(list->data));
    }
    else
    {
        window = weather_window_new();
        if (window == NULL)
        {
            g_error("cnWeather: Unable to create main window(%s, %d)\n", __FILE__, __LINE__);
            return ;
        }
        gtk_window_set_application(GTK_WINDOW(window), app);
        gtk_widget_show_all(window);
    }
}
