#include <sqlite3.h>
#include <glib/gstdio.h>
#include <string.h>

#include "common.h"
#include "config.h"

static const gchar *desktop_entry =
{
	"[Desktop Entry]\n"
	"Type=Application\n"
	"Exec="PACKAGE_NAME" -t\n"
	"Hidden=false\n"
	"NoDisplay=false\n"
	"X-GNOME-Autostart-enabled=true\n"
	"Name=cnWeather\n"
	"Comment=China Weather\n"
};

static gchar* desktop_entry_file();
static void _get_full_city(gpointer data, const gchar **result, gint row, gint col);

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
	sqlite3 *db = NULL;
	gchar **result = NULL;
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

gboolean check_auto_start()
{
	gchar *file;
	gboolean auto_start = FALSE;

	file = desktop_entry_file();
	if (file == NULL)
		return FALSE;

	do
	{
		FILE *fp;

		// file not exists
		if (!g_file_test(file, G_FILE_TEST_EXISTS))
			break;

		fp = fopen(file, "r");
		if (fp == NULL)
			break;

		do
		{
			gchar line[128];
			gchar **strv;
			gchar *p;
			
			p = fgets(line, 128, fp);
			if (p == NULL)
				break;

			strv = g_strsplit(line, "=", 0);
			if (strv == NULL)
				continue;

			if (g_strcmp0(strv[0], "X-GNOME-Autostart-enabled") == 0)
			{
				if (g_strcmp0(strv[1], "false\n") == 0)
					auto_start = FALSE;
				else
					auto_start = TRUE;

				g_strfreev(strv);
				break;
			}
			g_strfreev(strv);
		}
		while(!feof(fp));

		fclose(fp);
	}
	while(0);

	g_free(file);

	return auto_start;
}

gboolean set_auto_start(gboolean auto_start)
{
	gchar *file;
	gboolean result = FALSE;

	file = desktop_entry_file();
	if (file == NULL)
		return FALSE;

	do
	{
		if (!auto_start)
		{
			// no need to change
			if (!g_file_test(file, G_FILE_TEST_EXISTS))
			{
				result = TRUE;
				break;
			}

			// just delete it
			result = (g_remove(file) == 0);
		}
		else
		{
			FILE *fp;
			gchar *dir;

			dir = g_path_get_dirname(file);
			if (dir == NULL)
				break;

			g_mkdir_with_parents(dir, 0766);
			g_free(dir);

			fp = fopen(file, "wb+");
			if (fp == NULL)
				break;

			fwrite(desktop_entry, sizeof(gchar), strlen(desktop_entry), fp);

			fclose(fp);

			result = TRUE;
		}
	}
	while(0);

	return result;
}

static gchar* desktop_entry_file()
{
	return g_strdup_printf("%s/autostart/%s.desktop",
				g_get_user_config_dir(),
				PACKAGE_NAME);
}

gchar *
get_full_city(const gchar *db_file, const gchar *city_id)
{
	gchar *city = NULL;
	gchar *sql;

	sql = g_strdup_printf("SELECT pname, cname, tname FROM province p, "
				"city c, town t WHERE t.city_id='%s' AND "
				"c.cid=t.cid AND p.pid=c.pid",
				city_id);
	if (sql == NULL)
		return NULL;

	sql_query(db_file, sql, _get_full_city, &city);

	g_free(sql);

	return city;
}

static void _get_full_city(gpointer data, const gchar **result, gint row, gint col)
{
	const gchar *p, *c, *t;

	if (result == NULL || row == 0)
		return ;

	p = result[col];
	c = result[col + 1];
	t = result[col + 2];

	if (g_strcmp0(t, c) == 0 &&
		g_strcmp0(t, p) == 0
	   )
	{
		*((gchar **)data) = g_strdup_printf("%s", t);
	}
	else if(g_strcmp0(p, c) == 0)
	{
		*((gchar **)data) = g_strdup_printf("%s>%s", c, t);
	}
	else if(g_strcmp0(c, t) == 0)
	{
		*((gchar **)data) = g_strdup_printf("%s>%s", p, c);
	}
	else
	{
		*((gchar **)data) = g_strdup_printf("%s>%s>%s", p, c, t);
	}
}
