#include "wsettings.h"

static const gchar *_names[] =
{
	"weather", "wind", "temp", "img"
};

static void w_settings_init(wSettings *sett);
static void w_settings_class_init(wSettingsClass *klass);

GType w_settings_get_type()
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
        {
            sizeof(wSettingsClass),				        // class size
            NULL,									    // base init
            NULL,									    // base finalize
            (GClassInitFunc)w_settings_class_init,  // class init
            NULL,									    // class finalize
            NULL,									    // data
            sizeof(wSettings),					        // instance size
            0,										    // prealloc
            (GInstanceInitFunc)w_settings_init	    // instance init
        };

        type = g_type_register_static(G_TYPE_SETTINGS, "wSettings", &info, 0);
    }

    return type;
}

wSettings* w_settings_new()
{
    return W_SETTINGS(g_object_new(W_TYPE_SETTINGS,
					"schema-id", "com.timxx.cnweather",
					NULL));
}

static void w_settings_init(wSettings *sett)
{

}

static void w_settings_class_init(wSettingsClass *klass)
{
}

gboolean w_settings_get_show_tray(wSettings *sett)
{
	g_return_val_if_fail(sett != NULL, TRUE);

	return g_settings_get_boolean(G_SETTINGS(sett), "tray");
}

gboolean w_settings_set_show_tray(wSettings *sett, gboolean value)
{
	g_return_val_if_fail(sett != NULL, FALSE);

	return g_settings_set_boolean(G_SETTINGS(sett), "tray", value);
}

gboolean w_settings_get_weather(wSettings *sett, GList **list)
{
	gchar **city_id;
	gchar **city;
	gchar **weather[3];
	gchar **wind[3];
	gchar **temp[3];
	gchar **img[3];

	gchar name[10];
	int i;

	g_return_val_if_fail(sett != NULL && list != NULL, FALSE);

	city_id = g_settings_get_strv(G_SETTINGS(sett), "city-id");
	if (city_id == NULL)
		return TRUE;

	city = g_settings_get_strv(G_SETTINGS(sett), "city");
	if (city == NULL)
	{
		g_strfreev(city_id);
		return TRUE;
	}

	for(i=0; i<3; ++i)
	{
		g_snprintf(name, 10, "%s%d", _names[0], i + 1);
		weather[i] = g_settings_get_strv(G_SETTINGS(sett), name);

		g_snprintf(name, 10, "%s%d", _names[1], i + 1);
		wind[i] = g_settings_get_strv(G_SETTINGS(sett), name);

		g_snprintf(name, 10, "%s%d", _names[2], i + 1);
		temp[i] = g_settings_get_strv(G_SETTINGS(sett), name);

		g_snprintf(name, 10, "%s%d", _names[3], i + 1);
		img[i] = g_settings_get_strv(G_SETTINGS(sett), name);
	}

	i = 0;

	while(city_id[i])
	{
		WeatherInfo *wi;
		gint j;
		
		wi = weather_new_info();
		if (wi == NULL)
			g_error("weather_new_info failed (%s, %d)\n", __FILE__, __LINE__);

		wi->city_id = city_id[i];
		wi->city = city[i];

		for(j=0; j<3; ++j)
		{
			wi->weather[j].temperature = temp[j][i];
			wi->weather[j].wind = wind[j][i];
			wi->weather[j].weather = weather[j][i];

			wi->weather[j].img = g_strtod(img[j][i], NULL); 
		}

		*list = g_list_append(*list, wi);

		i++;
	}

	for(i=0; i<3; ++i)
		g_strfreev(img[i]);

	return TRUE;
}

gboolean w_settings_set_weather(wSettings *sett, GList *list)
{
	gchar name[10];
	gint i, count;
	GList *p;

	gchar **city_id = NULL;
	gchar **city = NULL;
	gchar **temp[3] = {NULL};
	gchar **wind[3] = {NULL};
	gchar **weather[3] = {NULL};
	gchar **img[3] = {NULL};

	g_return_val_if_fail(sett != NULL && list != NULL, FALSE);

	count = g_list_length(list);
	if (count > 0)
	{
		count = (count + 1) * sizeof(gchar *);
	
		city_id = (gchar **)g_malloc0(count);
		if (city_id == NULL)
			return FALSE;

		city	= (gchar **)g_malloc0(count);

		for(i=0; i<3; ++i)
		{
			temp[i]		= (gchar **)g_malloc0(count);
			wind[i]		= (gchar **)g_malloc0(count);
			weather[i]	= (gchar **)g_malloc0(count);
			img[i]		= (gchar **)g_malloc0(count);
		}
	}

	p = list;
	i = 0;
	while (p && city_id != NULL)
	{
		WeatherInfo *wi = (WeatherInfo *)p->data;
		gint j;

		city_id[i] = wi->city_id;
		city[i] = wi->city;

		for(j=0; j<3; ++j)
		{
			temp[j][i] = wi->weather[j].temperature;
			wind[j][i] = wi->weather[j].wind;
			weather[j][i] = wi->weather[j].weather;

			img[j][i] = g_strdup_printf("%d", wi->weather[j].img);
		}

		p = p->next;
		i++;
	}

	g_settings_set_strv(G_SETTINGS(sett), "city", (const gchar * const*)city);
	g_settings_set_strv(G_SETTINGS(sett), "city-id", (const gchar * const*)city_id);

	for(i=0; i<3; ++i)
	{
		g_snprintf(name, 10, "%s%d", _names[0], i + 1);
		g_settings_set_strv(G_SETTINGS(sett), name, (const gchar * const*)weather[i]);

		g_snprintf(name, 10, "%s%d", _names[1], i + 1);
		g_settings_set_strv(G_SETTINGS(sett), name, (const gchar * const*)wind[i]);

		g_snprintf(name, 10, "%s%d", _names[2], i + 1);
		g_settings_set_strv(G_SETTINGS(sett), name, (const gchar * const*)temp[i]);

		g_snprintf(name, 10, "%s%d", _names[3], i + 1);
		g_settings_set_strv(G_SETTINGS(sett), name, (const gchar * const*)img[i]);
	}

	if (city_id != NULL)
	{
		g_free(city_id);
		g_free(city);

		for(i=0; i<3; ++i)
		{
			g_free(weather[i]);
			g_free(wind[i]);
			g_free(temp[i]);
			g_strfreev(img[i]);
		}
	}

	return TRUE;
}

gboolean w_settings_get_window_size(wSettings *sett, gint *w, gint *h)
{
	g_return_val_if_fail( sett != NULL && w != NULL && h != NULL, FALSE);

	g_settings_get(G_SETTINGS(sett), "window-size", "(ii)", w, h);

	return TRUE;
}

gboolean w_settings_set_window_size(wSettings *sett, gint w, gint h)
{
	g_return_val_if_fail( sett != NULL, FALSE);

	return g_settings_set(G_SETTINGS(sett), "window-size", "(ii)", w, h);
}

gboolean w_settings_get_window_pos(wSettings *sett, gint *x, gint *y)
{
	g_return_val_if_fail( sett != NULL && x != NULL && y != NULL, FALSE);

	g_settings_get(G_SETTINGS(sett), "window-pos", "(ii)", x, y);

	return TRUE;
}

gboolean w_settings_set_window_pos(wSettings *sett, gint x, gint y)
{
	g_return_val_if_fail( sett != NULL, FALSE);

	return g_settings_set(G_SETTINGS(sett), "window-pos", "(ii)", x, y);
}

gint w_settings_get_duration(wSettings *sett)
{
	g_return_val_if_fail( sett != NULL, 0);

	return g_settings_get_int(G_SETTINGS(sett), "duration");
}

gboolean w_settings_set_duration(wSettings *sett, gint t)
{
	g_return_val_if_fail( sett != NULL, FALSE);

	return g_settings_set_int(G_SETTINGS(sett), "duration", t);
}

gboolean w_settings_get_window_state(wSettings *sett)
{
	g_return_val_if_fail(sett != NULL, FALSE);

	return g_settings_get_boolean(G_SETTINGS(sett), "window-maximized");
}

gboolean w_settings_set_window_state(wSettings *sett, gboolean value)
{
	g_return_val_if_fail(sett != NULL, FALSE);

	return g_settings_set_boolean(G_SETTINGS(sett), "window-maximized", value);
}

gchar* w_settings_get_theme(wSettings *sett)
{
	g_return_val_if_fail(sett != NULL, NULL);

	return g_settings_get_string(G_SETTINGS(sett), "theme");
}

gboolean w_settings_set_theme(wSettings *sett, const gchar *theme)
{
	g_return_val_if_fail(sett != NULL, FALSE);

	return g_settings_set_string(G_SETTINGS(sett), "theme", theme);
}

gint w_settings_get_city_id(wSettings *sett)
{
	g_return_val_if_fail(sett != NULL, 0);

	return g_settings_get_int(G_SETTINGS(sett), "city-id");
}
