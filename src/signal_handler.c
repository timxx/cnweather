#include "cnWeather.h"
#include "builder.h"
#include "config.h"

extern GtkWidget *main_window;

static gboolean is_empty_text(const gchar *text);

void on_button_preferences_clicked(GtkButton *button, gpointer data)
{
	weather_window_set_page(WEATHER_WINDOW(main_window), PAGE_PREFERENCES); 
}

void on_button_about_clicked(GtkButton *button, gpointer data)
{
	GtkBuilder *builder = NULL;
	static GtkWidget *widget;

	if (widget == NULL)
	{
		builder = builder_new(UI_DIR"/about.ui");
		if (builder == NULL)
		{
			g_warning("Missing about.glade ui file!\n");
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

	g_print("state: %d | old: %d\n", state, w_settings_get_show_tray(sett));

	w_settings_set_show_tray(sett, state);
}
