#include "wsettings.h"

static const gchar *_names[] =
{
	"weather", "wind", "temp"
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

gboolean w_settings_get_weather(wSettings *sett, WeatherInfo *wi)
{
	gchar name[10];
	int i;

	g_return_val_if_fail(sett != NULL && wi != NULL, FALSE);

	wi->city_id = g_settings_get_int(G_SETTINGS(sett), "city-id");
	wi->city = g_settings_get_string(G_SETTINGS(sett), "city");

	for(i=0; i<3; ++i)
	{
		g_snprintf(name, 10, "%s%d", _names[0], i + 1);
		wi->weather[i].weather = g_settings_get_string(G_SETTINGS(sett), name);

		g_snprintf(name, 10, "%s%d", _names[1], i + 1);
		wi->weather[i].wind = g_settings_get_string(G_SETTINGS(sett), name);

		g_snprintf(name, 10, "%s%d", _names[2], i + 1);
		wi->weather[i].temperature = g_settings_get_string(G_SETTINGS(sett), name);
	}

	return TRUE;
}

gboolean w_settings_set_weather(wSettings *sett, WeatherInfo *wi)
{
	gchar name[10];
	int i;

	g_return_val_if_fail(sett != NULL && wi != NULL, FALSE);

	g_settings_set_string(G_SETTINGS(sett), "city", wi->city);
	g_settings_set_int(G_SETTINGS(sett), "city-id", wi->city_id);

	for(i=0; i<3; ++i)
	{
		g_snprintf(name, 10, "%s%d", _names[0], i + 1);
		g_settings_set_string(G_SETTINGS(sett), name, wi->weather[i].weather);

		g_snprintf(name, 10, "%s%d", _names[1], i + 1);
		g_settings_set_string(G_SETTINGS(sett), name, wi->weather[i].wind);

		g_snprintf(name, 10, "%s%d", _names[2], i + 1);
		g_settings_set_string(G_SETTINGS(sett), name, wi->weather[i].temperature);
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

