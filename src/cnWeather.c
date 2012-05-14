#include "cnWeather.h"
#include "intl.h"
#include "config.h"
#include "builder.h"
#include "lib/weather.h"
#include "tray.h"
#include "common.h"
#include "weatherpage.h"

#define WEATHER_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), weather_window_get_type(), cnWeatherPrivate))

#define WIN_DEFAULT_WIDTH	500
#define WIN_DEFAULT_HEIGHT	300

typedef struct _cnWeatherPrivate 
{
	GList*			cityid_list;	/* list of city id */
	gchar*			city_id;		/* city id to query */

	GtkBuilder*		ui_main;
	GtkBuilder*		ui_search;
	GtkBuilder*		ui_pref;
	GtkBuilder*		ui_weather;

	GtkWidget*		nb_weather;	/* weather notebook */

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

	gchar*			cur_theme;	/* current theme directory */

}cnWeatherPrivate;

static void weather_window_init(cnWeather *window);
static void weather_window_class_init(cnWeatherClass *klass);
static void weather_window_finalize(GObject *obj);

/**
 * delete event callback
 */
static gboolean on_delete(GtkWidget *widget, GdkEvent *event, gpointer data);

static void		search_city(cnWeather *window, const gchar *city);

/**
 * get weather information by priv->city_id
 * for city search
 */
static gpointer get_weather_thread(gpointer data);
static gpointer get_cities_db_thread(gpointer data);

/**
 * update all the weather information by priv->cityid_list
 */
static gpointer update_weather_thread(gpointer data);


static gpointer get_default_city_thread(gpointer data);

/**
 * set tray icon/tooltips from current weather information
 */
static void update_tray(cnWeather *window);

static void load_settings(cnWeather *window);
static void save_settings(cnWeather *window);

/**
 * load preferences page settings
 */
static void set_preferences_page(cnWeather *window);

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

static gchar*	get_theme_img_file(cnWeather *window, gint index);
static gint		get_cur_time_hour();

/**
 * searching themes and add to cb_themes
 * @theme_dir: theme folder
 * @count: count of cb_themes
 * @return: themes found
 */
static gint set_theme_list(cnWeather *window, const gchar *theme_dir, gint count);

static void free_weather_info(gpointer data);

static void init_weather_page(cnWeather *window, GList *list);
static void append_weather_page(cnWeather *window, WeatherInfo *wi);

static void init_private_members(cnWeatherPrivate *priv);

static void update_page(cnWeather *window, gint page, WeatherInfo *wi);

static void	query_city_id(gpointer data, const gchar **result, gint row, gint col);

static void set_weather_query_page(cnWeather *window, WeatherInfo *wi);

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
    return g_object_new(TYPE_WEATHER_WINDOW,
				"title", _("cnWeather"),
				"default-height", WIN_DEFAULT_HEIGHT,
				"default-width", WIN_DEFAULT_WIDTH,
				"height-request", 250,
				"width-request", 480,
				NULL);
}

static void weather_window_init(cnWeather *window)
{
	cnWeatherPrivate *priv;

    GtkIconTheme *icon_theme;
	GtkWidget	 *box_main;
	GtkWidget	 *page2, *page3, *page4;
	GtkWidget	 *note_book;

	window->priv = WEATHER_WINDOW_GET_PRIVATE(window);
	if (window->priv == NULL)
	{
		g_error("window->priv == NULL!!!");
	}

	priv = window->priv;

	init_private_members(priv);

	box_main = builder_get_widget(priv->ui_main, "box_main");

	page2 = builder_get_widget(priv->ui_search, "box_search");
	page3 = builder_get_widget(priv->ui_pref, "box_preferences");
	page4 = builder_get_widget(priv->ui_weather, "box_weather");

	note_book = builder_get_widget(priv->ui_main, "note_book");

	if (box_main == NULL ||
		page2 == NULL || page3 == NULL ||
		page4 == NULL || 
		note_book == NULL
		)
	{
		g_error("UI file damaged!\n");
	}

	priv->nb_weather = gtk_notebook_new();
	if (priv->nb_weather == NULL)
		g_error("gtk_notebook_new failed\n");

	gtk_notebook_set_scrollable(GTK_NOTEBOOK(priv->nb_weather), TRUE);

	priv->spinner = builder_get_widget(priv->ui_main, "sp_progress");

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

	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->nb_weather), 0);
	gtk_widget_show(priv->nb_weather);

	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), priv->nb_weather, NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), page2, NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), page3, NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), page4, NULL);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(note_book), 0);

	gtk_container_add(GTK_CONTAINER(window), box_main);

    g_signal_connect(window, "delete-event", G_CALLBACK(on_delete), NULL);
	g_signal_connect(window, "window-state-event", G_CALLBACK(on_window_state), NULL);

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
	g_timeout_add_seconds(3, delay_get_weather, window);
}

static void weather_window_class_init(cnWeatherClass *klass)
{
	GObjectClass *obj_class = (GObjectClass *)klass;

	obj_class->finalize = weather_window_finalize;

    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(cnWeatherPrivate));
}

static void weather_window_finalize(GObject *obj)
{
	cnWeather *window = WEATHER_WINDOW(obj);
	cnWeatherPrivate *priv = window->priv;

	if (priv->cityid_list)
		g_list_free_full(priv->cityid_list, g_free);

	if (priv->city_id)
		g_free(priv->city_id);

	if (priv->ui_main)
		g_object_unref(priv->ui_main);
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

	if (priv->cur_theme)
		g_free(priv->cur_theme);
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

void weather_window_get_weather(cnWeather *window, const gchar *city_id)
{
	cnWeatherPrivate *priv;

	g_return_if_fail( window != NULL );
	priv = window->priv;

	if (priv->city_id != NULL)
		g_free(priv->city_id);

	priv->city_id = g_strdup(city_id);
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
	g_return_if_fail( window != NULL && city != NULL);

	search_city(window, city);
}

static gpointer get_weather_thread(gpointer data)
{
	cnWeather *window = (cnWeather *)data;
	cnWeatherPrivate *priv = window->priv;
	wSession *ws;
	WeatherInfo *wi = NULL;

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

		wi = weather_new_info();
		wi->city_id = g_strdup(priv->city_id);

		ret = weather_get(ws, wi);

		if (ret == 0)
		{
			gdk_threads_enter();
			set_weather_query_page(window, wi);
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
		weather_free_info(wi);
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

static void search_city(cnWeather *window, const gchar *city)
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
						_("Database does not exists, you may try to update local cache from preferences page")
						);
			break;
		}

		sql = g_strdup_printf("SELECT pname, cname, tname FROM province p, city c, town t WHERE "
					"(tname LIKE '%%%s%%' OR cname LIKE '%%%s%%' OR pname LIKE '%%%s%%') "
					"AND c.cid=t.cid AND p.pid=c.pid", city, city, city);

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
	gint status = 0;

	priv->getting_db = TRUE;

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	if (weather_get_city_db(priv->db_file) != 0)
		status = weather_get_city_list(priv->db_file);

	if (status == 0)
	{
/*		guint city_id;

		city_id = priv->city_id;
		gdk_threads_enter();
		weather_window_update_pref_cb(window, CB_PROVINCE, NULL);
		gdk_threads_leave();
	
		if (city_id == 0 || city_id != priv->city_id)
		{
			gdk_threads_enter();
			weather_window_get_weather(window, city_id);
			gdk_threads_leave();

			g_usleep(3 * G_USEC_PER_SEC);

			gdk_threads_enter();
			fill_pref_cb_by_town(window);
			gdk_threads_leave();
		}*/
	}
	else
	{
		gdk_threads_enter();

		weather_window_set_search_result(window, _("Unable to get "
						"city database"));

		gdk_threads_leave();
	}

	priv->getting_db = FALSE;

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	g_thread_unref(g_thread_self());

	return NULL;
}

void weather_window_set_search_result(cnWeather *window, const gchar *text)
{
	GtkWidget *label;

	g_return_if_fail(window != NULL && text != NULL);

	label = builder_get_widget(window->priv->ui_search, "label_result");
	if (label) {
		gtk_label_set_text(GTK_LABEL(label), text);
	}

	weather_window_set_page(window, PAGE_RESULT);
}

static void update_tray(cnWeather *window)
{
	cnWeatherPrivate *priv;

	gchar *buffer = NULL;

	g_return_if_fail(window != NULL);

	priv = window->priv;

	if (window->priv->tray == NULL)
		return ;

/*	buffer = g_strdup_printf(
				"<b>%s</b>\n"
				"%s\n"
				"%s\n"
				"%s"
				,
				priv->weather->city,
				priv->weather->weather[0].weather,
				priv->weather->weather[0].temperature,
				priv->weather->weather[0].wind
			);
*/
	if (buffer == NULL)
		return ;

	weather_tray_set_tooltips(priv->tray, buffer);
	g_free(buffer);

//	buffer = get_theme_img_file(window, window->priv->weather->weather[0].img);
	if (buffer)
	{
		weather_tray_set_icon(window->priv->tray, buffer);
		g_free(buffer);
	}
}

wSettings* weather_window_get_settings(cnWeather *window)
{
	g_return_val_if_fail(window != NULL, NULL);

	return window->priv->settings;
}

static void load_settings(cnWeather *window)
{
	cnWeatherPrivate *priv;
	GList *weather_list = NULL;
	int x, y;

	g_return_if_fail(window != NULL && window->priv->settings != NULL);

	priv = window->priv;

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

	priv->cur_theme = w_settings_get_theme(priv->settings);
	if (priv->cur_theme == NULL)
		priv->cur_theme = g_strdup_printf("%s/default", THEME_DIR);

	if (g_utf8_strlen(priv->cur_theme, -1) == 0)
	{
		g_free(priv->cur_theme);
		priv->cur_theme = g_strdup_printf("%s/default", THEME_DIR);
	}

	if (w_settings_get_weather(priv->settings, &weather_list))
	{
		if (weather_list)
		{
			init_weather_page(window, weather_list);
			g_list_free_full(weather_list, free_weather_info);
		}
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
	gchar *dir;
	gint count = 0;

	priv = window->priv;

	if (priv->tray)
	{
		widget = builder_get_widget(priv->ui_pref, "cb_show_tray");
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

	dir = g_strdup_printf("%s/%s/themes", g_get_user_config_dir(), PACKAGE_NAME);
	if (dir)
	{
		count = set_theme_list(window, dir, 0);
		g_free(dir);
	}

	set_theme_list(window, THEME_DIR, count);

	widget = builder_get_widget(priv->ui_pref, "cb_themes");
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == -1)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
	}
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
	GtkWidget *widget;

	g_return_if_fail( window != NULL );

	if (!state && window->priv->tray == NULL)
		return ;

	if (state)
	{
		if (window->priv->tray == NULL)
		{
			window->priv->tray = weather_tray_new();
			weather_tray_set_main_window(window->priv->tray, GTK_WIDGET(window));
			update_tray(window);
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

	widget = builder_get_widget(window->priv->ui_pref, "cb_show_tray");
	if (widget){
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), state);
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
	cnWeatherPrivate *priv = window->priv;

	GtkWidget *tree_view;
	GtkTreeModel *model;
	GtkTreeIter iter;

	gint index = col;
	gint i;
	gchar *buffer;

	if (result == NULL || row == 0)
	{
		weather_window_set_search_result(window,
					_("No match found\nYou should try update "
					"local cache if you sure you don't have a mistype"));
		return ;
	}

	if (row == 1)
	{
		gchar *id;

		weather_window_set_page(window, PAGE_WEATHER);
		id = weather_window_query_city_id(window, result[col+2]);
		weather_window_get_weather(window, id);

		g_free(id);

		return ;
	}

	tree_view = builder_get_widget(priv->ui_search, "tv_result");
	if (tree_view == NULL)
	{
		g_warning("Missing widget: tv_result\n");
		return ;
	}

	gtk_widget_set_visible(tree_view, TRUE);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
	if (model == NULL)
	{
		g_warning("Missing tree view model!\n");
		return ;
	}
	
	gtk_list_store_clear(GTK_LIST_STORE(model));

	//pname cname tname
	for(i=0; i<row; ++i)
	{
		gchar *label;
		const gchar *pname, *cname, *tname;

		pname = result[index++];
		cname = result[index++];
		tname = result[index++];

		label = g_strdup_printf("%s %s %s", pname, cname, tname);
		if (label == NULL)
			continue;

		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, label, -1);

		g_free(label);
	}

	gtk_widget_show_all(tree_view);
	
	buffer = g_strdup_printf(_("Found %d cities"), row);
	if (buffer)
	{
		weather_window_set_search_result(window, buffer);
		g_free(buffer);
	}
	else
	  weather_window_set_search_result(window, _("Result"));
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

void weather_window_update_pref_cb_by_town(cnWeather *window, gchar *name)
{
	g_return_if_fail(window != NULL && name != NULL);
/*
	if (window->priv->weather->city)
	{
		g_free(window->priv->weather->city);
		window->priv->weather->city = NULL;
	}

	window->priv->weather->city = g_strdup(name);
*/
	fill_pref_cb_by_town(window);
}

static void fill_pref_cb_by_town(cnWeather *window)
{
/*	gchar *sql = NULL;

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
	  g_free(sql);*/
	fill_by_town(NULL, NULL, 0, 0);
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
	cnWeatherPrivate *priv = window->priv;

	priv->ready_for_weather = TRUE;

	/* No city */
	/* try to get default city by ip */
	if (priv->cityid_list == NULL)
	{
		g_thread_new("GetDefaultCity", get_default_city_thread, window);
	}
	else 
	{
		get_weather_timer(window);
	}

	return FALSE;
}

static gboolean get_weather_timer(gpointer data)
{
	cnWeather *window = (cnWeather *)data;

	if (window->priv->ready_for_weather)
		g_thread_new("GetWeathers", update_weather_thread, window);

	return TRUE;
}

void weather_window_hide_result_tv(cnWeather *window)
{
	GtkWidget *widget;

	g_return_if_fail(window != NULL);

	widget = builder_get_widget(window->priv->ui_search, "tv_result");
	if (widget)
		gtk_widget_set_visible(widget, FALSE);
}

static gchar* get_theme_img_file(cnWeather *window, gint index)
{
	cnWeatherPrivate *priv;
	gchar *file = NULL;
	gint hour;

	g_return_val_if_fail(window != NULL, NULL);
	if (window->priv->cur_theme == NULL)
		return NULL;

	priv = window->priv;

	hour = get_cur_time_hour();
	if ((hour >= 18 && hour <= 24) || (hour >= 0 && hour <= 6))
		file =  g_strdup_printf("%s/n%02d.png", priv->cur_theme, index);
	else
		file =  g_strdup_printf("%s/%02d.png", priv->cur_theme, index);

	g_return_val_if_fail(file != NULL, NULL);

	if (!g_file_test(file, G_FILE_TEST_EXISTS))
	{
		if ((hour >= 18 && hour <= 24) || (hour >= 0 && hour <= 6))
		{
			g_free(file);
			file = g_strdup_printf("%s/%02d.png", priv->cur_theme, index);
			g_return_val_if_fail( file != NULL, NULL);

			if (!g_file_test(file, G_FILE_TEST_EXISTS))
			{
				g_free(file);
				return NULL;
			}
		}
		else
		{
			g_free(file);
			file = NULL;
		}
	}

	return file;
}

static gint get_cur_time_hour()
{
	GDateTime *dt;
	gint hour;

	dt = g_date_time_new_now_local();
	if (dt == NULL)
		return 0;

	hour = g_date_time_get_hour(dt);

	g_date_time_unref(dt);

	return hour;
}

static gint set_theme_list(cnWeather *window, const gchar *theme_dir, gint count)
{
	cnWeatherPrivate *priv;
	GtkWidget *widget;

    GDir *dir;
    const gchar *name;
    gchar *full_path;
	gint i = 0;

	g_return_val_if_fail(window != NULL, 0);
	priv = window->priv;

	widget = builder_get_widget(priv->ui_pref, "cb_themes");
	if (widget == NULL)
	{
		g_warning("Missing widget cb_themes\n");
		return 0;
	}

    dir = g_dir_open(theme_dir, 0, NULL);
    if (NULL == dir)
        return 0;

    while((name = g_dir_read_name(dir)))
    {
        full_path = g_build_filename(theme_dir, name, NULL);
        if (g_file_test(full_path, G_FILE_TEST_IS_DIR))
        {
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), full_path, name);
			if (g_strcmp0(full_path, priv->cur_theme) == 0)
			{
				gtk_combo_box_set_active(GTK_COMBO_BOX(widget), i + count);
			}

			i++;
        }
        g_free(full_path);
    }

    g_dir_close(dir);

	return i;
}

void weather_window_set_theme(cnWeather *window, const gchar *theme)
{
	g_return_if_fail( window != NULL && theme != NULL);

	if (window->priv->cur_theme)
		g_free(window->priv->cur_theme);

	window->priv->cur_theme = g_strdup(theme);

	weather_window_get_weather(window, 0);

	w_settings_set_theme(window->priv->settings, theme);
}

static void free_weather_info(gpointer data)
{
	weather_free_info((WeatherInfo *)data);
	g_free(data);
}

static void init_weather_page(cnWeather *window, GList *list)
{
	GList *p;

	p = list;
	while(p)
	{
		WeatherInfo *info;
		
		info = (WeatherInfo *)p->data;

		append_weather_page(window, info);

		p = p->next;
	}
}

static void append_weather_page(cnWeather *window, WeatherInfo *wi)
{
	cnWeatherPrivate *priv;
	GtkWidget *page;

	priv = window->priv;

	page = weather_page_new();

	gtk_notebook_append_page(GTK_NOTEBOOK(priv->nb_weather), page, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(priv->nb_weather), page, wi->city);

	priv->cityid_list = g_list_append(priv->cityid_list, g_strdup(wi->city_id));

	update_page(window, gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->nb_weather)) - 1, wi);
}

static void init_private_members(cnWeatherPrivate *priv)
{
	priv->cityid_list = NULL;
	priv->tray = NULL;
	priv->quit_from_tray = FALSE;

	priv->settings = w_settings_new();

	priv->ui_main = builder_new(UI_DIR"/main.ui");
	priv->ui_search = builder_new(UI_DIR"/search.ui");
	priv->ui_pref = builder_new(UI_DIR"/preferences.ui");
	priv->ui_weather = builder_new(UI_DIR"/weather.ui");

	if (priv->ui_main == NULL	||
		priv->ui_search == NULL	||
		priv->ui_pref == NULL	||
		priv->ui_weather == NULL
		)
	{
		g_error("UI files missing!\n");
	}

	priv->mutex = g_malloc(sizeof(GMutex));
	if (priv->mutex == NULL)
		g_error("g_malloc failed (%s, %d)\n", __FILE__, __LINE__);

	g_mutex_init(priv->mutex);

	priv->getting_weather = FALSE;
	priv->getting_db = FALSE;

	priv->ready_for_weather = FALSE;
	priv->timer = 0;
	priv->cur_theme = NULL;
	priv->city_id = NULL;
}

static gpointer update_weather_thread(gpointer data)
{
	cnWeather *window;
	cnWeatherPrivate *priv;
	GList *p;
	gint tab;
	GList *list = NULL;

	window = (cnWeather *)data;
	priv = window->priv;
	p = priv->cityid_list;

	g_mutex_lock(priv->mutex);
	priv->getting_weather = TRUE;
	g_mutex_unlock(priv->mutex);

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	tab = 0;
	while(p)
	{
		WeatherInfo *wi;
		wSession *ws;

		wi = weather_new_info();
		ws = weather_open();

		wi->city_id = g_strdup((gchar *)p->data);
		if (weather_get(ws, wi) == 0)
		{
			gdk_threads_enter();
			update_page(window, tab, wi);
			gdk_threads_leave();

			list = g_list_append(list, wi);
		}
		else
		{
			gdk_threads_enter();
			weather_window_set_search_result(window, 
						_("Couldn't get weather! \nPlease check out your networks"));
			gdk_threads_leave();
		}

		weather_close(ws);
		
		p = p->next;
		tab++;
	}

	if (list)
	{
		w_settings_set_weather(window->priv->settings, list);
		g_list_free_full(list, free_weather_info);
	}

	g_mutex_lock(priv->mutex);
	priv->getting_weather = FALSE;
	g_mutex_unlock(priv->mutex);

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	g_thread_unref(g_thread_self());

	return NULL;
}

static void update_page(cnWeather *window, gint page, WeatherInfo *wi)
{
	GtkWidget *widget;
	gint i;

	widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window->priv->nb_weather), page);
	if (widget)
	{
		weather_page_set_weather_info(WEATHER_PAGE(widget), wi);

		for(i=0; i<3; ++i)
		{
			gchar *image;
			image = get_theme_img_file(window, wi->weather[i].img);
			if (image)
			{
				weather_page_set_image(WEATHER_PAGE(widget), i, image);
				g_free(image);
			}
		}
	}
}

static gpointer get_default_city_thread(gpointer data)
{
	cnWeather *window;
	cnWeatherPrivate *priv;

	wSession *ws;
	WeatherInfo *wi;

	window = (cnWeather *)data;
	priv = window->priv;

	g_mutex_lock(priv->mutex);
	priv->getting_weather = TRUE;
	g_mutex_unlock(priv->mutex);

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	wi = weather_new_info();
	ws = weather_open();

	if (weather_get(ws, wi) == 0)
	{
		GList *list = NULL;

		gdk_threads_enter();
		append_weather_page(window, wi);
		gdk_threads_leave();

		list = g_list_append(list, wi);
		if (list)
		{
			w_settings_set_weather(window->priv->settings, list);
			g_list_free(list);
		}
	}
	else
	{
		gdk_threads_enter();
		weather_window_set_search_result(window, 
						_("Couldn't get weather! \nPlease check out your networks"));
		gdk_threads_leave();
	}

	weather_close(ws);
	weather_free_info(wi);

	g_mutex_lock(priv->mutex);
	priv->getting_weather = FALSE;
	g_mutex_unlock(priv->mutex);

	gdk_threads_enter();
	update_progress(window);
	gdk_threads_leave();

	g_thread_unref(g_thread_self());

	return NULL;
}

void weather_window_refresh(cnWeather *window)
{
	if (!window->priv->getting_weather)
		get_weather_timer(window);
}

gchar* weather_window_query_city_id(cnWeather *window, const gchar *city)
{
	gchar *sql;
	gchar *id = NULL;

	sql = g_strdup_printf("SELECT city_id FROM town WHERE tname='%s'", city);
	g_return_val_if_fail( sql != NULL, 0);

	sql_query(window->priv->db_file, sql, query_city_id, &id);

	g_free(sql);

	return id;
}

static void	query_city_id(gpointer data, const gchar **result, gint row, gint col)
{
	if (result == NULL || row == 0)
		return ;

	*((gchar **)data) = g_strdup(result[col]);
}

static void set_weather_query_page(cnWeather *window, WeatherInfo *wi)
{
	cnWeatherPrivate *priv;
	GtkWidget *widget;
	gint i;
	static const gchar *names[] =
	{
		"label_weather", "label_temperature", "image_weather"
	};
	gchar name[30];

	priv = window->priv;

	widget = builder_get_widget(priv->ui_weather, "label_city");
	gtk_label_set_text(GTK_LABEL(widget), wi->city);

	for (i=0; i<3; ++i)
	{
		gchar *image;

		g_snprintf(name, 30, "%s%d", names[0], i+1);
		widget = builder_get_widget(priv->ui_weather, name);
		gtk_label_set_text(GTK_LABEL(widget), wi->weather[i].weather);

		g_snprintf(name, 30, "%s%d", names[1], i+1);
		widget = builder_get_widget(priv->ui_weather, name);
		gtk_label_set_text(GTK_LABEL(widget), wi->weather[i].temperature);

		g_snprintf(name, 30, "%s%d", names[2], i+1);
		widget = builder_get_widget(priv->ui_weather, name);
		gtk_widget_set_tooltip_text(widget, wi->weather[i].wind);

		image = get_theme_img_file(window, wi->weather[i].img);
		if (image)
		{
			gtk_image_set_from_file(GTK_IMAGE(widget), image);
			g_free(image);
		}
	}

	// change to query result page
	weather_window_set_page(window, PAGE_WEATHER_QUERY);
}

void weather_window_add_query_city(cnWeather *window)
{
	cnWeatherPrivate *priv;
	WeatherInfo *wi;
	GtkWidget *widget;

	gint i;
	static const gchar *names[] =
	{
		"label_weather", "label_temperature", "image_weather"
	};
	gchar name[30];

	priv = window->priv;

	// should never happen
	g_return_if_fail(priv->city_id != NULL);

	wi = weather_new_info();
	g_return_if_fail( wi != NULL );

	// make weather info from query result
	wi->city_id = g_strdup(priv->city_id);

	widget = builder_get_widget(priv->ui_weather, "label_city");
	wi->city = g_strdup(gtk_label_get_text(GTK_LABEL(widget)));

	for(i=0; i<3; ++i)
	{
		gchar *image = NULL;

		g_snprintf(name, 30, "%s%d", names[0], i+1);
		widget = builder_get_widget(priv->ui_weather, name);
		wi->weather[i].weather = g_strdup(gtk_label_get_text(GTK_LABEL(widget)));

		g_snprintf(name, 30, "%s%d", names[1], i+1);
		widget = builder_get_widget(priv->ui_weather, name);
		 wi->weather[i].temperature = g_strdup(gtk_label_get_text(GTK_LABEL(widget)));

		g_snprintf(name, 30, "%s%d", names[2], i+1);
		widget = builder_get_widget(priv->ui_weather, name);
		wi->weather[i].wind = gtk_widget_get_tooltip_text(widget);

		// 
		g_object_get(widget, "file", &image, NULL);
		if (image)
		{
			gchar *name;
			name = g_path_get_basename(image);
			if (name)
			{
				gchar *p = name;
				gchar *digit;
				if (*p == 'n') //nXXX.png
					p++;

				digit = p;
				while (*p != '.')
				   p++;
				*p = 0;

				wi->weather[i].img = g_strtod(digit, NULL);

				g_free(name);
			}
			g_free(image);
		}
	}

	append_weather_page(window, wi);

	weather_free_info(wi);

	// switch to weather page
	// and change to the last tab
	weather_window_set_page(window, PAGE_WEATHER);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->nb_weather),
				gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->nb_weather)) - 1
				);

	// update so it can save the list
	get_weather_timer(window);
}
