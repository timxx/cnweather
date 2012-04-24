#include <string.h>

#include "cnWeather.h"
#include "builder.h"
#include "config.h"

extern GtkWidget *main_window;

static gboolean is_empty_text(const gchar *text);
static void check_and_set_sys_tray();
static void do_about();

void on_button_preferences_clicked(GtkButton *button, gpointer data)
{
	weather_window_set_page(WEATHER_WINDOW(main_window), PAGE_PREFERENCES); 
}

void on_button_about_clicked(GtkButton *button, gpointer data)
{
	do_about();
}

void on_search_entry_activate(GtkEntry *entry, gpointer data)
{
	const gchar *text;

	if (gtk_entry_get_text_length(entry) == 0)
		return ;

	text = gtk_entry_get_text(entry);
	if (is_empty_text(text))
		return ;

	weather_window_search(WEATHER_WINDOW(main_window), text);
}

void on_search_entry_changed(GtkEditable *edit, gpointer data)
{
	gboolean has_text;

	has_text = gtk_entry_get_text_length(GTK_ENTRY(edit)) > 0;
	gtk_entry_set_icon_sensitive(GTK_ENTRY(edit),
				GTK_ENTRY_ICON_SECONDARY,
				has_text);
}

void on_search_entry_icon_press(GtkEntry *entry,
			gint position,
			GdkEventButton *event,
			gpointer data
			)
{
	if (position == GTK_ENTRY_ICON_SECONDARY)
		gtk_entry_set_text(entry, "");
}

static gboolean is_empty_text(const gchar *text)
{
	const gchar *p;

	if (text == NULL)
		return TRUE;

	p = text;

	while(p && *p)
	{
		if (*p != ' '){
			return TRUE;
		}
		p++;
	}

	return FALSE;
}

void on_pref_button_back_clicked(GtkButton *button, gpointer data)
{
	weather_window_set_page(WEATHER_WINDOW(main_window), PAGE_WEATHER);
}

void on_pref_button_update_cache_clicked(GtkButton *button, gpointer data)
{
}

void on_pref_cb_show_tray_toggled(GtkToggleButton *button, gpointer data)
{
	wSettings *sett;
	gboolean state;

	if (main_window == NULL)
		return ;
	
	state = gtk_toggle_button_get_active(button);
	sett = weather_window_get_settings(WEATHER_WINDOW(main_window));

	w_settings_set_show_tray(sett, state);
	if (state){
		check_and_set_sys_tray();
	}

	weather_window_show_tray(WEATHER_WINDOW(main_window), state);
}

static void check_and_set_sys_tray()
{
	GSettings *sett;
	gchar **values;
	gboolean need_to_set = TRUE;
	gint len;
	gint i;

	sett = g_settings_new("com.canonical.Unity.Panel");
	if (sett == NULL)
		return ;

	do
	{
		i = 0;

		values = g_settings_get_strv(sett, "systray-whitelist");
		if (values == NULL)
			break;

		len = strlen(PACKAGE_NAME);

		while(values[i] != NULL)
		{
			if (g_ascii_strncasecmp(values[i], "all", 3) == 0)
			{
				need_to_set = FALSE;
				break;
			}
			else if(g_ascii_strncasecmp(values[i], PACKAGE_NAME, len) == 0)
			{
				need_to_set = FALSE;
				break;
			}

			i++;
		}
	}
	while(0);

	if (need_to_set)
	{
		if (values == NULL)
			i = 0;
		else
			i = g_strv_length(values);

		values = (gchar **)g_realloc(values, (i + 2 ) * sizeof(gchar *));
		values[i] = g_strdup(PACKAGE_NAME);
		values[i + 1] = NULL;

		g_settings_set_strv(sett, "systray-whitelist", (const gchar *const *)values);
	}

	if (values)
		g_strfreev(values);

	g_object_unref(sett);
}

void on_menu_tray_about_activate(GtkMenuItem *menuitem, gpointer data)
{
	do_about();
}

void on_menu_tray_quit_activate(GtkMenuItem *menuitem, gpointer data)
{
	weather_window_quit(WEATHER_WINDOW(main_window));
}

static void do_about()
{
	GtkBuilder *builder = NULL;
	static GtkWidget *widget;

	if (widget == NULL)
	{
		builder = builder_new(UI_DIR"/about.ui");
		if (builder == NULL)
		{
			g_warning("Missing about.ui file!\n");
			return ;
		}

		widget = builder_get_widget(builder, "dialog_about");
		if (widget == NULL){
			g_warning("Missing about dialog!\n");
		}
		else{
			gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(main_window));
		}

		g_object_unref(builder);
	}

	if (widget == NULL)
		return ;

	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_hide(widget);
}
