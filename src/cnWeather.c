#include "cnWeather.h"
#include "intl.h"
#include "config.h"
#include "builder.h"
#include "lib/weather.h"
#include "tray.h"

#define WEATHER_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), weather_window_get_type(), cnWeatherPrivate))

typedef struct _cnWeatherPrivate 
{
    unsigned int	city_id;
	WeatherInfo*	weather;

	GtkBuilder*		ui_main;
	GtkBuilder*		ui_search;
	GtkBuilder*		ui_weather;
	GtkBuilder*		ui_pref;

	GThread*		thread_get_weather;
	GThread*		thread_get_city;

	WeatherTray*	tray;

	wSettings*		settings;

}cnWeatherPrivate;

static void weather_window_init(cnWeather *window);
static void weather_window_class_init(cnWeatherClass *klass);
static void weather_window_finalize(GObject *obj);

static gboolean on_delete(GtkWidget *widget, GdkEvent *event, gpointer data);

static gpointer get_weather_thread(gpointer data);
static gpointer search_city_thread(gpointer data);

static void update_tray_tooltips(cnWeather *window);

static void load_settings(cnWeather *window);
static void save_settings(cnWeather *window);

static void set_preferences_page(cnWeather *window);

static gboolean check_auto_start(cnWeather *window);

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
	cnWeatherPrivate *priv;

    GtkIconTheme *icon_theme;
	GtkWidget	 *box_main;
	GtkWidget	 *first_page, *second_page, *third_page;
	GtkWidget	 *note_book;

	window->priv = WEATHER_WINDOW_GET_PRIVATE(window);
	if (window->priv == NULL)
	{
		g_error("window->priv == NULL!!!");
	}

	priv = window->priv;

	priv->weather = weather_new_info();
	if (priv->weather == NULL)
	{
		g_error("failed to molloc memory!(%s, %d)\n", __FILE__, __LINE__);
	}

	priv->tray = NULL;

	priv->thread_get_weather = NULL;
	priv->thread_get_city = NULL;

	priv->settings = w_settings_new();

	priv->ui_main = builder_new(UI_DIR"/main.ui");
	priv->ui_search = builder_new(UI_DIR"/search.ui");
	priv->ui_weather = builder_new(UI_DIR"/weather.ui");
	priv->ui_pref = builder_new(UI_DIR"/preferences.ui");

	if (priv->ui_main == NULL		||
		priv->ui_search == NULL		||
		priv->ui_weather == NULL	||
		priv->ui_pref == NULL
		)
	{
		g_error("UI files missing!\n");
	}

	box_main = builder_get_widget(priv->ui_main, "box_main");

	first_page = builder_get_widget(priv->ui_weather, "box_weather");
	second_page = builder_get_widget(priv->ui_search, "box_search");
	third_page = builder_get_widget(priv->ui_pref, "box_preferences");

	note_book = builder_get_widget(priv->ui_main, "note_book");

	if (box_main == NULL || first_page == NULL		||
		second_page == NULL || note_book == NULL	||
		third_page ==  NULL
		)
	{
		g_error("UI file damaged!\n");
	}

    gtk_window_set_title(GTK_WINDOW(window), _("cnWeather"));
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
	gtk_widget_set_size_request(GTK_WIDGET(window), 500, 250);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
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

	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), first_page, NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), second_page, NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), third_page, NULL);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(note_book), 0);

	gtk_container_add(GTK_CONTAINER(window), box_main);

    g_signal_connect(window, "delete-event", G_CALLBACK(on_delete), NULL);

	priv->city_id = 0;
	load_settings(window);

	weather_window_get_weather(window, priv->city_id);
}

static void weather_window_class_init(cnWeatherClass *klass)
{
	GObjectClass *obj_class = (GObjectClass *)klass;
//	GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;

	obj_class->finalize = weather_window_finalize;

    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(cnWeatherPrivate));
}

static void weather_window_finalize(GObject *obj)
{
	cnWeather *window = WEATHER_WINDOW(obj);
	cnWeatherPrivate *priv = window->priv;

	weather_free_info(priv->weather);
	if (priv->weather)
		g_free(priv->weather);

	if (priv->thread_get_weather)
		g_thread_unref(priv->thread_get_weather);
	if (priv->thread_get_city)
		g_thread_unref(priv->thread_get_city);

	if (priv->ui_main)
		g_object_unref(priv->ui_main);
	if (priv->ui_weather)
		g_object_unref(priv->ui_weather);
	if (priv->ui_search)
		g_object_unref(priv->ui_search);

	if (priv->settings)
		g_object_unref(priv->settings);
}

static gboolean on_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	cnWeather *window = WEATHER_WINDOW(widget);
	cnWeatherPrivate *priv = window->priv;

	save_settings(window);

	if (priv->tray)
		g_object_unref(priv->tray);

	gtk_widget_destroy(widget);

    return FALSE;
}

void weather_window_get_weather(cnWeather *window, unsigned int city_id)
{
	g_return_if_fail( window != NULL );

	window->priv->city_id = city_id;

	if (g_thread_supported())
	{
		window->priv->thread_get_weather = g_thread_new("GetWeather",
					&get_weather_thread, window);
	}
	else
	{
		get_weather_thread(window);
	}
}

void weather_window_set_page(cnWeather *window, int page)
{
	GtkWidget *notebook;

	g_return_if_fail( window != NULL );

	notebook = builder_get_widget(window->priv->ui_main, "note_book");

	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);
}

void weather_window_search(cnWeather *window, const gchar *city)
{
	cnWeatherPrivate *priv;

	g_return_if_fail( window != NULL && city != NULL);
	priv = window->priv;

	if (priv->weather->city != NULL)
	{
		g_free(priv->weather->city);
		priv->weather->city = NULL;
	}

	priv->weather->city = g_strdup(city);
	if (priv->weather->city == NULL)
	{
		g_warning("g_strdup failed (%s, %d)", __FILE__, __LINE__);
		return ;
	}

	if (g_thread_supported())
	{
		priv->thread_get_city = g_thread_new("GetCity",
					&search_city_thread, window);
	}
	else
	{
		search_city_thread(window);
	}
}

static gpointer get_weather_thread(gpointer data)
{
	cnWeather *window = (cnWeather *)data;
	wSession *ws;

	g_debug("geting weather\n");

	do
	{
		int ret;
		ws = weather_open();
		if (ws == NULL)
			break;

		if (window->priv->city_id == 0)
			ret = weather_get_default_city(ws, window->priv->weather);
		else
			ret = weather_get(ws, window->priv->city_id, window->priv->weather);

		if (ret == 0)
		{
			window->priv->city_id = window->priv->weather->city_id;
			weather_window_update(window);
		}
		else
		{
			g_debug("failed to get weather\n");
		}
	
		g_debug("geting weather finished\n");

		weather_close(ws);
	}
	while(0);


	if (window->priv->thread_get_weather)
	{
		g_thread_unref(window->priv->thread_get_weather);
		window->priv->thread_get_weather = NULL;
	}

	return NULL;
}

static gpointer search_city_thread(gpointer data)
{
	return NULL;
}

void weather_window_update(cnWeather *window)
{
	GtkWidget *widget;
	cnWeatherPrivate *priv;

	static const char *label[]=
	{
		"label_city", "image_weather", "label_weather",
		"label_temperature"
	};

	int i;

	gchar widget_name[30];

	g_return_if_fail( window != NULL );

	priv = window->priv;

	widget = builder_get_widget(priv->ui_weather, label[0]);
	if (widget) {
		gtk_label_set_text(GTK_LABEL(widget), priv->weather->city);
	}

	for(i=0; i<3; ++i)
	{
		snprintf(widget_name, 30, "%s%d", label[1], i+1);
		widget = builder_get_widget(priv->ui_weather, widget_name);
		if (widget)
		{
			gtk_image_set_from_stock(GTK_IMAGE(widget), GTK_STOCK_FIND, GTK_ICON_SIZE_DIALOG);
			gtk_widget_set_tooltip_text(widget, priv->weather->weather[i].wind);
		}

		snprintf(widget_name, 30, "%s%d", label[2], i+1);
		widget = builder_get_widget(priv->ui_weather, widget_name);
		if (widget){
			gtk_label_set_text(GTK_LABEL(widget), priv->weather->weather[i].weather);
		}

		snprintf(widget_name, 30, "%s%d", label[3], i+1);
		widget = builder_get_widget(priv->ui_weather, widget_name);
		if (widget){
			gtk_label_set_text(GTK_LABEL(widget), priv->weather->weather[i].temperature);
		}
	}

	update_tray_tooltips(window);
}

void weather_window_set_search_result(cnWeather *window, const gchar *text)
{
	GtkWidget *label;

	g_return_if_fail(window != NULL && text != NULL);

	label = builder_get_widget(window->priv->ui_search, "label_result");
	if (label) {
		gtk_label_set_text(GTK_LABEL(label), text);

	}
}

static void update_tray_tooltips(cnWeather *window)
{
	cnWeatherPrivate *priv;

	gchar tooltips[100];

	g_return_if_fail(window != NULL);

	priv = window->priv;

	if (window->priv->tray == NULL)
		return ;

	snprintf(tooltips, 100,
				"<b>%s</b>\n"
				"%s\n"
				"%s\n"
				"%s\n"
				,
				priv->weather->city,
				priv->weather->weather[0].weather,
				priv->weather->weather[0].temperature,
				priv->weather->weather[0].wind
			);

	weather_tray_set_tooltips(priv->tray, tooltips);
}

wSettings* weather_window_get_settings(cnWeather *window)
{
	g_return_val_if_fail(window != NULL, NULL);

	return window->priv->settings;
}

static void load_settings(cnWeather *window)
{
	cnWeatherPrivate *priv;
	int x, y;

	g_return_if_fail(window != NULL && window->priv->settings != NULL);

	priv = window->priv;

	if (w_settings_get_weather(priv->settings, priv->weather))
	{
		priv->city_id = priv->weather->city_id;
	}

	weather_window_update(window);
	
	if (w_settings_get_window_size(priv->settings, &x, &y))
	{
		gtk_window_set_default_size(GTK_WINDOW(window), x, y);
	}

	if (w_settings_get_window_pos(priv->settings, &x, &y))
	{
		gtk_window_move(GTK_WINDOW(window), x, y);
	}

	if (w_settings_get_show_tray(priv->settings))
	{
		priv->tray = weather_tray_new();
		update_tray_tooltips(window);
	}
	else
	{
		priv->tray = NULL;
	}

	set_preferences_page(window);
}

static void save_settings(cnWeather *window)
{
	gint x, y;
	cnWeatherPrivate *priv;

	g_return_if_fail(window != NULL && window->priv->settings != NULL);

	priv = window->priv;

	gtk_window_get_position(GTK_WINDOW(window), &x, &y);
	w_settings_set_window_pos(priv->settings, x, y);

	gtk_window_get_size(GTK_WINDOW(window), &x, &y);
	w_settings_set_window_size(priv->settings, x, y);
}

static void set_preferences_page(cnWeather *window)
{
	GtkWidget *widget;
	cnWeatherPrivate *priv;
	gint duration;

	priv = window->priv;

	if (priv->tray)
	{
		widget = builder_get_widget(priv->ui_pref, "cb_show_tray");
		if (widget){
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		}
	}

	if (check_auto_start(window))
	{
		widget = builder_get_widget(priv->ui_pref, "cb_auto_start");
		if (widget){
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		}
	}

	if ((duration = w_settings_get_duration(priv->settings)) != 0)
	{
		widget = builder_get_widget(priv->ui_pref, "sp_duration");
		if (widget){
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), duration);
		}
	}
}

static gboolean check_auto_start(cnWeather *window)
{
	return FALSE;
}
