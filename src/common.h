#ifndef __CONFIRM_DIALOG_H__
#define __CONFIRM_DIALOG_H__

#include <gtk/gtk.h>

gint confirm_dialog(GtkWidget *parent, const gchar *msg, const gchar *title);

gint sql_query(const gchar *dbfile, const gchar *sql,
			void (*deal_func)(gpointer, const gchar **, gint, gint),
			gpointer data);

gint get_image_number_from_uri(const gchar *uri);

/**
 * check if enable auto start
 */
gboolean
check_auto_start();

gboolean
set_auto_start(gboolean auto_start);


/**
 * get full city name from city id
 * format: PROVINCE>CITY>TOWN
 */
gchar *
get_full_city(const gchar *db_file, const gchar *city_id);

#endif /* __CONFIRM_DIALOG_H__ */
