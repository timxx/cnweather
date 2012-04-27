#ifndef __SETTINGS__
#define __SETTINGS__

#include <gio/gio.h>
#include "lib/weather.h"

#define W_TYPE_SETTINGS				(w_settings_get_type())
#define W_SETTINGS(inst)			(G_TYPE_CHECK_INSTANCE_CAST((inst),	W_TYPE_SETTINGS, wSettings))
#define W_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), W_TYPE_SETTINGS, wSettingsClass))
#define W_IS_SETTINGS(inst)         (G_TYPE_CHECK_INSTANCE_TYPE((inst), W_TYPE_SETTINGS))
#define W_IS_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), W_TYPE_SETTINGS))
#define W_SETTINGS_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS((inst), W_TYPE_SETTINGS, wSettingsClass))

typedef struct _wSettings wSettings;
typedef struct _wSettingsClass wSettingsClass;

struct _wSettings
{
	GSettings parent;
};

struct _wSettingsClass
{
	GSettingsClass parent_class;
};

wSettings*	w_settings_new();

gboolean	w_settings_get_show_tray(wSettings *sett);
gboolean	w_settings_set_show_tray(wSettings *sett, gboolean value);

gboolean	w_settings_get_weather(wSettings *sett, WeatherInfo *wi);
gboolean	w_settings_set_weather(wSettings *sett, WeatherInfo *wi);

gboolean	w_settings_get_window_size(wSettings *sett, gint *w, gint *h);
gboolean	w_settings_set_window_size(wSettings *sett, gint w, gint h);

gboolean	w_settings_get_window_pos(wSettings *sett, gint *x, gint *y);
gboolean	w_settings_set_window_pos(wSettings *sett, gint x, gint y);

gint		w_settings_get_duration(wSettings *sett);
gboolean	w_settings_set_duration(wSettings *sett, gint t);

gboolean	w_settings_get_window_state(wSettings *sett);
gboolean	w_settings_set_window_state(wSettings *sett, gboolean value);

gchar*		w_settings_get_theme(wSettings *sett);
gboolean	w_settings_set_theme(wSettings *sett, const gchar *theme);

#endif
