#include <gtk/gtk.h>
#include <getopt.h>
#include <stdlib.h> /* for exit status */
#include <curl/curl.h>
#include <signal.h>

#include "intl.h"
#include "config.h"
#include "cnWeather.h"
#include "lib/weather.h"
#include "common.h"

GtkWidget *main_window = NULL;
static gboolean is_tray_mode = FALSE;

static struct option long_opts[] =
{
	{"no-gui",		no_argument,		0,	'g'},
	{"city",		required_argument,	0,	'c'},
	{"city-id",		required_argument,	0,	'C'},
	{"tray",		no_argument,		0,	't'},
	{"update-cache",no_argument,		0,	'u'},
	{"version",		no_argument,		0,	'v'},
	{"help",		no_argument,		0,	'h'},
	{0, 0, 0, 0}
};

static const gchar *opts = "gc:C:tuvh";

static void on_activate(GtkApplication *app);

static int do_xterm(int argc, char **argv);

static void show_version();
static void show_help();

static gint		update_cache();

static gint		get_weather(const gchar *city_id, WeatherInfo *wi);
static gchar*	get_city_id(const gchar *city);

static void		print_weather_info(WeatherInfo *wi);

static void		query_city_id(gpointer data, const gchar **result, gint row, gint col);

static void		signal_handler(int signum);

static void		print_get_weather_fail_info(gint code);

static gint		list_city_weather(GList *list, wSettings *sett);
static gint		get_city_weather(const gchar *city_id);

int main(int argc, char **argv)
{
    GtkApplication *app;
    gint status;

	/* call setlocale so that terminal can g_print
	 * non-english chars without gtk
	 */
	setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

	curl_global_init(CURL_GLOBAL_ALL);

	if (argc > 1)
	{
		status = do_xterm(argc, argv);
		if (!is_tray_mode)
		{
			curl_global_cleanup();
			return status;
		}
	}

#if !GLIB_CHECK_VERSION(2, 32, 0)
	if (!g_thread_supported())
		g_thread_init(NULL);
#endif

    app = gtk_application_new("org.timxx.cnWeather", 0);
    if (app == NULL)
    {
        g_error("cnWeather: create app failed (%s, %d)\n", __FILE__, __LINE__);
		curl_global_cleanup();
        return -1;
    }

    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGABRT, signal_handler);

    status = g_application_run(G_APPLICATION(app), 0, NULL);

    g_object_unref(app);

	curl_global_cleanup();

	return status;
}

static void on_activate(GtkApplication *app)
{
    GList *list;
    list = gtk_application_get_windows(app);

    if (list)
    {
        gtk_window_present(GTK_WINDOW(list->data));
    }
    else
    {
        main_window = weather_window_new();
        if (main_window == NULL)
        {
            g_error("cnWeather: Unable to create main window(%s, %d)\n", __FILE__, __LINE__);
        }
        gtk_window_set_application(GTK_WINDOW(main_window), app);

		if (!is_tray_mode)
			gtk_widget_show_all(main_window);
		else
			weather_window_show_tray(WEATHER_WINDOW(main_window), TRUE);
    }
}

static int do_xterm(int argc, char **argv)
{
	gint c;
	gint status = EXIT_FAILURE;
	gboolean no_gui = FALSE;
	gchar *city = NULL;
	gchar *city_id = NULL;
	gchar *target_city_id = NULL;

	gboolean need_free_cityid = FALSE;
	
	GList *city_id_list = NULL;

	wSettings *sett = NULL;

	while( (c=getopt_long(argc, argv, opts, long_opts, NULL)) != -1)
	{
		switch(c)
		{
		case 'g':
			no_gui = TRUE;
			break;

		case 'c':
			city = optarg;
			break;

		case 'C':
			city_id = optarg;
			break;

		case 't':
			is_tray_mode = TRUE;
			return 0;

		case 'u':
			g_print(_("Updating cache...\n"));
			if (update_cache() != 0)
				g_print(_("Update cache failed\n"));
			else
				g_print(_("Update cache complete\n"));
			return EXIT_SUCCESS;

		case 'v':
			show_version();
			return EXIT_SUCCESS;

		case 'h':
			show_help();
			return EXIT_SUCCESS;
		}
	}

	g_type_init();

	sett = w_settings_new();
	if (sett == NULL)
	{
		g_error("w_settings_new failed\n");
	}

	/* if city_id and city both specifies */
	/* only city_id will check */
	if (city_id)
	{
		target_city_id = city_id;
	}
	
	else if(city)
	{
		gchar *cid;
		cid = get_city_id(city);
		if (cid == 0)
		{
			g_print(_("No city found: %s\n"), city);
			g_object_unref(sett);

			return EXIT_SUCCESS;
		}
		else
		{
			need_free_cityid = TRUE;
			target_city_id = cid;
		}
	}
	else if(no_gui)
	{
		city_id_list = w_settings_get_city_id_list(sett);
	}

	if (city_id_list)
	{
		status = list_city_weather(city_id_list, sett);
		g_list_free_full(city_id_list, g_free);
	}
	else
	{
		status = get_city_weather(target_city_id);
	}

	if (need_free_cityid)
		g_free(target_city_id);

	g_object_unref(sett);

	return status;
}

static void show_version()
{
	g_print(_("%s version %s\n"), PACKAGE, VERSION);
}

static void show_help()
{
	g_print(_("Usage: %s [ options ]\n\n"
			"Options:\n"
			"  -g, --no-gui\t\tshow weather information without gui\n"
			"  -c, --city\t\tspecify city name to show weather\n"
			"  -C, --city-id\t\tspecify city id to show weather\n"
			"  -t, --tray\t\tshow tray only when launch app\n"
			"  -u, --update-cache\tupdate local cache\n"
			"  -v, --version\t\tshow version\n"
			"  -h, --help\t\tshow help\n"
			),
				PACKAGE_NAME
				);
}

static gint update_cache()
{
	gchar *db_file = NULL;
	gint ret;

	db_file = g_strdup_printf("%s/%s/data/", g_get_user_config_dir(), PACKAGE_NAME);
	g_return_val_if_fail(db_file != NULL, -1);

	if (!g_file_test(db_file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
	{
		if (g_mkdir_with_parents(db_file, 0744) != 0)
		{
			g_free(db_file);
			return -1;
		}
	}

	db_file = g_strdup_printf("%s/%s/data/cities.db", g_get_user_config_dir(), PACKAGE_NAME);
	g_return_val_if_fail(db_file != NULL, -1);

	ret = weather_get_city_db(db_file);

	return ret;
}

static gint	get_weather(const gchar *city_id, WeatherInfo *wi)
{
	gint status;
	wSession *ws;

	ws = weather_open();
	if (ws == NULL)
	{
		g_warning("weather_open failed\n");
		return EXIT_FAILURE;
	}

	wi->city_id = g_strdup(city_id);
	status = weather_get(ws, wi);

	weather_close(ws);

	return status;
}

static gchar* get_city_id(const gchar *city)
{
	gchar *sql;
	gchar *db_file;
	gchar *id = NULL;

	sql = g_strdup_printf("SELECT city_id FROM town WHERE tname='%s'", city);
	g_return_val_if_fail( sql != NULL, 0);

	db_file = g_strdup_printf("%s/%s/data/cities.db", g_get_user_config_dir(), PACKAGE_NAME);
	if (db_file == NULL)
	{
		g_free(sql);
		return 0;
	}

	sql_query(db_file, sql, query_city_id, &id);

	g_free(sql);
	g_free(db_file);

	return id;
}

static void	print_weather_info(WeatherInfo *wi)
{
	if (wi == NULL)
		return;

	g_print(_("City: %s\n"
			"Weather: %s\n"
			"Temperature: %s ( %s  )\n"
			"Wind: %s\n"),
				wi->city,
				wi->weather[0].weather,
				wi->temp,
				wi->weather[0].temperature,
				wi->weather[0].wind
		   );
}

static void	query_city_id(gpointer data, const gchar **result, gint row, gint col)
{
	if (result == NULL || row == 0)
		return ;

	*((gchar **)data) = g_strdup(result[col]);
}

static void	signal_handler(int signum)
{
	if (main_window != NULL) /* quit so that it can save works */
	{
		weather_window_quit(WEATHER_WINDOW(main_window));
	}
}

static void	print_get_weather_fail_info(gint code)
{
	g_print(_("Unable to get weather information:\n%s\n"), curl_easy_strerror(code));
}

static gint	list_city_weather(GList *list, wSettings *sett)
{
	GList *p = list;
	GList *weather_list = NULL;
	gint status = EXIT_FAILURE;

	while(p)
	{
		WeatherInfo *info;

		info = weather_new_info();
		if (info == NULL)
		{
			g_error("weather_info_new failed\n");
		}

		status = get_weather(p->data, info);
		if (status != 0)
		{
			print_get_weather_fail_info(status);
			break;
		}

		print_weather_info(info);
		g_print("\n");
		weather_list = g_list_append(weather_list, info);

		p = p->next;

		status = EXIT_SUCCESS;
	}

	w_settings_set_weather(sett, weather_list);

	g_list_free_full(weather_list, (GDestroyNotify)weather_free_info);

	return status;
}

static gint	get_city_weather(const gchar *city_id)
{
	WeatherInfo *wi = NULL;
	gint status = EXIT_SUCCESS;

	wi = weather_new_info();
	if (wi == NULL)
	{
		g_print("weather_new_info failed (%s, %d)\n", __FILE__, __LINE__);
		return EXIT_FAILURE;
	}

	status = get_weather(city_id, wi);
	if (status != 0)
		print_get_weather_fail_info(status);
	else
		print_weather_info(wi);

	weather_free_info(wi);

	return status;
}
