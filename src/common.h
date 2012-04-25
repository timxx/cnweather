#ifndef __CONFIRM_DIALOG_H__
#define __CONFIRM_DIALOG_H__

#include <gtk/gtk.h>

gint confirm_dialog(GtkWidget *parent, const gchar *msg, const gchar *title);

gint sql_query(const gchar *dbfile, const gchar *sql,
			void (*deal_func)(gpointer, const gchar **, gint, gint),
			gpointer data);

#endif /* __CONFIRM_DIALOG_H__ */
