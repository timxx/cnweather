
#include "builder.h"
#include "config.h"

GtkBuilder* builder_new(const gchar *file)
{
	GtkBuilder *builder = NULL;

	builder = gtk_builder_new();
	g_return_val_if_fail(builder != NULL, NULL);

	gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);
	gtk_builder_add_from_file(builder, file, NULL);
	gtk_builder_connect_signals(builder, NULL);

	return builder;
}

GtkWidget* builder_get_widget(GtkBuilder *builder, const gchar *name)
{
	g_return_val_if_fail(name != NULL, NULL);

	g_return_val_if_fail(builder != NULL, NULL);

	return GTK_WIDGET(gtk_builder_get_object(builder, name));
}
