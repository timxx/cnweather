#include "cnWeather.h"
#include "intl.h"
#include "config.h"
#include "builder.h"
#include "lib/weather.h"
#include "tray.h"
#include "common.h"

#define WEATHER_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), weather_window_get_type(), cnWeatherPrivate))

#define WIN_DEFAULT_WIDTH	500
#define WIN_DEFAULT_HEIGHT	300

typedef struct _cnWeatherPrivate 
{
    guint			city_id;
	WeatherInfo*	weather;	/* current city weather information */

	GtkBuilder*		ui_main;
	GtkBuilder*		ui_search;
	GtkBuilder*		ui_weather;
	GtkBuilder*		ui_pref;

	WeatherTray*	tray;

	wSettings*		settings;

	gboolean		is_maximized;	/* for window maximized state */

	gchar*			db_file;		/* local city database file path */

	gboolean		quit_from_tray;	/* indicate signal from tray menu */

	GtkWidget*		spinner;

	GMutex*			mutex;

	gboolean		getting_weather;
	gboolean		getting_db;

	gboolean		ready_for_weather;	/* for delay getting weather at startup time */

	guint			timer;	/* timer to repeatly gets weather */

}cnWeatherPrivate;

static void weather_window_init(cnWeather *window);
static void weather_window_class_init(cnWeatherClass *klass);
static void weather_window_finalize(GObject *obj);

/**
 * delete event callback
 */
static gboolean on_delete(GtkWidget *widget, GdkEvent *event, gpointer data);

static gpointer get_weather_thread(gpointer data);
static void		search_city(cnWeather *window);
static gpointer get_cities_db_thread(gpointer data);

/**
 * set tray tooltips from current weather information
 */
static void update_tray_tooltips(cnWeather *window);

static void load_settings(cnWeather *window);
static void save_settings(cnWeather *window);

/**
 * load preferences page settings
 */
static void set_preferences_page(cnWeather *window);

static gboolean check_auto_start(cnWeather *window);

/**
 * window-state event callback
 */
static gboolean on_window_state(GtkWidget *widget,
			GdkEventWindowState  *event,
			gpointer   data);

static void update_progress(cnWeather *window);

static void valid_window_size(cnWeather *window, gint *w, gint *h);
static void valid_window_pos(cnWeather *window, gint *x, gint *y);

static void query_db_city(gpointer data, const gchar **result, gint row, gint col);

static void fill_province(cnWeather *window);
static void fill_city(cnWeather *window, const gchar *province);
static void fill_town(cnWeather *window, const gchar *city);

static void fill_province_real(gpointer data, const gchar **result, gint row, gint col);
static void fill_city_real(gpointer data, const gchar **result, gint row, gint col);
static void fill_town_real(gpointer data, const gchar **result, gint row, gint col);

static void fill_cb(gpointer data, const gchar **result, gint row, gint col, gchar *name);

static void fill_pref_cb_by_town(cnWeather *window);
static void fill_by_town(gpointer data, const gchar **result, gint row, gint col);

/**
 * search combobox text widget string list
 * @text: string to search
 * @iter: iter
 * @return: iter index or -1 if not found
 */
static gint cb_text_search(GtkComboBox *cb, const gchar *text, GtkTreeIter *iter);

static gboolean delay_load_settings(gpointer data);
static gboolean delay_get_weather(gpointer data);

static gboolean get_weather_timer(gpointer data);

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
	priv->quit_from_tray = FALSE;

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

	priv->mutex = g_malloc(sizeof(GMutex));
	g_mutex_init(priv->mutex);

	priv->getting_weather = FALSE;
	priv->getting_db = FALSE;

	priv->ready_for_weather = FALSE;
	priv->timer = 0;

	priv->spinner = builder_get_widget(priv->ui_main, "sp_progress");

    gtk_window_set_title(GTK_WINDOW(window), _("cnWeather"));
	gtk_window_set_default_size(GTK_WINDOW(window), WIN_DEFAULT_WIDTH, WIN_DEFAULT_HEIGHT);
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
	g_signal_connect(window, "window-state-event", G_CALLBACK(on_window_state), NULL);

	priv->city_id = 0;

	priv->db_file = g_strdup_printf("%s/%s/data/cities.db", g_get_user_config_dir(), PACKAGE_NAME);
	if (priv->db_file == NULL)
		g_warning("failed to alloc memory (%s, %d)\n", __FILE__, __LINE__);
	else
	{
		if (!g_file_test(priv->db_file, G_FILE_TEST_EXISTS))
		{
			gchar *tmp = g_strdup_printf("%s/%s/data/", g_get_user_config_dir(), PACKAGE_NAME);
			if (tmp)
			{
				if (g_mkdir_with_parents(tmp, 0744) == 0)
					weather_window_update_cache(window);

				g_free(tmp);
			}
		}
	}

	update_progress(window);
	g_timeout_add(10, delay_load_settings, window);
	g_timeout_add_seconds(5, delay_get_weather, window);
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

	if (priv->ui_main)
		g_object_unref(priv->ui_main);
	if (priv->ui_weather)
		g_object_unref(priv->ui_weather);
	if (priv->ui_search)
		g_object_unref(priv->ui_search);

	if (priv->settings)
		g_object_unref(priv->settings);

	if (priv->db_file)
		g_free(priv->db_file);

	if (priv->mutex)
	{
		g_mutex_clear(priv->mutex);
		g_free(priv->mutex);
	}

	if (priv->timer)
		g_source_remove(priv->timer);
}

static gboolean on_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	cnWeather *window = WEATHER_WINDOW(widget);
	cnWeatherPrivate *priv = window->priv;

	if (!priv->quit_from_tray && priv->tray &&
		gtk_status_icon_get_visible(GTK_STATUS_ICON(priv->tray)))
	{
		gtk_widget_hide(widget);

		return TRUE;
	}

	priv->quit_from_tray = FALSE;

	if (priv->getting_db)
	{
		gint response;

		response = confirm_dialog(widget,
						_("Getting city list data... Are you sure quit now?"),
						_("Quit?")
					);
		if (response == GTK_RESPONSE_NO)
		{
			return TRUE;
		}
	}

	save_settings(window);

	if (priv->tray)
		g_object_unref(priv->tray);

	gtk_widget_destroy(widget);

    return FALSE;
}

void weather_window_get_weather(cnWeather *window, guint city_id)
{
	g_return_if_fail( window != NULL );

	window->priv->city_id = city_id;

	if (window->priv->ready_for_weather)
		g_thread_new("GetWeather", get_weather_thread, window);
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

	search_city(window);
}

static gpointer get_weather_thread(gpointer data)
{
	cnWeather *window = (cnWeather *)data;
	cnWeatherPrivate *priv = window->priv;
	wSession *ws;

	g_mutex_lock(priv->mutex);
	priv->getting_weather = TRUE;
	g_mutex_unlock(priv->mutex);

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	do
	{
		int ret;
		ws = weather_open();
		if (ws == NULL)
			break;

		g_mutex_lock(priv->mutex);

		if (priv->city_id == 0)
			ret = weather_get_default_city(ws, priv->weather);
		else
			ret = weather_get(ws, priv->city_id, priv->weather);

		g_mutex_unlock(priv->mutex);

		if (ret == 0)
		{
			g_mutex_lock(priv->mutex);
			priv->city_id = priv->weather->city_id;
			g_mutex_unlock(priv->mutex);

			gdk_threads_enter();
			weather_window_update(window);
			gdk_threads_leave();
		}
		else
		{
			gdk_threads_enter();
			weather_window_set_search_result(window, 
						_("Couldn't get weather! \nPlease check out your networks"));
			gdk_threads_leave();
		}
	
		weather_close(ws);
	}
	while(0);

	g_mutex_lock(priv->mutex);
	priv->getting_weather = FALSE;
	g_mutex_unlock(priv->mutex);

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	g_thread_unref(g_thread_self());

	return NULL;
}

static void search_city(cnWeather *window)
{
	cnWeatherPrivate *priv = window->priv;
	gchar *sql = NULL;

	update_progress(window);
	do
	{
		if (priv->db_file == NULL ||
			!g_file_test(priv->db_file, G_FILE_TEST_EXISTS)
		 )
		{
			weather_window_set_search_result(window, 
						_("Database not exists, you may try to update local cache from preferences page")
						);
			break;
		}

		sql = g_strdup_printf("SELECT * FROM town WHERE tname LIKE '%%%s%%'", priv->weather->city);

		sql_query(priv->db_file, sql, query_db_city, window);
	}
	while(0);

	if (sql) {
		g_free(sql);
	}

	update_progress(window);
}

static gpointer get_cities_db_thread(gpointer data)
{
	cnWeather *window = (cnWeather*)data;
	cnWeatherPrivate *priv = window->priv;

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	weather_get_city_list(priv->db_file);

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	g_thread_unref(g_thread_self());

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

	w_settings_set_weather(priv->settings, priv->weather);
}

void weather_window_set_search_result(cnWeather *window, const gchar *text)
{
	GtkWidget *label;

	g_return_if_fail(window != NULL && text != NULL);

	label = builder_get_widget(window->priv->ui_search, "label_result");
	if (label) {
		gtk_label_set_text(GTK_LABEL(label), text);
	}

	weather_window_set_page(window, PAGE_SEARCH);
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
		valid_window_size(window, &x, &y);
		gtk_window_set_default_size(GTK_WINDOW(window), x, y);
	}

	if (w_settings_get_window_pos(priv->settings, &x, &y))
	{
		valid_window_pos(window, &x, &y);
		gtk_window_move(GTK_WINDOW(window), x, y);
	}

	if (w_settings_get_window_state(priv->settings))
		gtk_window_maximize(GTK_WINDOW(window));

	if (w_settings_get_show_tray(priv->settings))
	{
		weather_window_show_tray(window, TRUE);
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

	w_settings_set_window_state(priv->settings, priv->is_maximized);

	if (!priv->is_maximized)
	{
		gtk_window_get_position(GTK_WINDOW(window), &x, &y);
		w_settings_set_window_pos(priv->settings, x, y);

		gtk_window_get_size(GTK_WINDOW(window), &x, &y);
		w_settings_set_window_size(priv->settings, x, y);
	}
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

	if ((duration = w_settings_get_duration(priv->settings)) > 0)
	{
		widget = builder_get_widget(priv->ui_pref, "sp_duration");
		if (widget){
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), duration);
		}
	}
	else
	  duration = 5;	/* set default value */

	weather_window_set_duration(window, duration);

	if (priv->db_file != NULL &&
		g_file_test(priv->db_file, G_FILE_TEST_EXISTS))
	{
		weather_window_update_pref_cb(window, CB_PROVINCE, NULL);
		fill_pref_cb_by_town(window);
	}
}

static gboolean check_auto_start(cnWeather *window)
{
	return FALSE;
}

static gboolean on_window_state(GtkWidget *widget,
			GdkEventWindowState  *event,
			gpointer   data
			)
{
	cnWeather *window = WEATHER_WINDOW(widget);
	cnWeatherPrivate *priv = window->priv;

	if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
		priv->is_maximized = TRUE;
	}else{
		priv->is_maximized = FALSE;
	}

	return FALSE;
}

void weather_window_show_tray(cnWeather *window, gboolean state)
{
	g_return_if_fail( window != NULL );

	if (!state && window->priv->tray == NULL)
		return ;

	if (state)
	{
		if (window->priv->tray == NULL)
		{
			window->priv->tray = weather_tray_new();
			weather_tray_set_main_window(window->priv->tray, GTK_WIDGET(window));
			update_tray_tooltips(window);
		}
		else
		{
			gtk_status_icon_set_visible(GTK_STATUS_ICON(window->priv->tray), TRUE);
		}
	}
	else
	{
		gtk_status_icon_set_visible(GTK_STATUS_ICON(window->priv->tray), FALSE);
	}
}

void weather_window_quit(cnWeather *window)
{
	window->priv->quit_from_tray = TRUE;
	on_delete(GTK_WIDGET(window), NULL, NULL);
}

static void update_progress(cnWeather *window)
{
	cnWeatherPrivate *priv;
	gchar *tips[3] = { NULL };
	gint count = 0;

	g_return_if_fail( window != NULL && window->priv->spinner != NULL);

	priv = window->priv;

	if (priv->getting_weather)
	{
		tips[count++] = _("Getting weather in progress\n");
	}

	if (priv->getting_db)
	{
		tips[count++] = _("Getting city list in progress\n");
	}

	if (count == 0)
	{
		gtk_spinner_stop(GTK_SPINNER(priv->spinner));
		gtk_widget_set_visible(priv->spinner, FALSE);
	}
	else
	{
		gchar *spinner_tips;

		gtk_widget_set_visible(priv->spinner, TRUE);

		spinner_tips = g_strdup_printf(_("<b>Background task(s): %d\n</b>%s%s"),
					count, tips[0] ? tips[0] : "",
					tips[1] ? tips[1] : ""
					);
		if (spinner_tips)
		{
			gtk_widget_set_tooltip_markup(priv->spinner, spinner_tips);
			g_free(spinner_tips);
		}
		gtk_spinner_start(GTK_SPINNER(priv->spinner));
	}
}

void weather_window_update_cache(cnWeather *window)
{
	g_return_if_fail( window != NULL);

	if (!window->priv->getting_db)
		g_thread_new("GetCitisDb", get_cities_db_thread, window);
}

static void valid_window_size(cnWeather *window, gint *w, gint *h)
{
	GdkScreen *screen;
	gint scrx, scry;

	screen 	= gtk_window_get_screen(GTK_WINDOW(window));
	scrx 	= gdk_screen_get_width (screen);
	scry 	= gdk_screen_get_height(screen);

	if (*w > scrx)
	   *w = WIN_DEFAULT_WIDTH;
	if (*h > scry)
	   *h = WIN_DEFAULT_HEIGHT;
}

static void valid_window_pos(cnWeather *window, gint *x, gint *y)
{
	if (*x < 0)
		*x = 0;
	if (*y < 0)
		*y = 0;
}

static void query_db_city(gpointer data, const gchar **result, gint row, gint col)
{
	cnWeather *window = (cnWeather *)data;
//	cnWeatherPrivate *priv = window->priv;

	gint index = col;
	gint i;

	g_print("row: %d | col: %d\n", row, col);

	if (result == NULL || row == 0)
	{
		weather_window_set_search_result(window,
					_("No match found\nYou should try update "
					"local cache if you sure you don't have a mistype"));
		return ;
	}

	for(i=0; i<row; ++i)
	{
		gint j;
		for(j=0; j<col; ++j)
			g_print("%s ", result[index++]);
		g_print("\n");
	}
}

void weather_window_set_duration(cnWeather *window, gint duration)
{
	cnWeatherPrivate *priv;

	g_return_if_fail( window != NULL);
	priv = window->priv;

	if (priv->timer != 0)
		g_source_remove(priv->timer);

	priv->timer = g_timeout_add_seconds(duration * 60, get_weather_timer, window); 

	w_settings_set_duration(priv->settings, duration);
}

static void fill_province(cnWeather *window)
{
	sql_query(window->priv->db_file,
				"SELECT pname FROM province",
				fill_province_real,
				window);
}

static void fill_city(cnWeather *window, const gchar *province)
{
	gchar *sql;

	sql = g_strdup_printf("SELECT cname FROM city c,"
				"province p WHERE p.pname='%s' AND c.pid=p.pid",
				province);

	if (sql == NULL)
		return ;

	sql_query(window->priv->db_file, sql, fill_city_real, window);

	g_free(sql);
}

static void fill_town(cnWeather *window, const gchar *city)
{
	gchar *sql;

	sql = g_strdup_printf("SELECT city_id, tname FROM city c,"
				"town t WHERE c.cname='%s' AND t.cid=c.cid",
				city);

	if (sql == NULL)
		return ;

	sql_query(window->priv->db_file, sql, fill_town_real, window);

	g_free(sql);
}

static void fill_province_real(gpointer data, const gchar **result, gint row, gint col)
{
	fill_cb(data, result, row, col, "cb_province");
}

static void fill_city_real(gpointer data, const gchar **result, gint row, gint col)
{
	fill_cb(data, result, row, col, "cb_city");
}

static void fill_town_real(gpointer data, const gchar **result, gint row, gint col)
{
	cnWeather *window = (cnWeather *)data;
	GtkWidget *widget;
	gint i, j;

	if (result == NULL || row == 0)
		return ;

	widget = builder_get_widget(window->priv->ui_pref, "cb_town");
	if (widget == NULL)
	{
		g_warning("Missing widget: cb_town\n");
		return;
	}

	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(widget));

	j = col;
	for(i=0; i<row; ++i)
	{
		const gchar *id = result[j++];
		const gchar *name = result[j++];

		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), id, name);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
}

static void fill_cb(gpointer data, const gchar **result, gint row, gint col, gchar *name)
{
	cnWeather *window = (cnWeather *)data;
	GtkWidget *widget;
	gint i, j;

	if (result == NULL || row == 0)
		return ;

	widget = builder_get_widget(window->priv->ui_pref, name);
	if (widget == NULL)
	{
		g_warning("Missing widget: %s\n", name);
		return;
	}

	j = col;
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(widget));
	for(i=0; i<row; ++i)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), result[j++]);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
}

void weather_window_update_pref_cb(cnWeather *window, gint cb, gchar *name)
{
	g_return_if_fail( window != NULL);

	switch(cb)
	{
		case CB_PROVINCE:	fill_province(window);		break;
		case CB_CITY:		fill_city(window, name);	break;
		case CB_TOWN:		fill_town(window, name);	break;
	}
}

static void fill_pref_cb_by_town(cnWeather *window)
{
	gchar *sql = NULL;

	g_return_if_fail( window != NULL);

	if (window->priv->weather->city == NULL)
		return ;

	sql = g_strdup_printf("SELECT pname, cname FROM province p, city c, town t "
				"WHERE t.tname='%s' AND c.cid=t.cid AND c.pid=p.pid",
				window->priv->weather->city);

	if (sql == NULL)
	{
		g_warning("g_strdup_printf failed (%s, %d)\n", __FILE__, __LINE__);
		return ;
	}

	if (sql_query(window->priv->db_file, sql, fill_by_town, window) == 0)
	{
		GtkWidget *widget;

		widget = builder_get_widget(window->priv->ui_pref, "cb_town");
		if (widget)
		{
			gint idx;
			GtkTreeIter iter;

			idx = cb_text_search(GTK_COMBO_BOX(widget), window->priv->weather->city, &iter);
			if (idx >= 0)
				gtk_combo_box_set_active(GTK_COMBO_BOX(widget), idx);
		}
	}

	if (sql)
	  g_free(sql);
}

static void fill_by_town(gpointer data, const gchar **result, gint row, gint col)
{
	cnWeatherPrivate *priv;
	GtkWidget *widget;
	gint idx;
	gint i;

	static gchar *cb_names[] = { "cb_province", "cb_city" };

	if (result == NULL || row == 0)
	{
		g_print("No information returned(%s, %d)\n", __FILE__, __LINE__);
		return ;
	}

	priv = ((cnWeather *)data)->priv;

	for(i=0; i<2; i++)
	{
		GtkTreeIter iter;

		widget = builder_get_widget(priv->ui_pref, cb_names[i]);
		if (widget == NULL)
		{
			g_warning("Missing widget %s\n", cb_names[i]);
			return; 
		}

		idx = cb_text_search(GTK_COMBO_BOX(widget), result[col + i], &iter);
		if (idx == -1)
		{
			g_warning("No match item found\n");
			return ;
		}

		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), &iter);
	}
}

static gint cb_text_search(GtkComboBox *cb, const gchar *text, GtkTreeIter *iter)
{
    GtkTreeModel *model;
    gchar *cur_text = NULL;
	gint idx = 0;

    model = gtk_combo_box_get_model(cb);

    if (gtk_tree_model_get_iter_first(model, iter))
    {
        do
        {
            gtk_tree_model_get(model, iter, 0, &cur_text, -1);

            if (g_strcmp0(cur_text, text) == 0)
			{
				g_free(cur_text);
				return idx;
			}

            g_free(cur_text);
			idx++;
        }
        while (gtk_tree_model_iter_next(model, iter));
    }

	return -1;
}

static gboolean delay_load_settings(gpointer data)
{
	cnWeather *window = (cnWeather *)data;

	load_settings(window);

	return FALSE;
}

static gboolean delay_get_weather(gpointer data)
{
	cnWeather *window = (cnWeather *)data;

	window->priv->ready_for_weather = TRUE;

	weather_window_get_weather(window, window->priv->city_id);

	return FALSE;
}

static gboolean get_weather_timer(gpointer data)
{
	cnWeather *window = (cnWeather *)data;

	weather_window_get_weather(window, window->priv->city_id);

	return TRUE;
}
