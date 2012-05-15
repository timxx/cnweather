#include <sqlite3.h>

#include "common.h"

gint confirm_dialog(GtkWidget *parent, const gchar *msg, const gchar *title)
{
	GtkWidget *dialog;
	gint result;

	dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
				GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_YES_NO,
                msg,
				NULL);

	gtk_window_set_title(GTK_WINDOW(dialog), title);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	return result;
}

gint sql_query(const gchar *dbfile, const gchar *sql,
			void (*deal_func)(gpointer, const gchar **, gint, gint),
			gpointer data)
{
	sqlite3 *db;
	gchar **result;
	gint row, col;

	gint ret = -1;
	gchar *msg = NULL;

	if (deal_func == NULL)
		return -1;

	do
	{
		ret = sqlite3_open(dbfile, &db);
		if (ret != SQLITE_OK)
		{
			g_warning("sqlite3_open failed (%s, %d)\n", __FILE__, __LINE__);
			break;
		}

		ret = sqlite3_get_table(db, sql, &result, &row, &col, &msg);
		if (ret != SQLITE_OK)
		{
			g_warning("ret %d: %s (%s, %d)\n", ret, msg, __FILE__, __LINE__);
			break;
		}

		deal_func(data, (const gchar **)result, row, col);
	}
	while(0);

	if (msg) {
		sqlite3_free(msg);
	}

	if (result){
		sqlite3_free_table(result);
	}
	
	if (db) {
		sqlite3_close(db);
	}

	return ret;
}

gint get_image_number_from_uri(const gchar *uri)
{
	gchar *name;
	gint number = 0;

	name = g_path_get_basename(uri);
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

		number = g_strtod(digit, NULL);

		g_free(name);
	}

	return number;
}
