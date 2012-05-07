#include <string.h>

#include "cnWeather.h"
#include "builder.h"
#include "config.h"
#include "common.h"
#include "intl.h"

extern GtkWidget *main_window;

static gboolean is_empty_text(const gchar *text);
static void check_and_set_sys_tray();
static void do_about();

/**
 * preferences button clicked
 */
void on_button_preferences_clicked(GtkButton *button, gpointer data)
{
	weather_window_set_page(WEATHER_WINDOW(main_window), PAGE_PREFERENCES); 
}

/**
 * about button
 */
void on_button_about_clicked(GtkButton *button, gpointer data)
{
	do_about();
}

/**
 * User press Enter key on search entry
 */
void on_search_entry_activate(GtkEntry *entry, gpointer data)
{
	const gchar *text;

	if (gtk_entry_get_text_length(entry) == 0)
		return ;

	text = gtk_entry_get_text(entry);
	if (is_empty_text(text))
		return ;

	weather_window_hide_result_tv(WEATHER_WINDOW(main_window));
	weather_window_search(WEATHER_WINDOW(main_window), text);
}

/**
 * search entry text changed
 */
void on_search_entry_changed(GtkEditable *edit, gpointer data)
{
	gboolean has_text;

	has_text = gtk_entry_get_text_length(GTK_ENTRY(edit)) > 0;
	gtk_entry_set_icon_sensitive(GTK_ENTRY(edit),
				GTK_ENTRY_ICON_SECONDARY,
				has_text);
}

/**
 * search entry
 */
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
			return FALSE;
		}
		p++;
	}

	return TRUE;
}

/**
 * back button on preferences page
 */
void on_pref_button_back_clicked(GtkButton *button, gpointer data)
{
	weather_window_set_page(WEATHER_WINDOW(main_window), PAGE_WEATHER);
}

/**
 * search result page
 */
void on_search_button_back_clicked(GtkButton *button, gpointer data)
{
	weather_window_set_page(WEATHER_WINDOW(main_window), PAGE_WEATHER);
}

/**
 * preferences->update local cache
 */
void on_pref_button_update_cache_clicked(GtkButton *button, gpointer data)
{
	gint result;

	result = confirm_dialog(main_window, 
				_("Update will delete all the old cache! Really want to update?"), 
				_("Attention!"));

	if (result == GTK_RESPONSE_YES)
		weather_window_update_cache(WEATHER_WINDOW(main_window));
}

/**
 * preferences->show tray
 */
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

/**
 * add self to panel white list if
 * there isn't
 */
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
		else
		{
			gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(main_window));
			gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(widget), VERSION);
		}

		g_object_unref(builder);
	}

	if (widget == NULL)
		return ;

	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_hide(widget);
}

void on_pref_cb_province_changed(GtkComboBox *cb, gpointer data)
{
	gchar *province;

	if (main_window == NULL)
		return ;

	province = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cb));
	if (province != NULL)
		weather_window_update_pref_cb(WEATHER_WINDOW(main_window), CB_CITY, province);
}

void on_pref_cb_city_changed(GtkComboBox *cb, gpointer data)
{
	gchar *city;

	if (main_window == NULL)
		return ;

	city = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cb));
	if (city != NULL)
		weather_window_update_pref_cb(WEATHER_WINDOW(main_window), CB_TOWN, city);
}

void on_pref_cb_town_changed(GtkComboBox *cb, gpointer data)
{
	const gchar *text;

	if (main_window == NULL)
		return ;

	text = gtk_combo_box_get_active_id(cb);
	if (text != NULL)
	{
		guint id = g_strtod(text, NULL);
		if (id == weather_window_get_current_city_id(WEATHER_WINDOW(main_window)))
		{
			return ;
		}

		weather_window_get_weather(WEATHER_WINDOW(main_window), id);
	}
}

void on_pref_sp_duration_value_changed(GtkSpinButton *sb, gpointer data)
{
	gint duration;
	
	if (main_window == NULL)
		return ;

	duration = gtk_spin_button_get_value_as_int(sb);
	weather_window_set_duration(WEATHER_WINDOW(main_window), duration);
}

void on_tv_result_row_activated(GtkTreeView *tv,
			GtkTreePath       *path,
			GtkTreeViewColumn *column,
			gpointer           data)
{
	GtkTreeModel *model;
    GtkTreeIter   iter;
 
    model = gtk_tree_view_get_model(tv);
 
    if (gtk_tree_model_get_iter(model, &iter, path))
    {
       gchar *text;
	   gchar **names;

       gtk_tree_model_get(model, &iter, 0, &text, -1);

	   names = g_strsplit(text, " ", -1);
	   if (names)
	   {
		   weather_window_update_pref_cb_by_town(WEATHER_WINDOW(main_window), names[2]);
		   weather_window_set_page(WEATHER_WINDOW(main_window), PAGE_WEATHER);

		   g_strfreev(names);
	   }
 
       g_free(text);
    }
}

void on_pref_cb_themes_changed(GtkComboBox *cb, gpointer data)
{
	const gchar *theme;

	if (main_window == NULL)
		return ;

	theme = gtk_combo_box_get_active_id(cb);
	if (theme != NULL)
	{
		weather_window_set_theme(WEATHER_WINDOW(main_window), theme);
	}
}
